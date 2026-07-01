// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <node/mempool_args.h>
#include <policy/rbf.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/overflow.h>
#include <util/translation.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;

CAmount ConsumeModifiedFee(FuzzedDataProvider& fuzzed_data_provider)
{
    switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 7)) {
    case 0:
        return std::numeric_limits<CAmount>::min();
    case 1:
        return std::numeric_limits<CAmount>::min() + CAmount{1};
    case 2:
        return -MAX_MONEY;
    case 3:
        return CAmount{0};
    case 4:
        return MAX_MONEY;
    case 5:
        return std::numeric_limits<CAmount>::max() - CAmount{1};
    case 6:
        return std::numeric_limits<CAmount>::max();
    default:
        return fuzzed_data_provider.ConsumeIntegral<CAmount>();
    }
}

void CheckPaysForRBF(FuzzedDataProvider& fuzzed_data_provider)
{
    const CAmount original_fees{ConsumeModifiedFee(fuzzed_data_provider)};
    const CAmount replacement_fees{ConsumeModifiedFee(fuzzed_data_provider)};
    const int32_t replacement_vsize{fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, 1'000'000)};
    const CFeeRate relay_fee{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, 100'000)};
    const Txid txid{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider))};

    const auto pays{PaysForRBF(original_fees, replacement_fees, replacement_vsize, relay_fee, txid)};
    if (replacement_fees < original_fees) {
        assert(pays.has_value());
        return;
    }

    const CAmount relay_fee_due{relay_fee.GetFee(replacement_vsize)};
    assert(relay_fee_due >= 0);
    const auto required_fees{CheckedAdd(original_fees, relay_fee_due)};
    assert(pays.has_value() == (!required_fees || replacement_fees < *required_fees));
}

void CheckUniqueClusterCount(const CTxMemPool& pool, const CTxMemPool::setEntries& entries) EXCLUSIVE_LOCKS_REQUIRED(pool.cs)
{
    CTxMemPool::setEntries remaining{entries};
    size_t cluster_count{0};
    while (!remaining.empty()) {
        const auto txiter{*remaining.begin()};
        const auto cluster{pool.GetCluster(txiter->GetTx().GetHash())};
        assert(!cluster.empty());
        bool contains_txiter{false};
        for (const auto* entry : cluster) {
            assert(entry != nullptr);
            if (entry == &*txiter) {
                contains_txiter = true;
            }
            const auto cluster_txiter{pool.GetIter(entry->GetTx().GetHash())};
            assert(cluster_txiter);
            remaining.erase(*cluster_txiter);
        }
        assert(contains_txiter);
        ++cluster_count;
    }
    const size_t unique_cluster_count{pool.GetUniqueClusterCount(entries)};
    assert(unique_cluster_count == cluster_count);
    assert((unique_cluster_count == 0) == entries.empty());
    assert(unique_cluster_count <= entries.size());
}
} // namespace

const int NUM_ITERS = 10000;

std::vector<COutPoint> g_outpoints;

void initialize_rbf()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();
}

void initialize_package_rbf()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();

    // Create a fixed set of unique "UTXOs" to source parents from
    // to avoid fuzzer giving circular references
    for (int i = 0; i < NUM_ITERS; ++i) {
        g_outpoints.emplace_back();
        g_outpoints.back().n = i;
    }

}

