// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <util/threadpool.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <future>
#include <queue>
#include <vector>

struct ExpectedException : std::runtime_error {
    explicit ExpectedException(const std::string& msg) : std::runtime_error(msg) {}
};

struct ThrowTask {
    void operator()() const { throw ExpectedException("fail"); }
};

struct CounterTask {
    std::atomic_uint32_t& m_counter;
    explicit CounterTask(std::atomic_uint32_t& counter) : m_counter{counter} {}
    void operator()() const { m_counter.fetch_add(1, std::memory_order_relaxed); }
};

// Waits for a future to complete. Increments 'fail_counter' if the expected exception is thrown.
static void GetFuture(std::future<void>& future, uint32_t& fail_counter)
{
    try {
        future.get();
    } catch (const ExpectedException&) {
        fail_counter++;
    }
}

static void AssertRejectedSubmissionsDoNotQueueTasks(FuzzedDataProvider& fuzzed_data_provider)
{
    ThreadPool pool{"fuzzrej"};
    std::atomic_uint32_t rejected_counter{0};

    auto submit_rejected_tasks = [&](ThreadPool::SubmitError expected_error) {
        auto single_result{pool.Submit(CounterTask{rejected_counter})};
        assert(!single_result);
        assert(single_result.error() == expected_error);
        assert(pool.WorkQueueSize() == 0);

        const uint32_t range_size{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 8)};
        std::vector<std::function<void()>> tasks;
        tasks.reserve(range_size);
        for (uint32_t idx{0}; idx < range_size; ++idx) {
            tasks.emplace_back(CounterTask{rejected_counter});
        }
        auto range_result{pool.Submit(std::move(tasks))};
        assert(!range_result);
        assert(range_result.error() == expected_error);
        assert(pool.WorkQueueSize() == 0);
    };

    submit_rejected_tasks(ThreadPool::SubmitError::Inactive);
    assert(rejected_counter.load(std::memory_order_relaxed) == 0);
    if (fuzzed_data_provider.ConsumeBool()) {
        pool.Start(1);
        auto future{*Assert(pool.Submit([] {}))};
        future.get();
        pool.Stop();
        assert(rejected_counter.load(std::memory_order_relaxed) == 0);
    }

    pool.Start(1);
    pool.Interrupt();
    submit_rejected_tasks(ThreadPool::SubmitError::Interrupted);
    pool.Stop();
    assert(rejected_counter.load(std::memory_order_relaxed) == 0);
}

// Global thread pool for fuzzing. Persisting it across iterations prevents
// the excessive thread creation/destruction overhead that can lead to
// instability in the fuzzing environment.
// This is also how we use it in the app's lifecycle.
ThreadPool g_pool{"fuzz"};
// Global to verify we always have the same number of threads.
size_t g_num_workers = 3;

static void StartPoolIfNeeded()
{
    if (g_pool.WorkersCount() == g_num_workers) return;
    g_pool.Start(g_num_workers);
}

static void setup_threadpool_test()
{
    // Disable logging entirely. It seems to cause memory leaks.
    LogInstance().DisableLogging();
}

FUZZ_TARGET(threadpool, .init = setup_threadpool_test)
{
    // Because LibAFL calls fork() after calling the init setup function,
    // the child processes end up having one thread active and no workers.
    // To work around this limitation, start thread pool inside the first runner.
    StartPoolIfNeeded();

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    if (fuzzed_data_provider.ConsumeBool()) {
        AssertRejectedSubmissionsDoNotQueueTasks(fuzzed_data_provider);
    }

    const uint32_t num_tasks = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 1024);
    assert(g_pool.WorkersCount() == g_num_workers);
    assert(g_pool.WorkQueueSize() == 0);

    // Counters
    std::atomic_uint32_t task_counter{0};
    uint32_t fail_counter{0};
    uint32_t expected_task_counter{0};
    uint32_t expected_fail_tasks{0};

    auto drain_future = [&](std::future<void>& future) {
        GetFuture(future, fail_counter);
    };

    if (fuzzed_data_provider.ConsumeBool()) {
        std::vector<std::function<void()>> empty_tasks;
        auto empty_futures{*Assert(g_pool.Submit(std::move(empty_tasks)))};
        assert(empty_futures.empty());
        assert(g_pool.WorkQueueSize() == 0);
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        const auto range_size{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 32)};
        std::vector<uint32_t> expected_results;
        std::vector<std::function<uint32_t()>> ordered_tasks;
        expected_results.reserve(range_size);
        ordered_tasks.reserve(range_size);
        for (uint32_t task_idx{0}; task_idx < range_size; ++task_idx) {
            const uint32_t result{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
            expected_results.push_back(result);
            ordered_tasks.emplace_back([result] { return result; });
        }
        auto ordered_futures{*Assert(g_pool.Submit(std::move(ordered_tasks)))};
        assert(ordered_futures.size() == expected_results.size());
        for (size_t idx{0}; idx < ordered_futures.size(); ++idx) {
            assert(ordered_futures[idx].get() == expected_results[idx]);
        }
        assert(g_pool.WorkQueueSize() == 0);
    }

    std::queue<std::future<void>> futures;
    for (uint32_t i = 0; i < num_tasks;) {
        const bool submit_range = fuzzed_data_provider.ConsumeBool();
        const bool wait_immediately = fuzzed_data_provider.ConsumeBool();

        if (submit_range) {
            const uint32_t range_size = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, std::min<uint32_t>(16, num_tasks - i));
            std::vector<std::function<void()>> tasks;
            tasks.reserve(range_size);
            for (uint32_t task_idx{0}; task_idx < range_size; ++task_idx) {
                const bool will_throw = fuzzed_data_provider.ConsumeBool();
                if (will_throw) {
                    expected_fail_tasks++;
                    tasks.emplace_back(ThrowTask{});
                } else {
                    expected_task_counter++;
                    tasks.emplace_back(CounterTask{task_counter});
                }
            }
            auto range_futures{*Assert(g_pool.Submit(std::move(tasks)))};
            assert(range_futures.size() == range_size);
            for (auto& fut : range_futures) {
                if (wait_immediately) {
                    drain_future(fut);
                } else {
                    futures.emplace(std::move(fut));
                }
            }
            i += range_size;
        } else {
            const bool will_throw = fuzzed_data_provider.ConsumeBool();
            std::future<void> fut;
            if (will_throw) {
                expected_fail_tasks++;
                fut = *Assert(g_pool.Submit(ThrowTask{}));
            } else {
                expected_task_counter++;
                fut = *Assert(g_pool.Submit(CounterTask{task_counter}));
            }

            // If caller wants to wait immediately, consume the future here (safe).
            if (wait_immediately) {
                // Waits for this task to complete immediately; prior queued tasks may also complete
                // as they were queued earlier.
                drain_future(fut);
            } else {
                // Store task for a posterior check
                futures.emplace(std::move(fut));
            }
            ++i;
        }
    }

    // Drain remaining futures
    while (!futures.empty()) {
        auto fut = std::move(futures.front());
        futures.pop();
        GetFuture(fut, fail_counter);
    }

    assert(g_pool.WorkQueueSize() == 0);
    assert(task_counter.load() == expected_task_counter);
    assert(fail_counter == expected_fail_tasks);
}
