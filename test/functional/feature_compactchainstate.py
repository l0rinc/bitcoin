#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test scheduled chainstate compaction."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


DEBUG_ARGS = ["-debug=coindb"]
FORCE_FLUSH_MSG = "Writing chainstate to disk: flush mode=FORCE_FLUSH"
IBD_EXIT_MSG = "Leaving InitialBlockDownload"
COMPACTION_MSGS = [
    "Starting scheduled chainstate database compaction",
    "Finished scheduled chainstate database compaction",
]
COMPACTION_TRIGGER_MSGS = [IBD_EXIT_MSG, FORCE_FLUSH_MSG, *COMPACTION_MSGS]


class CompactChainstateTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [DEBUG_ARGS]

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Compact chainstate after leaving IBD")
        assert_equal(node.getblockchaininfo()["initialblockdownload"], True)
        with node.assert_debug_log(COMPACTION_TRIGGER_MSGS, timeout=30):
            self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(node.getblockchaininfo()["initialblockdownload"], False)

        for reindex_arg in ["-reindex-chainstate", "-reindex"]:
            self.log.info(f"Compact after block import during {reindex_arg}")
            with node.assert_debug_log(COMPACTION_TRIGGER_MSGS, timeout=30):
                self.restart_node(0, extra_args=[*DEBUG_ARGS, reindex_arg])
            assert_equal(node.getblockchaininfo()["initialblockdownload"], False)


if __name__ == "__main__":
    CompactChainstateTest(__file__).main()
