#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <optional>

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
    SUAP::~SUAP()
    {
        this->terminate();
    }

    void SUAP::start()
    {
        ASSERT(this->threads_.empty());
        ASSERT(this->threads_launch_channel_.empty());

        const size_t n_threads = this->num_workers();
        this->threads_launch_channel_.resize(n_threads);
        this->threads_.reserve(n_threads);
        for (size_t thread_id = 0; thread_id < n_threads; thread_id++)
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

    void SUAP::terminate()
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
}