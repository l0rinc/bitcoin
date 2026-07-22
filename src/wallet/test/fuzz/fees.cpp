// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

namespace wallet {
namespace {

struct FeeEstimatorTestingSetup : public TestingSetup {
    FeeEstimatorTestingSetup(const ChainType chain_type, TestOpts opts) : TestingSetup{chain_type, opts}
    {
    }

    ~FeeEstimatorTestingSetup() {
        m_node.fee_estimator.reset();
    }

    void SetFeeEstimator(std::unique_ptr<CBlockPolicyEstimator> fee_estimator)
    {
        m_node.fee_estimator = std::move(fee_estimator);
    }
};

FeeEstimatorTestingSetup* g_setup;

class FuzzedBlockPolicyEstimator : public CBlockPolicyEstimator
{
    FuzzedDataProvider& fuzzed_data_provider;
    mutable CFeeRate last_estimate;

public:
    FuzzedBlockPolicyEstimator(FuzzedDataProvider& provider)
        : CBlockPolicyEstimator(fs::path{}, false), fuzzed_data_provider(provider) {}

    CFeeRate estimateSmartFee(int confTarget, FeeCalculation* feeCalc, bool conservative) const override
    {
        last_estimate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/1'000'000)};
        return last_estimate;
    }

    unsigned int HighestTargetTracked(FeeEstimateHorizon horizon) const override
    {
        return fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 1000);
    }

    CFeeRate LastEstimate() const { return last_estimate; }
};

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<FeeEstimatorTestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(wallet_fees, .init = initialize_setup)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    auto& node{g_setup->m_node};
    Chainstate* chainstate = &node.chainman->ActiveChainstate();

    bilingual_str error;
    CTxMemPool::Options mempool_opts{
        .incremental_relay_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, 1'000'000)},
        .min_relay_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, 1'000'000)},
        .dust_relay_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, 1'000'000)}
    };
    node.mempool = std::make_unique<CTxMemPool>(mempool_opts, error);
    auto fee_estimator = std::make_unique<FuzzedBlockPolicyEstimator>(fuzzed_data_provider);
    FuzzedBlockPolicyEstimator* fee_estimator_ptr{fee_estimator.get()};
    g_setup->SetFeeEstimator(std::move(fee_estimator));
    auto target_feerate{CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/1'000'000)}};
    if (target_feerate > node.mempool->m_opts.incremental_relay_feerate &&
        target_feerate > node.mempool->m_opts.min_relay_feerate) {
        MockMempoolMinFee(target_feerate, *node.mempool);
    }
    std::unique_ptr<CWallet> wallet_ptr{std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase())};
    CWallet& wallet{*wallet_ptr};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetLastBlockProcessed(chainstate->m_chain.Height(), chainstate->m_chain.Tip()->GetBlockHash());
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        wallet.m_fallback_fee = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        wallet.m_discard_rate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }
    const CFeeRate discard_rate{GetDiscardRate(wallet)};
    const CFeeRate discard_estimate{fee_estimator_ptr->LastEstimate()};
    const CFeeRate expected_discard_rate{std::max(
        discard_estimate == CFeeRate{0} ? wallet.m_discard_rate : std::min(discard_estimate, wallet.m_discard_rate),
        wallet.chain().relayDustFee())};
    assert(discard_rate == expected_discard_rate);
    assert(discard_rate >= wallet.chain().relayDustFee());

    const auto tx_bytes{fuzzed_data_provider.ConsumeIntegralInRange(0, std::numeric_limits<int32_t>::max())};
    if (fuzzed_data_provider.ConsumeBool()) {
        wallet.m_min_fee = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }

    const CFeeRate required_fee_rate{GetRequiredFeeRate(wallet)};
    assert(required_fee_rate == std::max(wallet.m_min_fee, wallet.chain().relayMinFee()));
    assert(required_fee_rate >= wallet.m_min_fee);
    assert(required_fee_rate >= wallet.chain().relayMinFee());
    assert(GetRequiredFee(wallet, tx_bytes) == required_fee_rate.GetFee(tx_bytes));

    CCoinControl coin_control;
    if (fuzzed_data_provider.ConsumeBool()) {
        coin_control.m_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        coin_control.m_confirm_target = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 999'000);
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        coin_control.m_fee_mode = fuzzed_data_provider.ConsumeBool() ? FeeEstimateMode::CONSERVATIVE : FeeEstimateMode::ECONOMICAL;
    }

    FeeCalculation fee_calculation;
    FeeCalculation* maybe_fee_calculation{fuzzed_data_provider.ConsumeBool() ? nullptr : &fee_calculation};
    const CFeeRate mempool_min_fee{wallet.chain().mempoolMinFee()};
    const auto expected_minimum_fee_rate = [&](const CFeeRate& estimate) {
        if (coin_control.m_feerate) {
            return coin_control.fOverrideFeeRate ? *coin_control.m_feerate : std::max(*coin_control.m_feerate, required_fee_rate);
        }
        if (estimate == CFeeRate{0}) {
            if (wallet.m_fallback_fee == CFeeRate{0}) return CFeeRate{0};
            return std::max(std::max(wallet.m_fallback_fee, mempool_min_fee), required_fee_rate);
        }
        return std::max(std::max(estimate, mempool_min_fee), required_fee_rate);
    };
    const auto expected_fee_reason = [&](const CFeeRate& estimate) {
        if (coin_control.m_feerate) {
            return !coin_control.fOverrideFeeRate && *coin_control.m_feerate < required_fee_rate ? FeeReason::REQUIRED : FeeReason::NONE;
        }
        if (estimate == CFeeRate{0}) {
            if (wallet.m_fallback_fee == CFeeRate{0}) return FeeReason::FALLBACK;
            if (wallet.m_fallback_fee < mempool_min_fee) return required_fee_rate > mempool_min_fee ? FeeReason::REQUIRED : FeeReason::MEMPOOL_MIN;
            return required_fee_rate > wallet.m_fallback_fee ? FeeReason::REQUIRED : FeeReason::FALLBACK;
        }
        if (estimate < mempool_min_fee) return required_fee_rate > mempool_min_fee ? FeeReason::REQUIRED : FeeReason::MEMPOOL_MIN;
        return required_fee_rate > estimate ? FeeReason::REQUIRED : FeeReason::NONE;
    };

    const CFeeRate minimum_fee_rate{GetMinimumFeeRate(wallet, coin_control, maybe_fee_calculation)};
    const CFeeRate minimum_fee_estimate{fee_estimator_ptr->LastEstimate()};
    assert(minimum_fee_rate == expected_minimum_fee_rate(minimum_fee_estimate));
    if (maybe_fee_calculation) assert(fee_calculation.reason == expected_fee_reason(minimum_fee_estimate));
    if (coin_control.m_feerate && coin_control.fOverrideFeeRate) {
        assert(minimum_fee_rate == *coin_control.m_feerate);
    } else if (coin_control.m_feerate || minimum_fee_estimate != CFeeRate{0} || wallet.m_fallback_fee != CFeeRate{0}) {
        assert(minimum_fee_rate >= required_fee_rate);
    }

    FeeCalculation fee_calculation_for_fee;
    FeeCalculation* maybe_fee_calculation_for_fee{maybe_fee_calculation ? &fee_calculation_for_fee : nullptr};
    const CAmount minimum_fee{GetMinimumFee(wallet, tx_bytes, coin_control, maybe_fee_calculation_for_fee)};
    const CFeeRate minimum_fee_estimate_after_fee{fee_estimator_ptr->LastEstimate()};
    assert(minimum_fee == expected_minimum_fee_rate(minimum_fee_estimate_after_fee).GetFee(tx_bytes));
    if (maybe_fee_calculation_for_fee) assert(fee_calculation_for_fee.reason == expected_fee_reason(minimum_fee_estimate_after_fee));
}
} // namespace
} // namespace wallet
