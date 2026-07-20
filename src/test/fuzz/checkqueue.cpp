// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <checkqueue.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
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

std::optional<int> ExpectedResult(const std::vector<DumbCheck>& checks)
{
    for (const DumbCheck& check : checks) {
        if (!check.result) return 1;
    }
    return std::nullopt;
}
} // namespace

FUZZ_TARGET(checkqueue)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const unsigned int batch_size = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 1024);
    CCheckQueue<DumbCheck> check_queue_1{batch_size, /*worker_threads_num=*/0};
    CCheckQueue<DumbCheck> check_queue_2{batch_size, /*worker_threads_num=*/0};
    std::vector<DumbCheck> checks_1;
    std::vector<DumbCheck> checks_2;
    const int size = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 1024);
    for (int i = 0; i < size; ++i) {
        const bool result = fuzzed_data_provider.ConsumeBool();
        checks_1.emplace_back(result);
        checks_2.emplace_back(result);
    }
    const bool add_to_queue_1{fuzzed_data_provider.ConsumeBool()};
    const std::optional<int> expected_result_1{add_to_queue_1 ? ExpectedResult(checks_1) : std::nullopt};
    if (add_to_queue_1) {
        check_queue_1.Add(std::move(checks_1));
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::optional<int> result{check_queue_1.Complete()};
        assert(result.has_value() == expected_result_1.has_value());
        if (result) assert(*result == *expected_result_1);
    }

    CCheckQueueControl<DumbCheck> check_queue_control{check_queue_2};
    const bool add_to_queue_2{fuzzed_data_provider.ConsumeBool()};
    const std::optional<int> expected_result_2{add_to_queue_2 ? ExpectedResult(checks_2) : std::nullopt};
    if (add_to_queue_2) {
        check_queue_control.Add(std::move(checks_2));
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::optional<int> result{check_queue_control.Complete()};
        assert(result.has_value() == expected_result_2.has_value());
        if (result) assert(*result == *expected_result_2);
    }
}
