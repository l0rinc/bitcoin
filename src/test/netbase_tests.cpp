// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/compat.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <netbase.h>
#include <netgroup.h>
#include <protocol.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/common.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/asmap.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std::literals;
using namespace util::hex_literals;

BOOST_FIXTURE_TEST_SUITE(netbase_tests, BasicTestingSetup)

static CNetAddr ResolveIP(const std::string& ip)
{
    return LookupHost(ip, false).value_or(CNetAddr{});
}

static void CheckSubNetCanonicalRoundTrip(const std::string& subnet_str)
{
    const CSubNet subnet{LookupSubNet(subnet_str)};
    BOOST_REQUIRE(subnet.IsValid());

    const std::string canonical{subnet.ToString()};
    const CSubNet reparsed{LookupSubNet(canonical)};
    BOOST_CHECK(reparsed.IsValid());
    BOOST_CHECK(reparsed == subnet);
    BOOST_CHECK_EQUAL(reparsed.ToString(), canonical);
}

static CNetAddr CreateInternal(const std::string& host)
{
    CNetAddr addr;
    addr.SetInternal(host);
    return addr;
}

static void PushBytes(DynSock::Pipe& pipe, const std::vector<uint8_t>& bytes)
{
    if (!bytes.empty()) pipe.PushBytes(bytes.data(), bytes.size());
}

static std::vector<uint8_t> ReadBytes(DynSock::Pipe& pipe, size_t len)
{
    std::vector<uint8_t> ret(len);
    BOOST_REQUIRE_EQUAL(pipe.GetBytes(ret.data(), ret.size()), static_cast<ssize_t>(ret.size()));
    return ret;
}

static void CheckNoBytes(DynSock::Pipe& pipe)
{
    uint8_t byte;
    BOOST_CHECK_EQUAL(pipe.GetBytes(&byte, 1), -1);
}

static std::vector<std::byte> AsmapLookupBytes(const CNetAddr& address)
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
        BOOST_REQUIRE(address.IsIPv6());
        const auto addr_bytes{address.GetAddrBytes()};
        BOOST_REQUIRE_EQUAL(addr_bytes.size(), ip_bytes.size());
        std::ranges::copy(std::as_bytes(std::span{addr_bytes}), ip_bytes.begin());
    }
    return ip_bytes;
}

static std::vector<uint8_t> Socks5MethodSelectionReply(bool select_auth)
{
    return {
        0x05, static_cast<uint8_t>(select_auth ? 0x02 : 0x00),
    };
}

static std::vector<uint8_t> Socks5AuthSuccessReply()
{
    return {0x01, 0x00};
}

static std::vector<uint8_t> Socks5ConnectSuccessReply()
{
    return {0x05, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00};
}

static std::vector<uint8_t> Socks5SuccessReply(bool select_auth)
{
    std::vector<uint8_t> reply{Socks5MethodSelectionReply(select_auth)};
    if (select_auth) {
        const auto auth_success{Socks5AuthSuccessReply()};
        reply.insert(reply.end(), auth_success.begin(), auth_success.end());
    }
    const auto connect_success{Socks5ConnectSuccessReply()};
    reply.insert(reply.end(), connect_success.begin(), connect_success.end());
    return reply;
}

static std::vector<uint8_t> ExpectedSocks5MethodSelectionBytes(const ProxyCredentials* auth)
{
    std::vector<uint8_t> expected{
        0x05,
        static_cast<uint8_t>(auth ? 0x02 : 0x01),
        0x00,
    };
    if (auth) expected.push_back(0x02);
    return expected;
}

static std::vector<uint8_t> ExpectedSocks5AuthBytes(const ProxyCredentials& auth)
{
    std::vector<uint8_t> expected{0x01, static_cast<uint8_t>(auth.username.size())};
    expected.insert(expected.end(), auth.username.begin(), auth.username.end());
    expected.push_back(static_cast<uint8_t>(auth.password.size()));
    expected.insert(expected.end(), auth.password.begin(), auth.password.end());
    return expected;
}

static std::vector<uint8_t> ExpectedSocks5ClientBytes(const std::string& dest, uint16_t port, const ProxyCredentials* auth)
{
    std::vector<uint8_t> expected{ExpectedSocks5MethodSelectionBytes(auth)};
    if (auth) {
        const auto expected_auth{ExpectedSocks5AuthBytes(*auth)};
        expected.insert(expected.end(), expected_auth.begin(), expected_auth.end());
    }
    expected.insert(expected.end(), {0x05, 0x01, 0x00, 0x03, static_cast<uint8_t>(dest.size())});
    expected.insert(expected.end(), dest.begin(), dest.end());
    expected.push_back((port >> 8) & 0xFF);
    expected.push_back(port & 0xFF);
    return expected;
}

static void CheckSocks5Transcript(const std::string& dest, uint16_t port, const ProxyCredentials* auth)
{
    g_socks5_interrupt.reset();
    auto pipes{std::make_shared<DynSock::Pipes>()};
    PushBytes(pipes->recv, Socks5SuccessReply(auth != nullptr));
    DynSock sock{pipes};

    BOOST_CHECK(Socks5(dest, port, auth, sock));
    const auto expected{ExpectedSocks5ClientBytes(dest, port, auth)};
    BOOST_CHECK(ReadBytes(pipes->send, expected.size()) == expected);
    CheckNoBytes(pipes->send);
}

static void CheckSocks5MethodSelectionFailure(const std::string& dest, uint16_t port, const ProxyCredentials* auth, uint8_t version, uint8_t method)
{
    g_socks5_interrupt.reset();
    auto pipes{std::make_shared<DynSock::Pipes>()};
    PushBytes(pipes->recv, {version, method});
    DynSock sock{pipes};

    BOOST_CHECK(!Socks5(dest, port, auth, sock));
    const auto expected{ExpectedSocks5MethodSelectionBytes(auth)};
    BOOST_CHECK(ReadBytes(pipes->send, expected.size()) == expected);
    CheckNoBytes(pipes->send);
}

static void CheckSocks5AuthFailure(const std::string& dest, uint16_t port, const ProxyCredentials& auth, std::vector<uint8_t> auth_reply)
{
    g_socks5_interrupt.reset();
    auto pipes{std::make_shared<DynSock::Pipes>()};
    const std::vector<uint8_t> sentinel{0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33};
    PushBytes(pipes->recv, Socks5MethodSelectionReply(/*select_auth=*/true));
    PushBytes(pipes->recv, auth_reply);
    PushBytes(pipes->recv, sentinel);
    DynSock sock{pipes};

    BOOST_REQUIRE(!Socks5(dest, port, &auth, sock));
    const auto expected_method{ExpectedSocks5MethodSelectionBytes(&auth)};
    BOOST_REQUIRE(ReadBytes(pipes->send, expected_method.size()) == expected_method);
    const auto expected_auth{ExpectedSocks5AuthBytes(auth)};
    BOOST_REQUIRE(ReadBytes(pipes->send, expected_auth.size()) == expected_auth);
    CheckNoBytes(pipes->send);
    BOOST_REQUIRE(ReadBytes(pipes->recv, sentinel.size()) == sentinel);
}

static void CheckSocks5ConnectFailure(const std::string& dest, uint16_t port, const ProxyCredentials* auth, std::vector<uint8_t> connect_reply)
{
    g_socks5_interrupt.reset();
    auto pipes{std::make_shared<DynSock::Pipes>()};
    const std::vector<uint8_t> sentinel{0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33};
    PushBytes(pipes->recv, Socks5MethodSelectionReply(auth != nullptr));
    if (auth) PushBytes(pipes->recv, Socks5AuthSuccessReply());
    PushBytes(pipes->recv, connect_reply);
    PushBytes(pipes->recv, sentinel);
    DynSock sock{pipes};

    BOOST_REQUIRE(!Socks5(dest, port, auth, sock));
    const auto expected{ExpectedSocks5ClientBytes(dest, port, auth)};
    BOOST_REQUIRE(ReadBytes(pipes->send, expected.size()) == expected);
    CheckNoBytes(pipes->send);
    BOOST_REQUIRE(ReadBytes(pipes->recv, sentinel.size()) == sentinel);
}

