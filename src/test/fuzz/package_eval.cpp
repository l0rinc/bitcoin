// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <node/mining_types.h>
#include <policy/feerate.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <policy/truc_policy.h>
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
#include <util/hasher.h>
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
#include <unordered_map>
#include <utility>
#include <vector>
using node::NodeContext;

namespace {

const TestingSetup* g_setup;
std::vector<COutPoint> g_outpoints_coinbase_init_mature;

void AssertValidTransactionInfo(const TransactionInfo& info)
{
    Assert(info.m_tx);
    Assert(MoneyRange(info.m_fee));
    Assert(info.m_virtual_transaction_size > 0);
}

void AssertNonNullTransactionRefs(const std::vector<CTransactionRef>& txs)
{
    Assert(std::all_of(txs.cbegin(), txs.cend(), [](const auto& tx) { return tx != nullptr; }));
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
            .coinbase_output_script = P2WSH_EMPTY,
        })};
        if (i < COINBASE_MATURITY) {
            // Remember the txids to avoid expensive disk access later on
            g_outpoints_coinbase_init_mature.push_back(prevout);
        }
    }
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
}

struct OutpointsUpdater final : public CValidationInterface {
    std::set<COutPoint>& m_mempool_outpoints;

    explicit OutpointsUpdater(std::set<COutPoint>& r)
        : m_mempool_outpoints{r} {}

    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t mempool_sequence) override
    {
        AssertValidTransactionInfo(tx.info);
        Assert(mempool_sequence > 0);

        // for coins spent we always want to be able to rbf so they're not removed

        // outputs from this tx can now be spent
        for (uint32_t index{0}; index < tx.info.m_tx->vout.size(); ++index) {
            m_mempool_outpoints.insert(COutPoint{tx.info.m_tx->GetHash(), index});
        }
    }

    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override
    {
        Assert(tx);
        Assert(mempool_sequence > 0);

        // outpoints spent by this tx are now available
        for (const auto& input : tx->vin) {
            // Could already exist if this was a replacement
            m_mempool_outpoints.insert(input.prevout);
        }
        // outpoints created by this tx no longer exist
        for (uint32_t index{0}; index < tx->vout.size(); ++index) {
            m_mempool_outpoints.erase(COutPoint{tx->GetHash(), index});
        }
    }
};

struct TransactionsDelta final : public CValidationInterface {
    std::set<CTransactionRef>& m_added;

    explicit TransactionsDelta(std::set<CTransactionRef>& a)
        : m_added{a} {}

    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t mempool_sequence) override
    {
        AssertValidTransactionInfo(tx.info);
        Assert(mempool_sequence > 0);

        // Transactions may be entered and booted any number of times
        m_added.insert(tx.info.m_tx);
    }

    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override
    {
        Assert(tx);
        Assert(mempool_sequence > 0);

        // Transactions may be entered and booted any number of times
        m_added.erase(tx);
    }
};

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
    mempool_opts.limits.ancestor_count = fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 50);
    mempool_opts.limits.descendant_count = fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 50);
    mempool_opts.max_size_bytes = fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 200) * 1'000'000;
    mempool_opts.expiry = std::chrono::hours{fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 999)};
    // Only interested in 2 cases: sigop cost 0 or when single legacy sigop cost is >> 1KvB
    nBytesPerSigOp = fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, 1) * 10'000;

    mempool_opts.check_ratio = 1;
    mempool_opts.require_standard = fuzzed_data_provider.ConsumeBool();

    bilingual_str error;
    // ...and construct a CTxMemPool from it
    auto mempool{std::make_unique<CTxMemPool>(std::move(mempool_opts), error)};
    // ... ignore the error since it might be beneficial to fuzz even when the
    // mempool size is unreasonably small
    Assert(error.empty() || error.original.starts_with("-maxmempool must be at least "));
    return mempool;
}

std::unique_ptr<CTxMemPool> MakeEphemeralMempool(const NodeContext& node)
{
    // Take the default options for tests...
    CTxMemPool::Options mempool_opts{MemPoolOptionsForTest(node)};

    mempool_opts.check_ratio = 1;

    // Require standardness rules otherwise ephemeral dust is no-op
    mempool_opts.require_standard = true;

    // And set minrelay to 0 to allow ephemeral parent tx even with non-TRUC
    mempool_opts.min_relay_feerate = CFeeRate(0);

    bilingual_str error;
    // ...and construct a CTxMemPool from it
    auto mempool{std::make_unique<CTxMemPool>(std::move(mempool_opts), error)};
    Assert(error.empty());
    return mempool;
}

