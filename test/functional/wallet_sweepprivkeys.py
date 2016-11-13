#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test sweepprivkeys RPC."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import get_generate_key


class WalletSweepPrivKeysTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        self.generate(node, COINBASE_MATURITY + 1)

        self.log.info("Reject invalid and unfunded private keys")
        assert_raises_rpc_error(-5, "Invalid private key encoding", node.sweepprivkeys, {"privkeys": ["not-a-wif"]})
        empty_key = get_generate_key()
        assert_raises_rpc_error(-6, "No value to sweep", node.sweepprivkeys, {"privkeys": [empty_key.privkey]})

        self.log.info("Sweep a funded P2PKH key into the local wallet")
        funded_key = get_generate_key()
        node.sendtoaddress(funded_key.p2pkh_addr, Decimal("1"))
        self.generate(node, 1)

        sweep_txid = node.sweepprivkeys({"privkeys": [funded_key.privkey], "label": "swept"})
        assert_equal(sweep_txid in node.getrawmempool(), True)
        sweep_tx = node.getrawtransaction(sweep_txid, True)
        assert_equal(len(sweep_tx["vin"]), 1)
        assert_equal(len(sweep_tx["vout"]), 1)
        assert_greater_than(sweep_tx["vout"][0]["value"], Decimal("0.999"))

        self.generate(node, 1)
        wallet_tx = node.gettransaction(sweep_txid)
        receive_details = [detail for detail in wallet_tx["details"] if detail["category"] == "receive"]
        assert_equal(len(receive_details), 1)
        assert_equal(receive_details[0]["label"], "swept")
        assert_greater_than(receive_details[0]["amount"], Decimal("0.999"))

        assert_raises_rpc_error(-6, "No value to sweep", node.sweepprivkeys, {"privkeys": [funded_key.privkey]})


if __name__ == '__main__':
    WalletSweepPrivKeysTest(__file__).main()
