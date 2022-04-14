#include "kbm_utils.hpp"

#define EPISLON 0.001
#if 0

CORE::SYSTEM_STATE TP_ENGINE::execute(int n_iter, CORE::TIMER &timer)
{
    const size_t n_body = system_state_snapshot().size();

    std::vector<CORE::MASS> mass(n_body, 0);
    BUFFER buf_in(n_body);
    // Step 1: Prepare ic
    for (size_t i_body = 0; i_body < n_body; i_body++)
    {
        const auto &[body_pos, body_vel, body_mass] = system_state_snapshot()[i_body];
        buf_in.pos[i_body] = body_pos;
        buf_in.vel[i_body] = body_vel;
        mass[i_body] = body_mass;
    }
    timer.elapsed_previous("step1");

    // Step 2: Prepare acceleration for ic
    for (size_t i_target_body = 0; i_target_body < n_body; i_target_body++)
    {
        buf_in.acc[i_target_body].reset();
        for (size_t j_source_body = 0; j_source_body < n_body; j_source_body++)
        {
            if (i_target_body != j_source_body)
            {
                buf_in.acc[i_target_body] += CORE::ACC::from_gravity(buf_in.pos[j_source_body], mass[j_source_body], buf_in.pos[i_target_body]);
            }
        }
    }

    timer.elapsed_previous("step2");

    BUFFER buf_out(n_body);
    std::vector<CORE::VEL> vel_tmp(n_body);
    // Core iteration loop
    for (int i_iter = 0; i_iter < n_iter; i_iter++)
    {
        for (size_t i_target_body = 0; i_target_body < n_body; i_target_body++)
        {
            // Step 3: Compute temp velocity
            vel_tmp[i_target_body] =
                CORE::VEL::updated(buf_in.vel[i_target_body], buf_in.acc[i_target_body], dt());

            // Step 4: Update position
            buf_out.pos[i_target_body] =
                CORE::POS::updated(buf_in.pos[i_target_body], buf_in.vel[i_target_body], buf_in.acc[i_target_body], dt());
        }

        for (size_t i_target_body = 0; i_target_body < n_body; i_target_body++)
        {
            buf_out.acc[i_target_body].reset();
            // Step 5: Compute acceleration
            for (size_t j_source_body = 0; j_source_body < n_body; j_source_body++)
            {
                if (i_target_body != j_source_body)
                {
                    buf_out.acc[i_target_body] += CORE::ACC::from_gravity(buf_out.pos[j_source_body], mass[j_source_body], buf_out.pos[i_target_body]);
                }
            }

            // Step 6: Update velocity
            buf_out.vel[i_target_body] = CORE::VEL::updated(vel_tmp[i_target_body], buf_out.acc[i_target_body], dt());
        }
        // Prepare for next iteration
        std::swap(buf_in, buf_out);

        timer.elapsed_previous(std::string("iter") + std::to_string(i_iter), CORE::TIMER::TRIGGER_LEVEL::INFO);
    }

    timer.elapsed_previous("all_iters");

    return generate_system_state(buf_in, mass);
}

#endif

