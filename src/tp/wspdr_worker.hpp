#pragma once

#include <thread>
#include <vector>
#include <deque>
#include <atomic>
#include <algorithm>
#include <cstdlib>

#include "task.hpp"
#include "message.hpp"
#include "utils.hpp"

/// work stealing private deque worker - receiver initiated

namespace TP
{
    enum class WSPDR_POLICY
    {
        STEAL_ONE = 0,
        STEAL_HALF = 1,
        DEFAULT = STEAL_HALF
    };

    class WSPDR_WORKER
    {
    public:
        void init(int worker_id, std::vector<WSPDR_WORKER *> workers, WSPDR_POLICY policy = WSPDR_POLICY::DEFAULT)
        {
            this->worker_id_ = worker_id;
            this->workers_ = std::move(workers);
            this->policy_ = policy;
        }
        void run();                                  // Running on a thread
        void send_task(TASK task, bool is_anchored); // Can be used cross thread, but only when task deque is empty (with assert)
        void terminate();
        void status() const;

    private:
        void add_task(TASK task); // Must not be used cross thread (with assert)
        bool try_send_steal_request(int requester_worker_id);
        void distribute_task(std::vector<TASK> task);
        void communicate();
        bool try_acquire_once();
        void update_tasks_status();
        bool is_alive() const { return this->is_alive_; }

    private:
        static constexpr int NO_REQUEST = -1;
        struct TASK_HOLDER
        {
            TASK task;
            bool is_anchored;
        };

    private:
        std::deque<TASK_HOLDER> tasks_;
        std::vector<WSPDR_WORKER *> workers_; // back when using by self, front when using by other
        std::vector<TASK> received_tasks_;
        std::thread::id thread_id_;
        int worker_id_ = -1;
        int num_tasks_done_ = 0;
        WSPDR_POLICY policy_ = WSPDR_POLICY::DEFAULT;
        std::atomic<int> request_ = NO_REQUEST;
        std::atomic<bool> has_tasks_ = false;
        std::atomic<bool> received_tasks_notify_ = false;
        std::atomic<bool> terminate_notify_ = false;
        std::atomic<bool> is_alive_ = false;
    };
}

namespace TP
{
    inline void WSPDR_WORKER::run()
    {
        this->is_alive_ = true;
        this->thread_id_ = std::this_thread::get_id();
        info("[Worker %d] running @thread=%s\n", this->worker_id_, to_string(this->thread_id_).c_str());
        // Worker event loop
        while (true)
        {
            // Acquire loop
            while (this->tasks_.empty())
            {
                if (this->terminate_notify_)
                {
                    this->is_alive_ = false;
                    this->terminate_notify_ = false; // Reset

                    while (true)
                    {
                        int no_request = NO_REQUEST;
                        if (this->request_.compare_exchange_strong(no_request, this->worker_id_))
                        {
                            break;
                        }
                        this->communicate();
                    }

                    ASSERT(this->request_ == this->worker_id_);
                    info("[Worker %d] terminated\n", this->worker_id_);
                    return;
                }
                this->try_acquire_once();
            }

            TASK t = this->tasks_.back().task;
            this->tasks_.pop_back();
            this->update_tasks_status();
            this->communicate(); // wip
            debug("[Worker %d] going to run task, %lu tasks in the deque\n", this->worker_id_, this->tasks_.size());

            WORKER_PROXY worker_proxy;
            t(worker_proxy);
            for (const auto &new_task : worker_proxy.tasks)
            {
                this->add_task(new_task);
            }

            this->num_tasks_done_++;
            debug("[Worker %d] task done, %lu tasks in the deque\n", this->worker_id_, this->tasks_.size());
        }
    }

    inline void WSPDR_WORKER::send_task(TASK task, bool is_anchored)
    {
        ASSERT(this->tasks_.empty());
        info("[Worker %d] send_task from @thread=%s to @thread=%s with is_anchored=%s\n",
             this->worker_id_, to_string(std::this_thread::get_id()).c_str(),
             to_string(this->thread_id_).c_str(), bool_to_cstr(is_anchored));
        this->tasks_.emplace_back(TASK_HOLDER{std::move(task), is_anchored});
        this->update_tasks_status();
    }

    inline void WSPDR_WORKER::terminate()
    {
        debug("[Worker %d] terminate\n", this->worker_id_);

        this->terminate_notify_ = true;
    }

