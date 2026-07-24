// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>

#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util/net.h>
#include <test/util/random.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <span>
#include <vector>

namespace {

bool HasEmbeddedLegacyPrefix(const CNetAddr& net_addr, std::span<const uint8_t> prefix)
{
    if (!net_addr.IsIPv6()) return false;

    const auto address_bytes{net_addr.GetAddrBytes()};
    return address_bytes.size() == ADDR_IPV6_SIZE && std::equal(prefix.begin(), prefix.end(), address_bytes.begin());
}

void AssertNetAddrSerialization(const CNetAddr& net_addr)
{
    const bool embedded_ipv4_alias{HasEmbeddedLegacyPrefix(net_addr, IPV4_IN_IPV6_PREFIX)};
    const bool embedded_torv2_alias{HasEmbeddedLegacyPrefix(net_addr, TORV2_IN_IPV6_PREFIX)};
    assert(!(embedded_ipv4_alias && embedded_torv2_alias));

    DataStream v2_stream;
    v2_stream << CNetAddr::V2(net_addr);
    CNetAddr v2_roundtrip;
    v2_stream >> CNetAddr::V2(v2_roundtrip);
    assert(v2_stream.empty());
    if (embedded_ipv4_alias || embedded_torv2_alias) {
        assert(!v2_roundtrip.IsValid());
    } else {
        assert(net_addr == v2_roundtrip);
    }

    DataStream v1_stream;
    v1_stream << CNetAddr::V1(net_addr);
    CNetAddr v1_roundtrip;
    v1_stream >> CNetAddr::V1(v1_roundtrip);
    assert(v1_stream.empty());
    if (embedded_ipv4_alias) {
        assert(v1_roundtrip.IsIPv4());
    } else if (embedded_torv2_alias) {
        assert(!v1_roundtrip.IsValid());
    } else if (net_addr.IsAddrV1Compatible()) {
        assert(net_addr == v1_roundtrip);
    } else {
        assert(!v1_roundtrip.IsValid());
    }
}

void AssertServiceContracts(const CService& service, const CService& other_service)
{
    const auto address_bytes{service.GetAddrBytes()};
    const auto key{service.GetKey()};
    assert(key.size() == address_bytes.size() + 2);
    assert(std::equal(address_bytes.begin(), address_bytes.end(), key.begin()));
    assert(key[address_bytes.size()] == static_cast<uint8_t>(service.GetPort() >> 8));
    assert(key[address_bytes.size() + 1] == static_cast<uint8_t>(service.GetPort() & 0xff));

    const auto& service_addr{static_cast<const CNetAddr&>(service)};
    const auto& other_service_addr{static_cast<const CNetAddr&>(other_service)};
    const bool expected_less{service_addr < other_service_addr ||
                             (service_addr == other_service_addr && service.GetPort() < other_service.GetPort())};
    assert((service < other_service) == expected_less);
    assert(!(service < service));
}

} // namespace

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
    AssertNetAddrSerialization(net_addr);

    const CSubNet sub_net{net_addr, fuzzed_data_provider.ConsumeIntegral<uint8_t>()};
    (void)sub_net.IsValid();
    (void)sub_net.ToString();
    if (sub_net.IsValid() && net_addr.IsValid() && (net_addr.IsIPv4() || net_addr.IsIPv6())) {
        assert(sub_net.Match(net_addr));
    }

    const CService service{net_addr, fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    (void)service.GetKey();
    (void)service.GetPort();
    (void)service.ToStringAddrPort();
    (void)CServiceHash()(service);
    (void)CServiceHash(0, 0)(service);

    const CNetAddr other_net_addr = ConsumeNetAddr(fuzzed_data_provider);
    AssertNetAddrSerialization(other_net_addr);
    (void)net_addr.GetReachabilityFrom(other_net_addr);
    (void)sub_net.Match(other_net_addr);

    const CService other_service{fuzzed_data_provider.ConsumeBool() ? net_addr : other_net_addr, fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    assert((service == other_service) != (service != other_service));
    (void)(service < other_service);

    if (service.ToStringAddrPort() == other_service.ToStringAddrPort()) {
        assert(static_cast<CNetAddr>(service) == static_cast<CNetAddr>(other_service));
    }
    AssertServiceContracts(service, other_service);

    const CSubNet sub_net_copy_1{net_addr, other_net_addr};
    const CSubNet sub_net_copy_2{net_addr};

    CNetAddr mutable_net_addr;
    mutable_net_addr.SetIP(net_addr);
    assert(net_addr == mutable_net_addr);
}
