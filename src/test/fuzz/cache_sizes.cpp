// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <common/args.h>
#include <node/caches.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/byte_units.h>
#include <util/overflow.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>

namespace {
void AssertLargeDbCacheBoundary()
{
    ArgsManager args;
    args.ForceSetArg("-dbcache", "8796093022208"); // 2^43 MiB = 2^63 bytes.
    args.ForceSetArg("-txindex", "1");
    assert(node::CalculateCacheSizes(args).index.tx_index == 1_GiB);
}
} // namespace

FUZZ_TARGET(cache_sizes)
{
    AssertLargeDbCacheBoundary();
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const int64_t dbcache_mib{provider.ConsumeIntegral<int64_t>()};
    const bool txindex{provider.ConsumeBool()};
    const bool txospenderindex{provider.ConsumeBool()};
    const size_t filter_indexes{provider.ConsumeIntegral<size_t>()};

    ArgsManager args;
    args.ForceSetArg("-dbcache", std::to_string(dbcache_mib));
    args.ForceSetArg("-txindex", txindex ? "1" : "0");
    args.ForceSetArg("-txospenderindex", txospenderindex ? "1" : "0");
    const auto [index, kernel]{node::CalculateCacheSizes(args, filter_indexes)};

    const uint64_t nonnegative_mib{dbcache_mib < 0 ? 0 : static_cast<uint64_t>(dbcache_mib)};
    uint64_t total_cache{SaturatingLeftShift<uint64_t>(nonnegative_mib, 20)};
    if constexpr (sizeof(void*) == 4) total_cache = std::min(total_cache, 1_GiB);
    total_cache = std::max(total_cache, MIN_DB_CACHE);

    const uint64_t expected_txindex{txindex ? std::min(total_cache / 10, 1_GiB) : 0};
    const uint64_t expected_txospenderindex{txospenderindex ? std::min(total_cache / 20, 1_GiB) : 0};
    const uint64_t filter_budget{filter_indexes > 0 ? std::min(total_cache / 20, 1_GiB) : 0};
    const uint64_t expected_filter_index{filter_indexes > 0 ? filter_budget / filter_indexes : 0};
    const uint64_t expected_filter_total{expected_filter_index * filter_indexes};

    assert(index.tx_index == expected_txindex);
    assert(index.txospender_index == expected_txospenderindex);
    assert(index.filter_index == expected_filter_index);

    uint64_t remaining{total_cache - expected_txindex - expected_txospenderindex - expected_filter_total};
    const uint64_t expected_block_tree_db{std::min(remaining / 8, 2_MiB)};
    remaining -= expected_block_tree_db;
    const uint64_t expected_coins_db{std::min(remaining / 2, 8_MiB)};
    remaining -= expected_coins_db;

    assert(kernel.block_tree_db == expected_block_tree_db);
    assert(kernel.coins_db == expected_coins_db);
    assert(kernel.coins == remaining);
}
