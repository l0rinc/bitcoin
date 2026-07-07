// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/mempool_entry.h>
#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/block_policy_estimator_args.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/setup_common.h>

#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

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
} // namespace

void initialize_policy_estimator()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(policy_estimator, .init = initialize_policy_estimator)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    bool good_data{true};

    CBlockPolicyEstimator block_policy_estimator{FeeestPath(*g_setup->m_node.args), DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};

    uint32_t current_height{0};
    const auto advance_height{
        [&] { current_height = fuzzed_data_provider.ConsumeIntegralInRange<decltype(current_height)>(current_height, 1 << 30); },
    };
    advance_height();
    LIMITED_WHILE (good_data && fuzzed_data_provider.ConsumeBool(), 10'000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
                if (!mtx) {
                    good_data = false;
                    return;
                }
                const CTransaction tx{*mtx};
                const auto entry{ConsumeTxMemPoolEntry(fuzzed_data_provider, tx, current_height)};
                const auto tx_submitted_in_package = fuzzed_data_provider.ConsumeBool();
                const auto tx_has_mempool_parents = fuzzed_data_provider.ConsumeBool();
                const auto tx_info = NewMempoolTransactionInfo(entry.GetSharedTx(), entry.GetFee(),
                                                               entry.GetTxSize(), entry.GetHeight(),
                                                               /*mempool_limit_bypassed=*/false,
                                                               tx_submitted_in_package,
                                                               /*chainstate_is_current=*/true,
                                                               tx_has_mempool_parents);
                block_policy_estimator.processTransaction(tx_info);
                if (fuzzed_data_provider.ConsumeBool()) {
                    (void)block_policy_estimator.removeTx(tx.GetHash());
                }
            },
            [&] {
                std::list<CTxMemPoolEntry> mempool_entries;
                LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
                    const std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
                    if (!mtx) {
                        good_data = false;
                        break;
                    }
                    const CTransaction tx{*mtx};
                    mempool_entries.push_back(ConsumeTxMemPoolEntry(fuzzed_data_provider, tx, current_height));
                }
                std::vector<RemovedMempoolTransactionInfo> txs;
                txs.reserve(mempool_entries.size());
                for (const CTxMemPoolEntry& mempool_entry : mempool_entries) {
                    txs.emplace_back(mempool_entry);
                }
                advance_height();
                block_policy_estimator.processBlock(txs, current_height);
            },
            [&] {
                (void)block_policy_estimator.removeTx(Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)));
            },
            [&] {
                block_policy_estimator.FlushUnconfirmed();
            });
        const int legacy_target{fuzzed_data_provider.ConsumeIntegral<int>()};
        const CFeeRate legacy_fee{block_policy_estimator.estimateFee(legacy_target)};
        assert(legacy_fee.GetFeePerK() >= 0);
        if (legacy_target == 1) {
            assert(legacy_fee == CFeeRate(0));
        } else {
            assert(legacy_fee == block_policy_estimator.estimateRawFee(
                                     legacy_target, 0.95, FeeEstimateHorizon::MED_HALFLIFE));
        }
        EstimationResult result;
        result.pass.start = 17;
        result.pass.end = 19;
        result.pass.withinTarget = 23;
        result.pass.totalConfirmed = 29;
        result.pass.inMempool = 31;
        result.pass.leftMempool = 37;
        result.fail.start = 41;
        result.fail.end = 43;
        result.fail.withinTarget = 47;
        result.fail.totalConfirmed = 53;
        result.fail.inMempool = 59;
        result.fail.leftMempool = 61;
        result.decay = 67;
        result.scale = 71;
        const EstimationResult result_before{result};
        auto conf_target = fuzzed_data_provider.ConsumeIntegral<int>();
        constexpr double invalid_thresholds[]{
            -std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::quiet_NaN(),
            -1.0,
            1.0 + std::numeric_limits<double>::epsilon(),
            std::numeric_limits<double>::infinity(),
        };
        const double success_threshold = fuzzed_data_provider.ConsumeBool() ?
            fuzzed_data_provider.ConsumeFloatingPoint<double>() :
            fuzzed_data_provider.PickValueInArray(invalid_thresholds);
        auto horizon = fuzzed_data_provider.PickValueInArray(ALL_FEE_ESTIMATE_HORIZONS);
        auto* result_ptr = fuzzed_data_provider.ConsumeBool() ? &result : nullptr;
        const CFeeRate raw_fee{block_policy_estimator.estimateRawFee(conf_target, success_threshold, horizon, result_ptr)};
        assert(raw_fee.GetFeePerK() >= 0);
        if (!std::isfinite(success_threshold) || success_threshold < 0 || success_threshold > 1) {
            assert(raw_fee == CFeeRate(0));
            if (result_ptr) assert(EqualResults(result, result_before));
        }

        FeeCalculation fee_calculation;
        fee_calculation.est.pass.start = 73;
        fee_calculation.est.fail.start = 79;
        fee_calculation.est.decay = 83;
        fee_calculation.est.scale = 89;
        fee_calculation.reason = FeeReason::REQUIRED;
        fee_calculation.desiredTarget = 97;
        fee_calculation.returnedTarget = 101;
        fee_calculation.best_height = std::numeric_limits<unsigned int>::max();
        conf_target = fuzzed_data_provider.ConsumeIntegral<int>();
        const int smart_conf_target{conf_target};
        auto* fee_calc_ptr = fuzzed_data_provider.ConsumeBool() ? &fee_calculation : nullptr;
        auto conservative = fuzzed_data_provider.ConsumeBool();
        const CFeeRate smart_fee{block_policy_estimator.estimateSmartFee(conf_target, fee_calc_ptr, conservative)};
        assert(smart_fee.GetFeePerK() >= 0);
        if (fee_calc_ptr) {
            assert(fee_calculation.desiredTarget == smart_conf_target);
            assert(fee_calculation.best_height != std::numeric_limits<unsigned int>::max());
            const auto highest_target{block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE)};
            if (smart_conf_target <= 0 || smart_conf_target > static_cast<int>(highest_target)) {
                assert(smart_fee == CFeeRate(0));
                assert(fee_calculation.returnedTarget == smart_conf_target);
                assert(fee_calculation.reason == FeeReason::NONE);
                assert(EqualResults(fee_calculation.est, EstimationResult{}));
            }
        }

        (void)block_policy_estimator.HighestTargetTracked(fuzzed_data_provider.PickValueInArray(ALL_FEE_ESTIMATE_HORIZONS));
    }
    {
        FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
        AutoFile fuzzed_auto_file{fuzzed_file_provider.open()};
        block_policy_estimator.Write(fuzzed_auto_file);
        block_policy_estimator.Read(fuzzed_auto_file);
        (void)fuzzed_auto_file.fclose();
    }
}
