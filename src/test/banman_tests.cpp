// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <chainparams.h>
#include <node/interface_ui.h>
#include <netbase.h>
#include <scheduler.h>
#include <streams.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/readwritefile.h>

#include <atomic>
#include <boost/test/unit_test.hpp>
#include <future>
#include <thread>

BOOST_FIXTURE_TEST_SUITE(banman_tests, BasicTestingSetup)

namespace {
struct ScopedScheduler {
    CScheduler scheduler{};

    ScopedScheduler()
    {
        scheduler.m_service_thread = std::thread([this] { scheduler.serviceQueue(); });
    }
    ~ScopedScheduler()
    {
        scheduler.stop();
    }
    void MockForwardAndSync(std::chrono::seconds duration)
    {
        scheduler.MockForward(duration);
        std::promise<void> promise;
        scheduler.scheduleFromNow([&promise] { promise.set_value(); }, 0ms);
        promise.get_future().wait();
    }
};
} // namespace

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

BOOST_AUTO_TEST_CASE(sweep_banned_at_expiry_time)
{
    FakeNodeClock clock{100s};
    const fs::path banlist_path{m_args.GetDataDirBase() / "banlist_expiry_test"};
    BanMan banman{banlist_path, /*client_interface=*/nullptr, /*default_ban_time=*/0};
    const CSubNet subnet{LookupSubNet("1.2.3.4")};
    BOOST_REQUIRE(subnet.IsValid());

    banman.Ban(subnet, /*ban_time_offset=*/105, /*since_unix_epoch=*/true);
    banmap_t entries;
    banman.GetBanned(entries);
    BOOST_CHECK_EQUAL(entries.size(), 1);

    clock.set(105s);
    banman.GetBanned(entries);
    BOOST_CHECK(entries.empty());
}

BOOST_AUTO_TEST_CASE(scheduled_sweep_notifies_ui_at_expiry)
{
    FakeNodeClock clock{100s};
    CClientUIInterface client_interface;
    std::atomic<int> notifications{0};
    auto connection{client_interface.BannedListChanged.connect([&] { ++notifications; })};

    const fs::path banlist_path{m_args.GetDataDirBase() / "banlist_scheduled_expiry_test"};
    BanMan banman{banlist_path, &client_interface, /*default_ban_time=*/0};
    ScopedScheduler scheduler;
    banman.SetScheduler(scheduler.scheduler);

    const CSubNet subnet{LookupSubNet("2.3.4.5")};
    BOOST_REQUIRE(subnet.IsValid());

    banman.Ban(subnet, /*ban_time_offset=*/110, /*since_unix_epoch=*/true);
    BOOST_CHECK_EQUAL(notifications.load(), 1);

    banman.EnsureSweepScheduled();
    clock.set(110s);
    scheduler.MockForwardAndSync(10s);

    banmap_t entries;
    banman.GetBanned(entries);
    BOOST_CHECK(entries.empty());
    BOOST_CHECK_EQUAL(notifications.load(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
