// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <node/miner.h>
#include <node/mining_types.h>
#include <policy/feerate.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <policy/truc_policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/mining.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/overflow.h>
#include <util/string.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>
using node::BlockAssembler;
using node::BlockCreateOptions;
using node::NodeContext;
using util::ToString;

namespace {

const TestingSetup* g_setup;
std::vector<COutPoint> g_outpoints_coinbase_init_mature;
std::vector<COutPoint> g_outpoints_coinbase_init_immature;

void AssertValidTransactionInfo(const TransactionInfo& info)
{
    Assert(info.m_tx);
    Assert(MoneyRange(info.m_fee));
    Assert(info.m_virtual_transaction_size > 0);
}

struct MockedTxPool : public CTxMemPool {
    void RollingFeeUpdate() EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        lastRollingFeeUpdate = GetTime();
        blockSinceLastRollingFeeBump = true;
    }
};

void initialize_tx_pool()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    SetMockTime(WITH_LOCK(g_setup->m_node.chainman->GetMutex(), return g_setup->m_node.chainman->ActiveTip()->Time()));

    for (int i = 0; i < 2 * COINBASE_MATURITY; ++i) {
        COutPoint prevout{MineBlock(g_setup->m_node, {
            .coinbase_output_script = P2WSH_OP_TRUE,
        })};
        // Remember the txids to avoid expensive disk access later on
        auto& outpoints = i < COINBASE_MATURITY ?
                              g_outpoints_coinbase_init_mature :
                              g_outpoints_coinbase_init_immature;
        outpoints.push_back(prevout);
    }
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
}

struct TransactionsDelta final : public CValidationInterface {
    std::set<CTransactionRef>& m_removed;
    std::set<CTransactionRef>& m_added;

    explicit TransactionsDelta(std::set<CTransactionRef>& r, std::set<CTransactionRef>& a)
        : m_removed{r}, m_added{a} {}

    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t mempool_sequence) override
    {
        AssertValidTransactionInfo(tx.info);
        Assert(mempool_sequence > 0);
        Assert(m_added.insert(tx.info.m_tx).second);
    }

    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override
    {
        Assert(tx);
        Assert(mempool_sequence > 0);
        Assert(m_removed.insert(tx).second);
    }
};

void SetMempoolConstraints(ArgsManager& args, FuzzedDataProvider& fuzzed_data_provider)
{
    args.ForceSetArg("-limitclustercount",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(1, 64)));
    args.ForceSetArg("-limitclustersize",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(1, 250)));
    args.ForceSetArg("-maxmempool",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 200)));
    args.ForceSetArg("-mempoolexpiry",
                     ToString(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 999)));
}

void CheckTransactionAncestry(const CTxMemPool& tx_pool, const Txid& txid)
{
    size_t ancestors{std::numeric_limits<size_t>::max()};
    size_t cluster_count{std::numeric_limits<size_t>::max()};
    size_t ancestor_size{std::numeric_limits<size_t>::max()};
    CAmount ancestor_fees{std::numeric_limits<CAmount>::max()};
    tx_pool.GetTransactionAncestry(txid, ancestors, cluster_count, &ancestor_size, &ancestor_fees);

    LOCK(tx_pool.cs);
    const auto it{tx_pool.GetIter(txid)};
    if (!it) {
        Assert(ancestors == 0);
        Assert(cluster_count == 0);
        Assert(ancestor_size == 0);
        Assert(ancestor_fees == 0);
        Assert(tx_pool.GetCluster(txid).empty());
        return;
    }

    const auto [expected_ancestors, expected_size, expected_fees]{tx_pool.CalculateAncestorData(**it)};
    const auto cluster{tx_pool.GetCluster(txid)};
    const auto expected_cluster_count{cluster.size()};
    std::set<Txid> cluster_txids;
    for (const auto* entry : cluster) {
        Assert(entry != nullptr);
        const Txid entry_txid{entry->GetTx().GetHash()};
        Assert(cluster_txids.insert(entry_txid).second);
        Assert(tx_pool.exists(entry_txid));
    }
    Assert(cluster_txids.contains(txid));
    for (const auto* entry : cluster) {
        const auto member_cluster{tx_pool.GetCluster(entry->GetTx().GetHash())};
        std::set<Txid> member_cluster_txids;
        for (const auto* member_entry : member_cluster) {
            Assert(member_entry != nullptr);
            Assert(member_cluster_txids.insert(member_entry->GetTx().GetHash()).second);
        }
        Assert(member_cluster_txids == cluster_txids);
    }
    Assert(ancestors == expected_ancestors);
    Assert(cluster_count == expected_cluster_count);
    Assert(ancestor_size == expected_size);
    Assert(ancestor_fees == expected_fees);
    Assert(ancestors > 0);
    Assert(ancestors <= cluster_count);
}

void CheckTransactionAncestryAll(const CTxMemPool& tx_pool)
{
    for (const auto& info : tx_pool.infoAll()) {
        CheckTransactionAncestry(tx_pool, info.tx->GetHash());
    }
}

