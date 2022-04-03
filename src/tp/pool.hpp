#pragma once

#include "task.hpp"

namespace TP
{
    class POOL
    {
    public:
        explicit POOL(size_t num_workers) : num_workers_(num_workers) {}
        virtual ~POOL() = 0;

        virtual void start();
        virtual void terminate();
        // A single session of execution, blocking until completed
        virtual void execute(const std::vector<RAW_TASK> &tasks);
        virtual void status() const;

        size_t num_workers() const { return this->num_workers_; }

    private:
        size_t num_workers_;
    };
}

namespace TP
{
    inline POOL::~POOL() {}
}