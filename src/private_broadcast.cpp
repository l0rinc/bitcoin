// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <private_broadcast.h>

#include <util/check.h>

#include <algorithm>
#include <unordered_set>

PrivateBroadcast::AddResult PrivateBroadcast::Add(const CTransactionRef& tx)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    Assert(tx != nullptr);
    LOCK(m_mutex);
    // Re-adding an already-tracked transaction is a no-op regardless of the cap.
    if (m_transactions.contains(tx)) {
        AssertInvariants();
        return AddResult::AlreadyPresent;
    }

    if (m_transactions.size() >= m_max_transactions) {
        AssertInvariants();
        return AddResult::QueueFull;
    }

    m_transactions.try_emplace(tx);
    AssertInvariants();
    return AddResult::Added;
}

std::optional<size_t> PrivateBroadcast::Remove(const CTransactionRef& tx)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    Assert(tx != nullptr);
    LOCK(m_mutex);
    const auto handle{m_transactions.extract(tx)};
    AssertInvariants();
    if (handle) {
        const auto p{DerivePriority(handle.mapped().send_statuses)};
        return p.num_confirmed;
    }
    return std::nullopt;
}

std::optional<CTransactionRef> PrivateBroadcast::PickTxForSend(const NodeId& will_send_to_nodeid, const CService& will_send_to_address)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    Assert(!GetSendStatusByNode(will_send_to_nodeid).has_value());

    if (GetSendStatusByNode(will_send_to_nodeid).has_value()) { // nodeid reuse, shouldn't send >1 tx to a given node
        Assume(false);
        return std::nullopt;
    }

    const auto it{std::ranges::max_element(
            m_transactions,
            [](const auto& a, const auto& b) { return a < b; },
            [](const auto& el) { return DerivePriority(el.second.send_statuses); })};

    if (it != m_transactions.end()) {
        auto& [tx, state]{*it};
        state.send_statuses.emplace_back(will_send_to_nodeid, will_send_to_address, NodeClock::now());
        AssertInvariants();
        return tx;
    }

    return std::nullopt;
}

std::optional<CTransactionRef> PrivateBroadcast::GetTxForNode(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto tx_and_status{GetSendStatusByNode(nodeid)};
    if (tx_and_status.has_value()) {
        return tx_and_status.value().tx;
    }
    return std::nullopt;
}

void PrivateBroadcast::NodeConfirmedReception(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto tx_and_status{GetSendStatusByNode(nodeid)};
    if (tx_and_status.has_value()) {
        tx_and_status.value().send_status.confirmed = NodeClock::now();
    }
    AssertInvariants();
}

bool PrivateBroadcast::DidNodeConfirmReception(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto tx_and_status{GetSendStatusByNode(nodeid)};
    if (tx_and_status.has_value()) {
        return tx_and_status.value().send_status.confirmed.has_value();
    }
    return false;
}

bool PrivateBroadcast::HavePendingTransactions()
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    return !m_transactions.empty();
}

std::vector<CTransactionRef> PrivateBroadcast::GetStale() const
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto now{NodeClock::now()};
    std::vector<CTransactionRef> stale;
    for (const auto& [tx, state] : m_transactions) {
        const Priority p{DerivePriority(state.send_statuses)};
        if (p.num_confirmed == 0) {
            if (state.time_added < now - INITIAL_STALE_DURATION) stale.push_back(tx);
        } else {
            if (p.last_confirmed < now - STALE_DURATION) stale.push_back(tx);
        }
    }
    return stale;
}

std::vector<PrivateBroadcast::TxBroadcastInfo> PrivateBroadcast::GetBroadcastInfo() const
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    std::vector<TxBroadcastInfo> entries;
    entries.reserve(m_transactions.size());

    for (const auto& [tx, state] : m_transactions) {
        std::vector<PeerSendInfo> peers;
        peers.reserve(state.send_statuses.size());
        for (const auto& status : state.send_statuses) {
            peers.emplace_back(PeerSendInfo{.address = status.address, .sent = status.picked, .received = status.confirmed});
        }
        entries.emplace_back(TxBroadcastInfo{.tx = tx, .time_added = state.time_added, .peers = std::move(peers)});
    }

    return entries;
}

PrivateBroadcast::Priority PrivateBroadcast::DerivePriority(const std::vector<SendStatus>& sent_to)
{
    Priority p;
    p.num_picked = sent_to.size();
    for (const auto& send_status : sent_to) {
        p.last_picked = std::max(p.last_picked, send_status.picked);
        if (send_status.confirmed.has_value()) {
            ++p.num_confirmed;
            p.last_confirmed = std::max(p.last_confirmed, send_status.confirmed.value());
        }
    }
    return p;
}

std::optional<PrivateBroadcast::TxAndSendStatusForNode> PrivateBroadcast::GetSendStatusByNode(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
{
    AssertLockHeld(m_mutex);
    for (auto& [tx, state] : m_transactions) {
        for (auto& send_status : state.send_statuses) {
            if (send_status.nodeid == nodeid) {
                return TxAndSendStatusForNode{.tx = tx, .send_status = send_status};
            }
        }
    }
    return std::nullopt;
}

void PrivateBroadcast::AssertInvariants() const
    EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
{
    AssertLockHeld(m_mutex);
    std::unordered_set<NodeId> sent_nodes;
    for (const auto& [tx, state] : m_transactions) {
        Assert(tx != nullptr);

        size_t num_confirmed{0};
        NodeClock::time_point last_picked{};
        NodeClock::time_point last_confirmed{};
        for (const auto& send_status : state.send_statuses) {
            const auto [_, inserted]{sent_nodes.insert(send_status.nodeid)};
            Assert(inserted);
            last_picked = std::max(last_picked, send_status.picked);
            if (send_status.confirmed.has_value()) {
                ++num_confirmed;
                last_confirmed = std::max(last_confirmed, *send_status.confirmed);
            }
        }

        const Priority p{DerivePriority(state.send_statuses)};
        Assert(p.num_picked == state.send_statuses.size());
        Assert(p.last_picked == last_picked);
        Assert(p.num_confirmed == num_confirmed);
        Assert(p.last_confirmed == last_confirmed);
    }
}
