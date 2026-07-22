// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/messages.h>
#include <consensus/amount.h>
#include <policy/fees/block_policy_estimator.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstdint>
#include <set>
#include <string_view>

using common::StringForFeeReason;

namespace {
constexpr CAmount MAX_FILTER_FEERATE{10'000'000};
constexpr double FEE_FILTER_SPACING{1.1};

std::set<CAmount> ExpectedFeeFilterValues(const CFeeRate& min_incremental_fee)
{
    std::set<CAmount> fee_values{0};
    const CAmount min_fee_limit{std::max(CAmount{1}, min_incremental_fee.GetFeePerK() / 2)};
    for (double fee_boundary{static_cast<double>(min_fee_limit)}; fee_boundary <= MAX_FILTER_FEERATE; fee_boundary *= FEE_FILTER_SPACING) {
        fee_values.insert(static_cast<CAmount>(fee_boundary));
    }
    return fee_values;
}

void AssertFeeFilterContract(const std::set<CAmount>& fee_values, const CAmount current_minimum_fee, const CAmount rounded_fee)
{
    assert(fee_values.contains(rounded_fee));
    assert(MoneyRange(rounded_fee));

    auto upper{fee_values.lower_bound(current_minimum_fee)};
    if (upper == fee_values.end()) upper = std::prev(fee_values.end());
    auto lower{upper};
    if (*upper >= current_minimum_fee && lower != fee_values.begin()) --lower;
    assert(rounded_fee == *lower || rounded_fee == *upper);
}

std::string_view ExpectedFeeReason(const FeeReason reason)
{
    switch (reason) {
    case FeeReason::NONE: return "None";
    case FeeReason::HALF_ESTIMATE: return "Half Target 60% Threshold";
    case FeeReason::FULL_ESTIMATE: return "Target 85% Threshold";
    case FeeReason::DOUBLE_ESTIMATE: return "Double Target 95% Threshold";
    case FeeReason::CONSERVATIVE: return "Conservative Double Target longer horizon";
    case FeeReason::MEMPOOL_MIN: return "Mempool Min Fee";
    case FeeReason::FALLBACK: return "Fallback fee";
    case FeeReason::REQUIRED: return "Minimum Required Fee";
    }
    assert(false);
}
} // namespace

FUZZ_TARGET(fees)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CFeeRate minimal_incremental_fee{ConsumeMoney(fuzzed_data_provider)};
    FastRandomContext rng{/*fDeterministic=*/true};
    FeeFilterRounder fee_filter_rounder{minimal_incremental_fee, rng};
    const auto fee_values{ExpectedFeeFilterValues(minimal_incremental_fee)};
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const CAmount current_minimum_fee = ConsumeMoney(fuzzed_data_provider);
        const CAmount rounded_fee = fee_filter_rounder.round(current_minimum_fee);
        AssertFeeFilterContract(fee_values, current_minimum_fee, rounded_fee);
    }
    for (const auto reason : std::array{FeeReason::NONE, FeeReason::HALF_ESTIMATE, FeeReason::FULL_ESTIMATE, FeeReason::DOUBLE_ESTIMATE, FeeReason::CONSERVATIVE, FeeReason::MEMPOOL_MIN, FeeReason::FALLBACK, FeeReason::REQUIRED}) {
        assert(StringForFeeReason(reason) == ExpectedFeeReason(reason));
    }
}
