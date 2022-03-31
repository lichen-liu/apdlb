#include "utst.hpp"
#include "wspdr.hpp"
#include "timer.hpp"
#include <cstdio>
using namespace TP;

UTST_MAIN();

UTST_TEST(computation)
{
    TIMER timer("computation");

    // Collatz conjecture
    auto kernel = [](size_t lower, size_t upper)
    {
        int step = 0;
        for (size_t i = 1; i < upper; i++)
        {
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
    };

    constexpr int num_steps = 500;
    constexpr int shard_size = 500;
    std::atomic<size_t> result = 0;
    std::vector<TASK> tasks;
    for (int i = 0; i < num_steps; i++)
    {
        auto modified_kernel = [i, &kernel, &result]
        {
            result += kernel(i * shard_size, (i + 1) * shard_size);
            // printf("step %d done\n", i);
        };
        tasks.emplace_back(modified_kernel);
    }

    timer.elapsed_previous("init");

    // Serial execution result
    result = 0;
    for (const auto &task : tasks)
    {
        task();
    }
    size_t expected_result = result;
    printf("serial total_num_steps=%lu for num_steps=%d shard_size=%d\n", expected_result, num_steps, shard_size);
    timer.elapsed_previous("serial");

    // Pool execution result
    result = 0;
    WSPDR pool(4);
    pool.start();
    timer.elapsed_previous("pool_init");
    pool.execute(tasks);
    size_t actual_result = result;
    printf("pool total_num_steps=%lu for num_steps=%d shard_size=%d\n", actual_result, num_steps, shard_size);
    timer.elapsed_previous("pool");

    pool.status();
    UTST_ASSERT_EQUAL(expected_result, actual_result);
}