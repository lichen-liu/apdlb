#pragma once

#include <algorithm>
#include <atomic>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "macros.hpp"
#include "pool.hpp"

/// Statically and Uniformly Assigned Private pool

namespace TP
{
    class SUAP_WORKER;
    class SUAP : public POOL
    {
    public:
        explicit SUAP(size_t num_workers) : POOL(num_workers) {}
        virtual ~SUAP();

        virtual void start() override;
        virtual void terminate() override;
        // A single session of execution, blocking until completed
        virtual void execute(const std::vector<RAW_TASK> &tasks) override;

    private:
        std::vector<std::unique_ptr<SUAP_WORKER>> workers_;
        std::vector<std::thread> executors_;
    };

    /// A channel with capacity 1 that supports backpressure.
    /// 1. The channel has a capacity of 1, i.e., no buffering.
    /// 2. A send call fulfills the channel, and holds the content until
    ///    a receive call happens.
    ///    The try_send call is non-blocking:
    ///    If the channel is empty, it populates the channel right away;
    ///    If the channel is full, it immediately fails.
    /// 3. A receive call clears out the channel.
    ///    The receive call is blocking:
    ///    If the channel is full, it clears the channel right away;
    ///    If the channel is empty, it blocks and waits until the channel is full.
    /// 4. A single send call pairs with a single receive call.
    /// std::condition_variable: https://en.cppreference.com/w/cpp/thread/condition_variable
    template <typename T>
    class CHANNEL_LITE
    {
    public:
        CHANNEL_LITE() : state_ptr_(std::make_unique<STATE>()) {}

        // False if the channel is already full; True if successfully sent
        [[nodiscard]] bool try_send(T data);
        T receive();

    private:
        struct STATE
        {
            std::optional<T> parcel;
            std::mutex mutex;
            std::condition_variable cv_has_parcel;
        };
        std::unique_ptr<STATE> state_ptr_;
    };

    class SUAP_WORKER
    {
    public:
        void run(); // Running on a thread
        void send_task(RAW_TASK task);
        void terminate();

    private:
        using thread_task_type = std::function<bool()>; // true to continue thread event loop; false to terminate
        CHANNEL_LITE<thread_task_type> task_launch_channel_;
    };

}

namespace TP
{
    inline SUAP::~SUAP()
    {
        this->terminate();
    }

    inline void SUAP::start()
    {
        ASSERT(this->workers_.empty());
        ASSERT(this->executors_.empty());

        const size_t n_workers = this->num_workers();

        // Consruct workers
        this->workers_.reserve(n_workers);
        std::generate_n(std::back_inserter(this->workers_), n_workers, []()
                        { return std::make_unique<SUAP_WORKER>(); });

        // Initialize executors
        this->executors_.reserve(n_workers);
        for (const auto &worker : this->workers_)
        {
            this->executors_.emplace_back(&SUAP_WORKER::run, worker.get());
        }
    }

    inline void SUAP::terminate()
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

    inline void SUAP::execute(const std::vector<RAW_TASK> &tasks)
    {
        ASSERT(!tasks.empty());

        // Workers and executors must be launched already
        ASSERT(!this->workers_.empty());
        ASSERT(!this->executors_.empty());

        const size_t num_tasks = tasks.size();
        const size_t n_workers = this->num_workers();
        size_t num_tasks_per_thread = (num_tasks - 1) / n_workers + 1;

        // Prepare thread master task
        std::vector<RAW_TASK> thread_master_tasks;
        std::atomic<size_t> n_workers_done = 0;
        size_t num_tasks_added = 0;
        for (size_t worker_id = 0; worker_id < n_workers; worker_id++)
        {
            if (num_tasks_added >= num_tasks)
            {
                continue;
            }
            const size_t num_tasks_for_current_thread = std::min(num_tasks_per_thread, num_tasks - num_tasks_added);
            num_tasks_added += num_tasks_for_current_thread;

            std::vector<RAW_TASK> thread_tasks(
                tasks.begin() + num_tasks_added - num_tasks_for_current_thread,
                tasks.begin() + num_tasks_added);

            auto thread_master_task = [thread_tasks = std::move(thread_tasks), &n_workers_done]()
            {
                for (const auto &task : thread_tasks)
                {
                    task();
                }
                n_workers_done++;
            };
            thread_master_tasks.emplace_back(std::move(thread_master_task));
        }

        // Launch
        const size_t n_workers_launched = thread_master_tasks.size();
        for (size_t worker_id = 0; worker_id < n_workers_launched; worker_id++)
        {
            this->workers_[worker_id]->send_task(thread_master_tasks[worker_id]);
        }

        // Synchronize
        while (n_workers_done.load() != n_workers_launched)
        {
        }
    }

    template <typename T>
    bool CHANNEL_LITE<T>::try_send(T data)
    {
        std::lock_guard<std::mutex> lock_guard(state_ptr_->mutex);
        if (state_ptr_->parcel.has_value())
        {
            return false;
        }
        else
        {
            state_ptr_->parcel.emplace(std::move(data));
        }
        state_ptr_->cv_has_parcel.notify_one();
        return true;
    }

    template <typename T>
    T CHANNEL_LITE<T>::receive()
    {
        std::unique_lock<std::mutex> unique_lock(state_ptr_->mutex);
        state_ptr_->cv_has_parcel
            .wait(unique_lock, [this]()
                  { return state_ptr_->parcel.has_value(); });
        T data = std::move(state_ptr_->parcel.value());
        state_ptr_->parcel.reset();
        return data;
    }

    inline void SUAP_WORKER::run()
    {
        while (true)
        {
            thread_task_type task = this->task_launch_channel_.receive(); // Blocking wait
            bool should_continue = task();
            if (!should_continue)
            {
                break;
            }
        }
    }

    inline void SUAP_WORKER::send_task(RAW_TASK task)
    {
        auto thread_task = [task = std::move(task)]
        {
            task();
            return true;
        };
        bool is_sent = this->task_launch_channel_.try_send(std::move(thread_task));
        ASSERT(is_sent);
    }

    inline void SUAP_WORKER::terminate()
    {
        static const auto thread_quit_task = []()
        { return false; };
        bool is_sent = this->task_launch_channel_.try_send(thread_quit_task);
        ASSERT(is_sent);
    }
}