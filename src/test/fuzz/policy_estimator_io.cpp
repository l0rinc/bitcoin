// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/block_policy_estimator_args.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/check.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <memory>

namespace {
const BasicTestingSetup* g_setup;

bool EqualBuckets(const EstimatorBucket& a, const EstimatorBucket& b)
{
    return a.start == b.start &&
           a.end == b.end &&
           a.withinTarget == b.withinTarget &&
           a.totalConfirmed == b.totalConfirmed &&
           a.inMempool == b.inMempool &&
           a.leftMempool == b.leftMempool;
}

bool EqualResults(const EstimationResult& a, const EstimationResult& b)
{
    return EqualBuckets(a.pass, b.pass) &&
           EqualBuckets(a.fail, b.fail) &&
           a.decay == b.decay &&
           a.scale == b.scale;
}

bool EqualFeeCalculations(const FeeCalculation& a, const FeeCalculation& b)
{
    return EqualResults(a.est, b.est) &&
           a.reason == b.reason &&
           a.desiredTarget == b.desiredTarget &&
           a.returnedTarget == b.returnedTarget &&
           a.best_height == b.best_height;
}

struct FeeEstimatorSnapshot {
    std::array<unsigned int, ALL_FEE_ESTIMATE_HORIZONS.size()> targets;
    std::array<CAmount, ALL_FEE_ESTIMATE_HORIZONS.size()> raw_fees;
    std::array<EstimationResult, ALL_FEE_ESTIMATE_HORIZONS.size()> raw_results;
    CAmount smart_fee;
    FeeCalculation smart_calc;
};

FeeEstimatorSnapshot SnapshotEstimator(const CBlockPolicyEstimator& estimator)
{
    FeeEstimatorSnapshot snapshot;
    for (size_t i{0}; i < ALL_FEE_ESTIMATE_HORIZONS.size(); ++i) {
        const auto horizon{ALL_FEE_ESTIMATE_HORIZONS[i]};
        snapshot.targets[i] = estimator.HighestTargetTracked(horizon);
        snapshot.raw_fees[i] = estimator.estimateRawFee(1, 0.0, horizon, &snapshot.raw_results[i]).GetFeePerK();
    }
    snapshot.smart_fee = estimator.estimateSmartFee(2, &snapshot.smart_calc, /*conservative=*/false).GetFeePerK();
    return snapshot;
}

void AssertEqualSnapshots(const FeeEstimatorSnapshot& a, const FeeEstimatorSnapshot& b)
{
    assert(a.targets == b.targets);
    assert(a.raw_fees == b.raw_fees);
    for (size_t i{0}; i < ALL_FEE_ESTIMATE_HORIZONS.size(); ++i) {
        assert(EqualResults(a.raw_results[i], b.raw_results[i]));
    }
    assert(a.smart_fee == b.smart_fee);
    assert(EqualFeeCalculations(a.smart_calc, b.smart_calc));
}
} // namespace

void initialize_policy_estimator_io()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(policy_estimator_io, .init = initialize_policy_estimator_io)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    AutoFile fuzzed_auto_file{fuzzed_file_provider.open()};
    // Reusing block_policy_estimator across runs to avoid costly creation of CBlockPolicyEstimator object.
    static CBlockPolicyEstimator block_policy_estimator{FeeestPath(*g_setup->m_node.args), DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    const std::array<unsigned int, ALL_FEE_ESTIMATE_HORIZONS.size()> targets_before{
        block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::SHORT_HALFLIFE),
        block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::MED_HALFLIFE),
        block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE),
    };
    const FeeEstimatorSnapshot snapshot_before{SnapshotEstimator(block_policy_estimator)};
    if (block_policy_estimator.Read(fuzzed_auto_file)) {
        const auto horizon{fuzzed_data_provider.PickValueInArray(ALL_FEE_ESTIMATE_HORIZONS)};
        const unsigned int max_target{block_policy_estimator.HighestTargetTracked(horizon)};
        Assert(max_target > 0);
        Assert(max_target <= 1008);

        EstimationResult result;
        const int raw_target{fuzzed_data_provider.ConsumeIntegralInRange<int>(1, max_target)};
        const double success_threshold{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 100) / 100.0};
        const CFeeRate raw_fee{block_policy_estimator.estimateRawFee(raw_target, success_threshold, horizon, &result)};
        Assert(raw_fee.GetFeePerK() >= 0);

        FeeCalculation fee_calc;
        const CFeeRate smart_fee{block_policy_estimator.estimateSmartFee(
            raw_target,
            fuzzed_data_provider.ConsumeBool() ? &fee_calc : nullptr,
            fuzzed_data_provider.ConsumeBool())};
        Assert(smart_fee.GetFeePerK() >= 0);

        block_policy_estimator.Write(fuzzed_auto_file);
    } else {
        Assert(targets_before[0] == block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::SHORT_HALFLIFE));
        Assert(targets_before[1] == block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::MED_HALFLIFE));
        Assert(targets_before[2] == block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE));
        AssertEqualSnapshots(snapshot_before, SnapshotEstimator(block_policy_estimator));
    }
    (void)fuzzed_auto_file.fclose();
}
