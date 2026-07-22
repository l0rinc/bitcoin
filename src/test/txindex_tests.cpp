// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chain.h>
#include <chainparams.h>
#include <common/args.h>
#include <crypto/hex_base.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <index/disktxpos.h>
#include <index/txindex.h>
#include <index/txindex_key.h>
#include <interfaces/chain.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <streams.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <validation.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txindex_tests)

// Grants tests access to the otherwise non-public txindex database handle.
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

namespace {

SipHasher13UJ ReadHasher(const CDBWrapper& db)
{
    std::pair<uint64_t, uint64_t> salt;
    BOOST_REQUIRE(db.Read(txindex::DB_TXID_HASH_SALT, salt));
    return SipHasher13UJ{salt.first, salt.second};
}

std::vector<txindex::BlockTxPosition> BucketPositions(CDBWrapper& db, const txindex::TxHashKeyPrefix& prefix)
{
    std::vector<txindex::BlockTxPosition> positions;
    std::unique_ptr<CDBIterator> it{db.NewIterator()};
    txindex::DBKey key{prefix, {}};
    for (it->Seek(std::pair{txindex::DB_TXINDEX_HASHED, prefix}); it->Valid() && it->GetKey(key) && key.hash_prefix == prefix; it->Next()) {
        positions.push_back(key.pos);
    }
    return positions;
}

FlatFilePos BlockFilePos(const ChainstateManager& chainman, uint32_t height)
{
    LOCK(cs_main);
    const CBlockIndex* block_index{chainman.ActiveChain()[height]};
    BOOST_REQUIRE(block_index);
    return {block_index->nFile, block_index->nDataPos};
}

uint256 LookupTx(const TxIndex& txindex, const Txid& txid)
{
    CTransactionRef tx;
    uint256 block_hash;
    BOOST_REQUIRE_MESSAGE(txindex.FindTx(txid, block_hash, tx), "FindTx failed for " + txid.ToString());
    BOOST_CHECK(Assert(tx)->GetHash() == txid);
    return block_hash;
}

} // namespace

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
    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/true);
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
        LookupTx(txindex, txn->GetHash());
    }

    // Check that new transactions in new blocks make it into the index.
    for (int i = 0; i < 10; i++) {
        CScript coinbase_script_pub_key = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
        std::vector<CMutableTransaction> no_txns;
        const CBlock& block = CreateAndProcessBlock(no_txns, coinbase_script_pub_key);
        const CTransaction& txn = *block.vtx[0];

        BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());
        LookupTx(txindex, txn.GetHash());
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

