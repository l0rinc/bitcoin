#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

"""Test wallet catch-up after restarting from an unclean shutdown."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)


class WalletChainReprocess(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Testing that all blocks are reprocessed by the wallet after an unclean shutdown")
        node = self.nodes[0]
        node.createwallet("target")
        wallet = node.get_wallet_rpc("target")

        # Mine 1 block to the wallet, 101 to somewhere else and flush the chainstate with a restart
        self.generatetoaddress(node, 1, wallet.getnewaddress(), sync_fun=self.no_op)
        self.generate(node, 101, sync_fun=self.no_op)
        self.restart_node(0)
        self.connect_nodes(0, 1)
        node.loadwallet("target")
        wallet = node.get_wallet_rpc("target")

        # Each block contains a wallet transaction spending the previous transaction
        # After restarting, the wallet must process these blocks again to restore confirmations
        txids = []
        for _ in range(100):
            res = wallet.sendall([wallet.getnewaddress()])
            assert_equal(res["complete"], True)
            txids.append(res["txid"])
            self.generate(node, 1, sync_fun=self.no_op)

        # Sync nodes, kill node 0, and restart
        self.sync_all()
        tip_height = node.getblockcount()
        wallet_height = wallet.getwalletinfo()["lastprocessedblock"]["height"]
        assert_equal(tip_height, wallet_height)
        wallet.unloadwallet()
        node.kill_process()
        self.start_node(0)
        restart_tip_height = node.getblockcount()
        assert_greater_than(tip_height, restart_tip_height)
        assert_greater_than(wallet_height, restart_tip_height)

        node.loadwallet("target")
        wallet = node.get_wallet_rpc("target")
        assert_greater_than(wallet_height, wallet.getwalletinfo()["lastprocessedblock"]["height"])

        # Reconnect after loading the wallet, then wait for the chain and wallet to catch up
        self.connect_nodes(0, 1)
        self.sync_all()
        assert_equal(wallet.getwalletinfo()["lastprocessedblock"]["height"], node.getblockcount())

        # Check every transaction is confirmed
        for txid in txids:
            assert_greater_than(wallet.gettransaction(txid)["confirmations"], 0)

            # None of these transactions can be abandoned either
            assert_raises_rpc_error(-5, "Transaction not eligible for abandonment", wallet.abandontransaction, txid)


if __name__ == '__main__':
    WalletChainReprocess(__file__).main()
