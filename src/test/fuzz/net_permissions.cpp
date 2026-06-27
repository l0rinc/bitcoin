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

using NetPermissionBits = std::underlying_type_t<NetPermissionFlags>;

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

    NetPermissionFlags flags_with_implicit{net_permission_flags};
    NetPermissions::AddFlag(flags_with_implicit, NetPermissionFlags::Implicit);
    const auto random_flags_before_clear{flags_with_implicit};
    NetPermissions::ClearFlag(flags_with_implicit, NetPermissionFlags::Implicit);
    AssertClearImplicitPreservesExplicit(random_flags_before_clear, flags_with_implicit);

    NetWhitebindPermissions net_whitebind_permissions;
    bilingual_str error_net_whitebind_permissions;
    if (NetWhitebindPermissions::TryParse(s, net_whitebind_permissions, error_net_whitebind_permissions)) {
        (void)NetPermissions::ToStrings(net_whitebind_permissions.m_flags);
        (void)NetPermissions::AddFlag(net_whitebind_permissions.m_flags, net_permission_flags);
        assert(NetPermissions::HasFlag(net_whitebind_permissions.m_flags, net_permission_flags));
        const auto flags_before_clear{net_whitebind_permissions.m_flags};
        NetPermissions::ClearFlag(net_whitebind_permissions.m_flags, NetPermissionFlags::Implicit);
        AssertClearImplicitPreservesExplicit(flags_before_clear, net_whitebind_permissions.m_flags);
        (void)NetPermissions::ToStrings(net_whitebind_permissions.m_flags);
    }

    NetWhitelistPermissions net_whitelist_permissions;
    ConnectionDirection connection_direction;
    bilingual_str error_net_whitelist_permissions;
    if (NetWhitelistPermissions::TryParse(s, net_whitelist_permissions, connection_direction, error_net_whitelist_permissions)) {
        (void)NetPermissions::ToStrings(net_whitelist_permissions.m_flags);
        (void)NetPermissions::AddFlag(net_whitelist_permissions.m_flags, net_permission_flags);
        assert(NetPermissions::HasFlag(net_whitelist_permissions.m_flags, net_permission_flags));
        const auto flags_before_clear{net_whitelist_permissions.m_flags};
        NetPermissions::ClearFlag(net_whitelist_permissions.m_flags, NetPermissionFlags::Implicit);
        AssertClearImplicitPreservesExplicit(flags_before_clear, net_whitelist_permissions.m_flags);
        (void)NetPermissions::ToStrings(net_whitelist_permissions.m_flags);
    }
}
