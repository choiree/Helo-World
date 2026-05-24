#pragma once

#include <queue>
#include <unordered_set>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

namespace Hazel {

    class Task {
    public:
        Task() = default;

        template<class F, class... Args>
        Task(F&& f, Args&&... args) {
            m_task = [f = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable {
                std::invoke(f, std::forward<Args>(args)...);
                };
        }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        Task(Task&& other) noexcept
            : m_task(std::move(other.m_task)), m_task_id(other.m_task_id) {
            other.m_task_id = 0;
        }

        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                m_task = std::move(other.m_task);
                m_task_id = other.m_task_id;
                other.m_task_id = 0;
            }
            return *this;
        }

        void Execute() {
            if (m_task)
                m_task();
        }

        void SetId(uint64_t id) { m_task_id = id; }
        uint64_t GetId() const { return m_task_id; }

    private:
        std::function<void()> m_task;
        uint64_t m_task_id = 0;
    };


    class ThreadPool {
    public:
        using JobHandle = uint64_t;
        using TaskCallback = std::function<void()>;

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        explicit ThreadPool(uint32_t num_threads);
            
        ~ThreadPool() {
            Shutdown();
        }

        void ProcessMainThreadTasks();
        void WaitAll();
        bool IsFinished(JobHandle job_id);
        void ClearFinishedJobs();
        bool Cancel(JobHandle job_id);
        template<typename Func>
        JobHandle Submit(Func&& func) {
            return EnqueueInternal(std::forward<Func>(func));
        }

        template<class F, class... Args>
        JobHandle Submit(TaskCallback callback, F&& func, Args&&... args) {
            auto wrapped_task = [this, callback = std::move(callback),
                func = std::forward<F>(func),
                ...args = std::forward<Args>(args)]() mutable {
                func(std::forward<Args>(args)...);
                if (callback) {
                    PushMainThreadCallback(std::move(callback));
                }
                };
            return EnqueueInternal(wrapped_task);
        }

    private:
        template<typename Func>
        JobHandle EnqueueInternal(Func&& func) {
            if (!m_accepting_tasks.load(std::memory_order_acquire))
                throw std::runtime_error("ThreadPool is shutting down");

            auto task = std::make_unique<Task>(std::forward<Func>(func));
            JobHandle current_job_id;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                current_job_id = m_next_job_id.fetch_add(1);
                task->SetId(current_job_id);
                m_tasks.push(std::move(task));
            }
            m_pending_count.fetch_add(1, std::memory_order_release);
            m_condition.notify_one();
            return current_job_id;
        }

        void WorkerThread(std::stop_token stoken);
        void CommitFinishedBatch(std::vector<JobHandle>& batch);
        void PushMainThreadCallback(TaskCallback cb);
        void Shutdown();

    private:
        static constexpr size_t MAX_FINISHED_JOBS = 10'000;
        static constexpr size_t MAX_MAIN_THREAD_TASKS = 1'000;
        static constexpr size_t TASK_BATCH_SIZE = 16;

        std::mutex m_mutex;
        std::condition_variable_any m_condition;
        std::queue<std::unique_ptr<Task>> m_tasks;

        std::mutex m_finished_mutex;
        std::unordered_set<JobHandle> m_finished_jobs;
        std::queue<JobHandle> m_finished_order;

        std::mutex m_cancel_mutex;
        std::unordered_set<JobHandle> m_cancelled_jobs;

        std::mutex m_main_thread_tasks_mutex;
        std::queue<TaskCallback> m_main_thread_tasks;

        std::atomic<JobHandle> m_next_job_id{ 0 };
        std::atomic<bool> m_accepting_tasks{ true };
        std::atomic<size_t> m_pending_count{ 0 };
        std::vector<std::jthread> m_threads;
    };
}
