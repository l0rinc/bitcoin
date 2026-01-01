// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/nanobench.h>

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(nanobench_tests)

BOOST_AUTO_TEST_CASE(run_setup_not_measured)
{
    // Keep the test fairly quick, but use enough time to avoid flakiness.
    static constexpr size_t NUM_EPOCHS{3};
    static constexpr auto SETUP_SLEEP{200ms};

    uint64_t baseline_counter{0};
    ankerl::nanobench::Bench baseline;
    baseline.output(nullptr).warmup(0).epochs(NUM_EPOCHS).epochIterations(1);
    baseline.run([&] {
        return ++baseline_counter;
    });
    const double baseline_elapsed_s =
        baseline.results().back().median(ankerl::nanobench::Result::Measure::elapsed);

    size_t setup_calls{0};
    uint64_t counter{0};
    ankerl::nanobench::Bench bench;
    bench.output(nullptr).warmup(0).epochs(NUM_EPOCHS).epochIterations(1);

    const auto wall_start = std::chrono::steady_clock::now();
    bench.setup([&] {
            ++setup_calls;
            std::this_thread::sleep_for(SETUP_SLEEP);
        })
        .run([&] { return ++counter; });
    const auto wall_elapsed = std::chrono::steady_clock::now() - wall_start;

    BOOST_CHECK_EQUAL(setup_calls, NUM_EPOCHS);
    BOOST_CHECK_GE(wall_elapsed, SETUP_SLEEP * NUM_EPOCHS);

    const double elapsed_s =
        bench.results().back().median(ankerl::nanobench::Result::Measure::elapsed);

    // If setup was accidentally included, the elapsed time would be around 200ms.
    BOOST_CHECK_LT(elapsed_s, 0.1);
    BOOST_CHECK_LT(std::abs(elapsed_s - baseline_elapsed_s), 0.1);
}

BOOST_AUTO_TEST_CASE(run_setup_state_is_passed)
{
    static constexpr size_t NUM_EPOCHS{2};
    static constexpr uint64_t EPOCH_ITERATIONS{3};

    std::vector<uint64_t> states(NUM_EPOCHS);
    size_t setup_calls{0};
    size_t bench_calls{0};

    ankerl::nanobench::Bench bench;
    bench.output(nullptr).warmup(0).epochs(NUM_EPOCHS).epochIterations(EPOCH_ITERATIONS);
    bench.setup([&]() -> uint64_t& {
            auto& state = states.at(setup_calls);
            state = (++setup_calls) * 100;
            return state;
        })
        .run([&](uint64_t& state) -> uint64_t& {
            ++bench_calls;
            return ++state;
        });

    BOOST_CHECK_EQUAL(setup_calls, NUM_EPOCHS);
    BOOST_CHECK_EQUAL(bench_calls, NUM_EPOCHS * EPOCH_ITERATIONS);

    BOOST_CHECK_EQUAL(states[0], 103U);
    BOOST_CHECK_EQUAL(states[1], 203U);
}

BOOST_AUTO_TEST_CASE(run_config_bench_only)
{
    static constexpr size_t NUM_EPOCHS{3};

    size_t void_calls{0};
    uint64_t value_counter{0};

    ankerl::nanobench::Bench().output(nullptr).warmup(0).epochs(NUM_EPOCHS).epochIterations(1)
        .run([&] { ++void_calls; });

    ankerl::nanobench::Bench().output(nullptr).warmup(0).epochs(NUM_EPOCHS).epochIterations(1)
        .run([&] { return ++value_counter; });

    BOOST_CHECK_EQUAL(void_calls, NUM_EPOCHS);
    BOOST_CHECK_EQUAL(value_counter, NUM_EPOCHS);
}

BOOST_AUTO_TEST_CASE(run_config_setup_bench_void)
{
    static constexpr size_t NUM_EPOCHS{3};

    size_t setup_void_calls{0};
    size_t bench_void_calls{0};
    size_t setup_value_calls{0};
    size_t bench_value_calls{0};

    ankerl::nanobench::Bench().output(nullptr).warmup(0).epochs(NUM_EPOCHS).epochIterations(1)
        .setup([&] { ++setup_void_calls; })
        .run([&] { ++bench_void_calls; });

    ankerl::nanobench::Bench().output(nullptr).warmup(0).epochs(NUM_EPOCHS).epochIterations(1)
        .setup([&] { return ++setup_value_calls; })
        .run([&](size_t state) { bench_value_calls += state; });

    BOOST_CHECK_EQUAL(setup_void_calls, NUM_EPOCHS);
    BOOST_CHECK_EQUAL(bench_void_calls, NUM_EPOCHS);
    BOOST_CHECK_EQUAL(setup_value_calls, NUM_EPOCHS);
    BOOST_CHECK_EQUAL(bench_value_calls, NUM_EPOCHS * (NUM_EPOCHS + 1) / 2);
}

BOOST_AUTO_TEST_SUITE_END()
