// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <key_io.h>
#include <node/mempool_persist.h>
#include <policy/policy.h>
#include <streams.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <cstddef>
#include <limits>
#include <map>
#include <set>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(mempool_tests, TestingSetup)

static constexpr auto REMOVAL_REASON_DUMMY = MemPoolRemovalReason::REPLACED;

class MemPoolTest final : public CTxMemPool
{
public:
    using CTxMemPool::GetMinFee;
};

BOOST_AUTO_TEST_CASE(MempoolLookupTest)
{
    auto& pool = static_cast<MemPoolTest&>(*Assert(m_node.mempool));
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    auto make_tx = [](const uint64_t prevout_hash, const bool witness) {
        CMutableTransaction tx;
        tx.vin.emplace_back(COutPoint{Txid::FromUint256(uint256{static_cast<uint8_t>(prevout_hash)}), 0});
        tx.vin[0].scriptSig = CScript() << OP_1;
        if (witness) tx.vin[0].scriptWitness.stack.push_back({0x01});
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
        tx.vout[0].nValue = 10 * COIN;
        return tx;
    };

    const CMutableTransaction no_witness_mut{make_tx(1, /*witness=*/false)};
    const CTransaction no_witness_tx{no_witness_mut};
    BOOST_CHECK(no_witness_tx.GetHash().ToUint256() == no_witness_tx.GetWitnessHash().ToUint256());

    const CMutableTransaction witness_mut{make_tx(2, /*witness=*/true)};
    const CTransaction witness_tx{witness_mut};
    BOOST_CHECK(witness_tx.GetHash().ToUint256() != witness_tx.GetWitnessHash().ToUint256());

    for (const CTransaction& tx : {no_witness_tx, witness_tx}) {
        BOOST_CHECK(!pool.get(tx.GetHash()));
        BOOST_CHECK(!pool.get(tx.GetWitnessHash()));
    }

    auto check_lookup = [&](const CTransaction& tx) EXCLUSIVE_LOCKS_REQUIRED(pool.cs) {
        BOOST_CHECK(pool.exists(tx.GetHash()));
        BOOST_CHECK(pool.exists(tx.GetWitnessHash()));

        const auto by_txid_iter{pool.GetIter(tx.GetHash())};
        BOOST_REQUIRE(by_txid_iter);
        BOOST_CHECK((*by_txid_iter)->GetTx().GetHash() == tx.GetHash());
        BOOST_CHECK((*by_txid_iter)->GetTx().GetWitnessHash() == tx.GetWitnessHash());

        const auto by_wtxid_iter{pool.GetIter(tx.GetWitnessHash())};
        BOOST_REQUIRE(by_wtxid_iter);
        BOOST_CHECK(&**by_wtxid_iter == &**by_txid_iter);

        const auto by_txid{pool.get(tx.GetHash())};
        BOOST_REQUIRE(by_txid);
        BOOST_CHECK(by_txid->GetHash() == tx.GetHash());
        BOOST_CHECK(by_txid->GetWitnessHash() == tx.GetWitnessHash());

        const auto by_wtxid{pool.get(tx.GetWitnessHash())};
        BOOST_REQUIRE(by_wtxid);
        BOOST_CHECK(by_wtxid == by_txid);

        const uint64_t entry_sequence{(*by_txid_iter)->GetSequence()};
        BOOST_CHECK(!pool.info_for_relay(tx.GetHash(), entry_sequence).tx);
        BOOST_CHECK(!pool.info_for_relay(tx.GetWitnessHash(), entry_sequence).tx);
        if (entry_sequence < std::numeric_limits<uint64_t>::max()) {
            BOOST_CHECK(pool.info_for_relay(tx.GetHash(), entry_sequence + 1).tx == by_txid);
            BOOST_CHECK(pool.info_for_relay(tx.GetWitnessHash(), entry_sequence + 1).tx == by_txid);
        }
    };

    TryAddToMempool(pool, entry.Fee(1000LL).FromTx(no_witness_mut));
    check_lookup(no_witness_tx);

    TryAddToMempool(pool, entry.Fee(1000LL).FromTx(witness_mut));
    check_lookup(witness_tx);
}

static void CheckMempoolInputIndexCount(const CTxMemPool& pool)
{
    size_t input_count{0};
    for (const auto& tx_info : pool.infoAll()) {
        input_count += tx_info.tx->vin.size();
    }
    size_t indexed_input_count{0};
    {
        LOCK(pool.cs);
        indexed_input_count = pool.mapNextTx.size();
    }
    BOOST_CHECK_EQUAL(indexed_input_count, input_count);
}

static void CheckMempoolRandomizedIndex(const CTxMemPool& pool)
{
    LOCK(pool.cs);
    BOOST_CHECK_EQUAL(pool.txns_randomized.size(), pool.mapTx.size());

    std::set<Txid> randomized_txids;
    for (size_t index{0}; index < pool.txns_randomized.size(); ++index) {
        const auto& [wtxid, it] = pool.txns_randomized[index];
        BOOST_REQUIRE(it != pool.mapTx.end());
        BOOST_CHECK(it->GetTx().GetWitnessHash() == wtxid);
        BOOST_CHECK_EQUAL(it->idx_randomized, index);
        const Txid txid{it->GetTx().GetHash()};
        BOOST_CHECK(pool.mapTx.find(txid) == it);
        BOOST_CHECK(randomized_txids.insert(txid).second);
    }
    for (const auto& entry : pool.mapTx) {
        BOOST_CHECK(randomized_txids.contains(entry.GetTx().GetHash()));
    }
}

static void WriteFileBytes(const fs::path& path, const std::vector<std::byte>& bytes)
{
    AutoFile file{fsbridge::fopen(path, "wb")};
    BOOST_REQUIRE(!file.IsNull());
    file.write({bytes.data(), bytes.size()});
    BOOST_REQUIRE_EQUAL(file.fclose(), 0);
}

static std::vector<std::byte> ReadFileBytes(const fs::path& path)
{
    AutoFile file{fsbridge::fopen(path, "rb")};
    BOOST_REQUIRE(!file.IsNull());
    std::vector<std::byte> bytes(file.size());
    file.read({bytes.data(), bytes.size()});
    return bytes;
}

static void CheckFailedDumpPreservesFile(const CTxMemPool& pool, const fs::path& path, const std::vector<std::byte>& existing_bytes, fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen)
{
    fs::remove_all(path);
    fs::remove_all(path + ".new");
    WriteFileBytes(path, existing_bytes);
    BOOST_CHECK(!node::DumpMempool(pool, path, std::move(mockable_fopen_function)));
    BOOST_CHECK(ReadFileBytes(path) == existing_bytes);
}

BOOST_AUTO_TEST_CASE(MempoolRemoveForBlockRejectsNullTxRefs)
{
    test_only_CheckFailuresAreExceptionsNotAborts failed_asserts_throw{};
    auto& pool{*Assert(m_node.mempool)};
    std::vector<CTransactionRef> resized_block_vtx(1);

    LOCK(pool.cs);
    BOOST_CHECK_THROW(pool.removeForBlock(resized_block_vtx, /*nBlockHeight=*/1), NonFatalCheckError);
}

