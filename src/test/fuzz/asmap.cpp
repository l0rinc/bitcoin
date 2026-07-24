// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/asmap.h>

#include <netaddress.h>
#include <netgroup.h>
#include <test/fuzz/fuzz.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <span>
#include <vector>

using namespace util::hex_literals;

//! asmap code that consumes nothing
static const std::vector<std::byte> IPV6_PREFIX_ASMAP = {};

//! asmap code that consumes the 96 prefix bits of ::ffff:0/96 (IPv4-in-IPv6 map)
static const auto IPV4_PREFIX_ASMAP = "fb03ec0fb03fc0fe00fb03ec0fb03fc0fe00fb03ec0fb0fffffeff"_hex_v;

FUZZ_TARGET(asmap)
{
    // Encoding: [7 bits: asmap size] [1 bit: ipv6?] [3-130 bytes: asmap] [4 or 16 bytes: addr]
    if (buffer.size() < 1 + 3 + 4) return;
    int asmap_size = 3 + (buffer[0] & 127);
    bool ipv6 = buffer[0] & 128;
    const size_t addr_size = ipv6 ? ADDR_IPV6_SIZE : ADDR_IPV4_SIZE;
    if (buffer.size() < size_t(1 + asmap_size + addr_size)) return;
    std::vector<std::byte> asmap = ipv6 ? IPV6_PREFIX_ASMAP : IPV4_PREFIX_ASMAP;
    std::ranges::copy(std::as_bytes(buffer.subspan(1, asmap_size)), std::back_inserter(asmap));
    if (!CheckStandardAsmap(asmap)) return;

    const uint8_t* addr_data = buffer.data() + 1 + asmap_size;
    CNetAddr net_addr;
    if (ipv6) {
        assert(addr_size == ADDR_IPV6_SIZE);
        net_addr.SetLegacyIPv6({addr_data, addr_size});
    } else {
        assert(addr_size == ADDR_IPV4_SIZE);
        in_addr ipv4;
        memcpy(&ipv4, addr_data, addr_size);
        net_addr.SetIP(CNetAddr{ipv4});
    }
    auto netgroupman{NetGroupManager::WithEmbeddedAsmap(asmap)};
    assert(netgroupman.UsingASMap());
    assert(netgroupman.GetAsmapVersion() == AsmapVersion(asmap));

    std::vector<std::byte> ip_bytes(ADDR_IPV6_SIZE);
    if (net_addr.HasLinkedIPv4()) {
        const auto prefix{std::as_bytes(std::span{IPV4_IN_IPV6_PREFIX})};
        std::copy_n(prefix.begin(), prefix.size(), ip_bytes.begin());
        const uint32_t ipv4{net_addr.GetLinkedIPv4()};
        for (int i = 0; i < static_cast<int>(ADDR_IPV4_SIZE); ++i) {
            ip_bytes[ADDR_IPV6_SIZE - ADDR_IPV4_SIZE + i] = std::byte((ipv4 >> (24 - i * 8)) & 0xff);
        }
    } else {
        const std::vector<unsigned char> addr_bytes{net_addr.GetAddrBytes()};
        assert(addr_bytes.size() == ADDR_IPV6_SIZE);
        std::ranges::copy(std::as_bytes(std::span{addr_bytes}), ip_bytes.begin());
    }
    const bool asmap_applicable{net_addr.GetNetClass() == NET_IPV4 || net_addr.GetNetClass() == NET_IPV6};
    const uint32_t expected_asn{asmap_applicable ? Interpret(asmap, ip_bytes) : 0};
    assert(netgroupman.GetMappedAS(net_addr) == expected_asn);

    const std::vector<unsigned char> group{netgroupman.GetGroup(net_addr)};
    if (expected_asn != 0) {
        const std::vector<unsigned char> expected_group{
            static_cast<unsigned char>(NET_IPV6),
            static_cast<unsigned char>(expected_asn),
            static_cast<unsigned char>(expected_asn >> 8),
            static_cast<unsigned char>(expected_asn >> 16),
            static_cast<unsigned char>(expected_asn >> 24),
        };
        assert(group == expected_group);
    } else {
        assert(group == NetGroupManager::NoAsmap().GetGroup(net_addr));
    }
}
