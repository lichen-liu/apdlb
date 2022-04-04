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
    SUAP pool(4);
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