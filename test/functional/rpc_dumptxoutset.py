#!/usr/bin/env python3
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the generation of UTXO snapshots using `dumptxoutset`."""

import hashlib
import os
import subprocess
import sys

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    sha256sum_file,
)


ALL_ASCII_FIELDS = ("txid", "vout", "value", "coinbase", "height", "scriptPubKey")


class DumptxoutsetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def test_dumptxoutset_with_fork(self):
        node = self.nodes[0]
        tip = node.getbestblockhash()
        target_height = node.getblockcount() - 10
        target_hash = node.getblockhash(target_height)

        # Create a fork of two blocks at the target height
        invalid_block = node.getblockhash(target_height + 1)
        node.invalidateblock(invalid_block)
        # Reset mocktime to not regenerate the same blockhash
        node.setmocktime(0)
        self.generate(node, 2)

        # Move back on to actual main chain
        node.reconsiderblock(invalid_block)
        self.wait_until(lambda: node.getbestblockhash() == tip)

        # Use dumptxoutset at the forked height
        out = node.dumptxoutset("txoutset_fork.dat", "rollback", {"rollback": target_height})

        # Verify the snapshot was created at the target height and not the fork tip
        assert_equal(out["base_height"], target_height)
        assert_equal(out["base_hash"], target_hash)

        # Cover the same case as above with an in-memory database
        out_mem = node.dumptxoutset("txoutset_fork_mem.dat", "rollback", {"rollback": target_height, "in_memory": True})
        assert_equal(out_mem["base_height"], target_height)
        assert_equal(out_mem["base_hash"], target_hash)

    def check_ascii_dump(self, path, out, params):
        fields = params.get("format") or ALL_ASCII_FIELDS
        separator = params.get("separator", ",")
        show_header = params.get("show_header", True)

        with open(path, encoding="utf-8") as f:
            lines = [line.rstrip("\n") for line in f]

        if show_header:
            assert_equal(lines.pop(0), "#(blockhash {} ) {}".format(out["base_hash"], separator.join(fields)))

        assert_equal(len(lines), out["coins_written"])
        for line in lines:
            assert_equal(len(line.split(separator)), len(fields))

    def test_dump_file(self, testname, params, expected_digest=None):
        node = self.nodes[0]

        self.log.info(testname)
        filename = testname + "_txoutset.dat"
        is_human_readable = params.get("format") is not None

        out = node.dumptxoutset(path=filename, type="latest", **params)
        expected_path = node.chain_path / filename

        assert expected_path.is_file()

        assert_equal(out["coins_written"], 100)
        assert_equal(out["base_height"], 100)
        assert_equal(out["path"], str(expected_path))
        # Blockhash should be deterministic based on mocked time.
        assert_equal(out["base_hash"], "220aee93f0f5409631f35488898258f0930952bd620063cb4d7d87f7c28a8f50")

        if is_human_readable:
            self.check_ascii_dump(expected_path, out, params)
        else:
            # UTXO snapshot hash should be deterministic based on mocked time.
            assert_equal(sha256sum_file(str(expected_path)).hex(), expected_digest)

        if {"format"} == set(params) - {"show_header", "separator"}:
            # Test backward compatibility with Knots 0.20.0-28.1 positional
            # format/show_header/separator arguments.
            def test_dump_file_compat(*args, **kwargs):
                os.replace(expected_path, node.chain_path / (filename + ".old"))
                out2 = node.dumptxoutset(filename, *args, **kwargs)
                assert_equal(out, out2)
                if is_human_readable:
                    self.check_ascii_dump(expected_path, out2, params)
                else:
                    assert_equal(sha256sum_file(str(expected_path)).hex(), expected_digest)

            test_dump_file_compat(params.get("format"), params.get("show_header"), params.get("separator"))
            test_dump_file_compat(params.get("format"), params.get("show_header"), separator=params.get("separator"))
            test_dump_file_compat(params.get("format"), show_header=params.get("show_header"), separator=params.get("separator"))

        assert_equal(out["txoutset_hash"], "771d773b5c27b6f35f598ce764652a2cf28fbc268341eb1827844e416c629c7d")
        assert_equal(out["nchaintx"], 101)

    def test_dump_fifo(self, expected_digest):
        if not hasattr(os, "mkfifo"):
            self.log.info("Skipping dumptxoutset named pipe test; FIFOs are unavailable")
            return

        self.log.info("Test dumptxoutset writing to a named pipe")
        node = self.nodes[0]
        fifo_path = node.chain_path / "fifo_txoutset.dat"
        incomplete_path = fifo_path.parent / (fifo_path.name + ".incomplete")
        os.mkfifo(fifo_path)

        reader = subprocess.Popen(
            [
                sys.executable,
                "-c",
                "import pathlib, sys; sys.stdout.buffer.write(pathlib.Path(sys.argv[1]).read_bytes())",
                str(fifo_path),
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        try:
            out = node.dumptxoutset(path=str(fifo_path), type="latest")
            fifo_bytes, stderr = reader.communicate(timeout=10)
        finally:
            if reader.poll() is None:
                reader.kill()
                reader.communicate()
            fifo_path.unlink()

        assert_equal(reader.returncode, 0)
        assert_equal(stderr, b"")
        assert not incomplete_path.exists()
        assert_equal(out["path"], str(fifo_path))
        assert_equal(out["coins_written"], 100)
        assert_equal(hashlib.sha256(fifo_bytes).hexdigest(), expected_digest)

    def run_test(self):
        """Test a trivial usage of the dumptxoutset RPC command."""
        node = self.nodes[0]
        mocktime = node.getblockheader(node.getblockhash(0))["time"] + 1
        node.setmocktime(mocktime)
        self.generate(node, COINBASE_MATURITY)

        self.test_dump_file("no_option", {}, "e8c59b1bc1f19061c67eb7a392f4ea17eea83af58646ea2909e270546699c36c")
        self.test_dump_fifo("e8c59b1bc1f19061c67eb7a392f4ea17eea83af58646ea2909e270546699c36c")
        self.test_dump_file("all_data", {"format": ()})
        self.test_dump_file("partial_data_1", {"format": ("txid",)})
        self.test_dump_file("partial_data_order", {"format": ("height", "vout")})
        self.test_dump_file("partial_data_double", {"format": ("scriptPubKey", "scriptPubKey")})
        self.test_dump_file("no_header", {"format": (), "show_header": False})
        self.test_dump_file("separator", {"format": (), "separator": ":"})
        self.test_dump_file("all_options", {"format": (), "show_header": False, "separator": ":"})

        self.log.info("Test that a path to an existing or invalid file will fail")
        assert_raises_rpc_error(-8, "no_option_txoutset.dat already exists", node.dumptxoutset, "no_option_txoutset.dat", "latest")
        invalid_path = node.datadir_path / "invalid" / "path"
        assert_raises_rpc_error(
            -8,
            "Couldn't open file {}.incomplete for writing".format(invalid_path),
            node.dumptxoutset,
            invalid_path,
            "latest",
        )

        self.log.info("Test that dumptxoutset with unknown dump type fails")
        assert_raises_rpc_error(
            -8,
            'Invalid snapshot type "bogus" specified. Please specify "rollback" or "latest"',
            node.dumptxoutset,
            "utxos.dat",
            "bogus",
        )

        self.log.info("Test that dumptxoutset with unknown ASCII dump field fails")
        assert_raises_rpc_error(
            -8,
            "unable to find item 'sample'",
            node.dumptxoutset,
            path="xxx",
            type="latest",
            format=["sample"],
        )

        self.log.info("Testing dumptxoutset with chain fork at target height")
        self.test_dumptxoutset_with_fork()


if __name__ == "__main__":
    DumptxoutsetTest(__file__).main()