    inline void WSPDR_WORKER::status() const
    {
        std::string threda_id_str = to_string(this->thread_id_);
        std::string received_tasks_str = this->received_tasks_notify_ ? std::to_string(this->received_tasks_.size()) : "nullopt";
        warn("[Worker %d] @thread=%s, policy=%d, workers=%lu, tasks=%lu, tasks_done=%d, received_tasks=%s, request=%d, has_tasks=%s, terminate_notify=%s, is_alive=%s\n",
             this->worker_id_,
             threda_id_str.c_str(), static_cast<int>(this->policy_), this->workers_.size(), this->tasks_.size(), this->num_tasks_done_,
             received_tasks_str.c_str(), this->request_.load(),
             bool_to_cstr(this->has_tasks_), bool_to_cstr(this->terminate_notify_), bool_to_cstr(this->is_alive_));
    }

    inline void WSPDR_WORKER::add_task(TASK task)
    {
        ASSERT(std::this_thread::get_id() == this->thread_id_);
        this->tasks_.emplace_back(TASK_HOLDER{std::move(task), false});
        this->update_tasks_status();
    }

    inline bool WSPDR_WORKER::try_send_steal_request(int requester_worker_id)
    {
        if (this->has_tasks_)
        {
            // https://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange
            int no_request = NO_REQUEST;
            return this->request_.compare_exchange_strong(no_request, requester_worker_id);
        }
        return false;
    }

    inline void WSPDR_WORKER::distribute_task(std::vector<TASK> tasks)
    {
        ASSERT(!this->received_tasks_notify_);
        ASSERT(this->received_tasks_.empty());
        this->received_tasks_ = std::move(tasks);
        this->received_tasks_notify_ = true;
    }

    inline void WSPDR_WORKER::communicate()
    {
        int requester = this->request_;
        if (requester != NO_REQUEST)
        {
            if (this->tasks_.empty())
            {
                this->workers_[requester]->distribute_task({});
            }
            else
            {
                size_t num_tasks_to_send = 1;
                if (this->policy_ == WSPDR_POLICY::STEAL_HALF)
                {
                    num_tasks_to_send = this->tasks_.size() / 2;
                }
                std::vector<TASK> tasks_to_send;
                tasks_to_send.reserve(num_tasks_to_send);
                for (size_t itask = 0; itask < num_tasks_to_send; itask++)
                {
                    auto [t, is_anchored] = this->tasks_.front();
                    if (is_anchored)
                    {
                        break; // Stop right at the anchored task
                    }
                    else
                    {
                        this->tasks_.pop_front();
                        tasks_to_send.emplace_back(std::move(t));
                    }
                }
                this->workers_[requester]->distribute_task(std::move(tasks_to_send));
            }
            this->request_ = NO_REQUEST;
            this->update_tasks_status();
        }
    }

    inline bool WSPDR_WORKER::try_acquire_once()
    {
        int target_worker_id = std::rand() / ((RAND_MAX + 1u) / this->workers_.size());
        // Does not support self-steal
        if (target_worker_id != this->worker_id_)
        {
            if (this->workers_[target_worker_id]->is_alive() && this->workers_[target_worker_id]->try_send_steal_request(this->worker_id_))
            {
                // Request sent, now waiting for a response
                while (!this->received_tasks_notify_)
                {
                    // While waiting, still respond to other worker who has sent request to this worker
                    this->communicate();
                }
                std::vector<TASK> received_tasks = std::move(this->received_tasks_);
                this->received_tasks_.clear();
                this->received_tasks_notify_ = false;
                // Check whether the target worker sent real tasks to this worker
                if (!received_tasks.empty())
                {
                    for (const auto &received_task : received_tasks)
                    {
                        this->add_task(received_task);
                    }
                    debug("[Worker %d] acquired %lu task from worker %d, %lu tasks in the deque\n",
                          this->worker_id_, received_tasks.size(), target_worker_id, this->tasks_.size());
                    return true;
                }
            }
        }
        // While looking for target worker to steal from, still respond to other worker who has sent request to this worker
        this->communicate();
        return false;
    }

    inline void WSPDR_WORKER::update_tasks_status()
    {
        bool b = !this->tasks_.empty();
        if (this->has_tasks_ != b)
            this->has_tasks_ = b;
    }
}