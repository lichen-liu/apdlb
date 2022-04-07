#include <cstdlib>

void matvecp_kernel(size_t lower, size_t upper)
{
    const size_t offset = 1;
    const size_t scale = 20;

    for (size_t iteration = lower; iteration < upper; iteration++)
    {
        const size_t n = offset + iteration * scale;

        // Input mat
        float **mat = new float *[n];
        for (size_t i = 0; i < n; i++)
        {
            mat[i] = new float[n];
        }
        // Generate random data
        int seed = static_cast<int>(iteration);
        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = 0; j < n; j++)
            {
                seed = seed * 0x343fd + 0x269EC3; // a=214013, b=2531011
                float rand_val = (seed / 65536) & 0x7FFF;
                mat[i][j] = static_cast<float>(rand_val) / static_cast<float>(RAND_MAX);
            }
        }

        // Input vec
        float *vec = new float[n];
        for (size_t i = 0; i < n; i++)
        {
            seed = seed * 0x343fd + 0x269EC3; // a=214013, b=2531011
            float rand_val = (seed / 65536) & 0x7FFF;
            vec[i] = static_cast<float>(rand_val) / static_cast<float>(RAND_MAX);
        }

        // Output
        float *res = new float[n];

        // Computation
        for (size_t row_idx = 0; row_idx < n; row_idx++)
        {
            float result = 0;
            for (size_t col_idx = 0; col_idx < n; col_idx++)
            {
                result += mat[row_idx][col_idx] * vec[col_idx];
            }
            res[row_idx] = result;
        }

        // Clean up
        // Output
        delete[] res;

        // Input vec
        delete[] vec;

        // Input mat
        for (size_t i = 0; i < n; i++)
        {
            delete[] mat[i];
        }
        delete[] mat;
    }
}

int main()
{
    matvecp_kernel(0, 200);
}