#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test stale-first historical reorg proxy behavior."""

from pathlib import Path
import re

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.stale_reorg_proxy import (
    HistoricalReorgProxy,
    RPCChainView,
    RawBlockMessage,
    load_stale_blocks_from_dir,
)
from test_framework.messages import (
    CInv,
    MSG_BLOCK,
    msg_getdata,
    msg_headers,
    msg_inv,
    msg_notfound,
)
from test_framework.util import (
    assert_equal,
)

UPDATE_TIP_RE = re.compile(r"UpdateTip: new best=([0-9a-f]{64}) height=(\d+)")


class CapturingHistoricalReorgProxy(HistoricalReorgProxy):
    """Proxy variant for direct functional regression checks without a socket."""

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.sent_messages = []

    def send_without_ping(self, message, is_decoy=False):
        self.sent_messages.append(message)


class HistoricalReorgProxyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def setup_network(self):
        self.setup_nodes()

    def _write_stale_blocks(self, *, source_node, stale_dir):
        # Build one two-block stale branch at heights 80->81 and two independent stale blocks.
        stale_plan = [
            {"name": "h30", "height": 30, "parent": ("active", 29)},
            {"name": "h80", "height": 80, "parent": ("active", 79)},
            {"name": "h81", "height": 81, "parent": ("stale", "h80")},
            {"name": "h140", "height": 140, "parent": ("active", 139)},
        ]

        stale_hashes_by_name = {}
        stale_times_by_name = {}
        active_hashes_by_height = {}

        for entry in stale_plan:
            stale_height = entry["height"]
            parent_kind, parent_ref = entry["parent"]
            if parent_kind == "active":
                parent_hash = source_node.getblockhash(parent_ref)
                parent_time = source_node.getblockheader(parent_hash)["time"]
            else:
                parent_hash = stale_hashes_by_name[parent_ref]
                parent_time = stale_times_by_name[parent_ref]

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

            stale_hashes_by_name[entry["name"]] = stale_block.hash_hex
            stale_times_by_name[entry["name"]] = stale_time
            active_hashes_by_height[stale_height] = active_hash

        return stale_hashes_by_name, active_hashes_by_height

    def _run_direct_proxy_regression_checks(self, *, source_node, stale_blocks):
        self.log.info("Regression check: proactive stale announce on getdata-only progression")
        stale_80 = next(stale for stale in stale_blocks if stale.height == 80)
        proactive_proxy = CapturingHistoricalReorgProxy(
            chain_view=RPCChainView(source_node),
            stale_blocks=[stale_80],
            max_height=220,
            defer_stale_until_getdata=True,
            record_all_served_blocks=False,
            record_getdata_requests=False,
            active_prefetch_window=0,
            active_send_ahead=0,
        )
        proactive_proxy.on_getdata(
            msg_getdata([CInv(MSG_BLOCK, int(source_node.getblockhash(79), 16))])
        )
        assert_equal(proactive_proxy._stats["stale_headers"], 1)
        assert_equal(proactive_proxy._stats["stale_inv_nudge"], 1)
        assert stale_80.hash_int in proactive_proxy.announced_stale_headers
        announced_header_hashes = [
            header.hash_int
            for message in proactive_proxy.sent_messages
            if isinstance(message, msg_headers)
            for header in message.headers
        ]
        announced_inv_hashes = [
            inv.hash
            for message in proactive_proxy.sent_messages
            if isinstance(message, msg_inv)
            for inv in message.inv
        ]
        assert stale_80.hash_int in announced_header_hashes
        assert stale_80.hash_int in announced_inv_hashes
        assert any(
            isinstance(message, RawBlockMessage) for message in proactive_proxy.sent_messages
        )

        self.log.info("Regression check: stale re-request falls back to active chain")
        stale_30 = next(stale for stale in stale_blocks if stale.height == 30)
        repeat_proxy = CapturingHistoricalReorgProxy(
            chain_view=RPCChainView(source_node),
            stale_blocks=[stale_30],
            max_height=220,
            defer_stale_until_getdata=True,
            record_all_served_blocks=False,
            record_getdata_requests=False,
            active_prefetch_window=0,
            active_send_ahead=0,
        )
        repeat_proxy.on_getdata(msg_getdata([CInv(MSG_BLOCK, stale_30.hash_int)]))
        assert_equal(repeat_proxy._stats["stale_blocks"], 1)
        repeat_proxy.sent_messages = []
        repeat_proxy.on_getdata(msg_getdata([CInv(MSG_BLOCK, stale_30.hash_int)]))
        assert_equal(repeat_proxy._stats["stale_repeats"], 1)
        assert stale_30.hash_int in repeat_proxy.disabled_stale_blocks
        notfound_hashes = [
            inv.hash
            for message in repeat_proxy.sent_messages
            if isinstance(message, msg_notfound)
            for inv in message.vec
        ]
        fallback_inv_hashes = [
            inv.hash
            for message in repeat_proxy.sent_messages
            if isinstance(message, msg_inv)
            for inv in message.inv
        ]
        assert stale_30.hash_int in notfound_hashes
        assert int(source_node.getblockhash(stale_30.height), 16) in fallback_inv_hashes

        self.log.info("Regression check: inject stale block even if peer only requests active hash")
        inject_proxy = CapturingHistoricalReorgProxy(
            chain_view=RPCChainView(source_node),
            stale_blocks=[stale_80],
            max_height=220,
            defer_stale_until_getdata=True,
            record_all_served_blocks=True,
            record_getdata_requests=False,
            active_prefetch_window=0,
            active_send_ahead=0,
        )
        active_80_hash_int = int(source_node.getblockhash(80), 16)
        inject_proxy.on_getdata(msg_getdata([CInv(MSG_BLOCK, active_80_hash_int)]))
        assert_equal(inject_proxy._stats["stale_blocks"], 1)
        assert_equal(inject_proxy.served_blocks[0][0], "stale")
        assert_equal(inject_proxy.served_blocks[0][1], 80)
        assert_equal(inject_proxy.served_blocks[1], ("active", 80, active_80_hash_int))

        self.log.info("Regression check: keep advancing active headers after stale replay is exhausted")
        follow_proxy = CapturingHistoricalReorgProxy(
            chain_view=RPCChainView(source_node),
            stale_blocks=[],
            max_height=220,
            defer_stale_until_getdata=True,
            record_all_served_blocks=False,
            record_getdata_requests=False,
            active_prefetch_window=0,
            active_send_ahead=0,
        )
        follow_proxy._highest_announced_active_height = 100
        active_100_hash_int = int(source_node.getblockhash(100), 16)
        follow_proxy.on_getdata(msg_getdata([CInv(MSG_BLOCK, active_100_hash_int)]))
        header_batches = [
            message for message in follow_proxy.sent_messages if isinstance(message, msg_headers)
        ]
        assert header_batches, "proxy did not extend the active header frontier"
        followed_headers = header_batches[-1].headers
        assert_equal(followed_headers[0].hash_hex, source_node.getblockhash(101))
        assert_equal(followed_headers[-1].hash_hex, source_node.getblockhash(220))

    def run_test(self):
        source_node = self.nodes[0]
        sync_node = self.nodes[1]

        self.log.info("Generate canonical source chain")
        chain_length = 220
        self.generate(source_node, chain_length, sync_fun=self.no_op)

        self.log.info("Create stale blocks in stale-blocks format (<height>-<hash>.bin)")
        stale_dir = Path(sync_node.datadir_path) / "stale-blocks"
        stale_dir.mkdir(parents=True, exist_ok=True)
        stale_hashes_by_name, active_hashes_by_height = self._write_stale_blocks(
            source_node=source_node,
            stale_dir=stale_dir,
        )

        stale_blocks = load_stale_blocks_from_dir(stale_dir)
        assert_equal(len(stale_blocks), len(stale_hashes_by_name))
        self._run_direct_proxy_regression_checks(source_node=source_node, stale_blocks=stale_blocks)

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

        self.log.info("Verify each stale block became the active tip (and was later reorged out)")
        debug_log = Path(sync_node.datadir_path) / "regtest" / "debug.log"
        update_tips = []
        with debug_log.open("r", encoding="utf-8", errors="replace") as log_file:
            for line in log_file:
                match = UPDATE_TIP_RE.search(line)
                if match is None:
                    continue
                update_tips.append((int(match.group(2)), match.group(1)))

        self.log.info("Verify stale blocks were requested before their active counterparts")
        served_positions = {}
        for idx, (_, _, served_hash) in enumerate(proxy.served_blocks):
            served_positions.setdefault(served_hash, idx)

        for stale_height, stale_name in ((30, "h30"), (80, "h80"), (81, "h81"), (140, "h140")):
            stale_hash_int = int(stale_hashes_by_name[stale_name], 16)
            active_hash_int = int(active_hashes_by_height[stale_height], 16)
            assert stale_hash_int in served_positions
            assert active_hash_int in served_positions
            assert served_positions[stale_hash_int] < served_positions[active_hash_int]

            stale_hash_hex = stale_hashes_by_name[stale_name]
            assert any(
                height == stale_height and block_hash == stale_hash_hex
                for height, block_hash in update_tips
            ), f"stale block never became tip: {stale_height}-{stale_hash_hex}"
            header = sync_node.getblockheader(stale_hash_hex)
            assert header["confirmations"] < 0, f"stale block still on active chain: {stale_height}-{stale_hash_hex}"

        self.log.info("Verify multi-block stale branch served in order (80 -> 81)")
        stale_80 = int(stale_hashes_by_name["h80"], 16)
        stale_81 = int(stale_hashes_by_name["h81"], 16)
        assert served_positions[stale_80] < served_positions[stale_81]

        self.log.info("Verify stale tips remain indexed as forks")
        tips_by_hash = {tip["hash"]: tip["status"] for tip in sync_node.getchaintips()}
        for stale_hash in (
            stale_hashes_by_name["h30"],
            stale_hashes_by_name["h81"],
            stale_hashes_by_name["h140"],
        ):
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
