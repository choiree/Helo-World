#include "hzpch.h"
#include "ThreadPool.h"

Hazel::ThreadPool::ThreadPool(uint32_t num_threads)
    : m_next_job_id(0), m_accepting_tasks(true), m_pending_count(0) {
    if (num_threads == 0) num_threads = 1;
    for (size_t i = 0; i < num_threads; ++i) {
        m_threads.emplace_back([this](std::stop_token stoken) {
            WorkerThread(stoken);
            });
    }
}

void Hazel::ThreadPool::ProcessMainThreadTasks()
{
    while (true) {
        TaskCallback task;
        {
            std::lock_guard<std::mutex> lock(m_main_thread_tasks_mutex);
            if (m_main_thread_tasks.empty())
                break;
            task = std::move(m_main_thread_tasks.front());
            m_main_thread_tasks.pop();
        }
        if (task) {
            try {
                task();
            }
            catch (const std::exception& e) {
                HZ_CORE_ERROR("[ThreadPool] Main thread callback exception: {0}", e.what());
            }
            catch (...) {
                HZ_CORE_ERROR("[ThreadPool] Main thread callback unknown exception");
            }
        }
    }
}

void Hazel::ThreadPool::WaitAll()
{
    while (m_pending_count.load(std::memory_order_acquire) > 0) {
        ProcessMainThreadTasks();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ProcessMainThreadTasks();
}

bool Hazel::ThreadPool::IsFinished(JobHandle job_id)
{
    std::lock_guard<std::mutex> lock(m_finished_mutex);
    return m_finished_jobs.find(job_id) != m_finished_jobs.end();
}

void Hazel::ThreadPool::ClearFinishedJobs()
{
    {
        std::lock_guard<std::mutex> lock(m_finished_mutex);
        m_finished_jobs.clear();
        while (!m_finished_order.empty())
            m_finished_order.pop();
    }
    std::lock_guard<std::mutex> lock(m_cancel_mutex);
    m_cancelled_jobs.clear();
}

bool Hazel::ThreadPool::Cancel(JobHandle job_id)
{
    {
        std::lock_guard<std::mutex> lock(m_finished_mutex);
        if (m_finished_jobs.find(job_id) != m_finished_jobs.end())
            return false;
    }
    std::lock_guard<std::mutex> lock(m_cancel_mutex);
    m_cancelled_jobs.insert(job_id);
    return true;
}

void Hazel::ThreadPool::WorkerThread(std::stop_token stoken)
{
    std::vector<std::unique_ptr<Task>> local_tasks;
    local_tasks.reserve(TASK_BATCH_SIZE);
    std::vector<JobHandle> local_finished;

    while (true) {
        CommitFinishedBatch(local_finished);

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_condition.wait(lock, stoken, [this] { return !m_tasks.empty(); }))
                break;

            for (size_t i = 0; i < TASK_BATCH_SIZE && !m_tasks.empty(); ++i) {
                local_tasks.push_back(std::move(m_tasks.front()));
                m_tasks.pop();
            }
        }

        for (auto& task : local_tasks) {
            bool isCancelled = false;
            {
                std::lock_guard<std::mutex> lock(m_cancel_mutex);
                isCancelled = (m_cancelled_jobs.erase(task->GetId()) > 0);
            }

            if (!isCancelled) {
                try {
                    task->Execute();
                }
                catch (const std::exception& e) {
                    HZ_CORE_ERROR("[ThreadPool] Worker task exception: {0}", e.what());
                }
                catch (...) {
                    HZ_CORE_ERROR("[ThreadPool] Worker task unknown exception");
                }
            }

            local_finished.push_back(task->GetId());
        }

        CommitFinishedBatch(local_finished);
        local_tasks.clear();
    }

    CommitFinishedBatch(local_finished);
}

void Hazel::ThreadPool::CommitFinishedBatch(std::vector<JobHandle>& batch)
{
    if (batch.empty()) return;
    {
        std::lock_guard<std::mutex> lock(m_finished_mutex);
        for (auto id : batch) {
            if (m_finished_jobs.size() >= MAX_FINISHED_JOBS) {
                uint64_t oldest = m_finished_order.front();
                m_finished_order.pop();
                m_finished_jobs.erase(oldest);
                HZ_ERROR("[ThreadPool] Finished jobs limit reached, dropped oldest job {0}", oldest);
            }
            m_finished_jobs.insert(id);
            m_finished_order.push(id);
        }
    }
    for (size_t i = 0; i < batch.size(); ++i) {
        m_pending_count.fetch_sub(1, std::memory_order_release);
    }
    batch.clear();
}

void Hazel::ThreadPool::PushMainThreadCallback(TaskCallback cb)
{
    std::lock_guard<std::mutex> lock(m_main_thread_tasks_mutex);
    if (m_main_thread_tasks.size() >= MAX_MAIN_THREAD_TASKS) {
        HZ_CORE_ERROR("[ThreadPool] Main thread callback queue full, dropping oldest callback");
        m_main_thread_tasks.pop();
    }
    m_main_thread_tasks.push(std::move(cb));
}

void Hazel::ThreadPool::Shutdown()
{
    m_accepting_tasks.store(false, std::memory_order_release);
    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_tasks.empty())
            break;
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    m_threads.clear();
    if (!m_main_thread_tasks.empty()) {
        HZ_CORE_ERROR("[ThreadPool] Shutdown with unprocessed main thread callbacks");
        HZ_ASSERT(false, "Unprocessed main thread callbacks")
    }
}

