#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Knots' mempool statistics RPC."""

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)
from test_framework.wallet import MiniWallet


class RPCMempoolStatsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            [],
            ["-statsenable=1"],
        ]

    def setup_network(self):
        self.setup_nodes()

    def assert_empty_stats(self, node):
        stats = node.getmempoolstats()
        assert_equal(stats["time_from"], 0)
        assert_equal(stats["time_to"], 0)
        assert_equal(stats["samples"], [])

    def run_test(self):
        self.log.info("Check default bitcoind mempool statistics are disabled")
        self.assert_empty_stats(self.nodes[0])
        self.nodes[0].setmocktime(int(time.time()) + 10)
        MiniWallet(self.nodes[0]).send_self_transfer(from_node=self.nodes[0])
        assert_equal(self.nodes[0].getmempoolinfo()["size"], 1)
        self.assert_empty_stats(self.nodes[0])

        self.log.info("Check -statsenable=1 records mempool samples")
        initial_stats = self.nodes[1].getmempoolstats()
        initial_sample_count = len(initial_stats["samples"])
        assert_greater_than_or_equal(initial_sample_count, 1)

        self.nodes[1].setmocktime(int(time.time()) + 20)
        MiniWallet(self.nodes[1]).send_self_transfer(from_node=self.nodes[1])
        assert_equal(self.nodes[1].getmempoolinfo()["size"], 1)

        stats = self.nodes[1].getmempoolstats()
        assert_equal(len(stats["samples"]), initial_sample_count + 1)
        sample_time_delta, tx_count, dynamic_usage, min_fee_per_k = stats["samples"][-1]
        assert_greater_than(sample_time_delta, 0)
        assert_equal(tx_count, 1)
        assert_greater_than(dynamic_usage, 0)
        assert_greater_than(min_fee_per_k, 0)


if __name__ == "__main__":
    RPCMempoolStatsTest(__file__).main()
