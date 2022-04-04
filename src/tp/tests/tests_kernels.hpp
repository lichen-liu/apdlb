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
}