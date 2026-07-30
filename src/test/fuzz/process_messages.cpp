// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <banman.h>
#include <common/bloom.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <kernel/chainparams.h>
#include <net.h>
#include <netmessagemaker.h>
#include <net_processing.h>
#include <node/mining_types.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/mining.h>
#include <test/util/net.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <test/util/validation.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
TestingSetup* g_setup;
std::vector<std::pair<COutPoint, CAmount>> g_mature_coinbases;

void ResetChainman(TestingSetup& setup)
{
    SetMockTime(setup.m_node.chainman->GetParams().GenesisBlock().Time());
    setup.m_node.chainman.reset();
    setup.m_make_chainman();
    setup.LoadVerifyActivateChainstate();
    node::BlockCreateOptions options;
    options.coinbase_output_script = CScript() << OP_TRUE;
    g_mature_coinbases.clear();
    for (int i = 0; i < 2 * COINBASE_MATURITY; i++) {
        const COutPoint coinbase{MineBlock(setup.m_node, options)};
        if (i < 3) {
            LOCK(cs_main);
            g_mature_coinbases.emplace_back(coinbase, setup.m_node.chainman->ActiveChainstate().CoinsTip().GetCoin(coinbase)->out.nValue);
        }
    }
}

void ResetMempool(TestingSetup& setup)
{
    bilingual_str error{};
    setup.m_node.mempool.reset();
    setup.m_node.mempool = std::make_unique<CTxMemPool>(MemPoolOptionsForTest(setup.m_node), error);
    Assert(error.empty());
}

CTransactionRef MakeRelayTransaction(const std::pair<COutPoint, CAmount>& coinbase, uint32_t locktime)
{
    CMutableTransaction tx;
    tx.nLockTime = locktime;
    CTxIn input;
    input.prevout = coinbase.first;
    input.scriptWitness.stack = {WITNESS_STACK_ELEM_OP_TRUE};
    tx.vin.push_back(input);
    tx.vout.emplace_back(coinbase.second - 1000, CScript() << OP_TRUE);
    return MakeTransactionRef(tx);
}

void AssertSendQueueMemoryUsage(CNode& node)
{
    LOCK(node.cs_vSend);
    node.AssertSendQueueMemoryUsage();
}

void AssertSpecialPeerAddressRelayDisabled(const PeerManager& peerman, const CNode& node)
{
    if (!node.IsBlockOnlyConn() && !node.IsFeelerConn()) return;

    CNodeStateStats stats;
    if (peerman.GetNodeStateStats(node.GetId(), stats)) {
        Assert(!stats.m_addr_relay_enabled);
    }
}
} // namespace

void initialize_process_messages()
{
    static const auto testing_setup{
        MakeNoLogFileContext<TestingSetup>(
            /*chain_type=*/ChainType::REGTEST,
            {}),
    };
    g_setup = testing_setup.get();
    ResetChainman(*g_setup);
}

