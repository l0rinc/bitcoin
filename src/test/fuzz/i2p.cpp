// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <compat/compat.h>
#include <i2p.h>
#include <netaddress.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/fuzz/util/threadinterrupt.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/fs_helpers.h>
#include <util/threadinterrupt.h>

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace {
bool IsExpectedSocketReceiveError(std::string_view error)
{
    return error == "Connection unexpectedly closed by peer" ||
           error.starts_with("recv(): ") ||
           error.starts_with("Received too many bytes without a terminator (") ||
           error.starts_with("Receive timeout (received ") ||
           error.starts_with("Receive interrupted (received ") ||
           error.starts_with("recv() returned ");
}

bool IsExpectedSocketSendError(std::string_view error)
{
    return error.starts_with("send(): ") ||
           error.starts_with("Send timeout (sent only ") ||
           error.starts_with("Send interrupted (sent only ");
}
} // namespace

void initialize_i2p()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET(i2p, .init = initialize_i2p)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    ResetFuzzedSockMockedFds();
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    FakeSteadyClock steady_clock;

    // Mock CreateSock() to create FuzzedSock.
    auto CreateSockOrig = CreateSock;
    CreateSock = [&fuzzed_data_provider, &steady_clock](int, int, int) {
        return std::make_unique<FuzzedSock>(fuzzed_data_provider, steady_clock);
    };

    const fs::path private_key_path = gArgs.GetDataDirNet() / "fuzzed_i2p_private_key";
    const CService addr{in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT), 7656};
    const Proxy sam_proxy{addr, /*tor_stream_isolation=*/false};
    auto interrupt{ConsumeThreadInterrupt(fuzzed_data_provider)};

    i2p::sam::Session session{private_key_path, sam_proxy, interrupt};
    i2p::Connection conn;

    if (session.Listen(conn)) {
        assert(conn.sock != nullptr);
        assert(conn.me.IsI2P());
        assert(conn.me.GetPort() == I2P_SAM31_PORT);
        if (session.Accept(conn)) {
            assert(conn.peer.IsI2P());
            assert(conn.peer.GetPort() == I2P_SAM31_PORT);
            try {
                (void)conn.sock->RecvUntilTerminator('\n', 10ms, *interrupt, i2p::sam::MAX_MSG_SIZE);
            } catch (const std::runtime_error& e) {
                assert(IsExpectedSocketReceiveError(e.what()));
            }
        }
    }

    bool proxy_error;

    i2p::Connection outbound_conn;
    const uint16_t port{fuzzed_data_provider.ConsumeBool() ? I2P_SAM31_PORT : fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    const CService to{ConsumeNetAddr(fuzzed_data_provider), port};
    if (session.Connect(to, outbound_conn, proxy_error)) {
        assert(port == I2P_SAM31_PORT);
        assert(outbound_conn.sock != nullptr);
        assert(outbound_conn.peer == to);
        assert(outbound_conn.me.IsI2P());
        assert(outbound_conn.me.GetPort() == I2P_SAM31_PORT);
        try {
            outbound_conn.sock->SendComplete("verack\n", 10ms, *interrupt);
        } catch (const std::runtime_error& e) {
            assert(IsExpectedSocketSendError(e.what()));
        }
    } else if (port != I2P_SAM31_PORT) {
        assert(!proxy_error);
        assert(outbound_conn.sock == nullptr);
    }

    fs::remove(private_key_path);

    CreateSock = CreateSockOrig;
}
