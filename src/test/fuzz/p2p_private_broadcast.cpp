// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <net.h>
#include <net_processing.h>
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
#include <array>
#include <ios>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <tuple>
#include <vector>

namespace {
TestingSetup* g_setup;

constexpr size_t NUM_PRIVATE_BROADCAST_PER_TX{3};

struct ModelPeerSend {
    NodeId nodeid;
    CService address;
    NodeClock::time_point sent;
    std::optional<NodeClock::time_point> received;
};

struct ModelTransaction {
    CTransactionRef tx;
    NodeClock::time_point time_added;
    size_t disconnected_unconfirmed_picks{0};
    NodeClock::time_point last_disconnected_unconfirmed_pick{};
    std::vector<ModelPeerSend> peers;
};

struct ModelPriority {
    size_t num_picked{0};
    NodeClock::time_point last_picked{};
    size_t num_confirmed{0};
    NodeClock::time_point last_confirmed{};
};

ModelPriority DerivePriority(const ModelTransaction& tx)
{
    ModelPriority priority{
        .num_picked = tx.disconnected_unconfirmed_picks,
        .last_picked = tx.last_disconnected_unconfirmed_pick,
    };
    for (const auto& peer : tx.peers) {
        ++priority.num_picked;
        priority.last_picked = std::max(priority.last_picked, peer.sent);
        if (peer.received) {
            ++priority.num_confirmed;
            priority.last_confirmed = std::max(priority.last_confirmed, *peer.received);
        }
    }
    return priority;
}

bool AtLeastAsUrgent(const ModelPriority& lhs, const ModelPriority& rhs)
{
    return std::tie(lhs.num_picked, lhs.num_confirmed, lhs.last_picked, lhs.last_confirmed) <=
           std::tie(rhs.num_picked, rhs.num_confirmed, rhs.last_picked, rhs.last_confirmed);
}

using ModelTransactions = std::map<uint256, ModelTransaction>;

ModelPeerSend* FindPeerSend(ModelTransactions& transactions, NodeId nodeid)
{
    for (auto& [_, tx] : transactions) {
        for (auto& peer : tx.peers) {
            if (peer.nodeid == nodeid) return &peer;
        }
    }
    return nullptr;
}

ModelTransaction* FindTransactionForNode(ModelTransactions& transactions, NodeId nodeid)
{
    for (auto& [_, tx] : transactions) {
        for (const auto& peer : tx.peers) {
            if (peer.nodeid == nodeid) return &tx;
        }
    }
    return nullptr;
}

void initialize()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>(
        /*chain_type=*/ChainType::REGTEST);
    g_setup = testing_setup.get();
}

// Inbound message types with private broadcast specific handling.
// Used as the guided path in the CallOneOf() below.
constexpr std::array INBOUND_MSG_TYPES{
    NetMsgType::VERSION,
    NetMsgType::VERACK,
    NetMsgType::GETDATA,
    NetMsgType::PONG,
};

void ProcessInbound(ConnmanTestMsg& connman, PeerManager& peerman, CNode& peer,
                    CSerializedNetMsg msg)
    EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
{
    connman.FlushSendBuffer(peer);
    (void)connman.ReceiveMsgFrom(peer, std::move(msg));

    bool more_work{true};
    while (more_work) {
        peer.fPauseSend = false;
        try {
            more_work = connman.ProcessMessagesOnce(peer);
        } catch (const std::ios_base::failure&) {
        }
        peerman.SendMessages(peer);
    }
}
} // namespace

