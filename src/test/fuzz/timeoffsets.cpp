// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/timeoffsets.h>
#include <node/warnings.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <vector>

void initialize_timeoffsets()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::MAIN);
}

namespace {
using namespace std::chrono_literals;

constexpr size_t MAX_TIME_OFFSETS{50};
constexpr std::chrono::minutes WARN_THRESHOLD{10};

void AddExpectedOffset(std::deque<std::chrono::seconds>& offsets, std::chrono::seconds offset)
{
    if (offsets.size() >= MAX_TIME_OFFSETS) {
        offsets.pop_front();
    }
    offsets.push_back(offset);
    assert(offsets.size() <= MAX_TIME_OFFSETS);
}

std::chrono::seconds ExpectedMedian(const std::deque<std::chrono::seconds>& offsets)
{
    if (offsets.size() < 5) return 0s;

    std::vector<std::chrono::seconds> sorted_offsets{offsets.begin(), offsets.end()};
    std::sort(sorted_offsets.begin(), sorted_offsets.end());
    assert(std::is_sorted(sorted_offsets.begin(), sorted_offsets.end()));
    const auto median{sorted_offsets[sorted_offsets.size() / 2]};
    assert(median >= sorted_offsets.front());
    assert(median <= sorted_offsets.back());
    return median;
}

bool ExpectedWarnIfOutOfSync(std::chrono::seconds median)
{
    const auto min_safe_median{std::chrono::seconds{std::numeric_limits<std::chrono::seconds::rep>::min() + 1}};
    median = std::max(median, min_safe_median);
    assert(median > std::chrono::seconds::min());
    return std::chrono::abs(median) > WARN_THRESHOLD;
}
} // namespace

FUZZ_TARGET(timeoffsets, .init = initialize_timeoffsets)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    node::Warnings warnings{};
    warnings.Unset(node::Warning::PRE_RELEASE_TEST_BUILD);
    TimeOffsets offsets{warnings};
    std::deque<std::chrono::seconds> expected_offsets;
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 4'000)
    {
        const auto expected_median{ExpectedMedian(expected_offsets)};
        assert(offsets.Median() == expected_median);

        const bool expected_warning{ExpectedWarnIfOutOfSync(expected_median)};
        assert(offsets.WarnIfOutOfSync() == expected_warning);
        assert(warnings.GetMessages().empty() == !expected_warning);

        const auto offset{std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<std::chrono::seconds::rep>()}};
        offsets.Add(offset);
    AddExpectedOffset(expected_offsets, offset);
    }
}