BOOST_AUTO_TEST_CASE(MempoolPrioritisationTest)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    TestMemPoolEntryHelper entry;

    const auto make_tx = [](uint32_t prevout_n) {
        CMutableTransaction tx_mut;
        tx_mut.vin.resize(1);
        tx_mut.vin[0].prevout = COutPoint{Txid::FromUint256(uint256{static_cast<uint8_t>(prevout_n + 1)}), prevout_n};
        tx_mut.vin[0].scriptSig = CScript() << OP_11;
        tx_mut.vout.resize(1);
        tx_mut.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx_mut.vout[0].nValue = 1;
        return MakeTransactionRef(tx_mut);
    };

    const CTransactionRef tx{make_tx(0)};
    const Txid txid{tx->GetHash()};
    constexpr CAmount base_fee{10'000};

    {
        LOCK2(::cs_main, pool.cs);
        TryAddToMempool(pool, entry.Fee(base_fee).FromTx(tx));
    }

    const auto check_modified_fee = [&] (const CAmount expected_modified_fee) {
        LOCK(pool.cs);
        const auto it{pool.GetIter(txid)};
        BOOST_REQUIRE(it.has_value());
        BOOST_CHECK_EQUAL((*it)->GetModifiedFee(), expected_modified_fee);
    };
    const auto check_prioritisation = [&] (const CAmount expected_delta, const CAmount expected_modified_fee) {
        const auto deltas{pool.GetPrioritisedTransactions()};
        BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
        BOOST_CHECK(deltas.front().txid == txid);
        BOOST_CHECK(deltas.front().in_mempool);
        BOOST_CHECK_EQUAL(deltas.front().delta, expected_delta);
        BOOST_REQUIRE(deltas.front().modified_fee.has_value());
        BOOST_CHECK_EQUAL(*deltas.front().modified_fee, expected_modified_fee);
        check_modified_fee(expected_modified_fee);
    };

    pool.PrioritiseTransaction(txid, 1'234);
    check_prioritisation(/*expected_delta=*/1'234, /*expected_modified_fee=*/base_fee + 1'234);

    {
        LOCK(pool.cs);
        pool.ClearPrioritisation(txid);
    }
    BOOST_CHECK(pool.GetPrioritisedTransactions().empty());
    check_modified_fee(base_fee);

    constexpr CAmount max_delta{std::numeric_limits<CAmount>::max()};
    pool.PrioritiseTransaction(txid, max_delta);
    check_prioritisation(max_delta, max_delta);

    pool.PrioritiseTransaction(txid, -max_delta);
    BOOST_CHECK(pool.GetPrioritisedTransactions().empty());
    check_modified_fee(base_fee);

    const CTransactionRef tx2{make_tx(1)};
    const Txid txid2{tx2->GetHash()};
    {
        LOCK2(::cs_main, pool.cs);
        TryAddToMempool(pool, entry.Fee(base_fee).FromTx(tx2));
    }

    pool.PrioritiseTransaction(txid, max_delta);
    pool.PrioritiseTransaction(txid2, max_delta);
    {
        LOCK(pool.cs);
        const auto diagram{pool.GetFeerateDiagram()};
        BOOST_REQUIRE(!diagram.empty());
        BOOST_CHECK_EQUAL(diagram.back().fee, max_delta);
        pool.TrimToSize(0);
        BOOST_CHECK(pool.mapTx.empty());
    }
}

BOOST_AUTO_TEST_CASE(MempoolBlockBuilderChunkIdempotent)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    TestMemPoolEntryHelper entry;

    const auto make_tx = [](uint32_t prevout_n, CAmount value) {
        CMutableTransaction tx_mut;
        tx_mut.vin.resize(1);
        tx_mut.vin[0].prevout = COutPoint{Txid::FromUint256(uint256{static_cast<uint8_t>(prevout_n + 1)}), prevout_n};
        tx_mut.vin[0].scriptSig = CScript() << OP_11;
        tx_mut.vout.resize(1);
        tx_mut.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx_mut.vout[0].nValue = value;
        return MakeTransactionRef(tx_mut);
    };
    const auto same_entries = [](const auto& a, const auto& b) {
        if (a.size() != b.size()) return false;
        for (size_t i{0}; i < a.size(); ++i) {
            if (&a[i].get() != &b[i].get()) return false;
        }
        return true;
    };

    TryAddToMempool(pool, entry.Fee(20'000).FromTx(make_tx(/*prevout_n=*/0, /*value=*/2)));
    TryAddToMempool(pool, entry.Fee(10'000).FromTx(make_tx(/*prevout_n=*/1, /*value=*/1)));

    bool first_chunk_empty{true};
    bool same_chunk{false};
    bool same_feerate{false};
    bool include_diagram_matches{false};
    bool include_seen_all_entries{false};
    bool include_duplicate_entries{false};
    bool skipped_first_chunk_nonempty{false};
    bool skipped_chunk_reappeared{false};
    bool skip_duplicate_entries{false};
    {
        LOCK(pool.cs);
        pool.StartBlockBuilding();
        std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> first_entries;
        const FeePerWeight first_feerate{pool.GetBlockBuilderChunk(first_entries)};
        std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> second_entries;
        const FeePerWeight second_feerate{pool.GetBlockBuilderChunk(second_entries)};
        first_chunk_empty = first_entries.empty();
        same_chunk = same_entries(first_entries, second_entries);
        same_feerate = first_feerate == second_feerate;
        pool.StopBlockBuilding();

        const auto diagram{pool.GetFeerateDiagram()};
        std::vector<FeePerWeight> builder_diagram{FeePerWeight{}};
        std::set<const CTxMemPoolEntry*> seen_entries;
        pool.StartBlockBuilding();
        while (true) {
            std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> entries;
            const FeePerWeight chunk_feerate{pool.GetBlockBuilderChunk(entries)};
            if (chunk_feerate == FeePerWeight{}) break;
            for (const auto& entry_ref : entries) {
                include_duplicate_entries |= !seen_entries.insert(&entry_ref.get()).second;
            }
            builder_diagram.push_back(FeePerWeight{
                builder_diagram.back().fee + chunk_feerate.fee,
                builder_diagram.back().size + chunk_feerate.size,
            });
            pool.IncludeBuilderChunk();
        }
        pool.StopBlockBuilding();
        include_diagram_matches = builder_diagram == diagram;
        include_seen_all_entries = seen_entries.size() == pool.mapTx.size();

        std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> skipped_entries;
        std::set<const CTxMemPoolEntry*> skipped_set;
        pool.StartBlockBuilding();
        const FeePerWeight skipped_feerate{pool.GetBlockBuilderChunk(skipped_entries)};
        skipped_first_chunk_nonempty = skipped_feerate != FeePerWeight{} && !skipped_entries.empty();
        for (const auto& entry_ref : skipped_entries) {
            skip_duplicate_entries |= !skipped_set.insert(&entry_ref.get()).second;
        }
        if (skipped_first_chunk_nonempty) {
            pool.SkipBuilderChunk();
            std::set<const CTxMemPoolEntry*> seen_after_skip;
            while (true) {
                std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> entries;
                const FeePerWeight chunk_feerate{pool.GetBlockBuilderChunk(entries)};
                if (chunk_feerate == FeePerWeight{}) break;
                for (const auto& entry_ref : entries) {
                    skipped_chunk_reappeared |= skipped_set.contains(&entry_ref.get());
                    skip_duplicate_entries |= !seen_after_skip.insert(&entry_ref.get()).second;
                }
                pool.IncludeBuilderChunk();
            }
        }
        pool.StopBlockBuilding();
    }

    BOOST_CHECK(!first_chunk_empty);
    BOOST_CHECK(same_chunk);
    BOOST_CHECK(same_feerate);
    BOOST_CHECK(include_diagram_matches);
    BOOST_CHECK(include_seen_all_entries);
    BOOST_CHECK(!include_duplicate_entries);
    BOOST_CHECK(skipped_first_chunk_nonempty);
    BOOST_CHECK(!skipped_chunk_reappeared);
    BOOST_CHECK(!skip_duplicate_entries);
}

BOOST_AUTO_TEST_CASE(MempoolRemoveTest)
{
    // Test CTxMemPool::remove functionality

    TestMemPoolEntryHelper entry;
    // Parent transaction with three children,
    // and three grand-children:
    CMutableTransaction txParent;
    txParent.vin.resize(1);
    txParent.vin[0].scriptSig = CScript() << OP_11;
    txParent.vout.resize(3);
    for (int i = 0; i < 3; i++)
    {
        txParent.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txParent.vout[i].nValue = 33000LL;
    }
    CMutableTransaction txChild[3];
    for (int i = 0; i < 3; i++)
    {
        txChild[i].vin.resize(1);
        txChild[i].vin[0].scriptSig = CScript() << OP_11;
        txChild[i].vin[0].prevout.hash = txParent.GetHash();
        txChild[i].vin[0].prevout.n = i;
        txChild[i].vout.resize(1);
        txChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txChild[i].vout[0].nValue = 11000LL;
    }
    CMutableTransaction txGrandChild[3];
    for (int i = 0; i < 3; i++)
    {
        txGrandChild[i].vin.resize(1);
        txGrandChild[i].vin[0].scriptSig = CScript() << OP_11;
        txGrandChild[i].vin[0].prevout.hash = txChild[i].GetHash();
        txGrandChild[i].vin[0].prevout.n = 0;
        txGrandChild[i].vout.resize(1);
        txGrandChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txGrandChild[i].vout[0].nValue = 11000LL;
    }


    CTxMemPool& testPool = *Assert(m_node.mempool);
    LOCK2(::cs_main, testPool.cs);

    // Nothing in pool, remove should do nothing:
    unsigned int poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    CheckMempoolInputIndexCount(testPool);

    // Just the parent:
    TryAddToMempool(testPool, entry.FromTx(txParent));
    CheckMempoolInputIndexCount(testPool);
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 1);
    CheckMempoolInputIndexCount(testPool);

    // Parent, children, grandchildren:
    TryAddToMempool(testPool, entry.FromTx(txParent));
    for (int i = 0; i < 3; i++)
    {
        TryAddToMempool(testPool, entry.FromTx(txChild[i]));
        TryAddToMempool(testPool, entry.FromTx(txGrandChild[i]));
    }
    CheckMempoolInputIndexCount(testPool);
    // Remove Child[0], GrandChild[0] should be removed:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txChild[0]), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 2);
    CheckMempoolInputIndexCount(testPool);
    // ... make sure grandchild and child are gone:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txGrandChild[0]), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    CheckMempoolInputIndexCount(testPool);
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txChild[0]), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    CheckMempoolInputIndexCount(testPool);
    // Remove parent, all children/grandchildren should go:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 5);
    BOOST_CHECK_EQUAL(testPool.size(), 0U);
    CheckMempoolInputIndexCount(testPool);

    // Add children and grandchildren, but NOT the parent (simulate the parent being in a block)
    for (int i = 0; i < 3; i++)
    {
        TryAddToMempool(testPool, entry.FromTx(txChild[i]));
        TryAddToMempool(testPool, entry.FromTx(txGrandChild[i]));
    }
    CheckMempoolInputIndexCount(testPool);
    // Now remove the parent, as might happen if a block-re-org occurs but the parent cannot be
    // put into the mempool (maybe because it is non-standard):
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 6);
    BOOST_CHECK_EQUAL(testPool.size(), 0U);
    CheckMempoolInputIndexCount(testPool);
}

BOOST_AUTO_TEST_CASE(MempoolLoadUnbroadcastRequiresMempoolEntry)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    BOOST_REQUIRE(pool.GetUnbroadcastTxs().empty());

    const Txid missing_txid{Txid::FromUint256(uint256{1})};
    pool.AddUnbroadcastTx(missing_txid);
    BOOST_CHECK(pool.GetUnbroadcastTxs().empty());

    const fs::path mempool_path{m_args.GetDataDirBase() / "mempool_unbroadcast_requires_entry.dat"};
    {
        AutoFile file{fsbridge::fopen(mempool_path, "wb")};
        BOOST_REQUIRE(!file.IsNull());
        file << uint64_t{1}; // Legacy v1 mempool dump version, without an obfuscation key.
        file.SetObfuscation({});
        file << uint64_t{0}; // No transactions to load.
        file << std::map<Txid, CAmount>{};
        file << std::set<Txid>{missing_txid};
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }

    BOOST_CHECK(node::LoadMempool(pool, mempool_path, m_node.chainman->ActiveChainstate(), {}));
    BOOST_CHECK(!pool.exists(missing_txid));
    BOOST_CHECK(pool.GetUnbroadcastTxs().empty());
    fs::remove(mempool_path);
}

