#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Test structured output from bench_bitcoin."""

import json
import os
import subprocess

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class BenchOutputTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 0
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_bitcoin_bench()

    def setup_network(self):
        pass

    def run_benchmark(self, filter_name, output_name, extra_args):
        output_path = os.path.join(self.options.tmpdir, output_name)
        cmd = self.get_binaries().bench_argv() + [
            f"-filter={filter_name}",
            "-min-time=1",
            f"-output-json={output_path}",
            f"-testdatadir={self.options.tmpdir}",
        ] + extra_args
        self.log.info(f"Starting: {' '.join(cmd)}")
        env = os.environ.copy()
        env["TMPDIR"] = self.options.tmpdir
        subprocess.run(cmd, check=True, env=env)

        with open(output_path, encoding="utf-8") as output_file:
            return json.load(output_file)["results"]

    def run_test(self):
        results = self.run_benchmark("MempoolCheckEphemeralSpends", "asymptote.json", ["-asymptote=10,20,40"])
        assert_equal([result["complexityN"] for result in results], [10, 20, 40])

        results = self.run_benchmark("MerkleRoot", "multiple-runs.json", [])
        assert_equal([result["name"] for result in results], ["MerkleRoot", "MerkleRootWithMutation"])


if __name__ == "__main__":
    BenchOutputTest(__file__).main()