BOOST_AUTO_TEST_CASE(netbase_networks)
{
    BOOST_CHECK(ResolveIP("127.0.0.1").GetNetwork() == NET_UNROUTABLE);
    BOOST_CHECK(ResolveIP("::1").GetNetwork() == NET_UNROUTABLE);
    BOOST_CHECK(ResolveIP("8.8.8.8").GetNetwork() == NET_IPV4);
    BOOST_CHECK(ResolveIP("2001::8888").GetNetwork() == NET_IPV6);
    BOOST_CHECK(ResolveIP("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion").GetNetwork() == NET_ONION);
    BOOST_CHECK(CreateInternal("foo.com").GetNetwork() == NET_INTERNAL);
}

BOOST_AUTO_TEST_CASE(netbase_properties)
{

    BOOST_CHECK(ResolveIP("127.0.0.1").IsIPv4());
    BOOST_CHECK(ResolveIP("::FFFF:192.168.1.1").IsIPv4());
    BOOST_CHECK(ResolveIP("::1").IsIPv6());
    BOOST_CHECK(ResolveIP("10.0.0.1").IsRFC1918());
    BOOST_CHECK(ResolveIP("192.168.1.1").IsRFC1918());
    BOOST_CHECK(ResolveIP("172.31.255.255").IsRFC1918());
    BOOST_CHECK(ResolveIP("198.18.0.0").IsRFC2544());
    BOOST_CHECK(ResolveIP("198.19.255.255").IsRFC2544());
    BOOST_CHECK(ResolveIP("2001:0DB8::").IsRFC3849());
    BOOST_CHECK(ResolveIP("169.254.1.1").IsRFC3927());
    BOOST_CHECK(ResolveIP("2002::1").IsRFC3964());
    BOOST_CHECK(ResolveIP("FC00::").IsRFC4193());
    BOOST_CHECK(ResolveIP("2001::2").IsRFC4380());
    BOOST_CHECK(ResolveIP("2001:10::").IsRFC4843());
    BOOST_CHECK(ResolveIP("2001:20::").IsRFC7343());
    BOOST_CHECK(ResolveIP("FE80::").IsRFC4862());
    BOOST_CHECK(ResolveIP("64:FF9B::").IsRFC6052());
    BOOST_CHECK(ResolveIP("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion").IsTor());
    BOOST_CHECK(ResolveIP("127.0.0.1").IsLocal());
    BOOST_CHECK(ResolveIP("::1").IsLocal());
    BOOST_CHECK(ResolveIP("8.8.8.8").IsRoutable());
    BOOST_CHECK(ResolveIP("2001::1").IsRoutable());
    BOOST_CHECK(ResolveIP("127.0.0.1").IsValid());
    BOOST_CHECK(CreateInternal("FD6B:88C0:8724:edb1:8e4:3588:e546:35ca").IsInternal());
    BOOST_CHECK(CreateInternal("bar.com").IsInternal());

}

bool static TestSplitHost(const std::string& test, const std::string& host, uint16_t port, bool validPort=true)
{
    std::string hostOut;
    uint16_t portOut{0};
    bool validPortOut = SplitHostPort(test, portOut, hostOut);
    return hostOut == host && portOut == port && validPortOut == validPort;
}

