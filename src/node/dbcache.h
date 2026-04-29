// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_DBCACHE_H
#define BITCOIN_NODE_DBCACHE_H

#include <util/byte_units.h>

#include <cstddef>
#include <cstdint>
#include <limits>

//! min. -dbcache (bytes)
static constexpr uint64_t MIN_DBCACHE_BYTES{4_MiB};
//! -dbcache default (bytes)
static constexpr uint64_t DEFAULT_DBCACHE_BYTES{450_MiB};
//! Maximum dbcache size on current architecture.
static constexpr uint64_t MAX_DBCACHE_BYTES{sizeof(void*) == 4 ? 1_GiB : std::numeric_limits<uint64_t>::max()};

namespace node {
size_t GetDefaultDBCache();

constexpr bool ShouldWarnOversizedDbCache(uint64_t dbcache, uint64_t total_ram) noexcept
{
    const uint64_t cap{(total_ram < 2_GiB) ? DEFAULT_DBCACHE_BYTES : (total_ram / 100) * 75};
    return dbcache > cap;
}
} // namespace node

#endif // BITCOIN_NODE_DBCACHE_H
