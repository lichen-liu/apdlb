#pragma once

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <tuple>
#include <vector>

#include "task.hpp"
#include "macros.hpp"

namespace TESTS
{
    std::vector<ERT::RAW_TASK> generate_n_tasks(size_t num_tasks, std::function<void(size_t)> task);

    inline void simple_print_kernel(size_t i)
    {
        printf("%lu\n", i);
    }
    inline std::vector<ERT::RAW_TASK> generate_simple_print_tasks(size_t num_tasks)
    {
        return generate_n_tasks(num_tasks, simple_print_kernel);
    }

    size_t collatz_conjecture_kernel(size_t lower, size_t upper, size_t num_attempts = 1);
    std::tuple<std::function<size_t()>, std::vector<ERT::RAW_TASK>, std::unique_ptr<std::atomic<size_t>>>
    generate_collatz_conjecture_tasks();

    void sorting_kernel(size_t i);
    inline std::vector<ERT::RAW_TASK> generate_sorting_tasks(size_t num_tasks)
    {
        return generate_n_tasks(num_tasks, sorting_kernel);
    }

    void matvecp_kernel(size_t i);
    inline std::vector<ERT::RAW_TASK> generate_matvecp_tasks(size_t num_tasks)
    {
        return generate_n_tasks(num_tasks, matvecp_kernel);
    }

    std::vector<ERT::RAW_TASK> generate_shared_edge_tasks(
        std::vector<float> *out_pos,
        std::vector<float> *out_acc,
        std::vector<std::mutex> *out_acc_locks,
        std::vector<float> *mass);
}

/// Implementation
namespace TESTS
{
    inline std::vector<ERT::RAW_TASK> generate_n_tasks(size_t num_tasks, std::function<void(size_t)> task)
    {
        std::vector<ERT::RAW_TASK> tasks;
        tasks.reserve(num_tasks);
        for (size_t i = 0; i < num_tasks; i++)
        {
            tasks.emplace_back([i, task]()
                               { task(i); });
        }
        return tasks;
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
                    local_step++;
                }
            }
            step += local_step;
        }

        return step / num_attempts;
    }

    inline std::tuple<std::function<size_t()>, std::vector<ERT::RAW_TASK>, std::unique_ptr<std::atomic<size_t>>>
    generate_collatz_conjecture_tasks()
    {
        constexpr size_t num_attempts = 1;
        constexpr size_t offset = 0;
        constexpr size_t num_shards = 30000;
        constexpr size_t shard_size = 150;

        auto single_task = []()
        {
            return collatz_conjecture_kernel(offset, offset + num_shards * shard_size, num_attempts);
        };

        auto result_ptr = std::make_unique<std::atomic<size_t>>(0);
        std::vector<ERT::RAW_TASK> tasks =
            generate_n_tasks(num_shards,
                             [result = result_ptr.get()](size_t i)
                             { *result +=
                                   TESTS::collatz_conjecture_kernel(offset + i * shard_size, offset + (i + 1) * shard_size, num_attempts); });

        return {std::move(single_task), std::move(tasks), std::move(result_ptr)};
    }

    inline void sorting_kernel(size_t i)
    {
        constexpr size_t offset = 1;
        constexpr size_t scale = 5000;
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
        // printf("task=%lu size=%lu done\n", i, vec.size());
    }

    inline void matvecp_kernel(size_t i)
    {
        constexpr size_t offset = 1;
        constexpr size_t scale = 15;
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

    // auto out_pos = std::make_unique<std::vector<float>>(3 * n_body);
    // auto out_acc = std::make_unique<std::vector<float>>(3 * n_body);
    // auto out_acc_locks = std::make_unique<std::vector<std::mutex>>(n_body);
    // auto mass = std::make_unique<std::vector<float>>(n_body);
    inline std::vector<ERT::RAW_TASK> generate_shared_edge_tasks(
        std::vector<float> *out_pos,
        std::vector<float> *out_acc,
        std::vector<std::mutex> *out_acc_locks,
        std::vector<float> *mass)
    {
        constexpr float EPISLON = 0.001;
        constexpr int SQRT_NUM_ITERATION = 20;

        const int n_body = mass->size();
        ASSERT(out_pos->size() == out_acc->size());
        ASSERT(out_acc_locks->size() == mass->size());
        ASSERT(static_cast<int>(out_pos->size() / 3) == n_body);

        std::vector<ERT::RAW_TASK> tasks;
        for (int i_target_body = 0; i_target_body < n_body; i_target_body++)
        {
            auto task = [=]()
            {
                const int i_target_body_index = i_target_body * 3;
                (*out_acc)[i_target_body_index] = 0;
                (*out_acc)[i_target_body_index + 1] = 0;
                (*out_acc)[i_target_body_index + 2] = 0;

                // Step 5: Compute acceleration
                for (int j_source_body = i_target_body + 1; j_source_body < n_body; j_source_body++)
                {

                    const int j_source_body_index = j_source_body * 3;

                    const float x_displacement = (*out_pos)[j_source_body_index] - (*out_pos)[i_target_body_index];
                    const float y_displacement = (*out_pos)[j_source_body_index + 1] - (*out_pos)[i_target_body_index + 1];
                    const float z_displacement = (*out_pos)[j_source_body_index + 2] - (*out_pos)[i_target_body_index + 2];

                    const float denom_base =
                        x_displacement * x_displacement +
                        y_displacement * y_displacement +
                        z_displacement * z_displacement +
                        EPISLON * EPISLON;

                    float inverse_sqrt_denom_base = 0;
                    {
                        float z = 1;
                        for (int i = 0; i < SQRT_NUM_ITERATION; i++)
                        {
                            z -= (z * z - denom_base) / (2 * z);
                        }
                        inverse_sqrt_denom_base = 1 / z;
                    }

                    const float x_acc_without_mass = x_displacement / denom_base * inverse_sqrt_denom_base;
                    const float y_acc_without_mass = y_displacement / denom_base * inverse_sqrt_denom_base;
                    const float z_acc_without_mass = z_displacement / denom_base * inverse_sqrt_denom_base;

                    {
                        std::lock_guard<std::mutex> i_target_body_lock((*out_acc_locks)[i_target_body]);
                        (*out_acc)[i_target_body_index] += (*mass)[j_source_body] * x_acc_without_mass;
                        (*out_acc)[i_target_body_index + 1] += (*mass)[j_source_body] * y_acc_without_mass;
                        (*out_acc)[i_target_body_index + 2] += (*mass)[j_source_body] * z_acc_without_mass;
                    }
                    {
                        std::lock_guard<std::mutex> j_source_body_lock((*out_acc_locks)[j_source_body]);
                        (*out_acc)[j_source_body_index] -= (*mass)[i_target_body] * x_acc_without_mass;
                        (*out_acc)[j_source_body_index + 1] -= (*mass)[i_target_body] * y_acc_without_mass;
                        (*out_acc)[j_source_body_index + 2] -= (*mass)[i_target_body] * z_acc_without_mass;
                    }
                }
            };
            tasks.emplace_back(std::move(task));
        }
        return tasks;
    }
}