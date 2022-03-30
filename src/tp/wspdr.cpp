#include "wspdr.h"
#include "macros.hpp"
#include <cstdlib>

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

    void WSPDR_WORKER::acquire()
    {
        while (true)
        {
            this->received_task_opt_.reset();
            int target_worker_id = std::rand() / ((RAND_MAX + 1u) / this->workers_.size());
            // Retry if target_worker is this worker
            if (target_worker_id == this->worker_id_)
            {
                continue;
            }
            if (this->workers_[target_worker_id]->try_send_steal_request(this->worker_id_))
            {
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
                    return;
                }
            }
            // While looking for target worker to steal from, still respond to other worker who has sent request to this worker
            this->communicate();
        }
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