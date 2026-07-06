#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test -maxorphantx orphanage count policy."""

from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
)
from test_framework.p2p import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

SCRIPT_PUB_KEY_OP_TRUE = b"\x51\x75" * 15 + b"\x51"


class MaxOrphanTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            ["-acceptnonstdtxn=1", "-maxorphantx=3"],
            ["-acceptnonstdtxn=1", "-maxorphantx=0"],
        ]

    def make_orphans(self, count):
        orphans = []
        for i in range(count):
            tx = CTransaction()
            tx.vin.append(CTxIn(outpoint=COutPoint(i + 1, 333)))
            tx.vout.append(CTxOut(nValue=11 * COIN, scriptPubKey=SCRIPT_PUB_KEY_OP_TRUE))
            tx.rehash()
            orphans.append(tx)
        return orphans

    def run_test(self):
        for i, node in enumerate(self.nodes):
            node.add_outbound_p2p_connection(P2PDataStore(), p2p_idx=i)

        self.log.info("-maxorphantx limits the number of stored orphan transactions")
        self.nodes[0].p2ps[0].send_txs_and_test(self.make_orphans(4), self.nodes[0], success=False)
        self.wait_until(lambda: len(self.nodes[0].getorphantxs()) == 3)

        self.log.info("-maxorphantx=0 disables orphan transaction storage")
        self.nodes[1].p2ps[0].send_txs_and_test(self.make_orphans(1), self.nodes[1], success=False)
        self.nodes[1].p2ps[0].sync_with_ping()
        assert_equal(self.nodes[1].getorphantxs(), [])


if __name__ == "__main__":
    MaxOrphanTxTest(__file__).main()
