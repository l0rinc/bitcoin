#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test stale-first historical reorg proxy behavior."""

from pathlib import Path

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.stale_reorg_proxy import (
    HistoricalReorgProxy,
    RPCChainView,
    load_stale_blocks_from_dir,
)
from test_framework.util import (
    assert_equal,
)


class HistoricalReorgProxyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def setup_network(self):
        self.setup_nodes()

    def _write_stale_blocks(self, *, source_node, stale_dir, stale_heights):
        stale_hashes_by_height = {}
        active_hashes_by_height = {}
        for stale_height in stale_heights:
            parent_hash = source_node.getblockhash(stale_height - 1)
            parent_time = source_node.getblockheader(parent_hash)["time"]
            active_hash = source_node.getblockhash(stale_height)
            active_time = source_node.getblockheader(active_hash)["time"]

            # Ensure stale block differs from active block while remaining valid.
            stale_time = max(parent_time + 1, active_time + 1)
            stale_block = create_block(
                int(parent_hash, 16),
                create_coinbase(stale_height),
                stale_time,
            )
            stale_block.solve()

            stale_path = stale_dir / f"{stale_height}-{stale_block.hash_hex}.bin"
            stale_path.write_bytes(stale_block.serialize())

            stale_hashes_by_height[stale_height] = stale_block.hash_hex
            active_hashes_by_height[stale_height] = active_hash

        return stale_hashes_by_height, active_hashes_by_height

    def run_test(self):
        source_node = self.nodes[0]
        sync_node = self.nodes[1]

        self.log.info("Generate canonical source chain")
        chain_length = 220
        self.generate(source_node, chain_length, sync_fun=self.no_op)

        self.log.info("Create stale blocks in stale-blocks format (<height>-<hash>.bin)")
        stale_heights = [30, 80, 140]
        stale_dir = Path(sync_node.datadir_path) / "stale-blocks"
        stale_dir.mkdir(parents=True, exist_ok=True)
        stale_hashes_by_height, active_hashes_by_height = self._write_stale_blocks(
            source_node=source_node,
            stale_dir=stale_dir,
            stale_heights=stale_heights,
        )

        stale_blocks = load_stale_blocks_from_dir(stale_dir)
        assert_equal(len(stale_blocks), len(stale_heights))

        self.log.info("Connect sync node to stale-first proxy only")
        proxy = sync_node.add_outbound_p2p_connection(
            HistoricalReorgProxy(
                chain_view=RPCChainView(source_node),
                stale_blocks=stale_blocks,
                max_height=chain_length,
            ),
            p2p_idx=0,
            connection_type="outbound-full-relay",
        )

        self.log.info("Wait for full sync through proxy")
        self.wait_until(lambda: sync_node.getblockcount() == chain_length)
        assert_equal(sync_node.getbestblockhash(), source_node.getbestblockhash())

        self.log.info("Verify stale blocks were requested before their active counterparts")
        served_positions = {}
        for idx, (_, _, served_hash) in enumerate(proxy.served_blocks):
            served_positions.setdefault(served_hash, idx)

        for stale_height in stale_heights:
            stale_hash_int = int(stale_hashes_by_height[stale_height], 16)
            active_hash_int = int(active_hashes_by_height[stale_height], 16)
            assert stale_hash_int in served_positions
            assert active_hash_int in served_positions
            assert served_positions[stale_hash_int] < served_positions[active_hash_int]

        self.log.info("Verify stale tips remain indexed as forks")
        tips_by_hash = {tip["hash"]: tip["status"] for tip in sync_node.getchaintips()}
        for stale_hash in stale_hashes_by_height.values():
            assert stale_hash in tips_by_hash
            assert tips_by_hash[stale_hash] == "valid-fork"

        self.log.info("Snapshot and compare final UTXO set hash")
        source_utxo = source_node.gettxoutsetinfo("hash_serialized_3", use_index=False)
        sync_utxo = sync_node.gettxoutsetinfo("hash_serialized_3", use_index=False)
        assert_equal(sync_utxo["hash_serialized_3"], source_utxo["hash_serialized_3"])
        self.log.info(
            "Final UTXO snapshot hash_serialized_3 at height %d: %s",
            sync_utxo["height"],
            sync_utxo["hash_serialized_3"],
        )


if __name__ == '__main__':
    HistoricalReorgProxyTest(__file__).main()
