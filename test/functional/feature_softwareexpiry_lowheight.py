#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Knots developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test software expiry block rejection on a very low chain height."""

from test_framework.blocktools import create_block
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class SoftwareExpiryLowHeightTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-mocktime=1", "-softwareexpiry=1"]]

    def run_test(self):
        node = self.nodes[0]
        tip_hash = node.getbestblockhash()
        tip_time = node.getblock(tip_hash)["time"]
        block_time = tip_time + 1
        node.setmocktime(block_time)

        block = create_block(int(tip_hash, 16), height=1, ntime=block_time)
        block.solve()

        self.log.info("Expired height-1 block should be rejected without walking before genesis")
        with node.assert_debug_log(["node-expired"]):
            assert_equal(node.submitblock(block.serialize().hex()), "node-expired")
        assert_equal(node.getblockcount(), 0)


if __name__ == "__main__":
    SoftwareExpiryLowHeightTest(__file__).main()
