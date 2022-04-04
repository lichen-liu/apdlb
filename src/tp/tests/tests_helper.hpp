#pragma once

#include <memory>
#include <vector>

#include "task.hpp"
#include "timer.hpp"

namespace TESTS
{
    template <typename POOL_IF>
    void quick_launch(size_t num_workers, const std::vector<TP::RAW_TASK> &tasks)
    {
        TP::TIMER timer(typeid(POOL_IF).name());
        auto pool = std::make_unique<POOL_IF>(num_workers);
        pool->start();
        timer.elapsed_previous("init");
        pool->execute(tasks);
        timer.elapsed_previous("exec");
        pool->status();
        pool.reset();
        timer.elapsed_previous("dtor");
    }
}