BOOST_AUTO_TEST_CASE(netbase_splithost)
{
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org", "www.bitcoincore.org", 0));
    BOOST_CHECK(TestSplitHost("[www.bitcoincore.org]", "www.bitcoincore.org", 0));
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org:80", "www.bitcoincore.org", 80));
    BOOST_CHECK(TestSplitHost("[www.bitcoincore.org]:80", "www.bitcoincore.org", 80));
    BOOST_CHECK(TestSplitHost("127.0.0.1", "127.0.0.1", 0));
    BOOST_CHECK(TestSplitHost("127.0.0.1:8333", "127.0.0.1", 8333));
    BOOST_CHECK(TestSplitHost("[127.0.0.1]", "127.0.0.1", 0));
    BOOST_CHECK(TestSplitHost("[127.0.0.1]:8333", "127.0.0.1", 8333));
    BOOST_CHECK(TestSplitHost("::ffff:127.0.0.1", "::ffff:127.0.0.1", 0));
    BOOST_CHECK(TestSplitHost("[::ffff:127.0.0.1]:8333", "::ffff:127.0.0.1", 8333));
    BOOST_CHECK(TestSplitHost("[::]:8333", "::", 8333));
    BOOST_CHECK(TestSplitHost("::8333", "::8333", 0));
    BOOST_CHECK(TestSplitHost(":8333", "", 8333));
    BOOST_CHECK(TestSplitHost("[]:8333", "", 8333));
    BOOST_CHECK(TestSplitHost("", "", 0));
    BOOST_CHECK(TestSplitHost(":65535", "", 65535));
    BOOST_CHECK(TestSplitHost(":65536", ":65536", 0, false));
    BOOST_CHECK(TestSplitHost(":-1", ":-1", 0, false));
    BOOST_CHECK(TestSplitHost("[]:70001", "[]:70001", 0, false));
    BOOST_CHECK(TestSplitHost("[]:-1", "[]:-1", 0, false));
    BOOST_CHECK(TestSplitHost("[]:-0", "[]:-0", 0, false));
    BOOST_CHECK(TestSplitHost("[]:0", "", 0, false));
    BOOST_CHECK(TestSplitHost("[]:1/2", "[]:1/2", 0, false));
    BOOST_CHECK(TestSplitHost("[]:1E2", "[]:1E2", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:65536", "127.0.0.1:65536", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:0", "127.0.0.1", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:", "127.0.0.1:", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:1/2", "127.0.0.1:1/2", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:1E2", "127.0.0.1:1E2", 0, false));
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org:65536", "www.bitcoincore.org:65536", 0, false));
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org:0", "www.bitcoincore.org", 0, false));
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org:", "www.bitcoincore.org:", 0, false));
}

BOOST_AUTO_TEST_CASE(socks5_client_transcript)
{
    CheckSocks5Transcript("node.example", 8333, nullptr);

    ProxyCredentials credentials;
    credentials.username = "user";
    credentials.password = "passphrase";
    CheckSocks5Transcript("auth.example", 9050, &credentials);

    g_socks5_interrupt.reset();
    auto pipes{std::make_shared<DynSock::Pipes>()};
    DynSock sock{pipes};
    BOOST_CHECK(!Socks5(std::string(256, 'x'), 8333, nullptr, sock));
    CheckNoBytes(pipes->send);
}

BOOST_AUTO_TEST_CASE(socks5_method_selection_failure_stops_handshake)
{
    CheckSocks5MethodSelectionFailure("node.example", 8333, nullptr, 0x04, 0x00);
    CheckSocks5MethodSelectionFailure("node.example", 8333, nullptr, 0x05, 0x02);

    ProxyCredentials credentials;
    credentials.username = "user";
    credentials.password = "passphrase";
    CheckSocks5MethodSelectionFailure("auth.example", 9050, &credentials, 0x04, 0x00);
    CheckSocks5MethodSelectionFailure("auth.example", 9050, &credentials, 0x05, 0xff);
}

BOOST_AUTO_TEST_CASE(socks5_auth_failure_stops_before_connect)
{
    ProxyCredentials credentials;
    credentials.username = "user";
    credentials.password = "passphrase";

    CheckSocks5AuthFailure("auth.example", 9050, credentials, {0x00, 0x00});
    CheckSocks5AuthFailure("auth.example", 9050, credentials, {0x01, 0x01});
}

BOOST_AUTO_TEST_CASE(socks5_connect_failure_stops_after_connect)
{
    const std::vector<std::vector<uint8_t>> failure_replies{
        {0x04, 0x00, 0x00, 0x03}, // bad version
        {0x05, 0x01, 0x00, 0x03}, // non-success reply
        {0x05, 0x00, 0xff, 0x03}, // bad reserved byte
        {0x05, 0x00, 0x00, 0xff}, // unsupported address type
    };

    for (const auto& failure_reply : failure_replies) {
        CheckSocks5ConnectFailure("node.example", 8333, nullptr, failure_reply);
    }

    ProxyCredentials credentials;
    credentials.username = "user";
    credentials.password = "passphrase";
    for (const auto& failure_reply : failure_replies) {
        CheckSocks5ConnectFailure("auth.example", 9050, &credentials, failure_reply);
    }
}

bool static TestParse(std::string src, std::string canon)
{
    CService addr(LookupNumeric(src, 65535));
    return canon == addr.ToStringAddrPort();
}

BOOST_AUTO_TEST_CASE(netbase_lookupnumeric)
{
    BOOST_CHECK(TestParse("127.0.0.1", "127.0.0.1:65535"));
    BOOST_CHECK(TestParse("127.0.0.1:8333", "127.0.0.1:8333"));
    BOOST_CHECK(TestParse("::ffff:127.0.0.1", "127.0.0.1:65535"));
    BOOST_CHECK(TestParse("::", "[::]:65535"));
    BOOST_CHECK(TestParse("[::]:8333", "[::]:8333"));
    BOOST_CHECK(TestParse("[127.0.0.1]", "127.0.0.1:65535"));
    BOOST_CHECK(TestParse(":::", "[::]:0"));

    // verify that an internal address fails to resolve
    BOOST_CHECK(TestParse("[fd6b:88c0:8724:1:2:3:4:5]", "[::]:0"));
    // and that a one-off resolves correctly
    BOOST_CHECK(TestParse("[fd6c:88c0:8724:1:2:3:4:5]", "[fd6c:88c0:8724:1:2:3:4:5]:65535"));
}

BOOST_AUTO_TEST_CASE(netbase_lookup_wrappers_filter_internal_results)
{
    const std::string name{"example.com"};
    const CNetAddr first{ResolveIP("1.2.3.4")};
    const CNetAddr second{ResolveIP("5.6.7.8")};
    const std::vector<CNetAddr> dns_results{
        CreateInternal("ignored-first"),
        first,
        CreateInternal("ignored-second"),
        second,
    };
    auto dns_lookup = [&](const std::string& requested_name, bool allow_lookup) {
        BOOST_CHECK_EQUAL(requested_name, name);
        BOOST_CHECK(allow_lookup);
        return dns_results;
    };

    BOOST_CHECK(LookupHost(name, 1, /*fAllowLookup=*/true, dns_lookup) == std::vector<CNetAddr>{first});
    BOOST_CHECK(LookupHost(name, 0, /*fAllowLookup=*/true, dns_lookup) == std::vector<CNetAddr>({first, second}));

    const std::optional<CNetAddr> address{LookupHost(name, /*fAllowLookup=*/true, dns_lookup)};
    BOOST_REQUIRE(address);
    BOOST_CHECK(*address == first);

    BOOST_CHECK(Lookup(name, 8333, /*fAllowLookup=*/true, 0, dns_lookup) == std::vector<CService>({CService{first, 8333}, CService{second, 8333}}));
    const CService explicit_port_service{first, 12345};
    BOOST_CHECK(Lookup(name + ":12345", 8333, /*fAllowLookup=*/true, 1, dns_lookup) == std::vector<CService>{explicit_port_service});

    const std::optional<CService> service{Lookup(name + ":12345", 8333, /*fAllowLookup=*/true, dns_lookup)};
    BOOST_REQUIRE(service);
    BOOST_CHECK(*service == explicit_port_service);
}

BOOST_AUTO_TEST_CASE(embedded_test)
{
    CNetAddr addr1(ResolveIP("1.2.3.4"));
    CNetAddr addr2(ResolveIP("::FFFF:0102:0304"));
    BOOST_CHECK(addr2.IsIPv4());
    BOOST_CHECK_EQUAL(addr1.ToStringAddr(), addr2.ToStringAddr());
}

BOOST_AUTO_TEST_CASE(netbase_get_in_addr)
{
    const CNetAddr ipv4{ResolveIP("1.2.3.4")};
    const CNetAddr ipv6{ResolveIP("1:2:3:4:5:6:7:8")};

    struct in_addr in4;
    BOOST_REQUIRE(ipv4.GetInAddr(&in4));
    BOOST_CHECK(CNetAddr{in4} == ipv4);
    BOOST_CHECK(!ipv6.GetInAddr(&in4));

    struct in6_addr in6;
    BOOST_REQUIRE(ipv6.GetIn6Addr(&in6));
    BOOST_CHECK(CNetAddr{in6} == ipv6);
    BOOST_CHECK(!ipv4.GetIn6Addr(&in6));

    const std::vector<unsigned char> cjdns_bytes{
        CJDNS_PREFIX, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    DataStream stream;
    stream << static_cast<uint8_t>(CNetAddr::BIP155Network::CJDNS);
    stream << cjdns_bytes;
    CNetAddr cjdns;
    stream >> CAddress::V2_NETWORK(cjdns);
    BOOST_REQUIRE(cjdns.IsCJDNS());
    BOOST_REQUIRE(cjdns.GetIn6Addr(&in6));

    const auto* in6_begin{reinterpret_cast<const uint8_t*>(&in6)};
    BOOST_CHECK_EQUAL_COLLECTIONS(in6_begin, in6_begin + sizeof(in6), cjdns_bytes.begin(), cjdns_bytes.end());

    const CNetAddr cjdns_as_ipv6{in6};
    BOOST_CHECK(cjdns_as_ipv6.IsIPv6());
    BOOST_CHECK(cjdns_as_ipv6.HasCJDNSPrefix());

    struct in6_addr cjdns_as_ipv6_in6;
    BOOST_REQUIRE(cjdns_as_ipv6.GetIn6Addr(&cjdns_as_ipv6_in6));
    const auto* cjdns_as_ipv6_in6_begin{reinterpret_cast<const uint8_t*>(&cjdns_as_ipv6_in6)};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        cjdns_as_ipv6_in6_begin,
        cjdns_as_ipv6_in6_begin + sizeof(cjdns_as_ipv6_in6),
        cjdns_bytes.begin(),
        cjdns_bytes.end());
}

BOOST_AUTO_TEST_CASE(netbase_set_sock_addr)
{
    const auto check_rejected_lengths{[](CService service, const struct sockaddr_storage& storage, socklen_t exact_len) {
        const CService before{service};
        BOOST_CHECK(!service.SetSockAddr(reinterpret_cast<const struct sockaddr*>(&storage), exact_len - 1));
        BOOST_CHECK(service == before);
        BOOST_CHECK(!service.SetSockAddr(reinterpret_cast<const struct sockaddr*>(&storage), sizeof(storage)));
        BOOST_CHECK(service == before);
    }};

    const CNetAddr ipv4_addr{ResolveIP("1.2.3.4")};
    const CService ipv4_service{ipv4_addr, 0x1234};
    struct sockaddr_storage storage{};
    auto* ipv4_sock{reinterpret_cast<struct sockaddr_in*>(&storage)};
    ipv4_sock->sin_family = AF_INET;
    ipv4_sock->sin_port = htons(ipv4_service.GetPort());
    BOOST_REQUIRE(ipv4_addr.GetInAddr(&ipv4_sock->sin_addr));

    CService parsed;
    BOOST_REQUIRE(parsed.SetSockAddr(reinterpret_cast<const struct sockaddr*>(&storage), sizeof(struct sockaddr_in)));
    BOOST_CHECK(parsed == ipv4_service);
    check_rejected_lengths(parsed, storage, sizeof(struct sockaddr_in));

    const CNetAddr ipv6_addr{ResolveIP("1:2:3:4:5:6:7:8")};
    const CService ipv6_service{ipv6_addr, 0x5678};
    storage = {};
    auto* ipv6_sock{reinterpret_cast<struct sockaddr_in6*>(&storage)};
    ipv6_sock->sin6_family = AF_INET6;
    ipv6_sock->sin6_port = htons(ipv6_service.GetPort());
    BOOST_REQUIRE(ipv6_addr.GetIn6Addr(&ipv6_sock->sin6_addr));

    parsed = CService{};
    BOOST_REQUIRE(parsed.SetSockAddr(reinterpret_cast<const struct sockaddr*>(&storage), sizeof(struct sockaddr_in6)));
    BOOST_CHECK(parsed == ipv6_service);
    check_rejected_lengths(parsed, storage, sizeof(struct sockaddr_in6));

    const std::vector<unsigned char> cjdns_bytes{
        CJDNS_PREFIX, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    DataStream stream;
    stream << static_cast<uint8_t>(CNetAddr::BIP155Network::CJDNS);
    stream << cjdns_bytes;
    CNetAddr cjdns;
    stream >> CAddress::V2_NETWORK(cjdns);
    const CService cjdns_service{cjdns, 0xbeef};

    storage = {};
    auto* cjdns_sock{reinterpret_cast<struct sockaddr_in6*>(&storage)};
    cjdns_sock->sin6_family = AF_INET6;
    cjdns_sock->sin6_port = htons(cjdns_service.GetPort());
    BOOST_REQUIRE(cjdns.GetIn6Addr(&cjdns_sock->sin6_addr));

    parsed = CService{};
    BOOST_REQUIRE(parsed.SetSockAddr(reinterpret_cast<const struct sockaddr*>(&storage), sizeof(struct sockaddr_in6)));
    BOOST_CHECK(parsed.IsIPv6());
    BOOST_CHECK(parsed.HasCJDNSPrefix());
    BOOST_CHECK_EQUAL(parsed.GetPort(), cjdns_service.GetPort());
    struct in6_addr parsed_cjdns_in6;
    BOOST_REQUIRE(parsed.GetIn6Addr(&parsed_cjdns_in6));
    const auto* parsed_cjdns_begin{reinterpret_cast<const uint8_t*>(&parsed_cjdns_in6)};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        parsed_cjdns_begin,
        parsed_cjdns_begin + sizeof(parsed_cjdns_in6),
        cjdns_bytes.begin(),
        cjdns_bytes.end());
    check_rejected_lengths(cjdns_service, storage, sizeof(struct sockaddr_in6));

    struct sockaddr unsupported{};
    unsupported.sa_family = AF_UNSPEC;
    parsed = ipv4_service;
    BOOST_CHECK(!parsed.SetSockAddr(&unsupported, sizeof(unsupported)));
    BOOST_CHECK(parsed == ipv4_service);
}

BOOST_AUTO_TEST_CASE(subnet_test)
{
    CheckSubNetCanonicalRoundTrip("1.2.3.4/255.255.255.0");
    CheckSubNetCanonicalRoundTrip("1:2:3:4:5:6:7:8/ffff:ffff:ffff:ffff::");
    CheckSubNetCanonicalRoundTrip("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion");
    CheckSubNetCanonicalRoundTrip(std::string(52, 'a') + ".b32.i2p");
    CheckSubNetCanonicalRoundTrip("fc00::1");

    BOOST_CHECK(LookupSubNet("1.2.3.0/24") == LookupSubNet("1.2.3.0/255.255.255.0"));
    BOOST_CHECK(LookupSubNet("1.2.3.0/24") != LookupSubNet("1.2.4.0/255.255.255.0"));
    BOOST_CHECK(LookupSubNet("1.2.3.0/24").Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(!LookupSubNet("1.2.2.0/24").Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(LookupSubNet("1.2.3.4").Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(LookupSubNet("1.2.3.4/32").Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(!LookupSubNet("1.2.3.4").Match(ResolveIP("5.6.7.8")));
    BOOST_CHECK(!LookupSubNet("1.2.3.4/32").Match(ResolveIP("5.6.7.8")));
    BOOST_CHECK(LookupSubNet("::ffff:127.0.0.1").Match(ResolveIP("127.0.0.1")));
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:8").Match(ResolveIP("1:2:3:4:5:6:7:8")));
    BOOST_CHECK(!LookupSubNet("1:2:3:4:5:6:7:8").Match(ResolveIP("1:2:3:4:5:6:7:9")));
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:0/112").Match(ResolveIP("1:2:3:4:5:6:7:1234")));
    BOOST_CHECK(LookupSubNet("192.168.0.1/24").Match(ResolveIP("192.168.0.2")));
    BOOST_CHECK(LookupSubNet("192.168.0.20/29").Match(ResolveIP("192.168.0.18")));
    BOOST_CHECK(LookupSubNet("1.2.2.1/24").Match(ResolveIP("1.2.2.4")));
    BOOST_CHECK(LookupSubNet("1.2.2.110/31").Match(ResolveIP("1.2.2.111")));
    BOOST_CHECK(LookupSubNet("1.2.2.20/26").Match(ResolveIP("1.2.2.63")));
    // All-Matching IPv6 Matches arbitrary IPv6
    BOOST_CHECK(LookupSubNet("::/0").Match(ResolveIP("1:2:3:4:5:6:7:1234")));
    // But not `::` or `0.0.0.0` because they are considered invalid addresses
    BOOST_CHECK(!LookupSubNet("::/0").Match(ResolveIP("::")));
    BOOST_CHECK(!LookupSubNet("::/0").Match(ResolveIP("0.0.0.0")));
    // Addresses from one network (IPv4) don't belong to subnets of another network (IPv6)
    BOOST_CHECK(!LookupSubNet("::/0").Match(ResolveIP("1.2.3.4")));
    // All-Matching IPv4 does not Match IPv6
    BOOST_CHECK(!LookupSubNet("0.0.0.0/0").Match(ResolveIP("1:2:3:4:5:6:7:1234")));
    // Invalid subnets Match nothing (not even invalid addresses)
    BOOST_CHECK(!CSubNet().Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(!LookupSubNet("").Match(ResolveIP("4.5.6.7")));
    BOOST_CHECK(!LookupSubNet("bloop").Match(ResolveIP("0.0.0.0")));
    BOOST_CHECK(!LookupSubNet("bloop").Match(ResolveIP("hab")));
    // Check valid/invalid
    BOOST_CHECK(LookupSubNet("1.2.3.0/0").IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/-1").IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/+24").IsValid());
    BOOST_CHECK(LookupSubNet("1.2.3.0/32").IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/33").IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/300").IsValid());
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:8/0").IsValid());
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:8/33").IsValid());
    BOOST_CHECK(!LookupSubNet("1:2:3:4:5:6:7:8/-1").IsValid());
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:8/128").IsValid());
    BOOST_CHECK(!LookupSubNet("1:2:3:4:5:6:7:8/129").IsValid());
    BOOST_CHECK(!LookupSubNet("fuzzy").IsValid());

    //CNetAddr constructor test
    BOOST_CHECK(CSubNet(ResolveIP("127.0.0.1")).IsValid());
    BOOST_CHECK(CSubNet(ResolveIP("127.0.0.1")).Match(ResolveIP("127.0.0.1")));
    BOOST_CHECK(!CSubNet(ResolveIP("127.0.0.1")).Match(ResolveIP("127.0.0.2")));
    BOOST_CHECK(CSubNet(ResolveIP("127.0.0.1")).ToString() == "127.0.0.1/32");

    CSubNet subnet = CSubNet(ResolveIP("1.2.3.4"), 32);
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/32");
    subnet = CSubNet(ResolveIP("1.2.3.4"), 8);
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/8");
    subnet = CSubNet(ResolveIP("1.2.3.4"), 0);
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/0");

    subnet = CSubNet(ResolveIP("1.2.3.4"), ResolveIP("255.255.255.255"));
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/32");
    subnet = CSubNet(ResolveIP("1.2.3.4"), ResolveIP("255.0.0.0"));
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/8");
    subnet = CSubNet(ResolveIP("1.2.3.4"), ResolveIP("0.0.0.0"));
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/0");

    BOOST_CHECK(CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).IsValid());
    BOOST_CHECK(CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).Match(ResolveIP("1:2:3:4:5:6:7:8")));
    BOOST_CHECK(!CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).Match(ResolveIP("1:2:3:4:5:6:7:9")));
    BOOST_CHECK(CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).ToString() == "1:2:3:4:5:6:7:8/128");
    // IPv4 address with IPv6 netmask or the other way around.
    BOOST_CHECK(!CSubNet(ResolveIP("1.1.1.1"), ResolveIP("ffff::")).IsValid());
    BOOST_CHECK(!CSubNet(ResolveIP("::1"), ResolveIP("255.0.0.0")).IsValid());

    // Create Non-IP subnets.

    const CNetAddr tor_addr{
        ResolveIP("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion")};

    subnet = CSubNet(tor_addr);
    BOOST_CHECK(subnet.IsValid());
    BOOST_CHECK_EQUAL(subnet.ToString(), tor_addr.ToStringAddr());
    BOOST_CHECK(subnet.Match(tor_addr));
    BOOST_CHECK(
        !subnet.Match(ResolveIP("kpgvmscirrdqpekbqjsvw5teanhatztpp2gl6eee4zkowvwfxwenqaid.onion")));
    BOOST_CHECK(!subnet.Match(ResolveIP("1.2.3.4")));

    BOOST_CHECK(!CSubNet(tor_addr, 200).IsValid());
    BOOST_CHECK(!CSubNet(tor_addr, ResolveIP("255.0.0.0")).IsValid());

    subnet = LookupSubNet("1.2.3.4/255.255.255.255");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/32");
    subnet = LookupSubNet("1.2.3.4/255.255.255.254");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/31");
    subnet = LookupSubNet("1.2.3.4/255.255.255.252");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/30");
    subnet = LookupSubNet("1.2.3.4/255.255.255.248");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/29");
    subnet = LookupSubNet("1.2.3.4/255.255.255.240");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/28");
    subnet = LookupSubNet("1.2.3.4/255.255.255.224");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/27");
    subnet = LookupSubNet("1.2.3.4/255.255.255.192");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/26");
    subnet = LookupSubNet("1.2.3.4/255.255.255.128");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/25");
    subnet = LookupSubNet("1.2.3.4/255.255.255.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/24");
    subnet = LookupSubNet("1.2.3.4/255.255.254.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.2.0/23");
    subnet = LookupSubNet("1.2.3.4/255.255.252.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/22");
    subnet = LookupSubNet("1.2.3.4/255.255.248.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/21");
    subnet = LookupSubNet("1.2.3.4/255.255.240.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/20");
    subnet = LookupSubNet("1.2.3.4/255.255.224.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/19");
    subnet = LookupSubNet("1.2.3.4/255.255.192.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/18");
    subnet = LookupSubNet("1.2.3.4/255.255.128.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/17");
    subnet = LookupSubNet("1.2.3.4/255.255.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/16");
    subnet = LookupSubNet("1.2.3.4/255.254.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/15");
    subnet = LookupSubNet("1.2.3.4/255.252.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/14");
    subnet = LookupSubNet("1.2.3.4/255.248.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/13");
    subnet = LookupSubNet("1.2.3.4/255.240.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/12");
    subnet = LookupSubNet("1.2.3.4/255.224.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/11");
    subnet = LookupSubNet("1.2.3.4/255.192.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/10");
    subnet = LookupSubNet("1.2.3.4/255.128.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/9");
    subnet = LookupSubNet("1.2.3.4/255.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/8");
    subnet = LookupSubNet("1.2.3.4/254.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/7");
    subnet = LookupSubNet("1.2.3.4/252.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/6");
    subnet = LookupSubNet("1.2.3.4/248.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/5");
    subnet = LookupSubNet("1.2.3.4/240.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/4");
    subnet = LookupSubNet("1.2.3.4/224.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/3");
    subnet = LookupSubNet("1.2.3.4/192.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/2");
    subnet = LookupSubNet("1.2.3.4/128.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/1");
    subnet = LookupSubNet("1.2.3.4/0.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/0");

    subnet = LookupSubNet("1:2:3:4:5:6:7:8/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1:2:3:4:5:6:7:8/128");
    subnet = LookupSubNet("1:2:3:4:5:6:7:8/ffff:0000:0000:0000:0000:0000:0000:0000");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1::/16");
    subnet = LookupSubNet("1:2:3:4:5:6:7:8/0000:0000:0000:0000:0000:0000:0000:0000");
    BOOST_CHECK_EQUAL(subnet.ToString(), "::/0");
    // Invalid netmasks (with 1-bits after 0-bits)
    subnet = LookupSubNet("1.2.3.4/255.255.232.0");
    BOOST_CHECK(!subnet.IsValid());
    subnet = LookupSubNet("1.2.3.4/255.0.255.255");
    BOOST_CHECK(!subnet.IsValid());
    subnet = LookupSubNet("1:2:3:4:5:6:7:8/ffff:ffff:ffff:fffe:ffff:ffff:ffff:ff0f");
    BOOST_CHECK(!subnet.IsValid());
}

BOOST_AUTO_TEST_CASE(netbase_getgroup)
{
    auto netgroupman{NetGroupManager::NoAsmap()}; // use /16
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("127.0.0.1")) == std::vector<unsigned char>({0})); // Local -> !Routable()
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("257.0.0.1")) == std::vector<unsigned char>({0})); // !Valid -> !Routable()
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("10.0.0.1")) == std::vector<unsigned char>({0})); // RFC1918 -> !Routable()
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("169.254.1.1")) == std::vector<unsigned char>({0})); // RFC3927 -> !Routable()
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("1.2.3.4")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // IPv4
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("::FFFF:0:102:304")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // RFC6145
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("64:FF9B::102:304")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // RFC6052
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("2002:102:304:9999:9999:9999:9999:9999")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // RFC3964
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("2001:0:9999:9999:9999:9999:FEFD:FCFB")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // RFC4380
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("2001:470:abcd:9999:9999:9999:9999:9999")) == std::vector<unsigned char>({(unsigned char)NET_IPV6, 32, 1, 4, 112, 175})); //he.net
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("2001:2001:9999:9999:9999:9999:9999:9999")) == std::vector<unsigned char>({(unsigned char)NET_IPV6, 32, 1, 32, 1})); //IPv6

    // baz.net sha256 hash: 12929400eb4607c4ac075f087167e75286b179c693eb059a01774b864e8fe505
    std::vector<unsigned char> internal_group = {NET_INTERNAL, 0x12, 0x92, 0x94, 0x00, 0xeb, 0x46, 0x07, 0xc4, 0xac, 0x07};
    BOOST_CHECK(netgroupman.GetGroup(CreateInternal("baz.net")) == internal_group);

    BOOST_CHECK(!ResolveIP("127.0.0.1").HasLinkedIPv4());
    BOOST_CHECK(!ResolveIP("10.0.0.1").HasLinkedIPv4());
    BOOST_CHECK(!ResolveIP("169.254.1.1").HasLinkedIPv4());

    BOOST_CHECK(ResolveIP("1.2.3.4").HasLinkedIPv4());
    BOOST_CHECK_EQUAL(ResolveIP("1.2.3.4").GetLinkedIPv4(), 0x01020304U);
    BOOST_CHECK_EQUAL(ResolveIP("::FFFF:0:102:304").GetLinkedIPv4(), 0x01020304U);
    BOOST_CHECK_EQUAL(ResolveIP("64:FF9B::102:304").GetLinkedIPv4(), 0x01020304U);
    BOOST_CHECK_EQUAL(ResolveIP("2002:102:304:9999:9999:9999:9999:9999").GetLinkedIPv4(), 0x01020304U);
    BOOST_CHECK_EQUAL(ResolveIP("2001:0:9999:9999:9999:9999:FEFD:FCFB").GetLinkedIPv4(), 0x01020304U);
}

