// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <net.h>
#include <net_processing.h>
#include <node/protocol_version.h>
#include <protocol.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/validation.h>
#include <util/time.h>
#include <validationinterface.h>

#include <algorithm>
#include <ios>
#include <utility>
#include <vector>

namespace {
TestingSetup* g_setup;

void initialize()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>(
        /*chain_type=*/ChainType::REGTEST);
    g_setup = testing_setup.get();
}

void AssertHandshakeState(const std::vector<CNode*>& peers)
{
    for (const CNode* peer : peers) {
        const int version{peer->nVersion.load()};
        if (version == 0) {
            Assert(!peer->fSuccessfullyConnected.load());
            Assert(peer->GetCommonVersion() == INIT_PROTO_VERSION);
            continue;
        }

        Assert(version >= MIN_PEER_PROTO_VERSION);
        Assert(peer->GetCommonVersion() == std::min(version, peer->AdvertisedVersion()));
        if (peer->fSuccessfullyConnected.load()) {
            Assert(version != 0);
        }
    }
}
} // namespace

FUZZ_TARGET(p2p_handshake, .init = ::initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    auto& node{g_setup->m_node};
    auto& connman{static_cast<ConnmanTestMsg&>(*node.connman)};
    auto& chainman{static_cast<TestChainstateManager&>(*node.chainman)};
    FakeNodeClock clock{1610000000s}; // any time to successfully reset ibd
    FakeSteadyClock steady_clock;
    chainman.ResetIbd();

    node.banman.reset();
    node.addrman.reset();
    node.peerman.reset();
    node.addrman = std::make_unique<AddrMan>(
        *node.netgroupman, /*deterministic=*/true, /*consistency_check_ratio=*/0);
    node.peerman = PeerManager::make(connman, *node.addrman,
                                     /*banman=*/nullptr, chainman,
                                     *node.mempool, *node.warnings,
                                     PeerManager::Options{
                                         .reconcile_txs = true,
                                         .deterministic_rng = true,
                                     });
    connman.SetMsgProc(node.peerman.get());
    connman.SetAddrman(*node.addrman);

    LOCK(NetEventsInterface::g_msgproc_mutex);

    std::vector<CNode*> peers;
    const auto num_peers_to_add = fuzzed_data_provider.ConsumeIntegralInRange(1, 3);
    for (int i = 0; i < num_peers_to_add; ++i) {
        peers.push_back(ConsumeNodeAsUniquePtr(fuzzed_data_provider, steady_clock, i).release());
        connman.AddTestNode(*peers.back());
        node.peerman->InitializeNode(
            *peers.back(),
            static_cast<ServiceFlags>(fuzzed_data_provider.ConsumeIntegral<uint64_t>()));
    }

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 100) {
        CNode& connection = *PickValue(fuzzed_data_provider, peers);
        if (connection.fDisconnect) {
            // Skip if the connection was disconnected.
            continue;
        }
        const bool send_late_feature{connection.fSuccessfullyConnected.load() && fuzzed_data_provider.ConsumeBool()};
        if (connection.fSuccessfullyConnected.load() && !send_late_feature) {
            // Skip most messages once the version handshake was completed; this target is focused
            // on handshake transitions. Still permit a late FEATURE, which must disconnect.
            continue;
        }

        clock += std::chrono::seconds{
                    fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(
                        -std::chrono::seconds{10min}.count(), // Allow mocktime to go backwards slightly
                        std::chrono::seconds{TIMEOUT_INTERVAL}.count()),
        };

        CSerializedNetMsg net_msg;
        net_msg.m_type = send_late_feature ? NetMsgType::FEATURE : PickValue(fuzzed_data_provider, ALL_NET_MESSAGE_TYPES);
        net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider, MAX_PROTOCOL_MESSAGE_LENGTH);

        connman.FlushSendBuffer(connection);
        (void)connman.ReceiveMsgFrom(connection, std::move(net_msg));

        bool more_work{true};
        while (more_work) {
            connection.fPauseSend = false;

            try {
                more_work = connman.ProcessMessagesOnce(connection);
            } catch (const std::ios_base::failure&) {
            }
            node.peerman->SendMessages(connection);
        }
        if (send_late_feature) Assert(connection.fDisconnect.load());
        AssertHandshakeState(peers);
    }

    AssertHandshakeState(peers);
    node.connman->StopNodes();
}
