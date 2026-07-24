// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <hash.h>
#include <net.h>
#include <netmessagemaker.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <protocol.h>
#include <random.h>
#include <serialize.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/asmap.h>
#include <util/chaintype.h>
#include <util/time.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

void AssertReceivedMessageContract(const CNetMessage& message)
{
    assert(!message.m_type.empty());
    assert(message.m_message_size == message.m_recv.size());
    assert(message.m_raw_message_size >= message.m_message_size);
}

void ExerciseMessageQueueContracts()
{
    CNode node{
        /*id=*/0,
        std::make_shared<ZeroSock>(),
        CAddress{},
        /*nKeyedNetGroupIn=*/0,
        /*nLocalHostNonceIn=*/0,
        CService{},
        "",
        ConnectionType::INBOUND,
        /*inbound_onion=*/false,
        /*network_key=*/0,
        CNodeOptions{.recv_flood_size = 1, .use_v2transport = false}};

    CSerializedNetMsg message{NetMsg::Make(NetMsgType::PING, uint64_t{0x123456789abcdef0})};
    assert(node.m_transport->SetMessageToSend(message));

    bool complete{false};
    while (true) {
        const auto& [bytes, _more, _type] = node.m_transport->GetBytesToSend(false);
        if (bytes.empty()) break;
        assert(node.ReceiveMsgBytes(bytes, complete));
        node.m_transport->MarkBytesSent(bytes.size());
    }
    assert(complete);

    node.MarkReceivedMsgsForProcessing();
    assert(node.fPauseRecv);
    const auto polled{node.PollMessage()};
    assert(polled);
    AssertReceivedMessageContract(polled->first);
    assert(!polled->second);
    assert(!node.fPauseRecv);
    assert(!node.PollMessage());

    CMessageHeader empty_type_header{Params().MessageStart(), "", 0};
    const uint256 empty_payload_hash{Hash(std::vector<uint8_t>{})};
    std::copy_n(empty_payload_hash.begin(), CMessageHeader::CHECKSUM_SIZE, empty_type_header.pchChecksum);
    std::vector<uint8_t> empty_type_bytes;
    VectorWriter{empty_type_bytes, 0, empty_type_header};
    bool empty_type_complete{false};
    assert(node.ReceiveMsgBytes(empty_type_bytes, empty_type_complete));
    assert(!empty_type_complete);
    node.MarkReceivedMsgsForProcessing();
    assert(!node.PollMessage());

    assert(node.GetRefCount() == 0);
    assert(node.AddRef() == &node);
    assert(node.GetRefCount() == 1);
    node.Release();
    assert(node.GetRefCount() == 0);

    node.CloseSocketDisconnect();
    assert(node.fDisconnect);
    {
        LOCK(node.m_sock_mutex);
        assert(!node.m_sock);
    }
    node.CloseSocketDisconnect();
}

} // namespace

void initialize_net()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::MAIN);
}

FUZZ_TARGET(net, .init = initialize_net)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    FakeSteadyClock steady_clock;
    ExerciseMessageQueueContracts();
    CNode node{ConsumeNode(fuzzed_data_provider, steady_clock)};
    node.SetCommonVersion(fuzzed_data_provider.ConsumeIntegral<int>());
    if (const auto service_opt =
            ConsumeDeserializable<CService>(fuzzed_data_provider, ConsumeDeserializationParams<CNetAddr::SerParams>(fuzzed_data_provider)))
    {
        node.SetAddrLocal(*service_opt);
    }
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                node.CloseSocketDisconnect();
            },
            [&] {
                CNodeStats stats;
                node.CopyStats(stats);
            },
            [&] {
                const CNode* add_ref_node = node.AddRef();
                assert(add_ref_node == &node);
            },
            [&] {
                if (node.GetRefCount() > 0) {
                    node.Release();
                }
            },
            [&] {
                const std::vector<uint8_t> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                bool complete;
                const bool stay_connected = node.ReceiveMsgBytes(b, complete);
                if (stay_connected && complete && fuzzed_data_provider.ConsumeBool()) {
                    node.MarkReceivedMsgsForProcessing();
                }
            },
            [&] {
                node.MarkReceivedMsgsForProcessing();
            },
            [&] {
                const auto message{node.PollMessage()};
                if (!message) return;
                AssertReceivedMessageContract(message->first);
                if (message->second) {
                    const auto next_message{node.PollMessage()};
                    assert(next_message);
                    AssertReceivedMessageContract(next_message->first);
                } else {
                    assert(!node.PollMessage());
                }
            });
    }

    node.MarkReceivedMsgsForProcessing();
    while (true) {
        const auto message{node.PollMessage()};
        if (!message) break;
        AssertReceivedMessageContract(message->first);
    }

    (void)node.GetAddrLocal();
    (void)node.GetId();
    (void)node.GetLocalNonce();
    const int ref_count = node.GetRefCount();
    assert(ref_count >= 0);
    (void)node.GetCommonVersion();

    const NetPermissionFlags net_permission_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
    (void)node.HasPermission(net_permission_flags);
    (void)node.ConnectedThroughNetwork();
}

FUZZ_TARGET(local_address, .init = initialize_net)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    FakeSteadyClock steady_clock;
    CService service{ConsumeService(fuzzed_data_provider)};
    CNode node{ConsumeNode(fuzzed_data_provider, steady_clock)};
    {
        LOCK(g_maplocalhost_mutex);
        mapLocalHost.clear();
    }
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                service = ConsumeService(fuzzed_data_provider);
            },
            [&] {
                const bool added{AddLocal(service, fuzzed_data_provider.ConsumeIntegralInRange<int>(0, LOCAL_MAX - 1))};
                if (!added) return;
                assert(service.IsRoutable());
                assert(IsLocal(service));
                assert(SeenLocal(service));
            },
            [&] {
                (void)RemoveLocal(service);
            },
            [&] {
                (void)SeenLocal(service);
            },
            [&] {
                (void)IsLocal(service);
            },
            [&] {
                (void)GetLocalAddress(node);
            });
    }
}
