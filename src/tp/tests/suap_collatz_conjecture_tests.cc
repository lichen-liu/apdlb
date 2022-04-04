#define MESSAGE_LEVEL 0

#include <cstdio>

#include "suap_pool.hpp"
#include "tests_kernels.hpp"
#include "timer.hpp"
#include "utst.hpp"

using namespace TP;

UTST_MAIN();

UTST_TEST(computation)
{
    TIMER timer("computation");

    constexpr int num_shards = 1000000;
    constexpr int shard_size = 10;
    std::atomic<size_t> result = 0;
    std::vector<RAW_TASK> tasks;
    for (int i = 0; i < num_shards; i++)
    {
        auto modified_kernel = [i, &result]
        {
            result += TESTS::collatz_conjecture_kernel(i * shard_size, (i + 1) * shard_size);
            // printf("step %d done\n", i);
        };
        tasks.emplace_back(modified_kernel);
    }

    timer.elapsed_previous("init");

    // Serial execution result
    size_t serial_result = TESTS::collatz_conjecture_kernel(0, num_shards * shard_size);
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
    SUAP pool(4);
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