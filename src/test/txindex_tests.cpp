// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chain.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <crypto/siphash.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <index/disktxpos.h>
#include <index/txindex.h>
#include <index/txindex_key.h>
#include <interfaces/chain.h>
#include <key.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

BOOST_AUTO_TEST_SUITE(txindex_tests)

// Grants tests access to the otherwise non-public txindex database handle.
class TxIndexTest
{
public:
    static CDBWrapper& GetDB(const TxIndex& txindex) { return static_cast<CDBWrapper&>(txindex.GetDB()); }
};

namespace {

PresaltedSipHasher ReadHasher(const CDBWrapper& db)
{
    std::pair<uint64_t, uint64_t> salt;
    BOOST_REQUIRE(db.Read(std::string{"txid_hash_salt"}, salt));
    return PresaltedSipHasher{salt.first, salt.second};
}

std::vector<txindex::BlockTxPosition> BucketPositions(CDBWrapper& db, const txindex::TxHashKeyPrefix& prefix)
{
    std::vector<txindex::BlockTxPosition> positions;
    std::unique_ptr<CDBIterator> it{db.NewIterator()};
    txindex::DBKey key{prefix, {}};
    for (it->Seek(prefix); it->Valid() && it->GetKey(key) && key.hash_prefix == prefix; it->Next()) {
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

void CheckPositionEncoding(const std::vector<uint32_t>& tx_sizes, uint32_t height, const txindex::TxHashKeyPrefix& prefix)
{
    std::vector<uint32_t> tx_offsets;
    uint32_t tx_offset{static_cast<uint32_t>(GetSerializeSize(CBlockHeader{})) + GetSizeOfCompactSize(tx_sizes.size())};
    for (const uint32_t tx_size : tx_sizes) {
        tx_offsets.push_back(tx_offset);
        tx_offset += tx_size;
    }
    BOOST_REQUIRE_LE(tx_offset, MAX_BLOCK_SERIALIZED_SIZE);

    DataStream prefix_stream;
    prefix_stream << prefix;
    BOOST_CHECK_EQUAL(prefix_stream.size(), txindex::TxHashKeyPrefix::SERIALIZED_SIZE);

    for (const uint32_t expected_offset : tx_offsets) {
        DataStream stream;
        stream << txindex::DBKey{prefix, txindex::BlockTxPosition{height, expected_offset}};
        BOOST_CHECK_EQUAL(stream.size(), txindex::DBKey::SERIALIZED_SIZE);

        txindex::DBKey decoded{prefix, {}};
        stream >> decoded;
        BOOST_CHECK(stream.empty());
        BOOST_CHECK(decoded.hash_prefix == prefix);
        BOOST_CHECK_EQUAL(decoded.pos.block_height, height);

        const auto match{std::lower_bound(tx_offsets.begin(), tx_offsets.end(), decoded.pos.tx_offset_in_block)};
        BOOST_REQUIRE(match != tx_offsets.end());
        BOOST_CHECK(decoded.pos.ContainsOffset(*match));
        BOOST_CHECK_EQUAL(*match, expected_offset);
        BOOST_CHECK(std::next(match) == tx_offsets.end() || !decoded.pos.ContainsOffset(*std::next(match)));
    }
}

void CheckRandomPositionEncoding(uint32_t generate_attempts, uint32_t shuffle_attempts)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    const PresaltedSipHasher hasher{rng.rand64(), rng.rand64()};
    constexpr uint32_t min_tx_size{MIN_TRANSACTION_WEIGHT / WITNESS_SCALE_FACTOR};
    constexpr uint32_t typical_max_tx_size{10'000};

    for (uint32_t generate{0}; generate < generate_attempts; ++generate) {
        std::vector<uint32_t> tx_sizes;
        uint32_t remaining{MAX_BLOCK_SERIALIZED_SIZE - static_cast<uint32_t>(GetSerializeSize(CBlockHeader{})) - 9};
        const uint32_t generated_max_tx_size{rng.randbool() ? typical_max_tx_size : remaining};
        while (remaining >= min_tx_size) {
            const uint32_t max_tx_size{std::min(remaining, generated_max_tx_size)};
            const uint32_t tx_size{min_tx_size + rng.randrange(max_tx_size - min_tx_size + 1)};
            tx_sizes.push_back(tx_size);
            remaining -= tx_size;
        }

        for (uint32_t shuffle{0}; shuffle <= shuffle_attempts; ++shuffle) {
            const auto prefix{txindex::CreateKeyPrefix(hasher, Txid::FromUint256(rng.rand256()))};
            CheckPositionEncoding(tx_sizes, rng.randrange(txindex::MAX_TXINDEX_BLOCK_HEIGHT + 1), prefix);
            if (shuffle < shuffle_attempts) std::shuffle(tx_sizes.begin(), tx_sizes.end(), rng);
        }
    }
}

} // namespace

BOOST_AUTO_TEST_CASE(txindex_position_encoding)
{
    CheckRandomPositionEncoding(/*generate_attempts=*/10, /*shuffle_attempts=*/4);

    constexpr uint32_t min_tx_size{MIN_TRANSACTION_WEIGHT / WITNESS_SCALE_FACTOR};
    const uint32_t first_tx_size{MAX_BLOCK_SERIALIZED_SIZE - static_cast<uint32_t>(GetSerializeSize(CBlockHeader{})) - GetSizeOfCompactSize(2) - min_tx_size};
    for (uint8_t tag{txindex::DB_TXINDEX_HASHED}; tag < txindex::DB_TXINDEX_HASHED + txindex::DB_TXINDEX_HASHED_COUNT; ++tag) {
        CheckPositionEncoding({first_tx_size, min_tx_size}, txindex::MAX_TXINDEX_BLOCK_HEIGHT, txindex::TxHashKeyPrefix{tag});
    }

    DataStream stream;
    const txindex::DBKey overflow_key{txindex::TxHashKeyPrefix{}, txindex::BlockTxPosition{txindex::MAX_TXINDEX_BLOCK_HEIGHT + 1, 0}};
    BOOST_CHECK_THROW(stream << overflow_key, std::ios_base::failure);
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

BOOST_FIXTURE_TEST_CASE(txindex_collision_scan_path, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, true);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const PresaltedSipHasher hasher{ReadHasher(db)};

    // The first coinbase is at a lower height than the second, so its encoded
    // position sorts first if it shares the second's prefix. Forge a colliding
    // entry under the second coinbase's prefix pointing at the first coinbase, so
    // looking up the second tx must scan that false positive first.
    const Txid fake_txid{m_coinbase_txns.front()->GetHash()};
    const Txid target_txid{m_coinbase_txns.at(1)->GetHash()};
    const auto fake_prefix{txindex::CreateKeyPrefix(hasher, fake_txid)};
    const auto target_prefix{txindex::CreateKeyPrefix(hasher, target_txid)};
    // Distinct prefixes guarantee the target's bucket initially holds only the target.
    BOOST_REQUIRE(fake_prefix != target_prefix);

    // Read the first coinbase's encoded position straight from its bucket.
    const auto fake_bucket{BucketPositions(db, fake_prefix)};
    BOOST_REQUIRE_EQUAL(fake_bucket.size(), 1U);
    const txindex::BlockTxPosition fake_pos{fake_bucket.front()};

    // Model the short window during a reorg where an old-branch position is
    // resolved through the new active block at the same height. This offset is
    // within the serialized format but beyond the test block file.
    const txindex::BlockTxPosition stale_pos{0, MAX_BLOCK_SERIALIZED_SIZE - 1};
    db.Write(txindex::DBKey{target_prefix, stale_pos}, "");
    db.Write(txindex::DBKey{target_prefix, fake_pos}, "");

    // The target's bucket now holds the stale position and forged false positive
    // before the real target.
    const auto target_bucket{BucketPositions(db, target_prefix)};
    BOOST_REQUIRE_EQUAL(target_bucket.size(), 3U);
    BOOST_CHECK(target_bucket[0] == stale_pos);
    BOOST_CHECK(target_bucket[1] == fake_pos);
    BOOST_CHECK(target_bucket[2] != fake_pos);

    CTransactionRef tx_disk;
    uint256 block_hash;
    BOOST_REQUIRE(txindex.FindTx(target_txid, block_hash, tx_disk));
    BOOST_REQUIRE(tx_disk);
    BOOST_CHECK(tx_disk->GetHash() == target_txid);

    // A database created fresh by this version cannot contain legacy entries, so
    // lookups skip the legacy fallback: drop the first coinbase's hashed entry and
    // re-add it under the old 't' + txid schema (a physical CDiskTxPos), then
    // confirm the lookup misses even though the legacy row exists.
    // BlockTxPosition offsets are from the block start (header included), while
    // the legacy CDiskTxPos.nTxOffset is measured after the header.
    const CDiskTxPos fake_physical{BlockFilePos(*m_node.chainman, fake_pos.block_height), static_cast<uint32_t>(GetSizeOfCompactSize(1))};
    db.Erase(txindex::DBKey{fake_prefix, fake_pos});
    db.Write(std::make_pair(static_cast<uint8_t>('t'), fake_txid.ToUint256()), fake_physical);
    CTransactionRef legacy_tx;
    BOOST_CHECK(!txindex.FindTx(fake_txid, block_hash, legacy_tx));

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
        db.Write(std::make_pair(static_cast<uint8_t>('t'), legacy_txid.ToUint256()), legacy_pos);
    }

    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, /*f_memory=*/false);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    // Drop the hashed entries so only the legacy row remains, then confirm the
    // lookup succeeds through the fallback.
    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const auto prefix{txindex::CreateKeyPrefix(ReadHasher(db), legacy_txid)};
    const auto bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE(!bucket.empty());
    for (const auto& pos : bucket) db.Erase(txindex::DBKey{prefix, pos});

