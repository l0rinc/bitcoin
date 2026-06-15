// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dbwrapper.h>

#include <rocksdb/cache.h>
#include <rocksdb/db.h>
#include <rocksdb/env.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>
#include <rocksdb/table.h>
#include <rocksdb/write_batch.h>
#include <random.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <util/byte_units.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/log.h>
#include <util/obfuscation.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

static auto CharCast(const std::byte* data) { return reinterpret_cast<const char*>(data); }
static std::span<const std::byte> ToByteSpan(const rocksdb::Slice& slice) { return std::as_bytes(std::span{slice.data(), slice.size()}); }

bool DestroyDB(const std::string& path_str)
{
    return rocksdb::DestroyDB(path_str, {}).ok();
}

/** Handle database error by throwing dbwrapper_error exception.
 */
static void HandleError(const rocksdb::Status& status)
{
    if (status.ok())
        return;
    const std::string errmsg = "Fatal RocksDB error: " + status.ToString();
    LogError("%s", errmsg);
    LogInfo("You can use -debug=leveldb to get more complete diagnostic messages");
    throw dbwrapper_error(errmsg);
}

class CBitcoinRocksDBLogger : public rocksdb::Logger {
public:
    // This code is adapted from posix_logger.h, which is why it is using vsprintf.
    // Please do not do this in normal code
    void Logv(const rocksdb::InfoLogLevel, const char* format, va_list ap) override {
            if (!util::log::ShouldDebugLog(BCLog::LEVELDB)) {
                return;
            }
            char buffer[500];
            for (int iter = 0; iter < 2; iter++) {
                char* base;
                int bufsize;
                if (iter == 0) {
                    bufsize = sizeof(buffer);
                    base = buffer;
                }
                else {
                    bufsize = 30000;
                    base = new char[bufsize];
                }
                char* p = base;
                char* limit = base + bufsize;

                // Print the message
                if (p < limit) {
                    va_list backup_ap;
                    va_copy(backup_ap, ap);
                    // Do not use vsnprintf elsewhere in bitcoin source code, see above.
                    p += vsnprintf(p, limit - p, format, backup_ap);
                    va_end(backup_ap);
                }

                // Truncate to available space if necessary
                if (p >= limit) {
                    if (iter == 0) {
                        continue;       // Try again with larger buffer
                    }
                    else {
                        p = limit - 1;
                    }
                }

                // Add newline if necessary
                if (p == base || p[-1] != '\n') {
                    *p++ = '\n';
                }

                assert(p <= limit);
                base[std::min(bufsize - 1, (int)(p - base))] = '\0';
                LogDebug(BCLog::LEVELDB, "%s\n", util::RemoveSuffixView(base, "\n"));
                if (base != buffer) {
                    delete[] base;
                }
                break;
            }
    }
};

static void SetMaxOpenFiles(rocksdb::Options* options) {
    // On most platforms the default setting of max_open_files (which is 1000)
    // is optimal. On Windows using a large file count is OK because the handles
    // do not interfere with select() loops. On 64-bit Unix hosts this value is
    // also OK, because up to that amount LevelDB will use an mmap
    // implementation that does not use extra file descriptors (the fds are
    // closed after being mmap'ed).
    //
    // Increasing the value beyond the default is dangerous because LevelDB will
    // fall back to a non-mmap implementation when the file count is too large.
    // On 32-bit Unix host we should decrease the value because the handles use
    // up real fds, and we want to avoid fd exhaustion issues.
    //
    // See PR #12495 for further discussion.

    int default_open_files = options->max_open_files;
#ifndef WIN32
    if (sizeof(void*) < 8) {
        options->max_open_files = 64;
    }
#endif
    LogDebug(BCLog::LEVELDB, "RocksDB using max_open_files=%d (default=%d)\n",
             options->max_open_files, default_open_files);
}

static rocksdb::Options GetOptions(size_t nCacheSize)
{
    rocksdb::Options options;
    options.write_buffer_size = nCacheSize / 4; // up to two write buffers may be held in memory simultaneously
    options.compression = rocksdb::kNoCompression;
    options.info_log = std::make_shared<CBitcoinRocksDBLogger>();
    options.paranoid_checks = true;
    options.target_file_size_base = DBWRAPPER_MAX_FILE_SIZE;

    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = rocksdb::NewLRUCache(nCacheSize / 2);
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

    SetMaxOpenFiles(&options);
    return options;
}

struct CDBBatch::WriteBatchImpl {
    rocksdb::WriteBatch batch;
};

CDBBatch::CDBBatch(const CDBWrapper& _parent)
    : parent{_parent},
      m_impl_batch{std::make_unique<CDBBatch::WriteBatchImpl>()}
{
    m_key_scratch.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
    m_value_scratch.reserve(DBWRAPPER_PREALLOC_VALUE_SIZE);
    Clear();
};

CDBBatch::~CDBBatch() = default;

void CDBBatch::Clear()
{
    m_impl_batch->batch.Clear();
    assert(m_key_scratch.empty());
    assert(m_value_scratch.empty());
}

