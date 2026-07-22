// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <node/blockstorage.h>
#include <node/kernel_notifications.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/chainstate.h>
#include <test/util/coins.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/check.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <optional>
#include <vector>

class CTxMemPool;

BOOST_FIXTURE_TEST_SUITE(validation_chainstate_tests, ChainTestingSetup)

class CoinsViewAccessTracker : public CCoinsViewBacked
{
public:
    CoinsViewAccessTracker(CCoinsView& view, std::vector<COutPoint>& accesses)
        : CCoinsViewBacked{&view}, m_accesses{accesses}
    {
    }

    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override
    {
        m_accesses.push_back(outpoint);
        return CCoinsViewBacked::GetCoin(outpoint);
    }

private:
    std::vector<COutPoint>& m_accesses;
};

//! Test resizing coins-related Chainstate caches during runtime.
//!
BOOST_AUTO_TEST_CASE(validation_chainstate_resize_caches)
{
    ChainstateManager& manager = *Assert(m_node.chainman);
    CTxMemPool& mempool = *Assert(m_node.mempool);
    Chainstate& c1 = WITH_LOCK(cs_main, return manager.InitializeChainstate(&mempool));
    c1.InitCoinsDB(
        /*cache_size_bytes=*/8_MiB, /*in_memory=*/true, /*should_wipe=*/false);
    WITH_LOCK(::cs_main, c1.InitCoinsCache(8_MiB));
    BOOST_REQUIRE(manager.LoadGenesisBlock()); // Need at least one block loaded to be able to flush caches

    // Add a coin to the in-memory cache, upsize once, then downsize.
    {
        LOCK(::cs_main);
        const auto outpoint = AddTestCoin(m_rng, c1.CoinsTip());

        // Set a meaningless bestblock value in the coinsview cache - otherwise we won't
        // flush during ResizecoinsCaches() and will subsequently hit an assertion.
        c1.CoinsTip().SetBestBlock(m_rng.rand256());

        BOOST_CHECK(c1.CoinsTip().HaveCoinInCache(outpoint));

        c1.ResizeCoinsCaches(
            16_MiB, // upsizing the coinsview cache
            4_MiB // downsizing the coinsdb cache
        );

        // View should still have the coin cached, since we haven't destructed the cache on upsize.
        BOOST_CHECK(c1.CoinsTip().HaveCoinInCache(outpoint));

        c1.ResizeCoinsCaches(
            4_MiB, // downsizing the coinsview cache
            8_MiB // upsizing the coinsdb cache
        );

        // The view cache should be empty since we had to destruct to downsize.
        BOOST_CHECK(!c1.CoinsTip().HaveCoinInCache(outpoint));
    }
}

BOOST_FIXTURE_TEST_CASE(connect_tip_does_not_cache_inputs_on_failed_connect, TestChain100Setup)
{
    Chainstate& chainstate{Assert(m_node.chainman)->ActiveChainstate()};

    COutPoint outpoint;
    {
        LOCK(cs_main);
        outpoint = AddTestCoin(m_rng, chainstate.CoinsTip());
        chainstate.CoinsTip().Flush(/*reallocate_cache=*/false);
    }

    CMutableTransaction tx;
    tx.vin.emplace_back(outpoint);
    tx.vout.emplace_back(MAX_MONEY, CScript{} << OP_TRUE);

    const auto tip{WITH_LOCK(cs_main, return chainstate.m_chain.Tip()->GetBlockHash())};
    const CBlock block{CreateBlock({tx}, CScript{} << OP_TRUE)};
    BOOST_CHECK(Assert(m_node.chainman)->ProcessNewBlock(std::make_shared<CBlock>(block), true, true, nullptr));

    LOCK(cs_main);
    BOOST_CHECK_EQUAL(tip, chainstate.m_chain.Tip()->GetBlockHash()); // block rejected
    BOOST_CHECK(!chainstate.CoinsTip().HaveCoinInCache(outpoint));    // input not cached
}

