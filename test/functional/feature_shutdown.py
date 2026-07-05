#!/usr/bin/env python3
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoind shutdown."""

from concurrent.futures import ThreadPoolExecutor

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


def waitfornewblock(node):
    return node.waitfornewblock()


def waitforblockheight(node):
    return node.waitforblockheight(1)


class ShutdownTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-rpcthreads=4"]]

    def run_test(self):
        waitfornewblock_node = self.nodes[0].create_new_rpc_connection()
        waitforblockheight_node = self.nodes[0].create_new_rpc_connection()
        # Force connection establishment by executing dummy commands.
        waitfornewblock_node.getblockcount()
        waitforblockheight_node.getblockcount()

        with ThreadPoolExecutor(max_workers=2) as executor:
            waitfornewblock_result = executor.submit(waitfornewblock, waitfornewblock_node)
            waitforblockheight_result = executor.submit(waitforblockheight, waitforblockheight_node)
            # Wait until the server is executing both long wait RPCs.
            self.wait_until(lambda: {"waitfornewblock", "waitforblockheight"}.issubset(
                {cmd["method"] for cmd in self.nodes[0].getrpcinfo()["active_commands"]}
            ))
            # Wait 1 second after requesting shutdown but not before the `stop` call
            # finishes. This is to ensure event loop waits for current connections
            # to close.
            self.stop_node(0, wait=1000)

            assert_equal(waitfornewblock_result.result(timeout=10)["height"], 0)
            assert_equal(waitforblockheight_result.result(timeout=10)["height"], 0)

if __name__ == '__main__':
    ShutdownTest(__file__).main()