void CDBBatch::WriteImpl(std::span<const std::byte> key, DataStream& value)
{
    rocksdb::Slice slKey(CharCast(key.data()), key.size());
    dbwrapper_private::GetObfuscation(parent)(value);
    rocksdb::Slice slValue(CharCast(value.data()), value.size());
    m_impl_batch->batch.Put(slKey, slValue);
}

void CDBBatch::EraseImpl(std::span<const std::byte> key)
{
    rocksdb::Slice slKey(CharCast(key.data()), key.size());
    m_impl_batch->batch.Delete(slKey);
}

size_t CDBBatch::ApproximateSize() const
{
    return m_impl_batch->batch.GetDataSize();
}

struct RocksDBContext {
    //! custom environment this database is using (may be nullptr in case of default environment)
    rocksdb::Env* penv;

    //! database options used
    rocksdb::Options options;

    //! options used when reading from the database
    rocksdb::ReadOptions readoptions;

    //! options used when iterating over values of the database
    rocksdb::ReadOptions iteroptions;

    //! options used when writing to the database
    rocksdb::WriteOptions writeoptions;

    //! options used when sync writing to the database
    rocksdb::WriteOptions syncoptions;

    //! the database itself
    std::unique_ptr<rocksdb::DB> pdb;
};

CDBWrapper::CDBWrapper(const DBParams& params)
    : m_db_context{std::make_unique<RocksDBContext>()}, m_name{fs::PathToString(params.path.stem())}
{
    DBContext().penv = nullptr;
    DBContext().readoptions.verify_checksums = true;
    DBContext().iteroptions.verify_checksums = true;
    DBContext().iteroptions.fill_cache = false;
    DBContext().syncoptions.sync = true;
    DBContext().options = GetOptions(params.cache_bytes);
    DBContext().options.create_if_missing = true;
    DBContext().options.target_file_size_base = params.max_file_size;
    assert(!(params.testing_env && params.memory_only));
    if (params.testing_env) {
        DBContext().options.env = params.testing_env;
    } else if (params.memory_only) {
        DBContext().penv = rocksdb::NewMemEnv(rocksdb::Env::Default());
        DBContext().options.env = DBContext().penv;
    }
    if (!params.memory_only) {
        if (!params.testing_env) {
            TryCreateDirectories(params.path);
        }
        if (params.wipe_data) {
            LogInfo("Wiping RocksDB in %s", fs::PathToString(params.path));
            rocksdb::Status result = rocksdb::DestroyDB(fs::PathToString(params.path), DBContext().options);
            HandleError(result);
            TryCreateDirectories(params.path);
        }
        LogInfo("Opening RocksDB in %s", fs::PathToString(params.path));
    }
    rocksdb::Status status = rocksdb::DB::Open(DBContext().options, fs::PathToString(params.path), &DBContext().pdb);
    HandleError(status);
    LogInfo("Opened RocksDB successfully");

    if (params.options.force_compact) {
        LogInfo("Starting database compaction of %s", fs::PathToString(params.path));
        CompactFull();
        LogInfo("Finished database compaction of %s", fs::PathToString(params.path));
    }

    if (!Read(OBFUSCATION_KEY, m_obfuscation) && params.obfuscate && IsEmpty()) {
        // Generate and write the new obfuscation key.
        const Obfuscation obfuscation{FastRandomContext{}.randbytes<Obfuscation::KEY_SIZE>()};
        assert(!m_obfuscation); // Make sure the key is written without obfuscation.
        Write(OBFUSCATION_KEY, obfuscation);
        m_obfuscation = obfuscation;
        LogInfo("Wrote new obfuscation key for %s: %s", fs::PathToString(params.path), m_obfuscation.HexKey());
    }
    LogInfo("Using obfuscation key for %s: %s", fs::PathToString(params.path), m_obfuscation.HexKey());
}

CDBWrapper::~CDBWrapper()
{
    DBContext().pdb.reset();
    delete DBContext().penv;
    DBContext().options.env = nullptr;
}

void CDBWrapper::WriteBatch(CDBBatch& batch, bool fSync)
{
    const bool log_memory = util::log::ShouldDebugLog(BCLog::LEVELDB);
    double mem_before = 0;
    if (log_memory) {
        mem_before = DynamicMemoryUsage() / double(1_MiB);
    }
    rocksdb::Status status = DBContext().pdb->Write(fSync ? DBContext().syncoptions : DBContext().writeoptions, &batch.m_impl_batch->batch);
    HandleError(status);
    if (log_memory) {
        double mem_after{DynamicMemoryUsage() / double(1_MiB)};
        LogDebug(BCLog::LEVELDB, "WriteBatch memory usage: db=%s, before=%.1fMiB, after=%.1fMiB\n",
                 m_name, mem_before, mem_after);
    }
}

