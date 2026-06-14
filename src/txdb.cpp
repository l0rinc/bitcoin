// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txdb.h>

#include <coins.h>
#include <dbwrapper.h>
#include <logging/timer.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/log.h>
#include <util/threadnames.h>
#include <util/vector.h>

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <future>
#include <iterator>
#include <sstream>
#include <system_error>
#include <thread>
#include <utility>

static constexpr uint8_t DB_COIN{'C'};
static constexpr uint8_t DB_BEST_BLOCK{'B'};
static constexpr uint8_t DB_HEAD_BLOCKS{'H'};
// Keys used in previous version that might still be found in the DB:
static constexpr uint8_t DB_COINS{'c'};

// Threshold for warning when writing this many dirty cache entries to disk.
static constexpr size_t WARN_FLUSH_COINS_COUNT{10'000'000};

bool CCoinsViewDB::NeedsUpgrade()
{
    std::unique_ptr<CDBIterator> cursor{m_db->NewIterator()};
    // DB_COINS was deprecated in v0.15.0, commit
    // 1088b02f0ccd7358d2b7076bb9e122d59d502d02
    cursor->Seek(std::make_pair(DB_COINS, uint256{}));
    return cursor->Valid();
}

namespace {

struct CoinEntry {
    COutPoint* outpoint;
    uint8_t key{DB_COIN};
    explicit CoinEntry(const COutPoint* ptr) : outpoint(const_cast<COutPoint*>(ptr)) {}

    SERIALIZE_METHODS(CoinEntry, obj) { READWRITE(obj.key, obj.outpoint->hash, VARINT(obj.outpoint->n)); }
};

struct LevelDBCompactionIO {
    uint64_t read_bytes{0};
    uint64_t write_bytes{0};
};

uintmax_t DirectorySize(const fs::path& path)
{
    uintmax_t size{0};
    std::error_code ec;
    fs::recursive_directory_iterator it{path, fs::directory_options::skip_permission_denied, ec};
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        const fs::directory_entry& entry{*it};
        std::error_code entry_ec;
        if (entry.is_regular_file(entry_ec)) {
            const uintmax_t file_size{entry.file_size(entry_ec)};
            if (!entry_ec) size += file_size;
        }
        it.increment(ec);
    }
    return size;
}

LevelDBCompactionIO LevelDBCompactionStats(const CDBWrapper& db)
{
    LevelDBCompactionIO result;
    if (const auto stats{db.GetProperty("leveldb.stats")}) {
        std::istringstream stream{*stats};
        for (std::string line; std::getline(stream, line);) {
            std::istringstream line_stream{line};
            int level;
            int files;
            double size_mib;
            double time_seconds;
            double read_mib;
            double write_mib;
            if (line_stream >> level >> files >> size_mib >> time_seconds >> read_mib >> write_mib) {
                result.read_bytes += static_cast<uint64_t>(read_mib * 1_MiB);
                result.write_bytes += static_cast<uint64_t>(write_mib * 1_MiB);
            }
        }
    }
    return result;
}

LevelDBCompactionIO Delta(const LevelDBCompactionIO& before, const LevelDBCompactionIO& after)
{
    return {
        .read_bytes = after.read_bytes > before.read_bytes ? after.read_bytes - before.read_bytes : 0,
        .write_bytes = after.write_bytes > before.write_bytes ? after.write_bytes - before.write_bytes : 0,
    };
}

std::string LevelDBLevels(const CDBWrapper& db)
{
    std::array<std::string, 7> levels{};
    if (const auto stats{db.GetProperty("leveldb.stats")}) {
        std::istringstream stream{*stats};
        for (std::string line; std::getline(stream, line);) {
            std::istringstream line_stream{line};
            int level;
            int files;
            double size_mib;
            if (line_stream >> level >> files >> size_mib && level >= 0 && static_cast<size_t>(level) < levels.size()) {
                levels[level] = strprintf("%d:%d:%.0f", level, files, size_mib);
            }
        }
    }

    std::ostringstream result;
    for (size_t level{0}; level < levels.size(); ++level) {
        if (level != 0) result << ',';
        result << (levels[level].empty() ? strprintf("%d:0:0", level) : levels[level]);
    }
    return result.str();
}

