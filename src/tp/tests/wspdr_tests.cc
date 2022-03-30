#include "utst.hpp"
#include "wspdr.h"
#include <cstdio>
using namespace TP;

UTST_MAIN();

UTST_TEST(simple_2)
{
    std::vector<TASK> tasks;
    tasks.emplace_back([]()
                       { printf("0\n"); });
    tasks.emplace_back([]()
                       { printf("1\n"); });
    WSPDR pool(2);
    pool.start();
    pool.execute(tasks);
}

UTST_TEST(simple_2_idle_worker)
{
    std::vector<TASK> tasks;
    tasks.emplace_back([]()
                       { printf("0\n"); });
    tasks.emplace_back([]()
                       { printf("1\n"); });
    WSPDR pool(8);
    pool.start();
    pool.execute(tasks);
}