// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txindex.h>

#include <chain.h>
#include <common/args.h>
#include <crypto/siphash.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <index/base.h>
#include <index/disktxpos.h>
#include <index/txindex_key.h>
#include <interfaces/chain.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/log.h>
#include <validation.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <utility>
#include <vector>

constexpr uint8_t DB_TXINDEX{'t'};
const std::string DB_BEST_BLOCK_V2{"best_block_v2"};
static const std::string DB_TXID_HASH_SALT{"txid_hash_salt"};

std::unique_ptr<TxIndex> g_txindex;


/** Access to the txindex database (indexes/txindex/) */
class TxIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /// Read the legacy disk location of the transaction data with the given hash. Returns false if the
    /// transaction hash is not indexed.
    bool ReadTxPos(const Txid& txid, CDiskTxPos& pos) const;

    /// Write or erase a block of transaction positions to the DB.
    void WriteTxs(const interfaces::BlockInfo& block, bool erase = false);

    /// Used to hash the txid to compute the prefix.
    const PresaltedSipHasher m_hasher;

    /// Whether the database contains any legacy ('t' + txid) entries.
    const bool m_has_legacy;

    CBlockLocator ReadBestBlock() const override;
    void WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator) override;

private:
    DB(size_t n_cache_size, bool f_memory, bool f_wipe, bool f_obfuscate, bool f_bloom);
};

static fs::path TxIndexDBPath() { return gArgs.GetDataDirNet() / "indexes" / "txindex"; }

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    // Enable bloom filters only if legacy entries are present (they are point lookups)
    DB(n_cache_size, f_memory, f_wipe, /*f_obfuscate=*/false,
       /*f_bloom=*/!f_memory && !f_wipe && CDBWrapper::HasKeyStartingWith(TxIndexDBPath(), DB_TXINDEX))
{}

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe, bool f_obfuscate, bool f_bloom) :
    BaseIndex::DB(TxIndexDBPath(), n_cache_size, f_memory, f_wipe, f_obfuscate, f_bloom),
    m_hasher{[](CDBWrapper& db) {
        std::pair<uint64_t, uint64_t> salt;
        if (!db.Read(DB_TXID_HASH_SALT, salt)) {
            FastRandomContext rng{};
            salt = {rng.rand64(), rng.rand64()};
            db.Write(DB_TXID_HASH_SALT, salt, /*fSync=*/true);
        }
        return PresaltedSipHasher{salt.first, salt.second};
    }(*this)},
    m_has_legacy{f_bloom}
{}

bool TxIndex::DB::ReadTxPos(const Txid& txid, CDiskTxPos& pos) const
{
    return Read(std::make_pair(DB_TXINDEX, txid.ToUint256()), pos);
}

CBlockLocator TxIndex::DB::ReadBestBlock() const
{
    CBlockLocator locator;
    if (Read(DB_BEST_BLOCK_V2, locator)) {
        return locator;
    }
    // If we don't have a locator yet, start from the legacy best block.
    return BaseIndex::DB::ReadBestBlock();
}

void TxIndex::DB::WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator)
{
    batch.Write(DB_BEST_BLOCK_V2, locator);
}

void TxIndex::DB::WriteTxs(const interfaces::BlockInfo& block, bool erase)
{
    assert(block.data);
    CDBBatch batch(*this);
    uint32_t tx_offset_in_block{static_cast<uint32_t>(GetSerializeSize(CBlockHeader{})) + GetSizeOfCompactSize(block.data->vtx.size())};
    for (const auto& tx : block.data->vtx) {
        const txindex::DBKey key{txindex::CreateKeyPrefix(m_hasher, tx->GetHash()),
                                 txindex::BlockTxPosition{static_cast<uint32_t>(block.height), tx_offset_in_block}};
        if (erase) {
            batch.Erase(key);
        } else {
            // The tx position is encoded in the key, so the value is intentionally
            // empty. A 0-length byte array avoids the spurious '\0' that "" would store.
            batch.Write(key, std::array<std::byte, 0>{});
        }
        tx_offset_in_block += ::GetSerializeSize(TX_WITH_WITNESS(*tx));
    }
    WriteBatch(batch);
}

TxIndex::TxIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "txindex", "txidx"), m_db(std::make_unique<TxIndex::DB>(n_cache_size, f_memory, f_wipe))
{}

TxIndex::~TxIndex() = default;

interfaces::Chain::NotifyOptions TxIndex::CustomOptions()
{
    return {.disconnect_data = true};
}

bool TxIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    // Exclude genesis block transaction because outputs are not spendable.
    if (block.height == 0) return true;

    m_db->WriteTxs(block);
    return true;
}

bool TxIndex::CustomRemove(const interfaces::BlockInfo& block)
{
    assert(block.height > 0);
    m_db->WriteTxs(block, /*erase=*/true);
    return true;
}

BaseIndex::DB& TxIndex::GetDB() const { return *m_db; }

bool TxIndex::FindTx(const Txid& tx_hash, uint256& block_hash, CTransactionRef& tx) const
{
    const txindex::TxHashKeyPrefix prefix{txindex::CreateKeyPrefix(m_db->m_hasher, tx_hash)};
    std::unique_ptr<CDBIterator> it{m_db->NewIterator()};
    it->Seek(std::pair{txindex::DB_TXINDEX_HASHED, prefix});
    txindex::DBKey key{prefix, {}};
    for (; it->Valid(); it->Next()) {
        if (!it->GetKey(key)) return false;
        if (key.hash_prefix != prefix) break;
        FlatFilePos tx_pos;
        const CBlockIndex* block_index;
        {
            LOCK(cs_main);
            block_index = m_chainstate->m_chain[key.pos.block_height];
            if (!block_index) continue;
            tx_pos = FlatFilePos{block_index->nFile, block_index->nDataPos + key.pos.tx_offset_in_block};
        }
        AutoFile file{m_chainstate->m_blockman.OpenBlockFile(tx_pos, /*fReadOnly=*/true)};
        if (file.IsNull()) {
            LogError("OpenBlockFile failed");
            return false;
        }
        try {
            file >> TX_WITH_WITNESS(tx);
        } catch (const std::exception& e) {
            LogError("Deserialize or I/O error - %s", e.what());
            return false;
        }
        if (tx->GetHash() == tx_hash) {
            // For BIP30 duplicate txs, this returns the earliest block.
            // The legacy index returned the latest one instead,
            // as each write overwrote the position under the same txid key.
            block_hash = block_index->GetBlockHash();
            return true;
        }
    }
    tx.reset();
    if (!m_db->m_has_legacy) return false;
    // Fall back to legacy if no hashed entry matched. This makes misses pay an
    // extra lookup, but keeps existing full-txid entries readable after upgrade.
    CDiskTxPos postx;
    if (!m_db->ReadTxPos(tx_hash, postx)) {
        return false;
    }

    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(postx, true)};
    if (file.IsNull()) {
        LogError("OpenBlockFile failed");
        return false;
    }
    CBlockHeader header;
    try {
        file >> header;
        file.seek(postx.nTxOffset, SEEK_CUR);
        file >> TX_WITH_WITNESS(tx);
    } catch (const std::exception& e) {
        LogError("Deserialize or I/O error - %s", e.what());
        return false;
    }
    if (tx->GetHash() != tx_hash) {
        LogError("txid mismatch");
        return false;
    }
    block_hash = header.GetHash();
    return true;
}
