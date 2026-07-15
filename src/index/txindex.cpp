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
#include <node/blockfetch.h>
#include <node/blockstorage.h>
#include <node/interface_ui.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/log.h>
#include <util/translation.h>
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
    DB(size_t n_cache_size, bool f_memory, bool f_wipe, bool has_legacy);
};

static fs::path TxIndexDBPath() { return gArgs.GetDataDirNet() / "indexes" / "txindex"; }

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    // Enable bloom filters only if legacy entries are present (they are point lookups)
    DB(n_cache_size, f_memory, f_wipe,
       /*has_legacy=*/!f_memory && !f_wipe && CDBWrapper::HasKeyStartingWith(TxIndexDBPath(), DB_TXINDEX))
{}

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe, bool has_legacy) :
    BaseIndex::DB(TxIndexDBPath(), n_cache_size, f_memory, f_wipe, /*f_obfuscate=*/false, /*f_bloom=*/has_legacy),
    m_hasher{[](CDBWrapper& db) {
        std::pair<uint64_t, uint64_t> salt;
        if (!db.Read(DB_TXID_HASH_SALT, salt)) {
            FastRandomContext rng{};
            salt = {rng.rand64(), rng.rand64()};
            db.Write(DB_TXID_HASH_SALT, salt, /*fSync=*/true);
        }
        return PresaltedSipHasher{salt.first, salt.second};
    }(*this)},
    m_has_legacy{has_legacy}
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

bool TxIndex::CustomInit(const std::optional<interfaces::BlockRef>&)
{
    if (m_chainstate->m_blockman.IsPruneMode() && m_db->m_has_legacy) {
        return InitError(Untranslated("Pruned txindex requires a recreated database without legacy entries. Delete indexes/txindex and restart to rebuild it."));
    }
    return true;
}

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

bool TxIndex::FindTx(const Txid& tx_hash, uint256& block_hash, CTransactionRef& tx, bool allow_block_fetch, std::shared_ptr<const CBlock>* block_data, bool allow_local_only) const
{
    tx.reset();
    if (block_data) block_data->reset();
    const txindex::TxHashKeyPrefix prefix{txindex::CreateKeyPrefix(m_db->m_hasher, tx_hash)};
    std::unique_ptr<CDBIterator> it{m_db->NewIterator()};
    it->Seek(prefix);
    txindex::DBKey key{prefix, {}};
    std::vector<const CBlockIndex*> missing_blocks;
    for (; it->Valid(); it->Next()) {
        if (!it->GetKey(key)) return false;
        if (key.hash_prefix != prefix) break;
        FlatFilePos tx_pos;
        const CBlockIndex* block_index;
        bool have_data;
        {
            LOCK(cs_main);
            block_index = m_chainstate->m_chain[key.pos.block_height];
            if (!block_index) continue;
            have_data = (block_index->nStatus & BLOCK_HAVE_DATA) != 0 &&
                        (allow_local_only || !(block_index->nStatus & BLOCK_LOCAL_ONLY));
            if (have_data) tx_pos = FlatFilePos{block_index->nFile, block_index->nDataPos + key.pos.tx_offset_in_block};
        }
        if (!have_data) {
            if (allow_block_fetch && (missing_blocks.empty() || missing_blocks.back() != block_index)) {
                missing_blocks.push_back(block_index);
            }
            continue;
        }
        AutoFile file{m_chainstate->m_blockman.OpenBlockFile(tx_pos, /*fReadOnly=*/true)};
        if (file.IsNull()) {
            // Pruning may have raced the file open. Treat a still-present
            // block as an I/O failure rather than hiding corruption behind a fetch.
            if (WITH_LOCK(cs_main, return (block_index->nStatus & BLOCK_HAVE_DATA) != 0)) {
                LogError("OpenBlockFile failed");
                return false;
            }
            if (allow_block_fetch && (missing_blocks.empty() || missing_blocks.back() != block_index)) {
                missing_blocks.push_back(block_index);
            }
            continue;
        }
        bool deserialized{false};
        std::string deserialize_error;
        for (uint32_t offset{0}; offset < txindex::BLOCK_TX_OFFSET_GRANULARITY; ++offset) {
            CTransactionRef candidate_tx;
            try {
                file.seek(tx_pos.nPos + offset, SEEK_SET);
                file >> TX_WITH_WITNESS(candidate_tx);
                deserialized = true;
            } catch (const std::exception& e) {
                // A reorg may temporarily leave an old-branch position
                // resolving through the new active block at this height, or
                // this may not be the transaction's exact byte offset.
                deserialize_error = e.what();
                continue;
            }
            if (candidate_tx->GetHash() == tx_hash) {
                // Within this local-data pass, BIP30 duplicates return the earliest block.
                // The legacy index returned the latest one instead,
                // as each write overwrote the position under the same txid key.
                tx = std::move(candidate_tx);
                block_hash = block_index->GetBlockHash();
                return true;
            }
        }
        if (!deserialized) LogDebug(BCLog::VALIDATION, "Skipping unreadable txindex candidate at height %d, offset %u: %s", key.pos.block_height, key.pos.tx_offset_in_block, deserialize_error);
    }

    // Avoid fetching a false-positive collision when another candidate can be
    // resolved locally. Only fetch unavailable blocks after the local pass.
    for (const CBlockIndex* block_index : missing_blocks) {
        auto block{node::ReadBlockForLocalUse(*m_chain->context(), *block_index)};
        if (!block) continue;
        for (const auto& candidate_tx : (*block)->vtx) {
            if (candidate_tx->GetHash() != tx_hash) continue;
            tx = candidate_tx;
            block_hash = block_index->GetBlockHash();
            if (block_data) *block_data = *block;
            return true;
        }
    }

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
    if (!allow_local_only) {
        LOCK(cs_main);
        const CBlockIndex* index{m_chainstate->m_blockman.LookupBlockIndex(block_hash)};
        if (index && (index->nStatus & BLOCK_LOCAL_ONLY)) {
            tx.reset();
            return false;
        }
    }
    return true;
}
