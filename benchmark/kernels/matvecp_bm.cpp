#include "kbm_utils.hpp"

void matvecp_kernel(size_t lower, size_t upper)
{
    const size_t offset = 1;
    const size_t scale = 20;
    const size_t range = upper - lower;

    // Input mats
    float ***mats = new float **[range];
    for (size_t iteration = 0; iteration < range; iteration++)
    {
        const size_t n = offset + (iteration + lower) * scale;
        mats[iteration] = new float *[n];
        for (size_t i = 0; i < n; i++)
        {
            mats[iteration][i] = new float[n];
        }
    }

    // Input vecs
    float **vecs = new float *[range];
    for (size_t iteration = 0; iteration < range; iteration++)
    {
        const size_t n = offset + (iteration + lower) * scale;
        vecs[iteration] = new float[n];
    }

    // Output vecs
    float **ress = new float *[range];
    for (size_t iteration = 0; iteration < range; iteration++)
    {
        const size_t n = offset + (iteration + lower) * scale;
        ress[iteration] = new float[n];
    }

    // Main computation loop
    for (size_t iteration = lower; iteration < upper; iteration++)
    {
        const size_t n = offset + (iteration + lower) * scale;

        // Generate random data
        int seed = static_cast<int>(iteration);
        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = 0; j < n; j++)
            {
                seed = seed * 0x343fd + 0x269EC3; // a=214013, b=2531011
                float rand_val = (seed / 65536) & 0x7FFF;
                mats[iteration][i][j] = static_cast<float>(rand_val) / static_cast<float>(RAND_MAX);
            }
        }
        for (size_t i = 0; i < n; i++)
        {
            seed = seed * 0x343fd + 0x269EC3; // a=214013, b=2531011
            float rand_val = (seed / 65536) & 0x7FFF;
            vecs[iteration][i] = static_cast<float>(rand_val) / static_cast<float>(RAND_MAX);
        }

        // Computation
        for (size_t row_idx = 0; row_idx < n; row_idx++)
        {
            float result = 0;
            for (size_t col_idx = 0; col_idx < n; col_idx++)
            {
                result += mats[iteration][row_idx][col_idx] * vecs[iteration][col_idx];
            }
            ress[iteration][row_idx] = result;
        }
    }

    for (size_t iteration = 0; iteration < range; iteration++)
    {
        delete[] ress[iteration]; // 2nd dim
    }
    delete[] ress; // 1st dim
    for (size_t iteration = 0; iteration < range; iteration++)
    {
        delete[] vecs[iteration]; // 2nd dim
    }
    delete[] vecs; // 1st dim
    for (size_t iteration = 0; iteration < range; iteration++)
    {
        const size_t n = offset + (iteration + lower) * scale;
        for (size_t i = 0; i < n; i++)
        {
            delete[] mats[iteration][i]; // 3rd dim
        }
        delete[] mats[iteration]; // 2nd dim
    }
    delete[] mats; // 1st dim
}

int main(int argc, char *argv[])
{
    const double start_time = get_time_stamp();
    matvecp_kernel(0, 200);
    print_elapsed(argv[0], start_time);
}