void AssertMempoolInputIndex(const CTxMemPool& tx_pool)
{
    LOCK(tx_pool.cs);

    size_t input_count{0};
    for (const auto& entry : tx_pool.mapTx) {
        const CTransaction& tx{entry.GetTx()};
        for (const CTxIn& txin : tx.vin) {
            ++input_count;
            const auto input_index{tx_pool.mapNextTx.find(txin.prevout)};
            assert(input_index != tx_pool.mapNextTx.end());
            assert(input_index->first == &txin.prevout);
            assert(input_index->second != tx_pool.mapTx.end());
            assert(&input_index->second->GetTx() == &tx);
            assert(tx_pool.GetConflictTx(txin.prevout) == &tx);
        }
    }
    assert(tx_pool.mapNextTx.size() == input_count);
    for (const auto& [prevout, spender] : tx_pool.mapNextTx) {
        assert(spender != tx_pool.mapTx.end());
        const CTransaction& tx{spender->GetTx()};
        assert(std::ranges::any_of(tx.vin, [&](const CTxIn& txin) {
            return txin.prevout == *prevout;
        }));
        assert(tx_pool.GetConflictTx(*prevout) == &tx);
    }
}

struct MempoolEntrySnapshot {
    Txid txid;
    Wtxid wtxid;
    std::chrono::seconds time;
    CAmount fee;
    int32_t vsize;
    int64_t fee_delta;

    bool operator==(const MempoolEntrySnapshot&) const = default;
};

struct PrioritisationSnapshot {
    Txid txid;
    bool in_mempool;
    CAmount delta;
    std::optional<CAmount> modified_fee;

    bool operator==(const PrioritisationSnapshot&) const = default;
};

struct InputIndexSnapshot {
    COutPoint prevout;
    Txid spender;

    bool operator==(const InputIndexSnapshot&) const = default;
};

struct MempoolSnapshot {
    std::vector<MempoolEntrySnapshot> entries;
    std::vector<PrioritisationSnapshot> prioritisations;
    std::vector<InputIndexSnapshot> input_index;
    std::set<Txid> unbroadcast_txids;
    uint64_t total_tx_size;
    CAmount total_fee;
    unsigned int transactions_updated;
};

MempoolSnapshot SnapshotMempool(const CTxMemPool& pool)
{
    std::vector<MempoolEntrySnapshot> entries;
    for (const auto& info : pool.infoAll()) {
        entries.push_back({
            info.tx->GetHash(),
            info.tx->GetWitnessHash(),
            info.m_time,
            info.fee,
            info.vsize,
            info.nFeeDelta,
        });
    }
    std::ranges::sort(entries, {}, &MempoolEntrySnapshot::txid);

    std::vector<PrioritisationSnapshot> prioritisations;
    for (const auto& delta : pool.GetPrioritisedTransactions()) {
        prioritisations.push_back({
            delta.txid,
            delta.in_mempool,
            delta.delta,
            delta.modified_fee,
        });
    }
    std::ranges::sort(prioritisations, {}, &PrioritisationSnapshot::txid);

    std::vector<InputIndexSnapshot> input_index;
    {
        LOCK(pool.cs);
        input_index.reserve(pool.mapNextTx.size());
        for (const auto& [prevout, spender] : pool.mapNextTx) {
            input_index.push_back({*prevout, spender->GetTx().GetHash()});
        }
    }
    std::ranges::sort(input_index, std::less<>{}, &InputIndexSnapshot::prevout);

    const auto [total_tx_size, total_fee] = WITH_LOCK(pool.cs, return std::make_pair(pool.GetTotalTxSize(), pool.GetTotalFee()));

    return {
        entries,
        prioritisations,
        input_index,
        pool.GetUnbroadcastTxs(),
        total_tx_size,
        total_fee,
        pool.GetTransactionsUpdated(),
    };
}