void LogCompactionSnapshot(const char* event, const CDBWrapper& db, const fs::path& path, int height, bool in_ibd, int64_t elapsed_ms, uintmax_t baseline_disk_bytes, uintmax_t disk_bytes, uintmax_t peak_disk_bytes, const LevelDBCompactionIO& io_delta)
{
    LogInfo("CHAINSTATE_COMPACTION_SNAPSHOT event=%s height=%d in_ibd=%d elapsed_ms=%d baseline_disk_bytes=%u disk_bytes=%u peak_disk_bytes=%u peak_extra_bytes=%u compaction_read_bytes=%u compaction_write_bytes=%u levels=%s path=%s",
            event, height, in_ibd, elapsed_ms, baseline_disk_bytes, disk_bytes, peak_disk_bytes, peak_disk_bytes > baseline_disk_bytes ? peak_disk_bytes - baseline_disk_bytes : 0, io_delta.read_bytes, io_delta.write_bytes, LevelDBLevels(db), fs::PathToString(path));
}

} // namespace

CCoinsViewDB::CCoinsViewDB(DBParams db_params, CoinsViewOptions options) :
    m_db_params{std::move(db_params)},
    m_options{std::move(options)},
    m_db{std::make_unique<CDBWrapper>(m_db_params)} { }

CCoinsViewDB::~CCoinsViewDB()
{
    if (m_compaction.valid()) {
        if (m_compaction.wait_for(std::chrono::seconds{0}) != std::future_status::ready) {
            LogInfo("Waiting for background chainstate compaction of %s", fs::PathToString(m_db_params.path));
        }
        m_compaction.wait();
    }
}

void CCoinsViewDB::ResizeCache(size_t new_cache_size)
{
    // We can't do this operation with an in-memory DB since we'll lose all the coins upon
    // reset.
    if (!m_db_params.memory_only) {
        LOCK(m_db_mutex);
        // Have to do a reset first to get the original `m_db` state to release its
        // filesystem lock.
        m_db.reset();
        m_db_params.cache_bytes = new_cache_size;
        m_db_params.wipe_data = false;
        m_db = std::make_unique<CDBWrapper>(m_db_params);
    }
}

std::optional<Coin> CCoinsViewDB::GetCoin(const COutPoint& outpoint) const
{
    if (Coin coin; m_db->Read(CoinEntry(&outpoint), coin)) {
        Assert(!coin.IsSpent()); // The UTXO database should never contain spent coins
        return coin;
    }
    return std::nullopt;
}

std::optional<Coin> CCoinsViewDB::PeekCoin(const COutPoint& outpoint) const
{
    return GetCoin(outpoint);
}

bool CCoinsViewDB::HaveCoin(const COutPoint& outpoint) const
{
    return m_db->Exists(CoinEntry(&outpoint));
}

uint256 CCoinsViewDB::GetBestBlock() const {
    uint256 hashBestChain;
    if (!m_db->Read(DB_BEST_BLOCK, hashBestChain))
        return uint256();
    return hashBestChain;
}

std::vector<uint256> CCoinsViewDB::GetHeadBlocks() const {
    std::vector<uint256> vhashHeadBlocks;
    if (!m_db->Read(DB_HEAD_BLOCKS, vhashHeadBlocks)) {
        return std::vector<uint256>();
    }
    return vhashHeadBlocks;
}

