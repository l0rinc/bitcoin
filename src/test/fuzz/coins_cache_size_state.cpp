// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <util/overflow.h>
#include <validation.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace {
const TestingSetup* g_setup;

void initialize_coins_cache_size_state()
{
    static const auto testing_setup{MakeNoLogFileContext<const TestingSetup>()};
    g_setup = testing_setup.get();
}

size_t LargeCoinsCacheThresholdOracle(const size_t total_space)
{
    constexpr size_t MAX_BLOCK_COINSDB_USAGE_BYTES{10_MiB};
    const size_t ten_percent{total_space / 10 + (total_space % 10 != 0)};
    const size_t minimum_free_space{total_space < MAX_BLOCK_COINSDB_USAGE_BYTES ? 0 : total_space - MAX_BLOCK_COINSDB_USAGE_BYTES};
    return std::max(total_space - ten_percent, minimum_free_space);
}

CoinsCacheSizeState ExpectedState(Chainstate& chainstate, const size_t max_coins_cache_size_bytes, const size_t max_mempool_size_bytes) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    const size_t mempool_usage{chainstate.GetMempool() ? chainstate.GetMempool()->DynamicMemoryUsage() : 0};
    const size_t cache_size{chainstate.CoinsTip().DynamicMemoryUsage()};
    const size_t unused_mempool_space{max_mempool_size_bytes > mempool_usage ? max_mempool_size_bytes - mempool_usage : 0};
    const size_t total_space{SaturatingAdd(max_coins_cache_size_bytes, unused_mempool_space)};

    if (cache_size > total_space) return CoinsCacheSizeState::CRITICAL;
    if (cache_size > LargeCoinsCacheThresholdOracle(total_space)) return CoinsCacheSizeState::LARGE;
    return CoinsCacheSizeState::OK;
}

void CheckState(Chainstate& chainstate, const size_t max_coins_cache_size_bytes, const size_t max_mempool_size_bytes) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    assert(chainstate.GetCoinsCacheSizeState(max_coins_cache_size_bytes, max_mempool_size_bytes) ==
           ExpectedState(chainstate, max_coins_cache_size_bytes, max_mempool_size_bytes));
}
} // namespace

FUZZ_TARGET(coins_cache_size_state, .init = initialize_coins_cache_size_state)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    Chainstate& chainstate{g_setup->m_node.chainman->ActiveChainstate()};

    LOCK(::cs_main);

    // Always exercise the mixed signed/unsigned boundary that previously became CRITICAL.
    CheckState(chainstate, std::numeric_limits<size_t>::max(), 0);
    CheckState(chainstate, 0, std::numeric_limits<size_t>::max());
    CheckState(chainstate, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

    CheckState(chainstate, provider.ConsumeIntegral<size_t>(), provider.ConsumeIntegral<size_t>());
}
