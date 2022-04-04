#define MESSAGE_LEVEL 0

#include "suap_pool.hpp"
#include "tests_kernels.hpp"
#include "timer.hpp"
#include "utst.hpp"
#include "wspdr_pool.hpp"

using namespace TP;

UTST_MAIN();

UTST_TEST(sorting)
{
    TIMER timer("sorting");
    constexpr size_t num_tasks = 100;
    constexpr size_t num_workers = 4;

    std::vector<RAW_TASK> tasks = TESTS::generate_sorting_tasks(num_tasks);
    timer.elapsed_previous("init_tasks");

    {
        SUAP pool(num_workers);
        pool.start();
        timer.elapsed_previous("init_suap");

        pool.execute(tasks);
        timer.elapsed_previous("suap");
    }
    timer.elapsed_previous("terminate_suap");

    {
        WSPDR pool(num_workers);
        pool.start();
        timer.elapsed_previous("init_wspdr");

        pool.execute(tasks);
        timer.elapsed_previous("wspdr");
    }
    timer.elapsed_previous("terminate_wspdr");
}