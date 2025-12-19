#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test that coins database read errors trigger a shutdown message."""

from test_framework.test_framework import BitcoinTestFramework


class CoinsDBReadErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]

        for ldb_path in (node.chain_path / "chainstate").glob("*.ldb"):
            with open(ldb_path, "r+b") as f:
                f.write(b'\xff')

        with self.nodes[0].assert_debug_log(["block checksum mismatch", "Cannot read database, shutting down"]):
            try:
                txid = node.getblock(node.getblockhash(1))["tx"][0]
                node.gettxout(txid, 0)
            except Exception:
                pass

        self.wait_for_node_exit(0, timeout=10)

        # Mark as stopped to prevent cleanup
        node.running = False
        node.process = None


if __name__ == '__main__':
    CoinsDBReadErrorTest(__file__).main()
