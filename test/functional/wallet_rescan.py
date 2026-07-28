#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test wallet rescan RPC concurrency and reservation behavior."""

import concurrent.futures
import threading

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import JSONRPCException, assert_equal


class WalletRescanTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-blockfilterindex=0"]]
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        node.createwallet("w")
        wallet = node.get_wallet_rpc("w")
        address = wallet.getnewaddress()
        block_count = 1000
        self.generatetoaddress(node, block_count, address)

        for _ in range(10):
            start = threading.Barrier(3)

            def rescan_after_barrier():
                rpc = node.create_new_rpc_connection(mode="AUTHPROXY") / "wallet/w"
                start.wait(timeout=10)
                return rpc.rescanblockchain(0, block_count)

            with concurrent.futures.ThreadPoolExecutor(max_workers=2) as thread:
                rescans = [thread.submit(rescan_after_barrier) for _ in range(2)]
                start.wait(timeout=10)

                successes = 0
                conflicts = 0
                for future in concurrent.futures.as_completed(rescans, timeout=60):
                    try:
                        future.result()
                        successes += 1
                    except JSONRPCException as e:
                        assert_equal(e.error["code"], -4)
                        assert_equal(e.error["message"], "Wallet is currently rescanning. Abort existing rescan or wait.")
                        conflicts += 1

                assert_equal(successes, 1)
                assert_equal(conflicts, 1)


if __name__ == "__main__":
    WalletRescanTest(__file__).main()
