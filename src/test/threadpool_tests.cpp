// Copyright (c) 2025
// Distributed under the MIT software license.

#include <threadpool.h>

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <vector>

BOOST_AUTO_TEST_SUITE(threadpool_tests)

BOOST_AUTO_TEST_CASE(basic_run)
{
    ThreadPool pool{3};
    std::atomic counter{0};

    pool.Run([&](size_t){ counter.fetch_add(1, std::memory_order_relaxed); });
    BOOST_CHECK_EQUAL(counter.load(std::memory_order_relaxed), pool.Size());

    pool.Run([&](size_t){ counter.fetch_sub(1, std::memory_order_relaxed); });
    BOOST_CHECK_EQUAL(counter.load(std::memory_order_relaxed), 0);
}

BOOST_AUTO_TEST_CASE(index_coverage_once_per_worker)
{
    ThreadPool pool{4};
    std::vector<std::atomic<uint8_t>> seen(pool.Size());
    for (auto& s : seen) s.store(0, std::memory_order_relaxed);

    pool.Run([&](size_t i){
        BOOST_CHECK(i < pool.Size());
        seen[i].store(1, std::memory_order_relaxed);
    });

    for (size_t i{0}; i < pool.Size(); ++i) {
        BOOST_CHECK_EQUAL(seen[i].load(std::memory_order_relaxed), 1);
    }
}

BOOST_AUTO_TEST_CASE(exceptions_are_caught_and_no_deadlock)
{
    ThreadPool pool{5};
    std::atomic ok{0};

    pool.Run([&](size_t i){
        if (i == 1) { throw std::runtime_error("expected"); }
        ok.fetch_add(1, std::memory_order_relaxed);
    });

    // Two workers incremented; throwing worker was swallowed in worker loop.
    BOOST_CHECK_EQUAL(ok.load(std::memory_order_relaxed), pool.Size() - 1);
}

BOOST_AUTO_TEST_SUITE_END()
