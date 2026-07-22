// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chainparams.h>
#include <common/args.h>
#include <crypto/hex_base.h>
#include <dbwrapper.h>
#include <index/txindex.h>
#include <index/txindex_key.h>
#include <interfaces/chain.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <util/strencodings.h>
#include <validation.h>

#include <cstddef>
#include <string_view>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txindex_tests)

class TxIndexTest
{
public:
    static CDBWrapper& GetDB(const TxIndex& txindex) { return static_cast<CDBWrapper&>(txindex.GetDB()); }
    static CBlockLocator ReadBestBlock(const TxIndex& txindex) { return txindex.GetDB().ReadBestBlock(); }
    static void WriteBestBlock(const TxIndex& txindex, const CBlockLocator& locator)
    {
        auto& db{txindex.GetDB()};
        CDBBatch batch{db};
        db.WriteBestBlock(batch, locator);
        db.WriteBatch(batch);
    }
};

BOOST_AUTO_TEST_CASE(txindex_position_encoding)
{
    constexpr struct { txindex::BlockTxPosition position; std::string_view encoded; } test_vectors[]{
        {{0, 0}, "ffffff000000"},
        {{1, 2}, "fffffe000002"},
        {{10'000'000, 123}, "67697f00007b"},
        {{456, 3'999'999}, "fffe373d08ff"},
    };

    for (const auto& [position, encoded] : test_vectors) {
        BOOST_CHECK_EQUAL(HexStr(DataStream{} << position), encoded);

        txindex::BlockTxPosition decoded;
        BOOST_CHECK((DataStream{ParseHex(encoded)} >> decoded).empty());
        BOOST_CHECK(decoded == position);
    }

    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txindex::BlockSeqKey{1}), "73000001");
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txindex::DBKey{{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}}, {1, 2}}), "780102030405fffffe000002");
}

BOOST_AUTO_TEST_CASE(txindex_hash_prefix)
{
    BOOST_CHECK_EQUAL(
        HexStr(txindex::CreateKeyPrefix(
            SipHasher13UJ{0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL},
            Txid{"1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a09080706050403020100"})),
        "c67d87b08c");
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

BOOST_FIXTURE_TEST_CASE(txindex_locator_upgrade, TestChain100Setup)
{
    const CBlockLocator legacy_locator{{m_coinbase_txns.front()->GetHash().ToUint256()}};
    const CBlockLocator versioned_locator{{m_coinbase_txns.back()->GetHash().ToUint256()}};
    {
        CDBWrapper db{DBParams{.path = gArgs.GetDataDirNet() / "indexes" / "txindex", .cache_bytes = 1_MiB}};
        db.Write(uint8_t{'B'}, legacy_locator);
    }

    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false);
    BOOST_CHECK(TxIndexTest::ReadBestBlock(txindex).vHave == legacy_locator.vHave);

    TxIndexTest::WriteBestBlock(txindex, versioned_locator);
    BOOST_CHECK(TxIndexTest::ReadBestBlock(txindex).vHave == versioned_locator.vHave);

    CBlockLocator stored_legacy_locator;
    BOOST_REQUIRE(TxIndexTest::GetDB(txindex).Read(uint8_t{'B'}, stored_legacy_locator));
    BOOST_CHECK(stored_legacy_locator.vHave == legacy_locator.vHave);
}

BOOST_AUTO_TEST_SUITE_END()