void CCoinsViewDB::BatchWrite(CoinsViewCacheCursor& cursor, const uint256& block_hash)
{
    CDBBatch batch(*m_db);
    size_t count = 0;
    const size_t dirty_count{cursor.GetDirtyCount()};
    assert(!block_hash.IsNull());

    uint256 old_tip = GetBestBlock();
    if (old_tip.IsNull()) {
        // We may be in the middle of replaying.
        std::vector<uint256> old_heads = GetHeadBlocks();
        if (old_heads.size() == 2) {
            if (old_heads[0] != block_hash) {
                LogError("The coins database detected an inconsistent state, likely due to a previous crash or shutdown. You will need to restart bitcoind with the -reindex-chainstate or -reindex configuration option.\n");
            }
            assert(old_heads[0] == block_hash);
            old_tip = old_heads[1];
        }
    }

    if (dirty_count > WARN_FLUSH_COINS_COUNT) LogWarning("Flushing large (%d entries) UTXO set to disk, it may take several minutes", dirty_count);
    LOG_TIME_MILLIS_WITH_CATEGORY(strprintf("write coins cache to disk (%d out of %d cached coins)",
        dirty_count, cursor.GetTotalCount()), BCLog::BENCH);

    // In the first batch, mark the database as being in the middle of a
    // transition from old_tip to block_hash.
    // A vector is used for future extensibility, as we may want to support
    // interrupting after partial writes from multiple independent reorgs.
    batch.Erase(DB_BEST_BLOCK);
    batch.Write(DB_HEAD_BLOCKS, Vector(block_hash, old_tip));

    for (auto it{cursor.Begin()}; it != cursor.End();) {
        if (it->second.IsDirty()) {
            CoinEntry entry(&it->first);
            if (it->second.coin.IsSpent()) {
                batch.Erase(entry);
            } else {
                batch.Write(entry, it->second.coin);
            }
        }
        count++;
        it = cursor.NextAndMaybeErase(*it);
        if (batch.ApproximateSize() > m_options.batch_write_bytes) {
            LogDebug(BCLog::COINDB, "Writing partial batch of %.2f MiB\n", batch.ApproximateSize() / double(1_MiB));

            m_db->WriteBatch(batch);
            batch.Clear();
            if (m_options.simulate_crash_ratio) {
                static FastRandomContext rng;
                if (rng.randrange(m_options.simulate_crash_ratio) == 0) {
                    LogError("Simulating a crash. Goodbye.");
                    _Exit(0);
                }
            }
        }
    }

    // In the last batch, mark the database as consistent with block_hash again.
    batch.Erase(DB_HEAD_BLOCKS);
    batch.Write(DB_BEST_BLOCK, block_hash);

    LogDebug(BCLog::COINDB, "Writing final batch of %.2f MiB\n", batch.ApproximateSize() / double(1_MiB));
    m_db->WriteBatch(batch);
    LogDebug(BCLog::COINDB, "Committed %u changed transaction outputs (out of %u) to coin database...", (unsigned int)dirty_count, (unsigned int)count);
    LogInfo("CHAINSTATE_FLUSH_SNAPSHOT disk_bytes=%u levels=%s path=%s", DirectorySize(m_db_params.path), LevelDBLevels(*m_db), fs::PathToString(m_db_params.path));
}

size_t CCoinsViewDB::EstimateSize() const
{
    return m_db->EstimateSize(DB_COIN, uint8_t(DB_COIN + 1));
}

std::optional<std::string> CCoinsViewDB::GetDBProperty(const std::string& property)
{
    return m_db->GetProperty(property);
}