BOOST_FIXTURE_TEST_CASE(disconnect_block_coin_access_order, TestChain100Setup)
{
    mineBlocks(3);
    const CScript script_pub_key{CScript{} << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};
    const std::vector<COutPoint> parent_inputs{
        COutPoint{m_coinbase_txns[0]->GetHash(), 0},
        COutPoint{m_coinbase_txns[1]->GetHash(), 0},
    };
    const CMutableTransaction parent{CreateValidMempoolTransaction(
        {m_coinbase_txns[0], m_coinbase_txns[1]}, parent_inputs, /*input_height=*/1,
        {coinbaseKey}, {CTxOut{99 * COIN, script_pub_key}}, /*submit=*/false)};
    const CTransactionRef parent_ref{MakeTransactionRef(parent)};
    const std::vector<COutPoint> child_inputs{
        COutPoint{parent_ref->GetHash(), 0},
        COutPoint{m_coinbase_txns[2]->GetHash(), 0},
    };
    const CMutableTransaction child{CreateValidMempoolTransaction(
        {parent_ref, m_coinbase_txns[2]}, child_inputs, /*input_height=*/1,
        {coinbaseKey}, {CTxOut{148 * COIN, script_pub_key}}, /*submit=*/false)};
    const CBlock block{CreateAndProcessBlock({parent, child}, script_pub_key)};

    LOCK(cs_main);
    Chainstate& chainstate{Assert(m_node.chainman)->ActiveChainstate()};
    const CBlockIndex* tip{chainstate.m_chain.Tip()};
    BOOST_REQUIRE_EQUAL(tip->GetBlockHash(), block.GetHash());

    std::vector<COutPoint> accesses;
    CoinsViewAccessTracker tracker{chainstate.CoinsTip(), accesses};
    CCoinsViewCache view{&tracker};
    BOOST_REQUIRE_EQUAL(chainstate.DisconnectBlock(block, tip, view), DISCONNECT_OK);

    // Restoring the child's first input caches the parent output before the parent is disconnected.
    const std::vector<COutPoint> expected{
        COutPoint{block.vtx[2]->GetHash(), 0},
        child_inputs[1],
        child_inputs[0],
        parent_inputs[1],
        parent_inputs[0],
        COutPoint{block.vtx[0]->GetHash(), 0},
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(accesses.begin(), accesses.end(), expected.begin(), expected.end());
}

//! Test UpdateTip behavior for both active and background chainstates.
//!
//! When run on the background chainstate, UpdateTip should do a subset
//! of what it does for the active chainstate.
BOOST_FIXTURE_TEST_CASE(chainstate_update_tip, TestChain100Setup)
{
    ChainstateManager& chainman = *Assert(m_node.chainman);
    const auto get_notify_tip{[&]() {
        LOCK(m_node.notifications->m_tip_block_mutex);
        BOOST_REQUIRE(m_node.notifications->TipBlock());
        return *m_node.notifications->TipBlock();
    }};
    uint256 curr_tip = get_notify_tip();

    // Mine 10 more blocks, putting at us height 110 where a valid assumeutxo value can
    // be found.
    mineBlocks(10);

    // After adding some blocks to the tip, best block should have changed.
    BOOST_CHECK(get_notify_tip() != curr_tip);

    // Grab block 1 from disk; we'll add it to the background chain later.
    std::shared_ptr<CBlock> pblockone = std::make_shared<CBlock>();
    {
        LOCK(::cs_main);
        chainman.m_blockman.ReadBlock(*pblockone, *chainman.ActiveChain()[1]);
    }

    BOOST_REQUIRE(CreateAndActivateUTXOSnapshot(
        this, NoMalleation, /*reset_chainstate=*/ true));

    // Ensure our active chain is the snapshot chainstate.
    BOOST_CHECK(WITH_LOCK(::cs_main, return chainman.CurrentChainstate().m_from_snapshot_blockhash));

    curr_tip = get_notify_tip();

    // Mine a new block on top of the activated snapshot chainstate.
    mineBlocks(1);  // Defined in TestChain100Setup.

    // After adding some blocks to the snapshot tip, best block should have changed.
    BOOST_CHECK(get_notify_tip() != curr_tip);

    curr_tip = get_notify_tip();

    Chainstate& background_cs{*Assert(WITH_LOCK(::cs_main, return chainman.HistoricalChainstate()))};

    // Append the first block to the background chain.
    BlockValidationState state;
    CBlockIndex* pindex = nullptr;
    const CChainParams& chainparams = Params();
    bool newblock = false;

    // TODO: much of this is inlined from ProcessNewBlock(); just reuse PNB()
    // once it is changed to support multiple chainstates.
    {
        LOCK(::cs_main);
        bool checked = CheckBlock(*pblockone, state, chainparams.GetConsensus());
        BOOST_CHECK(checked);
        bool accepted = chainman.AcceptBlock(
            pblockone, state, &pindex, true, nullptr, &newblock, true);
        BOOST_CHECK(accepted);
    }

    // UpdateTip is called here
    bool block_added = background_cs.ActivateBestChain(state, pblockone);

    // Ensure tip is as expected
    BOOST_CHECK_EQUAL(background_cs.m_chain.Tip()->GetBlockHash(), pblockone->GetHash());

    // get_notify_tip() should be unchanged after adding a block to the background
    // validation chain.
    BOOST_CHECK(block_added);
    BOOST_CHECK_EQUAL(curr_tip, get_notify_tip());
}

BOOST_AUTO_TEST_SUITE_END()
