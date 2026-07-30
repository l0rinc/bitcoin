#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test v1 mempool persistence with multiple chain-conflicting packages."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet


class MempoolPersistV1ConflictingPackagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-persistmempoolv1=1"]]
        self.uses_wallet = None

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        self.generate(wallet, COINBASE_MATURITY + 5, sync_fun=self.no_op)

        confirmed_inputs = [wallet.get_utxo(confirmed_only=True) for _ in range(3)]

        parent_a = wallet.create_self_transfer(
            utxo_to_spend=confirmed_inputs[0], fee=Decimal("0.00001"))
        child_a = wallet.create_self_transfer(
            utxo_to_spend=parent_a["new_utxo"], fee=Decimal("0.00001"))
        parent_b = wallet.create_self_transfer(
            utxo_to_spend=confirmed_inputs[1], fee=Decimal("0.00001"))
        child_b = wallet.create_self_transfer(
            utxo_to_spend=parent_b["new_utxo"], fee=Decimal("0.00001"))
        retained = wallet.create_self_transfer(
            utxo_to_spend=confirmed_inputs[2], fee=Decimal("0.00001"))

        for tx in (parent_a, child_a, parent_b, child_b, retained):
            node.sendrawtransaction(tx["hex"])
        assert_equal(set(node.getrawmempool()), {
            parent_a["txid"], child_a["txid"], parent_b["txid"],
            child_b["txid"], retained["txid"],
        })

        deltas = {
            parent_a["txid"]: 1000,
            child_a["txid"]: 2000,
            parent_b["txid"]: 3000,
            child_b["txid"]: 4000,
        }
        for txid, delta in deltas.items():
            node.prioritisetransaction(txid=txid, fee_delta=delta)

        mempool_path = node.chain_path / "mempool.dat"
        self.stop_node(0)
        persisted = mempool_path.read_bytes()
        assert_equal(int.from_bytes(persisted[:8], byteorder="little"), 1)

        self.start_node(0, extra_args=["-persistmempool=0", "-persistmempoolv1=1"])
        assert_equal(node.getrawmempool(), [])

        conflict_a = wallet.create_self_transfer(
            utxo_to_spend=confirmed_inputs[0], fee=Decimal("0.00002"))
        conflict_b = wallet.create_self_transfer(
            utxo_to_spend=confirmed_inputs[1], fee=Decimal("0.00002"))
        node.sendrawtransaction(conflict_a["hex"])
        node.sendrawtransaction(conflict_b["hex"])
        self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(node.getrawmempool(), [])

        self.stop_node(0)
        assert_equal(mempool_path.read_bytes(), persisted)

        expected_log = (
            "Imported mempool transactions from file: 1 succeeded, 4 failed, "
            "0 expired, 0 already there, 5 waiting for initial broadcast"
        )
        with node.assert_debug_log(expected_msgs=[expected_log]):
            self.start_node(0, extra_args=["-persistmempool=1", "-persistmempoolv1=1"])

        retained_entry = node.getmempoolentry(retained["txid"])
        info = node.getmempoolinfo()
        assert_equal(info["loaded"], True)
        assert_equal(info["size"], 1)
        assert_equal(info["bytes"], retained_entry["vsize"])
        assert_equal(info["total_fee"], retained_entry["fees"]["base"])
        assert_equal(set(node.getrawmempool()), {retained["txid"]})

        priorities = node.getprioritisedtransactions()
        assert_equal(set(priorities), set(deltas))
        for txid, delta in deltas.items():
            assert_equal(priorities[txid]["fee_delta"], delta)
            assert_equal(priorities[txid]["in_mempool"], False)
            assert "modified_fee" not in priorities[txid]

        accounting_before = {
            key: info[key] for key in ("size", "bytes", "total_fee")
        }
        for stale_tx in (parent_a, child_a, parent_b, child_b):
            assert_raises_rpc_error(
                -25,
                "bad-txns-inputs-missingorspent",
                node.sendrawtransaction,
                stale_tx["hex"],
            )
        assert_equal(
            {key: node.getmempoolinfo()[key] for key in accounting_before},
            accounting_before,
        )
        assert_equal(node.getprioritisedtransactions(), priorities)


if __name__ == "__main__":
    MempoolPersistV1ConflictingPackagesTest(__file__).main()
