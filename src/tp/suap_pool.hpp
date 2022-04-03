#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <optional>
#include <atomic>

#include "macros.hpp"
#include "pool.hpp"

/// Statically and Uniformly Assigned Private pool

namespace TP
{
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

    class SUAP : public POOL
    {
    public:
        explicit SUAP(size_t num_workers) : POOL(num_workers) {}
        virtual ~SUAP();

        virtual void start() override;
        virtual void terminate() override;
        // A single session of execution, blocking until completed
        virtual void execute(const std::vector<RAW_TASK> &tasks) override;

        // Function signature: void(size_t thread_id)
        template <typename Function>
        void run(Function &&f);

    private:
        using thread_event_type = std::function<bool()>; // true to continue thread event loop; false to terminate
        using thread_event_launch_channel = CHANNEL_LITE<thread_event_type>;
        std::vector<std::thread> threads_;
        std::vector<thread_event_launch_channel> threads_launch_channel_;
    };
}

namespace TP
{
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

    inline SUAP::~SUAP()
    {
        this->terminate();
    }

    inline void SUAP::start()
    {
        ASSERT(this->threads_.empty());
        ASSERT(this->threads_launch_channel_.empty());

        const size_t num_threads = this->num_workers();
        this->threads_launch_channel_.resize(num_threads);
        this->threads_.reserve(num_threads);
        for (size_t thread_id = 0; thread_id < num_threads; thread_id++)
        {
            auto thread_worker = [&ch = threads_launch_channel_[thread_id]]()
            {
                while (true)
                {
                    thread_event_type event = ch.receive(); // Blocking wait
                    bool should_continue = event();
                    if (!should_continue)
                    {
                        break;
                    }
                }
            };
            this->threads_.emplace_back(std::move(thread_worker));
        }
        ASSERT(this->threads_launch_channel_.size() == this->threads_.size());
    }

    inline void SUAP::terminate()
    {
        auto thread_quit_event = []()
        { return false; };

        for (auto &ch : this->threads_launch_channel_)
        {
            bool is_sent = ch.try_send(thread_quit_event);
            ASSERT(is_sent);
        }
        for (auto &thread : this->threads_)
        {
            thread.join();
        }
        this->threads_.clear();
        this->threads_launch_channel_.clear();
    }

    inline void SUAP::execute(const std::vector<RAW_TASK> &tasks)
    {
        const size_t num_tasks = tasks.size();
        const size_t num_threads = this->num_workers();
        size_t num_tasks_per_thread = (num_tasks - 1) / num_threads + 1;
        std::atomic<size_t> num_threads_done = 0;

        // Launch
        size_t num_tasks_added = 0;
        for (size_t thread_id = 0; thread_id < num_threads; thread_id++)
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

            auto thread_master_task = [thread_tasks = std::move(thread_tasks), &num_threads_done]()
            {
                for (const auto &task : thread_tasks)
                {
                    task();
                }
                num_threads_done++;
                return true;
            };
            bool is_sent = this->threads_launch_channel_[thread_id].try_send(std::move(thread_master_task));
            ASSERT(is_sent);
        }

        // Synchronize
        while (num_threads_done != num_threads)
        {
        }
    }
}