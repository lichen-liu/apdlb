#include "wspdr.h"

namespace TP
{
    void WSPDR_WORKER::run()
    {
        while (true)
        {
            if (this->tasks_.empty())
            {
                if (this->should_terminate_)
                {
                    break;
                }
                acquire();
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

    void WSPDR_WORKER::communicate()
    {
    }

    void WSPDR_WORKER::acquire()
    {
    }

    void WSPDR_WORKER::update_status()
    {
        bool b = !this->tasks_.empty();
        if (this->has_tasks_ != b)
            this->has_tasks_ = b;
    }
    /*
THREAD_POOL::THREAD_POOL(size_t n_thread)
{
    resize(n_thread);
}

void THREAD_POOL::reset()
{
    auto thread_quit_event = []()
    { return false; };

    const size_t n_thread = size();
    for (size_t thread_id = 0; thread_id < n_thread; thread_id++)
    {
        bool is_sent = threads_launch_channel_[thread_id]
                           .try_send(thread_quit_event);
        ASSERT(is_sent);
        threads_[thread_id].join();
    }
    threads_.clear();
    threads_launch_channel_.clear();
}

void THREAD_POOL::resize(size_t n_thread)
{
    reset();

    if (n_thread == 0)
    {
        return;
    }

    threads_launch_channel_.resize(n_thread);
    threads_.reserve(n_thread);
    for (size_t thread_id = 0; thread_id < n_thread; thread_id++)
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
        threads_.emplace_back(std::move(thread_worker));
    }
    ASSERT(threads_launch_channel_.size() == threads_.size());
}
*/
}