BOOST_AUTO_TEST_CASE(MempoolDumpFailurePreservesExistingFile)
{
    const CTxMemPool& pool{*Assert(m_node.mempool)};
    const fs::path mempool_path{m_args.GetDataDirBase() / "mempool_dump_failure_preserves_existing.dat"};
    const fs::path redirected_path{m_args.GetDataDirBase() / "mempool_dump_failure_redirected.dat"};
    const std::vector<std::byte> existing_bytes{
        std::byte{0x6d},
        std::byte{0x65},
        std::byte{0x6d},
        std::byte{0x70},
        std::byte{0x6f},
        std::byte{0x6f},
        std::byte{0x6c},
    };

    fs::remove_all(mempool_path);
    fs::remove_all(mempool_path + ".new");
    WriteFileBytes(mempool_path, existing_bytes);
    BOOST_REQUIRE(fs::create_directory(mempool_path + ".new"));
    BOOST_CHECK(!node::DumpMempool(pool, mempool_path));
    BOOST_CHECK(ReadFileBytes(mempool_path) == existing_bytes);
    fs::remove_all(mempool_path + ".new");

    fs::remove_all(redirected_path);
    CheckFailedDumpPreservesFile(pool, mempool_path, existing_bytes, [&](const fs::path&, const char* mode) {
        return fsbridge::fopen(redirected_path, mode);
    });

    fs::remove_all(mempool_path);
    fs::remove_all(mempool_path + ".new");
    fs::remove_all(redirected_path);
}

BOOST_AUTO_TEST_CASE(MempoolLoadTruncatedMetadata)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    BOOST_REQUIRE_EQUAL(pool.size(), 0U);
    BOOST_REQUIRE(pool.GetPrioritisedTransactions().empty());
    BOOST_REQUIRE(pool.GetUnbroadcastTxs().empty());

    const fs::path mempool_path{m_args.GetDataDirBase() / "mempool_truncated_metadata.dat"};
    {
        AutoFile file{fsbridge::fopen(mempool_path, "wb")};
        BOOST_REQUIRE(!file.IsNull());
        file << uint64_t{1}; // Legacy v1 mempool dump version, without an obfuscation key.
        file.SetObfuscation({});
        file << uint64_t{0}; // No transactions to load.
        file << std::map<Txid, CAmount>{};
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }

    BOOST_CHECK(!node::LoadMempool(pool, mempool_path, m_node.chainman->ActiveChainstate(), {}));
    BOOST_CHECK_EQUAL(pool.size(), 0U);
    BOOST_CHECK(pool.GetPrioritisedTransactions().empty());
    BOOST_CHECK(pool.GetUnbroadcastTxs().empty());
    fs::remove(mempool_path);
}

