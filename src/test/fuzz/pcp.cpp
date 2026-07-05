// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/time.h>

#include <common/pcp.h>
#include <compat/compat.h>
#include <logging.h>
#include <netaddress.h>
#include <util/check.h>
#include <util/threadinterrupt.h>
#include <util/time.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

using namespace std::literals;

//! Fixed nonce to use in PCP port mapping requests.
constexpr PCPMappingNonce PCP_NONCE{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};

//! Number of attempts to request a NAT-PMP or PCP port mapping to the gateway.
constexpr int NUM_TRIES{5};

//! Timeout for each attempt to request a port mapping.
constexpr std::chrono::duration TIMEOUT{100ms};

void port_map_target_init()
{
    LogInstance().DisableLogging();
}

namespace {

CNetAddr IPv4Addr(uint32_t addr)
{
    in_addr ipv4;
    ipv4.s_addr = htonl(addr);
    return CNetAddr{ipv4};
}

class PCPResponseSock final : public ZeroSock
{
public:
    PCPResponseSock(CNetAddr local_addr, std::array<uint8_t, 8> response, size_t* send_count = nullptr)
        : m_local_addr{std::move(local_addr)}, m_response{response}, m_send_count{send_count}
    {
    }

    ssize_t Send(const void* data, size_t len, int flags) const override
    {
        if (m_send_count) ++*m_send_count;
        return ZeroSock::Send(data, len, flags);
    }

    ssize_t Recv(void* buf, size_t len, int) const override
    {
        if (m_consumed) return -1;

        const size_t copy_len{std::min(len, m_response.size())};
        std::memcpy(buf, m_response.data(), copy_len);
        m_consumed = true;
        return copy_len;
    }

    int GetSockName(sockaddr* name, socklen_t* name_len) const override
    {
        return CService{m_local_addr, 1}.GetSockAddr(name, name_len) ? 0 : -1;
    }

    bool Wait(std::chrono::milliseconds, Event requested, Event* occurred = nullptr) const override
    {
        if (occurred != nullptr) {
            *occurred = m_consumed ? 0 : requested;
        }
        return true;
    }

private:
    const CNetAddr m_local_addr;
    const std::array<uint8_t, 8> m_response;
    size_t* const m_send_count;
    mutable bool m_consumed{false};
};

void AssertMalformedPCPDowngradeRejected(const std::array<uint8_t, 8>& response)
{
    MockableSteadyClock::SetMockTime(MockableSteadyClock::INITIAL_MOCK_TIME);

    const CNetAddr gateway_addr{IPv4Addr(0xc0a80001)}; // 192.168.0.1
    const CNetAddr local_addr{IPv4Addr(0xc0a80006)}; // 192.168.0.6
    in_addr bind_any;
    bind_any.s_addr = htonl(INADDR_ANY);

    CreateSock = [local_addr, response](int domain, int type, int protocol) -> std::unique_ptr<Sock> {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) {
            return std::make_unique<PCPResponseSock>(local_addr, response);
        }
        return std::unique_ptr<Sock>();
    };

    CThreadInterrupt interrupt;
    const auto res{PCPRequestPortMap(PCP_NONCE, gateway_addr, CNetAddr{bind_any}, 1234, 1000, interrupt, 1, TIMEOUT)};
    const MappingError* err{std::get_if<MappingError>(&res)};
    Assert(err);
    Assert(*err == MappingError::NETWORK_ERROR);
}