void CheckPrioritisedTransactions(const CTxMemPool& tx_pool)
{
    const auto prioritised{tx_pool.GetPrioritisedTransactions()};
    std::set<Txid> seen_txids;

    LOCK(tx_pool.cs);
    Assert(prioritised.size() == tx_pool.mapDeltas.size());
    for (const auto& [txid, delta] : tx_pool.mapDeltas) {
        Assert(delta != 0);
    }
    for (const auto& delta_info : prioritised) {
        Assert(seen_txids.insert(delta_info.txid).second);
        Assert(delta_info.delta != 0);
        const auto delta_it{tx_pool.mapDeltas.find(delta_info.txid)};
        Assert(delta_it != tx_pool.mapDeltas.end());
        Assert(delta_it->second == delta_info.delta);

        const auto entry{tx_pool.GetIter(delta_info.txid)};
        Assert(delta_info.in_mempool == entry.has_value());
        Assert(delta_info.modified_fee.has_value() == entry.has_value());
        if (entry) {
            const CAmount expected_modified_fee{SaturatingAdd((*entry)->GetFee(), delta_info.delta)};
            Assert((*entry)->GetModifiedFee() == expected_modified_fee);
            Assert(*delta_info.modified_fee == expected_modified_fee);
        }
    }
    for (const auto& entry : tx_pool.mapTx) {
        const auto txid{entry.GetTx().GetHash()};
        const auto delta_it{tx_pool.mapDeltas.find(txid)};
        const CAmount delta{delta_it == tx_pool.mapDeltas.end() ? 0 : delta_it->second};
        Assert(entry.GetModifiedFee() == SaturatingAdd(entry.GetFee(), delta));
    }
}

void CheckRandomizedTxIndex(const CTxMemPool& tx_pool)
{
    LOCK(tx_pool.cs);
    Assert(tx_pool.txns_randomized.size() == tx_pool.mapTx.size());

    std::set<Txid> randomized_txids;
    for (size_t index{0}; index < tx_pool.txns_randomized.size(); ++index) {
        const auto& [wtxid, it] = tx_pool.txns_randomized[index];
        Assert(it != tx_pool.mapTx.end());
        Assert(it->GetTx().GetWitnessHash() == wtxid);
        Assert(it->idx_randomized == index);
        const Txid txid{it->GetTx().GetHash()};
        Assert(tx_pool.mapTx.find(txid) == it);
        Assert(randomized_txids.insert(txid).second);
    }
    for (const auto& entry : tx_pool.mapTx) {
        Assert(randomized_txids.contains(entry.GetTx().GetHash()));
    }
}

void AssertInfoEmpty(const TxMempoolInfo& info)
{
    Assert(!info.tx);
    Assert(info.m_time == std::chrono::seconds{});
    Assert(info.fee == 0);
    Assert(info.vsize == 0);
    Assert(info.nFeeDelta == 0);
}

void AssertInfoEqual(const TxMempoolInfo& expected, const TxMempoolInfo& actual)
{
    Assert(actual.tx == expected.tx);
    Assert(actual.m_time == expected.m_time);
    Assert(actual.fee == expected.fee);
    Assert(actual.vsize == expected.vsize);
    Assert(actual.nFeeDelta == expected.nFeeDelta);
}

