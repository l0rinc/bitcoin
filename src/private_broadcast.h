// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_PRIVATE_BROADCAST_H
#define BITCOIN_PRIVATE_BROADCAST_H

#include <net.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <sync.h>
#include <threadsafety.h>
#include <util/hasher.h>
#include <util/time.h>

#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

/**
 * Store a list of transactions to be broadcast privately. Supports the following operations:
 * - Add a new transaction
 * - Remove a transaction, after it has been seen by the network
 * - Mark a broadcast of a transaction (remember when and how many times)
 * - Get a transaction for broadcast, the one that has been broadcast fewer times and least recently
 */
class PrivateBroadcast
{
public:
    /**
     * Add a transaction to the storage.
     * @param[in] tx The transaction to add.
     * @retval true The transaction was added.
     * @retval false The transaction was already present.
     */
    bool Add(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Forget a transaction.
     * @return the number of times the transaction was broadcast if the transaction existed and was removed,
     * otherwise empty optional (the transaction was not in the storage).
     */
    std::optional<size_t> Remove(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get the transaction that has been broadcast fewest times and least recently.
     */
    std::optional<CTransactionRef> GetTxForBroadcast() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Mark a transaction as pushed to a given node. This is an intermediate state before
     * we get a PONG from the node which would confirm that the transaction has been received.
     * At the time we get the PONG we need to know which transaction we sent to that node,
     * so that we can account how many times we broadcast each transaction.
     */
    void PushedToNode(const NodeId& nodeid, const Txid& txid) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get the transaction that was pushed to a given node by PushedToNode().
     */
    std::optional<CTransactionRef> GetTxPushedToNode(const NodeId& nodeid) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Mark the end of a broadcast of a transaction. Either successful by receiving a PONG,
     * or unsuccessful by closing the connection to the node without getting PONG.
     * @return true if the reference by the given node id was removed and the transaction
     * we tried to send to this node is still in the private broadcast pool.
     */
    bool FinishBroadcast(const NodeId& nodeid, bool confirmed_by_node) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get the transactions that have not been broadcast recently.
     */
    std::vector<CTransactionRef> GetStale() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    struct Priority {
        size_t num_broadcasted{0};
        NodeClock::time_point last_broadcasted{};

        // Fewer broadcasts first, then older last_broadcasted first.
        bool operator<(const Priority& other) const
        {
            return std::tie(num_broadcasted, last_broadcasted) <
                   std::tie(other.num_broadcasted, other.last_broadcasted);
        }
    };

    struct Entry {
        CTransactionRef m_tx;
        Txid m_txid;
        Wtxid m_wtxid;
        Priority m_priority;

        Entry(const CTransactionRef& tx, Priority priority) : m_tx{tx},
                                                              m_txid{tx->GetHash()},
                                                              m_wtxid{tx->GetWitnessHash()},
                                                              m_priority{priority} {}

        friend bool operator<(const Entry& a, const Entry& b) { return a.m_priority < b.m_priority; }
    };

    static auto EqTxid(const Txid& txid) noexcept { return [txid](const Entry& e) { return e.m_txid == txid; }; }
    static auto EqTxid(const CTransactionRef& tx) noexcept { return EqTxid(tx->GetHash()); }
    static auto EqTxidWtxid(const CTransactionRef& tx) noexcept { return [tx](const Entry& e) { return EqTxid(tx)(e) && e.m_wtxid == tx->GetWitnessHash(); }; }

    mutable Mutex m_mutex;
    std::vector<Entry> m_entries GUARDED_BY(m_mutex);

    /**
     * Remember which transaction was sent to which node, so that when we get the PONG
     * from that node we can mark the transaction as broadcast.
     */
    std::unordered_map<NodeId, Txid> m_by_nodeid GUARDED_BY(m_mutex);
};

#endif // BITCOIN_PRIVATE_BROADCAST_H
