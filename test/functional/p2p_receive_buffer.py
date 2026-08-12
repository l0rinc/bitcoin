#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test aggregate memory retained by incomplete P2P messages."""

import struct

from test_framework.messages import (
    MAGIC_BYTES,
    msg_version,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework


class ReceiveBufferTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-maxconnections=20", "-v2transport=0"]]

    def run_test(self):
        node = self.nodes[0]

        def peer_id(peer):
            sockname = peer._transport.get_extra_info("socket").getsockname()
            peer_addr = f"{sockname[0]}:{sockname[1]}"
            return next(info["id"] for info in node.getpeerinfo() if info["addr"] == peer_addr)

        def bytes_received(peer_id_):
            peer_info = [info for info in node.getpeerinfo() if info["id"] == peer_id_]
            return peer_info[0]["bytesrecv"] if peer_info else None

        def received_at_least(peer_id_, target):
            received = bytes_received(peer_id_)
            return received is not None and received >= target

        peers = []
        for _ in range(7):
            peer = P2PInterface()
            version = msg_version()
            version.nVersion = 70016
            version.nServices = 1
            version.strSubVer = "/test:0.0.3/"
            version.relay = 0
            node.add_p2p_connection(peer, supports_v2_p2p=False, send_version=False, wait_for_verack=False)
            peer.send_without_ping(version)
            peer.wait_for_verack()
            peers.append(peer)
        peer_ids = [peer_id(peer) for peer in peers]
        large_consumer_ids = peer_ids[:6]
        trigger_id = peer_ids[6]

        header = (
            MAGIC_BYTES["regtest"]
            + b"block".ljust(12, b"\x00")
            + struct.pack("<I", 4_000_000)
            + b"\x00" * 4
        )
        partial_message = header + b"\x00" * 3_800_000
        trigger_message = header + b"\x00" * 2_500_000

        for peer, peer_id_ in zip(peers[:6], large_consumer_ids):
            bytes_before = bytes_received(peer_id_)
            peer.send_raw_message(partial_message)
            self.wait_until(lambda: received_at_least(peer_id_, bytes_before + len(partial_message)))

        bytes_before = bytes_received(trigger_id)
        peers[6].send_raw_message(trigger_message)
        self.wait_until(lambda: received_at_least(trigger_id, bytes_before + len(trigger_message)))
        self.wait_until(lambda: len(node.getpeerinfo()) < len(peer_ids))

        connected_ids = {info["id"] for info in node.getpeerinfo()}
        assert trigger_id in connected_ids
        assert len(set(large_consumer_ids) - connected_ids) == 1
        node.getblockcount()


if __name__ == "__main__":
    ReceiveBufferTest(__file__).main()
