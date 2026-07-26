// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <chainparams.h>
#include <common/args.h>
#include <net.h>
#include <net_processing.h>
#include <netaddress.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/fuzz/util/threadinterrupt.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/translation.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace {
const TestingSetup* g_setup;

int32_t GetCheckRatio()
{
    return std::clamp<int32_t>(g_setup->m_node.args->GetIntArg("-checkaddrman", 0), 0, 1000000);
}

void AssertDistinctFuzzedSockMapKeys()
{
    std::array<uint8_t, 1> bytes{};
    FuzzedDataProvider provider{bytes.data(), bytes.size()};
    FakeSteadyClock steady_clock;
    auto sock_a{std::make_shared<FuzzedSock>(provider, steady_clock)};
    auto sock_b{std::make_shared<FuzzedSock>(provider, steady_clock)};
    Sock::EventsPerSock events;
    assert(events.emplace(sock_a, Sock::Events{Sock::RecvEvent}).second);
    assert(events.emplace(sock_b, Sock::Events{Sock::SendEvent}).second);
}

} // namespace

void initialize_connman()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(connman, .init = initialize_connman)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    ResetFuzzedSockMockedFds();
    AssertDistinctFuzzedSockMapKeys();
    ResetFuzzedSockMockedFds();
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    FakeSteadyClock steady_clock;
    auto netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    auto addr_man_ptr{std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider, GetCheckRatio())};
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<uint8_t> serialized_data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
        DataStream ds{serialized_data};
        try {
            ds >> *addr_man_ptr;
        } catch (const std::ios_base::failure&) {
            addr_man_ptr = std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider, GetCheckRatio());
        }
    }
    AddrManDeterministic& addr_man{*addr_man_ptr};
    auto net_events{ConsumeNetEvents(fuzzed_data_provider)};

    // Mock CreateSock() to create FuzzedSock.
    auto CreateSockOrig = CreateSock;
    CreateSock = [&fuzzed_data_provider, &steady_clock](int, int, int) {
        return std::make_unique<FuzzedSock>(fuzzed_data_provider, steady_clock);
    };

    // Mock g_dns_lookup() to return a fuzzed address.
    auto g_dns_lookup_orig = g_dns_lookup;
    g_dns_lookup = [&fuzzed_data_provider](const std::string&, bool) {
        return std::vector<CNetAddr>{ConsumeNetAddr(fuzzed_data_provider)};
    };

    ConnmanTestMsg connman{fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
                     fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
                     addr_man,
                     netgroupman,
                     Params(),
                     fuzzed_data_provider.ConsumeBool(),
                     ConsumeThreadInterrupt(fuzzed_data_provider)};

    const uint64_t max_outbound_limit{fuzzed_data_provider.ConsumeIntegral<uint64_t>()};
    CConnman::Options options;
    options.m_msgproc = &net_events;
    options.nMaxOutboundLimit = max_outbound_limit;

    const auto local_services{ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS)};
    options.m_local_services = local_services;

    const auto use_addrman_outgoing{fuzzed_data_provider.ConsumeBool()};
    options.m_use_addrman_outgoing = use_addrman_outgoing;
    options.m_max_automatic_connections = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 1000);

    auto consume_whitelist = [&]() {
        std::vector<NetWhitelistPermissions> result(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 3));
        for (auto& entry : result) {
            entry.m_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
            entry.m_subnet = ConsumeSubNet(fuzzed_data_provider);
        }
        return result;
    };
    options.vWhitelistedRangeIncoming = consume_whitelist();
    options.vWhitelistedRangeOutgoing = consume_whitelist();

    connman.Init(options);

    const uint64_t total_bytes_recv_initial{connman.GetTotalBytesRecv()};
    const uint64_t total_bytes_sent_initial{connman.GetTotalBytesSent()};

    CNetAddr random_netaddr;
    CAddress random_address;
    CNode random_node = ConsumeNode(fuzzed_data_provider, steady_clock);
    CSubNet random_subnet;
    std::string random_string;
    std::vector<NodeId> node_ids;
    std::vector<std::string> node_addr_names;

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 100) {
        CNode& p2p_node{*ConsumeNodeAsUniquePtr(fuzzed_data_provider, steady_clock).release()};
        // Simulate post-handshake state.
        p2p_node.fSuccessfullyConnected = true;
        connman.AddTestNode(p2p_node);
        node_ids.push_back(p2p_node.GetId());
        node_addr_names.push_back(p2p_node.m_addr_name);
    }

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                random_netaddr = ConsumeNetAddr(fuzzed_data_provider);
            },
            [&] {
                random_address = ConsumeAddress(fuzzed_data_provider);
            },
            [&] {
                random_subnet = ConsumeSubNet(fuzzed_data_provider);
            },
            [&] {
                random_string = fuzzed_data_provider.ConsumeRandomLengthString(64);
            },
            [&] {
                const std::string& node_str = (!node_addr_names.empty() && fuzzed_data_provider.ConsumeBool())
                    ? PickValue(fuzzed_data_provider, node_addr_names)
                    : random_string;
                const auto added_node_info{connman.GetAddedNodeInfo(/*include_connected=*/true)};
                const auto add_node{connman.AddNode({node_str, /*use_v2transport=*/fuzzed_data_provider.ConsumeBool()})};
                if (add_node) {
                    assert(!connman.AddNode({node_str, /*use_v2transport=*/fuzzed_data_provider.ConsumeBool()}));
                    assert(added_node_info.size() < connman.GetAddedNodeInfo(/*include_connected=*/true).size());
                    const auto remove{fuzzed_data_provider.ConsumeBool()};
                    if (remove) {
                        assert(connman.RemoveAddedNode(node_str));
                        assert(added_node_info.size() == connman.GetAddedNodeInfo(/*include_connected=*/true).size());
                    }
                }
            },
            [&] {
                (void)connman.RemoveAddedNode(random_string);
            },
            [&] {
                connman.CheckIncomingNonce(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                connman.DisconnectNode(fuzzed_data_provider.ConsumeIntegral<NodeId>());
            },
            [&] {
                connman.DisconnectNode(random_netaddr);
            },
            [&] {
                connman.DisconnectNode(random_string);
            },
            [&] {
                connman.DisconnectNode(random_subnet);
            },
            [&] {
                NodeId id = node_ids.empty() || fuzzed_data_provider.ConsumeBool()
                    ? fuzzed_data_provider.ConsumeIntegral<NodeId>()
                    : PickValue(fuzzed_data_provider, node_ids);
                (void)connman.ForNode(id, [&](CNode* pnode) {
                    (void)pnode->GetId();
                    (void)pnode->IsInboundConn();
                    (void)pnode->IsFullOutboundConn();
                    return true;
                });
            },
            [&] {
                auto max_addresses = fuzzed_data_provider.ConsumeIntegral<size_t>();
                auto max_pct = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 100);
                auto filtered = fuzzed_data_provider.ConsumeBool();
                (void)connman.GetAddressesUnsafe(max_addresses, max_pct, /*network=*/std::nullopt, filtered);
            },
            [&] {
                auto max_addresses = fuzzed_data_provider.ConsumeIntegral<size_t>();
                auto max_pct = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 100);
                (void)connman.GetAddresses(/*requestor=*/random_node, max_addresses, max_pct);
            },
            [&] {
                (void)connman.GetDeterministicRandomizer(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                (void)connman.GetNodeCount(fuzzed_data_provider.PickValueInArray({ConnectionDirection::None, ConnectionDirection::In, ConnectionDirection::Out, ConnectionDirection::Both}));
            },
            [&] {
                (void)connman.OutboundTargetReached(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                CSerializedNetMsg serialized_net_msg;
                serialized_net_msg.m_type = fuzzed_data_provider.ConsumeRandomLengthString(CMessageHeader::MESSAGE_TYPE_SIZE);
                serialized_net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                connman.PushMessage(&random_node, std::move(serialized_net_msg));
            },
            [&] {
                const auto set_active{fuzzed_data_provider.ConsumeBool()};
                connman.SetNetworkActive(set_active);
                assert(connman.GetNetworkActive() == set_active);
            },
            [&] {
                connman.SetTryNewOutboundPeer(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                const auto services{ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS)};
                const auto before{connman.GetLocalServices()};
                if (fuzzed_data_provider.ConsumeBool()) {
                    connman.AddLocalServices(services);
                    assert((connman.GetLocalServices() & services) == services);
                    // Restore by clearing only the bits that weren't already set.
                    connman.RemoveLocalServices(ServiceFlags(services & ~before));
                } else {
                    connman.RemoveLocalServices(services);
                    assert((connman.GetLocalServices() & services) == 0);
                    // Restore by re-adding only the bits that were previously set.
                    connman.AddLocalServices(ServiceFlags(services & before));
                }
                assert(connman.GetLocalServices() == before);
            },
            [&] {
                ConnectionType conn_type{
                    fuzzed_data_provider.PickValueInArray(ALL_CONNECTION_TYPES)};
                if (conn_type == ConnectionType::INBOUND) { // INBOUND is not allowed
                    conn_type = ConnectionType::OUTBOUND_FULL_RELAY;
                }

                std::optional<Proxy> proxy_override;
                if (conn_type == ConnectionType::PRIVATE_BROADCAST || fuzzed_data_provider.ConsumeBool()) {
                    proxy_override.emplace(ConsumeService(fuzzed_data_provider));
                }

                connman.OpenNetworkConnection(
                    /*addrConnect=*/random_address,
                    /*fCountFailure=*/fuzzed_data_provider.ConsumeBool(),
                    /*grant_outbound=*/{},
                    /*pszDest=*/fuzzed_data_provider.ConsumeBool() ? nullptr : random_string.c_str(),
                    /*conn_type=*/conn_type,
                    /*use_v2transport=*/fuzzed_data_provider.ConsumeBool(),
                    /*proxy_override=*/proxy_override);
            },
            [&] {
                connman.SetNetworkActive(fuzzed_data_provider.ConsumeBool());
                const auto peer = ConsumeAddress(fuzzed_data_provider);
                connman.CreateNodeFromAcceptedSocketPublic(
                    /*sock=*/CreateSock(AF_INET, SOCK_STREAM, IPPROTO_TCP),
                    /*permissions=*/ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS),
                    /*addr_bind=*/ConsumeAddress(fuzzed_data_provider),
                    /*addr_peer=*/peer);
            },
            [&] {
                CConnman::Options options;

                options.vBinds = ConsumeServiceVector(fuzzed_data_provider);

                options.vWhiteBinds = std::vector<NetWhitebindPermissions>{
                    fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 5)};
                for (auto& wb : options.vWhiteBinds) {
                    wb.m_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
                    wb.m_service = ConsumeService(fuzzed_data_provider);
                }

                options.onion_binds = ConsumeServiceVector(fuzzed_data_provider);

                options.bind_on_any = options.vBinds.empty() && options.vWhiteBinds.empty() &&
                                      options.onion_binds.empty();

                connman.InitBindsPublic(options);
            },
            [&] {
                connman.SocketHandlerPublic();
            });
    }
    connman.ForEachNode([](CNode* pnode) {
        (void)pnode->GetId();
        (void)pnode->IsInboundConn();
        (void)pnode->IsFullOutboundConn();
        (void)pnode->ConnectionTypeAsString();
    });
    (void)connman.GetAddedNodeInfo(/*include_connected=*/false);
    (void)connman.GetExtraFullOutboundCount();
    assert(connman.GetLocalServices() == local_services);
    assert(connman.GetMaxOutboundTarget() == max_outbound_limit);
    const auto time_left_in_cycle{connman.GetMaxOutboundTimeLeftInCycle()};
    std::vector<CNodeStats> stats;
    connman.GetNodeStats(stats);
    const auto bytes_left{connman.GetOutboundTargetBytesLeft()};
    assert(bytes_left <= max_outbound_limit);
    if (max_outbound_limit == 0) {
        assert(bytes_left == 0);
        assert(time_left_in_cycle == std::chrono::seconds{0});
        assert(!connman.OutboundTargetReached(/*historicalBlockServingLimit=*/false));
        assert(!connman.OutboundTargetReached(/*historicalBlockServingLimit=*/true));
    }
    assert(connman.GetTotalBytesRecv() >= total_bytes_recv_initial);
    assert(connman.GetTotalBytesSent() >= total_bytes_sent_initial);
    (void)connman.GetTryNewOutboundPeer();
    assert(connman.GetUseAddrmanOutgoing() == use_addrman_outgoing);
    (void)connman.ASMapHealthCheck();

    connman.ClearTestNodes();
    g_dns_lookup = g_dns_lookup_orig;
    CreateSock = CreateSockOrig;
}

