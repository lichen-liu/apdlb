#pragma once

#include <cstdio>
#include <vector>

#include "task.hpp"

namespace TESTS
{
    inline std::vector<TP::RAW_TASK> generate_simple_print_tasks(size_t num_tasks)
    {
        std::vector<TP::RAW_TASK> tasks;
        tasks.reserve(num_tasks);
        for (size_t i = 0; i < num_tasks; i++)
        {
            tasks.emplace_back([i]()
                               { printf("%lu\n", i); });
        }
        return tasks;
    }

    inline size_t collatz_conjecture_kernel(size_t lower, size_t upper)
    {
        size_t step = 0;
        for (size_t i = lower; i < upper; i++)
        {
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
        }
        return step;
    }
}