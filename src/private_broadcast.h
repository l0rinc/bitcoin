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
#include <util/time.h>

#include <compare>
#include <optional>
#include <unordered_map>
#include <vector>

/**
 * Store a list of transactions to be broadcast privately. Supports the following operations:
 * - Add a new transaction
 * - Remove a transaction
 * - Pick a transaction for sending to one recipient
 * - Query which transaction has been picked for sending to a given recipient node
 * - Mark that a confirmation of reception for a transaction by a given recipient node
 * - Query whether a given recipient node has confirmed reception
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
     * @param[in] tx Transaction to forget.
     * @return The number of times the transaction was sent if the transaction existed
     * and was removed, otherwise empty optional (the transaction was not in the storage).
     */
    std::optional<size_t> Remove(const CTransactionRef& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Pick the transaction that has been sent fewest times and least recently.
     * @param[in] will_send_to_nodeid Will remember that the returned transaction
     * was picked for sending to this node.
     * @return Most urgent transaction or nullopt if there are no transactions.
     */
    std::optional<CTransactionRef> PickTxForSend(const NodeId& will_send_to_nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get the transaction that was picked for sending to a given node by PickTxForSend().
     * @param[in] nodeid Node to which a transaction is being (or was) sent.
     * @return Transaction or nullopt if the nodeid is unknown.
     */
    std::optional<CTransactionRef> GetTxForNode(const NodeId& nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Mark that the node has confirmed reception of the transaction we sent it by
     * responding with `PONG` to our `PING` message.
     * @param[in] nodeid Node that we sent a transaction to.
     */
    void NodeConfirmedReception(const NodeId& nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Check if the node has confirmed reception of the transaction.
     * @retval true Node has confirmed, `NodeConfirmedReception()` has been called.
     * @retval false Node has not confirmed, `NodeConfirmedReception()` has not been called.
     */
    bool DidNodeConfirmReception(const NodeId& nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get the transactions that have not been broadcast recently.
     */
    std::vector<CTransactionRef> GetStale() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    /// Status of a transaction sent to a given node.
    struct SendStatus {
        NodeId nodeid; /// Node to which the transaction will be sent (or was sent).
        NodeClock::time_point picked; ///< When was the transaction picked for sending to the node.
        std::optional<NodeClock::time_point> confirmed; ///< When was the transaction reception confirmed by the node (by PONG).
    };

    /// Cumulative stats from all the send attempts for a transaction. Used to prioritize transactions.
    struct Priority {
        size_t num_picked{0}; ///< Number of times the transaction was picked for sending.
        NodeClock::time_point last_picked{}; ///< The most recent time when the transaction was picked for sending.
        size_t num_confirmed{0}; ///< Number of nodes that have confirmed reception of a transaction (by PONG).
        NodeClock::time_point last_confirmed{}; ///< The most recent time when the transaction was confirmed.

        std::strong_ordering operator<=>(const Priority& other) const;
    };

    /// A pair of a transaction and a sent status for a given node. Convenience return type of GetSendStatusByNode().
    struct TxAndSendStatusForNode {
        CTransactionRef& tx;
        SendStatus& send_status;
    };

    // No need for salted hasher because we are going to store just a bunch of locally originating transactions.

    struct CTransactionRefHash {
        size_t operator()(const CTransactionRef& tx) const
        {
            return static_cast<size_t>(tx->GetHash().ToUint256().GetUint64(0));
        }
    };

    struct CTransactionRefComp {
        bool operator()(const CTransactionRef& a, const CTransactionRef& b) const
        {
            return a->GetHash() == b->GetHash() && a->GetWitnessHash() == b->GetWitnessHash();
        }
    };

    /**
     * Derive the sending priority of a transaction.
     * @param[in] sent_to List of nodes that the transaction has been sent to.
     */
    static Priority DerivePriority(const std::vector<SendStatus>& sent_to);

    /**
     * Find which transaction we sent to a given node (marked by PickTxForSend()).
     * @return That transaction together with the send status or nullopt if we did not
     * send any transaction to the given node.
     */
    std::optional<TxAndSendStatusForNode> GetSendStatusByNode(const NodeId& nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

    mutable Mutex m_mutex;
    std::unordered_map<CTransactionRef, std::vector<SendStatus>, CTransactionRefHash, CTransactionRefComp>
        m_transactions GUARDED_BY(m_mutex);
};

#endif // BITCOIN_PRIVATE_BROADCAST_H
