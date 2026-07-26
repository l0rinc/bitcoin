#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
"""Exercise the outbound side of transaction inventory rate limiting."""

import time

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

SEND_RATE = 1
BUCKET_CAP = SEND_RATE * 30
NUM_TXS = 40


class TxRelayRateLimitOutboundTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # The framework's default topology is node 1 -> node 0, making node 1
        # the sender and node 0 the receiver on an outbound connection.
        self.extra_args = [[], [f'-txsendrate={SEND_RATE}']]

    def run_test(self):
        sender = self.nodes[1]
        receiver = self.nodes[0]
        wallet = MiniWallet(sender)

        mocktime = int(time.time())
        sender.setmocktime(mocktime)
        receiver.setmocktime(mocktime)

        self.generate(wallet, COINBASE_MATURITY + NUM_TXS + 10)
        sender_trigger_peer = sender.add_p2p_connection(P2PTxInvStore())
        receiver_trigger_peer = receiver.add_p2p_connection(P2PTxInvStore())

        # Initialize the per-peer trickle timers after the mock clock is set.
        sender.bumpmocktime(10)
        receiver.bumpmocktime(10)
        sender_trigger_peer.sync_with_ping()
        receiver_trigger_peer.sync_with_ping()

        assert_equal(sender.getnetworkinfo()['tx_send_rate'], SEND_RATE)

        transactions = [
            wallet.send_self_transfer(from_node=sender)
            for _ in range(NUM_TXS)
        ]
        txids = {tx['txid'] for tx in transactions}

        # The first 30 entries fit in the outbound count bucket; the rest
        # must wait in the global outbound backlog.
        assert_equal(
            sender.getnetworkinfo()['inv_buckets']['outbound']['backlog'],
            NUM_TXS - BUCKET_CAP,
        )

        # Refill the bucket and allow the receiver's normal non-preferred
        # announcement delay to elapse before requiring full convergence.
        for _ in range(12):
            if (sender.getnetworkinfo()['inv_buckets']['outbound']['backlog'] == 0
                    and set(receiver.getrawmempool()) == txids):
                break
            sender.bumpmocktime(1)
            receiver.bumpmocktime(1)
            sender_trigger_peer.sync_with_ping()
            receiver_trigger_peer.sync_with_ping()

        assert_equal(sender.getnetworkinfo()['inv_buckets']['outbound']['backlog'], 0)
        self.sync_mempools()
        assert_equal(set(receiver.getrawmempool()), txids)


if __name__ == '__main__':
    TxRelayRateLimitOutboundTest(__file__).main()