void AssertInterruptedPortMapDoesNotSend(bool use_pcp)
{
    MockableSteadyClock::SetMockTime(MockableSteadyClock::INITIAL_MOCK_TIME);

    const CNetAddr gateway_addr{IPv4Addr(0xc0a80001)}; // 192.168.0.1
    const CNetAddr local_addr{IPv4Addr(0xc0a80006)}; // 192.168.0.6
    in_addr bind_any;
    bind_any.s_addr = htonl(INADDR_ANY);

    size_t send_count{0};
    bool created_sock{false};
    CreateSock = [local_addr, &send_count, &created_sock](int domain, int type, int protocol) -> std::unique_ptr<Sock> {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) {
            created_sock = true;
            return std::make_unique<PCPResponseSock>(local_addr, std::array<uint8_t, 8>{}, &send_count);
        }
        return nullptr;
    };

    CThreadInterrupt interrupt;
    interrupt();
    const auto res{use_pcp ?
        PCPRequestPortMap(PCP_NONCE, gateway_addr, CNetAddr{bind_any}, 1234, 1000, interrupt, 1, TIMEOUT) :
        NATPMPRequestPortMap(gateway_addr, 1234, 1000, interrupt, 1, TIMEOUT)};

    const MappingError* err{std::get_if<MappingError>(&res)};
    Assert(err);
    Assert(*err == MappingError::NETWORK_ERROR);
    Assert(created_sock);
    Assert(send_count == 0);
}

} // namespace

FUZZ_TARGET(pcp_request_port_map, .init = port_map_target_init)
{
    ResetFuzzedSockMockedFds();
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeSteadyClock steady_clock;

    // Create a mocked socket between random (and potentially invalid) client and gateway addresses.
    auto CreateSockOrig = CreateSock;
    CreateSock = [&](int domain, int type, int protocol) {
        if ((domain == AF_INET || domain == AF_INET6) && type == SOCK_DGRAM && protocol == IPPROTO_UDP) {
            return std::make_unique<FuzzedSock>(fuzzed_data_provider, steady_clock);
        }
        return std::unique_ptr<FuzzedSock>();
    };

    // Perform the port mapping request. The mocked socket will return fuzzer-provided data.
    const auto gateway_addr{ConsumeNetAddr(fuzzed_data_provider)};
    const auto local_addr{ConsumeNetAddr(fuzzed_data_provider)};
    const auto port{fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    const auto lifetime{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
    CThreadInterrupt interrupt;
    const auto res{PCPRequestPortMap(PCP_NONCE, gateway_addr, local_addr, port, lifetime, interrupt, NUM_TRIES, TIMEOUT)};

    // In case of success the mapping must be consistent with the request.
    if (const MappingResult* mapping = std::get_if<MappingResult>(&res)) {
        Assert(mapping);
        Assert(mapping->internal.GetPort() == port);
        mapping->ToString();
    }

    AssertMalformedPCPDowngradeRejected({
        0x00, 0x81, 0xff, 0x01, // valid opcode, but result is 0xff01, not UNSUPP_VERSION
        0x00, 0x00, 0x00, 0x00,
    });
    AssertMalformedPCPDowngradeRejected({
        0x00, 0x80, 0x00, 0x01, // valid result, but wrong opcode for a PCP MAP response
        0x00, 0x00, 0x00, 0x00,
    });
    AssertInterruptedPortMapDoesNotSend(/*use_pcp=*/true);
    CreateSock = CreateSockOrig;
}

FUZZ_TARGET(natpmp_request_port_map, .init = port_map_target_init)
{
    ResetFuzzedSockMockedFds();
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeSteadyClock steady_clock;

    // Create a mocked socket between random (and potentially invalid) client and gateway addresses.
    auto CreateSockOrig = CreateSock;
    CreateSock = [&](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) {
            return std::make_unique<FuzzedSock>(fuzzed_data_provider, steady_clock);
        }
        return std::unique_ptr<FuzzedSock>();
    };

    // Perform the port mapping request. The mocked socket will return fuzzer-provided data.
    const auto gateway_addr{ConsumeNetAddr(fuzzed_data_provider)};
    const auto port{fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    const auto lifetime{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
    CThreadInterrupt interrupt;
    const auto res{NATPMPRequestPortMap(gateway_addr, port, lifetime, interrupt, NUM_TRIES, TIMEOUT)};

    // In case of success the mapping must be consistent with the request.
    if (const MappingResult* mapping = std::get_if<MappingResult>(&res)) {
        Assert(mapping);
        Assert(mapping->internal.GetPort() == port);
        mapping->ToString();
    }
    AssertInterruptedPortMapDoesNotSend(/*use_pcp=*/false);
    CreateSock = CreateSockOrig;
}
