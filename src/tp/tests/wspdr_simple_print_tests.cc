#define MESSAGE_LEVEL 0

#include <algorithm>
#include <cstdio>

#include "tests_kernels.hpp"
#include "utst.hpp"
#include "wspdr_pool.hpp"

using namespace TP;

UTST_MAIN();

namespace
{
    std::unique_ptr<WSPDR_POOL> quick_launch(size_t num_workers, const std::vector<RAW_TASK> &tasks)
    {
        auto pool = std::make_unique<WSPDR_POOL>(num_workers);
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
    WSPDR_POOL pool(size);
    pool.start();
    for (int i = 1; i <= size; i++)
    {
        printf("generate_simple_print_tasks(%d)\n", i);
        pool.execute(TESTS::generate_simple_print_tasks(i));
        pool.status();
    }
}