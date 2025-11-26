// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <private_broadcast.h>
#include <util/check.h>

#include <algorithm>
#include <tuple>

/// If a transaction is not received back from the network for this duration
/// after it is broadcast, then we consider it stale / for rebroadcasting.
static constexpr auto STALE_DURATION{1min};

bool PrivateBroadcast::Add(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    return m_transactions.try_emplace(tx).second;
}

std::optional<size_t> PrivateBroadcast::Remove(const CTransactionRef& tx)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (const auto handle{m_transactions.extract(tx)}) {
        return DerivePriority(handle.mapped()).num_confirmed;
    }
    return std::nullopt;
}

std::optional<CTransactionRef> PrivateBroadcast::PickTxForSend(const NodeId& will_send_to_nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);

    if (const auto it{std::ranges::max_element(
            m_transactions,
            [](const auto& a, const auto& b) { return a < b; },
            [](const auto& el) { return DerivePriority(el.second); })};
        it != m_transactions.end()
    ) {
        auto& [tx, sent_to]{*it};
        sent_to.emplace_back(will_send_to_nodeid, NodeClock::now());
        return tx;
    }

    return std::nullopt;
}

std::optional<CTransactionRef> PrivateBroadcast::GetTxForNode(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (const auto tx_and_status{GetSendStatusByNode(nodeid)}) {
        return tx_and_status->tx;
    }
    return std::nullopt;
}

void PrivateBroadcast::NodeConfirmedReception(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (const auto tx_and_status{GetSendStatusByNode(nodeid)}) {
        tx_and_status->send_status.confirmed = NodeClock::now();
    }
}

bool PrivateBroadcast::DidNodeConfirmReception(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (const auto tx_and_status{GetSendStatusByNode(nodeid)}) {
        return !!tx_and_status->send_status.confirmed;
    }
    return false;
}

std::vector<CTransactionRef> PrivateBroadcast::GetStale() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto stale_time{NodeClock::now() - STALE_DURATION};
    std::vector<CTransactionRef> stale;
    stale.reserve(m_transactions.size());
    for (const auto& [tx, send_status] : m_transactions) {
        if (const Priority p{DerivePriority(send_status)}; p.last_confirmed < stale_time) {
            stale.push_back(tx);
        }
    }
    return stale;
}

PrivateBroadcast::Priority PrivateBroadcast::DerivePriority(const std::vector<SendStatus>& sent_to)
{
    Priority p;
    p.num_picked = sent_to.size();
    for (const auto& send_status : sent_to) {
        p.last_picked = std::max(p.last_picked, send_status.picked);
        if (send_status.confirmed) {
            ++p.num_confirmed;
            p.last_confirmed = std::max(p.last_confirmed, *send_status.confirmed);
        }
    }
    return p;
}

std::optional<PrivateBroadcast::TxAndSendStatusForNode> PrivateBroadcast::GetSendStatusByNode(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
{
    AssertLockHeld(m_mutex);
    for (auto& [tx, sent_to] : m_transactions) {
        for (auto& send_status : sent_to) {
            if (send_status.nodeid == nodeid) {
                return TxAndSendStatusForNode{.tx = tx, .send_status = send_status};
            }
        }
    }
    return std::nullopt;
}