void CheckMempoolInfoViews(const CTxMemPool& tx_pool)
{
    const auto infos{tx_pool.infoAll()};
    const auto entries{WITH_LOCK(tx_pool.cs, return tx_pool.entryAll())};
    Assert(infos.size() == entries.size());

    std::set<Txid> seen_txids;
    std::set<Wtxid> seen_wtxids;
    CAmount total_fee{0};
    uint64_t total_size{0};

    {
        LOCK(tx_pool.cs);
        Assert(infos.size() == tx_pool.mapTx.size());
        for (size_t index{0}; index < infos.size(); ++index) {
            const auto& info{infos[index]};
            Assert(info.tx);
            const auto& entry{entries[index].get()};
            Assert(info.tx == entry.GetSharedTx());
            Assert(info.m_time == entry.GetTime());
            Assert(info.fee == entry.GetFee());
            Assert(info.vsize == entry.GetTxSize());
            Assert(info.nFeeDelta == entry.GetModifiedFee() - entry.GetFee());

            const Txid txid{info.tx->GetHash()};
            const Wtxid wtxid{info.tx->GetWitnessHash()};
            const auto txid_iter{tx_pool.GetIter(txid)};
            const auto wtxid_iter{tx_pool.GetIter(wtxid)};
            Assert(txid_iter);
            Assert(wtxid_iter);
            Assert(*txid_iter == *wtxid_iter);
            Assert(tx_pool.mapTx.find(txid) == *txid_iter);
            Assert(tx_pool.mapTx.get<index_by_wtxid>().find(wtxid) != tx_pool.mapTx.get<index_by_wtxid>().end());
            Assert(seen_txids.insert(txid).second);
            Assert(seen_wtxids.insert(wtxid).second);
            total_fee += info.fee;
            total_size += info.vsize;
        }
        Assert(total_fee == tx_pool.GetTotalFee());
        Assert(total_size == tx_pool.GetTotalTxSize());
    }

    for (const auto& info : infos) {
        const Txid txid{info.tx->GetHash()};
        const Wtxid wtxid{info.tx->GetWitnessHash()};
        AssertInfoEqual(info, tx_pool.info(txid));
        AssertInfoEqual(info, tx_pool.info(wtxid));
        AssertInfoEqual(info, tx_pool.info_for_relay(txid, std::numeric_limits<uint64_t>::max()));
        AssertInfoEqual(info, tx_pool.info_for_relay(wtxid, std::numeric_limits<uint64_t>::max()));
        AssertInfoEmpty(tx_pool.info_for_relay(txid, /*last_sequence=*/0));
        AssertInfoEmpty(tx_pool.info_for_relay(wtxid, /*last_sequence=*/0));

        const uint64_t entry_sequence{WITH_LOCK(tx_pool.cs, return (*tx_pool.GetIter(txid))->GetSequence())};
        AssertInfoEmpty(tx_pool.info_for_relay(txid, entry_sequence));
        AssertInfoEmpty(tx_pool.info_for_relay(wtxid, entry_sequence));
        if (entry_sequence < std::numeric_limits<uint64_t>::max()) {
            AssertInfoEqual(info, tx_pool.info_for_relay(txid, entry_sequence + 1));
            AssertInfoEqual(info, tx_pool.info_for_relay(wtxid, entry_sequence + 1));
        }
    }

    std::vector<Txid> delta_only_txids;
    {
        LOCK(tx_pool.cs);
        for (const auto& [txid, _] : tx_pool.mapDeltas) {
            if (!seen_txids.contains(txid)) delta_only_txids.push_back(txid);
        }
    }
    for (const auto& txid : delta_only_txids) {
        AssertInfoEmpty(tx_pool.info(txid));
        AssertInfoEmpty(tx_pool.info_for_relay(txid, std::numeric_limits<uint64_t>::max()));
    }
}

void CheckMempoolBlockBuilder(const CTxMemPool& tx_pool)
{
    LOCK(tx_pool.cs);
    const auto diagram{tx_pool.GetFeerateDiagram()};
    Assert(!diagram.empty());
    Assert(diagram.front() == FeePerWeight{});

    std::vector<FeePerWeight> builder_diagram{FeePerWeight{}};
    std::set<const CTxMemPoolEntry*> seen_entries;

    tx_pool.StartBlockBuilding();
    while (true) {
        std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> entries;
        const FeePerWeight chunk_feerate{tx_pool.GetBlockBuilderChunk(entries)};
        std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> entries_again;
        const FeePerWeight chunk_feerate_again{tx_pool.GetBlockBuilderChunk(entries_again)};
        Assert(chunk_feerate_again == chunk_feerate);
        Assert(entries_again.size() == entries.size());
        for (size_t i{0}; i < entries.size(); ++i) {
            Assert(&entries_again[i].get() == &entries[i].get());
        }
        if (chunk_feerate == FeePerWeight{}) {
            Assert(entries.empty());
            break;
        }

        Assert(!entries.empty());
        for (const auto& entry_ref : entries) {
            const CTxMemPoolEntry& entry{entry_ref.get()};
            const auto it{tx_pool.mapTx.find(entry.GetTx().GetHash())};
            Assert(it != tx_pool.mapTx.end());
            Assert(&*it == &entry);
            Assert(seen_entries.insert(&entry).second);
            Assert(tx_pool.GetMainChunkFeerate(entry) == chunk_feerate);
        }

        builder_diagram.push_back(FeePerWeight{
            SaturatingAdd(builder_diagram.back().fee, chunk_feerate.fee),
            SaturatingAdd(builder_diagram.back().size, chunk_feerate.size),
        });
        tx_pool.IncludeBuilderChunk();
    }
    tx_pool.StopBlockBuilding();

    Assert(builder_diagram == diagram);
    Assert(seen_entries.size() == tx_pool.mapTx.size());

    tx_pool.StartBlockBuilding();
    std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> skipped_entries;
    const FeePerWeight skipped_feerate{tx_pool.GetBlockBuilderChunk(skipped_entries)};
    if (skipped_feerate != FeePerWeight{}) {
        Assert(!skipped_entries.empty());
        std::set<const CTxMemPoolEntry*> skipped_set;
        for (const auto& entry_ref : skipped_entries) {
            Assert(skipped_set.insert(&entry_ref.get()).second);
        }

        tx_pool.SkipBuilderChunk();
        std::set<const CTxMemPoolEntry*> seen_after_skip;
        while (true) {
            std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> entries;
            const FeePerWeight chunk_feerate{tx_pool.GetBlockBuilderChunk(entries)};
            if (chunk_feerate == FeePerWeight{}) {
                Assert(entries.empty());
                break;
            }

            Assert(!entries.empty());
            for (const auto& entry_ref : entries) {
                const CTxMemPoolEntry& entry{entry_ref.get()};
                Assert(!skipped_set.contains(&entry));
                Assert(seen_after_skip.insert(&entry).second);
            }
            tx_pool.IncludeBuilderChunk();
        }
    }
    tx_pool.StopBlockBuilding();
}

