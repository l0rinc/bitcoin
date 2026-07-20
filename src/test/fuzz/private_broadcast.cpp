// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <private_broadcast.h>

#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <net.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/overflow.h>
#include <util/time.h>

#include <algorithm>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

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

struct SendState {
    NodeId nodeid;
    CService address;
    NodeClock::time_point picked;
    std::optional<NodeClock::time_point> confirmed;
};

struct TransactionState {
    NodeClock::time_point time_added;
    size_t disconnected_unconfirmed_picks{0};
    NodeClock::time_point last_disconnected_unconfirmed_pick{};
    std::vector<SendState> sends;
};

struct PriorityState {
    size_t num_picked{0};
    NodeClock::time_point last_picked{};
    size_t num_confirmed{0};
    NodeClock::time_point last_confirmed{};
};

PriorityState DerivePriority(const TransactionState& state)
{
    PriorityState priority;
    priority.num_picked = state.disconnected_unconfirmed_picks;
    priority.last_picked = state.last_disconnected_unconfirmed_pick;
    for (const auto& send : state.sends) {
        ++priority.num_picked;
        priority.last_picked = std::max(priority.last_picked, send.picked);
        if (send.confirmed.has_value()) {
            ++priority.num_confirmed;
            priority.last_confirmed = std::max(priority.last_confirmed, *send.confirmed);
        }
    }
    return priority;
}

// Smaller values are selected by PrivateBroadcast's max-element priority
// ordering, so the chosen state must be lexicographically no greater than
// every state still in the queue.
bool AtLeastAsUrgent(const PriorityState& lhs, const PriorityState& rhs)
{
    return std::tie(lhs.num_picked, lhs.num_confirmed, lhs.last_picked, lhs.last_confirmed) <=
           std::tie(rhs.num_picked, rhs.num_confirmed, rhs.last_picked, rhs.last_confirmed);
}

