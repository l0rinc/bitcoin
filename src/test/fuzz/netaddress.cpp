// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util/net.h>
#include <test/util/random.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

FUZZ_TARGET(netaddress)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const CNetAddr net_addr = ConsumeNetAddr(fuzzed_data_provider);
    (void)net_addr.GetNetClass();
    if (net_addr.GetNetwork() == Network::NET_IPV4) {
        assert(net_addr.IsIPv4());
    }
    if (net_addr.GetNetwork() == Network::NET_IPV6) {
        assert(net_addr.IsIPv6());
    }
    if (net_addr.GetNetwork() == Network::NET_ONION) {
        assert(net_addr.IsTor());
    }
    if (net_addr.GetNetwork() == Network::NET_I2P) {
        assert(net_addr.IsI2P());
    }
    if (net_addr.GetNetwork() == Network::NET_CJDNS) {
        assert(net_addr.IsCJDNS());
    }
    if (net_addr.GetNetwork() == Network::NET_INTERNAL) {
        assert(net_addr.IsInternal());
    }
    if (net_addr.GetNetwork() == Network::NET_UNROUTABLE) {
        assert(!net_addr.IsRoutable());
    }
    (void)net_addr.IsBindAny();
    if (net_addr.IsInternal()) {
        assert(net_addr.GetNetwork() == Network::NET_INTERNAL);
    }
    if (net_addr.IsIPv4()) {
        assert(net_addr.GetNetwork() == Network::NET_IPV4 || net_addr.GetNetwork() == Network::NET_UNROUTABLE);
    }
    if (net_addr.IsIPv6()) {
        assert(net_addr.GetNetwork() == Network::NET_IPV6 || net_addr.GetNetwork() == Network::NET_UNROUTABLE);
    }
    (void)net_addr.IsLocal();
    if (net_addr.IsRFC1918() || net_addr.IsRFC2544() || net_addr.IsRFC6598() || net_addr.IsRFC5737() || net_addr.IsRFC3927()) {
        assert(net_addr.IsIPv4());
    }
    (void)net_addr.IsRFC2544();
    if (net_addr.IsRFC3849() || net_addr.IsRFC3964() || net_addr.IsRFC4380() || net_addr.IsRFC4843() || net_addr.IsRFC7343() || net_addr.IsRFC4862() || net_addr.IsRFC6052() || net_addr.IsRFC6145()) {
        assert(net_addr.IsIPv6());
    }
    (void)net_addr.IsRFC3927();
    (void)net_addr.IsRFC3964();
    if (net_addr.IsRFC4193()) {
        assert(net_addr.GetNetwork() == Network::NET_INTERNAL || net_addr.GetNetwork() == Network::NET_UNROUTABLE);
    }
    (void)net_addr.IsRFC4380();
    (void)net_addr.IsRFC4843();
    (void)net_addr.IsRFC4862();
    (void)net_addr.IsRFC5737();
    (void)net_addr.IsRFC6052();
    (void)net_addr.IsRFC6145();
    (void)net_addr.IsRFC6598();
    (void)net_addr.IsRFC7343();
    if (net_addr.HasLinkedIPv4()) {
        assert(net_addr.IsRoutable());
        assert(net_addr.GetNetClass() == Network::NET_IPV4);
        (void)net_addr.GetLinkedIPv4();
    }
    if (!net_addr.IsRoutable()) {
        assert(net_addr.GetNetwork() == Network::NET_UNROUTABLE || net_addr.GetNetwork() == Network::NET_INTERNAL);
    }
    if (net_addr.IsTor()) {
        assert(net_addr.GetNetwork() == Network::NET_ONION);
    }
    if (net_addr.IsI2P()) {
        assert(net_addr.GetNetwork() == Network::NET_I2P);
    }
    if (net_addr.IsCJDNS()) {
        assert(net_addr.GetNetwork() == Network::NET_CJDNS);
    }
    (void)net_addr.IsValid();
    (void)net_addr.ToStringAddr();

    struct in_addr ipv4_addr;
    const bool has_ipv4_addr{net_addr.GetInAddr(&ipv4_addr)};
    assert(has_ipv4_addr == net_addr.IsIPv4());
    if (has_ipv4_addr) {
        assert(CNetAddr{ipv4_addr} == net_addr);
    }

    struct in6_addr ipv6_addr;
    const bool has_ipv6_addr{net_addr.GetIn6Addr(&ipv6_addr)};
    assert(has_ipv6_addr == (net_addr.IsIPv6() || net_addr.IsCJDNS()));
    if (has_ipv6_addr) {
        const CNetAddr legacy_ipv6{ipv6_addr};
        if (net_addr.IsIPv6()) {
            assert(legacy_ipv6 == net_addr);
        } else {
            assert(net_addr.IsCJDNS());
            assert(legacy_ipv6.IsIPv6());
            assert(legacy_ipv6.HasCJDNSPrefix());
            struct in6_addr legacy_ipv6_addr;
            assert(legacy_ipv6.GetIn6Addr(&legacy_ipv6_addr));
            assert(memcmp(&legacy_ipv6_addr, &ipv6_addr, sizeof(ipv6_addr)) == 0);
        }
    }

    const CSubNet sub_net{net_addr, fuzzed_data_provider.ConsumeIntegral<uint8_t>()};
    (void)sub_net.IsValid();
    (void)sub_net.ToString();

    const CService service{net_addr, fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    (void)service.GetKey();
    (void)service.GetPort();
    (void)service.ToStringAddrPort();
    (void)CServiceHash()(service);
    (void)CServiceHash(0, 0)(service);

    const bool service_has_sockaddr{service.IsIPv4() || service.IsIPv6() || service.IsCJDNS()};
    struct sockaddr_storage sock_addr;
    socklen_t sock_addr_len{service.GetSAFamily() == AF_INET ? static_cast<socklen_t>(sizeof(struct sockaddr_in)) :
                                                              static_cast<socklen_t>(sizeof(struct sockaddr_in6))};
    if (!service_has_sockaddr) {
        sock_addr_len = sizeof(sock_addr);
    }
    const bool got_sockaddr{service.GetSockAddr(reinterpret_cast<struct sockaddr*>(&sock_addr), &sock_addr_len)};
    assert(got_sockaddr == service_has_sockaddr);
    if (got_sockaddr) {
        assert(sock_addr_len == (service.GetSAFamily() == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)));

        CService parsed_service;
        assert(parsed_service.SetSockAddr(reinterpret_cast<const struct sockaddr*>(&sock_addr), sock_addr_len));
        if (service.IsCJDNS()) {
            assert(parsed_service.IsIPv6());
            assert(parsed_service.HasCJDNSPrefix());
            assert(parsed_service.GetPort() == service.GetPort());
            struct in6_addr service_in6;
            struct in6_addr parsed_in6;
            assert(service.GetIn6Addr(&service_in6));
            assert(parsed_service.GetIn6Addr(&parsed_in6));
            assert(memcmp(&parsed_in6, &service_in6, sizeof(service_in6)) == 0);
        } else {
            assert(parsed_service == service);
        }

        CService rejected_service{service};
        if (sock_addr_len > 0) {
            assert(!rejected_service.SetSockAddr(reinterpret_cast<const struct sockaddr*>(&sock_addr), sock_addr_len - 1));
            assert(rejected_service == service);
        }
        assert(!rejected_service.SetSockAddr(reinterpret_cast<const struct sockaddr*>(&sock_addr), sizeof(sock_addr)));
        assert(rejected_service == service);
    }

    const CNetAddr other_net_addr = ConsumeNetAddr(fuzzed_data_provider);
    (void)net_addr.GetReachabilityFrom(other_net_addr);
    (void)sub_net.Match(other_net_addr);

    const CService other_service{fuzzed_data_provider.ConsumeBool() ? net_addr : other_net_addr, fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    assert((service == other_service) != (service != other_service));
    (void)(service < other_service);

    if (service.ToStringAddrPort() == other_service.ToStringAddrPort()) {
        assert(static_cast<CNetAddr>(service) == static_cast<CNetAddr>(other_service));
    }

    const CSubNet sub_net_copy_1{net_addr, other_net_addr};
    const CSubNet sub_net_copy_2{net_addr};

    CNetAddr mutable_net_addr;
    mutable_net_addr.SetIP(net_addr);
    assert(net_addr == mutable_net_addr);
}
