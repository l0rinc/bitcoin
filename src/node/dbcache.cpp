// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/caches.h>

#include <algorithm>

namespace node {
size_t GetDefaultDbCacheBytes() noexcept
{
    return std::min(DEFAULT_DB_CACHE, MAX_DBCACHE_BYTES);
}
} // namespace node
