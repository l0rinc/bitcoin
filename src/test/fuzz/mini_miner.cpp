// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mini_miner.h>

#include <consensus/amount.h>
#include <kernel/cs_main.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/script.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/translation.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace {

std::deque<COutPoint> g_available_coins;
void initialize_miner()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    for (uint32_t i = 0; i < uint32_t{100}; ++i) {
        g_available_coins.emplace_back(Txid::FromUint256(uint256::ZERO), i);
    }
}

void CheckGatherClusters(const CTxMemPool& pool, const std::vector<Txid>& mempool_txids, const std::vector<COutPoint>& outpoints)
    EXCLUSIVE_LOCKS_REQUIRED(pool.cs)
{
    std::vector<Txid> query;
    query.reserve(mempool_txids.size() * 2 + outpoints.size() + 1);
    for (const auto& txid : mempool_txids) {
        query.push_back(txid);
        query.push_back(txid);
    }
    for (const auto& outpoint : outpoints) {
        query.push_back(outpoint.hash);
    }
    query.push_back(Txid::FromUint256(uint256::ZERO));

    const auto gathered{pool.GatherClusters(query)};
    if (mempool_txids.empty()) {
        Assert(gathered.empty());
        return;
    }
    Assert(!gathered.empty());
    Assert(gathered.size() <= 500);
    const CTxMemPool::setEntries gathered_set{gathered.begin(), gathered.end()};
    Assert(gathered.size() == gathered_set.size());

    std::set<Txid> checked_txids;
    for (const auto& txid : query) {
        if (!checked_txids.insert(txid).second || !pool.exists(txid)) continue;
        const auto single_cluster{pool.GatherClusters({txid})};
        Assert(!single_cluster.empty());
        for (const auto& txiter : single_cluster) {
            Assert(gathered_set.contains(txiter));
        }
    }
}

void CheckTopologicalInclusionOrder(const CTxMemPool& pool, const std::map<Txid, uint32_t>& inclusion_order)
    EXCLUSIVE_LOCKS_REQUIRED(pool.cs)
{
    for (const auto& [txid, sequence] : inclusion_order) {
        const auto entry{pool.GetEntry(txid)};
        Assert(entry);
        for (const auto& input : entry->GetTx().vin) {
            if (const auto parent_it{inclusion_order.find(input.prevout.hash)}; parent_it != inclusion_order.end()) {
                Assert(parent_it->second <= sequence);
            }
        }
    }
}

void CheckManualMiniMinerLinearize(const CTxMemPool& pool, const std::vector<Txid>& mempool_txids)
    EXCLUSIVE_LOCKS_REQUIRED(pool.cs)
{
    if (mempool_txids.empty()) return;

    const auto cluster{pool.GatherClusters(mempool_txids)};
    if (cluster.empty()) return;

    const CTxMemPool::setEntries cluster_set{cluster.begin(), cluster.end()};
    std::vector<node::MiniMinerMempoolEntry> manual_entries;
    manual_entries.reserve(cluster.size());
    std::map<Txid, std::set<Txid>> descendant_caches;

    for (const auto& txiter : cluster) {
        const auto [_, ancestor_size, ancestor_fee]{pool.CalculateAncestorData(*txiter)};
        manual_entries.emplace_back(txiter->GetSharedTx(),
                                    txiter->GetTxSize(),
                                    static_cast<int64_t>(ancestor_size),
                                    txiter->GetModifiedFee(),
                                    ancestor_fee);
    }

    for (const auto& txiter : cluster) {
        CTxMemPool::setEntries descendants;
        pool.CalculateDescendants(txiter, descendants);
        std::set<Txid> descendant_txids;
        for (const auto& descendant : descendants) {
            Assert(cluster_set.contains(descendant));
            descendant_txids.insert(descendant->GetTx().GetHash());
        }
        Assert(descendant_txids.contains(txiter->GetTx().GetHash()));
        Assert(descendant_caches.emplace(txiter->GetTx().GetHash(), std::move(descendant_txids)).second);
    }

    node::MiniMiner manual_mini_miner{manual_entries, descendant_caches};
    assert(manual_mini_miner.IsReadyToCalculate());
    const auto inclusion_order{manual_mini_miner.Linearize()};
    assert(inclusion_order.size() == cluster.size());
    assert(manual_mini_miner.GetMockTemplateTxids().size() == cluster.size());
    for (const auto& txiter : cluster) {
        const auto txid{txiter->GetTx().GetHash()};
        assert(inclusion_order.contains(txid));
        assert(manual_mini_miner.GetMockTemplateTxids().contains(txid));
    }
    CheckTopologicalInclusionOrder(pool, inclusion_order);
}