std::shared_future<void> CCoinsViewDB::CompactFull(int height, bool in_ibd)
{
    AssertLockHeld(::cs_main);
    if (m_compaction.valid() && m_compaction.wait_for(std::chrono::seconds{0}) != std::future_status::ready) return m_compaction;
    m_compaction = std::async(std::launch::async, [this, height, in_ibd] {
        try {
            util::ThreadRename("utxocompact");
            LOCK(m_db_mutex);

            const auto start{std::chrono::steady_clock::now()};
            const uintmax_t before_disk_bytes{DirectorySize(m_db_params.path)};
            const LevelDBCompactionIO before_io{LevelDBCompactionStats(*m_db)};
            std::atomic<uintmax_t> peak_disk_bytes{before_disk_bytes};
            LogCompactionSnapshot("before", *m_db, m_db_params.path, height, in_ibd, 0, before_disk_bytes, before_disk_bytes, before_disk_bytes, {});

            {
                std::jthread disk_sampler{[this, &peak_disk_bytes](std::stop_token stop_token) {
                    while (!stop_token.stop_requested()) {
                        const uintmax_t disk_bytes{DirectorySize(m_db_params.path)};
                        uintmax_t previous_peak{peak_disk_bytes.load()};
                        while (disk_bytes > previous_peak && !peak_disk_bytes.compare_exchange_weak(previous_peak, disk_bytes)) {}
                        std::this_thread::sleep_for(std::chrono::seconds{1});
                    }
                }};
                LogDebug(BCLog::COINDB, "Starting chainstate compaction of %s", fs::PathToString(m_db_params.path));
                m_db->CompactFull();
                LogDebug(BCLog::COINDB, "Finished chainstate compaction of %s", fs::PathToString(m_db_params.path));
                disk_sampler.request_stop();
            }

            const uintmax_t after_disk_bytes{DirectorySize(m_db_params.path)};
            const LevelDBCompactionIO io_delta{Delta(before_io, LevelDBCompactionStats(*m_db))};
            if (after_disk_bytes > peak_disk_bytes.load()) peak_disk_bytes = after_disk_bytes;
            LogCompactionSnapshot("after", *m_db, m_db_params.path, height, in_ibd, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count(), before_disk_bytes, after_disk_bytes, peak_disk_bytes.load(), io_delta);
        } catch (const std::exception& e) {
            LogWarning("Failed chainstate compaction (%s)", e.what());
        }
    }).share();
    return m_compaction;
}

/** Specialization of CCoinsViewCursor to iterate over a CCoinsViewDB */
class CCoinsViewDBCursor: public CCoinsViewCursor
{
public:
    // Prefer using CCoinsViewDB::Cursor() since we want to perform some
    // cache warmup on instantiation.
    CCoinsViewDBCursor(CDBIterator* pcursorIn, const uint256& in_block_hash):
        CCoinsViewCursor(in_block_hash), pcursor(pcursorIn) {}
    ~CCoinsViewDBCursor() = default;

    bool GetKey(COutPoint &key) const override;
    bool GetValue(Coin &coin) const override;

    bool Valid() const override;
    void Next() override;

private:
    std::unique_ptr<CDBIterator> pcursor;
    std::pair<char, COutPoint> keyTmp;

    friend class CCoinsViewDB;
};

std::unique_ptr<CCoinsViewCursor> CCoinsViewDB::Cursor() const
{
    auto i = std::make_unique<CCoinsViewDBCursor>(
        const_cast<CDBWrapper&>(*m_db).NewIterator(), GetBestBlock());
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    i->pcursor->Seek(DB_COIN);
    // Cache key of first record
    if (i->pcursor->Valid()) {
        CoinEntry entry(&i->keyTmp.second);
        i->pcursor->GetKey(entry);
        i->keyTmp.first = entry.key;
    } else {
        i->keyTmp.first = 0; // Make sure Valid() and GetKey() return false
    }
    return i;
}

bool CCoinsViewDBCursor::GetKey(COutPoint &key) const
{
    // Return cached key
    if (keyTmp.first == DB_COIN) {
        key = keyTmp.second;
        return true;
    }
    return false;
}

bool CCoinsViewDBCursor::GetValue(Coin &coin) const
{
    return pcursor->GetValue(coin);
}

bool CCoinsViewDBCursor::Valid() const
{
    return keyTmp.first == DB_COIN;
}

void CCoinsViewDBCursor::Next()
{
    pcursor->Next();
    CoinEntry entry(&keyTmp.second);
    if (!pcursor->Valid() || !pcursor->GetKey(entry)) {
        keyTmp.first = 0; // Invalidate cached key after last record so that Valid() and GetKey() return false
    } else {
        keyTmp.first = entry.key;
    }
}
