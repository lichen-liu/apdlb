#include "utst.hpp"
#include "wspdr.hpp"
#include <cstdio>
#include <algorithm>
using namespace TP;

UTST_MAIN();

namespace
{
    std::unique_ptr<WSPDR> quick_launch(size_t num_workers, const std::vector<TASK> &tasks)
    {
        auto pool = std::make_unique<WSPDR>(num_workers);
        pool->start();
        pool->execute(tasks);
        return pool;
    }

    std::vector<TASK> generate_simple_print_tasks(size_t num_tasks)
    {
        std::vector<TASK> tasks;
        tasks.reserve(num_tasks);
        for (size_t i = 0; i < num_tasks; i++)
        {
            tasks.emplace_back([i]()
                               { printf("%lu\n", i); });
        }
        return tasks;
    }
}

UTST_TEST(simple)
{
    quick_launch(2, generate_simple_print_tasks(2));
}

UTST_TEST(simple_with_idle_worker)
{
    quick_launch(4, generate_simple_print_tasks(2));
}

UTST_TEST(multi_session)
{
    int size = 32;
    WSPDR pool(size);
    pool.start();
    for (int i = 1; i <= size; i++)
    {
        printf("generate_simple_print_tasks(%d)\n", i);
        pool.execute(generate_simple_print_tasks(i));
        pool.status();
    }
}