void CheckUpdatedBlockDependencies(const CTxMemPool& tx_pool, const std::vector<Txid>& txids)
{
    LOCK(tx_pool.cs);
    for (const auto& txid : txids) {
        const auto parent{tx_pool.GetIter(txid)};
        if (!parent) continue;
        auto child_it{tx_pool.mapNextTx.lower_bound(COutPoint(txid, 0))};
        for (; child_it != tx_pool.mapNextTx.end() && child_it->first->hash == txid; ++child_it) {
            const auto child{child_it->second};
            Assert(child != tx_pool.mapTx.end());
            if (child == *parent) continue;
            Assert(tx_pool.CalculateMemPoolAncestors(*child).contains(*parent));
        }
    }
}

void CheckHasDescendants(const CTxMemPool& tx_pool)
{
    std::vector<Txid> txids;
    {
        LOCK(tx_pool.cs);
        txids.reserve(tx_pool.mapTx.size());
        for (const auto& entry : tx_pool.mapTx) {
            txids.push_back(entry.GetTx().GetHash());
        }
    }

    for (const Txid& txid : txids) {
        bool has_direct_child{false};
        {
            LOCK(tx_pool.cs);
            auto parent{tx_pool.GetIter(txid)};
            Assert(parent);
            auto child_it{tx_pool.mapNextTx.lower_bound(COutPoint(txid, 0))};
            for (; child_it != tx_pool.mapNextTx.end() && child_it->first->hash == txid; ++child_it) {
                Assert(child_it->second != tx_pool.mapTx.end());
                if (child_it->second != *parent) {
                    has_direct_child = true;
                    break;
                }
            }
        }
        Assert(tx_pool.HasDescendants(txid) == has_direct_child);
    }
}

void CheckDirectMempoolEdges(const CTxMemPool& tx_pool)
{
    LOCK(tx_pool.cs);
    for (const auto& entry : tx_pool.mapTx) {
        const Txid txid{entry.GetTx().GetHash()};
        std::set<Txid> expected_parents;
        for (const CTxIn& txin : entry.GetTx().vin) {
            if (tx_pool.mapTx.find(txin.prevout.hash) != tx_pool.mapTx.end()) {
                expected_parents.insert(txin.prevout.hash);
            }
        }

        std::set<Txid> actual_parents;
        for (const auto& parent : tx_pool.GetParents(entry)) {
            const Txid parent_txid{parent.get().GetTx().GetHash()};
            Assert(actual_parents.insert(parent_txid).second);
            Assert(expected_parents.contains(parent_txid));

            bool found_child{false};
            for (const auto& child : tx_pool.GetChildren(parent.get())) {
                const Txid child_txid{child.get().GetTx().GetHash()};
                Assert(child_txid != parent_txid);
                if (child_txid == txid) found_child = true;
            }
            Assert(found_child);
        }
        Assert(actual_parents == expected_parents);

        std::set<Txid> expected_children;
        auto child_it{tx_pool.mapNextTx.lower_bound(COutPoint(txid, 0))};
        for (; child_it != tx_pool.mapNextTx.end() && child_it->first->hash == txid; ++child_it) {
            const auto child{child_it->second};
            Assert(child != tx_pool.mapTx.end());
            const Txid child_txid{child->GetTx().GetHash()};
            if (child_txid == txid) continue;
            Assert(std::ranges::any_of(child->GetTx().vin, [&](const CTxIn& txin) {
                return txin.prevout.hash == txid;
            }));
            expected_children.insert(child_txid);
        }

        std::set<Txid> actual_children;
        for (const auto& child : tx_pool.GetChildren(entry)) {
            const Txid child_txid{child.get().GetTx().GetHash()};
            Assert(child_txid != txid);
            Assert(actual_children.insert(child_txid).second);
            Assert(expected_children.contains(child_txid));

            bool found_parent{false};
            for (const auto& parent : tx_pool.GetParents(child.get())) {
                if (parent.get().GetTx().GetHash() == txid) found_parent = true;
            }
            Assert(found_parent);
        }
        Assert(actual_children == expected_children);
    }
}

[[nodiscard]] CAmount ConsumePriorityDelta(FuzzedDataProvider& fuzzed_data_provider)
{
    switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 4)) {
    case 0:
        return std::numeric_limits<CAmount>::max();
    case 1:
        return -std::numeric_limits<CAmount>::max();
    case 2:
        return 0;
    default:
        return fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-50 * COIN, 50 * COIN);
    }
}

