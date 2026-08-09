// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <latch>
#include <thread>

// Tests of generic BaseIndex functionality that is independent of which
// concrete index is being used. Concrete indexes are used here merely as
// convenient instantiations of BaseIndex.
BOOST_AUTO_TEST_SUITE(baseindex_tests)

// Test that the index does not commit ahead of the chainstate's last
// flushed block. If it did, a subsequent unclean shutdown would corrupt
// the index, because during reverting it would require blocks that were
// never flushed to disk.
BOOST_FIXTURE_TEST_CASE(baseindex_no_commit_ahead_of_flush, TestChain100Setup)
{
    Chainstate& chainstate = Assert(m_node.chainman)->ActiveChainstate();
    auto sync_index = [&](bool do_flush, int expected_sync_height, int expected_commit_height) {
        CoinStatsIndex index{interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB};
        BOOST_REQUIRE(index.Init());
        index.Sync();
        if (do_flush) {
            chainstate.ForceFlushStateToDisk();
            m_node.chain->context()->validation_signals->SyncWithValidationInterfaceQueue();
        }
        BOOST_CHECK_EQUAL(index.GetSummary().best_block_height, expected_sync_height);
        index.Stop();
        // Reload index to see which block data was actually committed.
        BOOST_REQUIRE(index.Init());
        BOOST_CHECK_EQUAL(index.GetSummary().best_block_height, expected_commit_height);
        index.Stop();
    };

    // Part 1: Sync, then "crash" (stop without flushing). Models a node that
    // started up, had its index catch up, but never flushed before going down.
    // The end-of-sync Commit() runs at chain tip (height 100) but
    // m_last_flushed_block is null, so it is skipped.
    sync_index(false, 100, 0);

    // Part 2: Restart cleanly. Sync, force a chainstate flush, and drain the
    // validation queue so the index's ChainStateFlushed callback runs.
    // Now m_last_flushed_block == tip == 100 and the index can commit.
    sync_index(true, 100, 100);

    // Part 3: Connect a new block on the chain without flushing
    // (m_last_flushed_block stays at 100). For a real node this would happen
    // in parallel with Sync(). Here we do it before Sync() to make the race
    // state deterministic.
    CreateAndProcessBlock({}, CScript() << OP_TRUE);
    sync_index(false, 101, 100);
}

// Test that index readers can run concurrently with the index being stopped
// and restarted. TxIndex is used because its FindTx reader runs off the sync
// thread without synchronization.
BOOST_FIXTURE_TEST_CASE(baseindex_restart_concurrent_reads, TestChain100Setup)
{
    TxIndex index{interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/true};
    BOOST_REQUIRE(index.Init());
    index.Sync();

    const Txid txid{m_coinbase_txns[1]->GetHash()};
    std::latch reader_started{1};
    std::atomic_bool run{true};
    std::thread reader{[&] {
        reader_started.count_down();
        while (run.load(std::memory_order_relaxed)) {
            CTransactionRef tx;
            uint256 block_hash;
            // These reads overlap Stop()/Init() and exercise the unsynchronized reader paths.
            index.BlockUntilSyncedToCurrentChain();
            index.FindTx(txid, block_hash, tx);
        }
    }};
    reader_started.wait();

    bool init_ok{true};
    for (int i{0}; init_ok && i < 1000; ++i) {
        index.Stop();
        init_ok = index.Init();
        std::this_thread::yield();
    }

    run = false;
    reader.join();
    index.Stop();
    BOOST_REQUIRE(init_ok);
}

BOOST_AUTO_TEST_SUITE_END()
