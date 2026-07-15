#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test persistent local-only reads of pruned blocks."""

import http.client
import time
import urllib.parse
from concurrent.futures import ThreadPoolExecutor

from test_framework.messages import (
    CBlock,
    CBlockHeader,
    CInv,
    MSG_BLOCK,
    from_hex,
    msg_getdata,
    msg_headers,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    sync_txindex,
)


class BlockFetchProxyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            ["-fastprune", "-prune=1", "-blockfetchproxy", "-rest", "-txindex", "-whitelist=noban@127.0.0.1"],
            [],
        ]

    def setup_network(self):
        self.setup_nodes()
        self.connect_nodes(0, 1)

    def run_test(self):
        node_url = urllib.parse.urlparse(self.nodes[0].url)

        def rest_status(path):
            connection = http.client.HTTPConnection(node_url.hostname, node_url.port)
            connection.request("GET", path)
            response = connection.getresponse()
            response.read()
            return response.status

        self.log.info("Build enough history to prune an old block")
        self.generate(self.nodes[1], 400)
        block_hashes = [self.nodes[1].getblockhash(height) for height in range(2, 5)]
        raw_blocks = [self.nodes[1].getblock(block_hash, 0) for block_hash in block_hashes]
        txids = [self.nodes[1].getblock(block_hash)["tx"][0] for block_hash in block_hashes]
        raw_txs = [self.nodes[1].getrawtransaction(txid, 0, block_hash) for txid, block_hash in zip(txids, block_hashes)]
        sync_txindex(self, self.nodes[0])
        assert_equal(self.nodes[0].pruneblockchain(300), 249)

        self.log.info("Do not fetch when requested verbosity requires undo data")
        with self.nodes[0].assert_debug_log(expected_msgs=[], unexpected_msgs=[f"Requesting block {block_hashes[1]} from peer="]):
            assert_raises_rpc_error(-1, "Undo data not available", self.nodes[0].getblock, block_hashes[1], 3)

        self.log.info("Fetch a pruned block for a local RPC and retain it on disk")
        with self.nodes[0].assert_debug_log([f"Requesting block {block_hashes[0]} from peer="]):
            assert_equal(self.nodes[0].getblock(block_hashes[0], 0), raw_blocks[0])
        self.nodes[0].pruneblockchain(300)
        with self.nodes[0].assert_debug_log(expected_msgs=[], unexpected_msgs=[f"Requesting block {block_hashes[0]} from peer="]):
            assert_equal(self.nodes[0].getblock(block_hashes[0], 0), raw_blocks[0])

        self.log.info("Use txindex to locate and read a transaction in a pruned block")
        with self.nodes[0].assert_debug_log([f"Requesting block {block_hashes[1]} from peer="]):
            assert_equal(self.nodes[0].getrawtransaction(txids[1]), raw_txs[1])

        self.log.info("Do not serve retained local-only blocks to peers")
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        peer.send_and_ping(msg_getdata([CInv(MSG_BLOCK, int(block_hashes[0], 16))]))
        assert "block" not in peer.last_message
        assert_equal(rest_status(f"/rest/block/{block_hashes[0]}.bin"), 404)
        assert_equal(rest_status(f"/rest/tx/{txids[1]}.bin"), 404)

        self.log.info("Read the retained block without a peer and after restart")
        self.disconnect_nodes(0, 1)
        assert_equal(self.nodes[0].getblock(block_hashes[0], 0), raw_blocks[0])
        self.restart_node(0)
        assert_equal(self.nodes[0].getblock(block_hashes[0], 0), raw_blocks[0])
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        peer.send_and_ping(msg_getdata([CInv(MSG_BLOCK, int(block_hashes[0], 16))]))
        assert "block" not in peer.last_message
        assert_raises_rpc_error(-1, "No outbound full-history peer has the requested block", self.nodes[0].getblock, block_hashes[2], 0)

        self.log.info("Require prune mode for the opt-in proxy")
        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(
            extra_args=["-blockfetchproxy"],
            expected_msg="Error: -blockfetchproxy requires prune mode.",
        )
        self.nodes[1].assert_start_raises_init_error(
            extra_args=["-prune=1", "-txindex"],
            expected_msg="Error: Prune mode with -txindex requires -blockfetchproxy.",
        )
        self.log.info("Interrupt a stalled local fetch promptly during shutdown")
        staller = self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=0)
        staller_id = self.nodes[0].getpeerinfo()[-1]["id"]
        stalled_hash = block_hashes[-1]
        old_header = CBlockHeader(from_hex(CBlock(), raw_blocks[-1]))
        staller.send_and_ping(msg_headers([old_header]))
        fetch_rpc = self.nodes[0].create_new_rpc_connection(mode="AUTHPROXY", client_timeout=60)
        with ThreadPoolExecutor(max_workers=1) as executor:
            request = executor.submit(fetch_rpc.getblock, stalled_hash, 0)
            staller.wait_until(lambda: any(inv.hash == int(stalled_hash, 16) for inv in staller.last_message.get("getdata", msg_getdata()).inv))
            assert_raises_rpc_error(
                -1,
                "Block is already requested for local use",
                self.nodes[0].getblockfrompeer,
                stalled_hash,
                staller_id,
            )
            start = time.monotonic()
            self.stop_node(0)
            assert time.monotonic() - start < 10
            assert request.exception(timeout=5) is not None


if __name__ == "__main__":
    BlockFetchProxyTest(__file__).main()
