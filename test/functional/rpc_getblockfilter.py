#!/usr/bin/env python3
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getblockfilter RPC."""

from test_framework.messages import hash256
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal, assert_is_hex_string, assert_raises_rpc_error,
    )

FILTER_TYPES = ["basic"]

class GetBlockFilterTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-blockfilterindex"], []]

    def run_test(self):
        # Create two chains by disconnecting nodes 0 & 1, mining, then reconnecting
        self.disconnect_nodes(0, 1)

        self.generate(self.nodes[0], 3, sync_fun=self.no_op)
        self.generate(self.nodes[1], 4, sync_fun=self.no_op)

        assert_equal(self.nodes[0].getblockcount(), 3)
        chain0_hashes = [self.nodes[0].getblockhash(block_height) for block_height in range(4)]

        node = self.nodes[0]
        # Commit chain 0's index locator before chain 1 replaces its height entries
        node.wait_for_index_sync()
        old_parent, old_tip = chain0_hashes[-2:]
        parent_header = node.getblockfilter(old_parent)["header"]
        tip_header = node.getblockfilter(old_tip)["header"]
        self.restart_node(0)

        # Reorg node 0 to a new chain
        self.connect_nodes(0, 1)
        self.sync_blocks()

        assert_equal(self.nodes[0].getblockcount(), 4)
        chain1_hashes = [self.nodes[0].getblockhash(block_height) for block_height in range(4)]

        # Test getblockfilter returns a filter for all blocks and filter types on active chain
        for block_hash in chain1_hashes:
            for filter_type in FILTER_TYPES:
                result = self.nodes[0].getblockfilter(block_hash, filter_type)
                assert_is_hex_string(result['filter'])

        # Test getblockfilter returns a filter for all blocks and filter types on stale chain
        for block_hash in chain0_hashes:
            for filter_type in FILTER_TYPES:
                result = self.nodes[0].getblockfilter(block_hash, filter_type)
                assert_is_hex_string(result['filter'])

        # Test getblockfilter with unknown block
        bad_block_hash = "0123456789abcdef" * 4
        assert_raises_rpc_error(-5, "Block not found", self.nodes[0].getblockfilter, bad_block_hash, "basic")

        self.disconnect_nodes(0, 1)

        # Test getblockfilter for a known header before connecting its block
        block_hash = self.generate(self.nodes[1], 1, sync_fun=self.no_op)[0]
        self.nodes[0].submitheader(self.nodes[1].getblockheader(block_hash, False))

        # The header is known, but the block has never been connected
        assert_raises_rpc_error(
            -5, "Filter not found. Block was not connected to active chain.",
            self.nodes[0].getblockfilter, block_hash, "basic",
        )

        self.connect_nodes(0, 1)
        self.sync_blocks()

        # Test getblockfilter with undefined filter type
        genesis_hash = self.nodes[0].getblockhash(0)
        assert_raises_rpc_error(-5, "Unknown filtertype", self.nodes[0].getblockfilter, genesis_hash, "unknown")

        # Leave the index ahead of the chain so shutdown skips its commit
        node.wait_for_index_sync()
        node.invalidateblock(block_hash)

        # Test getblockfilter fails on node without compact block filter index
        self.restart_node(0, extra_args=["-blockfilterindex=0"])
        for filter_type in FILTER_TYPES:
            assert_raises_rpc_error(-1, "Index is not enabled for filtertype {}".format(filter_type),
                                    self.nodes[0].getblockfilter, genesis_hash, filter_type)

        # Restore the committed branch while the index is offline
        node.invalidateblock(chain1_hashes[1])
        assert_equal(node.getbestblockhash(), old_tip)
        self.restart_node(0)

        def check_child_header(previous_header):
            child = self.generatetodescriptor(node, 1, "raw(51)", sync_fun=self.no_op)[0]
            result = node.getblockfilter(child)
            filter_hash = hash256(bytes.fromhex(result["filter"]))
            expected = hash256(filter_hash + bytes.fromhex(previous_header)[::-1])
            assert_equal(result["header"], expected[::-1].hex())
            return child

        check_child_header(tip_header)
        node.invalidateblock(old_tip)
        assert_equal(node.getbestblockhash(), old_parent)
        child_tip = check_child_header(parent_header)

        self.stop_node(0)
        (node.chain_path / "indexes" / "blockfilter" / "basic" / "fltr00000.dat").unlink()
        with node.assert_debug_log([f"Cannot read basic block filter index tip {child_tip}"]):
            node.assert_start_raises_init_error()

if __name__ == '__main__':
    GetBlockFilterTest(__file__).main()
