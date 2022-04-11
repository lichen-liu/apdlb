#include "kbm_utils.hpp"
#include "wspdr_pool.hpp"

void sorting_kernel(size_t lower, size_t upper)
{
    ERT::WSPDR_POOL __apert_ert_pool(2);
    __apert_ert_pool.start();
    {
        const size_t scale = 50;
        const size_t offset = 1;
        const size_t range = upper - lower;
        float **vecs = new float *[range];
        {
            std::vector<ERT::RAW_TASK> __apert_ert_tasks;
            // ================ APERT ================
            for (size_t iteration = 0; iteration <= range - 1; iteration += 1)
            {
                auto __apert_ert_task = [=]()
                {
                    const size_t n = offset + (iteration + lower) * scale;
                    vecs[iteration] = (new float[n]);
                };
                __apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
            }
            __apert_ert_pool.execute(std::move(__apert_ert_tasks));
        }
        {
            std::vector<ERT::RAW_TASK> __apert_ert_tasks;
            // Main computation loop
            // ================ APERT ================
            for (size_t iteration = 0; iteration <= range - 1; iteration += 1)
            {
                auto __apert_ert_task = [=]()
                {
                    const size_t n = offset + (iteration + lower) * scale;
                    // Generate random data
                    int seed = static_cast<int>((iteration + lower));
                    for (size_t i = 0; i <= n - 1; i += 1)
                    {
                        seed = seed * 0x343fd + 0x269EC3;
                        // a=214013, b=2531011
                        float rand_val = (seed / 65536 & 0x7FFF);
                        vecs[iteration][i] = (static_cast<float>(rand_val)) / (static_cast<float>(2147483647));
                    }
                    // Bubble sort
                    for (long i = 0; i <= (static_cast<long>(n)) - ((long)1) - 1; i += 1)
                    {
                        // Last i elements are already in place
                        for (long j = 0; j <= (static_cast<long>(n)) - i - ((long)1) - 1; j += 1)
                        {
                            if (vecs[iteration][j] > vecs[iteration][j + 1])
                            {
                                const float temp = vecs[iteration][j];
                                vecs[iteration][j] = vecs[iteration][j + 1];
                                vecs[iteration][j + 1] = temp;
                            }
                        }
                    }
                };
                __apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
            }
            __apert_ert_pool.execute(std::move(__apert_ert_tasks));
        }
        {
            std::vector<ERT::RAW_TASK> __apert_ert_tasks;
            // ================ APERT ================
            for (size_t iteration = 0; iteration <= range - 1; iteration += 1)
            {
                auto __apert_ert_task = [=]()
                {
                    delete[] vecs[iteration];
                };
                __apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
            }
            __apert_ert_pool.execute(std::move(__apert_ert_tasks));
        }
        delete[] vecs;
    }
}

int main(int argc, char *argv[])
{
    const double start_time = get_time_stamp();
    sorting_kernel(0, 200);
    print_elapsed(argv[0], start_time);
}