BOOST_AUTO_TEST_CASE(MempoolDumpLoadPrioritisationRoundtrip)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    BOOST_REQUIRE_EQUAL(pool.size(), 0U);
    BOOST_REQUIRE(pool.GetPrioritisedTransactions().empty());
    BOOST_REQUIRE(pool.GetUnbroadcastTxs().empty());

    const Txid missing_txid{Txid::FromUint256(uint256{2})};
    constexpr CAmount fee_delta{12345};
    pool.PrioritiseTransaction(missing_txid, fee_delta);
    pool.AddUnbroadcastTx(missing_txid);

    {
        const auto deltas{pool.GetPrioritisedTransactions()};
        BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
        BOOST_CHECK(deltas.front().txid == missing_txid);
        BOOST_CHECK(!deltas.front().in_mempool);
        BOOST_CHECK_EQUAL(deltas.front().delta, fee_delta);
        BOOST_CHECK(!deltas.front().modified_fee.has_value());
    }
    BOOST_CHECK(pool.GetUnbroadcastTxs().empty());

    const fs::path mempool_path{m_args.GetDataDirBase() / "mempool_prioritisation_roundtrip.dat"};
    fs::remove(mempool_path);
    fs::remove(mempool_path + ".new");
    BOOST_REQUIRE(node::DumpMempool(pool, mempool_path));

    {
        LOCK(pool.cs);
        pool.ClearPrioritisation(missing_txid);
    }
    BOOST_REQUIRE(pool.GetPrioritisedTransactions().empty());

    BOOST_REQUIRE(node::LoadMempool(pool, mempool_path, m_node.chainman->ActiveChainstate(), {}));
    BOOST_CHECK_EQUAL(pool.size(), 0U);
    BOOST_CHECK(pool.GetUnbroadcastTxs().empty());

    const auto deltas{pool.GetPrioritisedTransactions()};
    BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
    BOOST_CHECK(deltas.front().txid == missing_txid);
    BOOST_CHECK(!deltas.front().in_mempool);
    BOOST_CHECK_EQUAL(deltas.front().delta, fee_delta);
    BOOST_CHECK(!deltas.front().modified_fee.has_value());

    fs::remove(mempool_path);
    fs::remove(mempool_path + ".new");
}

BOOST_AUTO_TEST_CASE(MempoolDumpLoadV1PrioritisationRoundtrip)
{
    auto mempool_opts{MemPoolOptionsForTest(m_node)};
    mempool_opts.persist_v1_dat = true;
    bilingual_str error;
    CTxMemPool dump_pool{mempool_opts, error};
    BOOST_REQUIRE(error.empty());

    const Txid missing_txid{Txid::FromUint256(uint256{3})};
    constexpr CAmount fee_delta{12345};
    dump_pool.PrioritiseTransaction(missing_txid, fee_delta);
    dump_pool.AddUnbroadcastTx(missing_txid);
    BOOST_CHECK(dump_pool.GetUnbroadcastTxs().empty());

    const fs::path mempool_path{m_args.GetDataDirBase() / "mempool_v1_prioritisation_roundtrip.dat"};
    fs::remove(mempool_path);
    fs::remove(mempool_path + ".new");
    BOOST_REQUIRE(node::DumpMempool(dump_pool, mempool_path));

    {
        AutoFile file{fsbridge::fopen(mempool_path, "rb")};
        BOOST_REQUIRE(!file.IsNull());
        uint64_t version;
        file >> version;
        BOOST_CHECK_EQUAL(version, 1U);
    }

    bilingual_str reload_error;
    CTxMemPool reload_pool{MemPoolOptionsForTest(m_node), reload_error};
    BOOST_REQUIRE(reload_error.empty());
    BOOST_REQUIRE(node::LoadMempool(reload_pool, mempool_path, m_node.chainman->ActiveChainstate(), {}));
    BOOST_CHECK_EQUAL(reload_pool.size(), 0U);
    BOOST_CHECK(reload_pool.GetUnbroadcastTxs().empty());

    const auto deltas{reload_pool.GetPrioritisedTransactions()};
    BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
    BOOST_CHECK(deltas.front().txid == missing_txid);
    BOOST_CHECK(!deltas.front().in_mempool);
    BOOST_CHECK_EQUAL(deltas.front().delta, fee_delta);
    BOOST_CHECK(!deltas.front().modified_fee.has_value());

    fs::remove(mempool_path);
    fs::remove(mempool_path + ".new");
}

BOOST_AUTO_TEST_CASE(MempoolLoadSkipsDisabledPrioritisation)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    BOOST_REQUIRE_EQUAL(pool.size(), 0U);
    BOOST_REQUIRE(pool.GetPrioritisedTransactions().empty());
    BOOST_REQUIRE(pool.GetUnbroadcastTxs().empty());

    const Txid missing_txid{Txid::FromUint256(uint256{3})};
    constexpr CAmount fee_delta{12345};
    const fs::path mempool_path{m_args.GetDataDirBase() / "mempool_skip_disabled_prioritisation.dat"};
    {
        AutoFile file{fsbridge::fopen(mempool_path, "wb")};
        BOOST_REQUIRE(!file.IsNull());
        file << uint64_t{1}; // Legacy v1 mempool dump version, without an obfuscation key.
        file.SetObfuscation({});
        file << uint64_t{0}; // No transactions to load.
        file << std::map<Txid, CAmount>{{missing_txid, fee_delta}};
        file << std::set<Txid>{missing_txid};
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }

    BOOST_REQUIRE(node::LoadMempool(pool, mempool_path, m_node.chainman->ActiveChainstate(),
                                    {
                                        .apply_fee_delta_priority = false,
                                        .apply_unbroadcast_set = false,
                                    }));
    BOOST_CHECK(pool.GetPrioritisedTransactions().empty());
    BOOST_CHECK(pool.GetUnbroadcastTxs().empty());

    fs::remove(mempool_path);
    fs::remove(mempool_path + ".new");
}

BOOST_AUTO_TEST_CASE(MempoolLoadDisabledMetadataPreservesMissingPrioritisation)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    BOOST_REQUIRE_EQUAL(pool.size(), 0U);
    BOOST_REQUIRE(pool.GetPrioritisedTransactions().empty());
    BOOST_REQUIRE(pool.GetUnbroadcastTxs().empty());

    const Txid preserved_txid{Txid::FromUint256(uint256{4})};
    const Txid imported_txid{Txid::FromUint256(uint256{5})};
    const Txid unbroadcast_txid{Txid::FromUint256(uint256{6})};
    constexpr CAmount preserved_delta{12345};
    constexpr CAmount imported_delta{-6789};
    pool.PrioritiseTransaction(preserved_txid, preserved_delta);

    const fs::path mempool_path{m_args.GetDataDirBase() / "mempool_disabled_metadata_preserves_missing_prioritisation.dat"};
    {
        AutoFile file{fsbridge::fopen(mempool_path, "wb")};
        BOOST_REQUIRE(!file.IsNull());
        file << uint64_t{1}; // Legacy v1 mempool dump version, without an obfuscation key.
        file.SetObfuscation({});
        file << uint64_t{0}; // No transactions to load.
        file << std::map<Txid, CAmount>{{imported_txid, imported_delta}};
        file << std::set<Txid>{unbroadcast_txid};
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }

    BOOST_REQUIRE(node::LoadMempool(pool, mempool_path, m_node.chainman->ActiveChainstate(),
                                    {
                                        .apply_fee_delta_priority = false,
                                        .apply_unbroadcast_set = false,
                                    }));
    BOOST_CHECK(pool.GetUnbroadcastTxs().empty());

    const auto deltas{pool.GetPrioritisedTransactions()};
    BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
    BOOST_CHECK(deltas.front().txid == preserved_txid);
    BOOST_CHECK(!deltas.front().in_mempool);
    BOOST_CHECK_EQUAL(deltas.front().delta, preserved_delta);
    BOOST_CHECK(!deltas.front().modified_fee.has_value());

    fs::remove(mempool_path);
    fs::remove(mempool_path + ".new");
}

