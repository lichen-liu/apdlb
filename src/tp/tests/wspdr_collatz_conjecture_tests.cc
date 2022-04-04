#define MESSAGE_LEVEL 0

#include <cstdio>

#include "timer.hpp"
#include "utst.hpp"
#include "wspdr_pool.hpp"

using namespace TP;

namespace
{
    size_t collatz_conjecture_kernel(size_t lower, size_t upper)
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

UTST_MAIN();

UTST_TEST(computation)
{
    TIMER timer("computation");

    constexpr int num_shards = 100000;
    constexpr int shard_size = 100;
    std::atomic<size_t> result = 0;
    std::vector<RAW_TASK> tasks;
    for (int i = 0; i < num_shards; i++)
    {
        auto modified_kernel = [i, &result]
        {
            result += collatz_conjecture_kernel(i * shard_size, (i + 1) * shard_size);
            // printf("step %d done\n", i);
        };
        tasks.emplace_back(modified_kernel);
    }

    timer.elapsed_previous("init");

    // Serial execution result
    size_t serial_result = collatz_conjecture_kernel(0, num_shards * shard_size);
    printf("serial total_num_steps=%lu for num_shards=%d shard_size=%d\n", serial_result, num_shards, shard_size);
    timer.elapsed_previous("serial");

    // Serial chunk execution result
    result = 0;
    for (const auto &task : tasks)
    {
        task();
    }
    size_t serial_chunk_result = result;
    printf("serial chunk total_num_steps=%lu for num_shards=%d shard_size=%d\n", serial_chunk_result, num_shards, shard_size);
    timer.elapsed_previous("serial_chunk");

    // Pool execution result
    result = 0;
    WSPDR pool(4);
    pool.start();
    timer.elapsed_previous("pool_init");
    pool.execute(tasks);
    size_t pool_result = result;
    printf("pool total_num_steps=%lu for num_shards=%d shard_size=%d\n", pool_result, num_shards, shard_size);
    timer.elapsed_previous("pool");

    pool.status();

    UTST_ASSERT_EQUAL(serial_result, serial_chunk_result);
    UTST_ASSERT_EQUAL(serial_result, pool_result);
}