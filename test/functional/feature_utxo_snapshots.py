#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Check UTXO snapshot commitments, queries, restart, and chain updates."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, dumb_sync_blocks, try_rpc
from test_framework.wallet import MiniWallet


SNAPSHOT_HEIGHT = 299
SNAPSHOT_HASH = "0c552ced4721c249a389eb9b08cb8da261cd46f0e7b5f9d064d48f3113406853"
UTXO_HASH = "106b2c56233e378a824cf0d5ff2be42ed32c72f1605c9be288d00942908a40ac"


class UTXOSnapshotsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        self.start_nodes()

    def utxo_stats(self, node):
        stats = node.gettxoutsetinfo(use_index=False)
        return {key: stats[key] for key in ("height", "bestblock", "txouts", "bogosize", "hash_serialized_3", "total_amount")}

    def assert_utxo_sets_equal(self):
        assert_equal(*(self.utxo_stats(node) for node in self.nodes[:2]))

    def run_test(self):
        source, restored, empty_group_node, duplicate_node = self.nodes
        wallet = MiniWallet(source)
        source.setmocktime(source.getblockheader(source.getbestblockhash())["time"])
        self.log.info("Generate the chain committed to by regtest chainparams")
        for i in range(SNAPSHOT_HEIGHT - source.getblockcount()):
            if i % 3 == 0:
                wallet.send_self_transfer(from_node=source)
            self.generate(source, 1, sync_fun=self.no_op)
        assert_equal(source.getbestblockhash(), SNAPSHOT_HASH)
        snapshot = source.dumptxoutset("snapshot.dat", "latest")
        assert_equal(snapshot["txoutset_hash"], UTXO_HASH)

        self.log.info("Require the snapshot base header before importing coins")
        assert_raises_rpc_error(-32603, "must appear in the headers chain", restored.loadtxoutset, snapshot["path"])
        for height in range(restored.getblockcount() + 1, SNAPSHOT_HEIGHT + 1):
            header = source.getblockheader(source.getblockhash(height), False)
            for node in (restored, empty_group_node, duplicate_node):
                node.submitheader(header)

        self.log.info("Check snapshot transaction groups and announced coin count")
        contents = bytearray(source.chain_path.joinpath("snapshot.dat").read_bytes())
        header_size = 51
        empty_group = contents[:header_size] + bytes(33) + contents[header_size:]
        duplicate = contents[:header_size - 8] + (snapshot["coins_written"] * 2).to_bytes(8, "little") + contents[header_size:] * 2
        for node, name, data, rejected, expected_error in (
            (empty_group_node, "empty-group.dat", empty_group, False, "Bad snapshot data - txid has no coins"),  # TODO: Empty groups should be rejected
            (duplicate_node, "duplicate.dat", duplicate, False, f"Bad snapshot coins count: expected {snapshot['coins_written'] * 2}, got {snapshot['coins_written']}"),  # TODO: Announced count must match unique UTXOs
        ):
            path = source.chain_path / name
            path.write_bytes(data)
            assert_equal(try_rpc(-32603, expected_error, node.loadtxoutset, path), rejected)

        self.log.info("Reject a changed coin while preserving the original UTXO set")
        original = self.utxo_stats(restored)
        contents[header_size] ^= 1  # Change the first transaction hash
        invalid_path = source.chain_path / "changed-snapshot.dat"
        invalid_path.write_bytes(contents)
        assert_raises_rpc_error(-32603, "Bad snapshot content hash", restored.loadtxoutset, invalid_path)
        assert_equal(self.utxo_stats(restored), original)

        self.log.info("Import the committed UTXO set and preserve it across restart")
        loaded = restored.loadtxoutset(snapshot["path"])
        assert_equal(loaded["base_height"], SNAPSHOT_HEIGHT)
        assert_equal(loaded["coins_loaded"], snapshot["coins_written"])
        self.assert_utxo_sets_equal()
        self.restart_node(restored.index)
        self.assert_utxo_sets_equal()
        assert_raises_rpc_error(-32603, "more than once", restored.loadtxoutset, snapshot["path"])

        self.log.info("Spend snapshot coins and validate new blocks")
        tx = wallet.send_self_transfer(from_node=restored)
        assert tx["txid"] in restored.getrawmempool()
        source.sendrawtransaction(tx["hex"])
        self.generate(source, 3, sync_fun=self.no_op)
        dumb_sync_blocks(src=source, dst=restored)
        self.assert_utxo_sets_equal()
        assert_equal(restored.getrawmempool(), [])

        self.log.info("Disconnect and reconnect blocks above the snapshot base")
        old_tip = source.getbestblockhash()
        for node in self.nodes[:2]:
            node.invalidateblock(old_tip)
        self.assert_utxo_sets_equal()
        for node in self.nodes[:2]:
            node.reconsiderblock(old_tip)
        self.assert_utxo_sets_equal()
        self.restart_node(restored.index)
        self.assert_utxo_sets_equal()

        self.log.info("Rebuild from blocks when reindexing a restored UTXO set")
        self.restart_node(restored.index, extra_args=["-reindex"])
        self.connect_nodes(source.index, restored.index)
        self.sync_blocks(self.nodes[:2])
        self.assert_utxo_sets_equal()


if __name__ == "__main__":
    UTXOSnapshotsTest(__file__).main()
