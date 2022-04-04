#define MESSAGE_LEVEL 0

#include <algorithm>
#include <cstdio>

#include "tests_kernels.hpp"
#include "timer.hpp"
#include "utst.hpp"
#include "suap_pool.hpp"

using namespace TP;

UTST_MAIN();

namespace
{
    std::unique_ptr<SUAP_POOL> quick_launch(size_t num_workers, const std::vector<RAW_TASK> &tasks)
    {
        auto pool = std::make_unique<SUAP_POOL>(num_workers);
        pool->start();
        pool->execute(tasks);
        return pool;
    }
}

UTST_TEST(simple)
{
    quick_launch(2, TESTS::generate_simple_print_tasks(2));
}

UTST_TEST(simple_with_idle_worker)
{
    quick_launch(4, TESTS::generate_simple_print_tasks(2));
}

UTST_TEST(multi_session)
{
    constexpr int size = 32;
    SUAP_POOL pool(size);
    pool.start();
    for (int i = 1; i <= size; i++)
    {
        printf("generate_simple_print_tasks(%d)\n", i);
        pool.execute(TESTS::generate_simple_print_tasks(i));
        pool.status();
    }
}

UTST_TEST(collatz_conjecture)
{
    TIMER timer("collatz_conjecture");

    auto [serial_task, tasks, result_ptr] = TESTS::generate_collatz_conjecture_tasks();
    timer.elapsed_previous("init");

    // Serial execution result
    size_t serial_result = serial_task();
    printf("serial total_num_steps=%lu\n", serial_result);
    timer.elapsed_previous("serial");

    // Serial chunk execution result
    *result_ptr = 0;
    for (const auto &task : tasks)
    {
        task();
    }
    size_t serial_chunk_result = *result_ptr;
    printf("serial chunk total_num_steps=%lu\n", serial_chunk_result);
    timer.elapsed_previous("serial_chunk");

    // Pool execution result
    *result_ptr = 0;
    SUAP_POOL pool(4);
    pool.start();
    timer.elapsed_previous("pool_init");
    pool.execute(tasks);
    size_t pool_result = *result_ptr;
    printf("pool total_num_steps=%lu\n", pool_result);
    timer.elapsed_previous("pool");

    pool.status();

    UTST_ASSERT_EQUAL(serial_result, serial_chunk_result);
    UTST_ASSERT_EQUAL(serial_result, pool_result);
}