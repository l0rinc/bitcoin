// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <torcontrol.h>

#include <compat/compat.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void initialize_torcontrol()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET(torcontrol, .init = initialize_torcontrol)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    TorController tor_controller;
    CThreadInterrupt interrupt;
    TorControlConnection conn{interrupt};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        TorControlReply tor_control_reply;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tor_control_reply.code = TOR_REPLY_OK;
            },
            [&] {
                tor_control_reply.code = TOR_REPLY_UNRECOGNIZED;
            },
            [&] {
                tor_control_reply.code = TOR_REPLY_SYNTAX_ERROR;
            },
            [&] {
                tor_control_reply.code = fuzzed_data_provider.ConsumeIntegral<int>();
            });
        tor_control_reply.lines = ConsumeRandomLengthStringVector(fuzzed_data_provider);

        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tor_controller.add_onion_cb(conn, tor_control_reply, /*pow_was_enabled=*/true);
            },
            [&] {
                tor_controller.add_onion_cb(conn, tor_control_reply, /*pow_was_enabled=*/false);
            },
            [&] {
                tor_controller.auth_cb(conn, tor_control_reply);
            },
            [&] {
                tor_controller.authchallenge_cb(conn, tor_control_reply);
            },
            [&] {
                tor_controller.protocolinfo_cb(conn, tor_control_reply);
            },
            [&] {
                // Seed a distinct invalid post-state so a no-op or wrong-network
                // production update cannot be hidden by a previous fuzz input.
                const Proxy sentinel{CService{in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT), /*port=*/1}, /*tor_stream_isolation=*/false};
                assert(SetProxy(NET_ONION, sentinel));
                tor_controller.get_socks_cb(conn, tor_control_reply);
                const auto configured_proxy{GetProxy(NET_ONION)};
                assert(configured_proxy.has_value());
                assert(configured_proxy->IsValid());
                assert(configured_proxy->m_tor_stream_isolation);
                assert(g_reachable_nets.Contains(NET_ONION));
            });
    }
}