BOOST_AUTO_TEST_CASE(MempoolLoadDisabledMetadataPreservesExistingEntry)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    BOOST_REQUIRE_EQUAL(pool.size(), 0U);
    BOOST_REQUIRE(pool.GetPrioritisedTransactions().empty());
    BOOST_REQUIRE(pool.GetUnbroadcastTxs().empty());

    CMutableTransaction tx_mut;
    tx_mut.vin.resize(1);
    tx_mut.vin[0].prevout = COutPoint{Txid::FromUint256(uint256{4}), 0};
    tx_mut.vin[0].scriptSig = CScript() << OP_4;
    tx_mut.vout.resize(1);
    tx_mut.vout[0].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx_mut.vout[0].nValue = 10 * COIN;
    const CTransactionRef tx{MakeTransactionRef(tx_mut)};
    const Txid txid{tx->GetHash()};

    constexpr CAmount base_fee{10'000};
    constexpr CAmount fee_delta{1'234};
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, pool.cs);
        TryAddToMempool(pool, entry.Fee(base_fee).FromTx(tx));
    }
    pool.PrioritiseTransaction(txid, fee_delta);
    pool.AddUnbroadcastTx(txid);

    const fs::path mempool_path{m_args.GetDataDirBase() / "mempool_disabled_metadata_preserves_existing.dat"};
    fs::remove(mempool_path);
    fs::remove(mempool_path + ".new");
    BOOST_REQUIRE(node::DumpMempool(pool, mempool_path));

    pool.RemoveUnbroadcastTx(txid);
    BOOST_REQUIRE(pool.GetUnbroadcastTxs().empty());

    BOOST_REQUIRE(node::LoadMempool(pool, mempool_path, m_node.chainman->ActiveChainstate(),
                                    {
                                        .apply_fee_delta_priority = false,
                                        .apply_unbroadcast_set = false,
                                    }));
    BOOST_CHECK(pool.GetUnbroadcastTxs().empty());

    const auto deltas{pool.GetPrioritisedTransactions()};
    BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
    BOOST_CHECK(deltas.front().txid == txid);
    BOOST_CHECK(deltas.front().in_mempool);
    BOOST_CHECK_EQUAL(deltas.front().delta, fee_delta);
    BOOST_REQUIRE(deltas.front().modified_fee.has_value());
    BOOST_CHECK_EQUAL(*deltas.front().modified_fee, base_fee + fee_delta);

    fs::remove(mempool_path);
    fs::remove(mempool_path + ".new");
}

BOOST_AUTO_TEST_CASE(MempoolSizeLimitTest)
{
    auto& pool = static_cast<MemPoolTest&>(*Assert(m_node.mempool));
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vin.resize(1);
    tx1.vin[0].scriptSig = CScript() << OP_1;
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;
    TryAddToMempool(pool, entry.Fee(1000LL).FromTx(tx1));

    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_2;
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_2 << OP_EQUAL;
    tx2.vout[0].nValue = 10 * COIN;
    TryAddToMempool(pool, entry.Fee(500LL).FromTx(tx2));

    pool.TrimToSize(pool.DynamicMemoryUsage()); // should do nothing
    BOOST_CHECK(pool.exists(tx1.GetHash()));
    BOOST_CHECK(pool.exists(tx2.GetHash()));

    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4); // should remove the lower-feerate transaction
    BOOST_CHECK(pool.exists(tx1.GetHash()));
    BOOST_CHECK(!pool.exists(tx2.GetHash()));

    TryAddToMempool(pool, entry.FromTx(tx2));
    CMutableTransaction tx3 = CMutableTransaction();
    tx3.vin.resize(1);
    tx3.vin[0].prevout = COutPoint(tx2.GetHash(), 0);
    tx3.vin[0].scriptSig = CScript() << OP_2;
    tx3.vout.resize(1);
    tx3.vout[0].scriptPubKey = CScript() << OP_3 << OP_EQUAL;
    tx3.vout[0].nValue = 10 * COIN;
    TryAddToMempool(pool, entry.Fee(2000LL).FromTx(tx3));

    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4); // tx3 should pay for tx2 (CPFP)
    BOOST_CHECK(!pool.exists(tx1.GetHash()));
    BOOST_CHECK(pool.exists(tx2.GetHash()));
    BOOST_CHECK(pool.exists(tx3.GetHash()));

    pool.TrimToSize(GetVirtualTransactionSize(CTransaction(tx1))); // mempool is limited to tx1's size in memory usage, so nothing fits
    BOOST_CHECK(!pool.exists(tx1.GetHash()));
    BOOST_CHECK(!pool.exists(tx2.GetHash()));
    BOOST_CHECK(!pool.exists(tx3.GetHash()));

    CFeeRate maxFeeRateRemoved(2500, GetVirtualTransactionSize(CTransaction(tx3)) + GetVirtualTransactionSize(CTransaction(tx2)));
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE);

    CMutableTransaction tx4 = CMutableTransaction();
    tx4.vin.resize(2);
    tx4.vin[0].prevout.SetNull();
    tx4.vin[0].scriptSig = CScript() << OP_4;
    tx4.vin[1].prevout.SetNull();
    tx4.vin[1].scriptSig = CScript() << OP_4;
    tx4.vout.resize(2);
    tx4.vout[0].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[0].nValue = 10 * COIN;
    tx4.vout[1].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx5 = CMutableTransaction();
    tx5.vin.resize(2);
    tx5.vin[0].prevout = COutPoint(tx4.GetHash(), 0);
    tx5.vin[0].scriptSig = CScript() << OP_4;
    tx5.vin[1].prevout.SetNull();
    tx5.vin[1].scriptSig = CScript() << OP_5;
    tx5.vout.resize(2);
    tx5.vout[0].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[0].nValue = 10 * COIN;
    tx5.vout[1].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx6 = CMutableTransaction();
    tx6.vin.resize(2);
    tx6.vin[0].prevout = COutPoint(tx4.GetHash(), 1);
    tx6.vin[0].scriptSig = CScript() << OP_4;
    tx6.vin[1].prevout.SetNull();
    tx6.vin[1].scriptSig = CScript() << OP_6;
    tx6.vout.resize(2);
    tx6.vout[0].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[0].nValue = 10 * COIN;
    tx6.vout[1].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx7 = CMutableTransaction();
    tx7.vin.resize(2);
    tx7.vin[0].prevout = COutPoint(tx5.GetHash(), 0);
    tx7.vin[0].scriptSig = CScript() << OP_5;
    tx7.vin[1].prevout = COutPoint(tx6.GetHash(), 0);
    tx7.vin[1].scriptSig = CScript() << OP_6;
    tx7.vout.resize(2);
    tx7.vout[0].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[0].nValue = 10 * COIN;
    tx7.vout[1].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[1].nValue = 10 * COIN;

    TryAddToMempool(pool, entry.Fee(700LL).FromTx(tx4));
    auto usage_with_tx4_only = pool.DynamicMemoryUsage();
    TryAddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    TryAddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    TryAddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    // From the topology above, tx7 must be sorted last, so it should
    // definitely evicted first if we must trim. tx4 should definitely remain
    // in the mempool since it has a higher feerate than its descendants and
    // should be in its own chunk.
    pool.TrimToSize(pool.DynamicMemoryUsage() - 1);
    BOOST_CHECK(pool.exists(tx4.GetHash()));
    BOOST_CHECK(!pool.exists(tx7.GetHash()));

    // Tx5 and Tx6 may be removed as well because they're in the same chunk as
    // tx7, but this behavior need not be guaranteed.

    if (!pool.exists(tx5.GetHash()))
        TryAddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    if (!pool.exists(tx6.GetHash()))
        TryAddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    TryAddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    // If we trim sufficiently, everything but tx4 should be removed.
    pool.TrimToSize(usage_with_tx4_only + 1);
    BOOST_CHECK(pool.exists(tx4.GetHash()));
    BOOST_CHECK(!pool.exists(tx5.GetHash()));
    BOOST_CHECK(!pool.exists(tx6.GetHash()));
    BOOST_CHECK(!pool.exists(tx7.GetHash()));

    TryAddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    TryAddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    TryAddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    std::vector<CTransactionRef> vtx;
    FakeNodeClock clock{42s};
    constexpr std::chrono::seconds HALFLIFE{CTxMemPool::ROLLING_FEE_HALFLIFE};
    clock += HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE);
    // ... we should keep the same min fee until we get a block
    pool.removeForBlock(vtx, 1);
    clock += HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE)/2.0));
    // ... then feerate should drop 1/2 each halflife

    clock += HALFLIFE / 2;
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 5 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE)/4.0));
    // ... with a 1/2 halflife when mempool is < 1/2 its target size

    clock += HALFLIFE / 4;
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 9 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE)/8.0));
    // ... with a 1/4 halflife when mempool is < 1/4 its target size

    clock += 5 * HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), DEFAULT_INCREMENTAL_RELAY_FEE);
    // ... but feerate should never drop below DEFAULT_INCREMENTAL_RELAY_FEE

    clock += HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), 0);
    // ... unless it has gone all the way to 0 (after getting past DEFAULT_INCREMENTAL_RELAY_FEE/2)
}

