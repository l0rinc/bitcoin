// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <checkqueue.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
struct DumbCheck {
    bool result = false;
    std::atomic<size_t>* calls = nullptr;

    explicit DumbCheck(const bool _result, std::atomic<size_t>* calls_in = nullptr)
        : result(_result), calls(calls_in)
    {
    }

    std::optional<int> operator()() const
    {
        if (calls != nullptr) {
            calls->fetch_add(1, std::memory_order_relaxed);
        }
        if (result) return std::nullopt;
        return 1;
    }
};

std::vector<DumbCheck> MakeChecks(const std::vector<bool>& results, std::atomic<size_t>* calls)
{
    std::vector<DumbCheck> checks;
    checks.reserve(results.size());
    for (const bool result : results) {
        checks.emplace_back(result, calls);
    }
    return checks;
}

std::optional<int> ExpectedResult(const std::vector<bool>& results)
{
    if (std::ranges::any_of(results, [](bool result) { return !result; })) {
        return 1;
    }
    return std::nullopt;
}

void AssertResult(const std::optional<int>& result, const std::optional<int>& expected)
{
    assert(result.has_value() == expected.has_value());
    if (result) assert(*result == *expected);
}
} // namespace

FUZZ_TARGET(checkqueue)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const unsigned int batch_size = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 1024);
    const int worker_threads_num = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 2);
    CCheckQueue<DumbCheck> check_queue_1{batch_size, worker_threads_num};
    CCheckQueue<DumbCheck> check_queue_2{batch_size, worker_threads_num};
    std::vector<bool> check_results;
    const int max_checks{worker_threads_num == 0 ? 1024 : 128};
    const int size = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, max_checks);
    for (int i = 0; i < size; ++i) {
        check_results.emplace_back(fuzzed_data_provider.ConsumeBool());
    }
    const bool add_checks{fuzzed_data_provider.ConsumeBool()};
    const bool complete_checks{fuzzed_data_provider.ConsumeBool()};
    const std::optional<int> expected_result{add_checks ? ExpectedResult(check_results) : std::nullopt};
    const bool expect_all_checks_run{add_checks && complete_checks && !expected_result.has_value()};

    std::atomic<size_t> direct_calls{0};
    if (add_checks) {
        check_queue_1.Add(MakeChecks(check_results, complete_checks ? &direct_calls : nullptr));
    }
    if (complete_checks) {
        const auto direct_result{check_queue_1.Complete()};
        AssertResult(direct_result, expected_result);
        AssertResult(check_queue_1.Complete(), std::nullopt);
        if (expect_all_checks_run) {
            assert(direct_calls.load(std::memory_order_relaxed) == check_results.size());
        }
    }

    {
        std::atomic<size_t> control_calls{0};
        CCheckQueueControl<DumbCheck> check_queue_control{check_queue_2};
        if (add_checks) {
            check_queue_control.Add(MakeChecks(check_results, &control_calls));
        }
        if (complete_checks) {
            const auto control_result{check_queue_control.Complete()};
            AssertResult(control_result, expected_result);
            AssertResult(check_queue_control.Complete(), std::nullopt);
            if (expect_all_checks_run) {
                assert(control_calls.load(std::memory_order_relaxed) == check_results.size());
            }
        }
    }
}