FUZZ_TARGET(process_messages, .init = initialize_process_messages)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    ResetFuzzedSockMockedFds();
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FuzzedDataProvider rate_provider(buffer.data(), buffer.size());

    auto& node{g_setup->m_node};
    auto& connman{static_cast<ConnmanTestMsg&>(*node.connman)};
    connman.Reset();
    auto& chainman{static_cast<TestChainstateManager&>(*node.chainman)};
    const auto block_index_size{WITH_LOCK(chainman.GetMutex(), return chainman.BlockIndex().size())};
    FakeNodeClock clock{1610000000s}; // any time to successfully reset ibd
    FakeSteadyClock steady_clock;
    const unsigned int tx_send_rate{ConsumeTxSendRate(rate_provider, DEFAULT_TX_SEND_RATE)};
    chainman.ResetIbd();
    chainman.DisableNextWrite();

    // Reset, so that dangling pointers can be detected by sanitizers.
    node.banman.reset();
    node.addrman.reset();
    node.peerman.reset();
    ResetMempool(*g_setup);
    node.addrman = std::make_unique<AddrMan>(*node.netgroupman, /*deterministic=*/true, /*consistency_check_ratio=*/0);
    node.peerman = PeerManager::make(connman, *node.addrman,
                                     /*banman=*/nullptr, chainman,
                                     *node.mempool, *node.warnings,
                                     PeerManager::Options{
                                         .reconcile_txs = true,
                                         .deterministic_rng = true,
                                         .tx_send_rate = tx_send_rate,
                                     });
    connman.SetMsgProc(node.peerman.get());
    connman.SetAddrman(*node.addrman);

    node.validation_signals->RegisterValidationInterface(node.peerman.get());

    LOCK(NetEventsInterface::g_msgproc_mutex);

    std::vector<CNode*> peers;
    const auto num_peers_to_add = fuzzed_data_provider.ConsumeIntegralInRange(1, 3);
    for (int i = 0; i < num_peers_to_add; ++i) {
        peers.push_back(ConsumeNodeAsUniquePtr(fuzzed_data_provider, steady_clock, i).release());
        CNode& p2p_node = *peers.back();

        FillNode(fuzzed_data_provider, connman, p2p_node);

        connman.AddTestNode(p2p_node);
    }

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 30) {
        const std::string random_message_type{fuzzed_data_provider.ConsumeBytesAsString(CMessageHeader::MESSAGE_TYPE_SIZE).c_str()};

        clock.set(ConsumeTime(fuzzed_data_provider));

        CSerializedNetMsg net_msg;
        net_msg.m_type = random_message_type;
        net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider, MAX_PROTOCOL_MESSAGE_LENGTH);

        CNode& random_node = *PickValue(fuzzed_data_provider, peers);

        connman.FlushSendBuffer(random_node);
        AssertSendQueueMemoryUsage(random_node);
        (void)connman.ReceiveMsgFrom(random_node, std::move(net_msg));
        AssertSendQueueMemoryUsage(random_node);

        bool more_work{true};
        while (more_work) { // Ensure that every message is eventually processed in some way or another
            random_node.fPauseSend = false;

            more_work = connman.ProcessMessagesOnce(random_node);
            node.peerman->SendMessages(random_node);
            AssertSendQueueMemoryUsage(random_node);
        }
    }

    for (CNode* peer : peers) {
        AssertSendQueueMemoryUsage(*peer);
        AssertSpecialPeerAddressRelayDisabled(*node.peerman, *peer);
    }
    node.validation_signals->SyncWithValidationInterfaceQueue();
    node.connman->StopNodes();
    node.validation_signals->SyncWithValidationInterfaceQueue();
    node.validation_signals->UnregisterValidationInterface(node.peerman.get());
    node.peerman.reset();

    // Start the guided relay slice with no random peers. The global relay buckets deliberately
    // account for every eligible peer, so retaining the random peers would make the known-filter
    // oracle depend on their fuzzed handshake state.
    node.addrman.reset();
    node.addrman = std::make_unique<AddrMan>(*node.netgroupman, /*deterministic=*/true, /*consistency_check_ratio=*/0);
    node.peerman = PeerManager::make(connman, *node.addrman,
                                     /*banman=*/nullptr, chainman,
                                     *node.mempool, *node.warnings,
                                     PeerManager::Options{
                                         .reconcile_txs = true,
                                         .deterministic_rng = true,
                                         .tx_send_rate = tx_send_rate,
                                     });
    connman.SetMsgProc(node.peerman.get());
    connman.SetAddrman(*node.addrman);
    node.validation_signals->RegisterValidationInterface(node.peerman.get());

    // A local transaction with no eligible peers must not consume either global
    // relay bucket or leave an undeliverable backlog behind.
    TestMemPoolEntryHelper no_peer_entry;
    const CTransactionRef no_peer_tx{MakeRelayTransaction(g_mature_coinbases[0], /*locktime=*/0)};
    TryAddToMempool(*node.mempool, no_peer_entry.Fee(1000).SpendsCoinbase(true).FromTx(no_peer_tx));
    {
        LOCK(node.mempool->cs);
        Assert(node.mempool->GetIter(no_peer_tx->GetWitnessHash()).has_value());
    }
    const PeerManagerInfo before_no_peer{node.peerman->GetInfo()};
    node.peerman->InitiateTxBroadcastToAll(no_peer_tx->GetWitnessHash());
    const PeerManagerInfo after_no_peer{node.peerman->GetInfo()};
    Assert(after_no_peer.inbound_bucket.backlog_count == before_no_peer.inbound_bucket.backlog_count);
    Assert(after_no_peer.outbound_bucket.backlog_count == before_no_peer.outbound_bucket.backlog_count);
    Assert(after_no_peer.inbound_bucket.count_bucket == before_no_peer.inbound_bucket.count_bucket);
    Assert(after_no_peer.outbound_bucket.count_bucket == before_no_peer.outbound_bucket.count_bucket);
    Assert(after_no_peer.inbound_bucket.size_bucket == before_no_peer.inbound_bucket.size_bucket);
    Assert(after_no_peer.outbound_bucket.size_bucket == before_no_peer.outbound_bucket.size_bucket);
    {
        LOCK(node.mempool->cs);
        node.mempool->removeRecursive(*no_peer_tx, MemPoolRemovalReason::EXPIRY);
        Assert(!node.mempool->GetIter(no_peer_tx->GetWitnessHash()).has_value());
    }

    // The random-message loop configures tx_send_rate, but normally never creates a valid
    // mempool transaction. Exercise the global relay buckets with deterministic peers and
    // witness-shaped transactions so that known-filter, duplicate, and stale-entry paths are
    // reached independently of wire-message luck.
    auto make_relay_peer = [&](NodeId id, ConnectionType connection_type, bool relay_txs = true) NO_THREAD_SAFETY_ANALYSIS {
        auto peer = std::make_unique<CNode>(
            id, std::make_shared<ZeroSock>(), CAddress{}, /*nKeyedNetGroupIn=*/0,
            /*nLocalHostNonceIn=*/0, CService{}, /*addrNameIn=*/"", connection_type,
            /*inbound_onion=*/false, /*network_key=*/static_cast<uint64_t>(id),
            CNodeOptions{.permission_flags = NetPermissionFlags::NoBan});
        CNode* result{peer.release()};
        connman.AddTestNode(*result);
        connman.Handshake(
            *result,
            /*successfully_connected=*/false,
            /*remote_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
            /*local_services=*/relay_txs ? ServiceFlags(NODE_NETWORK | NODE_WITNESS) :
                                           ServiceFlags(NODE_NETWORK | NODE_WITNESS | NODE_BLOOM),
            /*version=*/PROTOCOL_VERSION,
            /*relay_txs=*/relay_txs);
        return result;
    };

    CNode& non_relay_peer{*make_relay_peer(/*id=*/103, ConnectionType::INBOUND, /*relay_txs=*/false)};

    auto process_relay_message = [&](CNode& peer, CSerializedNetMsg&& net_msg) NO_THREAD_SAFETY_ANALYSIS {
        connman.FlushSendBuffer(peer);
        Assert(connman.ReceiveMsgFrom(peer, std::move(net_msg)));
        bool more_work{true};
        while (more_work) {
            peer.fPauseSend = false;
            more_work = connman.ProcessMessagesOnce(peer);
            node.peerman->SendMessages(peer);
        }
    };

    process_relay_message(non_relay_peer, NetMsg::Make(NetMsgType::VERACK));

    auto peer_inv_to_send = [&](CNode& peer) {
        CNodeStateStats stats;
        Assert(node.peerman->GetNodeStateStats(peer.GetId(), stats));
        return stats.m_inv_to_send;
    };

    // A handshake-complete peer that negotiated relay=0 is not an eligible
    // recipient, even when NODE_BLOOM caused a TxRelay object to be created.
    TestMemPoolEntryHelper non_relay_entry;
    const CTransactionRef non_relay_tx{MakeRelayTransaction(g_mature_coinbases[0], /*locktime=*/1)};
    TryAddToMempool(*node.mempool, non_relay_entry.Fee(1000).SpendsCoinbase(true).FromTx(non_relay_tx));
    const PeerManagerInfo before_non_relay{node.peerman->GetInfo()};
    node.peerman->InitiateTxBroadcastToAll(non_relay_tx->GetWitnessHash());
    const PeerManagerInfo after_non_relay{node.peerman->GetInfo()};
    Assert(after_non_relay.inbound_bucket.backlog_count == before_non_relay.inbound_bucket.backlog_count);
    Assert(after_non_relay.outbound_bucket.backlog_count == before_non_relay.outbound_bucket.backlog_count);
    Assert(after_non_relay.inbound_bucket.count_bucket == before_non_relay.inbound_bucket.count_bucket);
    Assert(after_non_relay.outbound_bucket.count_bucket == before_non_relay.outbound_bucket.count_bucket);
    Assert(after_non_relay.inbound_bucket.size_bucket == before_non_relay.inbound_bucket.size_bucket);
    Assert(after_non_relay.outbound_bucket.size_bucket == before_non_relay.outbound_bucket.size_bucket);
    Assert(peer_inv_to_send(non_relay_peer) == 0);
    {
        LOCK(node.mempool->cs);
        node.mempool->removeRecursive(*non_relay_tx, MemPoolRemovalReason::EXPIRY);
        Assert(!node.mempool->GetIter(non_relay_tx->GetWitnessHash()).has_value());
    }

    // A relay-enabled BIP37 peer with a nonmatching filter must not consume a
    // global relay reservation when SendMessages() will suppress the inventory.
    CBloomFilter nonmatching_filter{/*nElements=*/10, /*nFPRate=*/0.000001, /*nTweakIn=*/0, BLOOM_UPDATE_NONE};
    const CTransactionRef filtered_tx{MakeRelayTransaction(g_mature_coinbases[0], /*locktime=*/2)};
    TestMemPoolEntryHelper filtered_entry;
    TryAddToMempool(*node.mempool, filtered_entry.Fee(1000).SpendsCoinbase(true).FromTx(filtered_tx));
    Assert(!nonmatching_filter.IsRelevantAndUpdate(*filtered_tx));
    process_relay_message(non_relay_peer, NetMsg::Make(NetMsgType::FILTERLOAD, nonmatching_filter));
    const PeerManagerInfo before_filtered{node.peerman->GetInfo()};
    node.peerman->InitiateTxBroadcastToAll(filtered_tx->GetWitnessHash());
    const PeerManagerInfo after_filtered{node.peerman->GetInfo()};
    Assert(after_filtered.inbound_bucket.backlog_count == before_filtered.inbound_bucket.backlog_count);
    Assert(after_filtered.outbound_bucket.backlog_count == before_filtered.outbound_bucket.backlog_count);
    Assert(after_filtered.inbound_bucket.count_bucket == before_filtered.inbound_bucket.count_bucket);
    Assert(after_filtered.outbound_bucket.count_bucket == before_filtered.outbound_bucket.count_bucket);
    Assert(after_filtered.inbound_bucket.size_bucket == before_filtered.inbound_bucket.size_bucket);
    Assert(after_filtered.outbound_bucket.size_bucket == before_filtered.outbound_bucket.size_bucket);
    Assert(peer_inv_to_send(non_relay_peer) == 0);
    {
        LOCK(node.mempool->cs);
        node.mempool->removeRecursive(*filtered_tx, MemPoolRemovalReason::EXPIRY);
        Assert(!node.mempool->GetIter(filtered_tx->GetWitnessHash()).has_value());
    }

    // A relay-enabled peer whose fee filter rejects a transaction must not consume a
    // global relay reservation when SendMessages() will suppress the inventory.
    process_relay_message(non_relay_peer, NetMsg::Make(NetMsgType::FILTERCLEAR));
    process_relay_message(non_relay_peer, NetMsg::Make(NetMsgType::FEEFILTER, CAmount{100'000}));
    const CTransactionRef fee_filtered_tx{MakeRelayTransaction(g_mature_coinbases[0], /*locktime=*/3)};
    TestMemPoolEntryHelper fee_filtered_entry;
    TryAddToMempool(*node.mempool, fee_filtered_entry.Fee(1000).SpendsCoinbase(true).FromTx(fee_filtered_tx));
    const PeerManagerInfo before_fee_filtered{node.peerman->GetInfo()};
    node.peerman->InitiateTxBroadcastToAll(fee_filtered_tx->GetWitnessHash());
    const PeerManagerInfo after_fee_filtered{node.peerman->GetInfo()};
    Assert(after_fee_filtered.inbound_bucket.backlog_count == before_fee_filtered.inbound_bucket.backlog_count);
    Assert(after_fee_filtered.outbound_bucket.backlog_count == before_fee_filtered.outbound_bucket.backlog_count);
    Assert(after_fee_filtered.inbound_bucket.count_bucket == before_fee_filtered.inbound_bucket.count_bucket);
    Assert(after_fee_filtered.outbound_bucket.count_bucket == before_fee_filtered.outbound_bucket.count_bucket);
    Assert(after_fee_filtered.inbound_bucket.size_bucket == before_fee_filtered.inbound_bucket.size_bucket);
    Assert(after_fee_filtered.outbound_bucket.size_bucket == before_fee_filtered.outbound_bucket.size_bucket);
    Assert(peer_inv_to_send(non_relay_peer) == 0);
    {
        LOCK(node.mempool->cs);
        node.mempool->removeRecursive(*fee_filtered_tx, MemPoolRemovalReason::EXPIRY);
        Assert(!node.mempool->GetIter(fee_filtered_tx->GetWitnessHash()).has_value());
    }

    CNode& inbound_relay_peer{*make_relay_peer(/*id=*/100, ConnectionType::INBOUND)};
    CNode& outbound_relay_peer{*make_relay_peer(/*id=*/101, ConnectionType::OUTBOUND_FULL_RELAY)};
    CNode& legacy_relay_peer{*make_relay_peer(/*id=*/102, ConnectionType::OUTBOUND_FULL_RELAY)};

    // Complete BIP339 negotiation before VERACK for two peers. ConnmanTestMsg::Handshake
    // intentionally drops the feature message from the synthetic wire, while this target needs
    // both wtxid and legacy txid relay paths.
    process_relay_message(inbound_relay_peer, NetMsg::Make(NetMsgType::WTXIDRELAY));
    process_relay_message(outbound_relay_peer, NetMsg::Make(NetMsgType::WTXIDRELAY));
    process_relay_message(inbound_relay_peer, NetMsg::Make(NetMsgType::VERACK));
    process_relay_message(outbound_relay_peer, NetMsg::Make(NetMsgType::VERACK));
    process_relay_message(legacy_relay_peer, NetMsg::Make(NetMsgType::VERACK));
    Assert(inbound_relay_peer.fSuccessfullyConnected);
    Assert(outbound_relay_peer.fSuccessfullyConnected);
    Assert(legacy_relay_peer.fSuccessfullyConnected);

    Assert(g_mature_coinbases.size() >= 3);
    auto add_relay_tx = [&](const CTransactionRef& tx) {
        TestMemPoolEntryHelper entry;
        TryAddToMempool(*node.mempool, entry.Fee(1000).SpendsCoinbase(true).FromTx(tx));
        LOCK(node.mempool->cs);
        Assert(node.mempool->GetIter(tx->GetWitnessHash()).has_value());
    };
    auto send_relay_inventory = [&](CNode& peer) NO_THREAD_SAFETY_ANALYSIS {
        node.peerman->SendMessages(peer);
        connman.FlushSendBuffer(peer);
    };

    const CTransactionRef mixed_tx{MakeRelayTransaction(g_mature_coinbases[0], /*locktime=*/0)};
    const CTransactionRef all_known_tx{MakeRelayTransaction(g_mature_coinbases[1], /*locktime=*/0)};
    const CTransactionRef stale_tx{MakeRelayTransaction(g_mature_coinbases[2], /*locktime=*/0)};
    add_relay_tx(mixed_tx);
    add_relay_tx(all_known_tx);
    add_relay_tx(stale_tx);

    // First queue a transaction for all three peers. Sending it to the inbound peer records the
    // hash only in that peer's known filter, so the next broadcast must refund the inbound bucket
    // while consuming one more outbound token. The third peer exercises the legacy txid filter.
    const PeerManagerInfo before_mixed{node.peerman->GetInfo()};
    node.peerman->InitiateTxBroadcastToAll(mixed_tx->GetWitnessHash());
    const PeerManagerInfo after_mixed{node.peerman->GetInfo()};
    Assert(after_mixed.inbound_bucket.count_bucket == before_mixed.inbound_bucket.count_bucket - 1);
    Assert(after_mixed.outbound_bucket.count_bucket == before_mixed.outbound_bucket.count_bucket - 1);
    Assert(peer_inv_to_send(inbound_relay_peer) == 1);
    Assert(peer_inv_to_send(outbound_relay_peer) == 1);
    Assert(peer_inv_to_send(legacy_relay_peer) == 1);
    send_relay_inventory(inbound_relay_peer);
    Assert(peer_inv_to_send(inbound_relay_peer) == 0);

    const PeerManagerInfo before_mixed_retry{node.peerman->GetInfo()};
    node.peerman->InitiateTxBroadcastToAll(mixed_tx->GetWitnessHash());
    const PeerManagerInfo after_mixed_retry{node.peerman->GetInfo()};
    Assert(after_mixed_retry.inbound_bucket.count_bucket == before_mixed_retry.inbound_bucket.count_bucket);
    Assert(after_mixed_retry.outbound_bucket.count_bucket == before_mixed_retry.outbound_bucket.count_bucket - 1);
    Assert(peer_inv_to_send(inbound_relay_peer) == 0);
    Assert(peer_inv_to_send(outbound_relay_peer) == 2);
    Assert(peer_inv_to_send(legacy_relay_peer) == 2);

    // Queue a second transaction for all peers, send it to all, and then rebroadcast it. Every
    // filter now knows the transaction, so neither bucket may spend another token. Repeating the
    // call exercises duplicate wtxids in consecutive global backlogs.
    const PeerManagerInfo before_all_unknown{node.peerman->GetInfo()};
    node.peerman->InitiateTxBroadcastToAll(all_known_tx->GetWitnessHash());
    const PeerManagerInfo after_all_unknown{node.peerman->GetInfo()};
    Assert(after_all_unknown.inbound_bucket.count_bucket == before_all_unknown.inbound_bucket.count_bucket - 1);
    Assert(after_all_unknown.outbound_bucket.count_bucket == before_all_unknown.outbound_bucket.count_bucket - 1);
    send_relay_inventory(inbound_relay_peer);
    send_relay_inventory(outbound_relay_peer);
    send_relay_inventory(legacy_relay_peer);
    Assert(peer_inv_to_send(inbound_relay_peer) == 0);
    Assert(peer_inv_to_send(outbound_relay_peer) == 0);
    Assert(peer_inv_to_send(legacy_relay_peer) == 0);

    const PeerManagerInfo before_all_known{node.peerman->GetInfo()};
    node.peerman->InitiateTxBroadcastToAll(all_known_tx->GetWitnessHash());
    const PeerManagerInfo after_all_known{node.peerman->GetInfo()};
    Assert(after_all_known.inbound_bucket.count_bucket == before_all_known.inbound_bucket.count_bucket);
    Assert(after_all_known.outbound_bucket.count_bucket == before_all_known.outbound_bucket.count_bucket);
    Assert(peer_inv_to_send(inbound_relay_peer) == 0);
    Assert(peer_inv_to_send(outbound_relay_peer) == 0);
    Assert(peer_inv_to_send(legacy_relay_peer) == 0);
    node.peerman->InitiateTxBroadcastToAll(all_known_tx->GetWitnessHash());
    const PeerManagerInfo after_duplicate{node.peerman->GetInfo()};
    Assert(after_duplicate.inbound_bucket.count_bucket == after_all_known.inbound_bucket.count_bucket);
    Assert(after_duplicate.outbound_bucket.count_bucket == after_all_known.outbound_bucket.count_bucket);

    // A transaction selected for relay can be evicted before SendMessages() extracts it from the
    // mempool. The queued entry must be dropped without leaving stale per-peer state behind. The
    // reservation is intentionally retained: it paid for the relay attempt at selection time.
    const PeerManagerInfo before_stale{node.peerman->GetInfo()};
    node.peerman->InitiateTxBroadcastToAll(stale_tx->GetWitnessHash());
    const PeerManagerInfo after_stale_queue{node.peerman->GetInfo()};
    Assert(after_stale_queue.inbound_bucket.backlog_count == 0);
    Assert(after_stale_queue.outbound_bucket.backlog_count == 0);
    Assert(after_stale_queue.inbound_bucket.count_bucket == before_stale.inbound_bucket.count_bucket - 1);
    Assert(after_stale_queue.outbound_bucket.count_bucket == before_stale.outbound_bucket.count_bucket - 1);
    Assert(peer_inv_to_send(inbound_relay_peer) == 1);
    Assert(peer_inv_to_send(outbound_relay_peer) == 1);
    Assert(peer_inv_to_send(legacy_relay_peer) == 1);

    {
        LOCK(node.mempool->cs);
        node.mempool->removeRecursive(*stale_tx, MemPoolRemovalReason::EXPIRY);
        Assert(!node.mempool->GetIter(stale_tx->GetWitnessHash()).has_value());
    }
    send_relay_inventory(inbound_relay_peer);
    send_relay_inventory(outbound_relay_peer);
    send_relay_inventory(legacy_relay_peer);
    const PeerManagerInfo after_stale{node.peerman->GetInfo()};
    Assert(after_stale.inbound_bucket.backlog_count == 0);
    Assert(after_stale.outbound_bucket.backlog_count == 0);
    Assert(after_stale.inbound_bucket.count_bucket == after_stale_queue.inbound_bucket.count_bucket);
    Assert(after_stale.outbound_bucket.count_bucket == after_stale_queue.outbound_bucket.count_bucket);
    Assert(peer_inv_to_send(inbound_relay_peer) == 0);
    Assert(peer_inv_to_send(outbound_relay_peer) == 0);
    Assert(peer_inv_to_send(legacy_relay_peer) == 0);

    AssertSendQueueMemoryUsage(inbound_relay_peer);
    AssertSendQueueMemoryUsage(outbound_relay_peer);
    AssertSendQueueMemoryUsage(legacy_relay_peer);
    {
        LOCK(node.mempool->cs);
        node.mempool->removeRecursive(*mixed_tx, MemPoolRemovalReason::REPLACED);
        node.mempool->removeRecursive(*all_known_tx, MemPoolRemovalReason::REPLACED);
        Assert(!node.mempool->GetIter(mixed_tx->GetWitnessHash()).has_value());
        Assert(!node.mempool->GetIter(all_known_tx->GetWitnessHash()).has_value());
    }
    node.validation_signals->SyncWithValidationInterfaceQueue();
    node.validation_signals->UnregisterValidationInterface(node.peerman.get());
    node.connman->StopNodes();
    if (block_index_size != WITH_LOCK(chainman.GetMutex(), return chainman.BlockIndex().size())) {
        // Reuse the global chainman, but reset it when it is dirty
        ResetChainman(*g_setup);
    }
}
