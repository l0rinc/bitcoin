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
#include <util/string.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>

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

struct MockedTxPool : public CTxMemPool {
    void RollingFeeUpdate() EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        lastRollingFeeUpdate = GetTime();
        blockSinceLastRollingFeeBump = true;
    }
};

struct MempoolEntryState final {
    Wtxid wtxid;
    std::chrono::seconds time;
    CAmount fee;
    int32_t vsize;
    int64_t fee_delta;

    friend bool operator==(const MempoolEntryState&, const MempoolEntryState&) = default;
};

struct MempoolState final {
    std::map<Txid, MempoolEntryState> entries;
    std::map<Txid, CAmount> fee_deltas;
    std::set<Txid> unbroadcast;
    uint64_t total_tx_size{0};
    CAmount total_fee{0};
    uint64_t sequence{0};
    unsigned transactions_updated{0};
    bool load_tried{false};

    friend bool operator==(const MempoolState&, const MempoolState&) = default;
};

MempoolState CaptureMempoolState(const CTxMemPool& tx_pool)
{
    MempoolState state;
    for (const auto& info : tx_pool.infoAll()) {
        const auto [_, inserted] = state.entries.emplace(
            info.tx->GetHash(),
            MempoolEntryState{info.tx->GetWitnessHash(), info.m_time, info.fee, info.vsize, info.nFeeDelta});
        Assert(inserted);
    }
    state.fee_deltas = WITH_LOCK(tx_pool.cs, return tx_pool.mapDeltas);
    state.unbroadcast = tx_pool.GetUnbroadcastTxs();
    state.total_tx_size = WITH_LOCK(tx_pool.cs, return tx_pool.GetTotalTxSize());
    state.total_fee = WITH_LOCK(tx_pool.cs, return tx_pool.GetTotalFee());
    state.sequence = WITH_LOCK(tx_pool.cs, return tx_pool.GetSequence());
    state.transactions_updated = tx_pool.GetTransactionsUpdated();
    state.load_tried = tx_pool.GetLoadTried();
    return state;
}

void AssertNoNewMempoolEntries(const MempoolState& before, const MempoolState& after)
{
    for (const auto& [txid, _] : after.entries) {
        Assert(before.entries.contains(txid));
    }
}

void AssertATMPTransition(const MempoolState& before, const MempoolState& after,
                          const CTransactionRef& tx, const MempoolAcceptResult& result)
{
    Assert(result.m_result_type == MempoolAcceptResult::ResultType::VALID ||
           result.m_result_type == MempoolAcceptResult::ResultType::INVALID);

    if (result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
        const auto txid{tx->GetHash()};
        Assert(after.entries.contains(txid));
        Assert(after.entries.at(txid).wtxid == tx->GetWitnessHash());
        for (const auto& [after_txid, _] : after.entries) {
            Assert(before.entries.contains(after_txid) || after_txid == txid);
        }
        for (const auto& replaced : result.m_replaced_transactions) {
            Assert(!after.entries.contains(replaced->GetHash()));
        }
        return;
    }

    // A failed ATMP call may evict old entries only when the failure is
    // reconsiderable (for example, a full mempool). It must never add one.
    AssertNoNewMempoolEntries(before, after);
    if (result.m_state.GetResult() != TxValidationResult::TX_RECONSIDERABLE) {
        Assert(before == after);
    }
}

std::set<Txid> LocalTxids(const std::set<CTransactionRef>& transactions,
                          const MempoolState& before, const MempoolState& after)
{
    std::set<Txid> txids;
    for (const auto& tx : transactions) {
        const auto txid{tx->GetHash()};
        // ValidationSignals is shared by the setup's mempools. Ignore a
        // callback that cannot belong to this local state transition.
        if (before.entries.contains(txid) || after.entries.contains(txid)) {
            Assert(txids.insert(txid).second);
        }
    }
    return txids;
}

