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

    // Sanity: an empty cache should be ≲ 1 chunk (~ 256 KiB).
    BOOST_CHECK_LT(view.DynamicMemoryUsage() / (256 * 1024.0), 1.1);

    constexpr size_t MAX_COINS_CACHE_BYTES{8_MiB};
    constexpr size_t MAX_MEMPOOL_CACHE_BYTES{4_MiB};
    constexpr size_t MAX_ATTEMPTS{50'000};

    // Run the same growth-path twice: first with 0 head-room, then with extra head-room
    for (size_t max_mempool_size_bytes : {size_t{0}, MAX_MEMPOOL_CACHE_BYTES}) {
        // Fresh baseline
        BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, max_mempool_size_bytes), CoinsCacheSizeState::OK);

        const int64_t full_cap{int64_t(MAX_COINS_CACHE_BYTES + max_mempool_size_bytes)};

        // OK → LARGE
        for (size_t i{1}; i <= MAX_ATTEMPTS; ++i) {
            AddTestCoin(m_rng, view);
            const auto state{chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, max_mempool_size_bytes)};
            const int64_t cache_size{int64_t(view.DynamicMemoryUsage())};
            if (i == MAX_ATTEMPTS || cache_size >= LargeCoinsCacheThreshold(full_cap)) {
                BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::LARGE);
                break;
            }
            BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::OK);
        }

        // LARGE → CRITICAL
        for (size_t i{1}; i <= MAX_ATTEMPTS; ++i) {
            AddTestCoin(m_rng, view);
            const auto state{chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, max_mempool_size_bytes)};
            const int64_t cache_size{int64_t(view.DynamicMemoryUsage())};
            if (i == MAX_ATTEMPTS || cache_size > full_cap) {
                BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::CRITICAL);
                break;
            }
            BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::LARGE);
        }
    }

    // Default thresholds (no explicit limits) permit many more coins.
    for (int i{0}; i < 1'000; ++i) {
        AddTestCoin(m_rng, view);
        BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(), CoinsCacheSizeState::OK);
    }

    // CRITICAL → OK via Flush
    BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/0), CoinsCacheSizeState::CRITICAL);
    view.SetBestBlock(m_rng.rand256());
    BOOST_CHECK(view.Flush());

    BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(MAX_COINS_CACHE_BYTES, /*max_mempool_size_bytes=*/0), CoinsCacheSizeState::OK);
}

BOOST_AUTO_TEST_SUITE_END()
