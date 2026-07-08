// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/time.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

extern std::chrono::milliseconds g_socks5_recv_timeout;

namespace {
decltype(g_socks5_recv_timeout) default_socks5_recv_timeout;

void PushBytes(DynSock::Pipe& pipe, const std::vector<uint8_t>& bytes)
{
    if (!bytes.empty()) pipe.PushBytes(bytes.data(), bytes.size());
}

std::vector<uint8_t> ReadBytes(DynSock::Pipe& pipe, size_t len)
{
    std::vector<uint8_t> ret(len);
    Assert(pipe.GetBytes(ret.data(), ret.size()) == static_cast<ssize_t>(ret.size()));
    return ret;
}

void AssertNoBytes(DynSock::Pipe& pipe)
{
    uint8_t byte;
    Assert(pipe.GetBytes(&byte, 1) == -1);
}

std::vector<uint8_t> MakeSocks5MethodSelectionReply(bool select_auth)
{
    return {
        0x05, static_cast<uint8_t>(select_auth ? 0x02 : 0x00),
    };
}

std::vector<uint8_t> MakeSocks5AuthSuccessReply()
{
    return {0x01, 0x00};
}

std::vector<uint8_t> MakeSocks5AuthFailureReply(FuzzedDataProvider& provider)
{
    if (provider.ConsumeBool()) {
        return {0x00, 0x00};
    }
    return {0x01, static_cast<uint8_t>(provider.ConsumeIntegralInRange<uint8_t>(1, 0xff))};
}

std::vector<uint8_t> MakeSocks5ConnectSuccessReply()
{
    return {
        0x05, 0x00, 0x00, 0x03, // VER, REP=success, RSV, ATYP=domain
        0x00,                   // zero-length bind address
        0x00, 0x00,             // bind port
    };
}

std::vector<uint8_t> MakeSocks5SuccessReply(bool select_auth)
{
    std::vector<uint8_t> reply{MakeSocks5MethodSelectionReply(select_auth)};
    if (select_auth) {
        const auto auth_success{MakeSocks5AuthSuccessReply()};
        reply.insert(reply.end(), auth_success.begin(), auth_success.end());
    }
    const auto connect_success{MakeSocks5ConnectSuccessReply()};
    reply.insert(reply.end(), connect_success.begin(), connect_success.end());
    return reply;
}

std::vector<uint8_t> MakeSocks5ConnectFailureReply(FuzzedDataProvider& provider)
{
    const uint8_t failure_reason{provider.ConsumeIntegralInRange<uint8_t>(1, 0xff)};
    switch (provider.ConsumeIntegralInRange<uint8_t>(0, 3)) {
    case 0:
        return {0x04, 0x00, 0x00, 0x03}; // bad version
    case 1:
        return {0x05, failure_reason, 0x00, 0x03}; // non-success reply
    case 2:
        return {0x05, 0x00, 0xff, 0x03}; // bad reserved byte
    default:
        return {0x05, 0x00, 0x00, 0xff}; // unsupported address type
    }
}

std::vector<uint8_t> MakeSocks5MethodSelectionFailureReply(const ProxyCredentials* auth, FuzzedDataProvider& provider)
{
    if (provider.ConsumeBool()) {
        return {0x04, 0x00};
    }
    return {0x05, static_cast<uint8_t>(auth ? 0xff : 0x02)};
}

std::vector<uint8_t> ExpectedSocks5MethodSelectionBytes(const ProxyCredentials* auth)
{
    std::vector<uint8_t> expected{
        0x05,
        static_cast<uint8_t>(auth ? 0x02 : 0x01),
        0x00,
    };
    if (auth) expected.push_back(0x02);
    return expected;
}

std::vector<uint8_t> ExpectedSocks5ClientBytes(const std::string& dest, uint16_t port, const ProxyCredentials* auth)
{
    std::vector<uint8_t> expected{ExpectedSocks5MethodSelectionBytes(auth)};
    if (auth) {
        expected.insert(expected.end(), {
            0x01,
            static_cast<uint8_t>(auth->username.size()),
        });
        expected.insert(expected.end(), auth->username.begin(), auth->username.end());
        expected.push_back(static_cast<uint8_t>(auth->password.size()));
        expected.insert(expected.end(), auth->password.begin(), auth->password.end());
    }
    expected.insert(expected.end(), {
        0x05,
        0x01,
        0x00,
        0x03,
        static_cast<uint8_t>(dest.size()),
    });
    expected.insert(expected.end(), dest.begin(), dest.end());
    expected.push_back((port >> 8) & 0xFF);
    expected.push_back(port & 0xFF);
    return expected;
}
};

void initialize_socks5()
{
    static const auto testing_setup = MakeNoLogFileContext<const BasicTestingSetup>();
    default_socks5_recv_timeout = g_socks5_recv_timeout;
}