void compute(int n_body, int n_iter, float dt, float *mass, float *pos, float *vel)
{
    // Allocate temporary buffers
    float *in_acc_buf_holder = new float[3 * n_body];
    float *out_pos_buf_holder = new float[3 * n_body];
    float *out_vel_buf_holder = new float[3 * n_body];
    float *out_acc_buf_holder = new float[3 * n_body];
    float *vel_tmp_buf_holder = new float[3 * n_body];

    // Compute space buffer pointers
    float *in_pos = pos;
    float *in_vel = vel;
    float *in_acc = in_acc_buf_holder;
    float *out_pos = out_pos_buf_holder;
    float *out_vel = out_vel_buf_holder;
    float *out_acc = out_acc_buf_holder;
    float *vel_tmp = vel_tmp_buf_holder;

    // Step 1: Prepare acceleration for ic
    for (int i_target_body = 0; i_target_body < n_body; i_target_body++)
    {
        const int i_target_body_index = i_target_body * 3;
        in_acc[i_target_body_index] = 0;
        in_acc[i_target_body_index + 1] = 0;
        in_acc[i_target_body_index + 2] = 0;
        for (int j_source_body = 0; j_source_body < n_body; j_source_body++)
        {
            if (i_target_body != j_source_body)
            {
                const int j_source_body_index = j_source_body * 3;

                const float x_displacement = in_pos[j_source_body_index] - in_pos[i_target_body_index];
                const float y_displacement = in_pos[j_source_body_index + 1] - in_pos[i_target_body_index + 1];
                const float z_displacement = in_pos[j_source_body_index + 2] - in_pos[i_target_body_index + 2];

                const float denom_base =
                    x_displacement * x_displacement +
                    y_displacement * y_displacement +
                    z_displacement * z_displacement +
                    EPISLON * EPISLON;

                float inverse_sqrt_denom_base = 0;
                {
                    float z = 1;
                    for (int i = 0; i < 10; i++)
                    {
                        z -= (z * z - denom_base) / (2 * z);
                    }
                    inverse_sqrt_denom_base = 1 / z;
                }
                in_acc[i_target_body_index] += mass[j_source_body] * x_displacement / denom_base * inverse_sqrt_denom_base;
                in_acc[i_target_body_index + 1] += mass[j_source_body] * y_displacement / denom_base * inverse_sqrt_denom_base;
                in_acc[i_target_body_index + 2] += mass[j_source_body] * z_displacement / denom_base * inverse_sqrt_denom_base;
            }
        }
    }
    // Core iteration loop
    for (int i_iter = 0; i_iter < n_iter; i_iter++)
    {
        for (int i_target_body = 0; i_target_body < n_body; i_target_body++)
        {
            const int i_target_body_index = i_target_body * 3;
            // Step 3: Compute temp velocity
            vel_tmp[i_target_body_index] = in_vel[i_target_body_index] + 0.5 * in_acc[i_target_body_index] * dt;
            vel_tmp[i_target_body_index + 1] = in_vel[i_target_body_index + 1] + 0.5 * in_acc[i_target_body_index + 1] * dt;
            vel_tmp[i_target_body_index + 2] = in_vel[i_target_body_index + 2] + 0.5 * in_acc[i_target_body_index + 2] * dt;

            // Step 4: Update position
            out_pos[i_target_body_index] =
                in_pos[i_target_body_index] + in_vel[i_target_body_index] * dt + 0.5 * in_acc[i_target_body_index] * dt * dt;
            out_pos[i_target_body_index + 1] =
                in_pos[i_target_body_index + 1] + in_vel[i_target_body_index + 1] * dt + 0.5 * in_acc[i_target_body_index + 1] * dt * dt;
            out_pos[i_target_body_index + 2] =
                in_pos[i_target_body_index + 2] + in_vel[i_target_body_index + 2] * dt + 0.5 * in_acc[i_target_body_index + 2] * dt * dt;

            for (int i_target_body = 0; i_target_body < n_body; i_target_body++)
            {
                const int i_target_body_index = i_target_body * 3;
                out_acc[i_target_body_index] = 0;
                out_acc[i_target_body_index + 1] = 0;
                out_acc[i_target_body_index + 2] = 0;

                // Step 5: Compute acceleration
                for (int j_source_body = 0; j_source_body < n_body; j_source_body++)
                {
                    if (i_target_body != j_source_body)
                    {
                        const int j_source_body_index = j_source_body * 3;

                        const float x_displacement = out_pos[j_source_body_index] - out_pos[i_target_body_index];
                        const float y_displacement = out_pos[j_source_body_index + 1] - out_pos[i_target_body_index + 1];
                        const float z_displacement = out_pos[j_source_body_index + 2] - out_pos[i_target_body_index + 2];

                        const float denom_base =
                            x_displacement * x_displacement +
                            y_displacement * y_displacement +
                            z_displacement * z_displacement +
                            EPISLON * EPISLON;

                        float inverse_sqrt_denom_base = 0;
                        {
                            float z = 1;
                            for (int i = 0; i < 10; i++)
                            {
                                z -= (z * z - denom_base) / (2 * z);
                            }
                            inverse_sqrt_denom_base = 1 / z;
                        }
                        out_acc[i_target_body_index] += mass[j_source_body] * x_displacement / denom_base * inverse_sqrt_denom_base;
                        out_acc[i_target_body_index + 1] += mass[j_source_body] * y_displacement / denom_base * inverse_sqrt_denom_base;
                        out_acc[i_target_body_index + 2] += mass[j_source_body] * z_displacement / denom_base * inverse_sqrt_denom_base;
                    }
                }

                // Step 6: Update velocity
                out_vel[i_target_body_index] = vel_tmp[i_target_body_index] + 0.5 * out_acc[i_target_body_index] * dt;
                out_vel[i_target_body_index + 1] = vel_tmp[i_target_body_index + 1] + 0.5 * out_acc[i_target_body_index + 1] * dt;
                out_vel[i_target_body_index + 2] = vel_tmp[i_target_body_index + 2] + 0.5 * out_acc[i_target_body_index + 2] * dt;
            }
        }
        // Prepare for next iteration
        float *tmp_pos = in_pos;
        float *tmp_vel = in_vel;
        float *tmp_acc = in_acc;
        in_pos = out_pos;
        in_vel = out_vel;
        in_acc = out_acc;
        out_pos = tmp_pos;
        out_vel = tmp_vel;
        out_acc = tmp_acc;
    }
    if (in_pos != pos && in_vel != vel)
    {
        // Copy in_pos to pos, in_vel to vel
        for (int i_target = 0; i_target < n_body; i_target++)
        {
            const int i_target_index = i_target * 3;
            pos[i_target_index] = in_pos[i_target_index];
            pos[i_target_index + 1] = in_pos[i_target_index + 1];
            pos[i_target_index + 2] = in_pos[i_target_index + 2];
            vel[i_target_index] = in_vel[i_target_index];
            vel[i_target_index + 1] = in_vel[i_target_index + 1];
            vel[i_target_index + 2] = in_vel[i_target_index + 2];
        }
    }
    // Release temporary buffers
    delete[] vel_tmp_buf_holder;
    delete[] out_acc_buf_holder;
    delete[] out_vel_buf_holder;
    delete[] out_pos_buf_holder;
    delete[] in_acc_buf_holder;
}

int main(int argc, char *argv[])
{
    const int n_body = 200;
    const int n_iter = 2;
    const float dt = 0.01;
    float *mass = new float[n_body];
    float *pos = new float[n_body * 3];
    float *vel = new float[n_body * 3];

    const double start_time = get_time_stamp();
    compute(n_body, n_iter, dt, mass, pos, vel);
    print_elapsed(argv[0], start_time);

    delete[] vel;
    delete[] pos;
    delete[] mass;
}