    CTransactionRef tx_disk;
    uint256 block_hash;

    // A malformed hashed row in this bucket must fail closed instead of
    // falling through to the valid legacy row.
    const auto malformed_key{prefix};
    db.Write(malformed_key, std::array<std::byte, 0>{});
    BOOST_CHECK(!txindex.FindTx(legacy_txid, block_hash, tx_disk));
    BOOST_CHECK(!tx_disk);
    db.Erase(malformed_key);

    // CDBIterator accepts trailing bytes after a deserialized key, so also
    // cover an overlong hashed row before falling back to the legacy entry.
    const auto overlong_key{std::pair{txindex::DBKey{prefix, bucket.front()}, uint8_t{0}}};
    db.Write(overlong_key, std::array<std::byte, 0>{});
    BOOST_CHECK(!txindex.FindTx(legacy_txid, block_hash, tx_disk));
    BOOST_CHECK(!tx_disk);
    db.Erase(overlong_key);

    BOOST_REQUIRE(txindex.FindTx(legacy_txid, block_hash, tx_disk));
    BOOST_REQUIRE(tx_disk);
    BOOST_CHECK(tx_disk->GetHash() == legacy_txid);

    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_reorg_erases_entries, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, true);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    const CScript coinbase_script{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};

    // Mine a unique (non-coinbase) transaction into a new block at height 101.
    CMutableTransaction unique_mtx{CreateValidMempoolTransaction(
        /*input_transaction=*/m_coinbase_txns[0],
        /*input_vout=*/0,
        /*input_height=*/1,
        /*input_signing_key=*/coinbaseKey,
        /*output_destination=*/CScript() << OP_TRUE,
        /*output_amount=*/CAmount{1 * COIN},
        /*submit=*/false)};
    const Txid unique_txid{MakeTransactionRef(unique_mtx)->GetHash()};
    CreateAndProcessBlock({unique_mtx}, coinbase_script);
    BOOST_REQUIRE(txindex.BlockUntilSyncedToCurrentChain());

    CTransactionRef tx_disk;
    uint256 block_hash;
    BOOST_REQUIRE(txindex.FindTx(unique_txid, block_hash, tx_disk));
    BOOST_CHECK(tx_disk->GetHash() == unique_txid);

    ChainstateManager& chainman{*m_node.chainman};

    // Invalidate the block holding the unique transaction, then mine a longer branch.
    {
        CBlockIndex* tip{WITH_LOCK(cs_main, return chainman.ActiveChain().Tip())};
        BlockValidationState state;
        BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, tip));
    }
    CreateAndProcessBlock({}, coinbase_script);
    CreateAndProcessBlock({}, coinbase_script);
    BOOST_REQUIRE(txindex.BlockUntilSyncedToCurrentChain());

    // The disconnected transaction's entry must have been erased.
    BOOST_CHECK(!txindex.FindTx(unique_txid, block_hash, tx_disk));

    txindex.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
