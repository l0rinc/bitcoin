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

std::unique_ptr<TxIndex> g_txindex;

namespace {
SipHasher13UJ ReadOrCreateTxidHasher(CDBWrapper& db)
{
    std::pair<uint64_t, uint64_t> salt;
    if (!db.Read(txindex::DB_TXID_HASH_SALT, salt)) {
        FastRandomContext rng{};
        salt = {rng.rand64(), rng.rand64()};
        db.Write(txindex::DB_TXID_HASH_SALT, salt, /*fSync=*/true);
    }
    return SipHasher13UJ{salt.first, salt.second};
}
} // namespace

/** Access to the txindex database (indexes/txindex/) */
class TxIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /// Write a block of transaction positions to the DB.
    void WriteTxs(const interfaces::BlockInfo& block);

    /// Used to hash the txid to compute the prefix.
    const SipHasher13UJ m_hasher;

    void WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator) override;
};

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "txindex", n_cache_size, f_memory, f_wipe),
    m_hasher{ReadOrCreateTxidHasher(*this)}
{}

void TxIndex::DB::WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator)
{
    batch.Write(DB_BEST_BLOCK_V2, locator);
}

void TxIndex::DB::WriteTxs(const interfaces::BlockInfo& block)
{
    if (Exists(txindex::BlockHashKey{block.hash})) return; // Keep its sequence when a block reconnects

    uint32_t block_seq{0};
    Read(txindex::DB_NEXT_BLOCK_SEQ, block_seq);

    CDBBatch batch(*this);
    batch.Write(txindex::BlockHashKey{block.hash}, block_seq);
    batch.Write(txindex::BlockSeqKey{block_seq}, block.hash);
    batch.Write(txindex::DB_NEXT_BLOCK_SEQ, block_seq + 1);
    uint32_t tx_offset_in_block{static_cast<uint32_t>(GetSerializeSize(CBlockHeader{})) + GetSizeOfCompactSize(block.data->vtx.size())};
    for (const auto& tx : block.data->vtx) {
        const txindex::DBKey key{txindex::CreateKeyPrefix(m_hasher, tx->GetHash()),
                                 txindex::BlockTxPosition{block_seq, tx_offset_in_block}};
        // The tx position is encoded in the key, so the value is intentionally
        // empty. A 0-length byte array avoids the spurious '\0' that "" would store.
        batch.Write(key, std::array<std::byte, 0>{});
        if (&tx != &block.data->vtx.back()) tx_offset_in_block += tx->ComputeTotalSize();
    }
    WriteBatch(batch);
}

TxIndex::TxIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "txindex", "txidx"), m_db(std::make_unique<TxIndex::DB>(n_cache_size, f_memory, f_wipe))
{}

TxIndex::~TxIndex() = default;

bool TxIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    // Exclude genesis block transaction because outputs are not spendable.
    if (block.height == 0) return true;

    assert(block.data);
    m_db->WriteTxs(block);
    return true;
}

BaseIndex::DB& TxIndex::GetDB() const { return *m_db; }

std::optional<bool> TxIndex::FindHashedTx(const Txid& tx_hash, uint256& block_hash, CTransactionRef& tx) const
{
    const txindex::TxHashKeyPrefix prefix{txindex::CreateKeyPrefix(m_db->m_hasher, tx_hash)};
    std::unique_ptr<CDBIterator> it{m_db->NewIterator()};
    it->Seek(std::pair{txindex::DB_TXINDEX_HASHED, prefix});
    txindex::DBKey key{prefix, {}};
    CTransactionRef stale_tx;
    uint256 stale_block_hash;
    for (; it->Valid() && it->GetKey(key) && key.hash_prefix == prefix; it->Next()) {
        uint256 seq_block_hash;
        if (!m_db->Read(txindex::BlockSeqKey{key.pos.block_seq}, seq_block_hash)) {
            LogError("Block sequence %u not found for txid %s", key.pos.block_seq, tx_hash.ToString());
            return false;
        }
        FlatFilePos tx_position;
        bool in_active_chain;
        {
            LOCK(cs_main);
            const CBlockIndex* block_index{m_chainstate->m_blockman.LookupBlockIndex(seq_block_hash)};
            if (!block_index) {
                LogError("Block index entry %s not found for txid %s", seq_block_hash.ToString(), tx_hash.ToString());
                return false;
            }
            if (!(block_index->nStatus & BLOCK_HAVE_DATA)) continue;
            tx_position = {block_index->nFile, block_index->nDataPos + key.pos.tx_offset_in_block};
            in_active_chain = m_chainstate->m_chain.Contains(*block_index);
        }
        if (!in_active_chain && stale_tx) continue;

        AutoFile file{m_chainstate->m_blockman.OpenBlockFile(tx_position, /*fReadOnly=*/true)};
        if (file.IsNull()) {
            LogWarning("OpenBlockFile failed");
            continue;
        }
        CTransactionRef candidate_tx;
        try {
            file >> TX_WITH_WITNESS(candidate_tx);
        } catch (const std::exception& e) {
            LogWarning("Deserialize or I/O error - %s", e.what());
            continue;
        }
        if (candidate_tx->GetHash() != tx_hash) continue;

        if (in_active_chain) {
            tx = std::move(candidate_tx);
            block_hash = seq_block_hash;
            return true;
        }
        stale_tx = std::move(candidate_tx);
        stale_block_hash = seq_block_hash;
    }

    if (stale_tx) {
        tx = std::move(stale_tx);
        block_hash = stale_block_hash;
        return true;
    }
    return std::nullopt;
}

bool TxIndex::FindLegacyTx(const Txid& tx_hash, uint256& block_hash, CTransactionRef& tx) const
{
    CDiskTxPos postx;
    if (!m_db->Read(txindex::LegacyTxKey(tx_hash), postx)) {
        return false;
    }

    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(postx, true)};
    if (file.IsNull()) {
        LogError("OpenBlockFile failed");
        return false;
    }
    CBlockHeader header;
    CTransactionRef candidate_tx;
    try {
        file >> header;
        file.seek(postx.nTxOffset, SEEK_CUR);
        file >> TX_WITH_WITNESS(candidate_tx);
    } catch (const std::exception& e) {
        LogError("Deserialize or I/O error - %s", e.what());
        return false;
    }
    if (candidate_tx->GetHash() != tx_hash) {
        LogError("txid mismatch");
        return false;
    }
    tx = std::move(candidate_tx);
    block_hash = header.GetHash();
    return true;
}

bool TxIndex::FindTx(const Txid& tx_hash, uint256& block_hash, CTransactionRef& tx) const
{
    if (auto result{FindHashedTx(tx_hash, block_hash, tx)}) return *result;

    // Fall back to legacy if no hashed entry matched. This makes misses pay an
    // extra lookup, but keeps existing full-txid entries readable after upgrade.
    return FindLegacyTx(tx_hash, block_hash, tx);
}
