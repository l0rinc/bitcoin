// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sync.h>
#include <test/util/coins.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_flush_tests, TestingSetup)

//! Verify that Chainstate::GetCoinsCacheSizeState() switches from OK→LARGE→CRITICAL
//! at the expected utilization thresholds, first with *no* mempool head-room,
//! then with additional mempool head-room.
BOOST_AUTO_TEST_CASE(getcoinscachesizestate)
{
    Chainstate& chainstate{m_node.chainman->ActiveChainstate()};

    LOCK(::cs_main);
    CCoinsViewCache& view{chainstate.CoinsTip()};

    // Sanity: an empty cache should comfortably fit within its configured size.
    const size_t empty_cache_bytes{view.DynamicMemoryUsage()};
    BOOST_CHECK_GT(empty_cache_bytes, 0U);
    BOOST_CHECK_LT(empty_cache_bytes, chainstate.m_coinstip_cache_size_bytes);

    // Use a small growth target on top of the current baseline so the test runs fast
    // regardless of cache preallocation heuristics.
    const size_t baseline_cache_bytes{view.ActiveMemoryUsage()};
    const size_t MAX_COINS_BYTES{baseline_cache_bytes + 8_MiB};
    constexpr size_t MAX_MEMPOOL_BYTES{4_MiB};
    constexpr size_t MAX_ATTEMPTS{50'000};

    // Run the same growth-path twice: first with 0 head-room, then with extra head-room
    for (size_t max_mempool_size_bytes : {size_t{0}, MAX_MEMPOOL_BYTES}) {
        const int64_t full_cap{int64_t(MAX_COINS_BYTES + max_mempool_size_bytes)};
        const int64_t large_cap{LargeCoinsCacheThreshold(full_cap)};

        // OK → LARGE
        auto state{chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, max_mempool_size_bytes)};
        for (size_t i{0}; i < MAX_ATTEMPTS && int64_t(view.ActiveMemoryUsage()) <= large_cap; ++i) {
            BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::OK);
            AddTestCoin(m_rng, view);
            state = chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, max_mempool_size_bytes);
        }

        // LARGE → CRITICAL
        for (size_t i{0}; i < MAX_ATTEMPTS && int64_t(view.ActiveMemoryUsage()) <= full_cap; ++i) {
            BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::LARGE);
            AddTestCoin(m_rng, view);
            state = chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, max_mempool_size_bytes);
        }
        BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::CRITICAL);
    }

    // Default thresholds (no explicit limits) permit many more coins.
    for (int i{0}; i < 1'000; ++i) {
        AddTestCoin(m_rng, view);
        BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(), CoinsCacheSizeState::OK);
    }

    // CRITICAL → OK via Flush
    BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, /*max_mempool_size_bytes=*/0), CoinsCacheSizeState::CRITICAL);
    view.SetBestBlock(m_rng.rand256());
    BOOST_REQUIRE(view.Flush());
    BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, /*max_mempool_size_bytes=*/0), CoinsCacheSizeState::OK);
}

BOOST_AUTO_TEST_SUITE_END()
