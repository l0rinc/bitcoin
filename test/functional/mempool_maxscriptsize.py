#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test maxscriptsize mempool policy."""

from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_2DROP,
    OP_TRUE,
)
from test_framework.script_util import (
    key_to_p2pkh_script,
    keys_to_multisig_script,
    script_to_p2wsh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import generate_keypair


class MaxScriptSizeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def create_wallet_spend(self, output_script):
        utxo = self.wallet.get_utxo()
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(utxo["txid"], 16), utxo["vout"]))]
        tx.vout = [CTxOut(int(utxo["value"] * COIN) - 1_000, output_script)]
        self.wallet.sign_tx(tx)
        tx.rehash()
        return tx

    def create_p2wsh_spend(self, fund, witness_stack, output_script=None):
        output_script = output_script or self.p2pkh_script
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(fund["txid"], 16), fund["sent_vout"]))]
        tx.wit.vtxinwit = [CTxInWitness()]
        tx.wit.vtxinwit[0].scriptWitness.stack = witness_stack
        tx.vout = [CTxOut(999_000, output_script)]
        tx.rehash()
        return tx

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        _, pubkey = generate_keypair(compressed=True)
        self.p2pkh_script = key_to_p2pkh_script(pubkey)

        self.log.info("Prepare confirmed P2WSH outputs for input and witness-size checks")
        true_script = CScript([OP_TRUE])
        drop_script = CScript([OP_2DROP, OP_TRUE])
        true_fund = self.wallet.send_to(
            from_node=node,
            scriptPubKey=script_to_p2wsh_script(true_script),
            amount=1_000_000,
        )
        drop_fund = self.wallet.send_to(
            from_node=node,
            scriptPubKey=script_to_p2wsh_script(drop_script),
            amount=1_000_000,
        )
        self.generate(node, 1)

        self.log.info("-maxscriptsize rejects oversized output scripts")
        bare_multisig_script = keys_to_multisig_script([
            generate_keypair(compressed=True)[1],
            generate_keypair(compressed=True)[1],
        ])
        assert len(bare_multisig_script) > 50
        self.restart_node(0, ["-maxscriptsize=50", "-permitbaremultisig=1", "-persistmempool=0"])
        node = self.nodes[0]
        tx_output = self.create_wallet_spend(bare_multisig_script)
        res = node.testmempoolaccept([tx_output.serialize().hex()], maxfeerate=0)[0]
        assert_equal(res["allowed"], False)
        assert_equal(res["reject-reason"], "scriptpubkey-size")

        self.log.info("-maxscriptsize rejects oversized spent scriptPubKeys")
        self.restart_node(0, ["-maxscriptsize=33", "-persistmempool=0"])
        node = self.nodes[0]
        tx_prevout = self.create_p2wsh_spend(true_fund, [bytes(true_script)])
        res = node.testmempoolaccept([tx_prevout.serialize().hex()], maxfeerate=0)[0]
        assert_equal(res["allowed"], False)
        assert_equal(res["reject-reason"], "bad-txns-input-script-size")

        self.log.info("-maxscriptsize rejects oversized aggregate witness stacks")
        self.restart_node(0, ["-maxscriptsize=100", "-persistmempool=0"])
        node = self.nodes[0]
        large_witness_stack = [b"a" * 60, b"b" * 60, bytes(drop_script)]
        tx_witness = self.create_p2wsh_spend(drop_fund, large_witness_stack)
        res = node.testmempoolaccept([tx_witness.serialize().hex()], maxfeerate=0)[0]
        assert_equal(res["allowed"], False)
        assert_equal(res["reject-reason"], "bad-witness-witness-size")

        self.log.info("-acceptnonstdtxn disables maxscriptsize unless explicitly overridden")
        self.restart_node(0, ["-acceptnonstdtxn=1", "-persistmempool=0"])
        node = self.nodes[0]
        res = node.testmempoolaccept([tx_witness.serialize().hex()], maxfeerate=0)[0]
        assert_equal(res["allowed"], True)


if __name__ == "__main__":
    MaxScriptSizeTest(__file__).main()
