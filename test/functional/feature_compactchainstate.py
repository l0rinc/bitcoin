#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test chainstate compaction after leaving IBD."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


MAX_TIP_AGE = 24 * 60 * 60
DEBUG_ARGS = ["-debug=coindb"]


def chainstate_compaction_msgs(node):
    chainstate_path = node.chain_path / "chainstate"
    return [
        f"Starting chainstate database compaction of {chainstate_path}",
        f"Finished chainstate database compaction of {chainstate_path}",
    ]


class CompactChainstateTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [DEBUG_ARGS.copy()]

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        node = self.nodes[0]
        compaction_msgs = chainstate_compaction_msgs(node)
        force_flush_msg = "Writing chainstate to disk: flush mode=FORCE_FLUSH"

        self.log.info("Compact chainstate after leaving IBD")
        assert_equal(node.getblockchaininfo()["initialblockdownload"], True)
        with node.assert_debug_log(compaction_msgs + ["Leaving InitialBlockDownload", force_flush_msg]):
            self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(node.getblockchaininfo()["initialblockdownload"], False)

        with node.assert_debug_log([], unexpected_msgs=compaction_msgs):
            self.generate(node, 1, sync_fun=self.no_op)

        self.log.info("Do not compact again after a restart and a second IBD exit")
        stale_time = node.getblockheader(node.getbestblockhash())["time"] + 2 * MAX_TIP_AGE
        self.restart_node(0, extra_args=[*DEBUG_ARGS, f"-mocktime={stale_time}"])
        assert_equal(node.getblockchaininfo()["initialblockdownload"], True)
        with node.assert_debug_log(["Leaving InitialBlockDownload"], unexpected_msgs=compaction_msgs + [force_flush_msg]):
            self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(node.getblockchaininfo()["initialblockdownload"], False)


if __name__ == "__main__":
    CompactChainstateTest(__file__).main()
