#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that a partially expired package is not reloaded as an orphan."""

import os
import time

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


MEMPOOL_EXPIRY_HOURS = 1


class MempoolPersistPartialPackageTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[f"-mempoolexpiry={MEMPOOL_EXPIRY_HOURS}"]]
        self.uses_wallet = None

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        self.generate(wallet, COINBASE_MATURITY + 1)

        parent_time = int(time.time())
        node.setmocktime(parent_time)
        parent = wallet.send_self_transfer(from_node=node)
        parent_utxo = wallet.get_utxo(txid=parent["txid"])
        parent_entry = node.getmempoolentry(parent["txid"])

        child_time = parent_time + 60 * 60 - 60
        node.setmocktime(child_time)
        child = wallet.send_self_transfer(from_node=node, utxo_to_spend=parent_utxo)
        child_entry = node.getmempoolentry(child["txid"])
        assert_equal(child_entry["depends"], [parent["txid"]])
        assert_equal(parent_entry["time"], parent_time)
        assert_equal(child_entry["time"], child_time)
        assert_equal(set(node.getrawmempool()), {parent["txid"], child["txid"]})

        reload_time = parent_time + 60 * 60 + 1
        node.setmocktime(reload_time)
        mempool_path = os.path.join(node.chain_path, "mempool.dat")
        self.stop_node(0)
        assert os.path.isfile(mempool_path)

        expected_import = "Imported mempool transactions from file: 0 succeeded, 1 failed, 1 expired, 0 already there, 2 waiting for initial broadcast"
        with node.assert_debug_log(expected_msgs=[expected_import]):
            self.start_node(0, extra_args=[
                f"-mempoolexpiry={MEMPOOL_EXPIRY_HOURS}",
                f"-mocktime={reload_time}",
            ])

        assert node.getmempoolinfo()["loaded"]
        assert_equal(node.getrawmempool(), [])
        assert_equal(node.getmempoolinfo()["size"], 0)
        assert_equal(node.getmempoolinfo()["bytes"], 0)
        assert_equal(node.getmempoolinfo()["total_fee"], 0)
        assert_equal(node.getmempoolinfo()["unbroadcastcount"], 0)


if __name__ == "__main__":
    MempoolPersistPartialPackageTest(__file__).main()