void AssertMempoolUnchanged(const CTxMemPool& pool, const MempoolSnapshot& expected)
{
    const auto actual{SnapshotMempool(pool)};
    Assert(actual.entries == expected.entries);
    Assert(actual.prioritisations == expected.prioritisations);
    Assert(actual.input_index == expected.input_index);
    Assert(actual.unbroadcast_txids == expected.unbroadcast_txids);
    Assert(actual.total_tx_size == expected.total_tx_size);
    Assert(actual.total_fee == expected.total_fee);
    Assert(actual.transactions_updated == expected.transactions_updated);
}

void AssertDuplicatePackageRejected(const std::vector<CTransactionRef>& txs)
{
    Assert(!txs.empty());
    auto duplicate_package{txs};
    duplicate_package.push_back(txs.front());

    Assert(!IsTopoSortedPackage(duplicate_package));
    PackageValidationState duplicate_state;
    Assert(!IsWellFormedPackage(duplicate_package, duplicate_state));
    Assert(duplicate_state.IsInvalid());
    Assert(duplicate_state.GetResult() == PackageValidationResult::PCKG_POLICY);

    int64_t duplicate_package_weight{0};
    for (const auto& tx : duplicate_package) {
        duplicate_package_weight += GetTransactionWeight(*tx);
    }

    if (duplicate_package.size() > MAX_PACKAGE_COUNT) {
        Assert(duplicate_state.GetRejectReason() == "package-too-many-transactions");
    } else if (duplicate_package.size() > 1 && duplicate_package_weight > MAX_PACKAGE_WEIGHT) {
        Assert(duplicate_state.GetRejectReason() == "package-too-large");
    } else {
        Assert(duplicate_state.GetRejectReason() == "package-contains-duplicates");
    }
}

void AssertATMPResultShape(const MempoolAcceptResult& res, const Wtxid& wtxid, bool submitted, bool txid_in_mempool, bool wtxid_in_mempool)
{
    switch (res.m_result_type) {
    case MempoolAcceptResult::ResultType::VALID:
        if (submitted) {
            Assert(txid_in_mempool);
            Assert(wtxid_in_mempool);
        }
        Assert(res.m_state.IsValid());
        Assert(!res.m_state.IsInvalid());
        Assert(res.m_vsize && *res.m_vsize > 0);
        Assert(res.m_base_fees && MoneyRange(*res.m_base_fees));
        Assert(res.m_effective_feerate);
        Assert(res.m_wtxids_fee_calculations);
        Assert(std::ranges::find(*res.m_wtxids_fee_calculations, wtxid) != res.m_wtxids_fee_calculations->end());
        Assert(!res.m_other_wtxid);
        break;
    case MempoolAcceptResult::ResultType::INVALID:
    {
        Assert(!res.m_state.IsValid());
        Assert(res.m_state.IsInvalid());
        Assert(!res.m_vsize);
        Assert(!res.m_base_fees);
        const bool is_reconsiderable{res.m_state.GetResult() == TxValidationResult::TX_RECONSIDERABLE};
        Assert(res.m_effective_feerate.has_value() == is_reconsiderable);
        Assert(res.m_wtxids_fee_calculations.has_value() == is_reconsiderable);
        if (res.m_wtxids_fee_calculations) {
            Assert(std::ranges::find(*res.m_wtxids_fee_calculations, wtxid) != res.m_wtxids_fee_calculations->end());
        }
        Assert(!res.m_other_wtxid);
        break;
    }
    case MempoolAcceptResult::ResultType::MEMPOOL_ENTRY:
    case MempoolAcceptResult::ResultType::DIFFERENT_WITNESS:
        Assert(false);
    }
}

void AssertPackageResultSubsetShape(const Package& txs,
                                    const PackageMempoolAcceptResult& result,
                                    const CTxMemPool& tx_pool,
                                    bool submitted)
{
    std::map<Wtxid, CTransactionRef> package_tx_by_wtxid;
    for (const auto& tx : txs) {
        package_tx_by_wtxid.emplace(tx->GetWitnessHash(), tx);
    }

    for (const auto& [wtxid, tx_result] : result.m_tx_results) {
        const auto tx_it{package_tx_by_wtxid.find(wtxid)};
        Assert(tx_it != package_tx_by_wtxid.end());
        AssertATMPResultShape(tx_result, wtxid, submitted,
                              tx_pool.exists(tx_it->second->GetHash()), tx_pool.exists(wtxid));
    }
}

