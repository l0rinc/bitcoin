// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <common/args.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/fs.h>
#include <util/overflow.h>
#include <util/readwritefile.h>
#include <util/time.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
int64_t ConsumeBanTimeOffset(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return fuzzed_data_provider.ConsumeIntegral<int64_t>();
}

void AssertNoBannedEntries(BanMan& ban_man)
{
    banmap_t banmap;
    ban_man.GetBanned(banmap);
    assert(banmap.empty());
}

void AssertSubnetUnbanned(BanMan& ban_man, const CSubNet& subnet)
{
    banmap_t banmap;
    ban_man.GetBanned(banmap);
    assert(!banmap.contains(subnet));
    assert(!ban_man.IsBanned(subnet));
}

void AssertBannedEntriesActive(BanMan& ban_man)
{
    banmap_t banmap;
    ban_man.GetBanned(banmap);
    const int64_t now{GetTime()};
    for (const auto& [subnet, ban_entry] : banmap) {
        assert(subnet.IsValid());
        assert(now < ban_entry.nBanUntil);
        assert(ban_man.IsBanned(subnet));
    }
}

int64_t ExpectedBanUntil(int64_t now, int64_t default_ban_time, int64_t ban_time_offset, bool since_unix_epoch)
{
    if (ban_time_offset <= 0) {
        return SaturatingAdd(now, default_ban_time);
    }
    return since_unix_epoch ? ban_time_offset : SaturatingAdd(now, ban_time_offset);
}

void AssertBanTransition(BanMan& ban_man, const CSubNet& subnet, const banmap_t& before, int64_t expected_ban_until, int64_t now)
{
    banmap_t after;
    ban_man.GetBanned(after);

    std::optional<int64_t> expected_active_until;
    if (const auto before_it{before.find(subnet)}; before_it != before.end()) {
        expected_active_until = before_it->second.nBanUntil;
    }
    if (now < expected_ban_until && (!expected_active_until || *expected_active_until < expected_ban_until)) {
        expected_active_until = expected_ban_until;
    }

    const auto after_it{after.find(subnet)};
    if (!expected_active_until) {
        assert(after_it == after.end());
        assert(!ban_man.IsBanned(subnet));
    } else {
        assert(after_it != after.end());
        assert(after_it->second.nBanUntil == *expected_active_until);
        assert(ban_man.IsBanned(subnet));
    }
}
} // namespace

void initialize_banman()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

static bool operator==(const CBanEntry& lhs, const CBanEntry& rhs)
{
    return lhs.nVersion == rhs.nVersion &&
           lhs.nCreateTime == rhs.nCreateTime &&
           lhs.nBanUntil == rhs.nBanUntil;
}

