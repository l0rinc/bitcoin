#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test unknown witness output policy."""

from test_framework.messages import CTxOut
from test_framework.script import (
    CScript,
    OP_2,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

FUTURE_WITNESS_OUTPUT_VALUE = 100_000
FUTURE_WITNESS_SCRIPT = CScript([OP_2, b"\x01" * 32])


class AcceptUnknownWitnessTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            [],
            ["-acceptunknownwitness=0"],
        ]

    def create_future_witness_tx(self):
        tx = self.wallet.create_self_transfer(fee_rate=0)["tx"]
        tx.vout[0].nValue -= FUTURE_WITNESS_OUTPUT_VALUE + 1_000
        tx.vout.append(CTxOut(FUTURE_WITNESS_OUTPUT_VALUE, FUTURE_WITNESS_SCRIPT))
        tx.rehash()
        return tx

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        self.log.info("Future witness outputs are accepted by default")
        default_tx = self.create_future_witness_tx()
        default_txid = self.wallet.sendrawtransaction(from_node=self.nodes[0], tx_hex=default_tx.serialize().hex())
        assert default_txid in self.nodes[0].getrawmempool()
        self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks)
        self.wallet.rescan_utxos()

        self.log.info("-acceptunknownwitness=0 rejects future witness outputs from mempool")
        policy_tx = self.create_future_witness_tx()
        policy_tx_hex = policy_tx.serialize().hex()
        result = self.nodes[1].testmempoolaccept([policy_tx_hex], maxfeerate=0)[0]
        assert_equal(result["allowed"], False)
        assert_equal(result["reject-reason"], "scriptpubkey-unknown-witnessversion")

        self.log.info("The same transaction remains valid in a block")
        policy_txid = self.wallet.sendrawtransaction(from_node=self.nodes[0], tx_hex=policy_tx_hex)
        block_hash = self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks)[0]
        assert_equal(self.nodes[1].getbestblockhash(), block_hash)
        assert policy_txid in self.nodes[1].getblock(block_hash)["tx"]


if __name__ == "__main__":
    AcceptUnknownWitnessTest(__file__).main()