void Finish(FuzzedDataProvider& fuzzed_data_provider, MockedTxPool& tx_pool, Chainstate& chainstate)
{
    WITH_LOCK(::cs_main, tx_pool.check(chainstate.CoinsTip(), chainstate.m_chain.Height() + 1));
    CheckTransactionAncestryAll(tx_pool);
    CheckHasDescendants(tx_pool);
    CheckDirectMempoolEdges(tx_pool);
    CheckPrioritisedTransactions(tx_pool);
    CheckRandomizedTxIndex(tx_pool);
    CheckMempoolInfoViews(tx_pool);
    CheckMempoolBlockBuilder(tx_pool);
    {
        BlockCreateOptions options{
            .block_min_fee_rate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)},
            .block_max_weight = fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(DEFAULT_BLOCK_RESERVED_WEIGHT, MAX_BLOCK_WEIGHT),
        };
        auto assembler = BlockAssembler{chainstate, &tx_pool, options};
        auto block_template = assembler.CreateNewBlock();
        Assert(block_template->block.vtx.size() >= 1);
        Assert(std::all_of(block_template->block.vtx.cbegin(), block_template->block.vtx.cend(),
            [](const auto& tx) { return tx != nullptr; }));

        // Try updating the mempool for this block, as though it were mined.
        LOCK2(::cs_main, tx_pool.cs);
        tx_pool.removeForBlock(block_template->block.vtx, chainstate.m_chain.Height() + 1);

        // Now try to add those transactions back, as though a reorg happened.
        std::vector<Txid> hashes_to_update;
        for (const auto& tx : block_template->block.vtx) {
            const auto res = AcceptToMemoryPool(chainstate, tx, GetTime(), true, /*test_accept=*/false);
            if (res.m_result_type == MempoolAcceptResult::ResultType::VALID) {
                hashes_to_update.push_back(tx->GetHash());
            } else {
                tx_pool.removeRecursive(*tx, MemPoolRemovalReason::REORG);
            }
        }
        tx_pool.UpdateTransactionsFromBlock(hashes_to_update);
        CheckUpdatedBlockDependencies(tx_pool, hashes_to_update);
        CheckHasDescendants(tx_pool);
        CheckDirectMempoolEdges(tx_pool);
        CheckRandomizedTxIndex(tx_pool);
        CheckMempoolInfoViews(tx_pool);
    }
    CheckPrioritisedTransactions(tx_pool);
    CheckHasDescendants(tx_pool);
    CheckDirectMempoolEdges(tx_pool);
    CheckRandomizedTxIndex(tx_pool);
    CheckMempoolInfoViews(tx_pool);
    const auto info_all = tx_pool.infoAll();
    if (!info_all.empty()) {
        const auto& tx_to_remove = *PickValue(fuzzed_data_provider, info_all).tx;
        WITH_LOCK(tx_pool.cs, tx_pool.removeRecursive(tx_to_remove, MemPoolRemovalReason::BLOCK /* dummy */));
        assert(tx_pool.size() < info_all.size());
        CheckTransactionAncestryAll(tx_pool);
        CheckHasDescendants(tx_pool);
        CheckDirectMempoolEdges(tx_pool);
        CheckPrioritisedTransactions(tx_pool);
        CheckRandomizedTxIndex(tx_pool);
        CheckMempoolInfoViews(tx_pool);
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        // Try eviction
        LOCK2(::cs_main, tx_pool.cs);
        tx_pool.TrimToSize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0U, tx_pool.DynamicMemoryUsage() * 2));
        CheckRandomizedTxIndex(tx_pool);
        CheckMempoolInfoViews(tx_pool);
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        // Try expiry
        LOCK2(::cs_main, tx_pool.cs);
        tx_pool.Expire(GetMockTime() - std::chrono::seconds(fuzzed_data_provider.ConsumeIntegral<uint32_t>()));
        CheckRandomizedTxIndex(tx_pool);
        CheckMempoolInfoViews(tx_pool);
    }
    WITH_LOCK(::cs_main, tx_pool.check(chainstate.CoinsTip(), chainstate.m_chain.Height() + 1));
    CheckTransactionAncestryAll(tx_pool);
    CheckHasDescendants(tx_pool);
    CheckDirectMempoolEdges(tx_pool);
    CheckPrioritisedTransactions(tx_pool);
    CheckRandomizedTxIndex(tx_pool);
    CheckMempoolInfoViews(tx_pool);
    CheckMempoolBlockBuilder(tx_pool);
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
}

void MockTime(FuzzedDataProvider& fuzzed_data_provider, const Chainstate& chainstate)
{
    const auto time = ConsumeTime(fuzzed_data_provider,
                                  chainstate.m_chain.Tip()->GetMedianTimePast() + 1,
                                  std::numeric_limits<decltype(chainstate.m_chain.Tip()->nTime)>::max());
    SetMockTime(time);
}

std::unique_ptr<CTxMemPool> MakeMempool(FuzzedDataProvider& fuzzed_data_provider, const NodeContext& node)
{
    // Take the default options for tests...
    CTxMemPool::Options mempool_opts{MemPoolOptionsForTest(node)};

    // ...override specific options for this specific fuzz suite
    mempool_opts.check_ratio = 1;
    mempool_opts.require_standard = fuzzed_data_provider.ConsumeBool();

    // ...and construct a CTxMemPool from it
    bilingual_str error;
    auto mempool{std::make_unique<CTxMemPool>(std::move(mempool_opts), error)};
    // ... ignore the error since it might be beneficial to fuzz even when the
    // mempool size is unreasonably small
    Assert(error.empty() || error.original.starts_with("-maxmempool must be at least "));
    return mempool;
}

