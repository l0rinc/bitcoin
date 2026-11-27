#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that connected blocks survive an unclean shutdown without re-downloads."""

from test_framework.messages import CBlock, CBlockHeader, from_hex, msg_headers
from test_framework.p2p import P2PDataStore, p2p_lock
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than

TOTAL_BLOCKS = 400
FIRST_RUN_BLOCKS = 300


class UncleanShutdownRedownloadTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        self.extra_args = [[], ["-fastprune"], ["-fastprune"]]

    def setup_network(self):
        # Each receiving node downloads exclusively from its own P2PDataStore peers.
        self.setup_nodes()

    def sync_first_run_and_kill(self, node, blocks, flush):
        assert_equal(node.getblockcount(), 0)
        peer = node.add_p2p_connection(P2PDataStore())
        with node.assert_debug_log([], unexpected_msgs=["Writing chainstate to disk:"]):
            # Announce the full header chain while serving only the first prefix.
            with p2p_lock:
                peer.block_store = {block.hash_int: block for block in blocks[:FIRST_RUN_BLOCKS]}
                peer.last_block_hash = blocks[FIRST_RUN_BLOCKS - 1].hash_int
                peer.send_without_ping(msg_headers([CBlockHeader(block) for block in blocks]))
            self.wait_until(lambda: node.getbestblockhash() == blocks[FIRST_RUN_BLOCKS - 1].hash_hex)
        assert_greater_than(len(list(node.blocks_path.glob("blk*.dat"))), 1)
        if flush:
            with node.assert_debug_log(["Writing chainstate to disk:"]):
                node.gettxoutsetinfo()
        # Keep the UTXO changes in memory in the case without a forced flush.
        node.disconnect_p2ps()
        node.kill_process()

    def run_case(self, node, blocks, flush):
        self.sync_first_run_and_kill(node, blocks, flush)
        self.start_node(node.index)
        assert_equal(node.getblockcount(), FIRST_RUN_BLOCKS)
        peer = node.add_p2p_connection(P2PDataStore())
        peer.send_blocks_and_test(blocks, node)
        node.disconnect_p2ps()
        assert_equal(set(peer.getdata_requests), {block.hash_int for block in blocks[FIRST_RUN_BLOCKS:]})

    def run_test(self):
        hashes = []
        batch_size = 100
        for start in range(0, TOTAL_BLOCKS, batch_size):
            hashes.extend(self.generate(self.nodes[0], min(batch_size, TOTAL_BLOCKS - start), sync_fun=self.no_op))
        blocks = [from_hex(CBlock(), self.nodes[0].getblock(h, 0)) for h in hashes]

        for node, flush in ((self.nodes[1], False), (self.nodes[2], True)):
            self.log.info(f"Kill with forced chainstate flush={flush}")
            self.run_case(node, blocks, flush=flush)


if __name__ == "__main__":
    UncleanShutdownRedownloadTest(__file__).main()
