#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test replacement of multiple direct mempool conflicts by one transaction."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet


class MempoolRbfDirectConflictsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.uses_wallet = None

    def run_test(self):
        source = self.nodes[0]
        wallet = MiniWallet(self.nodes[1])

        self.disconnect_nodes(0, 1)
        funding = wallet.create_self_transfer_multi(num_outputs=2, fee_per_output=1000)
        source.sendrawtransaction(hexstring=funding["hex"])

        conflict_1 = wallet.create_self_transfer_multi(
            utxos_to_spend=[funding["new_utxos"][0]], fee_per_output=1000)
        conflict_2 = wallet.create_self_transfer_multi(
            utxos_to_spend=[funding["new_utxos"][1]], fee_per_output=1000)
        source.sendrawtransaction(hexstring=conflict_1["hex"])
        source.sendrawtransaction(hexstring=conflict_2["hex"])
        source.prioritisetransaction(txid=conflict_1["txid"], fee_delta=1000)
        source.prioritisetransaction(txid=conflict_2["txid"], fee_delta=-500)

        before_info = source.getmempoolinfo()
        before_entries = source.getrawmempool(verbose=True)
        expected_before = {funding["txid"], conflict_1["txid"], conflict_2["txid"]}
        assert_equal(set(before_entries), expected_before)
        assert_equal(before_info["size"], len(expected_before))
        assert_equal(before_info["unbroadcastcount"], len(expected_before))
        assert_equal(
            before_entries[conflict_1["txid"]]["fees"]["modified"],
            before_entries[conflict_1["txid"]]["fees"]["base"] + Decimal("0.00001000"),
        )
        assert_equal(
            before_entries[conflict_2["txid"]]["fees"]["modified"],
            before_entries[conflict_2["txid"]]["fees"]["base"] - Decimal("0.00000500"),
        )
        for entry in before_entries.values():
            assert_equal(entry["unbroadcast"], True)

        low_fee_replacement = wallet.create_self_transfer_multi(
            utxos_to_spend=funding["new_utxos"], fee_per_output=2400)
        assert_raises_rpc_error(
            -26,
            "less fees than conflicting txs",
            source.sendrawtransaction,
            hexstring=low_fee_replacement["hex"],
        )
        assert_equal(source.getrawmempool(verbose=True), before_entries)
        assert_equal(source.getmempoolinfo()["total_fee"], before_info["total_fee"])
        assert_equal(source.getmempoolinfo()["unbroadcastcount"], before_info["unbroadcastcount"])

        high_fee_replacement = wallet.create_self_transfer_multi(
            utxos_to_spend=funding["new_utxos"], fee_per_output=10000)
        assert_equal(
            source.sendrawtransaction(hexstring=high_fee_replacement["hex"]),
            high_fee_replacement["txid"],
        )

        after_info = source.getmempoolinfo()
        after_entries = source.getrawmempool(verbose=True)
        expected_after = {funding["txid"], high_fee_replacement["txid"]}
        assert_equal(set(after_entries), expected_after)
        assert_equal(after_info["size"], len(expected_after))
        assert_equal(after_info["unbroadcastcount"], len(expected_after))
        assert_equal(after_info["total_fee"], sum(entry["fees"]["base"] for entry in after_entries.values()))
        assert_equal(after_entries[funding["txid"]]["spentby"], [high_fee_replacement["txid"]])
        assert_equal(after_entries[high_fee_replacement["txid"]]["depends"], [funding["txid"]])
        assert_equal(after_entries[funding["txid"]]["unbroadcast"], True)
        assert_equal(after_entries[high_fee_replacement["txid"]]["unbroadcast"], True)
        assert_equal(
            after_entries[high_fee_replacement["txid"]]["fees"]["modified"],
            after_entries[high_fee_replacement["txid"]]["fees"]["base"],
        )


if __name__ == "__main__":
    MempoolRbfDirectConflictsTest(__file__).main()
