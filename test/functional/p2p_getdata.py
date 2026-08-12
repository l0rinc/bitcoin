#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test GETDATA processing behavior"""
from collections import defaultdict

from test_framework.messages import (
    CInv,
    MAX_INV_SIZE,
    MSG_BLOCK,
    msg_getdata,
)
from test_framework.p2p import NetworkThread, P2PInterface
from test_framework.test_framework import BitcoinTestFramework


class P2PStoreBlock(P2PInterface):
    def __init__(self):
        super().__init__()
        self.blocks = defaultdict(int)

    def on_block(self, message):
        self.blocks[message.block.hash_int] += 1


class GetdataTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def test_invalid_getdata(self):
        p2p_block_store = self.nodes[0].add_p2p_connection(P2PStoreBlock())

        self.log.info("test that an invalid GETDATA doesn't prevent processing of future messages")

        # Send invalid message and verify that node responds to later ping
        invalid_getdata = msg_getdata()
        invalid_getdata.inv.append(CInv(t=0, h=0))  # INV type 0 is invalid.
        p2p_block_store.send_and_ping(invalid_getdata)

        # Check getdata still works by fetching tip block
        best_block = int(self.nodes[0].getbestblockhash(), 16)
        good_getdata = msg_getdata()
        good_getdata.inv.append(CInv(t=2, h=best_block))
        p2p_block_store.send_and_ping(good_getdata)
        p2p_block_store.wait_until(lambda: p2p_block_store.blocks[best_block] == 1)

    def test_aggregate_queue_limit(self):
        self.log.info("test concurrent maximum-size GETDATA request queues")
        node = self.nodes[0]
        node.disconnect_p2ps()
        peers = [node.add_p2p_connection(P2PStoreBlock(), supports_v2_p2p=False) for _ in range(4)]

        def peer_id(peer):
            sockname = peer._transport.get_extra_info("socket").getsockname()
            address = f"{sockname[0]}:{sockname[1]}"
            return next(info["id"] for info in node.getpeerinfo() if info["addr"] == address)

        best_block = int(node.getbestblockhash(), 16)
        getdata = msg_getdata([CInv(MSG_BLOCK, best_block)] * MAX_INV_SIZE)
        request = peers[0].build_message(getdata)
        received_logs = [
            f"received: getdata ({len(getdata.serialize())} bytes) peer={peer_id(peer)}"
            for peer in peers
        ]
        limit_log = f"aggregate queued requests would exceed {MAX_INV_SIZE}"

        # Keep the first accepted request queued behind send backpressure.
        for peer in peers:
            NetworkThread.network_event_loop.call_soon_threadsafe(peer._transport.pause_reading)

        with node.assert_debug_log(
            expected_msgs=[*received_logs, limit_log],
            timeout=30,
        ):
            for peer in peers:
                peer.send_raw_message(request)

        # An individually valid maximum-size request remains accepted.
        NetworkThread.network_event_loop.call_soon_threadsafe(peers[0]._transport.resume_reading)
        peers[0].wait_until(lambda: peers[0].blocks[best_block] >= 1)
        assert len(node.getpeerinfo()) == len(peers)

        for peer in peers:
            peer.peer_disconnect()
        node.disconnect_p2ps()

        # Disconnecting the queued peer releases its request capacity.
        self.test_invalid_getdata()

    def run_test(self):
        self.test_invalid_getdata()
        self.test_aggregate_queue_limit()


if __name__ == '__main__':
    GetdataTest(__file__).main()
