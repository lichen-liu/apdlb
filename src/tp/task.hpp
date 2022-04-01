#pragma once

#include <functional>
#include <vector>

namespace TP
{
    struct WORKER_PROXY;

    using TASK = std::function<void(WORKER_PROXY &)>;
    using RAW_TASK = std::function<void()>;

    struct WORKER_PROXY
    {
        std::vector<TASK> tasks; // new tasks to add
    };

    TASK to_task(RAW_TASK raw_task)
    {
        return [raw_task = std::move(raw_task)](WORKER_PROXY &)
        {
            raw_task();
        };
    }
}