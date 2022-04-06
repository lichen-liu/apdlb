#include <algorithm>
#include <random>
#include <vector>

void matvecp_kernel(size_t lower, size_t upper)
{
    constexpr size_t offset = 1;
    constexpr size_t scale = 50;

    for (size_t i = lower; i < upper; i++)
    {
        const size_t n = offset + i * scale;

        std::mt19937 mersenne_engine;
        std::uniform_real_distribution<float> dist{0, 1.0};
        auto gen = [&dist, &mersenne_engine]()
        {
            return dist(mersenne_engine);
        };

        // Input
        std::vector<std::vector<float>> mat(n, std::vector<float>(n, 0));
        for (auto &row_vec : mat)
        {
            std::generate(row_vec.begin(), row_vec.end(), gen);
        }
        std::vector<float> vec(n);
        std::generate(vec.begin(), vec.end(), gen);

        // Output
        std::vector<float> res(n, 0);

        // Computation
        for (size_t row_idx = 0; row_idx < n; row_idx++)
        {
            for (size_t col_idx = 0; col_idx < n; col_idx++)
            {
                res[row_idx] += mat[row_idx][col_idx] * vec[col_idx];
            }
        }
    }
}

int main()
{
    matvecp_kernel(1, 100);
}