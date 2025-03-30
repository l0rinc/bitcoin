// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_COINS_VIEW_ARGS_H
#define BITCOIN_NODE_COINS_VIEW_ARGS_H

#include <node/caches.h>

class ArgsManager;
struct CoinsViewOptions;

namespace node {
static constexpr size_t MIN_DB_CACHE_BATCH{2 * MIN_DB_CACHE};
static constexpr size_t DEFAULT_DB_CACHE_BATCH{20_MiB}; // The corresponding batch size for DEFAULT_KERNEL_CACHE
static constexpr size_t MAX_DB_CACHE_BATCH{256_MiB};

static constexpr size_t GetDefaultDbBatchSize(size_t dbcache_bytes)
{
    return MIN_DB_CACHE_BATCH +
           std::min<size_t>(
               MAX_DB_CACHE_BATCH,
               (dbcache_bytes * (DEFAULT_DB_CACHE_BATCH - MIN_DB_CACHE_BATCH) / DEFAULT_KERNEL_CACHE)
           );
}

void ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options, size_t coins);
} // namespace node

#endif // BITCOIN_NODE_COINS_VIEW_ARGS_H
