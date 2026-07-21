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

#include <cstddef>
#include <cstdio>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;

std::vector<std::byte> CaptureEstimatorState(CBlockPolicyEstimator& estimator)
{
    AutoFile file{std::tmpfile()};
    Assert(!file.IsNull());
    Assert(estimator.Write(file));
    const auto size{file.size()};
    file.seek(0, SEEK_SET);
    std::vector<std::byte> state(size);
    file.read(state);
    Assert(file.fclose() == 0);
    return state;
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
    uint32_t expected_best_height{0};
    std::set<Txid> expected_tracked;
    uint32_t transition_count{0};
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
                const auto tx_has_no_mempool_parents = fuzzed_data_provider.ConsumeBool();
                const auto txid{tx.GetHash()};
                const bool expected_already_tracked{expected_tracked.contains(txid)};
                const bool expected_eligible{
                    !expected_already_tracked && entry.GetHeight() == expected_best_height &&
                    !tx_submitted_in_package && tx_has_no_mempool_parents};
                const auto tx_info = NewMempoolTransactionInfo(entry.GetSharedTx(), entry.GetFee(),
                                                               entry.GetTxSize(), entry.GetHeight(),
                                                               /*mempool_limit_bypassed=*/false,
                                                               tx_submitted_in_package,
                                                               /*chainstate_is_current=*/true,
                                                               tx_has_no_mempool_parents);
                block_policy_estimator.processTransaction(tx_info);
                if (expected_eligible) expected_tracked.insert(txid);
                if (fuzzed_data_provider.ConsumeBool()) {
                    const bool expected_removed{expected_tracked.contains(txid)};
                    const bool removed{block_policy_estimator.removeTx(txid)};
                    assert(removed == expected_removed);
                    if (expected_removed) expected_tracked.erase(txid);
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
                if (current_height > expected_best_height) {
                    expected_best_height = current_height;
                    for (const auto& tx : txs) {
                        expected_tracked.erase(tx.info.m_tx->GetHash());
                    }
                }
            },
            [&] {
                const auto txid{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider))};
                const bool expected_removed{expected_tracked.contains(txid)};
                const bool removed{block_policy_estimator.removeTx(txid)};
                assert(removed == expected_removed);
                if (expected_removed) expected_tracked.erase(txid);
            },
            [&] {
                block_policy_estimator.FlushUnconfirmed();
                expected_tracked.clear();
            });
        const bool check_read_only_state{(transition_count++ & 0x3f) == 0};
        std::vector<std::byte> before_queries;
        if (check_read_only_state) before_queries = CaptureEstimatorState(block_policy_estimator);
        const auto estimate_fee{block_policy_estimator.estimateFee(fuzzed_data_provider.ConsumeIntegral<int>())};
        assert(estimate_fee.GetFeePerK() >= 0);
        EstimationResult result;
        auto conf_target = fuzzed_data_provider.ConsumeIntegral<int>();
        auto success_threshold = fuzzed_data_provider.ConsumeFloatingPoint<double>();
        auto horizon = fuzzed_data_provider.PickValueInArray(ALL_FEE_ESTIMATE_HORIZONS);
        auto* result_ptr = fuzzed_data_provider.ConsumeBool() ? &result : nullptr;
        const auto raw_fee{block_policy_estimator.estimateRawFee(conf_target, success_threshold, horizon, result_ptr)};
        assert(raw_fee.GetFeePerK() >= 0);
        if (result_ptr && result.scale != 0) assert(result.decay > 0 && result.decay < 1);

        FeeCalculation fee_calculation;
        conf_target = fuzzed_data_provider.ConsumeIntegral<int>();
        auto* fee_calc_ptr = fuzzed_data_provider.ConsumeBool() ? &fee_calculation : nullptr;
        auto conservative = fuzzed_data_provider.ConsumeBool();
        const auto smart_fee{block_policy_estimator.estimateSmartFee(conf_target, fee_calc_ptr, conservative)};
        assert(smart_fee.GetFeePerK() >= 0);

        const auto short_target{block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::SHORT_HALFLIFE)};
        const auto medium_target{block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::MED_HALFLIFE)};
        const auto long_target{block_policy_estimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE)};
        assert(short_target > 0 && short_target <= medium_target && medium_target <= long_target);
        if (fee_calc_ptr) assert(fee_calculation.best_height == expected_best_height);
        if (check_read_only_state) assert(CaptureEstimatorState(block_policy_estimator) == before_queries);
    }
    {
        FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
        AutoFile fuzzed_auto_file{fuzzed_file_provider.open()};
        block_policy_estimator.Write(fuzzed_auto_file);
        block_policy_estimator.Read(fuzzed_auto_file);
        (void)fuzzed_auto_file.fclose();
    }

    for (const auto& txid : expected_tracked) {
        assert(block_policy_estimator.removeTx(txid));
    }
}
