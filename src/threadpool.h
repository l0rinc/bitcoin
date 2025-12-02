#ifndef BITCOIN_THREADPOOL_H
#define BITCOIN_THREADPOOL_H

#include <logging.h>
#include <tinyformat.h>
#include <util/threadnames.h>

#include <barrier>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

class ThreadPool
{
    std::barrier<> m_barrier;
    std::vector<std::thread> m_threads;
    std::function<void(size_t)> m_task;

    void Loop(size_t thread_index) noexcept
    {
        LogDebug(BCLog::ALL, "Thread %s started.", thread_index);
        util::ThreadRename(std::string{strprintf("pool.%d", thread_index)});
        for (;;) {
            m_barrier.arrive_and_wait(); // Wait for work
            try {
                if (!m_task) break;
                m_task(thread_index);
            } catch (const std::runtime_error& e) {
                LogWarning("ThreadPool error for #%s: %s.", thread_index, e.what());
            }
            m_barrier.arrive_and_wait(); // Signal completion
        }
        m_barrier.arrive_and_drop();
    }

public:
    explicit ThreadPool(size_t size) : m_barrier{std::ptrdiff_t(size + 1)}
    {
        m_threads.reserve(size);
        for (size_t i{0}; i < size; ++i) m_threads.emplace_back(&ThreadPool::Loop, this, i);
    }

    ~ThreadPool() noexcept
    {
        m_task = {}; // Shutdown signal
        m_barrier.arrive_and_wait(); // Signal termination
        for (auto& t : m_threads) t.join();
    }

    template <typename Task>
    void Run(Task&& task)
    {
        m_task = std::forward<Task>(task);
        m_barrier.arrive_and_wait(); // Start work
        m_barrier.arrive_and_wait(); // Wait for completion
    }

    size_t Size() const { return m_threads.size(); }
};

#endif // BITCOIN_THREADPOOL_H
