// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <netaddress.h>
#include <private_broadcast.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/time.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

namespace {
constexpr NodeId MAX_NODE_ID{7};

struct ModelPeer {
    NodeId nodeid;
    CService address;
    NodeClock::time_point sent;
    std::optional<NodeClock::time_point> received;
};

struct ModelTx {
    CTransactionRef tx;
    NodeClock::time_point time_added;
    std::vector<ModelPeer> peers;
};

using Model = std::map<uint256, ModelTx>;

CTransactionRef MakeDummyTx(uint32_t id, size_t num_witness)
{
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].nSequence = id;
    if (num_witness > 0) {
        mtx.vin[0].scriptWitness = CScriptWitness{};
        mtx.vin[0].scriptWitness.stack.resize(num_witness);
    }
    return MakeTransactionRef(mtx);
}

uint256 Key(const CTransactionRef& tx)
{
    Assert(tx != nullptr);
    return tx->GetWitnessHash().ToUint256();
}

CService AddressForNode(NodeId nodeid)
{
    in_addr ipv4_addr;
    ipv4_addr.s_addr = static_cast<uint32_t>(0x0a000001U + static_cast<uint32_t>(nodeid));
    return CService{ipv4_addr, static_cast<uint16_t>(10000 + nodeid)};
}

std::tuple<size_t, size_t, NodeClock::time_point, NodeClock::time_point> PriorityTuple(const ModelTx& tx)
{
    size_t num_confirmed{0};
    NodeClock::time_point last_picked{};
    NodeClock::time_point last_confirmed{};
    for (const auto& peer : tx.peers) {
        last_picked = std::max(last_picked, peer.sent);
        if (peer.received.has_value()) {
            ++num_confirmed;
            last_confirmed = std::max(last_confirmed, *peer.received);
        }
    }
    return {tx.peers.size(), num_confirmed, last_picked, last_confirmed};
}

std::set<uint256> ExpectedStale(const Model& model)
{
    const auto now{NodeClock::now()};
    std::set<uint256> ret;
    for (const auto& [key, tx] : model) {
        const auto [num_picked, num_confirmed, last_picked, last_confirmed]{PriorityTuple(tx)};
        (void)num_picked;
        (void)last_picked;
        if (num_confirmed == 0) {
            if (tx.time_added < now - PrivateBroadcast::INITIAL_STALE_DURATION) ret.insert(key);
        } else {
            if (last_confirmed < now - PrivateBroadcast::STALE_DURATION) ret.insert(key);
        }
    }
    return ret;
}

std::optional<uint256> ExpectedTxForNode(const Model& model, NodeId nodeid)
{
    for (const auto& [key, tx] : model) {
        for (const auto& peer : tx.peers) {
            if (peer.nodeid == nodeid) return key;
        }
    }
    return std::nullopt;
}

bool ExpectedConfirmedForNode(const Model& model, NodeId nodeid)
{
    for (const auto& [_, tx] : model) {
        for (const auto& peer : tx.peers) {
            if (peer.nodeid == nodeid) return peer.received.has_value();
        }
    }
    return false;
}

std::optional<NodeId> PickUnassignedNode(FuzzedDataProvider& provider, const Model& model)
{
    std::vector<NodeId> unassigned;
    for (NodeId nodeid{0}; nodeid <= MAX_NODE_ID; ++nodeid) {
        if (!ExpectedTxForNode(model, nodeid).has_value()) unassigned.push_back(nodeid);
    }
    if (unassigned.empty()) return std::nullopt;
    return PickValue(provider, unassigned);
}

void AssertMatchesModel(PrivateBroadcast& pb, const Model& model)
{
    const auto infos{pb.GetBroadcastInfo()};
    Assert(pb.HavePendingTransactions() == !model.empty());
    Assert(infos.size() == model.size());

    std::set<uint256> seen;
    for (const auto& info : infos) {
        Assert(info.tx != nullptr);
        const auto key{Key(info.tx)};
        Assert(seen.insert(key).second);
        const auto model_it{model.find(key)};
        Assert(model_it != model.end());
        const ModelTx& expected{model_it->second};
        Assert(info.tx == expected.tx);
        Assert(info.time_added == expected.time_added);
        Assert(info.peers.size() == expected.peers.size());
        for (size_t i{0}; i < info.peers.size(); ++i) {
            Assert(info.peers[i].address == expected.peers[i].address);
            Assert(info.peers[i].sent == expected.peers[i].sent);
            Assert(info.peers[i].received == expected.peers[i].received);
        }
    }

    for (NodeId nodeid{0}; nodeid <= MAX_NODE_ID; ++nodeid) {
        const auto expected_tx{ExpectedTxForNode(model, nodeid)};
        const auto actual_tx{pb.GetTxForNode(nodeid)};
        Assert(actual_tx.has_value() == expected_tx.has_value());
        if (actual_tx.has_value()) Assert(Key(*actual_tx) == *expected_tx);
        Assert(pb.DidNodeConfirmReception(nodeid) == ExpectedConfirmedForNode(model, nodeid));
    }

    std::set<uint256> stale_actual;
    for (const auto& tx : pb.GetStale()) stale_actual.insert(Key(tx));
    Assert(stale_actual == ExpectedStale(model));
}

