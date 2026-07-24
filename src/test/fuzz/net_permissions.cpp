// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_permissions.h>

#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <util/translation.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

namespace {

using PermissionUnderlying = std::underlying_type_t<NetPermissionFlags>;

PermissionUnderlying PermissionBits(NetPermissionFlags flags)
{
    return static_cast<PermissionUnderlying>(flags);
}

bool ModelHasFlag(NetPermissionFlags flags, NetPermissionFlags permission)
{
    return (PermissionBits(flags) & PermissionBits(permission)) == PermissionBits(permission);
}

std::vector<std::string> ExpectedPermissionStrings(NetPermissionFlags flags)
{
    std::vector<std::string> result;
    if (ModelHasFlag(flags, NetPermissionFlags::BloomFilter)) result.emplace_back("bloomfilter");
    if (ModelHasFlag(flags, NetPermissionFlags::NoBan)) result.emplace_back("noban");
    if (ModelHasFlag(flags, NetPermissionFlags::ForceRelay)) result.emplace_back("forcerelay");
    if (ModelHasFlag(flags, NetPermissionFlags::Relay)) result.emplace_back("relay");
    if (ModelHasFlag(flags, NetPermissionFlags::Mempool)) result.emplace_back("mempool");
    if (ModelHasFlag(flags, NetPermissionFlags::Download)) result.emplace_back("download");
    if (ModelHasFlag(flags, NetPermissionFlags::Addr)) result.emplace_back("addr");
    return result;
}

std::string JoinPermissionStrings(NetPermissionFlags flags)
{
    std::string result;
    for (const std::string& permission : ExpectedPermissionStrings(flags)) {
        if (!result.empty()) result += ',';
        result += permission;
    }
    return result;
}

void AppendDirection(std::string& result, ConnectionDirection direction)
{
    if (direction & ConnectionDirection::In) {
        if (!result.empty()) result += ',';
        result += "in";
    }
    if (direction & ConnectionDirection::Out) {
        if (!result.empty()) result += ',';
        result += "out";
    }
}

void AssertFlagContracts(NetPermissionFlags flags, NetPermissionFlags added)
{
    constexpr NetPermissionFlags ALL_FLAGS[]{
        NetPermissionFlags::None,
        NetPermissionFlags::BloomFilter,
        NetPermissionFlags::Relay,
        NetPermissionFlags::ForceRelay,
        NetPermissionFlags::NoBan,
        NetPermissionFlags::Mempool,
        NetPermissionFlags::Download,
        NetPermissionFlags::Addr,
        NetPermissionFlags::Implicit,
        NetPermissionFlags::All,
    };

    for (const NetPermissionFlags permission : ALL_FLAGS) {
        assert(NetPermissions::HasFlag(flags, permission) == ModelHasFlag(flags, permission));
    }
    assert(NetPermissions::ToStrings(flags) == ExpectedPermissionStrings(flags));

    const PermissionUnderlying before{PermissionBits(flags)};
    NetPermissions::AddFlag(flags, added);
    assert(PermissionBits(flags) == (before | PermissionBits(added)));
    assert((PermissionBits(flags) & before) == before);
    assert(NetPermissions::HasFlag(flags, added) == ModelHasFlag(flags, added));
    assert(NetPermissions::ToStrings(flags) == ExpectedPermissionStrings(flags));

    NetPermissions::ClearFlag(flags, NetPermissionFlags::Implicit);
    assert(PermissionBits(flags) == ((before | PermissionBits(added)) & ~PermissionBits(NetPermissionFlags::Implicit)));
    assert(!NetPermissions::HasFlag(flags, NetPermissionFlags::Implicit));
    assert(NetPermissions::ToStrings(flags) == ExpectedPermissionStrings(flags));
}

void AssertWhitebindRoundTrip(const NetWhitebindPermissions& permissions)
{
    const std::string canonical{
        ModelHasFlag(permissions.m_flags, NetPermissionFlags::Implicit) ? permissions.m_service.ToStringAddrPort() : JoinPermissionStrings(permissions.m_flags) + "@" + permissions.m_service.ToStringAddrPort()};

    NetWhitebindPermissions reparsed;
    bilingual_str error;
    assert(NetWhitebindPermissions::TryParse(canonical, reparsed, error));
    assert(error.empty());
    assert(reparsed.m_flags == permissions.m_flags);
    assert(reparsed.m_service == permissions.m_service);
    assert(reparsed.m_service.GetPort() != 0);
}

void AssertWhitelistRoundTrip(const NetWhitelistPermissions& permissions, ConnectionDirection direction)
{
    std::string canonical;
    if (ModelHasFlag(permissions.m_flags, NetPermissionFlags::Implicit)) {
        canonical = permissions.m_subnet.ToString();
    } else {
        canonical = JoinPermissionStrings(permissions.m_flags);
        if (!canonical.empty()) AppendDirection(canonical, direction);
        canonical += "@" + permissions.m_subnet.ToString();
    }

    NetWhitelistPermissions reparsed;
    ConnectionDirection reparsed_direction{ConnectionDirection::None};
    bilingual_str error;
    assert(NetWhitelistPermissions::TryParse(canonical, reparsed, reparsed_direction, error));
    assert(error.empty());
    assert(reparsed.m_flags == permissions.m_flags);
    assert(reparsed.m_subnet == permissions.m_subnet);
    assert(reparsed_direction == direction);
}

} // namespace

FUZZ_TARGET(net_permissions)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string s = fuzzed_data_provider.ConsumeRandomLengthString(1000);
    const NetPermissionFlags net_permission_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);

    NetWhitebindPermissions net_whitebind_permissions;
    bilingual_str error_net_whitebind_permissions;
    if (NetWhitebindPermissions::TryParse(s, net_whitebind_permissions, error_net_whitebind_permissions)) {
        assert(error_net_whitebind_permissions.empty());
        AssertFlagContracts(net_whitebind_permissions.m_flags, net_permission_flags);
        AssertWhitebindRoundTrip(net_whitebind_permissions);
    } else {
        assert(!error_net_whitebind_permissions.empty());
    }

    NetWhitelistPermissions net_whitelist_permissions;
    ConnectionDirection connection_direction{ConnectionDirection::None};
    bilingual_str error_net_whitelist_permissions;
    if (NetWhitelistPermissions::TryParse(s, net_whitelist_permissions, connection_direction, error_net_whitelist_permissions)) {
        assert(error_net_whitelist_permissions.empty());
        assert(connection_direction == ConnectionDirection::In || connection_direction == ConnectionDirection::Out || connection_direction == ConnectionDirection::Both);
        AssertFlagContracts(net_whitelist_permissions.m_flags, net_permission_flags);
        AssertWhitelistRoundTrip(net_whitelist_permissions, connection_direction);
    } else {
        assert(!error_net_whitelist_permissions.empty());
    }
}
