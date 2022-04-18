#define MESSAGE_LEVEL 0

#include "serial_pool.hpp"
#include "suap_pool.hpp"
#include "tests_helper.hpp"
#include "tests_kernels.hpp"
#include "timer.hpp"
#include "utst.hpp"
#include "wspdr_pool.hpp"

using namespace ERT;

namespace
{
    constexpr size_t num_workers = 8;
}

UTST_MAIN();

UTST_TEST(sorting)
{
    constexpr size_t num_tasks = 200;

    std::vector<RAW_TASK> tasks = TESTS::generate_sorting_tasks(num_tasks);

    // TESTS::quick_launch<SERIAL_POOL>(num_workers, tasks);
    TESTS::quick_launch<SUAP_POOL>(num_workers, tasks);
    TESTS::quick_launch<WSPDR_POOL>(num_workers, tasks);
}

UTST_TEST(matvecp)
{
    constexpr size_t num_tasks = 200;

    std::vector<RAW_TASK> tasks = TESTS::generate_matvecp_tasks(num_tasks);

    // TESTS::quick_launch<SERIAL_POOL>(num_workers, tasks);
    TESTS::quick_launch<SUAP_POOL>(num_workers, tasks);
    TESTS::quick_launch<WSPDR_POOL>(num_workers, tasks);
}

UTST_TEST(shared_edge)
{
    constexpr int n_body = 10000;

    auto out_pos = std::make_unique<std::vector<float>>(3 * n_body);
    auto out_acc = std::make_unique<std::vector<float>>(3 * n_body);
    auto out_acc_locks = std::make_unique<std::vector<std::mutex>>(n_body);
    auto mass = std::make_unique<std::vector<float>>(n_body);
    std::vector<RAW_TASK> tasks = TESTS::generate_shared_edge_tasks(out_pos.get(), out_acc.get(), out_acc_locks.get(), mass.get());

    // TESTS::quick_launch<SERIAL_POOL>(num_workers, tasks);
    TESTS::quick_launch<SUAP_POOL>(num_workers, tasks);
    TESTS::quick_launch<WSPDR_POOL>(num_workers, tasks);
}