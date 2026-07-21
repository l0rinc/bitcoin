#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Knots minimum relay input age policy."""

from decimal import Decimal

from test_framework.messages import COIN
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class MempoolMinRelayTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-persistmempool=0"]]

    def assert_testmempoolaccept(self, *, tx_hex, allowed, reject_reason=None):
        result = self.nodes[0].testmempoolaccept([tx_hex])[0]
        assert_equal(result["allowed"], allowed)
        if reject_reason is not None:
            assert_equal(result["reject-reason"], reject_reason)

    def fresh_confirmed_utxo(self):
        node = self.nodes[0]
        funding_tx = self.wallet.send_self_transfer(from_node=node)
        self.generate(self.wallet, 1)
        return self.wallet.get_utxo(txid=funding_tx["txid"], mark_as_spent=False, confirmed_only=True)

    def restart_with_policy(self, extra_args):
        self.restart_node(0, extra_args=["-persistmempool=0", *extra_args])
        self.wallet = MiniWallet(self.nodes[0])

    def test_minrelaymaturity(self):
        self.log.info("Test -minrelaymaturity rejects spends of too-recent confirmed outputs")
        self.restart_with_policy(["-minrelaymaturity=2"])

        fresh_utxo = self.fresh_confirmed_utxo()
        spend = self.wallet.create_self_transfer(utxo_to_spend=fresh_utxo)

        self.assert_testmempoolaccept(
            tx_hex=spend["hex"],
            allowed=False,
            reject_reason="bad-txns-input-immature-depth",
        )

        self.generate(self.wallet, 1)
        self.assert_testmempoolaccept(tx_hex=spend["hex"], allowed=True)

    def test_minrelaycoinblocks(self):
        self.log.info("Test -minrelaycoinblocks rejects spends without enough coin-block age")
        self.restart_with_policy([f"-minrelaycoinblocks={75 * COIN}"])

        fresh_utxo = self.fresh_confirmed_utxo()
        spend = self.wallet.create_self_transfer(utxo_to_spend=fresh_utxo, fee_rate=Decimal("0.003"))

        self.assert_testmempoolaccept(
            tx_hex=spend["hex"],
            allowed=False,
            reject_reason="bad-txns-input-immature-coinblocks",
        )

        self.generate(self.wallet, 1)
        self.assert_testmempoolaccept(tx_hex=spend["hex"], allowed=True)

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        self.test_minrelaymaturity()
        self.test_minrelaycoinblocks()


if __name__ == "__main__":
    MempoolMinRelayTest(__file__).main()
