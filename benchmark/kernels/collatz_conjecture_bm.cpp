#include <cstdio>
#include <vector>

size_t collatz_conjecture_kernel(size_t lower, size_t upper)
{
    const size_t n = upper - lower;
    size_t *steps = new size_t[n];
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

    delete[] steps;
    return total_step;
}

int main()
{
    size_t result = collatz_conjecture_kernel(1000000, 10000000);
    printf("result=%lu\n", result);
}