void AssertValidationDelta(const MempoolState& before, const MempoolState& after,
                           const std::set<CTransactionRef>& removed,
                           const std::set<CTransactionRef>& added)
{
    std::set<Txid> expected_removed;
    for (const auto& [txid, _] : before.entries) {
        if (!after.entries.contains(txid)) expected_removed.insert(txid);
    }
    std::set<Txid> expected_added;
    for (const auto& [txid, _] : after.entries) {
        if (!before.entries.contains(txid)) expected_added.insert(txid);
    }
    Assert(expected_removed == LocalTxids(removed, before, after));
    Assert(expected_added == LocalTxids(added, before, after));
    const auto expected_updates{expected_removed.size() + expected_added.size()};
    Assert(after.transactions_updated >= before.transactions_updated);
    // A transaction can be added and evicted during one call, so the counter
    // may include transient entries that are absent from the net state delta.
    Assert(after.transactions_updated - before.transactions_updated >= expected_updates);
    for (const auto& tx : removed) {
        if (!before.entries.contains(tx->GetHash()) && !after.entries.contains(tx->GetHash())) continue;
        Assert(before.entries.contains(tx->GetHash()));
        Assert(!after.entries.contains(tx->GetHash()));
    }
    for (const auto& tx : added) {
        if (!before.entries.contains(tx->GetHash()) && !after.entries.contains(tx->GetHash())) continue;
        Assert(!before.entries.contains(tx->GetHash()));
        Assert(after.entries.contains(tx->GetHash()));
    }
}

void CheckMempoolContracts(MockedTxPool& tx_pool, Chainstate& chainstate)
{
    WITH_LOCK(::cs_main, tx_pool.check(chainstate.CoinsTip(), chainstate.m_chain.Height() + 1));
}

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

    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t /* mempool_sequence */) override
    {
        Assert(m_added.insert(tx.info.m_tx).second);
    }

    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t /* mempool_sequence */) override
    {
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

void Finish(FuzzedDataProvider& fuzzed_data_provider, MockedTxPool& tx_pool, Chainstate& chainstate)
{
    CheckMempoolContracts(tx_pool, chainstate);
    {
        BlockCreateOptions options{
            .block_min_fee_rate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)},
            .block_max_weight = fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(DEFAULT_BLOCK_RESERVED_WEIGHT, MAX_BLOCK_WEIGHT),
        };
        auto assembler = BlockAssembler{chainstate, &tx_pool, options};
        auto block_template = assembler.CreateNewBlock();
        Assert(block_template->block.vtx.size() >= 1);

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
    }
    CheckMempoolContracts(tx_pool, chainstate);
    const auto info_all = tx_pool.infoAll();
    if (!info_all.empty()) {
        const auto& tx_to_remove = *PickValue(fuzzed_data_provider, info_all).tx;
        WITH_LOCK(tx_pool.cs, tx_pool.removeRecursive(tx_to_remove, MemPoolRemovalReason::BLOCK /* dummy */));
        assert(tx_pool.size() < info_all.size());
        CheckMempoolContracts(tx_pool, chainstate);
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        // Try eviction
        {
            LOCK2(::cs_main, tx_pool.cs);
            tx_pool.TrimToSize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0U, tx_pool.DynamicMemoryUsage() * 2));
        }
        CheckMempoolContracts(tx_pool, chainstate);
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        // Try expiry
        {
            LOCK2(::cs_main, tx_pool.cs);
            tx_pool.Expire(GetMockTime() - std::chrono::seconds(fuzzed_data_provider.ConsumeIntegral<uint32_t>()));
        }
        CheckMempoolContracts(tx_pool, chainstate);
    }
    CheckMempoolContracts(tx_pool, chainstate);
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

void CheckATMPInvariants(const MempoolAcceptResult& res, bool txid_in_mempool, bool wtxid_in_mempool)
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
        Assert(res.m_effective_feerate);
        Assert(res.m_wtxids_fee_calculations);
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

