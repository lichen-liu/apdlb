#pragma once

#include <cstdio>
#include <functional>
#include <memory>
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

    inline size_t collatz_conjecture_kernel(size_t lower, size_t upper, size_t num_attempts = 1)
    {
        size_t step = 0;
        for (size_t itry = 0; itry < num_attempts; itry++)
        {
            size_t local_step = 0;
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
            step += local_step;
        }

        return step / num_attempts;
    }

    inline std::tuple<std::function<size_t()>, std::vector<TP::RAW_TASK>, std::unique_ptr<std::atomic<size_t>>>
    generate_collatz_conjecture_tasks()
    {
        constexpr size_t num_attempts = 1;
        constexpr size_t offset = 0;
        constexpr size_t num_shards = 50000;
        constexpr size_t shard_size = 200;

        auto result_ptr = std::make_unique<std::atomic<size_t>>(0);
        std::vector<TP::RAW_TASK> tasks;
        for (int i = 0; i < num_shards; i++)
        {
            auto modified_kernel = [i, result = result_ptr.get()]
            {
                *result += TESTS::collatz_conjecture_kernel(offset + i * shard_size, offset + (i + 1) * shard_size, num_attempts);
                // printf("step %d done\n", i);
            };
            tasks.emplace_back(modified_kernel);
        }

        auto single_task = []()
        {
            return collatz_conjecture_kernel(offset, offset + num_shards * shard_size);
        };

        return {std::move(single_task), std::move(tasks), std::move(result_ptr)};
    }
}