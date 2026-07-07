// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_permissions.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <util/string.h>
#include <util/translation.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

namespace {

using NetPermissionBits = std::underlying_type_t<NetPermissionFlags>;

std::vector<std::string> ExpectedPermissionStrings(NetPermissionFlags flags)
{
    std::vector<std::string> strings;
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::BloomFilter)) strings.emplace_back("bloomfilter");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::NoBan)) strings.emplace_back("noban");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::ForceRelay)) strings.emplace_back("forcerelay");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::Relay)) strings.emplace_back("relay");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::Mempool)) strings.emplace_back("mempool");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::Download)) strings.emplace_back("download");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::Addr)) strings.emplace_back("addr");
    return strings;
}

void AssertPermissionStringContracts(NetPermissionFlags flags)
{
    const auto strings{NetPermissions::ToStrings(flags)};
    assert(strings == ExpectedPermissionStrings(flags));

    NetWhitebindPermissions reparsed;
    bilingual_str error;
    assert(NetWhitebindPermissions::TryParse(util::Join(strings, ",") + "@1.2.3.4:32", reparsed, error));
    assert(error.empty());
    assert(!NetPermissions::HasFlag(reparsed.m_flags, NetPermissionFlags::Implicit));
    assert(NetPermissions::ToStrings(reparsed.m_flags) == strings);
}

void AssertClearImplicitPreservesExplicit(NetPermissionFlags before, NetPermissionFlags after)
{
    static constexpr auto implicit{static_cast<NetPermissionBits>(NetPermissionFlags::Implicit)};
    const auto before_bits{static_cast<NetPermissionBits>(before)};
    const auto after_bits{static_cast<NetPermissionBits>(after)};
    assert((after_bits & implicit) == 0);
    assert((after_bits & ~implicit) == (before_bits & ~implicit));
}

} // namespace

FUZZ_TARGET(net_permissions)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string s = fuzzed_data_provider.ConsumeRandomLengthString(1000);
    const NetPermissionFlags net_permission_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
    AssertPermissionStringContracts(net_permission_flags);

    NetPermissionFlags flags_with_implicit{net_permission_flags};
    NetPermissions::AddFlag(flags_with_implicit, NetPermissionFlags::Implicit);
    AssertPermissionStringContracts(flags_with_implicit);
    const auto random_flags_before_clear{flags_with_implicit};
    NetPermissions::ClearFlag(flags_with_implicit, NetPermissionFlags::Implicit);
    AssertClearImplicitPreservesExplicit(random_flags_before_clear, flags_with_implicit);
    AssertPermissionStringContracts(flags_with_implicit);

    NetWhitebindPermissions net_whitebind_permissions;
    bilingual_str error_net_whitebind_permissions;
    if (NetWhitebindPermissions::TryParse(s, net_whitebind_permissions, error_net_whitebind_permissions)) {
        AssertPermissionStringContracts(net_whitebind_permissions.m_flags);
        (void)NetPermissions::AddFlag(net_whitebind_permissions.m_flags, net_permission_flags);
        assert(NetPermissions::HasFlag(net_whitebind_permissions.m_flags, net_permission_flags));
        AssertPermissionStringContracts(net_whitebind_permissions.m_flags);
        const auto flags_before_clear{net_whitebind_permissions.m_flags};
        NetPermissions::ClearFlag(net_whitebind_permissions.m_flags, NetPermissionFlags::Implicit);
        AssertClearImplicitPreservesExplicit(flags_before_clear, net_whitebind_permissions.m_flags);
        AssertPermissionStringContracts(net_whitebind_permissions.m_flags);
    }

    NetWhitelistPermissions net_whitelist_permissions;
    net_whitelist_permissions.m_flags = NetPermissionFlags::All;
    net_whitelist_permissions.m_subnet = CSubNet{LookupSubNet("1.2.3.4/32")};
    ConnectionDirection connection_direction{ConnectionDirection::Both};
    const auto original_whitelist_permissions{net_whitelist_permissions};
    const auto original_connection_direction{connection_direction};
    bilingual_str error_net_whitelist_permissions;
    if (NetWhitelistPermissions::TryParse(
            s, net_whitelist_permissions, connection_direction, error_net_whitelist_permissions)) {
        AssertPermissionStringContracts(net_whitelist_permissions.m_flags);
        (void)NetPermissions::AddFlag(net_whitelist_permissions.m_flags, net_permission_flags);
        assert(NetPermissions::HasFlag(net_whitelist_permissions.m_flags, net_permission_flags));
        AssertPermissionStringContracts(net_whitelist_permissions.m_flags);
        const auto flags_before_clear{net_whitelist_permissions.m_flags};
        NetPermissions::ClearFlag(net_whitelist_permissions.m_flags, NetPermissionFlags::Implicit);
        AssertClearImplicitPreservesExplicit(flags_before_clear, net_whitelist_permissions.m_flags);
        AssertPermissionStringContracts(net_whitelist_permissions.m_flags);
    } else {
        assert(net_whitelist_permissions.m_flags == original_whitelist_permissions.m_flags);
        assert(net_whitelist_permissions.m_subnet == original_whitelist_permissions.m_subnet);
        assert(connection_direction == original_connection_direction);
    }
}
