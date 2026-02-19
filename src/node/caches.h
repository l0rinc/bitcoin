// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CACHES_H
#define BITCOIN_NODE_CACHES_H

#include <kernel/caches.h>
#include <util/byte_units.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

class ArgsManager;

//! min. -dbcache (bytes)
static constexpr size_t MIN_DB_CACHE{4_MiB};
//! Automatic -dbcache floor (bytes)
static constexpr size_t MIN_DEFAULT_DBCACHE{100_MiB};
//! Automatic -dbcache cap (bytes)
static constexpr size_t MAX_DEFAULT_DBCACHE{3000_MiB};
//! Assumed total RAM when we cannot determine it.
static constexpr size_t FALLBACK_RAM_BYTES{SIZE_MAX == UINT64_MAX ? 4096_MiB : 2048_MiB};
//! Reserved non-dbcache memory usage.
static constexpr int64_t RESERVED_RAM{2048_MiB};
//! Maximum dbcache size on current architecture.
static constexpr size_t MAX_DBCACHE_BYTES{SIZE_MAX == UINT64_MAX ? std::numeric_limits<size_t>::max() : 1024_MiB};

namespace node {
//! Compute the automatic -dbcache size from total RAM, reserving memory for non-dbcache usage.
constexpr size_t GetDefaultCache(size_t total_ram) noexcept
{
    const int64_t raw_default_dbcache{(int64_t(total_ram) - RESERVED_RAM) / 4}; // 25% of remaining memory
    return size_t(std::max<int64_t>(MIN_DEFAULT_DBCACHE, std::min<int64_t>(raw_default_dbcache, MAX_DEFAULT_DBCACHE)));
}

size_t GetDefaultDbCacheBytes() noexcept;

struct IndexCacheSizes {
    size_t tx_index{0};
    size_t filter_index{0};
    size_t txospender_index{0};
};
struct CacheSizes {
    IndexCacheSizes index;
    kernel::CacheSizes kernel;
};
CacheSizes CalculateCacheSizes(const ArgsManager& args, size_t n_indexes = 0);

constexpr bool ShouldWarnOversizedDbCache(size_t dbcache, size_t total_ram) noexcept
{
    if (total_ram < FALLBACK_RAM_BYTES) return dbcache > GetDefaultCache(total_ram);
    else return dbcache > (total_ram / 100) * 75;
}

void LogOversizedDbCache(const ArgsManager& args) noexcept;
} // namespace node

#endif // BITCOIN_NODE_CACHES_H
