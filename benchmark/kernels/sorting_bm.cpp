#include <cstdlib>

void sorting_kernel(size_t lower, size_t upper)
{
    const size_t offset = 1;
    const size_t scale = 50;
    for (size_t iteration = lower; iteration < upper; iteration++)
    {
        const size_t n = offset + iteration * scale;

        float *vec = new float[n];

        // Generate random data
        int seed = static_cast<int>(iteration);
        for (size_t i = 0; i < n; i++)
        {
            seed = seed * 0x343fd + 0x269EC3; // a=214013, b=2531011
            float rand_val = (seed / 65536) & 0x7FFF;
            vec[i] = static_cast<float>(rand_val) / static_cast<float>(RAND_MAX);
        }

        // Bubble sort
        for (size_t i = 0; i < n - 1; i++)
        {
            // Last i elements are already in place
            for (size_t j = 0; j < n - i - 1; j++)
            {
                if (vec[j] > vec[j + 1])
                {
                    const float temp = vec[j];
                    vec[j] = vec[j + 1];
                    vec[j + 1] = temp;
                }
            }
        }

        delete[] vec;
    }
}

int main()
{
    sorting_kernel(0, 200);
}