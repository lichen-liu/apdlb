#pragma once

#include "wspdr_worker.hpp"
#include <memory>

/// work stealing private deque pool - receiver initiated

namespace TP
{
    class WSPDR
    {
    public:
        explicit WSPDR(size_t num_workers);
        ~WSPDR();

        void start();
        void terminate();
        // A single session of execution, blocking until completed
        void execute(const std::vector<RAW_TASK> &tasks);
        void status() const;

    private:
        std::vector<std::unique_ptr<WSPDR_WORKER>> workers_;
        std::vector<std::thread> executors_;
    };
}

#include "wspdr_impl.hpp"