FUZZ_TARGET(p2p_private_broadcast, .init = ::initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    auto& node{g_setup->m_node};
    auto& connman{static_cast<ConnmanTestMsg&>(*node.connman)};
    connman.Reset();
    auto& chainman{static_cast<TestChainstateManager&>(*node.chainman)};

    FakeNodeClock clock_ctx{1610000000s};
    FakeSteadyClock steady_clock;
    chainman.ResetIbd();
    // Sometimes leave IBD: incoming TX processing (the broadcast-abort path)
    // returns early during IBD.
    if (fuzzed_data_provider.ConsumeBool()) chainman.JumpOutOfIbd();

    // Reset, so that dangling pointers can be detected by sanitizers.
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

    ModelTransactions model;
    size_t expected_num_to_open{0};
    const auto TrackInitiated = [&](const CTransactionRef& tx) {
        const auto key{tx->GetWitnessHash().ToUint256()};
        if (model.contains(key)) return;
        model.emplace(key, ModelTransaction{
                              .tx = tx,
                              .time_added = NodeClock::now(),
                              .peers = {},
                          });
        expected_num_to_open += NUM_PRIVATE_BROADCAST_PER_TX;
    };

    // Seed with 0-3 transactions to test multiple pending broadcasts; zero exercises
    // the connected-in-vain disconnect in PushPrivateBroadcastTx().
    const int num_txs{fuzzed_data_provider.ConsumeIntegralInRange(0, 3)};
    std::vector<CTransactionRef> seeded_txs;
    for (int i = 0; i < num_txs; ++i) {
        auto tx{MakeTransactionRef(ConsumeTransaction(fuzzed_data_provider, /*prevout_txids=*/std::nullopt))};
        Assert(node.peerman->InitiateTxBroadcastPrivate(tx) == node::TransactionError::OK);
        TrackInitiated(tx);
        seeded_txs.push_back(tx);
    }

    LOCK(NetEventsInterface::g_msgproc_mutex);

    static NodeId node_id{0};
    // Create at least one PRIVATE_BROADCAST peer, optionally add others of random types.
    std::vector<CNode*> peers;
    std::set<NodeId> finalized;
    std::set<CNode*> detached_private_peers;

    const auto CheckOracle = [&] {
        const auto actual{node.peerman->GetPrivateBroadcastInfo()};
        Assert(actual.size() == model.size());
        for (const auto& info : actual) {
            const auto it{model.find(info.tx->GetWitnessHash().ToUint256())};
            Assert(it != model.end());
            const auto& expected{it->second};
            Assert(info.time_added == expected.time_added);
            Assert(info.peers.size() == expected.peers.size());
            for (size_t i{0}; i < info.peers.size(); ++i) {
                Assert(info.peers[i].address == expected.peers[i].address);
                Assert(info.peers[i].sent == expected.peers[i].sent);
                Assert(info.peers[i].received == expected.peers[i].received);
            }
        }
        Assert(connman.m_private_broadcast.NumToOpen() == expected_num_to_open);
    };

    CNode* pb_node = new CNode(
        /*id=*/node_id++,
        /*sock=*/std::make_shared<FuzzedSock>(fuzzed_data_provider, steady_clock),
        /*addrIn=*/ConsumeAddress(fuzzed_data_provider),
        /*nKeyedNetGroupIn=*/0,
        /*nLocalHostNonceIn=*/0,
        /*addrBindIn=*/CService{},
        /*addrNameIn=*/"",
        /*conn_type_in=*/ConnectionType::PRIVATE_BROADCAST,
        /*inbound_onion=*/false,
        /*network_key=*/0);

    peers.push_back(pb_node);
    connman.AddTestNode(*pb_node);
    // Capture outbound messages to verify if well formed (and to learn the PING
    // nonce), before SocketSendData drains vSendMsg.
    connman.SetCaptureMessages(true);
    const auto CaptureMessageOrig = CaptureMessage;
    const CAddress pb_addr = pb_node->addr;
    std::optional<uint64_t> pb_ping_nonce;
    bool pb_ping_outstanding{false};
    std::vector<uint256> captured_inv_hashes;
    std::optional<CTransactionRef> expected_tx_on_wire;
    CaptureMessage = [&](const CAddress& addr, const std::string& msg_type,
                         std::span<const unsigned char> data, bool is_incoming) {
        if (is_incoming || addr != pb_addr) return;
        if (msg_type == NetMsgType::PING) {
            Assert(data.size() == sizeof(uint64_t));
            uint64_t nonce;
            SpanReader{data} >> nonce;
            pb_ping_nonce = nonce;
            pb_ping_outstanding = true;
            return;
        }
        if (msg_type == NetMsgType::VERACK) {
            Assert(data.empty());
            return;
        }
        if (msg_type == NetMsgType::VERSION) {
            SpanReader ds{data};
            int32_t version;
            uint64_t my_services, your_services, my_services_dup, nonce;
            int64_t my_time;
            CService your_addr, my_addr;
            std::string user_agent;
            int32_t height;
            bool relay;
            ds >> version >> my_services >> my_time >>
                your_services >> CNetAddr::V1(your_addr) >>
                my_services_dup >> CNetAddr::V1(my_addr) >>
                nonce >> user_agent >> height >> relay;
            Assert(version == WTXID_RELAY_VERSION);
            Assert(my_services == NODE_NONE && my_services_dup == NODE_NONE);
            Assert(my_time == 0);
            Assert(your_services == NODE_NONE);
            Assert(your_addr == CService{});
            Assert(user_agent == "/pynode:0.0.1/");
            Assert(height == 0);
            Assert(!relay);
            return;
        }
        if (msg_type == NetMsgType::TX) {
            Assert(expected_tx_on_wire.has_value());
            DataStream expected_data;
            expected_data << TX_WITH_WITNESS(*expected_tx_on_wire);
            Assert(data.size() == expected_data.size());
            Assert(std::equal(data.begin(), data.end(), expected_data.begin(),
                              [](const unsigned char lhs, const std::byte rhs) {
                                  return lhs == std::to_integer<unsigned char>(rhs);
                              }));
            expected_tx_on_wire.reset();
            return;
        }
        Assert(msg_type == NetMsgType::INV);
        SpanReader ds{data};
        std::vector<CInv> invs;
        ds >> invs;
        Assert(invs.size() == 1);
        Assert(invs[0].IsMsgTx());
        captured_inv_hashes.push_back(invs[0].hash);
    };

    const auto TrackNewPrivatePick = [&](const CNode& peer) {
        const auto actual{node.peerman->GetPrivateBroadcastInfo()};
        ModelTransaction* added_tx{nullptr};
        const PrivateBroadcast::PeerSendInfo* added_peer{nullptr};
        for (const auto& info : actual) {
            auto it{model.find(info.tx->GetWitnessHash().ToUint256())};
            Assert(it != model.end());
            for (const auto& actual_peer : info.peers) {
                const bool known{std::ranges::any_of(it->second.peers, [&](const auto& expected_peer) {
                    return expected_peer.address == actual_peer.address &&
                           expected_peer.sent == actual_peer.sent &&
                           expected_peer.received == actual_peer.received;
                })};
                if (!known) {
                    Assert(added_tx == nullptr);
                    Assert(actual_peer.address == CService{peer.addr});
                    added_tx = &it->second;
                    added_peer = &actual_peer;
                }
            }
        }

        // FuzzedSock can make the synthetic handshake stop before VERACK;
        // that is an out-of-domain transport outcome for this state model.
        if (!peer.fSuccessfullyConnected) {
            Assert(peer.fDisconnect);
            Assert(added_tx == nullptr);
            return;
        }

        if (model.empty()) {
            Assert(added_tx == nullptr);
            Assert(peer.fDisconnect);
            return;
        }

        Assert(added_tx != nullptr);
        Assert(added_peer != nullptr);
        const auto chosen_priority{DerivePriority(*added_tx)};
        for (const auto& [_, expected_tx] : model) {
            Assert(AtLeastAsUrgent(chosen_priority, DerivePriority(expected_tx)));
        }
        added_tx->peers.push_back(ModelPeerSend{
            .nodeid = peer.GetId(),
            .address = added_peer->address,
            .sent = added_peer->sent,
            .received = added_peer->received,
        });
        Assert(captured_inv_hashes.size() == 1);
        Assert(captured_inv_hashes.back() == added_tx->tx->GetHash().ToUint256());
    };

    const auto FinalizePeer = [&](CNode& peer) {
        if (!finalized.insert(peer.GetId()).second) return;

        const bool is_private{peer.IsPrivateBroadcastConn()};
        const bool had_pending_transactions{!model.empty()};
        const ModelPeerSend* send{is_private ? FindPeerSend(model, peer.GetId()) : nullptr};
        const bool confirmed{send && send->received.has_value()};
        peer.fDisconnect = true;
        node.peerman->FinalizeNode(peer);
        if (is_private) {
            connman.RemoveTestPrivateBroadcastNode(peer);
            detached_private_peers.insert(&peer);
        }

        if (is_private && !confirmed) {
            for (auto& [_, tx] : model) {
                for (auto it{tx.peers.begin()}; it != tx.peers.end();) {
                    if (it->nodeid == peer.GetId() && !it->received.has_value()) {
                        ++tx.disconnected_unconfirmed_picks;
                        tx.last_disconnected_unconfirmed_pick = std::max(
                            tx.last_disconnected_unconfirmed_pick, it->sent);
                        it = tx.peers.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            Assert(FindPeerSend(model, peer.GetId()) == nullptr);
            if (had_pending_transactions) ++expected_num_to_open;
        }
        CheckOracle();
    };

    const auto RemoveModelById = [&](const uint256& id) {
        std::set<uint256> removed;
        for (auto it{model.begin()}; it != model.end();) {
            const auto& tx{it->second.tx};
            if (tx->GetHash().ToUint256() != id && tx->GetWitnessHash().ToUint256() != id) {
                ++it;
                continue;
            }

            size_t confirmed{0};
            for (const auto& peer : it->second.peers) confirmed += peer.received.has_value();
            const size_t connections_cancelled{
                NUM_PRIVATE_BROADCAST_PER_TX > confirmed ? NUM_PRIVATE_BROADCAST_PER_TX - confirmed : 0};
            expected_num_to_open = expected_num_to_open > connections_cancelled
                                       ? expected_num_to_open - connections_cancelled
                                       : 0;
            removed.insert(it->first);
            it = model.erase(it);
        }
        return removed;
    };

    const auto RemoveModelByWitness = [&](const uint256& wtxid) {
        std::set<uint256> removed;
        for (auto it{model.begin()}; it != model.end();) {
            if (it->first != wtxid) {
                ++it;
                continue;
            }

            size_t confirmed{0};
            for (const auto& peer : it->second.peers) confirmed += peer.received.has_value();
            const size_t connections_cancelled{
                NUM_PRIVATE_BROADCAST_PER_TX > confirmed ? NUM_PRIVATE_BROADCAST_PER_TX - confirmed : 0};
            expected_num_to_open = expected_num_to_open > connections_cancelled
                                       ? expected_num_to_open - connections_cancelled
                                       : 0;
            removed.insert(it->first);
            it = model.erase(it);
        }
        return removed;
    };

    // Complete handshake so PushPrivateBroadcastTx runs.
    connman.Handshake(
        /*node=*/*pb_node,
        /*successfully_connected=*/true,
        /*remote_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
        /*local_services=*/NODE_NONE,
        /*version=*/PROTOCOL_VERSION,
        /*relay_txs=*/true);
    TrackNewPrivatePick(*pb_node);
    CheckOracle();

    // Exercise the real VERSION/relay=false rejection and its subsequent
    // FinalizeNode cleanup. This path is distinct from malformed-message
    // handling: the peer is protocol-valid but cannot serve a private relay.
    if (fuzzed_data_provider.ConsumeBool()) {
        auto no_relay_peer{std::make_unique<CNode>(
            /*id=*/node_id++,
            /*sock=*/std::make_shared<FuzzedSock>(fuzzed_data_provider, steady_clock),
            /*addrIn=*/ConsumeAddress(fuzzed_data_provider),
            /*nKeyedNetGroupIn=*/0,
            /*nLocalHostNonceIn=*/0,
            /*addrBindIn=*/CService{},
            /*addrNameIn=*/"",
            /*conn_type_in=*/ConnectionType::PRIVATE_BROADCAST,
            /*inbound_onion=*/false,
            /*network_key=*/0)};
        if (no_relay_peer->addr != pb_addr) {
            peers.push_back(no_relay_peer.release());
            connman.AddTestNode(*peers.back());
            connman.Handshake(
                /*node=*/*peers.back(),
                /*successfully_connected=*/true,
                /*remote_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
                /*local_services=*/NODE_NONE,
                /*version=*/PROTOCOL_VERSION,
                /*relay_txs=*/false);
            Assert(peers.back()->fDisconnect);
            Assert(!peers.back()->fSuccessfullyConnected);
            FinalizePeer(*peers.back());
        }
    }

    // Optionally add extra peers of random connection types.
    const int extra_peers{fuzzed_data_provider.ConsumeIntegralInRange(0, 2)};
    for (int i = 0; i < extra_peers; ++i) {
        auto extra_peer{ConsumeNodeAsUniquePtr(fuzzed_data_provider, steady_clock, node_id++)};
        // An address collision would match the capture hook's filter and fail
        // its assertions on this peer's (legitimate) other-typed messages.
        if (extra_peer->addr == pb_addr) continue;
        peers.push_back(extra_peer.release());
        connman.AddTestNode(*peers.back());
        node.peerman->InitializeNode(
            *peers.back(),
            static_cast<ServiceFlags>(fuzzed_data_provider.ConsumeIntegral<uint64_t>()));
    }
    CheckOracle();

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
    {
        // Pick any random peer to test interleaved message handling.
        CNode& p2p_node = *PickValue(fuzzed_data_provider, peers);
        if (p2p_node.fDisconnect) continue;

        clock_ctx += ConsumeDuration<std::chrono::seconds>(fuzzed_data_provider, 0s, 600s);

        std::optional<CSerializedNetMsg> net_msg;
        bool accepted_pong{false};
        bool malformed_pong{false};
        bool invalid_getdata{false};
        bool received_from_network{false};
        CTransactionRef received_tx;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                net_msg.emplace();
                net_msg->m_type = std::string{PickValue(fuzzed_data_provider, INBOUND_MSG_TYPES)};
            },
            [&] {
                net_msg.emplace();
                net_msg->m_type = fuzzed_data_provider.ConsumeRandomLengthString(CMessageHeader::MESSAGE_TYPE_SIZE);
            },
            [&] {
                const auto tx{MakeTransactionRef(ConsumeTransaction(fuzzed_data_provider, /*prevout_txids=*/std::nullopt))};
                const auto result{node.peerman->InitiateTxBroadcastPrivate(tx)};
                Assert(result == node::TransactionError::OK);
                TrackInitiated(tx);
            },
            [&] {
                // Construct a valid GETDATA only for the transaction actually
                // assigned to this private peer. A merely seeded transaction
                // is not a valid request until PushPrivateBroadcastTx() has
                // recorded its INV.
                if (p2p_node.IsPrivateBroadcastConn() && p2p_node.fSuccessfullyConnected) {
                    const auto* model_tx{FindTransactionForNode(model, p2p_node.GetId())};
                    if (!model_tx) return;
                    const auto& tx{model_tx->tx};
                    expected_tx_on_wire = tx;
                    net_msg.emplace(NetMsg::Make(
                        NetMsgType::GETDATA,
                        std::vector<CInv>{{MSG_TX, tx->GetHash().ToUint256()}}));
                }
            },
            [&] {
                // Confirm reception of the pushed TX with a PONG matching the captured PING nonce.
                if (&p2p_node == pb_node && pb_ping_nonce && pb_ping_outstanding) {
                    net_msg.emplace(NetMsg::Make(NetMsgType::PONG, *pb_ping_nonce));
                }
            },
            [&] {
                // Echo a seeded tx back from a non-private-broadcast peer to exercise
                // the received-from-network broadcast-abort path.
                if (!p2p_node.IsPrivateBroadcastConn() &&
                    p2p_node.fSuccessfullyConnected &&
                    !seeded_txs.empty()) {
                    const auto& tx{PickValue(fuzzed_data_provider, seeded_txs)};
                    net_msg.emplace(NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*tx)));
                    received_tx = tx;
                    received_from_network = !chainman.IsInitialBlockDownload() &&
                                             !p2p_node.IsBlockOnlyConn() && !p2p_node.IsFeelerConn();
                }
            },
            [&] {
                // A private peer may ask for exactly one INVed transaction.
                // Two entries are guaranteed to take the production reject
                // branch without relying on a random hash collision.
                if (&p2p_node == pb_node && p2p_node.fSuccessfullyConnected) {
                    std::vector<CInv> invs;
                    if (const auto* model_tx{FindTransactionForNode(model, p2p_node.GetId())}) {
                        invs = {{MSG_TX, model_tx->tx->GetHash().ToUint256()},
                                {MSG_TX, model_tx->tx->GetHash().ToUint256()}};
                    } else {
                        invs = {{MSG_TX, ConsumeUInt256(fuzzed_data_provider)}};
                    }
                    net_msg.emplace(NetMsg::Make(NetMsgType::GETDATA, std::move(invs)));
                    invalid_getdata = true;
                }
            },
            [&] {
                if (model.empty()) return;
                const uint256 id{fuzzed_data_provider.ConsumeBool()
                                     ? (PickValue(fuzzed_data_provider, model).second.tx->GetHash().ToUint256())
                                     : ConsumeUInt256(fuzzed_data_provider)};
                const auto expected_removed{RemoveModelById(id)};
                const auto actual_removed{node.peerman->AbortPrivateBroadcast(id)};
                std::set<uint256> actual_ids;
                for (const auto& tx : actual_removed) actual_ids.insert(tx->GetWitnessHash().ToUint256());
                Assert(actual_ids == expected_removed);
                CheckOracle();
            },
            [&] {
                // Model the socket layer closing the private connection before
                // it has acknowledged the transaction. FinalizeNode() is the
                // production owner of the retry and cleanup transition.
                if (!finalized.contains(pb_node->GetId())) FinalizePeer(*pb_node);
            });

        if (net_msg) {
            if (net_msg->data.empty()) {
                net_msg->data = ConsumeRandomLengthByteVector(fuzzed_data_provider, MAX_PROTOCOL_MESSAGE_LENGTH);
            }
            if (&p2p_node == pb_node && net_msg->m_type == NetMsgType::PONG) {
                if (net_msg->data.size() < sizeof(uint64_t)) {
                    malformed_pong = true;
                } else {
                    uint64_t pong_nonce{0};
                    SpanReader{net_msg->data} >> pong_nonce;
                    accepted_pong = pb_ping_outstanding && pb_ping_nonce && pong_nonce == *pb_ping_nonce;
                    malformed_pong = pong_nonce == 0;
                }
            }
            ProcessInbound(connman, *node.peerman, p2p_node, std::move(*net_msg));

            if (expected_tx_on_wire) {
                // MakeAndPushMessage() captures a TX synchronously, so a
                // valid GETDATA cannot leave an unmatched expected payload.
                Assert(false);
            }
            if (accepted_pong) {
                if (auto* peer_send{FindPeerSend(model, pb_node->GetId())}) {
                    Assert(!peer_send->received.has_value());
                    peer_send->received = NodeClock::now();
                }
                pb_ping_outstanding = false;
                Assert(pb_node->fDisconnect);
            }
            if (malformed_pong) pb_ping_outstanding = false;
            if (received_from_network) {
                Assert(received_tx);
                const auto expected_removed{RemoveModelByWitness(received_tx->GetWitnessHash().ToUint256())};
                Assert(expected_removed.size() <= 1);
            }
            if (invalid_getdata) Assert(pb_node->fDisconnect);
            CheckOracle();
        } else {
            CheckOracle();
        }
    }

    // Always give a still-live private peer one deterministic request/PONG
    // round. Random actions decide whether it remains live, but a corpus
    // input should not need to guess the only sequence that reaches the
    // acknowledgement contract.
    if (!finalized.contains(pb_node->GetId()) && pb_node->fSuccessfullyConnected && !pb_node->fDisconnect) {
        if (const auto* model_tx{FindTransactionForNode(model, pb_node->GetId())}) {
            expected_tx_on_wire = model_tx->tx;
            ProcessInbound(connman, *node.peerman, *pb_node,
                           NetMsg::Make(NetMsgType::GETDATA,
                                        std::vector<CInv>{{MSG_TX, model_tx->tx->GetHash().ToUint256()}}));
            Assert(!expected_tx_on_wire.has_value());
            CheckOracle();

            if (pb_ping_nonce && pb_ping_outstanding && !pb_node->fDisconnect) {
                ProcessInbound(connman, *node.peerman, *pb_node,
                               NetMsg::Make(NetMsgType::PONG, *pb_ping_nonce));
                if (auto* peer_send{FindPeerSend(model, pb_node->GetId())}) {
                    Assert(!peer_send->received.has_value());
                    peer_send->received = NodeClock::now();
                }
                pb_ping_outstanding = false;
                Assert(pb_node->fDisconnect);
                CheckOracle();
            }
        }
    }

    CaptureMessage = CaptureMessageOrig;
    connman.SetCaptureMessages(false);

    // The real connection manager calls FinalizeNode() before deleting a
    // peer. Keep that lifecycle edge in the harness for private peers so
    // RPC-visible queue state and retry accounting are checked as well.
    for (CNode* peer : peers) {
        if (!finalized.contains(peer->GetId()) && peer->IsPrivateBroadcastConn()) FinalizePeer(*peer);
    }

    node.connman->StopNodes();
    for (CNode* peer : detached_private_peers) delete peer;
}
