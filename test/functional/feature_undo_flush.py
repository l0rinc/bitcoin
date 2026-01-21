#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test behavior described in issue #25539.

Blocks are written to blk files in (often out-of-order) download order, while undo data is written to rev files
in validation order. This can lead to undo data being written to an older rev file after a newer blk file is in
use.

This test sets up that situation and then replaces `rev00000.dat` with a directory. A forced flush via
`gettxoutsetinfo` should then abort the node, demonstrating that the older rev file is flushed.

The `rev00000.dat` swap is a deterministic stand-in for power loss: if the node can write the block index
database without flushing older undo files, an unclean shutdown can leave `BLOCK_HAVE_UNDO` persisted while undo
data is missing on disk.
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

        # -fastprune uses smaller blockfiles (64KiB). Mine blocks sized so file 0 stays below the limit until we
        # explicitly cause a rotation to file 1.
        payload_bytes = 18_000
        nulldata = "ff" * payload_bytes

        def mine_large_block():
            self.generateblock(node, output=f"raw(6a{nulldata})", transactions=[])

        self.log.info("Mine 3 large blocks on the active chain")
        for _ in range(3):
            mine_large_block()
        assert_equal(node.getblockcount(), 3)

        fork_base_hash = node.getblockhash(1)
        fork_base_time = node.getblock(fork_base_hash)["time"]

        self.log.info("Submit a side chain block (remains non-active)")
        b2 = create_block(int(fork_base_hash, 16), create_coinbase(2), fork_base_time + 1)
        b2.solve()
        assert_equal(node.submitblock(b2.serialize().hex()), "inconclusive")
        assert_equal(node.getblockcount(), 3)

        self.log.info("Mine 1 more large block to rotate to blk00001.dat")
        mine_large_block()
        assert_equal(node.getblockcount(), 4)

        block_files = list(node.blocks_path.glob("blk[0-9][0-9][0-9][0-9][0-9].dat"))
        assert_greater_than(len(block_files), 1)

        self.log.info("Extend side chain to trigger reorg")
        tip = b2.hash_int
        height = 2
        block_time = b2.nTime + 1
        side_tip_hash = None

        for _ in range(3):  # Create blocks 3-5 on the side chain
            height += 1
            block = create_block(tip, create_coinbase(height), block_time)
            block.solve()
            res = node.submitblock(block.serialize().hex())
            assert res in ("inconclusive", None)
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

        # gettxoutsetinfo calls ForceFlushStateToDisk() before scanning the UTXO set.
        expected_stderr = (
            "Error: A fatal internal error occurred, see debug.log for details: "
            "Flushing undo file to disk failed. This is likely the result of an I/O error."
        )
        try:
            node.gettxoutsetinfo("none")
        except Exception:
            pass
        node.wait_until_stopped(timeout=5, expect_error=True, expected_stderr=expected_stderr)


if __name__ == "__main__":
    UndoFlushTest(__file__).main()
