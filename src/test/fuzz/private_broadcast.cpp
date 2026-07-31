// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <net.h>
#include <primitives/transaction.h>
#include <private_broadcast.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/overflow.h>
#include <util/time.h>

#include <algorithm>
#include <unordered_set>

struct CTransactionRefHash {
    size_t operator()(const CTransactionRef& tx) const
    {
        return static_cast<size_t>(tx->GetWitnessHash().ToUint256().GetUint64(0));
    }
};

struct CTransactionRefComp {
    bool operator()(const CTransactionRef& a, const CTransactionRef& b) const
    {
        return a->GetWitnessHash() == b->GetWitnessHash();
    }
};

struct ExpectedPeerSendInfo {
    NodeId nodeid;
    CService address;
    NodeClock::time_point sent;
    std::optional<NodeClock::time_point> received;
};

struct ExpectedTxState {
    NodeClock::time_point time_added;
    std::vector<ExpectedPeerSendInfo> peers;
};

FUZZ_TARGET(private_broadcast)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    FakeNodeClock clock_ctx{ConsumeTime(fdp)};

    PrivateBroadcast pb;

    // Random transaction that the test generated and passed to Add(). Trimmed when Remove() is called.
    // The values are the number of times a transaction was picked for sending.
    std::unordered_map<CTransactionRef, size_t, CTransactionRefHash, CTransactionRefComp> transactions;

    // Independent state model for exact output, priority, and stale-threshold checks.
    std::unordered_map<CTransactionRef, ExpectedTxState, CTransactionRefHash, CTransactionRefComp> expected;

    // Ids of nodes that were passed to PickTxForSend(). Trimmed when Remove() is called.
    std::unordered_set<NodeId> nodes_sent_to;

    // A subset of `nodes_sent_to`, node ids passed to NodeConfirmedReception(). Trimmed when Remove() is called.
    std::unordered_set<NodeId> nodes_that_confirmed_reception;

    NodeId next_nodeid{0}; // Generate unique node ids.

    const auto ExistentOrNewNodeId = [&next_nodeid, &fdp](){
        if (next_nodeid == 0 || fdp.ConsumeBool()) {
            return next_nodeid++;
        }
        return fdp.ConsumeIntegralInRange<NodeId>(0, next_nodeid - 1);
    };

    const auto FindExpectedPeer = [&expected](NodeId nodeid) {
        for (auto& [tx, state] : expected) {
            for (auto& peer : state.peers) {
                if (peer.nodeid == nodeid) return std::pair{tx, &peer};
            }
        }
        return std::pair<CTransactionRef, ExpectedPeerSendInfo*>{nullptr, nullptr};
    };

    const auto HigherPriority = [](const ExpectedTxState& a, const ExpectedTxState& b) {
        const auto CountConfirmed = [](const ExpectedTxState& state) {
            return std::ranges::count_if(state.peers, [](const auto& peer) { return peer.received.has_value(); });
        };
        const auto LastPicked = [](const ExpectedTxState& state) {
            if (state.peers.empty()) return NodeClock::time_point{};
            return std::ranges::max(state.peers, {}, [](const auto& peer) { return peer.sent; }).sent;
        };
        const auto LastConfirmed = [](const ExpectedTxState& state) {
            if (state.peers.empty()) return NodeClock::time_point{};
            return std::ranges::max(state.peers, {}, [](const auto& peer) { return peer.received.value_or(NodeClock::time_point{}); }).received.value_or(NodeClock::time_point{});
        };

        if (a.peers.size() != b.peers.size()) return a.peers.size() < b.peers.size();
        if (CountConfirmed(a) != CountConfirmed(b)) return CountConfirmed(a) < CountConfirmed(b);
        if (LastPicked(a) != LastPicked(b)) return LastPicked(a) < LastPicked(b);
        return LastConfirmed(a) < LastConfirmed(b);
    };

    LIMITED_WHILE(fdp.ConsumeBool(), 10000) {
        CallOneOf(
            fdp,
            [&] { // Add()
                CTransactionRef tx;
                bool from_transactions{false};
                if (transactions.empty() || fdp.ConsumeBool()) {
                    tx = MakeTransactionRef(ConsumeTransaction(fdp, std::nullopt));
                } else {
                    tx = PickIterator(fdp, transactions)->first;
                    from_transactions = true;
                }
                if (pb.Add(tx)) {
                    Assert(!from_transactions);
                    transactions.emplace(tx, 0);
                    const auto [_, inserted] = expected.emplace(tx, ExpectedTxState{NodeClock::now(), {}});
                    Assert(inserted);
                }
            },
            [&] { // Remove()
                if (transactions.empty()) {
                    return;
                }
                const auto transactions_it{PickIterator(fdp, transactions)};
                const CTransactionRef& tx{transactions_it->first};
                const auto expected_it{expected.find(tx)};
                Assert(expected_it != expected.end());

                size_t num_nodes_that_confirmed_tx{0};

                // Remove relevant entries from nodes_sent_to[] and nodes_that_confirmed_reception[] if any.
                for (auto it = nodes_sent_to.begin(); it != nodes_sent_to.end();) {
                    const NodeId nodeid{*it};
                    const auto opt_tx_for_node{pb.GetTxForNode(nodeid)};
                    if (opt_tx_for_node.has_value() && opt_tx_for_node.value() == tx) {
                        it = nodes_sent_to.erase(it);
                        if (nodes_that_confirmed_reception.erase(nodeid) > 0) {
                            ++num_nodes_that_confirmed_tx;
                        }
                    } else {
                        ++it;
                    }
                }

                const auto opt_num_confirmed{pb.Remove(tx)};

                Assert(opt_num_confirmed.has_value());
                Assert(opt_num_confirmed.value() == num_nodes_that_confirmed_tx);
                Assert(!pb.Remove(tx).has_value());
                transactions.erase(transactions_it);
                expected.erase(expected_it);
            },
            [&] { // PickTxForSend()
                // Only give pristine node ids to PickTxForSend() as required.
                const NodeId will_send_to_nodeid{next_nodeid++};
                const CService will_send_to_address{ConsumeService(fdp)};

                const auto opt_tx{pb.PickTxForSend(will_send_to_nodeid, will_send_to_address)};

                if (opt_tx.has_value()) {
                    Assert(transactions.contains(opt_tx.value()));

                    // "Number of times picked for sending" is the primary key in Priority's comparison
                    // (fewest sends = highest priority), so PickTxForSend() must return a transaction
                    // with the minimum send count of any in the queue. Ties are broken by state we
                    // don't model, so only check this key.
                    const size_t min_picked{std::ranges::min_element(
                        transactions, {}, [](const auto& el) { return el.second; })->second};
                    const auto picked_it{transactions.find(opt_tx.value())};
                    Assert(picked_it != transactions.end());
                    Assert(picked_it->second == min_picked); // picked the least-sent transaction
                    ++picked_it->second; // PickTxForSend() recorded exactly one send

                    const auto expected_it{expected.find(opt_tx.value())};
                    Assert(expected_it != expected.end());
                    for (const auto& [_, state] : expected) {
                        Assert(!HigherPriority(state, expected_it->second));
                    }
                    expected_it->second.peers.emplace_back(ExpectedPeerSendInfo{
                        .nodeid = will_send_to_nodeid,
                        .address = will_send_to_address,
                        .sent = NodeClock::now(),
                        .received = std::nullopt,
                    });

                    const auto& [_, inserted]{nodes_sent_to.emplace(will_send_to_nodeid)};
                    Assert(inserted);
                } else {
                    Assert(transactions.empty());
                }
            },
            [&] { // GetTxForNode()
                const NodeId nodeid{ExistentOrNewNodeId()};

                const auto opt_tx{pb.GetTxForNode(nodeid)};
                const auto [expected_tx, expected_peer]{FindExpectedPeer(nodeid)};

                if (nodes_sent_to.contains(nodeid)) {
                    Assert(opt_tx.has_value());
                    Assert(transactions.contains(opt_tx.value()));
                    Assert(expected_peer != nullptr);
                    Assert(opt_tx.value()->GetWitnessHash() == expected_tx->GetWitnessHash());
                } else {
                    Assert(!opt_tx.has_value());
                    Assert(expected_peer == nullptr);
                }
            },
            [&] { // NodeConfirmedReception()
                const NodeId nodeid{ExistentOrNewNodeId()};

                const auto [_, expected_peer]{FindExpectedPeer(nodeid)};
                pb.NodeConfirmedReception(nodeid);
                if (expected_peer != nullptr) expected_peer->received = NodeClock::now();

                if (nodes_sent_to.contains(nodeid)) {
                    // nodeid was previously passed to PickTxForSend(), so NodeConfirmedReception()
                    // must have changed the internal state. Remember this to later check that
                    // DidNodeConfirmReception() works correctly.
                    nodes_that_confirmed_reception.emplace(nodeid);
                }
            },
            [&] { // DidNodeConfirmReception()
                const NodeId nodeid{ExistentOrNewNodeId()};

                const bool confirmed{pb.DidNodeConfirmReception(nodeid)};
                const auto [_, expected_peer]{FindExpectedPeer(nodeid)};

                Assert(confirmed == (expected_peer != nullptr && expected_peer->received.has_value()));
            },
            [&] { // HavePendingTransactions()
                if (pb.HavePendingTransactions()) {
                    Assert(!transactions.empty());
                } else {
                    Assert(transactions.empty());
                }
            },
            [&] { // GetStale()
                const auto stale{pb.GetStale()};

                Assert(stale.size() <= transactions.size());

                const auto now{NodeClock::now()};
                const auto IsExpectedStale = [&now](const ExpectedTxState& state) {
                    const auto confirmed{std::ranges::count_if(state.peers, [](const auto& peer) { return peer.received.has_value(); })};
                    if (confirmed == 0) return state.time_added < now - PrivateBroadcast::INITIAL_STALE_DURATION;
                    const auto last_confirmed{std::ranges::max(state.peers, {}, [](const auto& peer) { return peer.received.value_or(NodeClock::time_point{}); }).received.value_or(NodeClock::time_point{})};
                    return last_confirmed < now - PrivateBroadcast::STALE_DURATION;
                };

                for (const auto& stale_tx : stale) {
                    Assert(transactions.contains(stale_tx));
                    Assert(IsExpectedStale(expected.at(stale_tx)));
                }
                for (const auto& [tx, state] : expected) {
                    const bool found{std::ranges::any_of(stale, [&](const auto& stale_tx) {
                        return stale_tx->GetWitnessHash() == tx->GetWitnessHash();
                    })};
                    Assert(found == IsExpectedStale(state));
                }
            },
            [&] { // GetBroadcastInfo()
                const auto all_broadcast_info{pb.GetBroadcastInfo()};

                Assert(all_broadcast_info.size() == transactions.size());

                for (const auto& info : all_broadcast_info) {
                    const auto it{expected.find(info.tx)};
                    Assert(it != expected.end());
                    Assert(info.time_added == it->second.time_added);
                    Assert(info.peers.size() == it->second.peers.size());
                    for (size_t i = 0; i < info.peers.size(); ++i) {
                        Assert(info.peers[i].address == it->second.peers[i].address);
                        Assert(info.peers[i].sent == it->second.peers[i].sent);
                        Assert(info.peers[i].received == it->second.peers[i].received);
                    }
                }
            },
            [&] {
                clock_ctx.set(ConsumeTime(fdp));
            });
        Assert(expected.size() == transactions.size());
    }
}
