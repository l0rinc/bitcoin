#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet state when a persisted transaction package is filtered on reload."""

from decimal import Decimal
import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, find_vout_for_address


class MempoolPersistWalletEvictionTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-walletbroadcast=0"]]
        self.uses_wallet = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    @staticmethod
    def wallet_tx_state(wallet, txid):
        tx = wallet.gettransaction(txid, verbose=True)
        return {
            key: tx.get(key, False) if key == "abandoned" else tx[key]
            for key in (
                "txid",
                "wtxid",
                "confirmations",
                "trusted",
                "abandoned",
                "walletconflicts",
                "mempoolconflicts",
                "time",
                "timereceived",
                "details",
            )
        }

    def run_test(self):
        node = self.nodes[0]
        wallet = node.get_wallet_rpc(self.default_wallet_name)
        self.generate(node, 101)

        parent_address = wallet.getnewaddress()
        parent_txid = wallet.sendtoaddress(parent_address, Decimal("1"), fee_rate=Decimal("2"))
        parent_hex = wallet.gettransaction(parent_txid)["hex"]
        node.sendrawtransaction(parent_hex)
        parent_vout = find_vout_for_address(node, parent_txid, parent_address)

        child_address = wallet.getnewaddress()
        child_txid = wallet.send(
            outputs=[{child_address: Decimal("0.5")}],
            fee_rate=Decimal("2"),
            options={
                "inputs": [{"txid": parent_txid, "vout": parent_vout}],
                "add_inputs": False,
            },
        )["txid"]
        child_hex = wallet.gettransaction(child_txid)["hex"]
        node.sendrawtransaction(child_hex)
        node.syncwithvalidationinterfacequeue()

        txids = [parent_txid, child_txid]
        assert_equal(set(node.getrawmempool()), set(txids))
        before_state = {txid: self.wallet_tx_state(wallet, txid) for txid in txids}
        before_balances = wallet.getbalances()["mine"]
        assert_equal([wallet.gettransaction(txid)["confirmations"] for txid in txids], [0, 0])

        mempool_path = os.path.join(node.chain_path, "mempool.dat")
        node.savemempool()
        backup_path = os.path.join(self.options.tmpdir, "mempool-wallet-eviction.dat")
        shutil.copyfile(mempool_path, backup_path)

        self.log.info("Reload the persisted package above the relay-fee policy")
        self.restart_node(0, extra_args=["-minrelaytxfee=0.0001"])
        wallet = node.get_wallet_rpc(self.default_wallet_name)
        assert node.getmempoolinfo()["loaded"]
        assert_equal(node.getrawmempool(), [])

        for txid in txids:
            state = self.wallet_tx_state(wallet, txid)
            assert_equal(state["txid"], before_state[txid]["txid"])
            assert_equal(state["wtxid"], before_state[txid]["wtxid"])
            assert_equal(state["confirmations"], 0)
            assert_equal(state["abandoned"], False)
            assert_equal(state["walletconflicts"], [])
            assert_equal(state["mempoolconflicts"], [])

        evicted_balances = wallet.getbalances()["mine"]
        assert_equal(evicted_balances["untrusted_pending"], Decimal("0E-8"))
        assert_equal(
            evicted_balances["trusted"]
            + evicted_balances["untrusted_pending"]
            + evicted_balances["nonmempool"],
            wallet.getbalance(),
        )
        assert child_txid not in {utxo["txid"] for utxo in wallet.listunspent(0)}

        self.stop_node(0)
        shutil.copyfile(backup_path, mempool_path)
        self.start_node(0, extra_args=["-minrelaytxfee=0.00001"])
        wallet = node.get_wallet_rpc(self.default_wallet_name)
        assert node.getmempoolinfo()["loaded"]
        assert_equal(set(node.getrawmempool()), set(txids))
        assert_equal(
            {txid: self.wallet_tx_state(wallet, txid) for txid in txids},
            before_state,
        )
        assert_equal(wallet.getbalances()["mine"], before_balances)

        os.remove(backup_path)


if __name__ == "__main__":
    MempoolPersistWalletEvictionTest(__file__).main()
