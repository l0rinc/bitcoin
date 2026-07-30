#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test delayed BIP54 activation through a minimum activation height."""

from test_framework.blocktools import (
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    MAX_SEQUENCE_NONFINAL,
    SEQUENCE_FINAL,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


CONSENSUS_CLEANUP_BIT = 3
CONSENSUS_CLEANUP_GBT_RULE = "!consensuscleanup"
DEPLOYMENT_PERIOD = 144
MIN_ACTIVATION_HEIGHT = 4 * DEPLOYMENT_PERIOD


class Bip54ActivationDelayTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[
            f"-vbparams=consensuscleanup:0:9223372036854775807:{MIN_ACTIVATION_HEIGHT}",
            "-checkblockindex=100",
        ]]

    def assert_deployment(self, *, status, status_next, active):
        deployment = self.nodes[0].getdeploymentinfo()["deployments"]["consensuscleanup"]
        bip9 = deployment["bip9"]
        assert_equal(bip9["status"], status)
        assert_equal(bip9["status_next"], status_next)
        assert_equal(bip9["min_activation_height"], MIN_ACTIVATION_HEIGHT)
        assert_equal(deployment["active"], active)

        template = self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_equal(CONSENSUS_CLEANUP_GBT_RULE in template["rules"], active)
        if active:
            assert_equal(CONSENSUS_CLEANUP_GBT_RULE in template["vbavailable"], False)
            assert_raises_rpc_error(
                -8,
                "Support for 'consensuscleanup' rule requires explicit client support",
                self.nodes[0].getblocktemplate,
                {"rules": ["segwit"]},
            )
        else:
            available = status_next in ("started", "locked_in")
            assert_equal(CONSENSUS_CLEANUP_GBT_RULE in template["vbavailable"], available)
            if available:
                assert_equal(template["vbavailable"][CONSENSUS_CLEANUP_GBT_RULE], CONSENSUS_CLEANUP_BIT)
            unsupported = self.nodes[0].getblocktemplate({"rules": ["segwit"]})
            assert_equal(unsupported["version"] & (1 << CONSENSUS_CLEANUP_BIT), 0)

    def legacy_coinbase_block(self):
        node = self.nodes[0]
        template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        coinbase = create_coinbase(template["height"])
        coinbase.nLockTime = 0
        coinbase.vin[0].nSequence = SEQUENCE_FINAL
        block = create_block(coinbase=coinbase, tmpl=template)
        block.solve()
        return block

    def assert_compliant_coinbase(self, block_hash, height):
        block = self.nodes[0].getblock(block_hash, 2)
        assert_equal(block["height"], height)
        assert_equal(block["tx"][0]["locktime"], height - 1)
        assert_equal(block["tx"][0]["vin"][0]["sequence"], MAX_SEQUENCE_NONFINAL)

    def run_test(self):
        node = self.nodes[0]

        self.assert_deployment(status="defined", status_next="defined", active=False)

        self.log.info("Signal BIP54 and lock it in before the minimum activation height")
        self.generate(node, DEPLOYMENT_PERIOD - 1)
        self.assert_deployment(status="defined", status_next="started", active=False)

        self.generate(node, 1)
        self.assert_deployment(status="started", status_next="started", active=False)

        self.generate(node, DEPLOYMENT_PERIOD - 1)
        self.assert_deployment(status="started", status_next="locked_in", active=False)

        self.generate(node, 1)
        self.assert_deployment(status="locked_in", status_next="locked_in", active=False)

        self.generate(node, DEPLOYMENT_PERIOD)
        assert_equal(node.getblockcount(), 3 * DEPLOYMENT_PERIOD)
        self.assert_deployment(status="locked_in", status_next="locked_in", active=False)

        self.log.info("Accept a legacy coinbase while activation remains delayed")
        legacy_block = self.legacy_coinbase_block()
        assert_equal(node.submitblock(legacy_block.serialize().hex()), None)
        assert_equal(node.getbestblockhash(), legacy_block.hash_hex)
        self.assert_deployment(status="locked_in", status_next="locked_in", active=False)

        self.generate(node, MIN_ACTIVATION_HEIGHT - node.getblockcount() - 1)
        assert_equal(node.getblockcount(), MIN_ACTIVATION_HEIGHT - 1)
        self.assert_deployment(status="locked_in", status_next="active", active=True)

        self.log.info("Reject a legacy coinbase at the first active height")
        old_tip = node.getbestblockhash()
        active_bad_block = self.legacy_coinbase_block()
        assert_equal(node.submitblock(active_bad_block.serialize().hex()), "bad-cb-locktime")
        assert_equal(node.getbestblockhash(), old_tip)

        first_active_hash = self.generate(node, 1)[0]
        assert_equal(node.getblockcount(), MIN_ACTIVATION_HEIGHT)
        self.assert_deployment(status="active", status_next="active", active=True)
        self.assert_compliant_coinbase(first_active_hash, MIN_ACTIVATION_HEIGHT)


if __name__ == "__main__":
    Bip54ActivationDelayTest(__file__).main()
