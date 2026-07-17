// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/validation.h>
#include <index/coinstatsindex.h>
#include <interfaces/chain.h>
#include <kernel/coinstats.h>
#include <kernel/types.h>
#include <key.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <util/check.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <optional>
#include <span>
#include <vector>

using kernel::ChainstateRole;

BOOST_AUTO_TEST_SUITE(coinstatsindex_tests)

BOOST_FIXTURE_TEST_CASE(coinstatsindex_initial_sync, TestChain100Setup)
{
    CoinStatsIndex coin_stats_index{interfaces::MakeChain(m_node), 1_MiB, true};
    BOOST_REQUIRE(coin_stats_index.Init());

    const CBlockIndex* block_index;
    {
        LOCK(cs_main);
        block_index = m_node.chainman->ActiveChain().Tip();
    }

    // CoinStatsIndex should not be found before it is started.
    BOOST_CHECK(!coin_stats_index.LookUpStats(*block_index));

    // BlockUntilSyncedToCurrentChain should return false before CoinStatsIndex
    // is started.
    BOOST_CHECK(!coin_stats_index.BlockUntilSyncedToCurrentChain());

    coin_stats_index.Sync();

    // Check that CoinStatsIndex works for genesis block.
    const CBlockIndex* genesis_block_index;
    {
        LOCK(cs_main);
        genesis_block_index = m_node.chainman->ActiveChain().Genesis();
    }
    BOOST_CHECK(coin_stats_index.LookUpStats(*genesis_block_index));

    // Check that CoinStatsIndex updates with new blocks.
    BOOST_CHECK(coin_stats_index.LookUpStats(*block_index));

    const CScript script_pub_key{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};
    std::vector<CMutableTransaction> noTxns;
    CreateAndProcessBlock(noTxns, script_pub_key);

    // Let the CoinStatsIndex to catch up again.
    BOOST_CHECK(coin_stats_index.BlockUntilSyncedToCurrentChain());

    const CBlockIndex* new_block_index;
    {
        LOCK(cs_main);
        new_block_index = m_node.chainman->ActiveChain().Tip();
    }
    BOOST_CHECK(coin_stats_index.LookUpStats(*new_block_index));

    BOOST_CHECK(block_index != new_block_index);

    // Shutdown sequence (c.f. Shutdown() in init.cpp)
    coin_stats_index.Stop();
}

// Test shutdown between BlockConnected and ChainStateFlushed notifications,
// make sure index is not corrupted and is able to reload.
BOOST_FIXTURE_TEST_CASE(coinstatsindex_unclean_shutdown, TestChain100Setup)
{
    Chainstate& chainstate = Assert(m_node.chainman)->ActiveChainstate();
    const CChainParams& params = Params();
    {
        CoinStatsIndex index{interfaces::MakeChain(m_node), 1_MiB};
        BOOST_REQUIRE(index.Init());
        index.Sync();
        std::shared_ptr<const CBlock> new_block;
        CBlockIndex* new_block_index = nullptr;
        {
            const CScript script_pub_key{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};
            const CBlock block = this->CreateBlock({}, script_pub_key);

            new_block = std::make_shared<CBlock>(block);

            LOCK(cs_main);
            BlockValidationState state;
            BOOST_CHECK(CheckBlock(block, state, params.GetConsensus()));
            BOOST_CHECK(m_node.chainman->AcceptBlock(new_block, state, &new_block_index, true, nullptr, nullptr, true));
            CCoinsViewCache view(&chainstate.CoinsTip());
            BOOST_CHECK(chainstate.ConnectBlock(block, state, new_block_index, view));
        }
        // Send block connected notification, then stop the index without
        // sending a chainstate flushed notification. Prior to #24138, this
        // would cause the index to be corrupted and fail to reload.
        ValidationInterfaceTest::BlockConnected(ChainstateRole{}, index, new_block, new_block_index);
        index.Stop();
    }

    {
        CoinStatsIndex index{interfaces::MakeChain(m_node), 1_MiB};
        BOOST_REQUIRE(index.Init());
        // Make sure the index can be loaded.
        BOOST_REQUIRE(index.StartBackgroundSync());
        index.Stop();
    }
}

BOOST_FIXTURE_TEST_CASE(coinstatsindex_reorg_restart, TestChain100Setup)
{
    auto& chainman{*Assert(m_node.chainman)};
    auto& chainstate{chainman.ActiveChainstate()};
    auto current_tip{[&] { return Assert(WITH_LOCK(::cs_main, return chainman.ActiveTip())); }};
    auto summary_after_callbacks{[&](CoinStatsIndex& index) {
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
        return index.GetSummary();
    }};
    auto summary_at{[](const CBlockIndex& tip) { return IndexSummary{"coinstatsindex", /*synced=*/true, tip.nHeight, tip.GetBlockHash()}; }};
    auto reconsider{[&](CBlockIndex& block) {
        {
            LOCK(::cs_main);
            chainstate.ResetBlockFailureFlags(&block);
            chainman.RecalculateBestHeader();
        }
        BlockValidationState state{};
        BOOST_REQUIRE(chainstate.ActivateBestChain(state));
    }};
    auto* old_tip{current_tip()};
    auto* old_first{Assert(old_tip->pprev)};
    auto* fork_point{Assert(old_first->pprev)};

    // Keep the locator at old_tip while a deeper replacement fork overwrites
    // its last two height entries.
    {
        CoinStatsIndex index{interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false, /*f_wipe=*/true};
        BOOST_REQUIRE(index.Init());
        index.Sync();
        chainstate.ForceFlushStateToDisk();
        BOOST_CHECK(summary_after_callbacks(index) == summary_at(*old_tip));

        BlockValidationState state{};
        BOOST_REQUIRE(chainstate.InvalidateBlock(state, old_first));
        BOOST_REQUIRE(chainstate.ActivateBestChain(state));
        BOOST_REQUIRE_EQUAL(current_tip(), fork_point);
        for (int i{0}; i < 3; ++i) {
            CreateAndProcessBlock({}, CScript{} << OP_FALSE);
        }
        BOOST_CHECK(summary_after_callbacks(index) == summary_at(*current_tip()));
        BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return chainstate.GetLastFlushedBlock()), old_tip);
    }
    auto* replacement_tip{current_tip()};
    auto* replacement_first{Assert(replacement_tip->GetAncestor(old_first->nHeight))};

    // Restore chainstate to the flushed branch without changing the index DB.
    BlockValidationState state{};
    BOOST_REQUIRE(chainstate.InvalidateBlock(state, replacement_first));
    BOOST_REQUIRE(chainstate.ActivateBestChain(state));
    reconsider(*old_first);
    BOOST_REQUIRE_EQUAL(current_tip(), old_tip);

    // Drain callbacks queued while the index was stopped before registering the restarted instance.
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    CoinStatsIndex index{interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false, /*f_wipe=*/false};
    BOOST_REQUIRE(index.Init());
    BOOST_CHECK(index.GetSummary() == summary_at(*old_tip));

    // Reverting old_tip requires its parent from the hash index because the
    // replacement fork already occupies that height entry.
    reconsider(*replacement_first);
    BOOST_REQUIRE_EQUAL(current_tip(), replacement_tip);
    const auto summary{summary_after_callbacks(index)};
    // FatalErrorf requests shutdown, distinguishing a failed revert from a lagging index.
    BOOST_CHECK(m_interrupt);                     // TODO: Recover the old parent from its hash entry.
    BOOST_CHECK(summary == summary_at(*old_tip)); // TODO: Advance to replacement_tip.
}

BOOST_AUTO_TEST_SUITE_END()