FUZZ_TARGET(rbf, .init = initialize_rbf)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CheckPaysForRBF(fuzzed_data_provider);
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (!mtx) {
        return;
    }

    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    Assert(error.empty());

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), NUM_ITERS) {
        const std::optional<CMutableTransaction> another_mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
        if (!another_mtx) {
            break;
        }
        const CTransaction another_tx{*another_mtx};
        if (fuzzed_data_provider.ConsumeBool() && !mtx->vin.empty()) {
            mtx->vin[0].prevout = COutPoint{another_tx.GetHash(), 0};
        }
        LOCK2(cs_main, pool.cs);
        if (!pool.GetIter(another_tx.GetHash())) {
            TryAddToMempool(pool, ConsumeTxMemPoolEntry(fuzzed_data_provider, another_tx));
        }
    }
    const CTransaction tx{*mtx};
    if (fuzzed_data_provider.ConsumeBool()) {
        LOCK2(cs_main, pool.cs);
        if (!pool.GetIter(tx.GetHash())) {
            TryAddToMempool(pool, ConsumeTxMemPoolEntry(fuzzed_data_provider, tx));
        }
    }
    {
        LOCK(pool.cs);
        (void)IsRBFOptIn(tx, pool);
    }
}

FUZZ_TARGET(package_rbf, .init = initialize_package_rbf)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};

    // "Real" virtual size is not important for this test since ConsumeTxMemPoolEntry generates its own virtual size values
    // so we construct small transactions for performance reasons. Child simply needs an input for later to perhaps connect to parent.
    CMutableTransaction child;
    child.vin.resize(1);

    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    Assert(error.empty());

    // Add a bunch of parent-child pairs to the mempool, and remember them.
    std::vector<CTransaction> mempool_txs;
    uint32_t iter{0};

    // Keep track of the total vsize of CTxMemPoolEntry's being added to the mempool to avoid overflow
    // Add replacement_vsize since this is added to new diagram during RBF check
    std::optional<CMutableTransaction> replacement_tx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (!replacement_tx) {
        return;
    }
    replacement_tx->vin.resize(1);
    replacement_tx->vin[0].prevout = g_outpoints.at(iter++);
    CTransaction replacement_tx_final{*replacement_tx};
    auto replacement_entry = ConsumeTxMemPoolEntry(fuzzed_data_provider, replacement_tx_final);
    int32_t replacement_weight = replacement_entry.GetAdjustedWeight();
    // Ensure that we don't hit FeeFrac limits, as we store TxGraph entries in terms of FeePerWeight
    int64_t running_vsize_total{replacement_entry.GetTxSize()};

    LOCK2(cs_main, pool.cs);

    while (fuzzed_data_provider.ConsumeBool()) {
        if (iter >= NUM_ITERS) break;

        // Make sure txns only have one input, and that a unique input is given to avoid circular references
        CMutableTransaction parent;
        parent.vin.resize(1);
        parent.vin[0].prevout = g_outpoints.at(iter++);
        parent.vout.emplace_back(0, CScript());

        mempool_txs.emplace_back(parent);
        const auto parent_entry = ConsumeTxMemPoolEntry(fuzzed_data_provider, mempool_txs.back());
        running_vsize_total += parent_entry.GetTxSize();
        if (running_vsize_total * WITNESS_SCALE_FACTOR > std::numeric_limits<int32_t>::max()) {
            // We aren't adding this final tx to mempool, so we don't want to conflict with it
            mempool_txs.pop_back();
            break;
        }
        assert(!pool.GetIter(parent_entry.GetTx().GetHash()));
        TryAddToMempool(pool, parent_entry);

        // It's possible that adding this to the mempool failed due to cluster
        // size limits; if so bail out.
        if(!pool.GetIter(parent_entry.GetTx().GetHash())) {
            mempool_txs.pop_back();
            continue;
        }

        child.vin[0].prevout = COutPoint{mempool_txs.back().GetHash(), 0};
        mempool_txs.emplace_back(child);
        const auto child_entry = ConsumeTxMemPoolEntry(fuzzed_data_provider, mempool_txs.back());
        running_vsize_total += child_entry.GetTxSize();
        if (running_vsize_total * WITNESS_SCALE_FACTOR > std::numeric_limits<int32_t>::max()) {
            // We aren't adding this final tx to mempool, so we don't want to conflict with it
            mempool_txs.pop_back();
            break;
        }
        if (!pool.GetIter(child_entry.GetTx().GetHash())) {
            TryAddToMempool(pool, child_entry);
            // Adding this transaction to the mempool may fail due to cluster
            // size limits; if so bail out.
            if(!pool.GetIter(child_entry.GetTx().GetHash())) {
                mempool_txs.pop_back();
                continue;
            }
        }

        if (fuzzed_data_provider.ConsumeBool()) {
            pool.PrioritiseTransaction(mempool_txs.back().GetHash(), fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(-100000, 100000));
        }
    }

    // Pick some transactions at random to be the direct conflicts
    CTxMemPool::setEntries direct_conflicts;
    for (auto& tx : mempool_txs) {
        if (fuzzed_data_provider.ConsumeBool() && pool.GetIter(tx.GetHash())) {
            direct_conflicts.insert(*pool.GetIter(tx.GetHash()));
        }
    }

    // Calculate all conflicts:
    CTxMemPool::setEntries all_conflicts;
    for (auto& txiter : direct_conflicts) {
        pool.CalculateDescendants(txiter, all_conflicts);
    }

    CheckUniqueClusterCount(pool, direct_conflicts);
    CheckUniqueClusterCount(pool, all_conflicts);

    CAmount replacement_fees = ConsumeMoney(fuzzed_data_provider);
    auto changeset = pool.GetChangeSet();
    for (auto& txiter : all_conflicts) {
        changeset->StageRemoval(txiter);
    }
    changeset->StageAddition(replacement_entry.GetSharedTx(), replacement_fees,
            replacement_entry.GetTime().count(), replacement_entry.GetHeight(),
            replacement_entry.GetSequence(), replacement_entry.GetSpendsCoinbase(),
            replacement_entry.GetSigOpCost(), replacement_entry.GetLockPoints());
    // Calculate the chunks for a replacement.
    auto calc_results{changeset->CalculateChunksForRBF()};

    if (calc_results.has_value()) {
        // Sanity checks on the chunks.

        // Feerates are monotonically decreasing.
        FeeFrac first_sum;
        for (size_t i = 0; i < calc_results->first.size(); ++i) {
            first_sum += calc_results->first[i];
            if (i) assert(ByRatio{calc_results->first[i - 1]} >= ByRatio{calc_results->first[i]});
        }
        FeeFrac second_sum;
        for (size_t i = 0; i < calc_results->second.size(); ++i) {
            second_sum += calc_results->second[i];
            if (i) assert(ByRatio{calc_results->second[i - 1]} >= ByRatio{calc_results->second[i]});
        }

        FeeFrac replaced;
        for (auto txiter : all_conflicts) {
            replaced.fee += txiter->GetModifiedFee();
            replaced.size += txiter->GetAdjustedWeight();
        }
        // The total fee & size of the new diagram minus replaced fee & size should be the total
        // fee & size of the old diagram minus replacement fee & size.
        assert((first_sum - replaced) == (second_sum - FeeFrac{replacement_fees, replacement_weight}));
    }

    // If internals report error, wrapper should too
    auto err_tuple{ImprovesFeerateDiagram(*changeset)};
    if (!calc_results.has_value()) {
         assert(err_tuple.value().first == DiagramCheckError::UNCALCULABLE);
    } else {
        // Diagram check succeeded
        auto old_sum = std::accumulate(calc_results->first.begin(), calc_results->first.end(), FeeFrac{});
        auto new_sum = std::accumulate(calc_results->second.begin(), calc_results->second.end(), FeeFrac{});
        const bool improves{std::is_gt(CompareChunks(calc_results->second, calc_results->first))};
        assert(improves == !err_tuple.has_value());
        if (improves) {
            // Strictly improving diagrams must not lose aggregate fees.
            assert(old_sum.fee <= new_sum.fee);
        } else {
            assert(err_tuple.value().first == DiagramCheckError::FAILURE);
        }
    }
}
