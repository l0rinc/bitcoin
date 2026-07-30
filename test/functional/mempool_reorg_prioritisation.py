#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool package accounting and prioritisation after block invalidation."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class MempoolReorgPrioritisationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.uses_wallet = None

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        self.generate(wallet, COINBASE_MATURITY + 5, sync_fun=self.no_op)

        inputs = [wallet.get_utxo(confirmed_only=True) for _ in range(3)]
        parent = wallet.create_self_transfer(
            utxo_to_spend=inputs[0], fee=Decimal("0.00001"))
        child = wallet.create_self_transfer(
            utxo_to_spend=parent["new_utxo"], fee=Decimal("0.00001"))
        independent = wallet.create_self_transfer(
            utxo_to_spend=inputs[1], fee=Decimal("0.00001"))

        transactions = (parent, child, independent)
        for tx in transactions:
            node.sendrawtransaction(tx["hex"])
        txids = {tx["txid"] for tx in transactions}
        assert_equal(set(node.getrawmempool()), txids)

        deltas = {parent["txid"]: 1000, child["txid"]: 2000}
        for txid, delta in deltas.items():
            node.prioritisetransaction(txid=txid, fee_delta=delta)

        entries = {txid: node.getmempoolentry(txid) for txid in txids}
        expected_bytes = sum(entry["vsize"] for entry in entries.values())
        expected_fee = sum(entry["fees"]["base"] for entry in entries.values())
        assert_equal(node.getmempoolinfo()["size"], len(txids))
        assert_equal(node.getmempoolinfo()["bytes"], expected_bytes)
        assert_equal(node.getmempoolinfo()["total_fee"], expected_fee)
        assert_equal(
            set(node.getprioritisedtransactions()), set(deltas))

        block_hash = self.generate(node, 1, sync_fun=self.no_op)[0]
        block_txids = set(node.getblock(block_hash)["tx"])
        assert txids < block_txids
        assert_equal(node.getrawmempool(), [])
        assert_equal(node.getprioritisedtransactions(), {})
        assert_equal(node.getmempoolinfo()["size"], 0)
        assert_equal(node.getmempoolinfo()["bytes"], 0)
        assert_equal(node.getmempoolinfo()["total_fee"], Decimal("0"))

        node.invalidateblock(block_hash)

        assert_equal(set(node.getrawmempool()), txids)
        assert_equal(node.getprioritisedtransactions(), {})
        reorg_entries = {txid: node.getmempoolentry(txid) for txid in txids}
        assert_equal(reorg_entries[parent["txid"]]["ancestorcount"], 1)
        assert_equal(reorg_entries[child["txid"]]["ancestorcount"], 2)
        for entry in reorg_entries.values():
            assert_equal(entry["fees"]["modified"], entry["fees"]["base"])

        info = node.getmempoolinfo()
        assert_equal(info["size"], len(txids))
        assert_equal(info["bytes"], sum(entry["vsize"] for entry in reorg_entries.values()))
        assert_equal(info["total_fee"], sum(entry["fees"]["base"] for entry in reorg_entries.values()))


if __name__ == "__main__":
    MempoolReorgPrioritisationTest(__file__).main()
