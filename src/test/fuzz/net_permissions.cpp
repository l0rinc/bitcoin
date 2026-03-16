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
#include <vector>

namespace {
std::string JoinPermissions(const std::vector<std::string>& permissions)
{
    std::string result;
    for (size_t i = 0; i < permissions.size(); ++i) {
        if (i != 0) result += ",";
        result += permissions[i];
    }
    return result;
}

std::string CanonicalWhitebindString(const NetWhitebindPermissions& permissions)
{
    if (NetPermissions::HasFlag(permissions.m_flags, NetPermissionFlags::Implicit)) {
        return permissions.m_service.ToStringAddrPort();
    }

    return JoinPermissions(NetPermissions::ToStrings(permissions.m_flags)) + "@" + permissions.m_service.ToStringAddrPort();
}

std::string CanonicalWhitelistString(const NetWhitelistPermissions& permissions, ConnectionDirection connection_direction)
{
    if (NetPermissions::HasFlag(permissions.m_flags, NetPermissionFlags::Implicit)) {
        return permissions.m_subnet.ToString();
    }

    std::vector<std::string> flags = NetPermissions::ToStrings(permissions.m_flags);
    if (connection_direction != ConnectionDirection::In) {
        if (connection_direction & ConnectionDirection::In) flags.emplace_back("in");
        if (connection_direction & ConnectionDirection::Out) flags.emplace_back("out");
    }
    return JoinPermissions(flags) + "@" + permissions.m_subnet.ToString();
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
        (void)NetPermissions::ToStrings(net_whitebind_permissions.m_flags);
        NetWhitebindPermissions roundtrip_whitebind_permissions;
        bilingual_str roundtrip_error;
        const std::string canonical_string = CanonicalWhitebindString(net_whitebind_permissions);
        assert(NetWhitebindPermissions::TryParse(canonical_string, roundtrip_whitebind_permissions, roundtrip_error));
        assert(roundtrip_whitebind_permissions.m_flags == net_whitebind_permissions.m_flags);
        assert(roundtrip_whitebind_permissions.m_service == net_whitebind_permissions.m_service);

        (void)NetPermissions::AddFlag(net_whitebind_permissions.m_flags, net_permission_flags);
        assert(NetPermissions::HasFlag(net_whitebind_permissions.m_flags, net_permission_flags));
        (void)NetPermissions::ClearFlag(net_whitebind_permissions.m_flags, NetPermissionFlags::Implicit);
        (void)NetPermissions::ToStrings(net_whitebind_permissions.m_flags);
    }

    NetWhitelistPermissions net_whitelist_permissions;
    ConnectionDirection connection_direction;
    bilingual_str error_net_whitelist_permissions;
    if (NetWhitelistPermissions::TryParse(s, net_whitelist_permissions, connection_direction, error_net_whitelist_permissions)) {
        (void)NetPermissions::ToStrings(net_whitelist_permissions.m_flags);
        NetWhitelistPermissions roundtrip_whitelist_permissions;
        ConnectionDirection roundtrip_connection_direction;
        bilingual_str roundtrip_error;
        const std::string canonical_string = CanonicalWhitelistString(net_whitelist_permissions, connection_direction);
        assert(NetWhitelistPermissions::TryParse(canonical_string, roundtrip_whitelist_permissions, roundtrip_connection_direction, roundtrip_error));
        assert(roundtrip_whitelist_permissions.m_flags == net_whitelist_permissions.m_flags);
        assert(roundtrip_whitelist_permissions.m_subnet == net_whitelist_permissions.m_subnet);
        assert(roundtrip_connection_direction == connection_direction);

        (void)NetPermissions::AddFlag(net_whitelist_permissions.m_flags, net_permission_flags);
        assert(NetPermissions::HasFlag(net_whitelist_permissions.m_flags, net_permission_flags));
        (void)NetPermissions::ClearFlag(net_whitelist_permissions.m_flags, NetPermissionFlags::Implicit);
        (void)NetPermissions::ToStrings(net_whitelist_permissions.m_flags);
    }
}
