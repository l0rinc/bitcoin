#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test relaxed prune flushing during prune-assumevalid IBD.

While -pruneassumevalid is active during initial block download, automatic
prune events delete block files without forcing a chainstate write - except
the first one, which anchors the chainstate so a crash never strands the
node without a starting point. The test kills the node after relaxed prune
events and asserts it recovers from the anchor without any peers.

Automatic pruning only fires once block file usage approaches the 550 MiB
minimum prune target, so the chain carries ~590 MiB of large blocks past the
assumevalid height. Pruning is checked when a new block file chunk is
allocated, and downloads race ahead of validation, so during the initial
sync every check happens while the validated tip is still too low to prune.
The prune events are instead triggered by mining extensions of large blocks
once the pruning node is synced. The source node mines under a mocktime two
days in the past: with an old tip the pruning node (on wall clock time)
never leaves initial block download, keeping the relaxed flush active, while
the source node (on the same mocktime) leaves it and announces its blocks.
"""

import time

from test_framework.blocktools import (
    COINBASE_MATURITY,
    create_block,
    create_coinbase,
)
from test_framework.script import (
    CScript,
    OP_NOP,
    OP_RETURN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

ASSUMEVALID_HEIGHT = COINBASE_MATURITY + 30
LARGE_BLOCKS = 620
# Complete the two-week proof-equivalent burial of the assumevalid block
# (2016 blocks at 10 minute spacing, plus margin).
SMALL_BLOCKS = 2100 - LARGE_BLOCKS
# Enough for multiple 16 MiB chunk allocations regardless of chunk fill.
EXTENSION_BLOCKS = 40
# Enough to lift block file usage back over the prune target after the first
# prune event removed a 128 MiB block file.
SECOND_EXTENSION_BLOCKS = 100

RELAXED_PRUNE_LOG = "Pruned block files without chainstate flush during -pruneassumevalid IBD"
FORCED_PRUNE_WRITE_LOG = "prune=1, large=0, critical=0, periodic=0"


def mine_large_blocks(node, n):
    # A large OP_RETURN + OP_NOP coinbase scriptPubKey is non-standard in a
    # non-coinbase transaction but consensus valid (pattern from
    # feature_pruning.py), giving ~950 KiB blocks without a mempool.
    big_script = CScript([OP_RETURN] + [OP_NOP] * 950000)
    best_block = node.getblock(node.getbestblockhash())
    height = best_block["height"] + 1
    ntime = best_block["time"] + 1
    previousblockhash = int(best_block["hash"], 16)

    for _ in range(n):
        block = create_block(hashprev=previousblockhash, ntime=ntime, coinbase=create_coinbase(height, script_pubkey=big_script))
        block.solve()
        node.submitblock(block.serialize().hex())
        previousblockhash = block.hash_int
        height += 1
        ntime += 1


class FeaturePruneAssumeValidFlushTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 120

    def setup_network(self):
        self.add_nodes(self.num_nodes, [["-maxreceivebuffer=20000"]] * self.num_nodes)
        self.start_node(0)

    def run_test(self):
        node0 = self.nodes[0]
        node0.setmocktime(int(time.time()) - 2 * 24 * 60 * 60)
        wallet = MiniWallet(node0)

        self.log.info("Mine an assumevalid chain buried under enough data to trigger automatic pruning")
        self.generate(wallet, ASSUMEVALID_HEIGHT, sync_fun=self.no_op)
        assumevalid_hash = node0.getbestblockhash()
        mine_large_blocks(node0, LARGE_BLOCKS)
        self.generate(wallet, SMALL_BLOCKS, sync_fun=self.no_op)
        synced_height = node0.getblockcount()
        assert_equal(synced_height, ASSUMEVALID_HEIGHT + LARGE_BLOCKS + SMALL_BLOCKS)
        final_chainwork = node0.getblockchaininfo()["chainwork"]
        prune_assumevalid_args = [
            "-maxreceivebuffer=20000",
            "-prune=550",
            "-pruneassumevalid",
            f"-assumevalid={assumevalid_hash}",
            f"-minimumchainwork={final_chainwork}",
        ]

        self.log.info("Sync assumevalid ancestors stripped and the rest stored")
        self.start_node(1, extra_args=prune_assumevalid_args)
        node1 = self.nodes[1]
        with node1.assert_debug_log(expected_msgs=["Requesting stripped prune-assumevalid block"], timeout=240):
            self.connect_nodes(1, 0)
            self.wait_until(lambda: node1.getblockcount() == synced_height, timeout=240)
        assert node1.getblockchaininfo()["initialblockdownload"]
        # Only the assumevalid ancestors were kept off disk so far.
        assert_equal(node1.getblockchaininfo()["pruneheight"], ASSUMEVALID_HEIGHT + 1)

        self.log.info("The first prune event still writes the chainstate to anchor a restart")
        with node1.assert_debug_log(expected_msgs=[FORCED_PRUNE_WRITE_LOG], unexpected_msgs=[RELAXED_PRUNE_LOG], timeout=180):
            mine_large_blocks(node0, EXTENSION_BLOCKS)
            self.sync_blocks(self.nodes)
        assert node1.getblockchaininfo()["pruneheight"] > ASSUMEVALID_HEIGHT + 1
        assert not (node1.blocks_path / "blk00000.dat").exists()

        self.log.info("Later prune events skip the chainstate write")
        with node1.assert_debug_log(expected_msgs=[RELAXED_PRUNE_LOG], timeout=180):
            mine_large_blocks(node0, SECOND_EXTENSION_BLOCKS)
            self.sync_blocks(self.nodes)
        final_height = synced_height + EXTENSION_BLOCKS + SECOND_EXTENSION_BLOCKS
        assert_equal(node1.getblockcount(), final_height)

        self.log.info("Recover from an unclean shutdown")
        node1.kill_process()
        self.start_node(1, extra_args=prune_assumevalid_args)
        node1 = self.nodes[1]
        # The relaxed prune events left the chainstate at the first prune
        # event's height, and everything above it survives in finalized block
        # files, so the node reconnects them locally instead of failing with
        # "Unable to replay blocks" or waiting forever for the pruned genesis
        # block.
        assert_equal(node1.getpeerinfo(), [])
        self.wait_until(lambda: node1.getblockcount() >= synced_height, timeout=240)
        # Redownload whatever the crash cut off of the unflushed block file tail.
        self.connect_nodes(1, 0)
        self.wait_until(lambda: node1.getblockcount() == final_height, timeout=240)
        assert_equal(node1.getbestblockhash(), node0.getbestblockhash())
        assert node1.getblockchaininfo()["pruneheight"] > ASSUMEVALID_HEIGHT + 1


if __name__ == "__main__":
    FeaturePruneAssumeValidFlushTest(__file__).main()
