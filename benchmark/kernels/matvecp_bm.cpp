#include <algorithm>
#include <vector>
//  int ms_rand(int& seed)
// {
//   seed = seed*0x343fd+0x269EC3;  // a=214013, b=2531011
//   return (seed >> 0x10) & 0x7FFF;
// }
void matvecp_kernel(size_t lower, size_t upper)
{
    const size_t offset = 1;
    const size_t scale = 15;

    for (size_t i = lower; i < upper; i++)
    {
        const size_t n = offset + i * scale;

        auto gen = []()
        {
            float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            return r;
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
    matvecp_kernel(0, 200);
}