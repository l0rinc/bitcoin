// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netgroup.h>
#include <test/fuzz/fuzz.h>
#include <util/asmap.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace util::hex_literals;

namespace {

//! asmap code that consumes nothing
static const std::vector<std::byte> IPV6_PREFIX_ASMAP = {};

//! asmap code that consumes the 96 prefix bits of ::ffff:0/96 (IPv4-in-IPv6 map)
static const auto IPV4_PREFIX_ASMAP = "fb03ec0fb03fc0fe00fb03ec0fb03fc0fe00fb03ec0fb0fffffeff"_hex_v;

std::vector<std::byte> AsmapLookupBytes(const CNetAddr& address)
{
    std::vector<std::byte> ip_bytes(16);
    if (address.HasLinkedIPv4()) {
        const auto prefix{std::as_bytes(std::span{IPV4_IN_IPV6_PREFIX})};
        std::copy_n(prefix.begin(), IPV4_IN_IPV6_PREFIX.size(), ip_bytes.begin());
        const uint32_t ipv4{address.GetLinkedIPv4()};
        for (int i{0}; i < 4; ++i) {
            ip_bytes[12 + i] = std::byte((ipv4 >> (24 - i * 8)) & 0xFF);
        }
    } else {
        assert(address.IsIPv6());
        const auto addr_bytes{address.GetAddrBytes()};
        assert(addr_bytes.size() == ip_bytes.size());
        std::ranges::copy(std::as_bytes(std::span{addr_bytes}), ip_bytes.begin());
    }
    return ip_bytes;
}

} // namespace

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
    const uint32_t mapped_as{netgroupman.GetMappedAS(net_addr)};
    const Network net_class{net_addr.GetNetClass()};
    if (net_class == NET_IPV4 || net_class == NET_IPV6) {
        assert(mapped_as == Interpret(asmap, AsmapLookupBytes(net_addr)));
    } else {
        assert(mapped_as == 0);
    }
}