void CheckATMPInvariants(const MempoolAcceptResult& res, const Wtxid& wtxid, bool txid_in_mempool, bool wtxid_in_mempool)
{

    switch (res.m_result_type) {
    case MempoolAcceptResult::ResultType::VALID:
    {
        Assert(txid_in_mempool);
        Assert(wtxid_in_mempool);
        Assert(res.m_state.IsValid());
        Assert(!res.m_state.IsInvalid());
        Assert(res.m_vsize);
        Assert(res.m_base_fees);
        Assert(*res.m_vsize > 0);
        Assert(MoneyRange(*res.m_base_fees));
        Assert(res.m_effective_feerate);
        Assert(res.m_wtxids_fee_calculations);
        Assert(!res.m_wtxids_fee_calculations->empty());
        Assert(std::find(res.m_wtxids_fee_calculations->begin(), res.m_wtxids_fee_calculations->end(), wtxid) != res.m_wtxids_fee_calculations->end());
        Assert(!res.m_other_wtxid);
        break;
    }
    case MempoolAcceptResult::ResultType::INVALID:
    {
        // It may be already in the mempool since in ATMP cases we don't set MEMPOOL_ENTRY or DIFFERENT_WITNESS
        Assert(!res.m_state.IsValid());
        Assert(res.m_state.IsInvalid());

        const bool is_reconsiderable{res.m_state.GetResult() == TxValidationResult::TX_RECONSIDERABLE};
        Assert(!res.m_vsize);
        Assert(!res.m_base_fees);
        // Fee information is provided if the failure is TX_RECONSIDERABLE.
        // In other cases, validation may be unable or unwilling to calculate the fees.
        Assert(res.m_effective_feerate.has_value() == is_reconsiderable);
        Assert(res.m_wtxids_fee_calculations.has_value() == is_reconsiderable);
        if (res.m_wtxids_fee_calculations) {
            Assert(!res.m_wtxids_fee_calculations->empty());
            Assert(std::find(res.m_wtxids_fee_calculations->begin(), res.m_wtxids_fee_calculations->end(), wtxid) != res.m_wtxids_fee_calculations->end());
        }
        Assert(!res.m_other_wtxid);
        break;
    }
    case MempoolAcceptResult::ResultType::MEMPOOL_ENTRY:
    {
        // ATMP never sets this; only set in package settings
        Assert(false);
        break;
    }
    case MempoolAcceptResult::ResultType::DIFFERENT_WITNESS:
    {
        // ATMP never sets this; only set in package settings
        Assert(false);
        break;
    }
    }
}