// Scan mempool for a tx that has spent dust and return a
// prevout of the child that isn't the dusty parent itself.
// This is used to double-spend the child out of the mempool,
// leaving the parent childless.
// This assumes CheckMempoolEphemeralInvariants has passed for tx_pool.
std::optional<COutPoint> GetChildEvictingPrevout(const CTxMemPool& tx_pool)
{
    LOCK(tx_pool.cs);
    for (const auto& tx_info : tx_pool.infoAll()) {
        const auto& entry = *Assert(tx_pool.GetEntry(tx_info.tx->GetHash()));
        std::vector<uint32_t> dust_indexes{GetDust(*tx_info.tx, tx_pool.m_opts.dust_relay_feerate)};
        if (!dust_indexes.empty()) {
            const auto& children = tx_pool.GetChildren(entry);
            if (!children.empty()) {
                Assert(children.size() == 1);
                // Find an input that doesn't spend from parent's txid
                const auto& only_child = children.begin()->get().GetTx();
                for (const auto& tx_input : only_child.vin) {
                    if (tx_input.prevout.hash != tx_info.tx->GetHash()) {
                        return tx_input.prevout;
                    }
                }
            }
        }
    }

    return std::nullopt;
}

FUZZ_TARGET(ephemeral_package_eval, .init = initialize_tx_pool)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto& node = g_setup->m_node;
    auto& chainstate{static_cast<DummyChainState&>(node.chainman->ActiveChainstate())};

    MockTime(fuzzed_data_provider, chainstate);

    // All RBF-spendable outpoints outside of the unsubmitted package
    std::set<COutPoint> mempool_outpoints;
    std::unordered_map<COutPoint, CAmount, SaltedOutpointHasher> outpoints_value;
    for (const auto& outpoint : g_outpoints_coinbase_init_mature) {
        Assert(mempool_outpoints.insert(outpoint).second);
        outpoints_value[outpoint] = 50 * COIN;
    }

    auto outpoints_updater = std::make_shared<OutpointsUpdater>(mempool_outpoints);
    node.validation_signals->RegisterSharedValidationInterface(outpoints_updater);

    auto tx_pool_{MakeEphemeralMempool(node)};
    MockedTxPool& tx_pool = *static_cast<MockedTxPool*>(tx_pool_.get());

    chainstate.SetMempool(&tx_pool);

    LIMITED_WHILE (fuzzed_data_provider.remaining_bytes() > 0, 300) {
        Assert(!mempool_outpoints.empty());

        std::vector<CTransactionRef> txs;

        // Find something we may want to double-spend with two input single tx
        std::optional<COutPoint> outpoint_to_rbf{fuzzed_data_provider.ConsumeBool() ? GetChildEvictingPrevout(tx_pool) : std::nullopt};

        // Make small packages
        const auto num_txs = outpoint_to_rbf ? 1 : fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4);

        std::set<COutPoint> package_outpoints;
        while (txs.size() < num_txs) {
            // Create transaction to add to the mempool
            txs.emplace_back([&] {
                CMutableTransaction tx_mut;
                tx_mut.version = CTransaction::CURRENT_VERSION;
                tx_mut.nLockTime = 0;
                // Last transaction in a package needs to be a child of parents to get further in validation
                // so the last transaction to be generated(in a >1 package) must spend all package-made outputs
                // Note that this test currently only spends package outputs in last transaction.
                bool last_tx = num_txs > 1 && txs.size() == num_txs - 1;
                const auto num_in = outpoint_to_rbf ? 2 :
                    last_tx ? fuzzed_data_provider.ConsumeIntegralInRange<int>(package_outpoints.size()/2 + 1, package_outpoints.size()) :
                    fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 4);
                const auto num_out = outpoint_to_rbf ? 1 : fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 4);

                auto& outpoints = last_tx ? package_outpoints : mempool_outpoints;

                Assert((int)outpoints.size() >= num_in && num_in > 0);

                CAmount amount_in{0};
                for (int i = 0; i < num_in; ++i) {
                    // Pop random outpoint. We erase them to avoid double-spending
                    // while in this loop, but later add them back (unless last_tx).
                    auto pop = outpoints.begin();
                    std::advance(pop, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, outpoints.size() - 1));
                    auto outpoint = *pop;

                    if (i == 0 && outpoint_to_rbf) {
                        outpoint = *outpoint_to_rbf;
                        outpoints.erase(outpoint);
                    } else {
                        outpoints.erase(pop);
                    }
                    // no need to update or erase from outpoints_value
                    amount_in += outpoints_value.at(outpoint);

                    // Create input
                    CTxIn in;
                    in.prevout = outpoint;
                    in.scriptWitness.stack = P2WSH_EMPTY_TRUE_STACK;

                    tx_mut.vin.push_back(in);
                }

                const auto amount_fee = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, amount_in);
                const auto amount_out = (amount_in - amount_fee) / num_out;
                for (int i = 0; i < num_out; ++i) {
                    tx_mut.vout.emplace_back(amount_out, P2WSH_EMPTY);
                }

                // Note output amounts can naturally drop to dust on their own.
                if (!outpoint_to_rbf && fuzzed_data_provider.ConsumeBool()) {
                    uint32_t dust_index = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, num_out);
                    tx_mut.vout.insert(tx_mut.vout.begin() + dust_index, CTxOut(0, P2WSH_EMPTY));
                }

                auto tx = MakeTransactionRef(tx_mut);
                // Restore previously removed outpoints, except in-package outpoints (to allow RBF)
                if (!last_tx) {
                    for (const auto& in : tx->vin) {
                        Assert(outpoints.insert(in.prevout).second);
                    }
                    // Cache the in-package outpoints being made
                    for (size_t i = 0; i < tx->vout.size(); ++i) {
                        package_outpoints.emplace(tx->GetHash(), i);
                    }
                }
                // We need newly-created values for the duration of this run
                for (size_t i = 0; i < tx->vout.size(); ++i) {
                    outpoints_value[COutPoint(tx->GetHash(), i)] = tx->vout[i].nValue;
                }
                return tx;
            }());
        }
        AssertNonNullTransactionRefs(txs);
        AssertDuplicatePackageRejected(txs);

        if (fuzzed_data_provider.ConsumeBool()) {
            const auto& txid = fuzzed_data_provider.ConsumeBool() ?
                                   txs.back()->GetHash() :
                                   PickValue(fuzzed_data_provider, mempool_outpoints).hash;
            const auto delta = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-50 * COIN, +50 * COIN);
            // We only prioritise out of mempool transactions since PrioritiseTransaction doesn't
            // filter for ephemeral dust
            if (tx_pool.exists(txid)) {
                const auto tx_info{tx_pool.info(txid)};
                if (GetDust(*tx_info.tx, tx_pool.m_opts.dust_relay_feerate).empty()) {
                    tx_pool.PrioritiseTransaction(txid, delta);
                }
            }
        }

        const bool single_submit{txs.size() == 1};
        const bool package_test_accept{single_submit || (txs.size() > 1 && fuzzed_data_provider.ConsumeBool())};

        std::optional<MempoolSnapshot> test_accept_mempool_snapshot;
        std::optional<std::set<COutPoint>> test_accept_outpoints_snapshot;
        if (package_test_accept) {
            test_accept_mempool_snapshot = SnapshotMempool(tx_pool);
            test_accept_outpoints_snapshot = mempool_outpoints;
        }

        const auto result_package = WITH_LOCK(::cs_main,
                                    return ProcessNewPackage(chainstate, tx_pool, txs, /*test_accept=*/package_test_accept, /*client_maxfeerate=*/{}));

        if (package_test_accept) {
            node.validation_signals->SyncWithValidationInterfaceQueue();
            AssertMempoolUnchanged(tx_pool, *test_accept_mempool_snapshot);
            Assert(mempool_outpoints == *test_accept_outpoints_snapshot);
        }

        if (package_test_accept) {
            AssertPackageResultSubsetShape(txs, result_package, tx_pool, /*submitted=*/false);
        }
        if (package_test_accept && result_package.m_state.IsValid()) {
            Assert(!CheckPackageMempoolAcceptResult(txs, result_package, result_package.m_state.IsValid(), nullptr));
        }

        const auto res = WITH_LOCK(::cs_main, return AcceptToMemoryPool(chainstate, txs.back(), GetTime(),
                                   /*bypass_limits=*/false, /*test_accept=*/!single_submit));
        const bool passed = res.m_result_type == MempoolAcceptResult::ResultType::VALID;
        AssertATMPResultShape(res, txs.back()->GetWitnessHash(), /*submitted=*/single_submit,
                              tx_pool.exists(txs.back()->GetHash()), tx_pool.exists(txs.back()->GetWitnessHash()));
        Assert(!CheckPackageMempoolAcceptResult({txs.back()}, PackageMempoolAcceptResult{txs.back()->GetWitnessHash(), res},
                                                passed, nullptr));

        if (package_test_accept && !single_submit) {
            node.validation_signals->SyncWithValidationInterfaceQueue();
            AssertMempoolUnchanged(tx_pool, *test_accept_mempool_snapshot);
            Assert(mempool_outpoints == *test_accept_outpoints_snapshot);
        }

        if (!single_submit && result_package.m_state.GetResult() == PackageValidationResult::PCKG_POLICY) {
            // This is empty if it fails early checks, or "full" if transactions are looked at deeper.
            Assert(result_package.m_tx_results.size() == txs.size() || result_package.m_tx_results.empty());
        } else if (!package_test_accept && result_package.m_state.GetResult() != PackageValidationResult::PCKG_POLICY) {
            // We don't know anything about the validity since transactions were randomly generated, so
            // just use result_package.m_state here. This makes the expect_valid check meaningless, but
            // we can still verify that the contents of m_tx_results are consistent with m_state.
            const bool expect_valid{result_package.m_state.IsValid()};
            Assert(!CheckPackageMempoolAcceptResult(txs, result_package, expect_valid, &tx_pool));
        }

        node.validation_signals->SyncWithValidationInterfaceQueue();

        AssertMempoolInputIndex(tx_pool);
        CheckMempoolEphemeralInvariants(tx_pool);
    }

    node.validation_signals->UnregisterSharedValidationInterface(outpoints_updater);

    WITH_LOCK(::cs_main, tx_pool.check(chainstate.CoinsTip(), chainstate.m_chain.Height() + 1));
}


