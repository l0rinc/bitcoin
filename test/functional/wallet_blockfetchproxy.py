#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test restoring a known descriptor by fetching matching pruned blocks."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)


class WalletBlockFetchProxyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            ["-fastprune", "-prune=1", "-blockfetchproxy", "-blockfilterindex=1", "-txindex"],
            ["-fastprune"],
        ]

    def setup_network(self):
        self.setup_nodes()
        self.connect_nodes(0, 1)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        source_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        funding_wallet = self.nodes[1].get_wallet_rpc(self.default_wallet_name)

        address = source_wallet.getnewaddress()
        descriptor = source_wallet.getaddressinfo(address)["desc"]
        payment_txid = funding_wallet.sendtoaddress(address, 1)
        payment_hash = self.generate(self.nodes[1], 1)[0]
        payment_block = self.nodes[1].getblock(payment_hash)
        payment_height = payment_block["height"]
        payment_time = payment_block["time"]
        spend_txid = source_wallet.sendall([funding_wallet.getnewaddress()])["txid"]
        self.sync_mempools()
        self.generate(self.nodes[1], 1)
        spend_hash = source_wallet.gettransaction(spend_txid)["blockhash"]

        self.log.info("Build enough history to prune the payment block")
        self.generate(self.nodes[1], 398)
        self.wait_until(lambda: all(index["synced"] for index in self.nodes[0].getindexinfo().values()))
        assert_greater_than(self.nodes[0].pruneblockchain(300), payment_height)

        self.log.info("Restore fully spent history for a known descriptor")
        self.nodes[0].createwallet("restored", disable_private_keys=True, blank=True)
        restored_wallet = self.nodes[0].get_wallet_rpc("restored")
        request_log = "Requesting block "
        requests_before = self.nodes[0].debug_log_path.read_text(encoding="utf-8").count(request_log)
        with self.nodes[0].assert_debug_log([
            f"Requesting block {payment_hash} from peer=",
            f"Requesting block {spend_hash} from peer=",
        ]):
            result = restored_wallet.importdescriptors([{"desc": descriptor, "timestamp": payment_time}])
        requests_after = self.nodes[0].debug_log_path.read_text(encoding="utf-8").count(request_log)
        assert_equal(result[0]["success"], True)
        assert_equal(requests_after - requests_before, 2)
        assert_equal(restored_wallet.gettransaction(payment_txid)["txid"], payment_txid)
        assert_equal(restored_wallet.gettransaction(spend_txid)["txid"], spend_txid)
        assert_equal(restored_wallet.getbalance(), 0)

        self.log.info("Require compact filters for fetch-backed wallet rescans")
        self.restart_node(1, extra_args=["-fastprune", "-prune=1", "-blockfetchproxy"])
        funding_wallet = self.nodes[1].get_wallet_rpc(self.default_wallet_name)
        assert_greater_than(self.nodes[1].pruneblockchain(300), payment_height)
        assert_raises_rpc_error(
            -1,
            "Block fetch proxy wallet rescans require -blockfilterindex=1",
            funding_wallet.rescanblockchain,
            0,
            payment_height,
        )


if __name__ == "__main__":
    WalletBlockFetchProxyTest(__file__).main()