std::optional<std::string> CDBWrapper::GetProperty(const std::string& property) const
{
    constexpr std::string_view leveldb_prefix{"leveldb."};
    constexpr std::string_view rocksdb_prefix{"rocksdb."};
    std::string property_name{property};
    if (property == "leveldb.approximate-memory-usage") {
        property_name = rocksdb::DB::Properties::kCurSizeAllMemTables;
    } else if (property == "leveldb.stats") {
        property_name = "rocksdb.levelstats";
    } else if (property_name.starts_with(leveldb_prefix)) {
        property_name.replace(0, leveldb_prefix.size(), rocksdb_prefix.data(), rocksdb_prefix.size());
    }

    if (std::string value; DBContext().pdb->GetProperty(property_name, &value)) {
        if (property == "leveldb.stats") {
            std::ostringstream stats;
            std::istringstream stream{value};
            for (std::string line; std::getline(stream, line);) {
                std::istringstream line_stream{line};
                int level;
                int files;
                double size_mib;
                if (line_stream >> level >> files >> size_mib) {
                    stats << level << ' ' << files << ' ' << size_mib << " 0 0 0\n";
                }
            }
            return stats.str();
        }
        return value;
    }
    return std::nullopt;
}

void CDBWrapper::CompactFull()
{
    rocksdb::FlushOptions flush_options;
    flush_options.wait = true;
    HandleError(DBContext().pdb->Flush(flush_options));
    HandleError(DBContext().pdb->CompactRange(rocksdb::CompactRangeOptions{}, nullptr, nullptr));
}

size_t CDBWrapper::DynamicMemoryUsage() const
{
    std::optional<size_t> parsed;
    if (auto memory{GetProperty("leveldb.approximate-memory-usage")}; !memory || !(parsed = ToIntegral<size_t>(*memory))) {
        LogDebug(BCLog::LEVELDB, "Failed to get approximate-memory-usage property\n");
        return 0;
    }
    return parsed.value();
}

std::optional<std::string> CDBWrapper::ReadImpl(std::span<const std::byte> key) const
{
    rocksdb::Slice slKey(CharCast(key.data()), key.size());
    std::string strValue;
    rocksdb::Status status = DBContext().pdb->Get(DBContext().readoptions, slKey, &strValue);
    if (!status.ok()) {
        if (status.IsNotFound())
            return std::nullopt;
        LogError("RocksDB read failure: %s", status.ToString());
        HandleError(status);
    }
    return strValue;
}

bool CDBWrapper::ExistsImpl(std::span<const std::byte> key) const
{
    rocksdb::Slice slKey(CharCast(key.data()), key.size());

    std::string strValue;
    rocksdb::Status status = DBContext().pdb->Get(DBContext().readoptions, slKey, &strValue);
    if (!status.ok()) {
        if (status.IsNotFound())
            return false;
        LogError("RocksDB read failure: %s", status.ToString());
        HandleError(status);
    }
    return true;
}

size_t CDBWrapper::EstimateSizeImpl(std::span<const std::byte> key1, std::span<const std::byte> key2) const
{
    rocksdb::Slice slKey1(CharCast(key1.data()), key1.size());
    rocksdb::Slice slKey2(CharCast(key2.data()), key2.size());
    uint64_t size = 0;
    rocksdb::Range range(slKey1, slKey2);
    DBContext().pdb->GetApproximateSizes(&range, 1, &size);
    return size;
}

bool CDBWrapper::IsEmpty()
{
    std::unique_ptr<CDBIterator> it(NewIterator());
    it->SeekToFirst();
    return !(it->Valid());
}

struct CDBIterator::IteratorImpl {
    const std::unique_ptr<rocksdb::Iterator> iter;

    explicit IteratorImpl(rocksdb::Iterator* _iter) : iter{_iter} {}
};

CDBIterator::CDBIterator(const CDBWrapper& _parent, std::unique_ptr<IteratorImpl> _piter) : parent(_parent),
                                                                                            m_impl_iter(std::move(_piter))
{
    m_scratch.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
}

CDBIterator* CDBWrapper::NewIterator()
{
    return new CDBIterator{*this, std::make_unique<CDBIterator::IteratorImpl>(DBContext().pdb->NewIterator(DBContext().iteroptions))};
}

void CDBIterator::SeekImpl(std::span<const std::byte> key)
{
    rocksdb::Slice slKey(CharCast(key.data()), key.size());
    m_impl_iter->iter->Seek(slKey);
}

std::span<const std::byte> CDBIterator::GetKeyImpl() const
{
    // The returned span borrows from the current iterator entry and is only
    // valid until the iterator is advanced.
    return ToByteSpan(m_impl_iter->iter->key());
}

std::span<const std::byte> CDBIterator::GetValueImpl() const
{
    return ToByteSpan(m_impl_iter->iter->value());
}

CDBIterator::~CDBIterator() = default;
bool CDBIterator::Valid() const { return m_impl_iter->iter->Valid(); }
void CDBIterator::SeekToFirst() { m_impl_iter->iter->SeekToFirst(); }
void CDBIterator::Next() { m_impl_iter->iter->Next(); }

namespace dbwrapper_private {

const Obfuscation& GetObfuscation(const CDBWrapper& w)
{
    return w.m_obfuscation;
}

} // namespace dbwrapper_private
