#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Test that empty HEADERS responses do not stall IBD peer selection."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class AddnodeIBDStallTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        stale_peer = self.nodes[0]
        syncing_node = self.nodes[1]
        useful_peer = self.nodes[2]

        target_height = 10
        self.generate(useful_peer, target_height, sync_fun=self.no_op)
        assert_equal(stale_peer.getblockcount(), 0)
        assert_equal(syncing_node.getblockcount(), 0)
        assert_equal(useful_peer.getblockcount(), target_height)

        # Connect to the stale peer first. It has no headers beyond our tip, so
        # it responds to the initial GETHEADERS with an empty HEADERS message.
        self.connect_nodes(1, 0)

        stale_subver = stale_peer.getnetworkinfo()["subversion"]

        def stale_connection():
            for peer in syncing_node.getpeerinfo():
                if peer["subver"] == stale_subver and not peer["inbound"]:
                    return peer
            return None

        self.wait_until(lambda: stale_connection() and stale_connection()["bytessent_per_msg"].get("getheaders", 0) > 0)
        self.wait_until(lambda: stale_connection() and stale_connection()["bytesrecv_per_msg"].get("headers", 0) > 0)

        # The empty response above must release the initial sync slot, allowing
        # the node to sync from the useful peer immediately instead of waiting
        # for the long headers timeout.
        self.connect_nodes(1, 2)
        self.wait_until(lambda: syncing_node.getblockcount() == target_height)


if __name__ == "__main__":
    AddnodeIBDStallTest(__file__).main()
