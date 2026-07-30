#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BIP54 deployment state and validation across reorgs."""

from test_framework.blocktools import (
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    MAX_SEQUENCE_NONFINAL,
    SEQUENCE_FINAL,
    tx_from_hex,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


DEPLOYMENT_PERIOD = 144
CONSENSUS_CLEANUP_BIT = 3
CONSENSUS_CLEANUP_GBT_RULE = "!consensuscleanup"


class Bip54ReorgTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        deployment_arg = "-vbparams=consensuscleanup:0:9223372036854775807"
        # Reorging hundreds of blocks across three nodes makes the per-connect block index
        # walk quadratic; keep it periodic rather than on every connect.
        args = [deployment_arg, "-checkblockindex=100"]
        self.extra_args = [
            args,
            args + ["-blockversion=4"],
            args,
        ]

    def assert_deployment(self, node, *, status, active):
        deployment = node.getdeploymentinfo()["deployments"]["consensuscleanup"]
        assert_equal(deployment["active"], active)
        assert_equal(deployment["bip9"]["status"], status)
        assert_equal(deployment["bip9"]["status_next"], status)
        rules = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)["rules"]
        assert_equal(CONSENSUS_CLEANUP_GBT_RULE in rules, active)

    def assert_legacy_coinbase(self, node, block_hash, height):
        block = node.getblock(block_hash, 2)
        assert_equal(block["height"], height)
        assert_equal(block["tx"][0]["locktime"], 0)
        assert_equal(block["tx"][0]["vin"][0]["sequence"], SEQUENCE_FINAL)

    def assert_compliant_coinbase(self, node, block_hash, height):
        block = node.getblock(block_hash, 2)
        assert_equal(block["height"], height)
        assert_equal(block["tx"][0]["locktime"], height - 1)
        assert_equal(block["tx"][0]["vin"][0]["sequence"], MAX_SEQUENCE_NONFINAL)
        coinbase = tx_from_hex(block["tx"][0]["hex"])
        assert len(coinbase.serialize_without_witness()) != 64

    def run_test(self):
        node0, node1, node2 = self.nodes

        self.log.info("Fork from a common tip in the started deployment state")
        self.generate(node0, DEPLOYMENT_PERIOD)
        common_tip = node0.getbestblockhash()
        assert_equal(node0.getblockcount(), DEPLOYMENT_PERIOD)
        assert node0.getblockheader(common_tip)["version"] & (1 << CONSENSUS_CLEANUP_BIT)
        for node in self.nodes:
            assert_equal(node.getbestblockhash(), common_tip)
            self.assert_deployment(node, status="started", active=False)

        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(1, 2)
        self.connect_nodes(0, 2)

        self.log.info("Activate BIP54 on the signaling node0/node2 branch")
        self.generate(node2, 2 * DEPLOYMENT_PERIOD, sync_fun=self.no_op)
        self.sync_blocks([node0, node2])
        active_height = 3 * DEPLOYMENT_PERIOD
        active_tip = node0.getbestblockhash()
        assert_equal(node0.getblockcount(), active_height)
        assert_equal(node2.getbestblockhash(), active_tip)
        self.assert_deployment(node0, status="active", active=True)
        self.assert_deployment(node2, status="active", active=True)
        self.assert_compliant_coinbase(node0, active_tip, active_height)

        self.log.info("Build a longer non-signaling branch with a legacy coinbase")
        legacy_height = DEPLOYMENT_PERIOD + 1
        legacy_coinbase = create_coinbase(legacy_height)
        legacy_coinbase.nLockTime = 0
        legacy_coinbase.vin[0].nSequence = SEQUENCE_FINAL
        assert len(legacy_coinbase.serialize_without_witness()) != 64

        template = node1.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_equal(template["height"], legacy_height)
        assert_equal(template["version"], 4)
        assert CONSENSUS_CLEANUP_GBT_RULE not in template["rules"]
        legacy_block = create_block(coinbase=legacy_coinbase, tmpl=template)
        legacy_block.solve()
        assert_equal(node1.submitblock(legacy_block.serialize().hex()), None)
        legacy_hash = legacy_block.hash_hex
        assert_equal(node1.getbestblockhash(), legacy_hash)
        self.assert_legacy_coinbase(node1, legacy_hash, legacy_height)

        self.generate(node1, 2 * DEPLOYMENT_PERIOD, sync_fun=self.no_op)
        old_height = active_height + 1
        old_tip = node1.getbestblockhash()
        assert_equal(node1.getblockcount(), old_height)
        assert_equal(node1.getblockheader(old_tip)["version"], 4)
        self.assert_deployment(node1, status="started", active=False)
        assert_equal(node1.getblockhash(legacy_height), legacy_hash)

        self.log.info("Keep a still-longer active branch private on node2")
        self.disconnect_nodes(0, 2)
        self.generate(node2, 2, sync_fun=self.no_op)
        final_active_height = old_height + 1
        final_active_tip = node2.getbestblockhash()
        assert_equal(node2.getblockcount(), final_active_height)
        self.assert_deployment(node2, status="active", active=True)
        self.assert_compliant_coinbase(node2, final_active_tip, final_active_height)

        self.log.info("Reorg node0 from its active branch to the longer old-rules branch")
        self.connect_nodes(0, 1)
        self.sync_blocks([node0, node1])
        assert_equal(node0.getbestblockhash(), old_tip)
        assert_equal(node0.getblockcount(), old_height)
        self.assert_deployment(node0, status="started", active=False)
        assert_equal(node0.getblockhash(legacy_height), legacy_hash)
        self.assert_legacy_coinbase(node0, legacy_hash, legacy_height)
        tips = {tip["hash"]: tip for tip in node0.getchaintips()}
        assert_equal(tips[old_tip]["status"], "active")
        assert_equal(tips[active_tip]["status"], "valid-fork")

        self.log.info("Reorg node0 back to node2's longer active branch")
        self.disconnect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.sync_blocks([node0, node2])
        assert_equal(node0.getbestblockhash(), final_active_tip)
        assert_equal(node0.getblockcount(), final_active_height)
        self.assert_deployment(node0, status="active", active=True)
        self.assert_compliant_coinbase(node0, final_active_tip, final_active_height)
        assert node0.getblockhash(legacy_height) != legacy_hash
        tips = {tip["hash"]: tip for tip in node0.getchaintips()}
        assert_equal(tips[final_active_tip]["status"], "active")
        assert_equal(tips[old_tip]["status"], "valid-fork")


if __name__ == "__main__":
    Bip54ReorgTest(__file__).main()
