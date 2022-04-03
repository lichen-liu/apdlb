#pragma once

#include "task.hpp"

namespace TP
{
    class POOL
    {
    public:
        explicit POOL(size_t num_workers) : num_workers_(num_workers) {}
        virtual ~POOL() = default;

        virtual void start() = 0;
        virtual void terminate() = 0;
        // A single session of execution, blocking until completed
        virtual void execute(const std::vector<RAW_TASK> &tasks) = 0;
        virtual void status() const = 0;

        size_t num_workers() const { return this->num_workers_; }

    private:
        size_t num_workers_;
    };
}
