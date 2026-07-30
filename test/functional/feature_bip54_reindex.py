#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BIP54 validation during chainstate and full reindex."""

from test_framework.blocktools import (
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class Bip54ReindexTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def assert_deployment(self, *, active):
        deployments = self.nodes[0].getdeploymentinfo()["deployments"]
        if not active:
            assert_equal(deployments["consensuscleanup"], {
                "type": "bip9",
                "bip9": {
                    "start_time": -2,
                    "timeout": 9223372036854775807,
                    "min_activation_height": 0,
                    "status": "failed",
                    "since": 0,
                    "status_next": "failed",
                },
                "active": False,
            })
            assert "testdummy" not in deployments
        else:
            assert_equal(deployments["consensuscleanup"]["active"], True)

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Create a BIP54-valid chain")
        self.assert_deployment(active=True)
        valid_tip = self.generate(node, 1)[0]
        valid_height = node.getblockcount()

        self.log.info("Disable BIP54 and accept a legacy coinbase")
        self.restart_node(0, extra_args=[
            "-vbparams=consensuscleanup:-2:9223372036854775807",
            "-vbparams=testdummy:-2:9223372036854775807",
        ])
        self.assert_deployment(active=False)
        block = create_block(tmpl=node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS))
        block.vtx[0].nLockTime -= 1
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        assert_equal(node.submitblock(block.serialize().hex()), None)
        invalid_tip = block.hash_hex
        invalid_height = valid_height + 1
        assert_equal(node.getbestblockhash(), invalid_tip)
        assert_equal(node.getblockcount(), invalid_height)

        self.log.info("Restore active BIP54 without reindexing")
        self.restart_node(0, extra_args=[])
        self.assert_deployment(active=True)
        assert_equal(node.getbestblockhash(), invalid_tip)
        assert_equal(node.getblockcount(), invalid_height)

        self.log.info("Reindex the chainstate with active BIP54")
        self.restart_node(0, extra_args=["-reindex-chainstate"])
        self.assert_deployment(active=True)
        assert_equal(node.getbestblockhash(), invalid_tip)
        assert_equal(node.getblockcount(), invalid_height)

        self.log.info("Fully reindex with active BIP54")
        with node.assert_debug_log(["bad-cb-locktime"]):
            self.restart_node(0, extra_args=["-reindex"])
        self.assert_deployment(active=True)
        assert_equal(node.getbestblockhash(), valid_tip)
        assert_equal(node.getblockcount(), valid_height)


if __name__ == "__main__":
    Bip54ReindexTest(__file__).main()
