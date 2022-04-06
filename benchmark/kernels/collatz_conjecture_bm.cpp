#include <vector>

size_t collatz_conjecture_kernel(size_t lower, size_t upper)
{
    const size_t n = upper - lower;
    std::vector<size_t> steps(n, 0);
    for (size_t i = lower; i < upper; i++)
    {
        size_t step = 0;
        if (i == 0)
        {
            continue;
        }
        size_t num = i;
        while (num != 1)
        {
            if (num % 2 == 0)
            {
                num /= 2;
            }
            else
            {
                num *= 3;
                num++;
            }
            step++;
        }
        steps[i - lower] = step;
    }

    size_t total_step = 0;
    for (size_t i = 0; i < n; i++)
    {
        total_step += steps[i];
    }
    return total_step;
}

int main()
{
    collatz_conjecture_kernel(800000, 8000000);
}