BOOST_FIXTURE_TEST_CASE(txindex_collision_scan_path, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const SipHasher13UJ hasher{ReadHasher(db)};

    // Lookups scan candidates in descending sequence order, so entries of
    // later-connected blocks are tried first. Forge a colliding entry under the
    // first coinbase's prefix pointing at the last coinbase, so looking up the
    // first tx must scan that false positive first.
    const Txid fake_txid{m_coinbase_txns.back()->GetHash()};
    const Txid target_txid{m_coinbase_txns.front()->GetHash()};
    const auto fake_prefix{txindex::CreateKeyPrefix(hasher, fake_txid)};
    const auto target_prefix{txindex::CreateKeyPrefix(hasher, target_txid)};
    // Distinct prefixes guarantee the target's bucket initially holds only the target.
    BOOST_REQUIRE(fake_prefix != target_prefix);

    // Read the last coinbase's encoded position straight from its bucket.
    const auto fake_bucket{BucketPositions(db, fake_prefix)};
    BOOST_REQUIRE_EQUAL(fake_bucket.size(), 1U);
    const txindex::BlockTxPosition fake_pos{fake_bucket.front()};
    const txindex::BlockTxPosition unreadable_pos{
        fake_pos.block_seq,
        static_cast<uint32_t>(BigEndianFormatter<txindex::TX_OFFSET_SIZE>::MAX),
    };

    db.Write(txindex::DBKey{target_prefix, fake_pos}, std::array<std::byte, 0>{});
    db.Write(txindex::DBKey{target_prefix, unreadable_pos}, std::array<std::byte, 0>{});

    // The target's bucket now holds the forged false positives first (higher
    // sequence number), then the real target.
    const auto target_bucket{BucketPositions(db, target_prefix)};
    BOOST_REQUIRE_EQUAL(target_bucket.size(), 3U);
    BOOST_CHECK(target_bucket[0] == fake_pos);
    BOOST_CHECK(target_bucket[1] == unreadable_pos);
    BOOST_CHECK(target_bucket[2] != fake_pos);

    LookupTx(txindex, target_txid);

    // A database created fresh by this version cannot contain legacy entries, so
    // lookups skip the legacy fallback: drop the last coinbase's hashed entry and
    // re-add it under the old 't' + txid schema (a physical CDiskTxPos), then
    // confirm the lookup misses even though the legacy row exists.
    // BlockTxPosition offsets are from the block start (header included), while
    // the legacy CDiskTxPos.nTxOffset is measured after the header.
    const CDiskTxPos fake_physical{BlockFilePos(*m_node.chainman, fake_pos.block_seq + 1), fake_pos.tx_offset_in_block - static_cast<uint32_t>(GetSerializeSize(CBlockHeader{}))};
    db.Erase(txindex::DBKey{fake_prefix, fake_pos});
    db.Write(txindex::LegacyTxKey(fake_txid), fake_physical);
    CTransactionRef legacy_tx{m_coinbase_txns.front()};
    uint256 block_hash{uint256::ONE};
    BOOST_CHECK(!txindex.FindTx(fake_txid, block_hash, legacy_tx));
    BOOST_CHECK(legacy_tx == m_coinbase_txns.front());
    BOOST_CHECK(block_hash == uint256::ONE);

    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_legacy_fallback, TestChain100Setup)
{
    // Seed the on-disk database with a legacy ('t' + txid) entry before the index
    // is opened, as if it had been written by a pre-hashing version.
    const Txid legacy_txid{m_coinbase_txns.front()->GetHash()};
    // The block at height 1 holds only the coinbase, so the tx starts right after
    // the header and the 1-byte tx count.
    const CDiskTxPos legacy_pos{BlockFilePos(*m_node.chainman, 1), 1};
    {
        CDBWrapper db{DBParams{.path = gArgs.GetDataDirNet() / "indexes" / "txindex", .cache_bytes = 1_MiB}};
        db.Write(txindex::LegacyTxKey(legacy_txid), legacy_pos);
    }

    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    // Drop the hashed entries so only the legacy row remains, then confirm the
    // lookup succeeds through the fallback.
    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const auto prefix{txindex::CreateKeyPrefix(ReadHasher(db), legacy_txid)};
    const auto bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE(!bucket.empty());
    for (const auto& pos : bucket) db.Erase(txindex::DBKey{prefix, pos});

    LookupTx(txindex, legacy_txid);

    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_missing_sequence_mapping, TestChain100Setup)
{
    const Txid legacy_txid{m_coinbase_txns.front()->GetHash()};
    const auto legacy_key{txindex::LegacyTxKey(legacy_txid)};
    {
        CDBWrapper db{DBParams{.path = gArgs.GetDataDirNet() / "indexes" / "txindex", .cache_bytes = 1_MiB}};
        db.Write(legacy_key, CDiskTxPos{BlockFilePos(*m_node.chainman, 1), 1});
    }

    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const auto prefix{txindex::CreateKeyPrefix(ReadHasher(db), legacy_txid)};
    const auto bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE_EQUAL(bucket.size(), 1U);
    const txindex::DBKey hashed_key{prefix, bucket.front()};

    db.Erase(hashed_key);
    LookupTx(txindex, legacy_txid);
    db.Write(hashed_key, std::array<std::byte, 0>{});
    db.Erase(txindex::BlockSeqKey{bucket.front().block_seq});

    CTransactionRef tx;
    uint256 block_hash;
    BOOST_CHECK(!txindex.FindTx(legacy_txid, block_hash, tx));

    txindex.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