FUZZ_TARGET(tx_pool_standard, .init = initialize_tx_pool)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto& node = g_setup->m_node;
    auto& chainstate{static_cast<DummyChainState&>(node.chainman->ActiveChainstate())};

    MockTime(fuzzed_data_provider, chainstate);

    // All RBF-spendable outpoints
    std::set<COutPoint> outpoints_rbf;
    // All outpoints counting toward the total supply (subset of outpoints_rbf)
    std::set<COutPoint> outpoints_supply;
    for (const auto& outpoint : g_outpoints_coinbase_init_mature) {
        Assert(outpoints_supply.insert(outpoint).second);
    }
    outpoints_rbf = outpoints_supply;

    // The sum of the values of all spendable outpoints
    constexpr CAmount SUPPLY_TOTAL{COINBASE_MATURITY * 50 * COIN};

    SetMempoolConstraints(*node.args, fuzzed_data_provider);
    auto tx_pool_{MakeMempool(fuzzed_data_provider, node)};
    MockedTxPool& tx_pool = *static_cast<MockedTxPool*>(tx_pool_.get());

    chainstate.SetMempool(&tx_pool);

    // Helper to query an amount
    const CCoinsViewMemPool amount_view{WITH_LOCK(::cs_main, return &chainstate.CoinsTip()), tx_pool};
    const auto GetAmount = [&](const COutPoint& outpoint) {
        auto coin{amount_view.GetCoin(outpoint).value()};
        return coin.out.nValue;
    };

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 100) {
        {
            // Total supply is the mempool fee + all outpoints
            CAmount supply_now{WITH_LOCK(tx_pool.cs, return tx_pool.GetTotalFee())};
            for (const auto& op : outpoints_supply) {
                supply_now += GetAmount(op);
            }
            Assert(supply_now == SUPPLY_TOTAL);
        }
        Assert(!outpoints_supply.empty());

        // Create transaction to add to the mempool
        const CTransactionRef tx = [&] {
            CMutableTransaction tx_mut;
            tx_mut.version = fuzzed_data_provider.ConsumeBool() ? TRUC_VERSION : CTransaction::CURRENT_VERSION;
            tx_mut.nLockTime = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            const auto num_in = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, outpoints_rbf.size());
            const auto num_out = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, outpoints_rbf.size() * 2);

            CAmount amount_in{0};
            for (int i = 0; i < num_in; ++i) {
                // Pop random outpoint
                auto pop = outpoints_rbf.begin();
                std::advance(pop, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, outpoints_rbf.size() - 1));
                const auto outpoint = *pop;
                outpoints_rbf.erase(pop);
                amount_in += GetAmount(outpoint);

                // Create input
                const auto sequence = ConsumeSequence(fuzzed_data_provider);
                const auto script_sig = CScript{};
                const auto script_wit_stack = std::vector<std::vector<uint8_t>>{WITNESS_STACK_ELEM_OP_TRUE};
                CTxIn in;
                in.prevout = outpoint;
                in.nSequence = sequence;
                in.scriptSig = script_sig;
                in.scriptWitness.stack = script_wit_stack;

                tx_mut.vin.push_back(in);
            }

            // Check sigops in mempool + block template creation
            bool add_sigops{fuzzed_data_provider.ConsumeBool()};

            const auto amount_fee = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-1000, amount_in);
            const auto amount_out = (amount_in - amount_fee) / num_out;
            for (int i = 0; i < num_out; ++i) {
                if (i == 0 && add_sigops) {
                    tx_mut.vout.emplace_back(amount_out, CScript() << std::vector<unsigned char>(33, 0x02) << OP_CHECKSIG);
                } else {
                    tx_mut.vout.emplace_back(amount_out, P2WSH_OP_TRUE);
                }
            }

            auto tx = MakeTransactionRef(tx_mut);
            // Restore previously removed outpoints
            for (const auto& in : tx->vin) {
                Assert(outpoints_rbf.insert(in.prevout).second);
            }
            return tx;
        }();

        if (fuzzed_data_provider.ConsumeBool()) {
            MockTime(fuzzed_data_provider, chainstate);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            tx_pool.RollingFeeUpdate();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            const auto& txid = fuzzed_data_provider.ConsumeBool() ?
                                   tx->GetHash() :
                                   PickValue(fuzzed_data_provider, outpoints_rbf).hash;
            const auto delta = ConsumePriorityDelta(fuzzed_data_provider);
            tx_pool.PrioritiseTransaction(txid, delta);
            CheckPrioritisedTransactions(tx_pool);
        }

        // Remember all removed and added transactions
        std::set<CTransactionRef> removed;
        std::set<CTransactionRef> added;
        auto txr = std::make_shared<TransactionsDelta>(removed, added);
        node.validation_signals->RegisterSharedValidationInterface(txr);

        // Make sure ProcessNewPackage on one transaction works.
        // The result is not guaranteed to be the same as what is returned by ATMP.
        const auto result_package = WITH_LOCK(::cs_main,
                                    return ProcessNewPackage(chainstate, tx_pool, {tx}, true, /*client_maxfeerate=*/{}));
        // If something went wrong due to a package-specific policy, it might not return a
        // validation result for the transaction.
        if (result_package.m_state.GetResult() != PackageValidationResult::PCKG_POLICY) {
            Assert(!CheckPackageMempoolAcceptResult({tx}, result_package, result_package.m_state.IsValid(), nullptr));
            auto it = result_package.m_tx_results.find(tx->GetWitnessHash());
            Assert(it != result_package.m_tx_results.end());
            Assert(it->second.m_result_type == MempoolAcceptResult::ResultType::VALID ||
                   it->second.m_result_type == MempoolAcceptResult::ResultType::INVALID);
        }

        const auto res = WITH_LOCK(::cs_main, return AcceptToMemoryPool(chainstate, tx, GetTime(), /*bypass_limits=*/false, /*test_accept=*/false));
        const bool accepted = res.m_result_type == MempoolAcceptResult::ResultType::VALID;
        node.validation_signals->SyncWithValidationInterfaceQueue();
        node.validation_signals->UnregisterSharedValidationInterface(txr);

        bool txid_in_mempool = tx_pool.exists(tx->GetHash());
        bool wtxid_in_mempool = tx_pool.exists(tx->GetWitnessHash());
        CheckATMPInvariants(res, tx->GetWitnessHash(), txid_in_mempool, wtxid_in_mempool);
        Assert(!CheckPackageMempoolAcceptResult({tx}, PackageMempoolAcceptResult{tx->GetWitnessHash(), res},
                                                accepted, nullptr));
        if (txid_in_mempool) CheckTransactionAncestry(tx_pool, tx->GetHash());
        CheckDirectMempoolEdges(tx_pool);
        CheckPrioritisedTransactions(tx_pool);
        CheckRandomizedTxIndex(tx_pool);
        CheckMempoolInfoViews(tx_pool);

        Assert(accepted != added.empty());
        if (accepted) {
            Assert(added.size() == 1); // For now, no package acceptance
            Assert(tx == *added.begin());
            CheckMempoolTRUCInvariants(tx_pool);
        } else {
            // Do not consider rejected transaction removed
            removed.erase(tx);
        }

        // Helper to insert spent and created outpoints of a tx into collections
        using Sets = std::vector<std::reference_wrapper<std::set<COutPoint>>>;
        const auto insert_tx = [](Sets created_by_tx, Sets consumed_by_tx, const auto& tx) {
            for (size_t i{0}; i < tx.vout.size(); ++i) {
                for (auto& set : created_by_tx) {
                    Assert(set.get().emplace(tx.GetHash(), i).second);
                }
            }
            for (const auto& in : tx.vin) {
                for (auto& set : consumed_by_tx) {
                    Assert(set.get().insert(in.prevout).second);
                }
            }
        };
        // Add created outpoints, remove spent outpoints
        {
            // Outpoints that no longer exist at all
            std::set<COutPoint> consumed_erased;
            // Outpoints that no longer count toward the total supply
            std::set<COutPoint> consumed_supply;
            for (const auto& removed_tx : removed) {
                insert_tx(/*created_by_tx=*/{consumed_erased}, /*consumed_by_tx=*/{outpoints_supply}, /*tx=*/*removed_tx);
            }
            for (const auto& added_tx : added) {
                insert_tx(/*created_by_tx=*/{outpoints_supply, outpoints_rbf}, /*consumed_by_tx=*/{consumed_supply}, /*tx=*/*added_tx);
            }
            for (const auto& p : consumed_erased) {
                Assert(outpoints_supply.erase(p) == 1);
                Assert(outpoints_rbf.erase(p) == 1);
            }
            for (const auto& p : consumed_supply) {
                Assert(outpoints_supply.erase(p) == 1);
            }
        }
        CheckPrioritisedTransactions(tx_pool);
        CheckDirectMempoolEdges(tx_pool);
        CheckRandomizedTxIndex(tx_pool);
        CheckMempoolInfoViews(tx_pool);
    }
    Finish(fuzzed_data_provider, tx_pool, chainstate);
}

