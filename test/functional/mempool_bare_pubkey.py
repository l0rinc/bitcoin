#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bare pubkey output policy interaction with output-size limits."""

from test_framework.messages import CTxOut
from test_framework.script_util import key_to_p2pk_script
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import generate_keypair


class BarePubkeyPolicyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def create_bare_pubkey_tx(self):
        _, pubkey = generate_keypair(compressed=True)
        p2pk_script = key_to_p2pk_script(pubkey)
        assert_equal(len(p2pk_script), 35)

        tx = self.wallet.create_self_transfer()["tx"]
        tx.vout[0].nValue -= 1_000
        tx.vout.append(CTxOut(1_000, p2pk_script))
        tx.rehash()
        return tx

    def assert_output_size_rejected(self, tx):
        res = self.nodes[0].testmempoolaccept([tx.serialize().hex()], maxfeerate=0)[0]
        assert_equal(res["allowed"], False)
        assert_equal(res["reject-reason"], "bad-txns-vout-script-toolarge")

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        tx = self.create_bare_pubkey_tx()

        self.log.info("Bare pubkey outputs are rejected by the output-size rule by default")
        self.assert_output_size_rejected(tx)

        self.log.info("-permitbarepubkey=1 does not override output-size rejection")
        self.restart_node(0, ["-permitbarepubkey=1", "-persistmempool=0"])
        self.assert_output_size_rejected(tx)

        self.log.info("-acceptnonstdtxn=1 does not override output-size rejection")
        self.restart_node(0, ["-acceptnonstdtxn=1", "-persistmempool=0"])
        self.assert_output_size_rejected(tx)

        self.log.info("The same transaction is still valid in a block before RDTS activation")
        self.generateblock(self.nodes[0], output=self.wallet.get_address(), transactions=[tx.serialize().hex()])


if __name__ == "__main__":
    BarePubkeyPolicyTest(__file__).main()
