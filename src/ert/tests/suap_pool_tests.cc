#define MESSAGE_LEVEL 0

#include <algorithm>
#include <cstdio>

#include "tests_helper.hpp"
#include "tests_kernels.hpp"
#include "timer.hpp"
#include "utst.hpp"
#include "serial_pool.hpp"
#include "suap_pool.hpp"

using namespace ERT;

UTST_MAIN();

UTST_TEST(simple)
{
    TESTS::quick_launch<SUAP_POOL>(2, TESTS::generate_simple_print_tasks(2));
}

UTST_TEST(simple_with_idle_worker)
{
    TESTS::quick_launch<SUAP_POOL>(4, TESTS::generate_simple_print_tasks(2));
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
    auto [serial_task, tasks, result_ptr] = TESTS::generate_collatz_conjecture_tasks();

    // Serial execution result
    TIMER timer("serial");
    size_t serial_result = serial_task();
    printf("serial total_num_steps=%lu\n", serial_result);
    timer.elapsed_start();

    // Serial chunk execution result
    *result_ptr = 0;
    TESTS::quick_launch<SERIAL_POOL>(1, tasks);
    size_t serial_chunk_result = *result_ptr;
    printf("serial chunk total_num_steps=%lu\n", serial_chunk_result);

    // Pool execution result
    *result_ptr = 0;
    TESTS::quick_launch<SUAP_POOL>(4, tasks);
    size_t pool_result = *result_ptr;
    printf("pool total_num_steps=%lu\n", pool_result);

    UTST_ASSERT_EQUAL(serial_result, serial_chunk_result);
    UTST_ASSERT_EQUAL(serial_result, pool_result);
}