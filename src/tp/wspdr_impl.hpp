#pragma once

#include "wspdr.hpp"
#include "macros.hpp"
#include "message.hpp"
#include "utils.hpp"
#include <algorithm>

namespace TP
{
    inline WSPDR::WSPDR(size_t num_workers)
    {
        // Construct workers
        this->workers_.reserve(num_workers);
        std::generate_n(std::back_inserter(this->workers_), num_workers, []()
                        { return std::make_unique<WSPDR_WORKER>(); });

        // Initialize workers
        std::vector<WSPDR_WORKER *> worker_ptrs;
        worker_ptrs.reserve(num_workers);
        std::transform(this->workers_.begin(), this->workers_.end(), std::back_inserter(worker_ptrs), [](const auto &p)
                       { return p.get(); });
        for (size_t worker_id = 0; worker_id < num_workers; worker_id++)
        {
            this->workers_[worker_id]->init(worker_id, worker_ptrs);
        }
    }

    inline WSPDR::~WSPDR()
    {
        this->terminate();
    }

    inline void WSPDR::start()
    {
        ASSERT(this->executors_.empty());
        this->executors_.reserve(this->workers_.size());
        for (const auto &worker : this->workers_)
        {
            this->executors_.emplace_back(&WSPDR_WORKER::run, worker.get());
        }
    }

    inline void WSPDR::terminate()
    {
        for (const auto &worker : this->workers_)
        {
            worker->terminate();
        }
        for (auto &executor : this->executors_)
        {
            executor.join();
        }
        // Do not delete the workers
        this->executors_.clear();
    }

    inline void WSPDR::execute(const std::vector<RAW_TASK> &tasks)
    {
        ASSERT(!tasks.empty());

        // Executors must be launched already
        ASSERT(!this->executors_.empty());

        // For synchronization
        const size_t total_num_tasks = tasks.size();
        std::atomic<size_t> num_tasks_done = 0;

        // Integrate synchronization into argument tasks
        std::vector<TASK> synced_tasks;
        synced_tasks.reserve(total_num_tasks);
        for (const auto &task : tasks)
        {
            auto synced_task = [task, &num_tasks_done](WORKER_PROXY &)
            {
                task();
                num_tasks_done++;
            };
            synced_tasks.emplace_back(std::move(synced_task));
        }

        // Create a scheduler task to do worker->add_task
        auto scheduler_task = [synced_tasks = std::move(synced_tasks)](WORKER_PROXY &worker_proxy)
        {
            worker_proxy.tasks = std::move(synced_tasks);
            info("scheduler_task done @thread=%s, num_tasks_added=%lu\n", to_string(std::this_thread::get_id()).c_str(), worker_proxy.tasks.size());
        };
        // Scheduler task must be anchored to add_task to sheduler_worker
        this->workers_.front()->send_task(std::move(scheduler_task), true);

        // Wait for all tasks do be done
        while (num_tasks_done.load() != total_num_tasks)
        {
        }
    }

    inline void WSPDR::status() const
    {
        warn("===================\n");
        warn("[WSPDR] workers=%lu, executors=%lu]\n", this->workers_.size(), this->executors_.size());
        for (const auto &worker : this->workers_)
        {
            worker->status();
        }
        warn("===================\n");
    }
}