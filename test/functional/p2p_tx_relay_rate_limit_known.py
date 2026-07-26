#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
"""Ensure already-known transactions do not consume the global relay budget."""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import CInv, MSG_WTX, msg_inv
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


SEND_RATE = 1
BUCKET_CAP = SEND_RATE * 30


class TxRelayRateLimitKnownTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f"-txsendrate={SEND_RATE}"]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        node.setmocktime(1_700_000_000)
        self.generate(wallet, COINBASE_MATURITY + BUCKET_CAP + 10)

        peers = [node.add_p2p_connection(P2PInterface()) for _ in range(2)]
        node.bumpmocktime(10)
        for peer in peers:
            peer.sync_with_ping()

        # Tell both peers about each transaction before submitting it locally.
        # The node records those INV hashes in each peer's known filter, while
        # P2PInterface deliberately does not answer the resulting GETDATA.
        for _ in range(BUCKET_CAP):
            tx = wallet.create_self_transfer()
            inv = msg_inv([CInv(MSG_WTX, int(tx["wtxid"], 16))])
            for peer in peers:
                peer.send_and_ping(inv)
            wallet.sendrawtransaction(from_node=node, tx_hex=tx["hex"])

        for peer in peers:
            peer.sync_with_ping()
            assert_equal(peer.message_count["inv"], 0)

        bucket = node.getnetworkinfo()["inv_buckets"]["inbound"]
        assert_equal(bucket["backlog"], 0)
        assert_equal(bucket["count_tok"], BUCKET_CAP)

        # This transaction is unknown to both peers and should consume one
        # token and enter each peer's ordinary per-peer queue immediately.
        unknown = wallet.create_self_transfer()
        wallet.sendrawtransaction(from_node=node, tx_hex=unknown["hex"])
        bucket = node.getnetworkinfo()["inv_buckets"]["inbound"]
        assert_equal(bucket["backlog"], 0)
        assert_equal(bucket["count_tok"], BUCKET_CAP - 1)
        for peer in peers:
            assert_equal(peer.message_count["inv"], 0)


if __name__ == "__main__":
    TxRelayRateLimitKnownTest(__file__).main()
