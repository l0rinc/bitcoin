// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txospenderindex.h>
#include <dbwrapper.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <atomic>
#include <future>
#include <thread>

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

BOOST_FIXTURE_TEST_CASE(txospenderindex_multibyte_tx_count_offsets, TestChain100Setup)
{
    constexpr size_t SPENDER_COUNT{253};
    const CScript& coinbase_script = m_coinbase_txns[0]->vout[0].scriptPubKey;
    const COutPoint parent_input{m_coinbase_txns[0]->GetHash(), 0};

    CMutableTransaction parent;
    parent.vin.resize(1);
    parent.vin[0].prevout = parent_input;
    parent.vout.resize(SPENDER_COUNT);
    for (auto& output : parent.vout) {
        output.nValue = 1'000'000;
        output.scriptPubKey = CScript() << OP_TRUE;
    }

    std::vector<unsigned char> signature;
    const uint256 signature_hash = SignatureHash(coinbase_script, parent, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_REQUIRE(coinbaseKey.Sign(signature_hash, signature));
    signature.push_back(static_cast<unsigned char>(SIGHASH_ALL));
    parent.vin[0].scriptSig << signature;

    const CBlock parent_block = CreateAndProcessBlock({parent}, coinbase_script);

    std::vector<CMutableTransaction> spenders;
    spenders.reserve(SPENDER_COUNT);
    for (uint32_t output_index{0}; output_index < SPENDER_COUNT; ++output_index) {
        CMutableTransaction spender;
        spender.vin.resize(1);
        spender.vin[0].prevout = COutPoint(parent.GetHash(), output_index);
        spender.vout.resize(1);
        spender.vout[0].nValue = 500'000;
        spender.vout[0].scriptPubKey = CScript() << OP_TRUE;
        spenders.push_back(std::move(spender));
    }
    const CBlock spender_block = CreateAndProcessBlock(spenders, coinbase_script);

    TxoSpenderIndex txospenderindex(interfaces::MakeChain(m_node), 1 << 20, true);
    BOOST_REQUIRE(txospenderindex.Init());
    txospenderindex.Sync();

    const auto parent_spender = txospenderindex.FindSpender(parent_input);
    BOOST_REQUIRE(parent_spender.has_value());
    BOOST_REQUIRE(parent_spender->has_value());
    BOOST_CHECK_EQUAL((*parent_spender)->tx->GetHash(), parent.GetHash());
    BOOST_CHECK_EQUAL((*parent_spender)->block_hash, parent_block.GetHash());

    for (uint32_t output_index{0}; output_index < SPENDER_COUNT; ++output_index) {
        const auto spender = txospenderindex.FindSpender(COutPoint(parent.GetHash(), output_index));
        BOOST_REQUIRE(spender.has_value());
        BOOST_REQUIRE(spender->has_value());
        BOOST_CHECK_EQUAL((*spender)->tx->GetHash(), spenders[output_index].GetHash());
        BOOST_CHECK_EQUAL((*spender)->block_hash, spender_block.GetHash());
    }

    txospenderindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txospenderindex_reinit_reader_race, TestChain100Setup)
{
    const CScript& coinbase_script = m_coinbase_txns[0]->vout[0].scriptPubKey;
    for (int i = 0; i < 10; i++) CreateAndProcessBlock({}, coinbase_script);

    CMutableTransaction spender;
    spender.version = 1;
    spender.vin.resize(1);
    spender.vin[0].prevout = COutPoint(m_coinbase_txns[0]->GetHash(), 0);
    spender.vout.resize(1);
    spender.vout[0].nValue = m_coinbase_txns[0]->GetValueOut();
    spender.vout[0].scriptPubKey = coinbase_script;

    std::vector<unsigned char> vchSig;
    const uint256 hash = SignatureHash(coinbase_script, spender, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_REQUIRE(coinbaseKey.Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    spender.vin[0].scriptSig << vchSig;

    CreateAndProcessBlock({spender}, coinbase_script);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    TxoSpenderIndex txospenderindex(interfaces::MakeChain(m_node), 1 << 20, true);
    BOOST_REQUIRE(txospenderindex.Init());
    txospenderindex.Sync();
    const COutPoint outpoint{spender.vin[0].prevout};

    std::promise<void> reader_started_promise;
    auto reader_started{reader_started_promise.get_future()};
    std::atomic<bool> run{true};
    std::thread reader{[&] {
        reader_started_promise.set_value();
        while (run.load(std::memory_order_relaxed)) {
            (void)txospenderindex.FindSpender(outpoint);
        }
    }};
    reader_started.wait();

    bool init_ok{true};
    for (int i{0}; i < 1000; ++i) {
        txospenderindex.Stop();
        if (!txospenderindex.Init()) {
            init_ok = false;
            break;
        }
        std::this_thread::yield();
    }

    run.store(false, std::memory_order_relaxed);
    reader.join();
    txospenderindex.Stop();
    BOOST_REQUIRE(init_ok);
}

BOOST_FIXTURE_TEST_CASE(txospenderindex_rejects_corrupt_siphash_key, TestChain100Setup)
{
    const CScript& coinbase_script = m_coinbase_txns[0]->vout[0].scriptPubKey;
    for (int i = 0; i < 10; ++i) CreateAndProcessBlock({}, coinbase_script);

    CMutableTransaction spender;
    spender.vin.resize(1);
    spender.vin[0].prevout = COutPoint(m_coinbase_txns[0]->GetHash(), 0);
    spender.vout.resize(1);
    spender.vout[0].nValue = m_coinbase_txns[0]->GetValueOut();
    spender.vout[0].scriptPubKey = coinbase_script;

    std::vector<unsigned char> signature;
    const uint256 signature_hash = SignatureHash(coinbase_script, spender, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_REQUIRE(coinbaseKey.Sign(signature_hash, signature));
    signature.push_back(static_cast<unsigned char>(SIGHASH_ALL));
    spender.vin[0].scriptSig << signature;

    const CBlock block = CreateAndProcessBlock({spender}, coinbase_script);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    const COutPoint spent{spender.vin[0].prevout};
    const fs::path db_path{m_args.GetDataDirNet() / "indexes" / "txospenderindex" / "db"};

    {
        TxoSpenderIndex index{interfaces::MakeChain(m_node), 1 << 20, false, true};
        BOOST_REQUIRE(index.Init());
        index.Sync();
        BOOST_REQUIRE(index.FindSpender(spent).value().has_value());
        BOOST_CHECK_EQUAL(index.GetSummary().best_block_hash, block.GetHash());
        index.Stop();
    }

    {
        CDBWrapper db({.path = db_path, .cache_bytes = 1 << 20, .obfuscate = false, .bloom_filter = false});
        db.Write("siphash_key", std::array<uint8_t, 1>{0});
    }

    TxoSpenderIndex index{interfaces::MakeChain(m_node), 1 << 20, false, false};
    BOOST_CHECK(!index.Init());
    index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