FUZZ_TARGET(private_broadcast)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    FakeNodeClock clock_ctx{ConsumeTime(fdp)};

    const size_t cap{fdp.ConsumeIntegralInRange<size_t>(1, 12)};
    PrivateBroadcast pb{cap};

    // Model every field that affects priority, stale selection, RPC state, or
    // the disconnect cleanup path. The production map is intentionally not
    // exposed, so these are checked through its public operations.
    std::unordered_map<CTransactionRef, TransactionState, CTransactionRefHash, CTransactionRefComp> transactions;

    NodeId next_nodeid{0}; // Generate unique node ids.

    const auto ExistentOrNewNodeId = [&next_nodeid, &fdp]() {
        if (next_nodeid == 0 || fdp.ConsumeBool()) {
            return next_nodeid++;
        }
        return fdp.ConsumeIntegralInRange<NodeId>(0, next_nodeid - 1);
    };

    const auto FindSend = [&transactions](NodeId nodeid) -> SendState* {
        for (auto& [_, state] : transactions) {
            for (auto& send : state.sends) {
                if (send.nodeid == nodeid) return &send;
            }
        }
        return nullptr;
    };

    LIMITED_WHILE (fdp.ConsumeBool(), 10000) {
        CallOneOf(
            fdp,
            [&] { // Add()
                CTransactionRef tx;
                if (transactions.empty() || fdp.ConsumeBool()) {
                    tx = MakeTransactionRef(ConsumeTransaction(fdp, std::nullopt));
                } else {
                    tx = PickIterator(fdp, transactions)->first;
                }

                const bool present_before{transactions.contains(tx)};
                const auto res{pb.Add(tx)};
                if (present_before) {
                    Assert(res == PrivateBroadcast::AddResult::AlreadyPresent);
                } else if (transactions.size() >= cap) {
                    Assert(res == PrivateBroadcast::AddResult::QueueFull);
                } else {
                    Assert(res == PrivateBroadcast::AddResult::Added);
                    transactions.emplace(tx, TransactionState{
                                                 .time_added = NodeClock::now(),
                                                 .sends = {},
                                             });
                }
            },
            [&] { // Remove()
                if (transactions.empty()) return;

                const auto transactions_it{PickIterator(fdp, transactions)};
                const CTransactionRef& tx{transactions_it->first};
                size_t num_confirmed{0};
                for (const auto& send : transactions_it->second.sends) {
                    num_confirmed += send.confirmed.has_value();
                }

                const auto opt_num_confirmed{pb.Remove(tx)};
                Assert(opt_num_confirmed.has_value());
                Assert(opt_num_confirmed.value() == num_confirmed);
                Assert(!pb.Remove(tx).has_value());
                transactions.erase(transactions_it);
            },
            [&] { // PickTxForSend()
                // Only give pristine node ids to PickTxForSend() as required.
                const NodeId will_send_to_nodeid{next_nodeid++};
                const CService will_send_to_address{ConsumeService(fdp)};
                const auto picked{NodeClock::now()};

                const auto opt_tx{pb.PickTxForSend(will_send_to_nodeid, will_send_to_address)};

                if (opt_tx.has_value()) {
                    const auto picked_it{transactions.find(opt_tx.value())};
                    Assert(picked_it != transactions.end());

                    const auto chosen_priority{DerivePriority(picked_it->second)};
                    for (const auto& [_, state] : transactions) {
                        Assert(AtLeastAsUrgent(chosen_priority, DerivePriority(state)));
                    }
                    picked_it->second.sends.push_back(SendState{
                        .nodeid = will_send_to_nodeid,
                        .address = will_send_to_address,
                        .picked = picked,
                        .confirmed = std::nullopt,
                    });
                } else {
                    Assert(transactions.empty());
                }
            },
            [&] { // GetTxForNode()
                const NodeId nodeid{ExistentOrNewNodeId()};
                const auto opt_tx{pb.GetTxForNode(nodeid)};
                const auto* send{FindSend(nodeid)};

                if (send != nullptr) {
                    Assert(opt_tx.has_value());
                    Assert(transactions.contains(opt_tx.value()));
                } else {
                    Assert(!opt_tx.has_value());
                }
            },
            [&] { // NodeConfirmedReception()
                const NodeId nodeid{ExistentOrNewNodeId()};
                pb.NodeConfirmedReception(nodeid);
                if (auto* send{FindSend(nodeid)}) send->confirmed = NodeClock::now();
            },
            [&] { // DidNodeConfirmReception()
                const NodeId nodeid{ExistentOrNewNodeId()};
                const bool confirmed{pb.DidNodeConfirmReception(nodeid)};
                const auto* send{FindSend(nodeid)};
                Assert(confirmed == (send != nullptr && send->confirmed.has_value()));
            },
            [&] { // RemoveUnconfirmedNode()
                const NodeId nodeid{ExistentOrNewNodeId()};
                bool removed{false};
                for (auto& [_, state] : transactions) {
                    for (auto it = state.sends.begin(); it != state.sends.end();) {
                        if (it->nodeid == nodeid && !it->confirmed.has_value()) {
                            ++state.disconnected_unconfirmed_picks;
                            state.last_disconnected_unconfirmed_pick = std::max(
                                state.last_disconnected_unconfirmed_pick, it->picked);
                            it = state.sends.erase(it);
                            removed = true;
                        } else {
                            ++it;
                        }
                    }
                }
                Assert(pb.RemoveUnconfirmedNode(nodeid) == removed);
            },
            [&] { // HavePendingTransactions()
                Assert(pb.HavePendingTransactions() == !transactions.empty());
            },
            [&] { // GetStale()
                const auto stale{pb.GetStale()};
                const auto now{NodeClock::now()};
                size_t expected_stale{0};

                for (const auto& [tx, state] : transactions) {
                    const auto priority{DerivePriority(state)};
                    const bool is_stale{priority.num_confirmed == 0 ? state.time_added < now - PrivateBroadcast::INITIAL_STALE_DURATION : priority.last_confirmed < now - PrivateBroadcast::STALE_DURATION};
                    expected_stale += is_stale;
                    if (is_stale) {
                        Assert(std::ranges::find(stale, tx) != stale.end());
                    }
                }

                Assert(stale.size() == expected_stale);
                for (const auto& stale_tx : stale)
                    Assert(transactions.contains(stale_tx));
            },
            [&] { // GetBroadcastInfo()
                const auto all_broadcast_info{pb.GetBroadcastInfo()};
                Assert(all_broadcast_info.size() == transactions.size());

                for (const auto& info : all_broadcast_info) {
                    const auto it{transactions.find(info.tx)};
                    Assert(it != transactions.end());
                    const auto& state{it->second};
                    Assert(info.time_added == state.time_added);
                    Assert(info.peers.size() == state.sends.size());
                    for (size_t i{0}; i < info.peers.size(); ++i) {
                        Assert(info.peers[i].address == state.sends[i].address);
                        Assert(info.peers[i].sent == state.sends[i].picked);
                        Assert(info.peers[i].received == state.sends[i].confirmed);
                    }
                }
            },
            [&] { // Advance mock time.
                clock_ctx.set(ConsumeTime(fdp));
            });
    }
}
