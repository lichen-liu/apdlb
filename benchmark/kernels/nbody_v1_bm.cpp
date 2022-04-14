#include "kbm_utils.hpp"

#define EPISLON 0.001
#define SQRT_NUM_ITERATION 20

void compute(int n_body, int n_iter, float dt, float *mass, float *pos, float *vel)
{
    // Allocate temporary buffers
    float *acc_x_buf = new float[n_body * 2];
    float *acc_y_buf = new float[n_body * 2];
    float *acc_z_buf = new float[n_body * 2];

    float *pos_x_buf = new float[n_body * 2];
    float *pos_y_buf = new float[n_body * 2];
    float *pos_z_buf = new float[n_body * 2];

    float *vel_x_buf = new float[n_body * 2];
    float *vel_y_buf = new float[n_body * 2];
    float *vel_z_buf = new float[n_body * 2];

    float *vel_tmp_x_buf = new float[n_body];
    float *vel_tmp_y_buf = new float[n_body];
    float *vel_tmp_z_buf = new float[n_body];

    // Step 0: Prepare buff
    for (int i_target_body = 0; i_target_body < n_body; i_target_body++)
    {
        pos_x_buf[i_target_body] = pos[i_target_body + i_target_body + i_target_body];
        pos_y_buf[i_target_body] = pos[i_target_body + i_target_body + i_target_body + 1];
        pos_z_buf[i_target_body] = pos[i_target_body + i_target_body + i_target_body + 2];
        vel_x_buf[i_target_body] = vel[i_target_body + i_target_body + i_target_body];
        vel_y_buf[i_target_body] = vel[i_target_body + i_target_body + i_target_body + 1];
        vel_z_buf[i_target_body] = vel[i_target_body + i_target_body + i_target_body + 2];
    }
    // Step 1: Prepare acceleration for ic
    for (int i_target_body = 0; i_target_body < n_body; i_target_body++)
    {
        acc_x_buf[i_target_body] = 0;
        acc_y_buf[i_target_body] = 0;
        acc_z_buf[i_target_body] = 0;

        for (int j_source_body = 0; j_source_body < n_body; j_source_body++)
        {
            if (i_target_body != j_source_body)
            {
                const float x_displacement = pos_x_buf[j_source_body] - pos_x_buf[i_target_body];
                const float y_displacement = pos_y_buf[j_source_body] - pos_y_buf[i_target_body];
                const float z_displacement = pos_z_buf[j_source_body] - pos_z_buf[i_target_body];

                const float denom_base =
                    x_displacement * x_displacement +
                    y_displacement * y_displacement +
                    z_displacement * z_displacement +
                    EPISLON * EPISLON;

                float inverse_sqrt_denom_base = 0;
                {
                    float z = 1;
                    for (int i = 0; i < SQRT_NUM_ITERATION; i++)
                    {
                        z -= (z * z - denom_base) / (2 * z);
                    }
                    inverse_sqrt_denom_base = 1 / z;
                }
                acc_x_buf[i_target_body] += mass[j_source_body] * x_displacement / denom_base * inverse_sqrt_denom_base;
                acc_y_buf[i_target_body] += mass[j_source_body] * y_displacement / denom_base * inverse_sqrt_denom_base;
                acc_z_buf[i_target_body] += mass[j_source_body] * z_displacement / denom_base * inverse_sqrt_denom_base;
            }
        }
    }
    // Core iteration loop
    for (int i_iter = 0; i_iter < n_iter; i_iter++)
    {
        const int input_buf_offset = (i_iter % 2) * n_body;
        const int output_buf_offset = (1 - i_iter % 2) * n_body;

        for (int i_target_body = 0; i_target_body < n_body; i_target_body++)
        {
            // Step 3: Compute temp velocity
            vel_tmp_x_buf[i_target_body] =
                vel_x_buf[input_buf_offset + i_target_body] +
                0.5 * acc_x_buf[input_buf_offset + i_target_body] * dt;
            vel_tmp_y_buf[i_target_body] =
                vel_y_buf[input_buf_offset + i_target_body] +
                0.5 * acc_y_buf[input_buf_offset + i_target_body] * dt;
            vel_tmp_z_buf[i_target_body] =
                vel_z_buf[input_buf_offset + i_target_body] +
                0.5 * acc_z_buf[input_buf_offset + i_target_body] * dt;

            // Step 4: Update position
            pos_x_buf[output_buf_offset + i_target_body] =
                pos_x_buf[input_buf_offset + i_target_body] +
                vel_x_buf[input_buf_offset + i_target_body] * dt +
                0.5 * acc_x_buf[input_buf_offset + i_target_body] * dt * dt;
            pos_y_buf[output_buf_offset + i_target_body] =
                pos_y_buf[input_buf_offset + i_target_body] +
                vel_y_buf[input_buf_offset + i_target_body] * dt +
                0.5 * acc_y_buf[input_buf_offset + i_target_body] * dt * dt;
            pos_z_buf[output_buf_offset + i_target_body] =
                pos_z_buf[input_buf_offset + i_target_body] +
                vel_z_buf[input_buf_offset + i_target_body] * dt +
                0.5 * acc_z_buf[input_buf_offset + i_target_body] * dt * dt;

            for (int i_target_body = 0; i_target_body < n_body; i_target_body++)
            {
                acc_x_buf[output_buf_offset + i_target_body] = 0;
                acc_y_buf[output_buf_offset + i_target_body] = 0;
                acc_z_buf[output_buf_offset + i_target_body] = 0;

                // Step 5: Compute acceleration
                for (int j_source_body = 0; j_source_body < n_body; j_source_body++)
                {
                    if (i_target_body != j_source_body)
                    {
                        const float x_displacement =
                            pos_x_buf[output_buf_offset + j_source_body] -
                            pos_x_buf[output_buf_offset + i_target_body];
                        const float y_displacement =
                            pos_y_buf[output_buf_offset + j_source_body] -
                            pos_y_buf[output_buf_offset + i_target_body];
                        const float z_displacement =
                            pos_z_buf[output_buf_offset + j_source_body] -
                            pos_z_buf[output_buf_offset + i_target_body];

                        const float denom_base =
                            x_displacement * x_displacement +
                            y_displacement * y_displacement +
                            z_displacement * z_displacement +
                            EPISLON * EPISLON;

                        float inverse_sqrt_denom_base = 0;
                        {
                            float z = 1;
                            for (int i = 0; i < SQRT_NUM_ITERATION; i++)
                            {
                                z -= (z * z - denom_base) / (2 * z);
                            }
                            inverse_sqrt_denom_base = 1 / z;
                        }
                        acc_x_buf[output_buf_offset + i_target_body] += mass[j_source_body] * x_displacement / denom_base * inverse_sqrt_denom_base;
                        acc_y_buf[output_buf_offset + i_target_body] += mass[j_source_body] * y_displacement / denom_base * inverse_sqrt_denom_base;
                        acc_z_buf[output_buf_offset + i_target_body] += mass[j_source_body] * z_displacement / denom_base * inverse_sqrt_denom_base;
                    }
                }

                // Step 6: Update velocity
                vel_x_buf[output_buf_offset + i_target_body] =
                    vel_tmp_x_buf[i_target_body] +
                    0.5 * acc_x_buf[output_buf_offset + i_target_body] * dt;

                vel_y_buf[output_buf_offset + i_target_body] =
                    vel_tmp_y_buf[i_target_body] +
                    0.5 * acc_y_buf[output_buf_offset + i_target_body] * dt;

                vel_z_buf[output_buf_offset + i_target_body] =
                    vel_tmp_z_buf[i_target_body] +
                    0.5 * acc_z_buf[output_buf_offset + i_target_body] * dt;
            }
        }
    }
    {
        // Set result
        const int input_buf_offset = (n_iter % 2) * n_body;
        for (int i_target_body = 0; i_target_body < n_body; i_target_body++)
        {
            pos[i_target_body + i_target_body + i_target_body] = pos_x_buf[input_buf_offset + i_target_body];
            pos[i_target_body + i_target_body + i_target_body + 1] = pos_y_buf[input_buf_offset + i_target_body];
            pos[i_target_body + i_target_body + i_target_body + 2] = pos_z_buf[input_buf_offset + i_target_body];
            vel[i_target_body + i_target_body + i_target_body] = vel_x_buf[input_buf_offset + i_target_body];
            vel[i_target_body + i_target_body + i_target_body + 1] = vel_y_buf[input_buf_offset + i_target_body];
            vel[i_target_body + i_target_body + i_target_body + 2] = vel_z_buf[input_buf_offset + i_target_body];
        }
    }

    // Release temporary buffers
    delete[] acc_x_buf;
    delete[] acc_y_buf;
    delete[] acc_z_buf;

    delete[] pos_x_buf;
    delete[] pos_y_buf;
    delete[] pos_z_buf;

    delete[] vel_x_buf;
    delete[] vel_y_buf;
    delete[] vel_z_buf;

    delete[] vel_tmp_x_buf;
    delete[] vel_tmp_y_buf;
    delete[] vel_tmp_z_buf;
}

int main(int argc, char *argv[])
{
    const int n_body = 250;
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