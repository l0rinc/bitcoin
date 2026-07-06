#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Knots' RBF policy option compatibility."""

import re

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class MempoolRBFOptionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 5
        self.extra_args = [
            [],
            ["-mempoolfullrbf=0"],
            ["-mempoolreplacement=0"],
            ["-mempoolreplacement=fee,optin", "-mempoolfullrbf=1"],
            ["-mempoolreplacement=0", "-mempoolfullrbf=1"],
        ]

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        self.log.info("Check RBF policy option interactions")
        assert_equal(self.nodes[0].getmempoolinfo()["rbf_policy"], "always")
        assert_equal(self.nodes[1].getmempoolinfo()["rbf_policy"], "optin")
        assert_equal(self.nodes[2].getmempoolinfo()["rbf_policy"], "never")
        assert_equal(self.nodes[3].getmempoolinfo()["rbf_policy"], "optin")
        assert_equal(self.nodes[4].getmempoolinfo()["rbf_policy"], "never")

        self.stop_node(
            4,
            expected_stderr=re.compile(
                "Warning: False mempoolreplacement option contradicts true mempoolfullrbf; disallowing all RBF",
                re.DOTALL,
            ),
        )


if __name__ == "__main__":
    MempoolRBFOptionsTest(__file__).main()
