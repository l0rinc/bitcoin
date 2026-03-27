#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test automatic dbcache reduction after initial block download."""

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_greater_than


DEFAULT_MAX_TIP_AGE = 24 * 60 * 60
MOCK_TOTAL_RAM_MIB = 8208


class DBCachePostIBDTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], [f"-mocktotalram={MOCK_TOTAL_RAM_MIB}"]]

    def get_active_chainstate(self, node):
        return node.getchainstates()["chainstates"][-1]

    def run_test(self):
        miner = self.nodes[0]
        ibd_node = self.nodes[1]

        now = int(time.time())
        miner.setmocktime(now - DEFAULT_MAX_TIP_AGE - 1000)
        ibd_node.setmocktime(now)
        self.connect_nodes(0, 1)

        self.log.info("Sync an old chain so the receiving node remains in IBD.")
        self.generate(miner, 10)
        self.wait_until(lambda: ibd_node.getblockcount() == 10)
        assert ibd_node.getblockchaininfo()["initialblockdownload"]

        before = self.get_active_chainstate(ibd_node)
        before_total = before["coins_db_cache_bytes"] + before["coins_tip_cache_bytes"]
        self.log.info(f"IBD cache sizes: coinsdb={before['coins_db_cache_bytes']} coinstip={before['coins_tip_cache_bytes']}")

        self.log.info("Mine a recent block and verify automatic dbcache is reduced immediately after IBD.")
        with ibd_node.assert_debug_log(
            expected_msgs=[
                "Leaving InitialBlockDownload (latching to false)",
                "resized coinsdb cache",
                "resized coinstip cache",
            ],
            timeout=10,
        ):
            miner.setmocktime(now)
            self.generate(miner, 1)
            self.wait_until(lambda: not ibd_node.getblockchaininfo()["initialblockdownload"])

        after = self.get_active_chainstate(ibd_node)
        after_total = after["coins_db_cache_bytes"] + after["coins_tip_cache_bytes"]
        self.log.info(f"Post-IBD cache sizes: coinsdb={after['coins_db_cache_bytes']} coinstip={after['coins_tip_cache_bytes']}")

        assert ibd_node.getmempoolinfo()["usage"] == 0
        assert_greater_than(before_total, after_total)
        assert_greater_than(before["coins_tip_cache_bytes"], after["coins_tip_cache_bytes"])


if __name__ == "__main__":
    DBCachePostIBDTest(__file__).main()