FUZZ_TARGET(connman_inbound_relay_capacity, .init = initialize_connman)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    ResetFuzzedSockMockedFds();
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{1610000000s};
    FakeSteadyClock steady_clock;

    auto netgroupman{ConsumeNetGroupManager(provider)};
    auto addr_man{std::make_unique<AddrManDeterministic>(netgroupman, provider, GetCheckRatio())};
    ConnmanTestMsg connman{provider.ConsumeIntegral<uint64_t>(),
                           provider.ConsumeIntegral<uint64_t>(),
                           *addr_man,
                           netgroupman,
                           Params()};

    CConnman::Options options;
    options.m_max_automatic_connections = provider.ConsumeIntegralInRange<int>(0, 200);
    options.m_full_relay_inbound_percent = provider.ConsumeIntegralInRange<int>(0, 100);
    connman.Init(options);

    const int max_inbound{std::max(0, options.m_max_automatic_connections -
                                          MAX_OUTBOUND_FULL_RELAY_CONNECTIONS -
                                          MAX_BLOCK_RELAY_ONLY_CONNECTIONS -
                                          MAX_FEELER_CONNECTIONS)};
    const int max_inbound_full_relay{std::max(
        0, static_cast<int>(options.m_full_relay_inbound_percent / 100.0 * max_inbound))};

    std::vector<CNode*> nodes;
    const size_t num_nodes{provider.ConsumeIntegralInRange<size_t>(1, 64)};
    for (size_t i{0}; i < num_nodes; ++i) {
        const ConnectionType connection_type{i == 0
                                                  ? ConnectionType::INBOUND
                                                  : provider.PickValueInArray(ALL_CONNECTION_TYPES)};
        auto node{std::make_unique<CNode>(
            static_cast<NodeId>(i),
            std::make_shared<FuzzedSock>(provider, steady_clock),
            ConsumeAddress(provider),
            provider.ConsumeIntegral<uint64_t>(),
            provider.ConsumeIntegral<uint64_t>(),
            ConsumeAddress(provider),
            provider.ConsumeRandomLengthString(64),
            connection_type,
            connection_type == ConnectionType::INBOUND && provider.ConsumeBool(),
            provider.ConsumeIntegral<uint64_t>(),
            CNodeOptions{
                .permission_flags = ConsumeWeakEnum(provider, ALL_NET_PERMISSION_FLAGS),
                .prefer_evict = provider.ConsumeBool(),
            })};
        node->m_relays_txs = i == 0 || provider.ConsumeBool();
        node->m_bloom_filter_loaded = provider.ConsumeBool();
        node->m_has_all_wanted_services = provider.ConsumeBool();
        node->m_last_block_time = ConsumeTime(provider).time_since_epoch();
        node->m_last_tx_time = ConsumeTime(provider).time_since_epoch();
        node->m_min_ping_time = ConsumeDuration<decltype(CNode::m_min_ping_time.load())>(
            provider, /*min=*/std::chrono::microseconds{0}, /*max=*/std::chrono::seconds{1});
        node->fDisconnect = i != 0 && provider.ConsumeBool();
        nodes.push_back(node.release());
        connman.AddTestNode(*nodes.back());
    }

    const std::optional<NodeId> protect_peer{provider.ConsumeBool()
                                                 ? std::optional<NodeId>{provider.ConsumeIntegralInRange<NodeId>(
                                                       0, static_cast<NodeId>(nodes.size() - 1))}
                                                 : std::nullopt};

    const auto check_eviction{[&](const std::optional<NodeId> protected_peer) {
        std::vector<bool> disconnected_before;
        disconnected_before.reserve(nodes.size());
        for (const CNode* node : nodes) disconnected_before.push_back(node->fDisconnect.load());

        int live_inbound_tx_peers{0};
        for (const CNode* node : nodes) {
            if (!node->fDisconnect && node->IsInboundConn() && node->m_relays_txs) {
                ++live_inbound_tx_peers;
            }
        }

        const bool result{connman.EvictTxPeerIfFull(protected_peer)};
        size_t newly_disconnected{0};
        for (size_t i{0}; i < nodes.size(); ++i) {
            if (!disconnected_before[i] && nodes[i]->fDisconnect) {
                ++newly_disconnected;
                assert(nodes[i]->IsInboundConn());
                assert(nodes[i]->m_relays_txs);
                assert(!protected_peer || nodes[i]->GetId() != *protected_peer);
            }
        }

        if (live_inbound_tx_peers <= max_inbound_full_relay) {
            assert(result);
            assert(newly_disconnected == 0);
        } else if (result) {
            assert(newly_disconnected == 1);
        } else {
            assert(newly_disconnected == 0);
        }
    }};

    check_eviction(protect_peer);
    if (provider.ConsumeBool()) {
        check_eviction(provider.ConsumeBool() && !nodes.empty()
                           ? std::optional<NodeId>{provider.ConsumeIntegralInRange<NodeId>(
                                 0, static_cast<NodeId>(nodes.size() - 1))}
                           : std::nullopt);
    }

    connman.ClearTestNodes();
    ResetFuzzedSockMockedFds();
}
