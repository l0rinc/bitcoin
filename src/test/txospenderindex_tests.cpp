// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dbwrapper.h>
#include <index/txospenderindex.h>
#include <script/interpreter.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

BOOST_AUTO_TEST_SUITE(txospenderindex_tests)

BOOST_FIXTURE_TEST_CASE(txospenderindex_initial_sync, TestChain100Setup)
{
    // Setup phase:
    // Mine blocks for coinbase maturity, so we can spend some coinbase outputs in the test.
    const CScript& coinbase_script = m_coinbase_txns[0]->vout[0].scriptPubKey;
    for (int i = 0; i < 10; i++) CreateAndProcessBlock({}, coinbase_script);

    // Spend 10 outputs
    std::vector<COutPoint> spent(10);
    std::vector<CMutableTransaction> spender(spent.size());
    for (size_t i = 0; i < spent.size(); i++) {
        // Outpoint
        auto coinbase_tx = m_coinbase_txns[i];
        spent[i] = COutPoint(coinbase_tx->GetHash(), 0);

        // Spending tx
        spender[i].version = 1;
        spender[i].vin.resize(1);
        spender[i].vin[0].prevout.hash = spent[i].hash;
        spender[i].vin[0].prevout.n = spent[i].n;
        spender[i].vout.resize(1);
        spender[i].vout[0].nValue = coinbase_tx->GetValueOut();
        spender[i].vout[0].scriptPubKey = coinbase_script;

        // Sign
        std::vector<unsigned char> vchSig;
        const uint256 hash = SignatureHash(coinbase_script, spender[i], 0, SIGHASH_ALL, 0, SigVersion::BASE);
        BOOST_REQUIRE(coinbaseKey.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        spender[i].vin[0].scriptSig << vchSig;
    }

    // Generate and ensure block has been fully processed
    const uint256 tip_hash = CreateAndProcessBlock(spender, coinbase_script).GetHash();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return m_node.chainman->ActiveTip()->GetBlockHash()), tip_hash);

    // Now we concluded the setup phase, run index
    TxoSpenderIndex txospenderindex(interfaces::MakeChain(m_node), 1 << 20, true);
    BOOST_REQUIRE(txospenderindex.Init());
    BOOST_CHECK(!txospenderindex.BlockUntilSyncedToCurrentChain()); // false when not synced
    BOOST_CHECK_NE(txospenderindex.GetSummary().best_block_hash, tip_hash);

    // Transaction should not be found in the index before it is synced.
    for (const auto& outpoint : spent) {
        BOOST_CHECK(!txospenderindex.FindSpender(outpoint).value());
    }

    txospenderindex.Sync();
    BOOST_CHECK_EQUAL(txospenderindex.GetSummary().best_block_hash, tip_hash);

    for (size_t i = 0; i < spent.size(); i++) {
        const auto tx_spender{txospenderindex.FindSpender(spent[i])};
        BOOST_REQUIRE(tx_spender.has_value());
        BOOST_REQUIRE(tx_spender->has_value());
        BOOST_CHECK_EQUAL((*tx_spender)->tx->GetHash(), spender[i].GetHash());
        BOOST_CHECK_EQUAL((*tx_spender)->block_hash, tip_hash);
    }

    // Shutdown sequence (c.f. Shutdown() in init.cpp)
    txospenderindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txospenderindex_reorg_recovery, TestChain100Setup)
{
    auto& chainman{*Assert(m_node.chainman)};
    auto& chainstate{chainman.ActiveChainstate()};
    const CScript& coinbase_script{m_coinbase_txns.front()->vout.front().scriptPubKey};
    const CTransactionRef& coinbase_tx{m_coinbase_txns.front()};
    const COutPoint spent{coinbase_tx->GetHash(), 0};

    auto current_tip{[&] { return Assert(WITH_LOCK(::cs_main, return chainstate.m_chain.Tip())); }};
    auto make_index{[&](bool wipe) { return TxoSpenderIndex{interfaces::MakeChain(m_node), 1_MiB, false, wipe}; }};
    auto sync_callbacks{[&] { m_node.validation_signals->SyncWithValidationInterfaceQueue(); }};
    auto find_spender{[&](TxoSpenderIndex& index) {
        auto result{index.FindSpender(spent)};
        BOOST_REQUIRE(result.has_value());
        BOOST_REQUIRE(result->has_value());
        return (*result)->tx->GetHash();
    }};
    auto invalidate{[&](CBlockIndex* block) {
        BlockValidationState state{};
        BOOST_REQUIRE(chainstate.InvalidateBlock(state, block) && state.IsValid());
    }};
    auto make_spender{[&](int32_t version) {
        CMutableTransaction tx;
        tx.version = version;
        tx.vin.resize(1);
        tx.vin.front().prevout = spent;
        tx.vout.resize(1);
        tx.vout.front().nValue = coinbase_tx->GetValueOut();
        tx.vout.front().scriptPubKey = coinbase_script;

        std::vector<unsigned char> signature;
        BOOST_REQUIRE(coinbaseKey.Sign(SignatureHash(coinbase_script, tx, 0, SIGHASH_ALL, 0, SigVersion::BASE), signature));
        signature.push_back(static_cast<unsigned char>(SIGHASH_ALL));
        tx.vin.front().scriptSig << signature;
        return tx;
    }};

    auto spender_a{make_spender(/*version=*/1)};
    auto spender_b{make_spender(/*version=*/2)};
    auto* fork_parent{current_tip()};

    // Store B before A so its key sorts first; keep B invalid until A is durable.
    auto block_b{std::make_shared<const CBlock>(CreateBlock(fork_parent, {spender_b}, coinbase_script))};
    BOOST_REQUIRE(chainman.ProcessNewBlock(block_b, /*force_processing=*/true, /*min_pow_checked=*/true, nullptr));
    auto* block_b_index{Assert(WITH_LOCK(::cs_main, return chainstate.m_blockman.LookupBlockIndex(block_b->GetHash())))};
    invalidate(block_b_index);

    CreateAndProcessBlock({spender_a}, coinbase_script);
    sync_callbacks();
    auto* old_tip{current_tip()};

    // Commit A, reorg the live index to B without flushing, then restore A.
    {
        auto index{make_index(/*wipe=*/true)};
        BOOST_REQUIRE(index.Init());
        index.Sync();
        chainstate.ForceFlushStateToDisk();
        sync_callbacks();
        BOOST_CHECK_EQUAL(find_spender(index), spender_a.GetHash());

        {
            LOCK(::cs_main);
            chainstate.ResetBlockFailureFlags(block_b_index);
            chainman.RecalculateBestHeader();
        }
        auto block_c{std::make_shared<const CBlock>(CreateBlock(block_b_index, {}, coinbase_script))};
        BOOST_REQUIRE(chainman.ProcessNewBlock(block_c, /*force_processing=*/true, /*min_pow_checked=*/true, nullptr));
        sync_callbacks();
        BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return chainstate.GetLastFlushedBlock()), old_tip);
        BOOST_CHECK_EQUAL(find_spender(index), spender_b.GetHash());
        index.Stop();

        invalidate(block_b_index);
        BlockValidationState state{};
        BOOST_REQUIRE(chainstate.ActivateBestChain(state, nullptr) && state.IsValid());
        sync_callbacks();
        BOOST_REQUIRE_EQUAL(current_tip(), old_tip);
    }

    auto block_b_pos{WITH_LOCK(::cs_main, return block_b_index->GetBlockPos())};
    auto erase_block_mapping{[&] {
        auto block_b_key{std::pair{uint8_t{'b'}, block_b_pos}}; // Matches DB_TXOSPENDER_BLOCK
        CDBWrapper db{{
            .path = gArgs.GetDataDirNet() / "indexes" / "txospenderindex" / "db",
            .cache_bytes = 1_MiB,
        }};
        BOOST_REQUIRE(db.Exists(block_b_key));
        db.Erase(block_b_key);
        BOOST_CHECK(!db.Exists(block_b_key));
    }};
    auto corrupt_spender_b{[&] {
        auto corrupt_pos{block_b_pos};
        corrupt_pos.nPos += static_cast<unsigned>(::GetSerializeSize(static_cast<const CBlockHeader&>(*block_b))) + GetSizeOfCompactSize(block_b->vtx.size()) +
                            static_cast<unsigned>(::GetSerializeSize(TX_WITH_WITNESS(*block_b->vtx.front()))) + sizeof(spender_b.version);
        AutoFile file{chainstate.m_blockman.OpenBlockFile(corrupt_pos, /*fReadOnly=*/false)};
        BOOST_REQUIRE(!file.IsNull());
        std::array<std::byte, 9> invalid_compact_size{std::byte{0xff}};
        file.write(invalid_compact_size);
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }};
    auto restart_index{[&] {
        auto index{make_index(/*wipe=*/false)};
        BOOST_REQUIRE(index.Init());
        auto spender_hash{find_spender(index)};
        index.Stop();
        return spender_hash;
    }};
    corrupt_spender_b();

    // The block mapping rejects inactive B before reading its corrupt transaction.
    BOOST_CHECK_EQUAL(restart_index(), spender_a.GetHash());

    // Without the mapping, model legacy data and continue from unreadable B to active A.
    erase_block_mapping();
    BOOST_CHECK_EQUAL(restart_index(), spender_a.GetHash());
}

BOOST_AUTO_TEST_SUITE_END()
