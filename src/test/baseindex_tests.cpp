// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/base.h>
#include <index/coinstatsindex.h>
#include <interfaces/chain.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {

class ReinitGateIndex final : public BaseIndex
{
public:
    ReinitGateIndex(std::unique_ptr<interfaces::Chain> chain, const fs::path& db_path)
        : BaseIndex(std::move(chain), "reinit-gate", "reinitgate"),
          m_db(std::make_unique<DB>(db_path, 1_MiB))
    {
    }

    void BlockNextInit()
    {
        m_block_init.store(true);
        m_release_init.store(false);
        m_init_entered.store(false);
    }

    void WaitForInit()
    {
        while (!m_init_entered.load()) std::this_thread::yield();
    }

    void ReleaseInit()
    {
        m_release_init.store(true);
    }

    void FailNextInit()
    {
        m_fail_next_init.store(true);
    }

protected:
    bool AllowPrune() const override { return true; }

    bool CustomInit(const std::optional<interfaces::BlockRef>&) override
    {
        if (m_block_init.load()) {
            m_init_entered.store(true);
            while (!m_release_init.load()) std::this_thread::yield();
        }
        return !m_fail_next_init.exchange(false);
    }

    DB& GetDB() const override { return *m_db; }

private:
    std::unique_ptr<DB> m_db;
    std::atomic<bool> m_block_init{false};
    std::atomic<bool> m_init_entered{false};
    std::atomic<bool> m_release_init{false};
    std::atomic<bool> m_fail_next_init{false};
};

} // namespace

// Tests of generic BaseIndex functionality that is independent of which
// concrete index is being used. CoinStatsIndex is used here merely as a
// convenient instantiation of BaseIndex.
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

// Reinitialization is used while RPC/REST callers remain available during
// snapshot activation. They must not observe the old synced flag before the
// index-specific state has been restored by CustomInit().
BOOST_FIXTURE_TEST_CASE(baseindex_reinit_not_synced_during_custom_init, TestChain100Setup)
{
    Chainstate& chainstate = Assert(m_node.chainman)->ActiveChainstate();
    ReinitGateIndex index{interfaces::MakeChain(m_node),
                          m_args.GetDataDirNet() / "indexes" / "reinit-gate"};
    BOOST_REQUIRE(index.Init());
    index.Sync();

    chainstate.ForceFlushStateToDisk();
    m_node.chain->context()->validation_signals->SyncWithValidationInterfaceQueue();

    index.Stop();
    BOOST_CHECK(!index.BlockUntilSyncedToCurrentChain());
    index.BlockNextInit();
    std::atomic<bool> init_ok{false};
    std::thread init_thread{[&] { init_ok.store(index.Init()); }};
    index.WaitForInit();

    std::atomic<bool> query_done{false};
    std::atomic<bool> query_result{true};
    std::thread query_thread{[&] {
        query_result.store(index.BlockUntilSyncedToCurrentChain());
        query_done.store(true);
    }};
    const auto query_deadline{std::chrono::steady_clock::now() + std::chrono::seconds(1)};
    while (!query_done.load() && std::chrono::steady_clock::now() < query_deadline) {
        std::this_thread::yield();
    }
    const bool query_completed_during_init{query_done.load()};

    index.ReleaseInit();
    init_thread.join();
    query_thread.join();
    BOOST_REQUIRE(init_ok.load());
    BOOST_CHECK(query_completed_during_init);
    BOOST_CHECK(!query_result.load());
    index.Stop();
}

BOOST_FIXTURE_TEST_CASE(baseindex_failed_reinit_clears_initialized_state, TestChain100Setup)
{
    ReinitGateIndex index{interfaces::MakeChain(m_node),
                          m_args.GetDataDirNet() / "indexes" / "reinit-gate-failure"};
    BOOST_REQUIRE(index.Init());
    index.Stop();

    index.FailNextInit();
    BOOST_CHECK(!index.Init());
    BOOST_CHECK_THROW((void)index.StartBackgroundSync(), std::logic_error);

    index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
