#pragma once

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>
#include <random>
#include <vector>

#include "task.hpp"

namespace TESTS
{
    std::vector<TP::RAW_TASK> generate_n_tasks(size_t num_tasks, std::function<void(size_t)> task);

    std::vector<TP::RAW_TASK> generate_simple_print_tasks(size_t num_tasks);

    size_t collatz_conjecture_kernel(size_t lower, size_t upper, size_t num_attempts = 1);

    std::tuple<std::function<size_t()>, std::vector<TP::RAW_TASK>, std::unique_ptr<std::atomic<size_t>>>
    generate_collatz_conjecture_tasks();

    std::vector<TP::RAW_TASK> generate_sorting_tasks(size_t num_tasks);
}

/// Implementation
namespace TESTS
{
    inline std::vector<TP::RAW_TASK> generate_n_tasks(size_t num_tasks, std::function<void(size_t)> task)
    {
        std::vector<TP::RAW_TASK> tasks;
        tasks.reserve(num_tasks);
        for (size_t i = 0; i < num_tasks; i++)
        {
            tasks.emplace_back([i, task]()
                               { task(i); });
        }
        return tasks;
    }

    inline std::vector<TP::RAW_TASK> generate_simple_print_tasks(size_t num_tasks)
    {
        return generate_n_tasks(num_tasks, [](size_t i)
                                { printf("%lu\n", i); });
    }

    inline size_t collatz_conjecture_kernel(size_t lower, size_t upper, size_t num_attempts)
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

        auto single_task = []()
        {
            return collatz_conjecture_kernel(offset, offset + num_shards * shard_size, num_attempts);
        };

        auto result_ptr = std::make_unique<std::atomic<size_t>>(0);
        std::vector<TP::RAW_TASK> tasks =
            generate_n_tasks(num_shards,
                             [result = result_ptr.get()](size_t i)
                             { *result +=
                                   TESTS::collatz_conjecture_kernel(offset + i * shard_size, offset + (i + 1) * shard_size, num_attempts); });

        return {std::move(single_task), std::move(tasks), std::move(result_ptr)};
    }

    inline std::vector<TP::RAW_TASK> generate_sorting_tasks(size_t num_tasks)
    {
        constexpr size_t offset = 1;
        constexpr size_t scale = 30000;

        auto task = [](size_t task_id)
        {
            size_t n = offset + task_id * scale;

            std::mt19937 mersenne_engine;
            std::uniform_real_distribution<float> dist{0, 1.0};
            auto gen = [&dist, &mersenne_engine]()
            {
                return dist(mersenne_engine);
            };
            std::vector<float> vec(n);
            std::generate(vec.begin(), vec.end(), gen);

            std::sort(vec.begin(), vec.end());
            // printf("task=%lu size=%lu done\n", task_id, vec.size());
        };

        return generate_n_tasks(num_tasks, [task](size_t i)
                                { task(i); });
    }
}