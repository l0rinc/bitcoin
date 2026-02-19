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
    BOOST_CHECK(FALLBACK_RAM_BYTES >= 1024_MiB);
    BOOST_CHECK_EQUAL(GetDefaultCache(512_MiB), MIN_DEFAULT_DBCACHE);
    BOOST_CHECK_EQUAL(GetDefaultCache(1024_MiB), MIN_DEFAULT_DBCACHE);
    BOOST_CHECK_EQUAL(GetDefaultCache(RESERVED_RAM), MIN_DEFAULT_DBCACHE);

    constexpr size_t total_ram{3072_MiB};
    BOOST_CHECK_EQUAL(GetDefaultCache(total_ram), (total_ram - RESERVED_RAM) / 4);

    if constexpr (SIZE_MAX == UINT64_MAX) {
        BOOST_CHECK_EQUAL(GetDefaultCache(8192_MiB), 1536_MiB);
        BOOST_CHECK_EQUAL(GetDefaultCache(16384_MiB), MAX_DEFAULT_DBCACHE);
        BOOST_CHECK_EQUAL(GetDefaultCache(32768_MiB), MAX_DEFAULT_DBCACHE);
    }
}

BOOST_AUTO_TEST_CASE(oversized_dbcache_warning)
{
    {
        constexpr size_t total_ram{1024_MiB};
        constexpr size_t default_cache{GetDefaultCache(total_ram)};
        BOOST_CHECK(!ShouldWarnOversizedDbCache(MIN_DB_CACHE, total_ram));      // Under cap
        BOOST_CHECK(!ShouldWarnOversizedDbCache(default_cache, total_ram));     // At cap
        BOOST_CHECK( ShouldWarnOversizedDbCache(default_cache + 1, total_ram)); // Over cap
    }

    {
        constexpr size_t total_ram{FALLBACK_RAM_BYTES - 1_MiB};
        constexpr size_t default_cache{GetDefaultCache(total_ram)};
        BOOST_CHECK(!ShouldWarnOversizedDbCache(default_cache, total_ram));
        BOOST_CHECK( ShouldWarnOversizedDbCache(default_cache + 1, total_ram));
    }

    {
        constexpr size_t total_ram{FALLBACK_RAM_BYTES};
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
    BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultCache(1024_MiB), 1024_MiB));
    BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultCache(2048_MiB), 2048_MiB));
    BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultCache(3072_MiB), 3072_MiB));

    if constexpr (SIZE_MAX == UINT64_MAX) {
        BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultCache(4096_MiB), 4096_MiB));
        BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultCache(8192_MiB), 8192_MiB));
        BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultCache(16384_MiB), 16384_MiB));
        BOOST_CHECK(!ShouldWarnOversizedDbCache(GetDefaultCache(32768_MiB), 32768_MiB));
    }
}

BOOST_AUTO_TEST_SUITE_END()
