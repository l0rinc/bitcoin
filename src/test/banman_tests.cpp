// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <chainparams.h>
#include <netbase.h>
#include <streams.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/readwritefile.h>

#include <boost/test/unit_test.hpp>

#include <limits>

BOOST_FIXTURE_TEST_SUITE(banman_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(file)
{
    FakeNodeClock clock{777s};
    const fs::path banlist_path{m_args.GetDataDirBase() / "banlist_test"};
    {
        const std::string entries_write{
            "{ \"banned_nets\": ["
            "  { \"version\": 1, \"ban_created\": 0, \"banned_until\": 778, \"address\": \"aaaaaaaaa\" },"
            "  { \"version\": 2, \"ban_created\": 0, \"banned_until\": 778, \"address\": \"bbbbbbbbb\" },"
            "  { \"version\": 1, \"ban_created\": 0, \"banned_until\": 778, \"address\": \"1.0.0.0/8\" }"
            "] }",
        };
        BOOST_REQUIRE(WriteBinaryFile(banlist_path + ".json", entries_write));
        {
            // The invalid entries will be dropped, but the valid one remains
            ASSERT_DEBUG_LOG("Dropping entry with unparseable address or subnet (aaaaaaaaa) from ban list");
            ASSERT_DEBUG_LOG("Dropping entry with unknown version (2) from ban list");
            BanMan banman{banlist_path, /*client_interface=*/nullptr, /*default_ban_time=*/0};
            banmap_t entries_read;
            banman.GetBanned(entries_read);
            BOOST_CHECK_EQUAL(entries_read.size(), 1);
        }
    }
}

BOOST_AUTO_TEST_CASE(expired_at_boundary_is_not_returned)
{
    FakeNodeClock clock{777s};
    const fs::path banlist_path{m_args.GetDataDirBase() / "banlist_expiry_boundary_test"};

    const CSubNet subnet{LookupSubNet("1.2.3.0/24")};
    const CNetAddr subnet_addr{LookupHost("1.2.3.4", /*fAllowLookup=*/false).value()};
    BOOST_REQUIRE(subnet.IsValid());

    {
        BanMan banman{banlist_path, /*client_interface=*/nullptr, /*default_ban_time=*/0};
        banman.Ban(subnet, /*ban_time_offset=*/777, /*since_unix_epoch=*/true);
        BOOST_CHECK(!banman.IsBanned(subnet));
        BOOST_CHECK(!banman.IsBanned(subnet_addr));

        banmap_t entries;
        banman.GetBanned(entries);
        BOOST_CHECK(entries.empty());
    }
}

BOOST_AUTO_TEST_CASE(relative_ban_time_saturates)
{
    FakeNodeClock clock{777s};
    const fs::path banlist_path{m_args.GetDataDirBase() / "banlist_saturating_time_test"};

    const CSubNet subnet{LookupSubNet("1.2.3.0/24")};
    const CNetAddr subnet_addr{LookupHost("1.2.3.4", /*fAllowLookup=*/false).value()};
    BOOST_REQUIRE(subnet.IsValid());

    {
        BanMan banman{banlist_path, /*client_interface=*/nullptr, /*default_ban_time=*/0};
        banman.Ban(subnet, /*ban_time_offset=*/std::numeric_limits<int64_t>::max(), /*since_unix_epoch=*/false);

        BOOST_CHECK(banman.IsBanned(subnet));
        BOOST_CHECK(banman.IsBanned(subnet_addr));

        banmap_t entries;
        banman.GetBanned(entries);
        BOOST_REQUIRE_EQUAL(entries.count(subnet), 1U);
        BOOST_CHECK_EQUAL(entries.at(subnet).nBanUntil, std::numeric_limits<int64_t>::max());
    }

    {
        BanMan banman{banlist_path, /*client_interface=*/nullptr, /*default_ban_time=*/0};

        banmap_t entries;
        banman.GetBanned(entries);
        BOOST_REQUIRE_EQUAL(entries.count(subnet), 1U);
        BOOST_CHECK_EQUAL(entries.at(subnet).nBanUntil, std::numeric_limits<int64_t>::max());
        BOOST_CHECK(banman.IsBanned(subnet_addr));
    }
}

BOOST_AUTO_TEST_SUITE_END()
