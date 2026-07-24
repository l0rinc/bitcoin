// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/time.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>

extern std::chrono::milliseconds g_socks5_recv_timeout;

namespace {
decltype(g_socks5_recv_timeout) default_socks5_recv_timeout;

class TranscriptSock final : public Sock
{
public:
    explicit TranscriptSock(std::vector<uint8_t> response)
        : Sock{INVALID_SOCKET}, m_response{std::move(response)}
    {
    }

    ssize_t Send(const void* data, size_t len, int) const override
    {
        if (len != 0) {
            const auto* bytes{static_cast<const uint8_t*>(data)};
            m_sent.insert(m_sent.end(), bytes, bytes + len);
        }
        return len;
    }

    ssize_t Recv(void* data, size_t len, int) const override
    {
        if (len == 0 || m_response_pos == m_response.size()) return 0;
        const size_t ret{std::min<size_t>(1, std::min(len, m_response.size() - m_response_pos))};
        std::memcpy(data, m_response.data() + m_response_pos, ret);
        m_response_pos += ret;
        return ret;
    }

    bool IsSelectable() const override { return true; }

    bool Wait(std::chrono::milliseconds, Event requested, Event* occurred = nullptr) const override
    {
        if (occurred != nullptr) *occurred = requested;
        return true;
    }

    const std::vector<uint8_t>& Sent() const { return m_sent; }
    size_t Remaining() const { return m_response.size() - m_response_pos; }

private:
    const std::vector<uint8_t> m_response;
    mutable size_t m_response_pos{0};
    mutable std::vector<uint8_t> m_sent;
};

std::vector<uint8_t> MakeSuccessfulResponse(bool auth, uint8_t atyp)
{
    std::vector<uint8_t> response{5, static_cast<uint8_t>(auth ? 2 : 0)};
    if (auth) response.insert(response.end(), {1, 0});
    response.insert(response.end(), {5, 0, 0, atyp});
    switch (atyp) {
    case 1:
        response.insert(response.end(), {127, 0, 0, 1});
        break;
    case 3:
        response.insert(response.end(), {3, 'f', 'o', 'o'});
        break;
    case 4:
        response.insert(response.end(), 16, 0);
        break;
    default:
        assert(false);
    }
    response.insert(response.end(), {0, 0});
    return response;
}

void AssertSuccessfulTranscript(std::span<const uint8_t> buffer)
{
    FuzzedDataProvider transcript_provider{buffer.data(), buffer.size()};
    const std::string str_dest{transcript_provider.ConsumeRandomLengthString(255)};
    ProxyCredentials proxy_credentials;
    proxy_credentials.username = transcript_provider.ConsumeRandomLengthString(255);
    proxy_credentials.password = transcript_provider.ConsumeRandomLengthString(255);
    const bool use_auth{transcript_provider.ConsumeBool()};
    const uint16_t port{transcript_provider.ConsumeIntegral<uint16_t>()};
    constexpr std::array<uint8_t, 3> atypes{1, 3, 4};
    const uint8_t atyp{atypes[transcript_provider.ConsumeIntegralInRange<size_t>(0, atypes.size() - 1)]};

    TranscriptSock sock{MakeSuccessfulResponse(use_auth, atyp)};
    g_socks5_interrupt.reset();
    const bool result{Socks5(str_dest, port, use_auth ? &proxy_credentials : nullptr, sock)};
    assert(result);
    assert(sock.Remaining() == 0);

    std::vector<uint8_t> expected{
        5,
        static_cast<uint8_t>(use_auth ? 2 : 1),
        0,
    };
    if (use_auth) expected.push_back(2);
    if (use_auth) {
        expected.push_back(1);
        expected.push_back(proxy_credentials.username.size());
        expected.insert(expected.end(), proxy_credentials.username.begin(), proxy_credentials.username.end());
        expected.push_back(proxy_credentials.password.size());
        expected.insert(expected.end(), proxy_credentials.password.begin(), proxy_credentials.password.end());
    }
    expected.insert(expected.end(), {5, 1, 0, 3, static_cast<uint8_t>(str_dest.size())});
    expected.insert(expected.end(), str_dest.begin(), str_dest.end());
    expected.push_back(port >> 8);
    expected.push_back(port);
    assert(sock.Sent() == expected);
}
} // namespace

void initialize_socks5()
{
    static const auto testing_setup = MakeNoLogFileContext<const BasicTestingSetup>();
    default_socks5_recv_timeout = g_socks5_recv_timeout;
}

FUZZ_TARGET(socks5, .init = initialize_socks5)
{
    g_socks5_interrupt.reset();
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
    const bool result{Socks5(str_dest, port, auth, fuzzed_sock)};
    assert(!result || str_dest.size() <= 255);

    g_socks5_interrupt.reset();
    AssertSuccessfulTranscript(buffer);
}