BOOST_AUTO_TEST_CASE(netbase_parsenetwork)
{
    const std::pair<Network, std::string> public_networks[]{
        {NET_IPV4, "ipv4"},
        {NET_IPV6, "ipv6"},
        {NET_ONION, "onion"},
        {NET_I2P, "i2p"},
        {NET_CJDNS, "cjdns"},
    };
    for (const auto& [network, name] : public_networks) {
        BOOST_CHECK_EQUAL(ParseNetwork(name), network);
        BOOST_CHECK_EQUAL(ParseNetwork(ToUpper(name)), network);
        BOOST_CHECK_EQUAL(GetNetworkName(network), name);
        BOOST_CHECK_EQUAL(ParseNetwork(GetNetworkName(network)), network);
    }

    BOOST_CHECK_EQUAL(ParseNetwork("IPv4"), NET_IPV4);
    BOOST_CHECK_EQUAL(ParseNetwork("IPv6"), NET_IPV6);
    BOOST_CHECK_EQUAL(ParseNetwork("ONION"), NET_ONION);
    BOOST_CHECK_EQUAL(ParseNetwork("I2P"), NET_I2P);
    BOOST_CHECK_EQUAL(ParseNetwork("CJDNS"), NET_CJDNS);

    // "tor" as a network specification was deprecated in 60dc8e4208 in favor of
    // "onion" and later removed.
    BOOST_CHECK_EQUAL(ParseNetwork("tor"), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork("TOR"), NET_UNROUTABLE);

    BOOST_CHECK_EQUAL(ParseNetwork(GetNetworkName(NET_UNROUTABLE)), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork(GetNetworkName(NET_INTERNAL)), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork(std::string{"ipv4\0", 5}), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork(":)"), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork("oniÖn"), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork("\xfe\xff"), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork(""), NET_UNROUTABLE);
}

