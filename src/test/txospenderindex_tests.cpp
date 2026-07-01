// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txospenderindex.h>
#include <dbwrapper.h>
#include <kernel/chain.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <validation.h>

#include <cstddef>
#include <span>
#include <utility>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txospenderindex_tests)

struct TxoSpenderIndexTestSetup : public TestChain100Setup {
    struct SpenderBlock {
        CBlock block;
        uint256 tip_hash;
        const CBlockIndex* tip;
        std::vector<COutPoint> spent;
        std::vector<CMutableTransaction> spender;
    };

    SpenderBlock CreateSpenderBlock(size_t count)
    {
        // Setup phase:
        // Mine blocks for coinbase maturity, so we can spend some coinbase outputs in the test.
        const CScript& coinbase_script = m_coinbase_txns[0]->vout[0].scriptPubKey;
        for (int i = 0; i < 10; i++) CreateAndProcessBlock({}, coinbase_script);

        // Spend outputs
        std::vector<COutPoint> spent(count);
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
        CBlock block{CreateAndProcessBlock(spender, coinbase_script)};
        uint256 tip_hash{block.GetHash()};
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
        const CBlockIndex* tip{WITH_LOCK(::cs_main, return m_node.chainman->ActiveTip())};
        BOOST_CHECK_EQUAL(tip->GetBlockHash(), tip_hash);

        return {std::move(block), tip_hash, tip, std::move(spent), std::move(spender)};
    }
};

BOOST_FIXTURE_TEST_CASE(txospenderindex_initial_sync, TxoSpenderIndexTestSetup)
{
    const auto [block, tip_hash, tip, spent, spender]{CreateSpenderBlock(10)};

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

BOOST_FIXTURE_TEST_CASE(txospenderindex_value_markers, TxoSpenderIndexTestSetup)
{
    const auto [block, tip_hash, tip, spent, spender]{CreateSpenderBlock(2)};
    const auto entries{txospenderindex::BuildSpenderPositions(kernel::MakeBlockInfo(tip, &block))};
    BOOST_REQUIRE_EQUAL(entries.size(), 2U);

    {
        constexpr std::pair<uint64_t, uint64_t> siphash_key{1, 2};
        CDBWrapper dbw({.path = m_args.GetDataDirNet() / "indexes" / "txospenderindex" / "db", .cache_bytes = 1_MiB});
        dbw.Write("siphash_key", siphash_key);
        CDBBatch batch(dbw);
        batch.Write(txospenderindex::CreateKey(siphash_key, entries[0].first, entries[0].second), ""); // Old value format
        batch.Write(txospenderindex::CreateKey(siphash_key, entries[1].first, entries[1].second), std::span<const std::byte>{}); // New value format
        dbw.WriteBatch(batch);
    }

    TxoSpenderIndex index(interfaces::MakeChain(m_node), 1_MiB);
    BOOST_REQUIRE(index.Init());

    for (size_t i{0}; i < spent.size(); ++i) {
        const auto tx_spender{index.FindSpender(spent[i])};
        const auto& [tx, block_hash]{*Assert(*Assert(tx_spender))};
        BOOST_CHECK_EQUAL(tx->GetHash(), spender[i].GetHash());
        BOOST_CHECK_EQUAL(block_hash, tip_hash);
    }

    index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
