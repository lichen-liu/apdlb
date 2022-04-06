#include <algorithm>
#include <vector>

void sorting_kernel(size_t lower, size_t upper)
{
    const size_t offset = 1;
    const size_t scale = 1500;
    for (size_t i = lower; i < upper; i++)
    {
        const size_t n = offset + i * scale;

        auto gen = []()
        {
            float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            return r;
        };
        std::vector<float> vec(n);
        std::generate(vec.begin(), vec.end(), gen);

        std::sort(vec.begin(), vec.end());
    }
}

int main()
{
    sorting_kernel(0, 200);
}