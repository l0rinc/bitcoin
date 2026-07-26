#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
"""Exercise the serialized-size side of transaction inventory rate limiting."""

from decimal import Decimal

from p2p_tx_relay_rate_limit import TxRelayRateLimitTest
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.p2p import P2PTxInvStore
from test_framework.util import assert_equal, assert_greater_than
from test_framework.wallet import MiniWallet


class TxRelayRateSizeTest(TxRelayRateLimitTest):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-txsendrate=1000']]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        num_txs = 140
        target_vsize = 100_000

        node.setmocktime(1_700_000_000)
        self.generate(wallet, COINBASE_MATURITY + num_txs + 10)

        peer = node.add_p2p_connection(P2PTxInvStore())
        node.bumpmocktime(10)
        peer.sync_with_ping()
        assert_equal(len(peer.get_invs()), 0)

        transactions = []
        for _ in range(num_txs):
            tx = wallet.create_self_transfer(
                confirmed_only=True,
                fee_rate=Decimal("0.01"),
                target_vsize=target_vsize,
            )
            assert_equal(tx["tx"].get_vsize(), target_vsize)
            node.sendrawtransaction(tx["hex"])
            transactions.append(tx)

        bucket = node.getnetworkinfo()["inv_buckets"]["inbound"]
        assert_greater_than(bucket["backlog"], 0)

        for _ in range(40):
            if node.getnetworkinfo()["inv_buckets"]["inbound"]["backlog"] == 0:
                break
            node.bumpmocktime(30)
            peer.sync_with_ping()

        announced = set(peer.get_invs())
        assert_equal(node.getnetworkinfo()["inv_buckets"]["inbound"]["backlog"], 0)
        assert_equal(len(announced), num_txs)
        assert_equal(announced, {int(tx["wtxid"], 16) for tx in transactions})


if __name__ == '__main__':
    TxRelayRateSizeTest(__file__).main()