FUZZ_TARGET(socks5, .init = initialize_socks5)
{
    ResetFuzzedSockMockedFds();
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    FakeSteadyClock steady_clock;
    ProxyCredentials proxy_credentials;
    proxy_credentials.username = fuzzed_data_provider.ConsumeRandomLengthString(512);
    proxy_credentials.password = fuzzed_data_provider.ConsumeRandomLengthString(512);
    if (fuzzed_data_provider.ConsumeBool()) {
        g_socks5_interrupt();
    }
    // Set FUZZED_SOCKET_FAKE_LATENCY=1 to exercise recv timeout code paths. This
    // will slow down fuzzing.
    g_socks5_recv_timeout = (fuzzed_data_provider.ConsumeBool() && std::getenv("FUZZED_SOCKET_FAKE_LATENCY") != nullptr) ? 1ms : default_socks5_recv_timeout;
    FuzzedSock fuzzed_sock = ConsumeSock(fuzzed_data_provider, steady_clock);
    // This Socks5(...) fuzzing harness would have caught CVE-2017-18350 within
    // a few seconds of fuzzing.
    auto str_dest = fuzzed_data_provider.ConsumeRandomLengthString(512);
    auto port = fuzzed_data_provider.ConsumeIntegral<uint16_t>();
    auto* auth = fuzzed_data_provider.ConsumeBool() ? &proxy_credentials : nullptr;
    (void)Socks5(str_dest, port, auth, fuzzed_sock);

    g_socks5_interrupt.reset();
    auto pipes{std::make_shared<DynSock::Pipes>()};
    DynSock scripted_sock{pipes};
    if (str_dest.size() > 255) {
        Assert(!Socks5(str_dest, port, auth, scripted_sock));
        AssertNoBytes(pipes->send);
        return;
    }
    if (auth && (auth->username.size() > 255 || auth->password.size() > 255)) {
        PushBytes(pipes->recv, {0x05, 0x02});
        Assert(!Socks5(str_dest, port, auth, scripted_sock));
        const std::vector<uint8_t> expected_init{0x05, 0x02, 0x00, 0x02};
        Assert(ReadBytes(pipes->send, expected_init.size()) == expected_init);
        AssertNoBytes(pipes->send);
        return;
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        PushBytes(pipes->recv, MakeSocks5MethodSelectionFailureReply(auth, fuzzed_data_provider));
        Assert(!Socks5(str_dest, port, auth, scripted_sock));
        const auto expected{ExpectedSocks5MethodSelectionBytes(auth)};
        Assert(ReadBytes(pipes->send, expected.size()) == expected);
        AssertNoBytes(pipes->send);
        return;
    }
    const std::vector<uint8_t> sentinel{0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33};
    if (auth && fuzzed_data_provider.ConsumeBool()) {
        PushBytes(pipes->recv, MakeSocks5MethodSelectionReply(/*select_auth=*/true));
        PushBytes(pipes->recv, MakeSocks5AuthFailureReply(fuzzed_data_provider));
        PushBytes(pipes->recv, sentinel);
        Assert(!Socks5(str_dest, port, auth, scripted_sock));
        const auto expected_method{ExpectedSocks5MethodSelectionBytes(auth)};
        Assert(ReadBytes(pipes->send, expected_method.size()) == expected_method);
        std::vector<uint8_t> expected_auth{
            0x01,
            static_cast<uint8_t>(auth->username.size()),
        };
        expected_auth.insert(expected_auth.end(), auth->username.begin(), auth->username.end());
        expected_auth.push_back(static_cast<uint8_t>(auth->password.size()));
        expected_auth.insert(expected_auth.end(), auth->password.begin(), auth->password.end());
        Assert(ReadBytes(pipes->send, expected_auth.size()) == expected_auth);
        AssertNoBytes(pipes->send);
        Assert(ReadBytes(pipes->recv, sentinel.size()) == sentinel);
        return;
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        PushBytes(pipes->recv, MakeSocks5MethodSelectionReply(auth != nullptr));
        if (auth) PushBytes(pipes->recv, MakeSocks5AuthSuccessReply());
        PushBytes(pipes->recv, MakeSocks5ConnectFailureReply(fuzzed_data_provider));
        PushBytes(pipes->recv, sentinel);
        Assert(!Socks5(str_dest, port, auth, scripted_sock));
        const auto expected{ExpectedSocks5ClientBytes(str_dest, port, auth)};
        Assert(ReadBytes(pipes->send, expected.size()) == expected);
        AssertNoBytes(pipes->send);
        Assert(ReadBytes(pipes->recv, sentinel.size()) == sentinel);
        return;
    }
    PushBytes(pipes->recv, MakeSocks5SuccessReply(auth != nullptr));
    Assert(Socks5(str_dest, port, auth, scripted_sock));
    const auto expected{ExpectedSocks5ClientBytes(str_dest, port, auth)};
    Assert(ReadBytes(pipes->send, expected.size()) == expected);
    AssertNoBytes(pipes->send);
}