std::vector<uint256> BestPickKeys(const Model& model)
{
    std::vector<uint256> ret;
    if (model.empty()) return ret;
    const auto best_priority{std::ranges::min_element(model, {}, [](const auto& entry) { return PriorityTuple(entry.second); })->second};
    const auto best_tuple{PriorityTuple(best_priority)};
    for (const auto& [key, tx] : model) {
        if (PriorityTuple(tx) == best_tuple) ret.push_back(key);
    }
    return ret;
}
} // namespace

FUZZ_TARGET(private_broadcast)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    FakeNodeClock clock{std::chrono::seconds{1'600'000'000}};

    const std::vector<CTransactionRef> txs{
        MakeDummyTx(/*id=*/0, /*num_witness=*/0),
        MakeDummyTx(/*id=*/0, /*num_witness=*/1),
        MakeDummyTx(/*id=*/1, /*num_witness=*/0),
        MakeDummyTx(/*id=*/2, /*num_witness=*/0),
        MakeDummyTx(/*id=*/3, /*num_witness=*/2),
        MakeDummyTx(/*id=*/4, /*num_witness=*/0),
        MakeDummyTx(/*id=*/5, /*num_witness=*/1),
        MakeDummyTx(/*id=*/6, /*num_witness=*/0),
    };
    Assert(txs[0]->GetHash() == txs[1]->GetHash());
    Assert(txs[0]->GetWitnessHash() != txs[1]->GetWitnessHash());

    const size_t cap{provider.ConsumeIntegralInRange<size_t>(1, txs.size())};
    PrivateBroadcast pb{cap};
    Model model;
    AssertMatchesModel(pb, model);

    LIMITED_WHILE(provider.remaining_bytes() > 0, 500)
    {
        CallOneOf(
            provider,
            [&] {
                const auto& tx{PickValue(provider, txs)};
                const auto key{Key(tx)};
                const auto result{pb.Add(tx)};
                if (model.contains(key)) {
                    Assert(result == PrivateBroadcast::AddResult::AlreadyPresent);
                } else if (model.size() >= cap) {
                    Assert(result == PrivateBroadcast::AddResult::QueueFull);
                } else {
                    Assert(result == PrivateBroadcast::AddResult::Added);
                    model.emplace(key, ModelTx{.tx = tx, .time_added = NodeClock::now(), .peers = {}});
                }
            },
            [&] {
                const auto& tx{PickValue(provider, txs)};
                const auto key{Key(tx)};
                const auto model_it{model.find(key)};
                const auto removed{pb.Remove(tx)};
                if (model_it == model.end()) {
                    Assert(!removed.has_value());
                } else {
                    Assert(removed.has_value());
                    const auto [_, num_confirmed, last_picked, last_confirmed]{PriorityTuple(model_it->second)};
                    (void)last_picked;
                    (void)last_confirmed;
                    Assert(*removed == num_confirmed);
                    model.erase(model_it);
                }
            },
            [&] {
                const auto nodeid{PickUnassignedNode(provider, model)};
                if (!nodeid.has_value()) return;
                const auto address{AddressForNode(*nodeid)};
                const auto picked{pb.PickTxForSend(*nodeid, address)};
                if (model.empty()) {
                    Assert(!picked.has_value());
                    return;
                }
                Assert(picked.has_value());
                const auto picked_key{Key(*picked)};
                const auto best_keys{BestPickKeys(model)};
                Assert(std::ranges::find(best_keys, picked_key) != best_keys.end());
                model.at(picked_key).peers.emplace_back(ModelPeer{.nodeid = *nodeid, .address = address, .sent = NodeClock::now(), .received = std::nullopt});
            },
            [&] {
                const NodeId nodeid{provider.ConsumeIntegralInRange<NodeId>(0, MAX_NODE_ID)};
                pb.NodeConfirmedReception(nodeid);
                if (const auto expected_tx{ExpectedTxForNode(model, nodeid)}) {
                    auto& peers{model.at(*expected_tx).peers};
                    const auto it{std::ranges::find(peers, nodeid, &ModelPeer::nodeid)};
                    Assert(it != peers.end());
                    it->received = NodeClock::now();
                }
            },
            [&] {
                const NodeId nodeid{provider.ConsumeIntegralInRange<NodeId>(0, MAX_NODE_ID)};
                const auto tx{pb.GetTxForNode(nodeid)};
                const auto expected_tx{ExpectedTxForNode(model, nodeid)};
                Assert(tx.has_value() == expected_tx.has_value());
                if (tx.has_value()) Assert(Key(*tx) == *expected_tx);
            },
            [&] {
                const auto seconds{provider.ConsumeIntegralInRange<int64_t>(0, 600)};
                clock += std::chrono::seconds{seconds};
            });
        AssertMatchesModel(pb, model);
    }
}
