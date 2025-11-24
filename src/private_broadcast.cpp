// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <private_broadcast.h>

#include <algorithm>

using namespace std::ranges;

/// If a transaction is not received back from the network for this duration
/// after it is broadcast, then we consider it stale for rebroadcasting.
static constexpr auto STALE_DURATION{1min};

bool PrivateBroadcast::Add(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (any_of(m_entries, EqTxid(tx))) return false;
    m_entries.emplace_back(tx, Priority{});
    return true;
}

std::optional<size_t> PrivateBroadcast::Remove(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (auto it{find_if(m_entries, EqTxidWtxid(tx))}; it != m_entries.end()) {
        const auto num_broadcasted{it->m_priority.num_broadcasted};
        *it = std::move(m_entries.back()); m_entries.pop_back();
        return num_broadcasted;
    }
    return std::nullopt;
}

std::optional<CTransactionRef> PrivateBroadcast::GetTxForBroadcast() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (auto it{std::min_element(m_entries.begin(), m_entries.end())}; it != m_entries.end()) return it->m_tx;
    return std::nullopt;
}

void PrivateBroadcast::PushedToNode(const NodeId& nodeid, const Txid& txid) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    WITH_LOCK(m_mutex, m_by_nodeid.emplace(nodeid, txid));
}

std::optional<CTransactionRef> PrivateBroadcast::GetTxPushedToNode(const NodeId& nodeid) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (auto it_id{m_by_nodeid.find(nodeid)}; it_id != m_by_nodeid.end()) {
        if (auto it_tx{find_if(m_entries, EqTxid(it_id->second))}; it_tx != m_entries.end()) return it_tx->m_tx;
    }
    return std::nullopt;
}

bool PrivateBroadcast::FinishBroadcast(const NodeId& nodeid, bool confirmed_by_node) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (auto handle{m_by_nodeid.extract(nodeid)}) {
        if (auto it{find_if(m_entries, EqTxid(handle.mapped()))}; it != m_entries.end()) {
            if (confirmed_by_node) {
                ++it->m_priority.num_broadcasted;
                it->m_priority.last_broadcasted = NodeClock::now();
            }
            return true;
        }
    }
    return false;
}

std::vector<CTransactionRef> PrivateBroadcast::GetStale() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto stale_time{NodeClock::now() - STALE_DURATION};
    std::vector<CTransactionRef> stale;
    stale.reserve(m_entries.size());
    for (const auto& [tx, _, __, priority] : m_entries) {
        if (priority.last_broadcasted < stale_time) stale.push_back(tx);
    }
    return stale;
}