FUZZ_TARGET(tx_package_eval, .init = initialize_tx_pool)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto& node = g_setup->m_node;
    auto& chainstate{static_cast<DummyChainState&>(node.chainman->ActiveChainstate())};

    MockTime(fuzzed_data_provider, chainstate);

    // All RBF-spendable outpoints outside of the unsubmitted package
    std::set<COutPoint> mempool_outpoints;
    std::unordered_map<COutPoint, CAmount, SaltedOutpointHasher> outpoints_value;
    for (const auto& outpoint : g_outpoints_coinbase_init_mature) {
        Assert(mempool_outpoints.insert(outpoint).second);
        outpoints_value[outpoint] = 50 * COIN;
    }

    auto outpoints_updater = std::make_shared<OutpointsUpdater>(mempool_outpoints);
    node.validation_signals->RegisterSharedValidationInterface(outpoints_updater);

    auto tx_pool_{MakeMempool(fuzzed_data_provider, node)};
    MockedTxPool& tx_pool = *static_cast<MockedTxPool*>(tx_pool_.get());

    chainstate.SetMempool(&tx_pool);

    LIMITED_WHILE (fuzzed_data_provider.remaining_bytes() > 0, 300) {
        Assert(!mempool_outpoints.empty());

        std::vector<CTransactionRef> txs;

        // Make packages of 1-to-26 transactions
        const auto num_txs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 26);
        std::set<COutPoint> package_outpoints;
        while (txs.size() < num_txs) {
            // Create transaction to add to the mempool
            txs.emplace_back([&] {
                CMutableTransaction tx_mut;
                tx_mut.version = fuzzed_data_provider.ConsumeBool() ? TRUC_VERSION : CTransaction::CURRENT_VERSION;
                tx_mut.nLockTime = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<uint32_t>();
                // Last transaction in a package needs to be a child of parents to get further in validation
                // so the last transaction to be generated(in a >1 package) must spend all package-made outputs
                // Note that this test currently only spends package outputs in last transaction.
                bool last_tx = num_txs > 1 && txs.size() == num_txs - 1;
                const auto num_in = last_tx ? package_outpoints.size()  : fuzzed_data_provider.ConsumeIntegralInRange<int>(1, mempool_outpoints.size());
                auto num_out = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, mempool_outpoints.size() * 2);

                auto& outpoints = last_tx ? package_outpoints : mempool_outpoints;

                Assert(!outpoints.empty());

                CAmount amount_in{0};
                for (size_t i = 0; i < num_in; ++i) {
                    // Pop random outpoint. We erase them to avoid double-spending
                    // while in this loop, but later add them back (unless last_tx).
                    auto pop = outpoints.begin();
                    std::advance(pop, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, outpoints.size() - 1));
                    const auto outpoint = *pop;
                    outpoints.erase(pop);
                    // no need to update or erase from outpoints_value
                    amount_in += outpoints_value.at(outpoint);

                    // Create input
                    const auto sequence = ConsumeSequence(fuzzed_data_provider);
                    const auto script_sig = CScript{};
                    const auto script_wit_stack = fuzzed_data_provider.ConsumeBool() ? P2WSH_EMPTY_TRUE_STACK : P2WSH_EMPTY_TWO_STACK;

                    CTxIn in;
                    in.prevout = outpoint;
                    in.nSequence = sequence;
                    in.scriptSig = script_sig;
                    in.scriptWitness.stack = script_wit_stack;

                    tx_mut.vin.push_back(in);
                }

                // Duplicate an input
                bool dup_input = fuzzed_data_provider.ConsumeBool();
                if (dup_input) {
                    tx_mut.vin.push_back(tx_mut.vin.back());
                }

                // Refer to a non-existent input
                if (fuzzed_data_provider.ConsumeBool()) {
                    tx_mut.vin.emplace_back();
                }

                // Make a p2pk output to make sigops adjusted vsize to violate TRUC rules, potentially, which is never spent
                if (last_tx && amount_in > 1000 && fuzzed_data_provider.ConsumeBool()) {
                    tx_mut.vout.emplace_back(1000, CScript() << std::vector<unsigned char>(33, 0x02) << OP_CHECKSIG);
                    // Don't add any other outputs.
                    num_out = 1;
                    amount_in -= 1000;
                }

                const auto amount_fee = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, amount_in);
                const auto amount_out = (amount_in - amount_fee) / num_out;
                for (int i = 0; i < num_out; ++i) {
                    tx_mut.vout.emplace_back(amount_out, P2WSH_EMPTY);
                }
                auto tx = MakeTransactionRef(tx_mut);
                // Restore previously removed outpoints, except in-package outpoints
                if (!last_tx) {
                    for (const auto& in : tx->vin) {
                        // It's a fake input, or a new input, or a duplicate
                        Assert(in == CTxIn() || outpoints.insert(in.prevout).second || dup_input);
                    }
                    // Cache the in-package outpoints being made
                    for (size_t i = 0; i < tx->vout.size(); ++i) {
                        package_outpoints.emplace(tx->GetHash(), i);
                    }
                }
                // We need newly-created values for the duration of this run
                for (size_t i = 0; i < tx->vout.size(); ++i) {
                    outpoints_value[COutPoint(tx->GetHash(), i)] = tx->vout[i].nValue;
                }
                return tx;
            }());
        }
        AssertNonNullTransactionRefs(txs);
        AssertDuplicatePackageRejected(txs);

        if (fuzzed_data_provider.ConsumeBool()) {
            MockTime(fuzzed_data_provider, chainstate);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            tx_pool.RollingFeeUpdate();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            const auto& txid = fuzzed_data_provider.ConsumeBool() ?
                                   txs.back()->GetHash() :
                                   PickValue(fuzzed_data_provider, mempool_outpoints).hash;
            const auto delta = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-50 * COIN, +50 * COIN);
            tx_pool.PrioritiseTransaction(txid, delta);
        }

        // Remember all added transactions
        std::set<CTransactionRef> added;
        auto txr = std::make_shared<TransactionsDelta>(added);
        node.validation_signals->RegisterSharedValidationInterface(txr);

        // When there is only 1 transaction, we can run package test-accept followed by an ATMP submission.
        // Multi-transaction package test-accept keeps ATMP in test-accept mode so the whole iteration must
        // leave mempool state unchanged.
        const bool single_submit{txs.size() == 1 && fuzzed_data_provider.ConsumeBool()};
        const bool package_test_accept{single_submit || (txs.size() > 1 && fuzzed_data_provider.ConsumeBool())};

        // Exercise client_maxfeerate logic
        std::optional<CFeeRate> client_maxfeerate{};
        if (fuzzed_data_provider.ConsumeBool()) {
            client_maxfeerate = CFeeRate(fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-1, 50 * COIN), 100);
        }

        std::optional<MempoolSnapshot> test_accept_mempool_snapshot;
        std::optional<std::set<COutPoint>> test_accept_outpoints_snapshot;
        if (package_test_accept) {
            test_accept_mempool_snapshot = SnapshotMempool(tx_pool);
            test_accept_outpoints_snapshot = mempool_outpoints;
        }

        const auto result_package = WITH_LOCK(::cs_main,
                                    return ProcessNewPackage(chainstate, tx_pool, txs, /*test_accept=*/package_test_accept, client_maxfeerate));

        if (package_test_accept) {
            node.validation_signals->SyncWithValidationInterfaceQueue();
            AssertMempoolUnchanged(tx_pool, *test_accept_mempool_snapshot);
            Assert(mempool_outpoints == *test_accept_outpoints_snapshot);
            Assert(added.empty());
        }

        if (package_test_accept) {
            AssertPackageResultSubsetShape(txs, result_package, tx_pool, /*submitted=*/false);
        }
        if (package_test_accept && result_package.m_state.IsValid()) {
            Assert(!CheckPackageMempoolAcceptResult(txs, result_package, result_package.m_state.IsValid(), nullptr));
        }

        // Always set bypass_limits to false because it is not supported in ProcessNewPackage and
        // can be a source of divergence.
        const auto res = WITH_LOCK(::cs_main, return AcceptToMemoryPool(chainstate, txs.back(), GetTime(),
                                   /*bypass_limits=*/false, /*test_accept=*/!single_submit));
        const bool passed = res.m_result_type == MempoolAcceptResult::ResultType::VALID;
        AssertATMPResultShape(res, txs.back()->GetWitnessHash(), /*submitted=*/single_submit,
                              tx_pool.exists(txs.back()->GetHash()), tx_pool.exists(txs.back()->GetWitnessHash()));
        Assert(!CheckPackageMempoolAcceptResult({txs.back()}, PackageMempoolAcceptResult{txs.back()->GetWitnessHash(), res},
                                                passed, nullptr));

        node.validation_signals->SyncWithValidationInterfaceQueue();
        node.validation_signals->UnregisterSharedValidationInterface(txr);

        if (package_test_accept && !single_submit) {
            AssertMempoolUnchanged(tx_pool, *test_accept_mempool_snapshot);
            Assert(mempool_outpoints == *test_accept_outpoints_snapshot);
            Assert(added.empty());
        }

        // There is only 1 transaction in the package. We did a test-package-accept and a ATMP
        if (single_submit) {
            Assert(passed != added.empty());
            Assert(passed == res.m_state.IsValid());
            if (passed) {
                Assert(added.size() == 1);
                Assert(txs.back() == *added.begin());
            }
        } else if (result_package.m_state.GetResult() == PackageValidationResult::PCKG_POLICY) {
            // This is empty if it fails early checks, or "full" if transactions are looked at deeper
            Assert(result_package.m_tx_results.size() == txs.size() || result_package.m_tx_results.empty());
        } else if (!package_test_accept) {
            // We don't know anything about the validity since transactions were randomly generated, so
            // just use result_package.m_state here. This makes the expect_valid check meaningless, but
            // we can still verify that the contents of m_tx_results are consistent with m_state.
            const bool expect_valid{result_package.m_state.IsValid()};
            Assert(!CheckPackageMempoolAcceptResult(txs, result_package, expect_valid, &tx_pool));
        }

        AssertMempoolInputIndex(tx_pool);
        CheckMempoolTRUCInvariants(tx_pool);

        // Dust checks only make sense when dust is enforced
        if (tx_pool.m_opts.require_standard) {
            CheckMempoolEphemeralInvariants(tx_pool);
        }
    }

    node.validation_signals->UnregisterSharedValidationInterface(outpoints_updater);

    WITH_LOCK(::cs_main, tx_pool.check(chainstate.CoinsTip(), chainstate.m_chain.Height() + 1));
}
} // namespace
