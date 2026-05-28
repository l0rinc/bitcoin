#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test chainstate compaction after connecting the assumevalid block."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


NODE_ARGS = ["-debug=coindb"]
STALE_TIP_AGE = 25 * 60 * 60
FORCE_FLUSH_MSG = "Writing chainstate to disk: flush mode=FORCE_FLUSH"
IBD_EXIT_MSG = "Leaving InitialBlockDownload"
COMPACTION_MSGS = [
    "Starting chainstate compaction",
    "Finished chainstate compaction",
]
COMPACTION_TRIGGER_MSGS = [FORCE_FLUSH_MSG, *COMPACTION_MSGS]


class CompactChainstateTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [NODE_ARGS.copy(), NODE_ARGS.copy()]

    def setup_network(self):
        self.add_nodes(self.num_nodes, self.extra_args)
        self.start_node(0)

    def run_test(self):
        miner = self.nodes[0]
        compact_node = self.nodes[1]

        self.log.info("Mine the assumevalid target block")
        blocks = self.generate(miner, 3, sync_fun=self.no_op)
        assumed_valid_block = blocks[1]

        self.log.info("Compact chainstate after connecting the assumevalid block")
        self.start_node(1, extra_args=NODE_ARGS + [f"-assumevalid={assumed_valid_block}"])
        assert_equal(compact_node.getblockchaininfo()["initialblockdownload"], True)
        with compact_node.assert_debug_log(COMPACTION_TRIGGER_MSGS, timeout=30):
            self.connect_nodes(0, 1)
            self.sync_blocks([miner, compact_node])
            compact_node.syncwithvalidationinterfacequeue()
        assert_equal(compact_node.getbestblockhash(), miner.getbestblockhash())

        self.log.info("Do not compact again after the assumevalid block has been connected")
        with compact_node.assert_debug_log([], unexpected_msgs=[FORCE_FLUSH_MSG, *COMPACTION_MSGS]):
            self.generate(compact_node, 1, sync_fun=self.no_op)
            compact_node.syncwithvalidationinterfacequeue()

        self.log.info("Do not compact again after a stale restart exits IBD")
        tip_time = compact_node.getblock(compact_node.getbestblockhash())["time"]
        self.restart_node(1, extra_args=NODE_ARGS + [f"-assumevalid={assumed_valid_block}", f"-mocktime={tip_time + STALE_TIP_AGE}"])
        compact_node = self.nodes[1]
        assert_equal(compact_node.getblockchaininfo()["initialblockdownload"], True)
        with compact_node.assert_debug_log([IBD_EXIT_MSG], unexpected_msgs=[FORCE_FLUSH_MSG, *COMPACTION_MSGS], timeout=30):
            self.generate(compact_node, 1, sync_fun=self.no_op)
            compact_node.syncwithvalidationinterfacequeue()
        assert_equal(compact_node.getblockchaininfo()["initialblockdownload"], False)


if __name__ == "__main__":
    CompactChainstateTest(__file__).main()
