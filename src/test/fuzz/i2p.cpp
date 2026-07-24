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

void initialize_i2p()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET(i2p, .init = initialize_i2p)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
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
        assert(conn.me.IsValid());
        assert(conn.me.IsI2P());
        assert(conn.me.GetPort() == I2P_SAM31_PORT);

        if (session.Accept(conn)) {
            assert(conn.sock != nullptr);
            assert(conn.peer.IsValid());
            assert(conn.peer.IsI2P());
            assert(conn.peer.GetPort() == I2P_SAM31_PORT);

            try {
                (void)conn.sock->RecvUntilTerminator('\n', 10ms, *interrupt, i2p::sam::MAX_MSG_SIZE);
            } catch (const std::runtime_error&) {
            }
        }
    }

    // The port guard must reject a non-SAM port without touching the output connection.
    i2p::Connection invalid_port_conn;
    bool proxy_error{true};
    const CService invalid_port_destination{in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT), /*port=*/1};
    assert(!session.Connect(invalid_port_destination, invalid_port_conn, proxy_error));
    assert(!proxy_error);
    assert(invalid_port_conn.sock == nullptr);
    assert(!invalid_port_conn.me.IsValid());
    assert(!invalid_port_conn.peer.IsValid());

    // ConsumeNetAddr can produce a valid I2P name. Only those inputs belong to the
    // protocol path; other networks are not valid Session::Connect callers.
    const CNetAddr destination_addr{ConsumeNetAddr(fuzzed_data_provider)};
    if (destination_addr.IsValid() && destination_addr.IsI2P()) {
        const CService destination{destination_addr, I2P_SAM31_PORT};
        i2p::Connection outgoing_conn;
        proxy_error = false;

        if (session.Connect(destination, outgoing_conn, proxy_error)) {
            assert(outgoing_conn.sock != nullptr);
            assert(outgoing_conn.me.IsValid());
            assert(outgoing_conn.me.IsI2P());
            assert(outgoing_conn.me.GetPort() == I2P_SAM31_PORT);
            assert(outgoing_conn.peer == destination);

            try {
                outgoing_conn.sock->SendComplete("verack\n", 10ms, *interrupt);
            } catch (const std::runtime_error&) {
            }
        } else {
            assert(outgoing_conn.sock == nullptr);
        }
    }

    fs::remove(private_key_path);

    CreateSock = CreateSockOrig;
}
