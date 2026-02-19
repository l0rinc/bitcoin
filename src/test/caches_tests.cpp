// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <node/caches.h>
#include <util/byte_units.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <limits>

using namespace node;

BOOST_AUTO_TEST_SUITE(caches_tests)

BOOST_AUTO_TEST_CASE(default_dbcache_formula_by_total_ram)
{
    // The current default dbcache is fixed.
    // Future commits will switch to a total-RAM-based default.
    BOOST_CHECK_EQUAL(GetDefaultDbCacheBytes(), DEFAULT_DB_CACHE);
}

BOOST_AUTO_TEST_CASE(oversized_dbcache_warning)
{
    {
        constexpr size_t total_ram{1024_MiB};
        const size_t cap{GetDefaultDbCacheBytes()};
        BOOST_CHECK(!ShouldWarnOversizedDbCache(MIN_DB_CACHE, total_ram));  // Under cap
        BOOST_CHECK(!ShouldWarnOversizedDbCache(cap, total_ram));           // At cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(cap + 1, total_ram));       // Over cap
    }

    {
        constexpr size_t total_ram{3072_MiB};
        constexpr size_t cap{(total_ram / 100) * 75};
        BOOST_CHECK(!ShouldWarnOversizedDbCache(cap, total_ram));
        BOOST_CHECK( ShouldWarnOversizedDbCache(cap + 1, total_ram));
    }

    if constexpr (SIZE_MAX == UINT64_MAX) {
        BOOST_CHECK(!ShouldWarnOversizedDbCache(/*dbcache=*/12'000_MiB, /*total_ram=*/16384_MiB));
        BOOST_CHECK( ShouldWarnOversizedDbCache(/*dbcache=*/13'000_MiB, /*total_ram=*/16384_MiB));
    }
}

BOOST_AUTO_TEST_CASE(default_dbcache_never_warns)
{
    BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultDbCacheBytes(), /*total_ram=*/1024_MiB));
    BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultDbCacheBytes(), /*total_ram=*/2048_MiB));
    BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultDbCacheBytes(), /*total_ram=*/3072_MiB));

    if constexpr (SIZE_MAX == UINT64_MAX) {
        BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultDbCacheBytes(), /*total_ram=*/4096_MiB));
        BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultDbCacheBytes(), /*total_ram=*/16384_MiB));
    }
}

BOOST_AUTO_TEST_SUITE_END()
