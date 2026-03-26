#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test datacarrier functionality"""
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    MAX_OP_RETURN_RELAY,
)
from test_framework.script import (
    CScript,
    OP_1,
    OP_2DROP,
    OP_DROP,
    OP_RETURN,
    taproot_construct,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import TestNode
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal,
    JSONRPCException,
)
from test_framework.wallet import MiniWallet

from random import randbytes

CUSTOM_DATACARRIER_ARG = MAX_OP_RETURN_RELAY

class DataCarrierTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.extra_args = [
            ["-acceptnonstddatacarrier=1", "-datacarrierfullcount"], # default capped at MAX_OP_RETURN_RELAY
            ["-datacarrier=0"], # no relay of datacarrier
            ["-datacarrier=1", f"-datacarriersize={CUSTOM_DATACARRIER_ARG}"],
            ["-datacarrier=1", "-datacarriersize=2", "-acceptnonstddatacarrier=1", "-datacarrierfullcount"],
        ]

    def test_null_data_transaction(self, node: TestNode, data, success: bool) -> None:
        tx = self.wallet.create_self_transfer(fee_rate=0)["tx"]
        data = [] if data is None else [data]
        tx.vout.append(CTxOut(nValue=0, scriptPubKey=CScript([OP_RETURN] + data)))
        tx.vout[0].nValue -= tx.get_vsize()  # simply pay 1sat/vbyte fee

        tx_hex = tx.serialize().hex()

        if success:
            self.wallet.sendrawtransaction(from_node=node, tx_hex=tx_hex)
            assert tx.txid_hex in node.getrawmempool(True), f'{tx_hex} not in mempool'
        else:
            try:
                self.wallet.sendrawtransaction(from_node=node, tx_hex=tx_hex)
            except JSONRPCException as e:
                assert_equal(e.error["code"], -26)
                assert any(reason in e.error["message"] for reason in ["datacarrier", "scriptpubkey"]), e.error["message"]
            else:
                raise AssertionError(f"{tx_hex} unexpectedly accepted")

    def test_opnet_transaction(self, node: TestNode, success: bool) -> None:
        minimal_script = CScript([OP_2DROP, OP_DROP, b'op', OP_DROP, OP_1])
        internal_key = b'\x01' * 32
        tap = taproot_construct(internal_key, [("leaf", minimal_script), ("dummy", CScript([OP_1]))])
        leaf = tap.leaves["leaf"]
        control_block = bytes([leaf.version | tap.negflag]) + tap.internal_pubkey + leaf.merklebranch
        assert len(control_block) == 65

        utxo = self.wallet.get_utxo()
        funding_tx = CTransaction()
        funding_tx.vin = [CTxIn(COutPoint(int(utxo['txid'], 16), utxo['vout']))]
        funding_value = int(utxo['value'] * 100_000_000) - 1000
        funding_tx.vout = [CTxOut(funding_value, tap.scriptPubKey)]
        funding_tx.version = 2
        self.wallet.sign_tx(funding_tx)
        funding_tx.rehash()
        self.nodes[0].sendrawtransaction(funding_tx.serialize().hex())
        self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks)

        spend_tx = CTransaction()
        spend_tx.version = 2
        spend_tx.vin = [CTxIn(COutPoint(int(funding_tx.hash, 16), 0))]
        spend_tx.vout = [CTxOut(funding_value - 1000, tap.scriptPubKey)]
        spend_tx.wit.vtxinwit = [CTxInWitness()]
        spend_tx.wit.vtxinwit[0].scriptWitness.stack = [
            b'',                    # stack[0]: empty (minimises opnet bytes)
            b'',                    # stack[1]: cleared by OP_2DROP
            b'',                    # stack[2]: cleared by OP_2DROP
            bytes(minimal_script),  # stack[3]: tapscript containing \x02op
            control_block,          # stack[4]: control block (65 bytes)
        ]
        tx_hex = spend_tx.serialize().hex()

        if success:
            self.wallet.sendrawtransaction(from_node=node, tx_hex=tx_hex)
            assert spend_tx.rehash() in node.getrawmempool(True)
        else:
            assert_raises_rpc_error(-26, "txn-datacarrier-exceeded",
                                    self.wallet.sendrawtransaction, from_node=node, tx_hex=tx_hex)


    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        # Test that bare multisig is allowed by default. Do it here rather than create a new test for it.
        assert_equal(self.nodes[0].getmempoolinfo()["permitbaremultisig"], True)

        assert_equal(self.nodes[0].getmempoolinfo()["maxdatacarriersize"], MAX_OP_RETURN_RELAY)
        assert_equal(self.nodes[1].getmempoolinfo()["maxdatacarriersize"], 0)
        assert_equal(self.nodes[2].getmempoolinfo()["maxdatacarriersize"], CUSTOM_DATACARRIER_ARG)
        assert_equal(self.nodes[3].getmempoolinfo()["maxdatacarriersize"], 2)

        # By default and when explicitly set to 83,
        # only 80 bytes are used for data (+1 for OP_RETURN, +2 for the pushdata opcodes).
        custom_size_data = randbytes(CUSTOM_DATACARRIER_ARG - 3)
        too_long_data = randbytes(CUSTOM_DATACARRIER_ARG - 2)
        one_byte = randbytes(1)
        zero_bytes = randbytes(0)

        self.log.info("Testing null data transaction with default -datacarrier and -datacarriersize values.")
        self.test_null_data_transaction(node=self.nodes[0], data=custom_size_data, success=True)
        self.test_null_data_transaction(node=self.nodes[0], data=too_long_data, success=False)

        self.log.info("Testing a null data transaction with -datacarrier=false.")
        self.test_null_data_transaction(node=self.nodes[1], data=custom_size_data, success=False)

        self.log.info("Testing a null data transaction with a size larger than accepted by -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[2], data=too_long_data, success=False)

        self.log.info("Testing a null data transaction with a size equal to -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[2], data=custom_size_data, success=True)

        self.log.info("Testing a null data transaction with no data.")
        self.test_null_data_transaction(node=self.nodes[0], data=None, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=None, success=False)
        self.test_null_data_transaction(node=self.nodes[2], data=None, success=True)
        self.test_null_data_transaction(node=self.nodes[3], data=None, success=True)

        self.log.info("Testing a null data transaction with zero bytes of data.")
        self.test_null_data_transaction(node=self.nodes[0], data=zero_bytes, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=zero_bytes, success=False)
        self.test_null_data_transaction(node=self.nodes[2], data=zero_bytes, success=True)
        self.test_null_data_transaction(node=self.nodes[3], data=zero_bytes, success=True)

        self.log.info("Testing a null data transaction with one byte of data.")
        self.test_null_data_transaction(node=self.nodes[0], data=one_byte, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=one_byte, success=False)
        self.test_null_data_transaction(node=self.nodes[2], data=one_byte, success=True)
        self.test_null_data_transaction(node=self.nodes[3], data=one_byte, success=False)

        self.log.info("Testing an OPNet transaction (just pushing 'op') with default -datacarriersize.")
        self.test_opnet_transaction(node=self.nodes[0], success=True)

        self.log.info("Testing an OPNet transaction (just pushing 'op') with -datacarriersize=2.")
        self.test_opnet_transaction(node=self.nodes[3], success=False)


if __name__ == '__main__':
    DataCarrierTest(__file__).main()