FUZZ_TARGET(banman, .init = initialize_banman)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    fs::path banlist_file = gArgs.GetDataDirNet() / "fuzzed_banlist";

    const bool start_with_corrupted_banlist{fuzzed_data_provider.ConsumeBool()};
    bool force_read_and_write_to_err{false};
    if (start_with_corrupted_banlist) {
        assert(WriteBinaryFile(banlist_file + ".json",
                               fuzzed_data_provider.ConsumeRandomLengthString()));
    } else {
        force_read_and_write_to_err = fuzzed_data_provider.ConsumeBool();
        if (force_read_and_write_to_err) {
            banlist_file = fs::path{"path"} / "to" / "inaccessible" / "fuzzed_banlist";
        }
    }

    {
        const int64_t default_ban_time{ConsumeBanTimeOffset(fuzzed_data_provider)};
        BanMan ban_man{banlist_file, /*client_interface=*/nullptr, /*default_ban_time=*/default_ban_time};
        // The complexity is O(N^2), where N is the input size, because each call
        // might call DumpBanlist (or other methods that are at least linear
        // complexity of the input size).
        bool contains_invalid{false};
        LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 300) {
            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    CNetAddr net_addr{ConsumeNetAddr(fuzzed_data_provider)};
                    if (!net_addr.IsCJDNS() || !net_addr.IsValid()) {
                        const std::optional<CNetAddr>& addr{LookupHost(net_addr.ToStringAddr(), /*fAllowLookup=*/false)};
                        if (addr.has_value() && addr->IsValid()) {
                            net_addr = *addr;
                        } else {
                            contains_invalid = true;
                        }
                    }
                    auto ban_time_offset = ConsumeBanTimeOffset(fuzzed_data_provider);
                    auto since_unix_epoch = fuzzed_data_provider.ConsumeBool();
                    banmap_t before;
                    if (net_addr.IsValid()) ban_man.GetBanned(before);
                    const int64_t now{GetTime()};
                    ban_man.Ban(net_addr, ban_time_offset, since_unix_epoch);
                    if (net_addr.IsValid()) {
                        const CSubNet subnet{net_addr};
                        AssertBanTransition(ban_man, subnet, before, ExpectedBanUntil(now, default_ban_time, ban_time_offset, since_unix_epoch), now);
                    }
                },
                [&] {
                    CSubNet subnet{ConsumeSubNet(fuzzed_data_provider)};
                    subnet = LookupSubNet(subnet.ToString());
                    if (!subnet.IsValid()) {
                        contains_invalid = true;
                    }
                    auto ban_time_offset = ConsumeBanTimeOffset(fuzzed_data_provider);
                    auto since_unix_epoch = fuzzed_data_provider.ConsumeBool();
                    banmap_t before;
                    if (subnet.IsValid()) ban_man.GetBanned(before);
                    const int64_t now{GetTime()};
                    ban_man.Ban(subnet, ban_time_offset, since_unix_epoch);
                    if (subnet.IsValid()) {
                        AssertBanTransition(ban_man, subnet, before, ExpectedBanUntil(now, default_ban_time, ban_time_offset, since_unix_epoch), now);
                    }
                },
                [&] {
                    ban_man.ClearBanned();
                    AssertNoBannedEntries(ban_man);
                },
                [&] {
                    ban_man.IsBanned(ConsumeNetAddr(fuzzed_data_provider));
                },
                [&] {
                    ban_man.IsBanned(ConsumeSubNet(fuzzed_data_provider));
                },
                [&] {
                    const CNetAddr net_addr{ConsumeNetAddr(fuzzed_data_provider)};
                    const CSubNet subnet{net_addr};
                    const bool unbanned{ban_man.Unban(net_addr)};
                    if (unbanned) {
                        AssertSubnetUnbanned(ban_man, subnet);
                    }
                },
                [&] {
                    const CSubNet subnet{ConsumeSubNet(fuzzed_data_provider)};
                    const bool unbanned{ban_man.Unban(subnet)};
                    if (unbanned) {
                        AssertSubnetUnbanned(ban_man, subnet);
                    }
                },
                [&] {
                    banmap_t banmap;
                    ban_man.GetBanned(banmap);
                    AssertBannedEntriesActive(ban_man);
                },
                [&] {
                    ban_man.DumpBanlist();
                    AssertBannedEntriesActive(ban_man);
                },
                [&] {
                    clock.set(ConsumeTime(fuzzed_data_provider));
                    AssertBannedEntriesActive(ban_man);
                },
                [&] {
                    ban_man.Discourage(ConsumeNetAddr(fuzzed_data_provider));
                },
                [&] {
                    ban_man.IsDiscouraged(ConsumeNetAddr(fuzzed_data_provider));
                });
        }
        if (!force_read_and_write_to_err) {
            ban_man.DumpBanlist();
            clock.set(ConsumeTime(fuzzed_data_provider));
            banmap_t banmap;
            ban_man.GetBanned(banmap);
            AssertBannedEntriesActive(ban_man);
            BanMan ban_man_read{banlist_file, /*client_interface=*/nullptr, /*default_ban_time=*/0};
            banmap_t banmap_read;
            ban_man_read.GetBanned(banmap_read);
            AssertBannedEntriesActive(ban_man_read);
            if (!contains_invalid) {
                assert(banmap == banmap_read);
            }
        }
    }
    fs::remove(fs::PathToString(banlist_file + ".json"));
}
