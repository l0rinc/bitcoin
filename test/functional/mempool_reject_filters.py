#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Knots rejectparasites and rejecttokens mempool policy."""

from test_framework.messages import CTxOut
from test_framework.script import (
    CScript,
    OP_0,
    OP_13,
    OP_FALSE,
    OP_RETURN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


OLGA_HEADER = CScript([OP_0, bytes.fromhex("003e7374616d703a000000000000000000000000000000000000000000000000")])


class RejectFiltersTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def create_filter_tx(self, *, extra_outputs, locktime=0):
        tx = self.wallet.create_self_transfer()["tx"]
        tx.nLockTime = locktime
        for output in extra_outputs:
            tx.vout[0].nValue -= output.nValue
            tx.vout.append(output)
        tx.rehash()
        return tx

    def assert_test_accept(self, tx, *, allowed, reject_reason=None):
        res = self.nodes[0].testmempoolaccept([tx.serialize().hex()], maxfeerate=0)[0]
        assert_equal(res["allowed"], allowed)
        if reject_reason is not None:
            assert_equal(res["reject-reason"], reject_reason)

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        self.log.info("-rejectparasites rejects cat21 locktime transactions")
        parasite_tx = self.create_filter_tx(
            extra_outputs=[CTxOut(0, CScript([OP_RETURN]))],
            locktime=21,
        )
        self.assert_test_accept(parasite_tx, allowed=True)
        self.restart_node(0, ["-rejectparasites=1", "-persistmempool=0"])
        self.assert_test_accept(parasite_tx, allowed=False, reject_reason="parasite-cat21")
        non_cat21_tx = self.create_filter_tx(
            extra_outputs=[CTxOut(0, CScript([OP_RETURN]))],
            locktime=0,
        )
        self.assert_test_accept(non_cat21_tx, allowed=True)

        self.log.info("-rejecttokens rejects Runes-style OP_RETURN transactions")
        runes_tx = self.create_filter_tx(
            extra_outputs=[CTxOut(0, CScript([OP_RETURN, OP_13, OP_FALSE]))],
        )
        self.restart_node(0, ["-persistmempool=0"])
        self.assert_test_accept(runes_tx, allowed=True)
        self.restart_node(0, ["-rejecttokens=1", "-persistmempool=0"])
        self.assert_test_accept(runes_tx, allowed=False, reject_reason="tokens-runes")

        self.log.info("-rejecttokens rejects OLGA-style P2WSH-looking outputs")
        olga_tx = self.create_filter_tx(
            extra_outputs=[
                CTxOut(1_000, OLGA_HEADER),
                CTxOut(1_000, OLGA_HEADER),
            ],
        )
        self.assert_test_accept(olga_tx, allowed=False, reject_reason="tokens-olga")
        self.restart_node(0, ["-persistmempool=0"])
        self.assert_test_accept(olga_tx, allowed=True)


if __name__ == "__main__":
    RejectFiltersTest(__file__).main()