inline CTransactionRef make_tx(std::vector<CAmount>&& output_values, std::vector<CTransactionRef>&& inputs=std::vector<CTransactionRef>(), std::vector<uint32_t>&& input_indices=std::vector<uint32_t>())
{
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(inputs.size());
    tx.vout.resize(output_values.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        tx.vin[i].prevout.hash = inputs[i]->GetHash();
        tx.vin[i].prevout.n = input_indices.size() > i ? input_indices[i] : 0;
    }
    for (size_t i = 0; i < output_values.size(); ++i) {
        tx.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout[i].nValue = output_values[i];
    }
    return MakeTransactionRef(tx);
}

static std::set<Txid> EntryRefTxids(const std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef>& entries)
{
    std::set<Txid> txids;
    for (const auto& entry : entries) {
        BOOST_CHECK(txids.insert(entry.get().GetTx().GetHash()).second);
    }
    return txids;
}

BOOST_AUTO_TEST_CASE(MempoolHasDescendantsMatchesChildren)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    TestMemPoolEntryHelper entry;

    CTransactionRef parent{make_tx(/*output_values=*/{10 * COIN})};
    CTransactionRef child{make_tx(/*output_values=*/{9 * COIN}, /*inputs=*/{parent})};
    CTransactionRef unrelated{make_tx(/*output_values=*/{8 * COIN})};

    {
        LOCK2(::cs_main, pool.cs);
        TryAddToMempool(pool, entry.Fee(10'000).FromTx(parent));
        TryAddToMempool(pool, entry.Fee(10'000).FromTx(unrelated));
    }

    BOOST_CHECK(!pool.HasDescendants(parent->GetHash()));
    BOOST_CHECK(!pool.HasDescendants(unrelated->GetHash()));
    BOOST_CHECK(!pool.HasDescendants(Txid::FromUint256(uint256{1})));

    {
        LOCK2(::cs_main, pool.cs);
        TryAddToMempool(pool, entry.Fee(10'000).FromTx(child));
    }

    BOOST_CHECK(pool.HasDescendants(parent->GetHash()));
    BOOST_CHECK(!pool.HasDescendants(child->GetHash()));
    BOOST_CHECK(!pool.HasDescendants(unrelated->GetHash()));

    {
        LOCK(pool.cs);
        pool.removeRecursive(*child, REMOVAL_REASON_DUMMY);
    }

    BOOST_CHECK(!pool.HasDescendants(parent->GetHash()));
}

BOOST_AUTO_TEST_CASE(MempoolDirectParentChildEdges)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    TestMemPoolEntryHelper entry;

    CTransactionRef parent_a{make_tx(/*output_values=*/{10 * COIN, 10 * COIN, 10 * COIN})};
    CTransactionRef parent_b{make_tx(/*output_values=*/{10 * COIN})};
    CTransactionRef same_parent_child{
        make_tx(/*output_values=*/{18 * COIN}, /*inputs=*/{parent_a, parent_a}, /*input_indices=*/{0, 1})};
    CTransactionRef multi_parent_child{
        make_tx(/*output_values=*/{17 * COIN}, /*inputs=*/{parent_a, parent_b}, /*input_indices=*/{2, 0})};

    {
        LOCK2(::cs_main, pool.cs);
        TryAddToMempool(pool, entry.Fee(10'000).FromTx(parent_a));
        TryAddToMempool(pool, entry.Fee(10'000).FromTx(parent_b));
        TryAddToMempool(pool, entry.Fee(10'000).FromTx(same_parent_child));
        TryAddToMempool(pool, entry.Fee(10'000).FromTx(multi_parent_child));
    }

    LOCK(pool.cs);
    const auto parent_a_entry{pool.GetIter(parent_a->GetHash())};
    const auto parent_b_entry{pool.GetIter(parent_b->GetHash())};
    const auto same_parent_child_entry{pool.GetIter(same_parent_child->GetHash())};
    const auto multi_parent_child_entry{pool.GetIter(multi_parent_child->GetHash())};
    BOOST_REQUIRE(parent_a_entry);
    BOOST_REQUIRE(parent_b_entry);
    BOOST_REQUIRE(same_parent_child_entry);
    BOOST_REQUIRE(multi_parent_child_entry);

    const auto parent_a_children{EntryRefTxids(pool.GetChildren(**parent_a_entry))};
    BOOST_CHECK_EQUAL(parent_a_children.size(), 2U);
    BOOST_CHECK(parent_a_children.contains(same_parent_child->GetHash()));
    BOOST_CHECK(parent_a_children.contains(multi_parent_child->GetHash()));

    const auto parent_b_children{EntryRefTxids(pool.GetChildren(**parent_b_entry))};
    BOOST_CHECK_EQUAL(parent_b_children.size(), 1U);
    BOOST_CHECK(parent_b_children.contains(multi_parent_child->GetHash()));

    const auto same_child_parents{EntryRefTxids(pool.GetParents(**same_parent_child_entry))};
    BOOST_CHECK_EQUAL(same_child_parents.size(), 1U);
    BOOST_CHECK(same_child_parents.contains(parent_a->GetHash()));

    const auto multi_child_parents{EntryRefTxids(pool.GetParents(**multi_parent_child_entry))};
    BOOST_CHECK_EQUAL(multi_child_parents.size(), 2U);
    BOOST_CHECK(multi_child_parents.contains(parent_a->GetHash()));
    BOOST_CHECK(multi_child_parents.contains(parent_b->GetHash()));

    BOOST_CHECK_EQUAL(pool.GetConflictTx(COutPoint{parent_a->GetHash(), 0}), same_parent_child.get());
    BOOST_CHECK_EQUAL(pool.GetConflictTx(COutPoint{parent_a->GetHash(), 1}), same_parent_child.get());
    BOOST_CHECK_EQUAL(pool.GetConflictTx(COutPoint{parent_a->GetHash(), 2}), multi_parent_child.get());
    BOOST_CHECK_EQUAL(pool.GetConflictTx(COutPoint{parent_b->GetHash(), 0}), multi_parent_child.get());
    BOOST_CHECK_EQUAL(pool.GetConflictTx(COutPoint{parent_b->GetHash(), 1}), nullptr);

    pool.removeRecursive(*same_parent_child, REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(pool.GetConflictTx(COutPoint{parent_a->GetHash(), 0}), nullptr);
    BOOST_CHECK_EQUAL(pool.GetConflictTx(COutPoint{parent_a->GetHash(), 1}), nullptr);
    BOOST_CHECK_EQUAL(pool.GetConflictTx(COutPoint{parent_a->GetHash(), 2}), multi_parent_child.get());
    const auto parent_a_children_after_remove{EntryRefTxids(pool.GetChildren(**parent_a_entry))};
    BOOST_CHECK_EQUAL(parent_a_children_after_remove.size(), 1U);
    BOOST_CHECK(parent_a_children_after_remove.contains(multi_parent_child->GetHash()));
}

BOOST_AUTO_TEST_CASE(MempoolRandomizedIndexTest)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;
    const auto make_witness_tx = [](uint32_t prevout_n, CAmount value) {
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout = COutPoint{Txid::FromUint256(uint256{static_cast<uint8_t>(prevout_n + 1)}), prevout_n};
        tx.vin[0].scriptWitness.stack = {std::vector<unsigned char>{static_cast<unsigned char>(prevout_n + 1)}};
        tx.vout.emplace_back(value, CScript() << OP_11 << OP_EQUAL);
        return MakeTransactionRef(tx);
    };

    for (uint32_t i{0}; i < 3; ++i) {
        TryAddToMempool(pool, entry.Fee(10'000).FromTx(make_witness_tx(i, (10 + i) * COIN)));
        CheckMempoolRandomizedIndex(pool);
    }

    BOOST_REQUIRE_GE(pool.txns_randomized.size(), 2U);
    const CTransactionRef tx{pool.txns_randomized.front().second->GetSharedTx()};
    pool.removeRecursive(*tx, REMOVAL_REASON_DUMMY);
    CheckMempoolRandomizedIndex(pool);
}


BOOST_AUTO_TEST_CASE(MempoolAncestryTests)
{
    size_t ancestors, clustersize;

    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    /* Base transaction */
    //
    // [tx1]
    //
    CTransactionRef tx1 = make_tx(/*output_values=*/{10 * COIN});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tx1));

    // Ancestors / clustersize should be 1 / 1 (itself / itself)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 1ULL);

    /* Child transaction */
    //
    // [tx1].0 <- [tx2]
    //
    CTransactionRef tx2 = make_tx(/*output_values=*/{495 * CENT, 5 * COIN}, /*inputs=*/{tx1});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tx2));

    // Ancestors / clustersize should be:
    // transaction  ancestors   clustersize
    // ============ =========== ===========
    // tx1          1 (tx1)     2 (tx1,2)
    // tx2          2 (tx1,2)   2 (tx1,2)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 2ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 2ULL);

    /* Grand-child 1 */
    //
    // [tx1].0 <- [tx2].0 <- [tx3]
    //
    CTransactionRef tx3 = make_tx(/*output_values=*/{290 * CENT, 200 * CENT}, /*inputs=*/{tx2});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tx3));

    // Ancestors / clustersize should be:
    // transaction  ancestors   clustersize
    // ============ =========== ===========
    // tx1          1 (tx1)     3 (tx1,2,3)
    // tx2          2 (tx1,2)   3 (tx1,2,3)
    // tx3          3 (tx1,2,3) 3 (tx1,2,3)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 3ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 3ULL);
    pool.GetTransactionAncestry(tx3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 3ULL);

    /* Grand-child 2 */
    //
    // [tx1].0 <- [tx2].0 <- [tx3]
    //              |
    //              \---1 <- [tx4]
    //
    CTransactionRef tx4 = make_tx(/*output_values=*/{290 * CENT, 250 * CENT}, /*inputs=*/{tx2}, /*input_indices=*/{1});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tx4));

    // Ancestors / clustersize should be:
    // transaction  ancestors   clustersize
    // ============ =========== ===========
    // tx1          1 (tx1)     4 (tx1,2,3,4)
    // tx2          2 (tx1,2)   4 (tx1,2,3,4)
    // tx3          3 (tx1,2,3) 4 (tx1,2,3,4)
    // tx4          3 (tx1,2,4) 4 (tx1,2,3,4)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    pool.GetTransactionAncestry(tx3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    pool.GetTransactionAncestry(tx4->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    size_t ancestorsize;
    CAmount ancestorfees;
    pool.GetTransactionAncestry(tx4->GetHash(), ancestors, clustersize, &ancestorsize, &ancestorfees);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    const auto tx4_iter{pool.GetIter(tx4->GetHash())};
    BOOST_REQUIRE(tx4_iter.has_value());
    const auto [expected_ancestors, expected_size, expected_fees]{pool.CalculateAncestorData(**tx4_iter)};
    BOOST_CHECK_EQUAL(ancestors, expected_ancestors);
    BOOST_CHECK_EQUAL(ancestorsize, expected_size);
    BOOST_CHECK_EQUAL(ancestorfees, expected_fees);

    ancestors = clustersize = ancestorsize = std::numeric_limits<size_t>::max();
    ancestorfees = std::numeric_limits<CAmount>::max();
    pool.GetTransactionAncestry(Txid::FromUint256(uint256::ONE), ancestors, clustersize, &ancestorsize, &ancestorfees);
    BOOST_CHECK_EQUAL(ancestors, 0U);
    BOOST_CHECK_EQUAL(clustersize, 0U);
    BOOST_CHECK_EQUAL(ancestorsize, 0U);
    BOOST_CHECK_EQUAL(ancestorfees, 0);

    /* Make an alternate branch that is longer and connect it to tx3 */
    //
    // [ty1].0 <- [ty2].0 <- [ty3].0 <- [ty4].0 <- [ty5].0
    //                                              |
    // [tx1].0 <- [tx2].0 <- [tx3].0 <- [ty6] --->--/
    //              |
    //              \---1 <- [tx4]
    //
    CTransactionRef ty1, ty2, ty3, ty4, ty5;
    CTransactionRef* ty[5] = {&ty1, &ty2, &ty3, &ty4, &ty5};
    CAmount v = 5 * COIN;
    for (uint64_t i = 0; i < 5; i++) {
        CTransactionRef& tyi = *ty[i];
        tyi = make_tx(/*output_values=*/{v}, /*inputs=*/i > 0 ? std::vector<CTransactionRef>{*ty[i - 1]} : std::vector<CTransactionRef>{});
        v -= 50 * CENT;
        TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tyi));
        pool.GetTransactionAncestry(tyi->GetHash(), ancestors, clustersize);
        BOOST_CHECK_EQUAL(ancestors, i+1);
        BOOST_CHECK_EQUAL(clustersize, i+1);
    }
    CTransactionRef ty6 = make_tx(/*output_values=*/{5 * COIN}, /*inputs=*/{tx3, ty5});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(ty6));

    // Ancestors / clustersize should be:
    // transaction  ancestors           clustersize
    // ============ =================== ===========
    // tx1          1 (tx1)             10 (tx1-5, ty1-5)
    // tx2          2 (tx1,2)           10
    // tx3          3 (tx1,2,3)         10
    // tx4          3 (tx1,2,4)         10
    // ty1          1 (ty1)             10
    // ty2          2 (ty1,2)           10
    // ty3          3 (ty1,2,3)         10
    // ty4          4 (y1234)           10
    // ty5          5 (y12345)          10
    // ty6          9 (tx123, ty123456) 10
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(tx3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(tx4->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty4->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 4ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty5->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 5ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty6->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 9ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
}

BOOST_AUTO_TEST_CASE(UpdateTransactionsFromBlockRestoresChildAncestry)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    CTransactionRef parent{make_tx(/*output_values=*/{10 * COIN})};
    CTransactionRef child{make_tx(/*output_values=*/{9 * COIN}, /*inputs=*/{parent})};
    TryAddToMempool(pool, entry.Fee(10'000).FromTx(parent));
    TryAddToMempool(pool, entry.Fee(10'000).FromTx(child));

    pool.removeForBlock({parent}, /*nBlockHeight=*/1);
    BOOST_CHECK(!pool.exists(parent->GetHash()));
    BOOST_REQUIRE(pool.exists(child->GetHash()));

    TryAddToMempool(pool, entry.Fee(10'000).FromTx(parent));
    const auto parent_it{pool.GetIter(parent->GetHash())};
    const auto child_it{pool.GetIter(child->GetHash())};
    BOOST_REQUIRE(parent_it.has_value());
    BOOST_REQUIRE(child_it.has_value());

    const auto pre_update_ancestors{pool.CalculateMemPoolAncestors(**child_it)};
    BOOST_CHECK(!pre_update_ancestors.contains(*parent_it));

    pool.UpdateTransactionsFromBlock({parent->GetHash()});
    const auto post_update_ancestors{pool.CalculateMemPoolAncestors(**child_it)};
    BOOST_CHECK(post_update_ancestors.contains(*parent_it));

    size_t ancestors, clustersize;
    pool.GetTransactionAncestry(child->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2U);
    BOOST_CHECK_EQUAL(clustersize, 2U);
}

BOOST_AUTO_TEST_CASE(MempoolAncestryTestsDiamond)
{
    size_t ancestors, descendants;

    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    /* Ancestors represented more than once ("diamond") */
    //
    // [ta].0 <- [tb].0 -----<------- [td].0
    //            |                    |
    //            \---1 <- [tc].0 --<--/
    //
    CTransactionRef ta, tb, tc, td;
    ta = make_tx(/*output_values=*/{10 * COIN});
    tb = make_tx(/*output_values=*/{5 * COIN, 3 * COIN}, /*inputs=*/ {ta});
    tc = make_tx(/*output_values=*/{2 * COIN}, /*inputs=*/{tb}, /*input_indices=*/{1});
    td = make_tx(/*output_values=*/{6 * COIN}, /*inputs=*/{tb, tc}, /*input_indices=*/{0, 0});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(ta));
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tb));
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tc));
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(td));

    // Ancestors / descendants should be:
    // transaction  ancestors           descendants
    // ============ =================== ===========
    // ta           1 (ta               4 (ta,tb,tc,td)
    // tb           2 (ta,tb)           4 (ta,tb,tc,td)
    // tc           3 (ta,tb,tc)        4 (ta,tb,tc,td)
    // td           4 (ta,tb,tc,td)     4 (ta,tb,tc,td)
    pool.GetTransactionAncestry(ta->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry(tb->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry(tc->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry(td->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 4ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);

    const auto check_cluster = [&](const CTransactionRef& query) EXCLUSIVE_LOCKS_REQUIRED(pool.cs) {
        const auto cluster{pool.GetCluster(query->GetHash())};
        BOOST_CHECK_EQUAL(cluster.size(), 4U);
        std::set<Txid> cluster_txids;
        for (const auto* entry : cluster) {
            BOOST_REQUIRE(entry);
            BOOST_CHECK(cluster_txids.insert(entry->GetTx().GetHash()).second);
        }
        for (const auto& tx : {ta, tb, tc, td}) {
            BOOST_CHECK(cluster_txids.contains(tx->GetHash()));
        }
    };
    check_cluster(ta);
    check_cluster(td);
    BOOST_CHECK(pool.GetCluster(Txid::FromUint256(uint256::ZERO)).empty());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_CASE(MempoolCheckSaturatingFeeDiagram, TestChain100Setup)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    const CScript output_script{GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()))};
    mineBlocks(3);

    constexpr CAmount fee{10'000};
    const auto submit = [&](const CTransactionRef& tx) {
        LOCK(cs_main);
        const MempoolAcceptResult result{m_node.chainman->ProcessTransaction(tx)};
        BOOST_REQUIRE_MESSAGE(result.m_result_type == MempoolAcceptResult::ResultType::VALID, result.m_state.ToString());
    };

    auto independent{MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.at(0), /*input_vout=*/0, /*input_height=*/0, coinbaseKey,
        output_script, m_coinbase_txns.at(0)->vout.at(0).nValue - fee, /*submit=*/false))};
    submit(independent);

    std::vector<CTransactionRef> parents;
    std::vector<COutPoint> child_inputs;
    CAmount child_input_value{0};
    for (size_t i{0}; i < 3; ++i) {
        const auto& coinbase{m_coinbase_txns.at(i + 1)};
        auto parent{MakeTransactionRef(CreateValidMempoolTransaction(
            coinbase, /*input_vout=*/0, /*input_height=*/0, coinbaseKey,
            output_script, coinbase->vout.at(0).nValue - fee, /*submit=*/false))};
        submit(parent);
        parents.push_back(parent);
        child_inputs.emplace_back(parent->GetHash(), 0);
        child_input_value += parent->vout.at(0).nValue;
    }

    auto child{MakeTransactionRef(CreateValidMempoolTransaction(
        /*input_transactions=*/parents,
        /*inputs=*/child_inputs,
        /*input_height=*/0,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut{child_input_value - fee, output_script}},
        /*submit=*/false))};
    submit(child);

    for (const auto& parent : parents) {
        pool.PrioritiseTransaction(parent->GetHash(), std::numeric_limits<CAmount>::min());
    }
    pool.PrioritiseTransaction(child->GetHash(), std::numeric_limits<CAmount>::max());

    {
        LOCK(pool.cs);
        const auto diagram{pool.GetFeerateDiagram()};
        BOOST_REQUIRE(!diagram.empty());
        BOOST_CHECK_EQUAL(diagram.back().fee, std::numeric_limits<CAmount>::min() + fee);
    }
    WITH_LOCK(::cs_main, pool.check(m_node.chainman->ActiveChainstate().CoinsTip(), m_node.chainman->ActiveChain().Height() + 1));
}

BOOST_FIXTURE_TEST_CASE(MempoolCheckMixedSignSaturatingFeeDiagram, TestChain100Setup)
{
    CTxMemPool& pool{*Assert(m_node.mempool)};
    const CScript output_script{GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()))};
    mineBlocks(3);

    constexpr CAmount fee{10'000};
    const auto submit = [&](const CTransactionRef& tx) {
        LOCK(cs_main);
        const MempoolAcceptResult result{m_node.chainman->ProcessTransaction(tx)};
        BOOST_REQUIRE_MESSAGE(result.m_result_type == MempoolAcceptResult::ResultType::VALID, result.m_state.ToString());
    };

    std::vector<CTransactionRef> positive_txs;
    for (size_t i{0}; i < 3; ++i) {
        const auto& coinbase{m_coinbase_txns.at(i)};
        auto tx{MakeTransactionRef(CreateValidMempoolTransaction(
            coinbase, /*input_vout=*/0, /*input_height=*/0, coinbaseKey,
            output_script, coinbase->vout.at(0).nValue - fee, /*submit=*/false))};
        submit(tx);
        positive_txs.push_back(tx);
    }

    const auto& parent_coinbase{m_coinbase_txns.at(3)};
    auto parent{MakeTransactionRef(CreateValidMempoolTransaction(
        parent_coinbase, /*input_vout=*/0, /*input_height=*/0, coinbaseKey,
        output_script, parent_coinbase->vout.at(0).nValue - fee, /*submit=*/false))};
    submit(parent);

    auto child{MakeTransactionRef(CreateValidMempoolTransaction(
        parent, /*input_vout=*/0, /*input_height=*/0, coinbaseKey,
        output_script, parent->vout.at(0).nValue - fee, /*submit=*/false))};
    submit(child);

    constexpr CAmount positive_delta{5'000'000'000};
    constexpr CAmount negative_child_delta{-3'000'000'000};
    for (const auto& tx : positive_txs) {
        pool.PrioritiseTransaction(tx->GetHash(), positive_delta);
    }
    pool.PrioritiseTransaction(parent->GetHash(), std::numeric_limits<CAmount>::min());
    pool.PrioritiseTransaction(child->GetHash(), negative_child_delta);

    const CAmount positive_modified{SaturatingAdd(fee, positive_delta)};
    const CAmount parent_modified{SaturatingAdd(fee, std::numeric_limits<CAmount>::min())};
    const CAmount child_modified{SaturatingAdd(fee, negative_child_delta)};
    CAmount transaction_order_total{0};
    for (size_t i{0}; i < positive_txs.size(); ++i) {
        transaction_order_total = *Assert(CheckedAdd(transaction_order_total, positive_modified));
    }
    transaction_order_total = *Assert(CheckedAdd(transaction_order_total, parent_modified));
    transaction_order_total = *Assert(CheckedAdd(transaction_order_total, child_modified));
    {
        LOCK(pool.cs);
        const auto diagram{pool.GetFeerateDiagram()};
        BOOST_REQUIRE(!diagram.empty());
        BOOST_CHECK_NE(diagram.back().fee, transaction_order_total);
    }
    WITH_LOCK(::cs_main, pool.check(m_node.chainman->ActiveChainstate().CoinsTip(), m_node.chainman->ActiveChain().Height() + 1));
}
