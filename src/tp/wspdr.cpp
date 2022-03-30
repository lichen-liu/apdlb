#include "wspdr.h"
#include "macros.hpp"
#include <algorithm>
#include <cstdlib>

namespace TP
{
    void WSPDR_WORKER::run()
    {
        this->should_terminate_ = false; // A fresh start
        // Worker event loop
        while (true)
        {
            if (this->tasks_.empty())
            {
                // Acquire loop
                while (true)
                {
                    if (this->try_acquire_once())
                    {
                        // Exit acquire loop
                        break;
                    }
                    else if (this->should_terminate_)
                    {
                        return;
                    }
                }
            }
            else
            {
                TASK t = this->tasks_.back();
                this->tasks_.pop_back();
                this->update_status();
                this->communicate();
                t();
            }
        }
    }

    void WSPDR_WORKER::add_task(TASK task)
    {
        this->tasks_.emplace_back(std::move(task));
        this->update_status();
    }

    bool WSPDR_WORKER::try_send_steal_request(int requester_worker_id)
    {
        if (this->has_tasks_)
        {
            // https://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange
            int no_request = NO_REQUEST;
            return std::atomic_compare_exchange_strong(&this->request_, &no_request, requester_worker_id);
        }
        return false;
    }

    void WSPDR_WORKER::distribute_task(TASK task)
    {
        ASSERT(!this->received_task_opt_.has_value());
        this->received_task_opt_.emplace(std::move(task));
    }

    void WSPDR_WORKER::communicate()
    {
        int requester = this->request_;
        if (requester != NO_REQUEST)
        {
            if (this->tasks_.empty())
            {
                this->workers_[requester]->distribute_task(nullptr);
            }
            else
            {
                TASK t = this->tasks_.front();
                this->tasks_.pop_front();
                this->workers_[requester]->distribute_task(std::move(t));
            }
            this->request_ = NO_REQUEST;
            this->update_status();
        }
    }

    bool WSPDR_WORKER::try_acquire_once()
    {
        this->received_task_opt_.reset();
        int target_worker_id = std::rand() / ((RAND_MAX + 1u) / this->workers_.size());
        // Does not support self-steal
        if (target_worker_id != this->worker_id_)
        {
            if (this->workers_[target_worker_id]->try_send_steal_request(this->worker_id_))
            {
                // Request sent, now waiting for a response
                while (!this->received_task_opt_.has_value())
                {
                    // While waiting, still respond to other worker who has sent request to this worker
                    this->communicate();
                }
                if (this->received_task_opt_.value())
                {
                    // Check whether the target worker sent a real task to this worker
                    this->add_task(this->received_task_opt_.value());
                    // this->request_ = NO_REQUEST;
                    return true;
                }
            }
            // While looking for target worker to steal from, still respond to other worker who has sent request to this worker
            this->communicate();
        }
        return false;
    }

    void WSPDR_WORKER::update_status()
    {
        bool b = !this->tasks_.empty();
        if (this->has_tasks_ != b)
            this->has_tasks_ = b;
    }

    WSPDR::WSPDR(size_t num_workers)
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
        for (int worker_id = 0; worker_id < num_workers; worker_id++)
        {
            this->workers_[worker_id]->init(worker_id, worker_ptrs);
        }
    }

    WSPDR::~WSPDR()
    {
        this->terminate();
    }

    void WSPDR::start()
    {
        this->executors_.clear();
        this->executors_.reserve(this->workers_.size());
        for (const auto &worker : this->workers_)
        {
            this->executors_.emplace_back(&WSPDR_WORKER::run, worker.get());
        }
    }

    void WSPDR::terminate()
    {
        for (const auto &worker : this->workers_)
        {
            worker->request_terminate();
        }
        for (auto &executor : this->executors_)
        {
            executor.join();
        }
        this->executors_.clear();
    }

    void WSPDR::execute(const std::vector<TASK> &tasks)
    {
        // Executors must be launched already
        ASSERT(!this->executors_.empty());

        // For synchronization
        int total_num_tasks = tasks.size();
        std::atomic<int> num_task_done = 0;

        // Integrate synchronization into argument tasks
        std::vector<TASK> synced_tasks;
        synced_tasks.reserve(tasks.size());
        for (const auto &task : tasks)
        {
            auto synced_task = [&task, &num_task_done]()
            {
                task();
                num_task_done++;
            };
            synced_tasks.emplace_back(std::move(synced_task));
        }

        // Create a scheduler task to do worker->add_task
        WSPDR_WORKER *scheduler_worker = this->workers_.front().get();
        auto scheduler_task = [scheduler_worker, &synced_tasks]()
        {
            for (const auto &t : synced_tasks)
            {
                scheduler_worker->add_task(t);
            }
        };
        scheduler_worker->add_task(scheduler_task);

        // Wait for all tasks do be done
        while (num_task_done.load() != total_num_tasks)
        {
        }
    }
}