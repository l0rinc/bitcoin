// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chainparams.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <node/mining_types.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/byte_units.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txindex_tests)

struct TxIndexChainTestingSetup : public ChainTestingSetup {
    TxIndexChainTestingSetup() : ChainTestingSetup{ChainType::REGTEST} {}
};

BOOST_FIXTURE_TEST_CASE(txindex_block_until_synced_before_genesis_activation, TxIndexChainTestingSetup)
{
    auto& chainman{*Assert(m_node.chainman)};
    LoadVerifyChainstate();

    BOOST_REQUIRE(!WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()));

    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, true);
    BOOST_REQUIRE(txindex.Init());
    BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());

    BlockValidationState state;
    BOOST_REQUIRE(chainman.ActiveChainstate().ActivateBestChain(state));
    BOOST_REQUIRE(WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip() != nullptr));
    BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());

    const COutPoint coinbase{MineBlock(m_node, {
        .use_mempool = false,
        .coinbase_output_script = CScript{} << OP_TRUE,
    })};
    BOOST_REQUIRE(!coinbase.IsNull());
    BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());
    CTransactionRef tx_disk;
    uint256 block_hash;
    BOOST_REQUIRE(txindex.FindTx(coinbase.hash, block_hash, tx_disk));
    BOOST_CHECK(tx_disk->GetHash() == coinbase.hash);

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
