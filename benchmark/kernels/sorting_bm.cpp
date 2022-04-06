#include <algorithm>
#include <random>
#include <vector>

void sorting_kernel(size_t lower, size_t upper)
{
    const size_t offset = 1;
    const size_t scale = 1500;
    for (size_t i = lower; i < upper; i++)
    {
        const size_t n = offset + i * scale;

        std::mt19937 mersenne_engine;
        std::uniform_real_distribution<float> dist{0, 1.0};
        auto gen = [&dist, &mersenne_engine]()
        {
            return dist(mersenne_engine);
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