void CheckPackageResultInvariants(const MempoolAcceptResult& res)
{
    Assert(res.m_result_type == MempoolAcceptResult::ResultType::VALID ||
           res.m_result_type == MempoolAcceptResult::ResultType::INVALID);
    if (res.m_result_type == MempoolAcceptResult::ResultType::VALID) {
        Assert(res.m_state.IsValid());
        Assert(res.m_vsize);
        Assert(res.m_base_fees);
        Assert(res.m_effective_feerate);
        Assert(res.m_wtxids_fee_calculations);
        Assert(!res.m_other_wtxid);
    } else {
        Assert(res.m_state.IsInvalid());
        Assert(!res.m_vsize);
        Assert(!res.m_base_fees);
        Assert(res.m_effective_feerate.has_value() == res.m_wtxids_fee_calculations.has_value());
        Assert(!res.m_other_wtxid);
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
            const auto delta = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-50 * COIN, +50 * COIN);
            tx_pool.PrioritiseTransaction(txid, delta);
        }

        // Remember all removed and added transactions
        const auto before_package_test = CaptureMempoolState(tx_pool);
        std::set<CTransactionRef> removed;
        std::set<CTransactionRef> added;
        auto txr = std::make_shared<TransactionsDelta>(removed, added);
        node.validation_signals->RegisterSharedValidationInterface(txr);

        // Make sure ProcessNewPackage on one transaction works.
        // The result is not guaranteed to be the same as what is returned by ATMP.
        const auto result_package = WITH_LOCK(::cs_main,
                                    return ProcessNewPackage(chainstate, tx_pool, {tx}, true, /*client_maxfeerate=*/{}));
        Assert(CaptureMempoolState(tx_pool) == before_package_test);
        // If something went wrong due to a package-specific policy, it might not return a
        // validation result for the transaction.
        if (result_package.m_state.GetResult() != PackageValidationResult::PCKG_POLICY) {
            const auto it = result_package.m_tx_results.find(tx->GetWitnessHash());
            Assert(it != result_package.m_tx_results.end());
            CheckPackageResultInvariants(it->second);
        }

        const auto before_atmp = CaptureMempoolState(tx_pool);
        const auto res = WITH_LOCK(::cs_main, return AcceptToMemoryPool(chainstate, tx, GetTime(), /*bypass_limits=*/false, /*test_accept=*/false));
        const bool accepted = res.m_result_type == MempoolAcceptResult::ResultType::VALID;
        node.validation_signals->SyncWithValidationInterfaceQueue();
        node.validation_signals->UnregisterSharedValidationInterface(txr);

        const auto after_atmp = CaptureMempoolState(tx_pool);
        AssertATMPTransition(before_atmp, after_atmp, tx, res);
        AssertValidationDelta(before_atmp, after_atmp, removed, added);
        CheckMempoolContracts(tx_pool, chainstate);

        const bool txid_in_mempool = tx_pool.exists(tx->GetHash());
        const bool wtxid_in_mempool = tx_pool.exists(tx->GetWitnessHash());
        CheckATMPInvariants(res, txid_in_mempool, wtxid_in_mempool);

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
            const auto delta = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-50 * COIN, +50 * COIN);
            tx_pool.PrioritiseTransaction(txid, delta);
        }

        const bool bypass_limits{fuzzed_data_provider.ConsumeBool()};
        ever_bypassed_limits |= bypass_limits;

        const auto tx = MakeTransactionRef(mut_tx);
        const auto before_atmp = CaptureMempoolState(tx_pool);
        const auto res = WITH_LOCK(::cs_main, return AcceptToMemoryPool(chainstate, tx, GetTime(), bypass_limits, /*test_accept=*/false));
        const bool accepted = res.m_result_type == MempoolAcceptResult::ResultType::VALID;
        const auto after_atmp = CaptureMempoolState(tx_pool);
        AssertATMPTransition(before_atmp, after_atmp, tx, res);
        CheckMempoolContracts(tx_pool, chainstate);
        if (accepted) {
            Assert(tx_pool.exists(tx->GetHash()));
            Assert(tx_pool.exists(tx->GetWitnessHash()));
        }
        if (accepted) {
            txids.push_back(tx->GetHash());
            if (!ever_bypassed_limits) {
                CheckMempoolTRUCInvariants(tx_pool);
            }
        }
    }
    Finish(fuzzed_data_provider, tx_pool, chainstate);
}
} // namespace
