#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the undo-file flush ordering described in issue #25539.

Blocks are written in download order, while undo data is written in validation
order. Undo data can therefore be added to an older rev file after the block
cursor has advanced to a newer blk file.

Replace the older rev file with a directory and force a chainstate flush. The
current behavior succeeds because it does not try to flush that older file.
"""

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
)


class UndoFlushTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-fastprune=1"]]

    def run_test(self):
        node = self.nodes[0]

        # -fastprune uses 64 KiB blockfiles. Keep file 0 below the limit until
        # explicitly rotating to file 1.
        payload_bytes = 18_000
        nulldata = "ff" * payload_bytes

        def mine_large_block():
            self.generateblock(node, output=f"raw(6a{nulldata})", transactions=[])

        self.log.info("Mine three large blocks on the active chain")
        for _ in range(3):
            mine_large_block()
        assert_equal(node.getblockcount(), 3)

        fork_base_hash = node.getblockhash(1)
        fork_base_time = node.getblock(fork_base_hash)["time"]

        self.log.info("Submit a side-chain block that remains inactive")
        block2 = create_block(int(fork_base_hash, 16), create_coinbase(2), ntime=fork_base_time + 1)
        block2.solve()
        assert_equal(node.submitblock(block2.serialize().hex()), "inconclusive")

        self.log.info("Rotate to blk00001.dat")
        mine_large_block()
        assert_equal(node.getblockcount(), 4)
        block_files = list(node.blocks_path.glob("blk[0-9][0-9][0-9][0-9][0-9].dat"))
        assert_greater_than(len(block_files), 1)

        self.log.info("Extend the side chain to trigger a reorg")
        tip = block2.hash_int
        height = 2
        block_time = block2.nTime + 1
        side_tip_hash = None
        for _ in range(3):
            height += 1
            block = create_block(tip, create_coinbase(height), ntime=block_time)
            block.solve()
            assert node.submitblock(block.serialize().hex()) in ("inconclusive", None)
            tip = block.hash_int
            block_time += 1
            side_tip_hash = block.hash_hex

        assert side_tip_hash is not None
        self.wait_until(lambda: node.getbestblockhash() == side_tip_hash, timeout=10)
        assert_equal(node.getblockcount(), 5)

        rev0 = node.blocks_path / "rev00000.dat"
        assert rev0.exists()
        self.log.info("Replace rev00000.dat with a directory and force a flush")
        rev0.unlink()
        rev0.mkdir()

        # TODO: The old dirty undo file must be flushed and abort this node.
        node.gettxoutsetinfo("none")
        assert_equal(node.getblockcount(), 5)


if __name__ == "__main__":
    UndoFlushTest(__file__).main()