BOOST_AUTO_TEST_CASE(netpermissions_test)
{
    bilingual_str error;
    NetWhitebindPermissions whitebindPermissions;
    NetWhitelistPermissions whitelistPermissions;
    ConnectionDirection connection_direction;

    // Detect invalid white bind
    BOOST_CHECK(!NetWhitebindPermissions::TryParse("", whitebindPermissions, error));
    BOOST_CHECK(error.original.find("Cannot resolve -whitebind address") != std::string::npos);
    BOOST_CHECK(!NetWhitebindPermissions::TryParse("127.0.0.1", whitebindPermissions, error));
    BOOST_CHECK(error.original.find("Need to specify a port with -whitebind") != std::string::npos);
    BOOST_CHECK(!NetWhitebindPermissions::TryParse("", whitebindPermissions, error));

    // If no permission flags, assume backward compatibility
    BOOST_CHECK(NetWhitebindPermissions::TryParse("1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK(error.empty());
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::Implicit);
    BOOST_CHECK(NetPermissions::HasFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit));
    NetPermissions::ClearFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit);
    BOOST_CHECK(!NetPermissions::HasFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::None);
    NetPermissions::AddFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit);
    BOOST_CHECK(NetPermissions::HasFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit));

    auto explicit_and_implicit{NetPermissionFlags::Implicit | NetPermissionFlags::BloomFilter | NetPermissionFlags::ForceRelay | NetPermissionFlags::NoBan};
    NetPermissions::ClearFlag(explicit_and_implicit, NetPermissionFlags::Implicit);
    BOOST_CHECK_EQUAL(explicit_and_implicit, NetPermissionFlags::BloomFilter | NetPermissionFlags::ForceRelay | NetPermissionFlags::NoBan);
    BOOST_CHECK(NetPermissions::HasFlag(explicit_and_implicit, NetPermissionFlags::Relay));
    BOOST_CHECK(NetPermissions::HasFlag(explicit_and_implicit, NetPermissionFlags::Download));
    BOOST_CHECK(!NetPermissions::HasFlag(explicit_and_implicit, NetPermissionFlags::Implicit));

    // Can set one permission
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter);
    BOOST_CHECK(NetWhitebindPermissions::TryParse("@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::None);

    NetWhitebindPermissions noban, noban_download, download_noban, download;

    // "noban" implies "download"
    BOOST_REQUIRE(NetWhitebindPermissions::TryParse("noban@1.2.3.4:32", noban, error));
    BOOST_CHECK_EQUAL(noban.m_flags, NetPermissionFlags::NoBan);
    BOOST_CHECK(NetPermissions::HasFlag(noban.m_flags, NetPermissionFlags::Download));
    BOOST_CHECK(NetPermissions::HasFlag(noban.m_flags, NetPermissionFlags::NoBan));

    // "noban,download" is equivalent to "noban"
    BOOST_REQUIRE(NetWhitebindPermissions::TryParse("noban,download@1.2.3.4:32", noban_download, error));
    BOOST_CHECK_EQUAL(noban_download.m_flags, noban.m_flags);

    // "download,noban" is equivalent to "noban"
    BOOST_REQUIRE(NetWhitebindPermissions::TryParse("download,noban@1.2.3.4:32", download_noban, error));
    BOOST_CHECK_EQUAL(download_noban.m_flags, noban.m_flags);

    // "download" excludes (does not imply) "noban"
    BOOST_REQUIRE(NetWhitebindPermissions::TryParse("download@1.2.3.4:32", download, error));
    BOOST_CHECK_EQUAL(download.m_flags, NetPermissionFlags::Download);
    BOOST_CHECK(NetPermissions::HasFlag(download.m_flags, NetPermissionFlags::Download));
    BOOST_CHECK(!NetPermissions::HasFlag(download.m_flags, NetPermissionFlags::NoBan));

    // Happy path, can parse flags
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,forcerelay@1.2.3.4:32", whitebindPermissions, error));
    // forcerelay should also activate the relay permission
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::ForceRelay | NetPermissionFlags::Relay);
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,relay,noban@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::Relay | NetPermissionFlags::NoBan);
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,forcerelay,noban@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK(NetWhitebindPermissions::TryParse("all@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::All);

    // Allow dups
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,relay,noban,noban@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::Relay | NetPermissionFlags::NoBan | NetPermissionFlags::Download); // "noban" implies "download"

    // Allow empty
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,relay,,noban@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::Relay | NetPermissionFlags::NoBan);
    BOOST_CHECK(NetWhitebindPermissions::TryParse(",@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::None);
    BOOST_CHECK(NetWhitebindPermissions::TryParse(",,@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::None);

    BOOST_CHECK(!NetWhitebindPermissions::TryParse("out,forcerelay@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK(error.original.find("whitebind may only be used for incoming connections (\"out\" was passed)") != std::string::npos);

    // Detect invalid flag
    BOOST_CHECK(!NetWhitebindPermissions::TryParse("bloom,forcerelay,oopsie@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK(error.original.find("Invalid P2P permission") != std::string::npos);

    // Check netmask error
    whitelistPermissions.m_flags = NetPermissionFlags::Addr;
    whitelistPermissions.m_subnet = LookupSubNet("5.6.7.8/32");
    const auto original_whitelist_permissions{whitelistPermissions};
    connection_direction = ConnectionDirection::Both;
    BOOST_CHECK(!NetWhitelistPermissions::TryParse("bloom,forcerelay,noban@1.2.3.4:32", whitelistPermissions, connection_direction, error));
    BOOST_CHECK(error.original.find("Invalid netmask specified in -whitelist") != std::string::npos);
    BOOST_CHECK_EQUAL(whitelistPermissions.m_flags, original_whitelist_permissions.m_flags);
    BOOST_CHECK(whitelistPermissions.m_subnet == original_whitelist_permissions.m_subnet);
    BOOST_CHECK_EQUAL(connection_direction, ConnectionDirection::Both);

    // Happy path for whitelist parsing
    BOOST_CHECK(NetWhitelistPermissions::TryParse("noban@1.2.3.4", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(whitelistPermissions.m_flags, NetPermissionFlags::NoBan);
    BOOST_CHECK(NetPermissions::HasFlag(whitelistPermissions.m_flags, NetPermissionFlags::NoBan));

    BOOST_CHECK(NetWhitelistPermissions::TryParse("bloom,forcerelay,noban,relay@1.2.3.4/32", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(whitelistPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::ForceRelay | NetPermissionFlags::NoBan | NetPermissionFlags::Relay);
    BOOST_CHECK(error.empty());
    BOOST_CHECK_EQUAL(whitelistPermissions.m_subnet.ToString(), "1.2.3.4/32");
    BOOST_CHECK(NetWhitelistPermissions::TryParse("bloom,forcerelay,noban,relay,mempool@1.2.3.4/32", whitelistPermissions, connection_direction, error));
    BOOST_CHECK(NetWhitelistPermissions::TryParse("in,relay@1.2.3.4", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(connection_direction, ConnectionDirection::In);
    BOOST_CHECK(NetWhitelistPermissions::TryParse("out,bloom@1.2.3.4", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(connection_direction, ConnectionDirection::Out);
    BOOST_CHECK(NetWhitelistPermissions::TryParse("in,out,bloom@1.2.3.4", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(connection_direction, ConnectionDirection::Both);

    const auto check_permission_strings = [](NetPermissionFlags flags, const std::vector<std::string>& expected) {
        const auto strings{NetPermissions::ToStrings(flags)};
        BOOST_CHECK_EQUAL_COLLECTIONS(strings.begin(), strings.end(), expected.begin(), expected.end());

        NetWhitebindPermissions parsed;
        bilingual_str error;
        BOOST_REQUIRE(NetWhitebindPermissions::TryParse(util::Join(strings, ",") + "@1.2.3.4:32", parsed, error));
        BOOST_CHECK(error.empty());
        BOOST_CHECK_EQUAL(parsed.m_flags, flags);
    };
    check_permission_strings(NetPermissionFlags::None, {});
    check_permission_strings(NetPermissionFlags::BloomFilter, {"bloomfilter"});
    check_permission_strings(NetPermissionFlags::ForceRelay, {"forcerelay", "relay"});
    check_permission_strings(NetPermissionFlags::NoBan, {"noban", "download"});
    const std::vector<std::string> all_permissions{
        "bloomfilter", "noban", "forcerelay", "relay", "mempool", "download", "addr"};
    check_permission_strings(NetPermissionFlags::All, all_permissions);
}

BOOST_AUTO_TEST_CASE(netbase_dont_resolve_strings_with_embedded_nul_characters)
{
    BOOST_CHECK(LookupHost("127.0.0.1"s, false).has_value());
    BOOST_CHECK(!LookupHost("127.0.0.1\0"s, false).has_value());
    BOOST_CHECK(!LookupHost("127.0.0.1\0example.com"s, false).has_value());
    BOOST_CHECK(!LookupHost("127.0.0.1\0example.com\0"s, false).has_value());

    BOOST_CHECK(LookupSubNet("1.2.3.0/24"s).IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/24\0"s).IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/24\0example.com"s).IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/24\0example.com\0"s).IsValid());
    BOOST_CHECK(LookupSubNet("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"s).IsValid());
    BOOST_CHECK(!LookupSubNet("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion\0"s).IsValid());
    BOOST_CHECK(!LookupSubNet("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion\0example.com"s).IsValid());
    BOOST_CHECK(!LookupSubNet("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion\0example.com\0"s).IsValid());
}

BOOST_AUTO_TEST_CASE(service_flags_to_str)
{
    BOOST_CHECK(serviceFlagsToStr(NODE_NONE).empty());

    const uint64_t flags{uint64_t{NODE_NETWORK} | uint64_t{NODE_P2P_V2} | (uint64_t{1} << 24) | (uint64_t{1} << 63)};
    const std::vector<std::string> expected{"NETWORK", "P2P_V2", "UNKNOWN[2^24]", "UNKNOWN[2^63]"};
    const std::vector<std::string> strings{serviceFlagsToStr(flags)};

    BOOST_CHECK_EQUAL_COLLECTIONS(strings.begin(), strings.end(), expected.begin(), expected.end());
}

// Since CNetAddr (un)ser is tested separately in net_tests.cpp here we only
// try a few edge cases for port, service flags and time.

static const std::vector<CAddress> fixture_addresses({
    CAddress{
        CService(CNetAddr(in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT)), 0 /* port */),
        NODE_NONE,
        NodeSeconds{0x4966bc61s}, /* Fri Jan  9 02:54:25 UTC 2009 */
    },
    CAddress{
        CService(CNetAddr(in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT)), 0x00f1 /* port */),
        NODE_NETWORK,
        NodeSeconds{0x83766279s}, /* Tue Nov 22 11:22:33 UTC 2039 */
    },
    CAddress{
        CService(CNetAddr(in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT)), 0xf1f2 /* port */),
        static_cast<ServiceFlags>(NODE_WITNESS | NODE_COMPACT_FILTERS | NODE_NETWORK_LIMITED),
        NodeSeconds{0xffffffffs}, /* Sun Feb  7 06:28:15 UTC 2106 */
    },
});

// fixture_addresses should equal to this when serialized in V1 format.
// When this is unserialized from V1 format it should equal to fixture_addresses.
static constexpr const char* stream_addrv1_hex =
    "03" // number of entries

    "61bc6649"                         // time, Fri Jan  9 02:54:25 UTC 2009
    "0000000000000000"                 // service flags, NODE_NONE
    "00000000000000000000000000000001" // address, fixed 16 bytes (IPv4 embedded in IPv6)
    "0000"                             // port

    "79627683"                         // time, Tue Nov 22 11:22:33 UTC 2039
    "0100000000000000"                 // service flags, NODE_NETWORK
    "00000000000000000000000000000001" // address, fixed 16 bytes (IPv6)
    "00f1"                             // port

    "ffffffff"                         // time, Sun Feb  7 06:28:15 UTC 2106
    "4804000000000000"                 // service flags, NODE_WITNESS | NODE_COMPACT_FILTERS | NODE_NETWORK_LIMITED
    "00000000000000000000000000000001" // address, fixed 16 bytes (IPv6)
    "f1f2";                            // port

// fixture_addresses should equal to this when serialized in V2 format.
// When this is unserialized from V2 format it should equal to fixture_addresses.
static constexpr const char* stream_addrv2_hex =
    "03" // number of entries

    "61bc6649"                         // time, Fri Jan  9 02:54:25 UTC 2009
    "00"                               // service flags, COMPACTSIZE(NODE_NONE)
    "02"                               // network id, IPv6
    "10"                               // address length, COMPACTSIZE(16)
    "00000000000000000000000000000001" // address
    "0000"                             // port

    "79627683"                         // time, Tue Nov 22 11:22:33 UTC 2039
    "01"                               // service flags, COMPACTSIZE(NODE_NETWORK)
    "02"                               // network id, IPv6
    "10"                               // address length, COMPACTSIZE(16)
    "00000000000000000000000000000001" // address
    "00f1"                             // port

    "ffffffff"                         // time, Sun Feb  7 06:28:15 UTC 2106
    "fd4804"                           // service flags, COMPACTSIZE(NODE_WITNESS | NODE_COMPACT_FILTERS | NODE_NETWORK_LIMITED)
    "02"                               // network id, IPv6
    "10"                               // address length, COMPACTSIZE(16)
    "00000000000000000000000000000001" // address
    "f1f2";                            // port

BOOST_AUTO_TEST_CASE(caddress_serialize_v1)
{
    DataStream s{};

    s << CAddress::V1_NETWORK(fixture_addresses);
    BOOST_CHECK_EQUAL(HexStr(s), stream_addrv1_hex);
}

BOOST_AUTO_TEST_CASE(caddress_unserialize_v1)
{
    std::vector<CAddress> addresses_unserialized;

    SpanReader{ParseHex(stream_addrv1_hex)} >> CAddress::V1_NETWORK(addresses_unserialized);
    BOOST_CHECK(fixture_addresses == addresses_unserialized);
}

BOOST_AUTO_TEST_CASE(caddress_serialize_v2)
{
    DataStream s{};

    s << CAddress::V2_NETWORK(fixture_addresses);
    BOOST_CHECK_EQUAL(HexStr(s), stream_addrv2_hex);
}

BOOST_AUTO_TEST_CASE(caddress_unserialize_v2)
{
    std::vector<CAddress> addresses_unserialized;

    SpanReader{ParseHex(stream_addrv2_hex)} >> CAddress::V2_NETWORK(addresses_unserialized);
    BOOST_CHECK(fixture_addresses == addresses_unserialized);
}

BOOST_AUTO_TEST_CASE(isbadport)
{
    BOOST_CHECK(IsBadPort(1));
    BOOST_CHECK(IsBadPort(22));
    BOOST_CHECK(IsBadPort(6000));

    BOOST_CHECK(!IsBadPort(80));
    BOOST_CHECK(!IsBadPort(443));
    BOOST_CHECK(!IsBadPort(8333));

    // Check all possible ports and ensure we only flag the expected amount as bad
    std::list<int> ports(std::numeric_limits<uint16_t>::max());
    std::iota(ports.begin(), ports.end(), 1);
    BOOST_CHECK_EQUAL(std::ranges::count_if(ports, IsBadPort), 85);
}


BOOST_AUTO_TEST_CASE(asmap_sanity_check_input_length_monotonicity)
{
    // RETURN 1 with zero padding is valid without consuming any address bits,
    // so accepting it for 0 bits must imply accepting it for every longer input.
    const auto return_one{"000000"_hex};
    for (int bits = 0; bits <= 128; ++bits) {
        BOOST_CHECK(SanityCheckAsmap(return_one, bits));
    }
    BOOST_CHECK(CheckStandardAsmap(return_one));

    const std::vector<std::byte> no_addr;
    const auto one_addr{"ff"_hex};
    BOOST_CHECK_EQUAL(Interpret(return_one, no_addr), 1);
    BOOST_CHECK_EQUAL(Interpret(return_one, one_addr), 1);
}

BOOST_AUTO_TEST_CASE(asmap_test_vectors)
{
    // Randomly generated encoded ASMap with 128 ranges, up to 20-bit AS numbers.
    constexpr auto ASMAP_DATA{
        "fd38d50f7d5d665357f64bba6bfc190d6078a7e68e5d3ac032edf47f8b5755f87881bfd3633d9aa7c1fa279b3"
        "6fe26c63bbc9de44e0f04e5a382d8e1cddbe1c26653bc939d4327f287e8b4d1f8aff33176787cb0ff7cb28e3f"
        "daef0f8f47357f801c9f7ff7a99f7f9c9f99de7f3156ae00f23eb27a303bc486aa3ccc31ec19394c2f8a53ddd"
        "ea3cc56257f3b7e9b1f488be9c1137db823759aa4e071eef2e984aaf97b52d5f88d0f373dd190fe45e06efef1"
        "df7278be680a73a74c76db4dd910f1d30752c57fe2bc9f079f1a1e1b036c2a69219f11c5e11980a3fa51f4f82"
        "d36373de73b1863a8c27e36ae0e4f705be3d76ecff038a75bc0f92ba7e7f6f4080f1c47c34d095367ecf4406c"
        "1e3bbc17ba4d6f79ea3f031b876799ac268b1e0ea9babf0f9a8e5f6c55e363c6363df46afc696d7afceaf49b6"
        "e62df9e9dc27e70664cafe5c53df66dd0b8237678ada90e73f05ec60e6f6e96c3cbb1ea2f9dece115d5bdba10"
        "33e53662a7d72a29477b5beb35710591d3e23e5f0379baea62ffdee535bcdf879cbf69b88d7ea37c8015381cf"
        "63dc33d28f757a4a5e15d6a08"_hex};

    // Construct NetGroupManager with this data.
    auto netgroup{NetGroupManager::WithEmbeddedAsmap(ASMAP_DATA)};
    BOOST_CHECK(netgroup.UsingASMap());

    // This ASMap first consumes the IPv4-in-IPv6 prefix, then returns ASN 1.
    constexpr auto IPV4_MATCH_ASMAP{
        "fb03ec0fb03fc0fe00fb03ec0fb03fc0fe00fb03ec0fb0fffffeff000000"_hex};
    BOOST_REQUIRE(CheckStandardAsmap(IPV4_MATCH_ASMAP));
    auto ipv4_netgroup{NetGroupManager::WithEmbeddedAsmap(IPV4_MATCH_ASMAP)};
    const auto ipv4_addr{ResolveIP("1.2.3.4")};
    BOOST_CHECK_EQUAL(Interpret(IPV4_MATCH_ASMAP, AsmapLookupBytes(ipv4_addr)), 1);
    BOOST_CHECK_EQUAL(ipv4_netgroup.GetMappedAS(ipv4_addr), 1);
    BOOST_CHECK_EQUAL(ipv4_netgroup.GetMappedAS(ResolveIP("198.51.100.49")), 0);

    // Check some randomly-generated IPv6 addresses in it (biased towards the very beginning and
    // very end of the 128-bit range).
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("0:1559:183:3728:224c:65a5:62e6:e991", false)), 961340);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("d0:d493:faa0:8609:e927:8b75:293c:f5a4", false)), 961340);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("2a0:26f:8b2c:2ee7:c7d1:3b24:4705:3f7f", false)), 693761);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("a77:7cd4:4be5:a449:89f2:3212:78c6:ee38", false)), 0);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("1336:1ad6:2f26:4fe3:d809:7321:6e0d:4615", false)), 672176);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("1d56:abd0:a52f:a8d5:d5a7:a610:581d:d792", false)), 499880);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("378e:7290:54e5:bd36:4760:971c:e9b9:570d", false)), 0);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("406c:820b:272a:c045:b74e:fc0a:9ef2:cecc", false)), 248495);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("46c2:ae07:9d08:2d56:d473:2bc7:57e3:20ac", false)), 248495);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("50d2:3db6:52fa:2e7:12ec:5bc4:1bd1:49f9", false)), 124471);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("53e1:1812:ffa:dccf:f9f2:64be:75fa:795", false)), 539993);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("544d:eeba:3990:35d1:ad66:f9a3:576d:8617", false)), 374443);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("6a53:40dc:8f1d:3ffa:efeb:3aa3:df88:b94b", false)), 435070);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("87aa:d1c9:9edb:91e7:aab1:9eb9:baa0:de18", false)), 244121);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("9f00:48fa:88e3:4b67:a6f3:e6d2:5cc1:5be2", false)), 862116);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("c49f:9cc6:86ad:ba08:4580:315e:dbd1:8a62", false)), 969411);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("dff5:8021:61d:b17d:406d:7888:fdac:4a20", false)), 969411);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("e888:6791:2960:d723:bcfd:47e1:2d8c:599f", false)), 824019);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("ffff:d499:8c4b:4941:bc81:d5b9:b51e:85a8", false)), 824019);
}

BOOST_AUTO_TEST_SUITE_END()
