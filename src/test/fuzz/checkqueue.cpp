// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <checkqueue.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
struct DumbCheck {
    bool result = false;

    explicit DumbCheck(const bool _result) : result(_result)
    {
    }

    std::optional<int> operator()() const
    {
        if (result) return std::nullopt;
        return 1;
    }
};

std::vector<DumbCheck> MakeChecks(const std::vector<bool>& results)
{
    std::vector<DumbCheck> checks;
    checks.reserve(results.size());
    for (const bool result : results) {
        checks.emplace_back(result);
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
    CCheckQueue<DumbCheck> check_queue_1{batch_size, /*worker_threads_num=*/0};
    CCheckQueue<DumbCheck> check_queue_2{batch_size, /*worker_threads_num=*/0};
    std::vector<bool> check_results;
    const int size = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 1024);
    for (int i = 0; i < size; ++i) {
        check_results.emplace_back(fuzzed_data_provider.ConsumeBool());
    }
    const bool add_checks{fuzzed_data_provider.ConsumeBool()};
    const bool complete_checks{fuzzed_data_provider.ConsumeBool()};
    const std::optional<int> expected_result{add_checks ? ExpectedResult(check_results) : std::nullopt};

    if (add_checks) {
        check_queue_1.Add(MakeChecks(check_results));
    }
    if (complete_checks) {
        const auto direct_result{check_queue_1.Complete()};
        AssertResult(direct_result, expected_result);
        AssertResult(check_queue_1.Complete(), std::nullopt);
    }

    {
        CCheckQueueControl<DumbCheck> check_queue_control{check_queue_2};
        if (add_checks) {
            check_queue_control.Add(MakeChecks(check_results));
        }
        if (complete_checks) {
            const auto control_result{check_queue_control.Complete()};
            AssertResult(control_result, expected_result);
            AssertResult(check_queue_control.Complete(), std::nullopt);
        }
    }
}
