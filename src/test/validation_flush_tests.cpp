// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <kernel/mempool_options.h>
#include <node/mempool_args.h>
#include <sync.h>
#include <test/util/coins.h>
#include <test/util/random.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <limits>
#include <string>

BOOST_FIXTURE_TEST_SUITE(validation_flush_tests, TestingSetup)

//! Verify that Chainstate::GetCoinsCacheSizeState() switches from OK→LARGE→CRITICAL
//! at the expected utilization thresholds, first with *no* mempool head-room,
//! then with additional mempool head-room.
BOOST_AUTO_TEST_CASE(getcoinscachesizestate)
{
    constexpr uint64_t MAX_COINS_BYTES{8_MiB};
    constexpr uint64_t MAX_MEMPOOL_BYTES{4_MiB};
    constexpr uint64_t MAX_ATTEMPTS{50'000};
    Chainstate& chainstate{m_node.chainman->ActiveChainstate()};

    LOCK(::cs_main);
    BOOST_REQUIRE(chainstate.ResizeCoinsCaches(MAX_COINS_BYTES, /*coinsdb_size=*/1_MiB));
    CCoinsViewCache& view{chainstate.CoinsTip()};

    // Sanity: an empty cache should be ≲ 1 chunk (~ 256 KiB).
    BOOST_CHECK_LT(view.DynamicMemoryUsage() / (256 * 1024.0), 1.1);

    // Run the same growth-path twice: first with 0 head-room, then with extra head-room
    for (uint64_t max_mempool_size_bytes : {uint64_t{0}, MAX_MEMPOOL_BYTES}) {
        const size_t full_cap{MAX_COINS_BYTES + max_mempool_size_bytes};
        const size_t large_cap{LargeCoinsCacheThreshold(full_cap)};

        // OK → LARGE
        auto state{chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, max_mempool_size_bytes)};
        for (uint64_t i{0}; i < MAX_ATTEMPTS && view.DynamicMemoryUsage() <= large_cap; ++i) {
            BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::OK);
            AddTestCoin(m_rng, view);
            state = chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, max_mempool_size_bytes);
        }

        // LARGE → CRITICAL
        for (uint64_t i{0}; i < MAX_ATTEMPTS && view.DynamicMemoryUsage() <= full_cap; ++i) {
            BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::LARGE);
            AddTestCoin(m_rng, view);
            state = chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, max_mempool_size_bytes);
        }
        BOOST_CHECK_EQUAL(state, CoinsCacheSizeState::CRITICAL);
    }

    // Unused mempool space permits many more coins.
    for (int i{0}; i < 1'000; ++i) {
        AddTestCoin(m_rng, view);
        BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(), CoinsCacheSizeState::OK);
    }

    // CRITICAL → OK via Flush
    BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, /*max_mempool_size_bytes=*/0), CoinsCacheSizeState::CRITICAL);
    BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(), CoinsCacheSizeState::OK);
    view.SetBestBlock(m_rng.rand256());
    view.Flush();
    BOOST_CHECK_EQUAL(chainstate.GetCoinsCacheSizeState(MAX_COINS_BYTES, /*max_mempool_size_bytes=*/0), CoinsCacheSizeState::OK);
}

BOOST_AUTO_TEST_CASE(extreme_cache_capacity_is_not_critical)
{
    if constexpr (std::numeric_limits<size_t>::max() <= std::numeric_limits<int64_t>::max()) return;

    Chainstate& chainstate{m_node.chainman->ActiveChainstate()};
    LOCK(::cs_main);
    BOOST_REQUIRE(chainstate.ResizeCoinsCaches(8_MiB, /*coinsdb_size=*/1_MiB));

    // An empty cache with the largest representable capacity has ample room.
    BOOST_CHECK_EQUAL(
        chainstate.GetCoinsCacheSizeState(std::numeric_limits<size_t>::max(), /*max_mempool_size_bytes=*/0),
        CoinsCacheSizeState::OK);
}

BOOST_AUTO_TEST_CASE(maxmempool_byte_size_overflow_is_rejected)
{
    ArgsManager args;
    args.ForceSetArg("-maxmempool", std::to_string(std::numeric_limits<int64_t>::max() / 1'000'000 + 1));

    kernel::MemPoolOptions options{};
    const auto result{ApplyArgsManOptions(args, Params(), options)};
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_SUITE_END()
