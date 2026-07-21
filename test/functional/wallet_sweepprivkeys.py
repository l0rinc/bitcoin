#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test sweepprivkeys RPC."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.descriptors import descsum_create
from test_framework.key import ECKey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import (
    bytes_to_wif,
    get_generate_key,
)


class WalletSweepPrivKeysTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def assert_sweep(self, key, label):
        node = self.nodes[0]
        sweep_txid = node.sweepprivkeys(privkeys=[key.privkey], label=label)
        assert_equal(sweep_txid in node.getrawmempool(), True)

        sweep_tx = node.getrawtransaction(sweep_txid, True)
        assert_equal(len(sweep_tx["vin"]), 1)
        assert_equal(len(sweep_tx["vout"]), 1)
        assert_greater_than(sweep_tx["vout"][0]["value"], Decimal("0.999"))

        wallet_tx = node.gettransaction(sweep_txid)
        receive_details = [detail for detail in wallet_tx["details"] if detail["category"] == "receive"]
        assert_equal(len(receive_details), 1)
        assert_equal(receive_details[0]["label"], label)
        assert_greater_than(receive_details[0]["amount"], Decimal("0.999"))

        return sweep_txid

    def run_test(self):
        node = self.nodes[0]
        self.generate(node, COINBASE_MATURITY + 1)

        self.log.info("Reject invalid and unfunded private keys")
        assert_raises_rpc_error(-5, "Invalid private key encoding", node.sweepprivkeys, {"privkeys": ["not-a-wif"]})
        empty_key = get_generate_key()
        assert_raises_rpc_error(-6, "No value to sweep", node.sweepprivkeys, {"privkeys": [empty_key.privkey]})

        # This test is not meant to test fee estimation and we'd like
        # to be sure all txs are sent at a consistent desired feerate
        self.tx_feerate = max(self.nodes[0].getnetworkinfo()['relayfee'], self.nodes[0].getwalletinfo()['mintxfee']) * 2
        node.settxfee(self.tx_feerate)

        self.log.info("Sweep an unconfirmed P2PKH output from the mempool")
        mempool_key = get_generate_key()
        node.sendtoaddress(mempool_key.p2pkh_addr, Decimal("1"))
        mempool_sweep_txid = self.assert_sweep(mempool_key, "swept from mempool")
        self.generate(node, 1)
        assert_equal(node.gettransaction(mempool_sweep_txid)["confirmations"], 1)
        assert_raises_rpc_error(-6, "No value to sweep", node.sweepprivkeys, {"privkeys": [mempool_key.privkey]})

        self.log.info("Sweep a confirmed P2PKH output from the UTXO set")
        confirmed_key = get_generate_key()
        node.sendtoaddress(confirmed_key.p2pkh_addr, Decimal("1"))
        self.generate(node, 1)
        confirmed_sweep_txid = self.assert_sweep(confirmed_key, "swept from chain")
        self.generate(node, 1)
        assert_equal(node.gettransaction(confirmed_sweep_txid)["confirmations"], 1)
        assert_raises_rpc_error(-6, "No value to sweep", node.sweepprivkeys, {"privkeys": [confirmed_key.privkey]})

        self.log.info("Sweep confirmed P2WPKH, P2SH-P2WPKH, and P2TR outputs")
        for desc_template, label in (
            ("wpkh({})", "swept P2WPKH"),
            ("sh(wpkh({}))", "swept P2SH-P2WPKH"),
            ("tr({})", "swept P2TR"),
        ):
            eckey = ECKey()
            eckey.generate(compressed=True)
            privkey = bytes_to_wif(eckey.get_bytes(), compressed=True)
            address = node.deriveaddresses(descsum_create(desc_template.format(privkey)))[0]
            node.sendtoaddress(address, Decimal("1"))
            self.generate(node, 1)
            sweep_txid = node.sweepprivkeys(privkeys=[privkey], label=label)
            assert_equal(sweep_txid in node.getrawmempool(), True)
            self.generate(node, 1)
            assert_equal(node.gettransaction(sweep_txid)["confirmations"], 1)
            assert_raises_rpc_error(-6, "No value to sweep", node.sweepprivkeys, {"privkeys": [privkey]})


if __name__ == '__main__':
    WalletSweepPrivKeysTest(__file__).main()
