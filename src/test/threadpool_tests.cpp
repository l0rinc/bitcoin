// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/threadpool.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(threadpool_tests)

BOOST_AUTO_TEST_CASE(threadpool_basic)
{
    // Test Cases
    // 1) Submit tasks and verify completion.
    // 2) Maintain all threads busy except one.
    // 3) Wait for work to finish.
    // 4) Wait for result object.
    // 5) The task throws an exception, catch must be done in the consumer side.
    // 6) Busy workers, help them by processing tasks from outside.

    const int NUM_WORKERS_DEFAULT = 3;

    // Test case 1, submit tasks and verify completion.
    {
        int num_tasks = 50;

        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::atomic<int> counter = 0;

        // Store futures to ensure completion before checking counter.
        std::vector<std::future<void>> futures;
        futures.reserve(num_tasks);

        for (int i = 1; i <= num_tasks; i++) {
            futures.emplace_back(threadPool.Submit([&counter, i]() {
                counter.fetch_add(i);
            }));
        }

        // Wait for all tasks to finish
        for (auto& fut : futures) fut.wait();
        int expected_value = (num_tasks * (num_tasks + 1)) / 2; // Gauss sum.
        BOOST_CHECK_EQUAL(counter.load(), expected_value);
    }

    // Test case 2, maintain all threads busy except one.
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::promise<void> blocker;
        std::shared_future<void> blocker_future(blocker.get_future());
        for (int i = 0; i < NUM_WORKERS_DEFAULT - 1; i++) {
            threadPool.Submit([blocker_future]() {
                blocker_future.wait();
            });
        }

        // Now execute tasks on the single available worker
        // and check that all the tasks are executed.
        int num_tasks = 15;
        std::atomic<int> counter = 0;

        // Store futures to wait on
        std::vector<std::future<void>> futures;
        futures.reserve(num_tasks);
        for (int i = 0; i < num_tasks; i++) {
            futures.emplace_back(threadPool.Submit([&counter]() {
                counter.fetch_add(1);
            }));
        }

        for (auto& fut : futures) fut.wait();
        BOOST_CHECK_EQUAL(counter.load(), num_tasks);

        blocker.set_value();
        threadPool.Stop();
        BOOST_CHECK_EQUAL(threadPool.WorkersCount(), 0);
    }

    // Test case 3, wait for work to finish.
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::atomic<bool> flag = false;
        std::future<void> future = threadPool.Submit([&flag]() {
            std::this_thread::sleep_for(std::chrono::milliseconds{200});
            flag.store(true);
        });
        future.wait();
        BOOST_CHECK(flag.load());
    }

    // Test case 4, wait for result object.
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::future<bool> future_bool = threadPool.Submit([]() {
            return true;
        });
        BOOST_CHECK(future_bool.get());

        std::future<std::string> future_str = threadPool.Submit([]() {
            return std::string("true");
        });
        BOOST_CHECK_EQUAL(future_str.get(), "true");
    }

    // Test case 5, throw exception and catch it on the consumer side.
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);

        int ROUNDS = 5;
        std::string err_msg{"something wrong happened"};
        std::vector<std::future<void>> futures;
        futures.reserve(ROUNDS);
        for (int i = 0; i < ROUNDS; i++) {
            futures.emplace_back(threadPool.Submit([err_msg, i]() {
                throw std::runtime_error(err_msg + util::ToString(i));
            }));
        }

        for (int i = 0; i < ROUNDS; i++) {
            try {
                futures.at(i).get();
                BOOST_FAIL("Expected exception not thrown");
            } catch (const std::runtime_error& e) {
                BOOST_CHECK_EQUAL(e.what(), err_msg + util::ToString(i));
            }
        }
    }

    // Test case 6, all workers are busy, help them by processing tasks from outside.
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);

        std::promise<void> blocker;
        std::shared_future<void> blocker_future(blocker.get_future());

        // Submit infinite blocking tasks that wait forever
        for (int i = 0; i < NUM_WORKERS_DEFAULT; i++) {
            threadPool.Submit([blocker_future]() {
                blocker_future.wait();
            });
        }

        // Now submit tasks and check that none of them are executed.
        int num_tasks = 20;
        std::atomic<int> counter = 0;
        for (int i = 0; i < num_tasks; i++) {
            threadPool.Submit([&counter]() {
                counter.fetch_add(1);
            });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 20);

        // Now process manually
        for (int i = 0; i < num_tasks; i++) {
            threadPool.ProcessTask();
        }
        BOOST_CHECK_EQUAL(counter.load(), num_tasks);
        blocker.set_value();
        threadPool.Stop();
    }
}

BOOST_AUTO_TEST_SUITE_END()
