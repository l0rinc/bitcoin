// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/caches.h>

#include <common/system.h>
#include <logging.h>

#include <algorithm>
#include <atomic>

namespace node {
size_t GetDefaultDbCacheBytes() noexcept
{
    const auto total_ram{GetTotalRAM()};
    if (!total_ram) {
        static std::atomic_bool logged_fallback{false};
        if (!logged_fallback.exchange(true)) {
            LogInfo("Cannot determine total RAM, assuming %s GiB for -dbcache calculation.", FALLBACK_RAM_BYTES >> 30);
        }
    }
    return std::min(GetDefaultCache(total_ram.value_or(FALLBACK_RAM_BYTES)), MAX_DBCACHE_BYTES);
}
} // namespace node