FUZZ_TARGET(tx_pool, .init = initialize_tx_pool)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto& node = g_setup->m_node;
    auto& chainstate{static_cast<DummyChainState&>(node.chainman->ActiveChainstate())};

    MockTime(fuzzed_data_provider, chainstate);

    std::vector<Txid> txids;
    txids.reserve(g_outpoints_coinbase_init_mature.size());
    for (const auto& outpoint : g_outpoints_coinbase_init_mature) {
        txids.push_back(outpoint.hash);
    }
    for (int i{0}; i <= 3; ++i) {
        // Add some immature and non-existent outpoints
        txids.push_back(g_outpoints_coinbase_init_immature.at(i).hash);
        txids.push_back(Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)));
    }

    SetMempoolConstraints(*node.args, fuzzed_data_provider);
    auto tx_pool_{MakeMempool(fuzzed_data_provider, node)};
    MockedTxPool& tx_pool = *static_cast<MockedTxPool*>(tx_pool_.get());

    chainstate.SetMempool(&tx_pool);

    // If we ever bypass limits, do not do TRUC invariants checks
    bool ever_bypassed_limits{false};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 300) {
        const auto mut_tx = ConsumeTransaction(fuzzed_data_provider, txids);

        if (fuzzed_data_provider.ConsumeBool()) {
            MockTime(fuzzed_data_provider, chainstate);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            tx_pool.RollingFeeUpdate();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            const auto txid = fuzzed_data_provider.ConsumeBool() ?
                                   mut_tx.GetHash() :
                                   PickValue(fuzzed_data_provider, txids);
            const auto delta = ConsumePriorityDelta(fuzzed_data_provider);
            tx_pool.PrioritiseTransaction(txid, delta);
            CheckPrioritisedTransactions(tx_pool);
        }

        const bool bypass_limits{fuzzed_data_provider.ConsumeBool()};
        ever_bypassed_limits |= bypass_limits;

        const auto tx = MakeTransactionRef(mut_tx);
        const auto res = WITH_LOCK(::cs_main, return AcceptToMemoryPool(chainstate, tx, GetTime(), bypass_limits, /*test_accept=*/false));
        const bool accepted = res.m_result_type == MempoolAcceptResult::ResultType::VALID;
        const bool txid_in_mempool{tx_pool.exists(tx->GetHash())};
        const bool wtxid_in_mempool{tx_pool.exists(tx->GetWitnessHash())};
        CheckATMPInvariants(res, tx->GetWitnessHash(), txid_in_mempool, wtxid_in_mempool);
        Assert(!CheckPackageMempoolAcceptResult({tx}, PackageMempoolAcceptResult{tx->GetWitnessHash(), res},
                                                accepted, nullptr));
        if (accepted) {
            txids.push_back(tx->GetHash());
            CheckTransactionAncestry(tx_pool, tx->GetHash());
            CheckDirectMempoolEdges(tx_pool);
            CheckRandomizedTxIndex(tx_pool);
            CheckMempoolInfoViews(tx_pool);
            if (!ever_bypassed_limits) {
                CheckMempoolTRUCInvariants(tx_pool);
            }
        }
        CheckDirectMempoolEdges(tx_pool);
        CheckPrioritisedTransactions(tx_pool);
        CheckMempoolInfoViews(tx_pool);
    }
    Finish(fuzzed_data_provider, tx_pool, chainstate);
}
} // namespace
