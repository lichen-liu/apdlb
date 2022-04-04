#pragma once

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <deque>
#include <memory>
#include <thread>
#include <vector>

#include "macros.hpp"
#include "message.hpp"
#include "pool.hpp"
#include "task.hpp"
#include "utils.hpp"

/// Work Stealing Private Deque pool - Receiver initiated

namespace TP
{
    class WSPDR_WORKER;
    class WSPDR : public POOL
    {
    public:
        explicit WSPDR(size_t num_workers) : POOL(num_workers) {}
        virtual ~WSPDR();

        virtual void start() override;
        virtual void terminate() override;
        // A single session of execution, blocking until completed
        virtual void execute(const std::vector<RAW_TASK> &tasks) override;
        virtual void status() const override;

    private:
        std::vector<std::unique_ptr<WSPDR_WORKER>> workers_;
        std::vector<std::thread> executors_;
    };

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
    inline WSPDR::~WSPDR()
    {
        this->terminate();
    }

    inline void WSPDR::start()
    {
        ASSERT(this->workers_.empty());
        ASSERT(this->executors_.empty());

        const size_t n_workers = this->num_workers();

        // Construct workers
        this->workers_.reserve(n_workers);
        std::generate_n(std::back_inserter(this->workers_), n_workers, []()
                        { return std::make_unique<WSPDR_WORKER>(); });

        // Initialize workers
        std::vector<WSPDR_WORKER *> worker_ptrs;
        worker_ptrs.reserve(n_workers);
        std::transform(this->workers_.begin(), this->workers_.end(), std::back_inserter(worker_ptrs), [](const auto &p)
                       { return p.get(); });
        for (size_t worker_id = 0; worker_id < n_workers; worker_id++)
        {
            this->workers_[worker_id]->init(worker_id, worker_ptrs);
        }

        // Initialize executors
        this->executors_.reserve(n_workers);
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
        this->workers_.clear();
        this->executors_.clear();
    }

    inline void WSPDR::execute(const std::vector<RAW_TASK> &tasks)
    {
        ASSERT(!tasks.empty());

        // Workers and executors must be launched already
        ASSERT(!this->workers_.empty());
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