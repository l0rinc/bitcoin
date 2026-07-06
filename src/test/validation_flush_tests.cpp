// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <sync.h>
#include <test/util/coins.h>
#include <test/util/random.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <util/mempressure.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <limits>

BOOST_FIXTURE_TEST_SUITE(validation_flush_tests, TestingSetup)

struct LowMemoryThresholdGuard {
    const size_t old_threshold{g_low_memory_threshold};
    ~LowMemoryThresholdGuard() { g_low_memory_threshold = old_threshold; }
};

BOOST_AUTO_TEST_CASE(available_memory_below_threshold)
{
    BOOST_CHECK_EQUAL(g_low_memory_threshold, 0U);
    BOOST_CHECK(!AvailableMemoryBelowThreshold(/*threshold=*/0, /*free_ram=*/0, /*buffer_ram=*/0));
    BOOST_CHECK(!AvailableMemoryBelowThreshold(/*threshold=*/1_MiB, /*free_ram=*/1_MiB, /*buffer_ram=*/0));
    BOOST_CHECK(!AvailableMemoryBelowThreshold(
        /*threshold=*/1_MiB,
        /*free_ram=*/1_MiB / 2,
        /*buffer_ram=*/1_MiB / 2));
    BOOST_CHECK(AvailableMemoryBelowThreshold(
        /*threshold=*/1_MiB,
        /*free_ram=*/1_MiB / 2,
        /*buffer_ram=*/(1_MiB / 2) - 1));
    BOOST_CHECK(!AvailableMemoryBelowThreshold(
        /*threshold=*/std::numeric_limits<size_t>::max(),
        /*free_ram=*/std::numeric_limits<uint64_t>::max(),
        /*buffer_ram=*/1));
}

//! Verify that Chainstate::GetCoinsCacheSizeState() switches from OK→LARGE→CRITICAL
//! at the expected utilization thresholds, first with *no* mempool head-room,
//! then with additional mempool head-room.
BOOST_AUTO_TEST_CASE(getcoinscachesizestate)
{
    Chainstate& chainstate{m_node.chainman->ActiveChainstate()};

    LOCK(::cs_main);
    CCoinsViewCache& view{chainstate.CoinsTip()};

    // Sanity: an empty cache should be <= 1 chunk (~ 256 KiB).
    BOOST_CHECK_LT(view.DynamicMemoryUsage() / (256 * 1024.0), 1.1);

    constexpr size_t MAX_COINS_BYTES{8_MiB};
    constexpr size_t MAX_MEMPOOL_BYTES{4_MiB};
    constexpr size_t MAX_ATTEMPTS{50'000};

    // Run the same growth-path twice: first with 0 head-room, then with extra head-room
    for (size_t max_mempool_size_bytes : {size_t{0}, MAX_MEMPOOL_BYTES}) {
        const int64_t full_cap{int64_t(MAX_COINS_BYTES + max_mempool_size_bytes)};
        const int64_t large_cap{LargeCoinsCacheThreshold(full_cap)};

        // OK → LARGE
        auto state{chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, max_mempool_size_bytes)};
        for (size_t i{0}; i < MAX_ATTEMPTS && int64_t(view.DynamicMemoryUsage()) <= large_cap; ++i) {
            BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::OK);
            AddTestCoin(m_rng, view);
            state = chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, max_mempool_size_bytes);
        }

        // LARGE → CRITICAL
        for (size_t i{0}; i < MAX_ATTEMPTS && int64_t(view.DynamicMemoryUsage()) <= full_cap; ++i) {
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
    view.Flush();
    BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, /*max_mempool_size_bytes=*/0), CoinsCacheSizeState::OK);
}

BOOST_AUTO_TEST_CASE(flush_if_needed_when_system_needs_memory_released)
{
#if defined(HAVE_LINUX_SYSINFO) || defined(WIN32)
    if constexpr (std::numeric_limits<size_t>::digits < 64) {
        BOOST_WARN_MESSAGE(false, "skipping low-memory flush test: threshold cannot exceed likely host RAM on 32-bit size_t");
        return;
    }

    LowMemoryThresholdGuard threshold_guard;
    Chainstate& chainstate{m_node.chainman->ActiveChainstate()};

    COutPoint outpoint;
    {
        LOCK(::cs_main);
        CCoinsViewCache& view{chainstate.CoinsTip()};
        outpoint = AddTestCoin(m_rng, view);
        if (view.GetBestBlock().IsNull()) view.SetBestBlock(m_rng.rand256());
        BOOST_REQUIRE(view.HaveCoinInCache(outpoint));
        BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(), CoinsCacheSizeState::OK);
    }

    BlockValidationState state;

    g_low_memory_threshold = 0;
    BOOST_CHECK(chainstate.FlushStateToDisk(state, FlushStateMode::IF_NEEDED));
    {
        LOCK(::cs_main);
        BOOST_CHECK(chainstate.CoinsTip().HaveCoinInCache(outpoint));
    }

    g_low_memory_threshold = std::numeric_limits<size_t>::max();
    BOOST_REQUIRE(SystemNeedsMemoryReleased());
    BOOST_CHECK(chainstate.FlushStateToDisk(state, FlushStateMode::IF_NEEDED));
    {
        LOCK(::cs_main);
        BOOST_CHECK(!chainstate.CoinsTip().HaveCoinInCache(outpoint));
    }
#else
    BOOST_WARN_MESSAGE(false, "skipping low-memory flush test: no supported host memory pressure probe");
#endif
}

BOOST_AUTO_TEST_SUITE_END()
