#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test coin-age priority transaction selection for mining."""

from decimal import Decimal

from test_framework.blocktools import NORMAL_GBT_REQUEST_PARAMS
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_greater_than
from test_framework.wallet import MiniWallet


class MiningCoinAgePriorityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[
            "-minrelaytxfee=0",
            "-blockmintxfee=0",
            "-blockprioritysize=1000000",
            "-persistmempool=1",
        ]]
        self.supports_cli = False

    def template_txids(self):
        return [tx["txid"] for tx in self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)["transactions"]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        self.log.info("Mine enough blocks to create mature coinbase outputs with different ages")
        self.generate(wallet, 130)

        confirmed_utxos = wallet.get_utxos(confirmed_only=True, mark_as_spent=False)
        old_utxo = min(confirmed_utxos, key=lambda utxo: utxo["height"])
        recent_utxo = max(confirmed_utxos, key=lambda utxo: utxo["height"])
        assert_greater_than(recent_utxo["height"], old_utxo["height"])

        self.log.info("Create an older zero-fee spend and a newer fee-paying spend")
        old_free_tx = wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=old_utxo,
            fee_rate=Decimal("0"),
        )
        recent_fee_tx = wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=recent_utxo,
            fee_rate=Decimal("0.001"),
        )

        self.log.info("Coin-age priority area mines the older zero-fee spend first")
        txids = self.template_txids()
        assert txids.index(old_free_tx["txid"]) < txids.index(recent_fee_tx["txid"])

        self.log.info("Without the priority area, fee sorting mines the fee-paying spend first")
        self.restart_node(0, extra_args=[
            "-minrelaytxfee=0",
            "-blockmintxfee=0",
            "-blockprioritysize=0",
            "-persistmempool=1",
        ])
        txids = self.template_txids()
        assert txids.index(recent_fee_tx["txid"]) < txids.index(old_free_tx["txid"])


if __name__ == "__main__":
    MiningCoinAgePriorityTest(__file__).main()
