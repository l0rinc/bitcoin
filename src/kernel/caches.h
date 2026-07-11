// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CACHES_H
#define BITCOIN_KERNEL_CACHES_H

#include <util/byte_units.h>

#include <cstdint>

//! Suggested default amount of cache reserved for the kernel (bytes)
static constexpr uint64_t DEFAULT_KERNEL_CACHE{450_MiB};
//! Default LevelDB write batch size
static constexpr uint64_t DEFAULT_DB_CACHE_BATCH{32_MiB};

#endif // BITCOIN_KERNEL_CACHES_H
