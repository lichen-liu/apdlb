#pragma once

#include <vector>

#include "macros.hpp"
#include "message.hpp"
#include "pool.hpp"
#include "task.hpp"

/// SERIAL POOL

namespace ERT
{
    class SERIAL_POOL : public POOL
    {
    public:
        explicit SERIAL_POOL(size_t num_workers) : POOL(1) {}

        // A single session of execution, blocking until completed
        virtual void execute(const std::vector<RAW_TASK> &tasks) override;
    };
}

namespace ERT
{
    inline void SERIAL_POOL::execute(const std::vector<RAW_TASK> &tasks)
    {
        ASSERT(!tasks.empty());

        for (const auto &task : tasks)
        {
            task();
        }
    }
}