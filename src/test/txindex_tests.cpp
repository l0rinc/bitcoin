// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chainparams.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <kernel/types.h>
#include <node/chainstate.h>
#include <node/kernel_notifications.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/byte_units.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txindex_tests)

BOOST_FIXTURE_TEST_CASE(txindex_block_until_synced_before_genesis_activation, ChainTestingSetup)
{
    auto& chainman{*Assert(m_node.chainman)};
    node::ChainstateLoadOptions options;
    options.mempool = Assert(m_node.mempool.get());
    options.coins_db_in_memory = m_coins_db_in_memory;
    options.wipe_chainstate_db = m_args.GetBoolArg("-reindex", false) || m_args.GetBoolArg("-reindex-chainstate", false);
    options.prune = chainman.m_blockman.IsPruneMode();
    options.check_blocks = m_args.GetIntArg("-checkblocks", DEFAULT_CHECKBLOCKS);
    options.check_level = m_args.GetIntArg("-checklevel", DEFAULT_CHECKLEVEL);
    options.require_full_verification = m_args.IsArgSet("-checkblocks") || m_args.IsArgSet("-checklevel");

    auto [status, error]{node::LoadChainstate(chainman, m_kernel_cache_sizes, options)};
    BOOST_REQUIRE(status == node::ChainstateLoadStatus::SUCCESS);
    std::tie(status, error) = node::VerifyLoadedChainstate(chainman, options);
    BOOST_REQUIRE(status == node::ChainstateLoadStatus::SUCCESS);
    m_node.notifications->setChainstateLoaded(true);

    BOOST_REQUIRE(!WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()));

    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, true);
    BOOST_REQUIRE(txindex.Init());
    BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());

    const uint256 genesis_hash{Params().GenesisBlock().GetHash()};
    BOOST_REQUIRE(WITH_LOCK(::cs_main, return chainman.m_blockman.LookupBlockIndex(genesis_hash) != nullptr));
    m_node.validation_signals->ChainStateFlushed(kernel::ChainstateRole{}, CBlockLocator{{genesis_hash}});
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());

    BlockValidationState state;
    BOOST_REQUIRE(chainman.ActiveChainstate().ActivateBestChain(state));
    BOOST_REQUIRE(WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip() != nullptr));
    BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());
    BOOST_CHECK(txindex.GetSummary().best_block_hash == Params().GenesisBlock().GetHash());

    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_initial_sync, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, true);
    BOOST_REQUIRE(txindex.Init());

    CTransactionRef tx_disk;
    uint256 block_hash;

    // Transaction should not be found in the index before it is started.
    for (const auto& txn : m_coinbase_txns) {
        BOOST_CHECK(!txindex.FindTx(txn->GetHash(), block_hash, tx_disk));
    }

    // BlockUntilSyncedToCurrentChain should return false before txindex is started.
    BOOST_CHECK(!txindex.BlockUntilSyncedToCurrentChain());

    txindex.Sync();

    // Check that txindex excludes genesis block transactions.
    const CBlock& genesis_block = Params().GenesisBlock();
    for (const auto& txn : genesis_block.vtx) {
        BOOST_CHECK(!txindex.FindTx(txn->GetHash(), block_hash, tx_disk));
    }

    // Check that txindex has all txs that were in the chain before it started.
    for (const auto& txn : m_coinbase_txns) {
        if (!txindex.FindTx(txn->GetHash(), block_hash, tx_disk)) {
            BOOST_ERROR("FindTx failed");
        } else if (tx_disk->GetHash() != txn->GetHash()) {
            BOOST_ERROR("Read incorrect tx");
        }
    }

    // Check that new transactions in new blocks make it into the index.
    for (int i = 0; i < 10; i++) {
        CScript coinbase_script_pub_key = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
        std::vector<CMutableTransaction> no_txns;
        const CBlock& block = CreateAndProcessBlock(no_txns, coinbase_script_pub_key);
        const CTransaction& txn = *block.vtx[0];

        BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());
        if (!txindex.FindTx(txn.GetHash(), block_hash, tx_disk)) {
            BOOST_ERROR("FindTx failed");
        } else if (tx_disk->GetHash() != txn.GetHash()) {
            BOOST_ERROR("Read incorrect tx");
        }
    }

    // shutdown sequence (c.f. Shutdown() in init.cpp)
    txindex.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
