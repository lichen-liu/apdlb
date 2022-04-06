#include <cstdlib>
#include <vector>
void sorting_kernel(size_t lower, size_t upper)
{
    const size_t offset = 1;
    const size_t scale = 1500;
    for (size_t i = lower; i < upper; i++)
    {
        const size_t n = offset + i * scale;
        float *vec = new float[n];
        for (size_t i = 0; i < n; i++)
        {
            vec[i] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        }
        delete[] vec;

        std::vector<float> vvec;
        for (size_t i = 0; i < n; i++)
        {
            vvec.push_back(static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
        }
        // std::sort(vec.begin(), vec.end());
    }
}

int main()
{
    sorting_kernel(0, 200);
}