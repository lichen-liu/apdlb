#pragma once
#include <thread>
#include <optional>
#include <vector>
#include <functional>
#include <deque>
#include <atomic>
#include <memory>

/// work stealing private dequeue - receiver

namespace TP
{
    using TASK = std::function<void()>;
    class WSPDR;

    class WSPDR_WORKER
    {
    public:
        void init(int worker_id, std::vector<WSPDR_WORKER *> workers)
        {
            this->set_worker_id(worker_id);
            this->set_worker_list(std::move(workers));
        }
        void run();               // Running on a thread
        void add_task(TASK task); // Not thread-safe
        void terminate();
        void status() const;
        bool is_alive() const { return this->is_alive_; }

    private:
        bool try_send_steal_request(int requester_worker_id);
        void distribute_task(TASK task);

        void set_worker_id(int worker_id) { this->worker_id_ = worker_id; }
        void set_worker_list(std::vector<WSPDR_WORKER *> workers) { this->workers_ = std::move(workers); }

        void communicate();
        bool try_acquire_once();
        void update_tasks_status();

    private:
        static constexpr int NO_REQUEST = -1;

    private:
        // struct TASK_HOLDER {
        //     TASK task;
        //     bool no_move
        // };
    private:
        std::deque<TASK> tasks_;
        std::vector<WSPDR_WORKER *> workers_; // back when using by self, front when using by other
        std::optional<TASK> received_task_opt_;
        std::thread::id thread_id_;
        int worker_id_ = -1;
        std::atomic<int> request_ = NO_REQUEST;
        bool has_tasks_ = false;
        bool should_terminate_ = false; // std::atomic?
        bool is_alive_ = false;
    };

    class WSPDR
    {
    public:
        explicit WSPDR(size_t num_workers);
        ~WSPDR();

        void start();
        void terminate();
        // A single session of execution, blocking until completed
        void execute(const std::vector<TASK> &tasks);
        void status() const;

    private:
        std::vector<std::unique_ptr<WSPDR_WORKER>> workers_;
        std::vector<std::thread> executors_;
    };
}