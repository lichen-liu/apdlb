#include "wspdr.h"
#include "macros.hpp"
#include "message.hpp"
#include "utils.h"
#include <algorithm>
#include <cstdlib>

namespace TP
{
    void WSPDR_WORKER::run()
    {
        this->is_alive_ = true;
        this->thread_id_ = std::this_thread::get_id();
        info("[Worker %d] running @thread=%s\n", this->worker_id_, to_string(this->thread_id_).c_str());
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
                        break; // Exit acquire loop
                    }
                    else if (this->send_task_notify_)
                    {
                        this->send_task_notify_ = false; // Reset
                        break;                           // Exit acquire loop
                    }
                    else if (this->terminate_notify_)
                    {
                        this->is_alive_ = false;
                        this->terminate_notify_ = false; // Reset
                        this->communicate();
                        info("[Worker %d] terminated\n", this->worker_id_);
                        return;
                    }
                }
            }
            else
            {
                TASK t = this->tasks_.back().task;
                this->tasks_.pop_back();
                this->update_tasks_status();
                this->communicate();
                debug("[Worker %d] going to run task, %lu tasks in the deque\n", this->worker_id_, this->tasks_.size());
                t();
                debug("[Worker %d] task done, %lu tasks in the deque\n", this->worker_id_, this->tasks_.size());
            }
        }
    }

    void WSPDR_WORKER::add_task(TASK task)
    {
        ASSERT(std::this_thread::get_id() == this->thread_id_);
        this->tasks_.emplace_back(TASK_HOLDER{std::move(task), false});
        this->update_tasks_status();
    }

    void WSPDR_WORKER::send_task(TASK task, bool is_anchored)
    {
        ASSERT(this->tasks_.empty());
        info("[Worker %d] send_task from @thread=%s to @thread=%s with is_anchored=%s\n",
             this->worker_id_, to_string(std::this_thread::get_id()).c_str(),
             to_string(this->thread_id_).c_str(), bool_to_cstr(is_anchored));
        this->tasks_.emplace_back(TASK_HOLDER{std::move(task), is_anchored});
        this->update_tasks_status();
        this->send_task_notify_ = true;
    }

    void WSPDR_WORKER::terminate()
    {
        debug("[Worker %d] terminate\n", this->worker_id_);

        this->terminate_notify_ = true;
    }

    void WSPDR_WORKER::status() const
    {
        warn("[Worker %d] @thread=%s, workers=%lu, tasks=%lu, received_task=%s, request=%d, has_tasks=%s, send_task_notify=%s, terminate_notify=%s, is_alive=%s\n",
             this->worker_id_,
             to_string(this->thread_id_).c_str(), this->workers_.size(), this->tasks_.size(),
             bool_to_cstr(this->received_task_opt_.has_value()), this->request_.load(),
             bool_to_cstr(this->has_tasks_), bool_to_cstr(this->send_task_notify_),
             bool_to_cstr(this->terminate_notify_), bool_to_cstr(this->is_alive_));
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
                auto [t, is_anchored] = this->tasks_.front();
                if (is_anchored)
                {
                    this->workers_[requester]->distribute_task(nullptr);
                }
                else
                {
                    this->tasks_.pop_front();
                    this->workers_[requester]->distribute_task(std::move(t));
                }
            }
            this->request_ = NO_REQUEST;
            this->update_tasks_status();
        }
    }

    bool WSPDR_WORKER::try_acquire_once()
    {
        this->received_task_opt_.reset();
        int target_worker_id = std::rand() / ((RAND_MAX + 1u) / this->workers_.size());
        // Does not support self-steal
        if (target_worker_id != this->worker_id_)
        {
            if (this->workers_[target_worker_id]->is_alive() && this->workers_[target_worker_id]->try_send_steal_request(this->worker_id_))
            {
                // Request sent, now waiting for a response
                while (!this->received_task_opt_.has_value())
                {
                    // While waiting, still respond to other worker who has sent request to this worker
                    this->communicate();
                }
                TASK received_task = this->received_task_opt_.value();
                this->received_task_opt_.reset();
                if (received_task)
                {
                    // Check whether the target worker sent a real task to this worker
                    this->add_task(received_task);
                    debug("[Worker %d] acquired a task from worker %d, %lu tasks in the deque\n", this->worker_id_, target_worker_id, this->tasks_.size());
                    // this->request_ = NO_REQUEST;
                    return true;
                }
            }
            // While looking for target worker to steal from, still respond to other worker who has sent request to this worker
            this->communicate();
        }
        return false;
    }

    void WSPDR_WORKER::update_tasks_status()
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
        for (size_t worker_id = 0; worker_id < num_workers; worker_id++)
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
        ASSERT(this->executors_.empty());
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
            worker->terminate();
        }
        for (auto &executor : this->executors_)
        {
            executor.join();
        }
        // Do not delete the workers
        this->executors_.clear();
    }

    void WSPDR::execute(const std::vector<TASK> &tasks)
    {
        ASSERT(!tasks.empty());

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
            auto synced_task = [task, &num_task_done]()
            {
                task();
                num_task_done++;
            };
            synced_tasks.emplace_back(std::move(synced_task));
        }

        // Create a scheduler task to do worker->add_task
        WSPDR_WORKER *scheduler_worker = this->workers_.front().get();
        auto scheduler_task = [scheduler_worker, synced_tasks = std::move(synced_tasks)]()
        {
            for (const auto &t : synced_tasks)
            {
                scheduler_worker->add_task(t);
            }
            warn("scheduler_task done @thread=%s, num_tasks_added=%lu\n", to_string(std::this_thread::get_id()).c_str(), synced_tasks.size());
        };
        // Scheduler task must be anchored to add_task to sheduler_worker
        scheduler_worker->send_task(scheduler_task, true);

        // Wait for all tasks do be done
        while (num_task_done.load() != total_num_tasks)
        {
        }
    }

    void WSPDR::status() const
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