// Test that the MiniMiner can run with various outpoints and feerates.
FUZZ_TARGET(mini_miner, .init = initialize_miner)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    bilingual_str error;
    CTxMemPool pool{CTxMemPool::Options{}, error};
    Assert(error.empty());
    std::vector<COutPoint> outpoints;
    std::vector<Txid> mempool_txids;
    std::deque<COutPoint> available_coins = g_available_coins;
    LOCK2(::cs_main, pool.cs);
    // Cluster size cannot exceed 500
    LIMITED_WHILE (!available_coins.empty(), 100) {
        CMutableTransaction mtx = CMutableTransaction();
        const size_t num_inputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, available_coins.size());
        const size_t num_outputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 50);
        for (size_t n{0}; n < num_inputs; ++n) {
            auto prevout = available_coins.front();
            mtx.vin.emplace_back(prevout, CScript());
            available_coins.pop_front();
        }
        for (uint32_t n{0}; n < num_outputs; ++n) {
            mtx.vout.emplace_back(100, P2WSH_OP_TRUE);
        }
        CTransactionRef tx = MakeTransactionRef(mtx);
        TestMemPoolEntryHelper entry;
        const CAmount fee{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY/100000)};
        assert(MoneyRange(fee));
        TryAddToMempool(pool, entry.Fee(fee).FromTx(tx));
        if (pool.exists(tx->GetHash())) {
            mempool_txids.push_back(tx->GetHash());
            if (fuzzed_data_provider.ConsumeBool()) {
                const CAmount fee_delta{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY / 100000)};
                pool.PrioritiseTransaction(tx->GetHash(), fuzzed_data_provider.ConsumeBool() ? fee_delta : -fee_delta);
            }
        }

        // All outputs are available to spend
        for (uint32_t n{0}; n < num_outputs; ++n) {
            if (fuzzed_data_provider.ConsumeBool()) {
                available_coins.emplace_back(tx->GetHash(), n);
            }
        }

        if (fuzzed_data_provider.ConsumeBool() && !tx->vout.empty()) {
            // Add outpoint from this tx (may or not be spent by a later tx)
            outpoints.emplace_back(tx->GetHash(),
                                          (uint32_t)fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, tx->vout.size()));
        } else {
            // Add some random outpoint (will be interpreted as confirmed or not yet submitted
            // to mempool).
            auto outpoint = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
            if (outpoint.has_value() && std::find(outpoints.begin(), outpoints.end(), *outpoint) == outpoints.end()) {
                outpoints.push_back(*outpoint);
            }
        }
    }

    CheckGatherClusters(pool, mempool_txids, outpoints);
    CheckManualMiniMinerLinearize(pool, mempool_txids);

    const CFeeRate target_feerate{CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY/1000)}};
    const CFeeRate alternate_target_feerate{CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY/1000)}};
    const auto [lower_target_feerate, higher_target_feerate]{
        target_feerate <= alternate_target_feerate ?
            std::make_pair(target_feerate, alternate_target_feerate) :
            std::make_pair(alternate_target_feerate, target_feerate)};
    const auto duplicated_outpoints{[&] {
        std::vector<COutPoint> ret{outpoints.rbegin(), outpoints.rend()};
        ret.insert(ret.end(), outpoints.begin(), outpoints.end());
        if (!outpoints.empty()) ret.push_back(outpoints.front());
        return ret;
    }()};
    const std::set<COutPoint> unique_outpoints{outpoints.begin(), outpoints.end()};
    std::optional<CAmount> total_bumpfee;
    CAmount sum_fees = 0;
    {
        const auto linearize = [&](const std::vector<COutPoint>& requested_outpoints) EXCLUSIVE_LOCKS_REQUIRED(pool.cs) {
            node::MiniMiner mini_miner{pool, requested_outpoints};
            assert(mini_miner.IsReadyToCalculate());
            const auto inclusion_order{mini_miner.Linearize()};
            const auto template_txids{mini_miner.GetMockTemplateTxids()};
            assert(!mini_miner.IsReadyToCalculate());
            assert(mini_miner.Linearize().empty());
            assert(mini_miner.CalculateBumpFees(target_feerate).empty());
            assert(!mini_miner.CalculateTotalBumpFees(target_feerate).has_value());
            assert(inclusion_order.size() == template_txids.size());
            for (const auto& [txid, _] : inclusion_order) {
                assert(template_txids.contains(txid));
            }
            CheckTopologicalInclusionOrder(pool, inclusion_order);
            return std::make_pair(inclusion_order, template_txids);
        };
        const auto [inclusion_order, template_txids]{linearize(outpoints)};
        const auto [duplicated_inclusion_order, duplicated_template_txids]{linearize(duplicated_outpoints)};
        assert(inclusion_order == duplicated_inclusion_order);
        assert(template_txids == duplicated_template_txids);
    }
    {
        node::MiniMiner mini_miner{pool, outpoints};
        assert(mini_miner.IsReadyToCalculate());
        const auto bump_fees = mini_miner.CalculateBumpFees(target_feerate);
        node::MiniMiner duplicate_mini_miner{pool, duplicated_outpoints};
        assert(duplicate_mini_miner.IsReadyToCalculate());
        const auto duplicated_bump_fees = duplicate_mini_miner.CalculateBumpFees(target_feerate);
        assert(!duplicate_mini_miner.IsReadyToCalculate());
        assert(bump_fees == duplicated_bump_fees);
        assert(bump_fees.size() == unique_outpoints.size());
        for (const auto& [outpoint, _] : bump_fees) {
            assert(unique_outpoints.contains(outpoint));
        }
        for (const auto& outpoint : outpoints) {
            auto it = bump_fees.find(outpoint);
            assert(it != bump_fees.end());
            assert(it->second >= 0);
            sum_fees += it->second;
        }
        assert(!mini_miner.IsReadyToCalculate());
        assert(mini_miner.Linearize().empty());
        assert(!mini_miner.CalculateTotalBumpFees(target_feerate).has_value());
    }
    {
        node::MiniMiner mini_miner{pool, outpoints};
        assert(mini_miner.IsReadyToCalculate());
        total_bumpfee = mini_miner.CalculateTotalBumpFees(target_feerate);
        node::MiniMiner duplicate_mini_miner{pool, duplicated_outpoints};
        assert(duplicate_mini_miner.IsReadyToCalculate());
        const auto duplicated_total_bumpfee = duplicate_mini_miner.CalculateTotalBumpFees(target_feerate);
        assert(!duplicate_mini_miner.IsReadyToCalculate());
        assert(total_bumpfee == duplicated_total_bumpfee);
        assert(total_bumpfee.has_value());
        assert(*total_bumpfee >= 0);
        assert(!mini_miner.IsReadyToCalculate());
        assert(mini_miner.Linearize().empty());
        assert(mini_miner.CalculateBumpFees(target_feerate).empty());
    }
    {
        node::MiniMiner lower_mini_miner{pool, outpoints};
        node::MiniMiner higher_mini_miner{pool, outpoints};
        assert(lower_mini_miner.IsReadyToCalculate());
        assert(higher_mini_miner.IsReadyToCalculate());
        const auto lower_bump_fees{lower_mini_miner.CalculateBumpFees(lower_target_feerate)};
        const auto higher_bump_fees{higher_mini_miner.CalculateBumpFees(higher_target_feerate)};
        assert(!lower_mini_miner.IsReadyToCalculate());
        assert(!higher_mini_miner.IsReadyToCalculate());
        assert(lower_bump_fees.size() == higher_bump_fees.size());
        for (const auto& [outpoint, lower_bump_fee] : lower_bump_fees) {
            const auto higher_it{higher_bump_fees.find(outpoint)};
            assert(higher_it != higher_bump_fees.end());
            assert(higher_it->second >= lower_bump_fee);
        }

        node::MiniMiner lower_total_mini_miner{pool, outpoints};
        node::MiniMiner higher_total_mini_miner{pool, outpoints};
        assert(lower_total_mini_miner.IsReadyToCalculate());
        assert(higher_total_mini_miner.IsReadyToCalculate());
        const auto lower_total_bumpfee{lower_total_mini_miner.CalculateTotalBumpFees(lower_target_feerate)};
        const auto higher_total_bumpfee{higher_total_mini_miner.CalculateTotalBumpFees(higher_target_feerate)};
        assert(!lower_total_mini_miner.IsReadyToCalculate());
        assert(!higher_total_mini_miner.IsReadyToCalculate());
        assert(lower_total_bumpfee.has_value());
        assert(higher_total_bumpfee.has_value());
        assert(*higher_total_bumpfee >= *lower_total_bumpfee);
    }
    // Overlapping ancestry across multiple outpoints can only reduce the total bump fee.
    assert (sum_fees >= *total_bumpfee);
}
} // namespace
