#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test runtime import of a package and its composite mempool metadata."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import COIN, MiniWallet


class MempoolRuntimeImportTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [["-persistmempool=0"], ["-persistmempool=0"], []]
        self.uses_wallet = None

    def run_test(self):
        source, target = self.nodes[0], self.nodes[1]
        wallet = MiniWallet(self.nodes[2])

        # Keep the source isolated so its locally submitted package remains
        # unbroadcast while the target starts with an empty mempool.
        self.disconnect_nodes(0, 1)
        chain = wallet.create_self_transfer_chain(chain_length=3)
        for tx in chain:
            source.sendrawtransaction(hexstring=tx["hex"])

        fee_deltas = [1000, -200, 3000]
        for tx, fee_delta in zip(chain, fee_deltas):
            source.prioritisetransaction(txid=tx["txid"], fee_delta=fee_delta)

        source_info = source.getmempoolinfo()
        source_mempool = source.getrawmempool(verbose=True)
        assert_equal(source_info["size"], len(chain))
        assert_equal(source_info["unbroadcastcount"], len(chain))
        assert_equal(source_info["total_fee"], sum(entry["fees"]["base"] for entry in source_mempool.values()))

        stable_fields = [
            "vsize",
            "weight",
            "descendantcount",
            "descendantsize",
            "ancestorcount",
            "ancestorsize",
            "wtxid",
            "fees",
            "depends",
            "spentby",
            "unbroadcast",
        ]
        expected_entries = {}
        for tx, fee_delta in zip(chain, fee_deltas):
            txid = tx["txid"]
            entry = source_mempool[txid]
            assert_equal(entry["unbroadcast"], True)
            assert_equal(entry["fees"]["modified"], entry["fees"]["base"] + Decimal(fee_delta) / COIN)
            expected_entries[txid] = {field: entry[field] for field in stable_fields}

        mempool_path = source.savemempool()["filename"]
        assert_equal({}, target.importmempool(mempool_path, {
            "apply_fee_delta_priority": True,
            "apply_unbroadcast_set": True,
        }))

        target_info = target.getmempoolinfo()
        target_mempool = target.getrawmempool(verbose=True)
        assert_equal(target_info["size"], source_info["size"])
        assert_equal(target_info["bytes"], source_info["bytes"])
        assert_equal(target_info["total_fee"], source_info["total_fee"])
        assert_equal(target_info["unbroadcastcount"], source_info["unbroadcastcount"])
        for txid, expected in expected_entries.items():
            actual = target_mempool[txid]
            for field in stable_fields:
                assert_equal(actual[field], expected[field])


if __name__ == "__main__":
    MempoolRuntimeImportTest(__file__).main()
