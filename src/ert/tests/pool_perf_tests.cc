#define MESSAGE_LEVEL 0

#include "serial_pool.hpp"
#include "suap_pool.hpp"
#include "tests_helper.hpp"
#include "tests_kernels.hpp"
#include "timer.hpp"
#include "utst.hpp"
#include "wspdr_pool.hpp"

using namespace ERT;

UTST_MAIN();

UTST_TEST(sorting)
{
    constexpr size_t num_tasks = 200;
    constexpr size_t num_workers = 4;

    std::vector<RAW_TASK> tasks = TESTS::generate_sorting_tasks(num_tasks);

    // TESTS::quick_launch<SERIAL_POOL>(num_workers, tasks);
    TESTS::quick_launch<SUAP_POOL>(num_workers, tasks);
    TESTS::quick_launch<WSPDR_POOL>(num_workers, tasks);
}

UTST_TEST(matvecp)
{
    constexpr size_t num_tasks = 200;
    constexpr size_t num_workers = 4;

    std::vector<RAW_TASK> tasks = TESTS::generate_matvecp_tasks(num_tasks);

    // TESTS::quick_launch<SERIAL_POOL>(num_workers, tasks);
    TESTS::quick_launch<SUAP_POOL>(num_workers, tasks);
    TESTS::quick_launch<WSPDR_POOL>(num_workers, tasks);
}