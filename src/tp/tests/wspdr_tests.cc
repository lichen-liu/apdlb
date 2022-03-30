#include "utst.hpp"
#include "wspdr.h"
#include <iostream>
using namespace TP;

UTST_MAIN();

UTST_TEST(simple_2)
{
    std::vector<TASK> tasks;
    tasks.emplace_back([]()
                       { std::cout << "0" << std::endl; });
    tasks.emplace_back([]()
                       { std::cout << "1" << std::endl; });
    WSPDR pool(2);
    pool.start();
    pool.execute(tasks);
}