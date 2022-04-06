#include <cstdlib>
#include "utils.h"

void sorting_kernel(size_t lower, size_t upper)
{
    const size_t offset = 1;
    const size_t scale = 1500;
    for (size_t i = lower; i < upper; i++)
    {
        const size_t n = offset + i * scale;
        float *vec = new float[n];

        int seed = static_cast<int>(i);
        for (size_t i = 0; i < n; i++)
        {
            vec[i] = static_cast<float>(KBM::rand_r(seed)) / static_cast<float>(RAND_MAX);
        }

        qsort(vec, n, sizeof(float), KBM::fcomp);
        delete[] vec;
    }
}

int main()
{
    sorting_kernel(0, 200);
}