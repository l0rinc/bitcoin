#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test prune-assumevalid IBD mode."""

from test_framework.blocktools import (
    COINBASE_MATURITY,
)
from test_framework.messages import (
    CBlock,
    CBlockHeader,
    MAX_HEADERS_RESULTS,
    MSG_BLOCK,
    MSG_TYPE_MASK,
    MSG_WITNESS_FLAG,
    from_hex,
    msg_block,
    msg_headers,
    msg_no_witness_block,
)
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
)
from test_framework.wallet import MiniWallet


CACHED_TEST_HEIGHT = 128
ASSUMEVALID_HEIGHT = max(COINBASE_MATURITY + 2, CACHED_TEST_HEIGHT + 2)
BURIAL_BLOCKS = 2100


def as_list(peers):
    return peers if isinstance(peers, list) else [peers]


class AssumeValidBlockStore(P2PInterface):
    def __init__(self, blocks, max_height_to_serve, first_height=1, force_no_witness=False, withheld_heights=()):
        super().__init__()
        self.blocks = blocks
        self.first_height = first_height
        hashes = [block.hash_int for block in blocks]
        self.blocks_by_hash = dict(zip(hashes, blocks))
        self.height_by_hash = {block_hash: height for height, block_hash in enumerate(hashes, start=first_height)}
        self.max_height_to_serve = max_height_to_serve
        self.force_no_witness = force_no_witness
        self.withheld_heights = set(withheld_heights)
        self.pending_getdata = []
        self.request_types_by_height = {}

    def send_headers_for_blocks(self, blocks):
        for start in range(0, len(blocks), MAX_HEADERS_RESULTS):
            self.send_without_ping(msg_headers([CBlockHeader(block) for block in blocks[start:start + MAX_HEADERS_RESULTS]]))

    def on_getheaders(self, message):
        start = 0
        for locator_hash in message.locator.vHave:
            if locator_hash in self.height_by_hash:
                start = self.height_by_hash[locator_hash] - self.first_height + 1
                break
        headers = []
        for block in self.blocks[start:]:
            headers.append(CBlockHeader(block))
            if block.hash_int == message.hashstop or len(headers) == MAX_HEADERS_RESULTS:
                break
        self.send_without_ping(msg_headers(headers))

    def on_getdata(self, message):
        for inv in message.inv:
            height = self.height_by_hash.get(inv.hash)
            if height is None:
                continue
            self.request_types_by_height.setdefault(height, []).append(inv.type)
            if self._can_serve(height):
                self._send_block(inv)
            else:
                self.pending_getdata.append(inv)

    def _can_serve(self, height):
        return height <= self.max_height_to_serve and height not in self.withheld_heights

    def serve_until_height(self, height):
        with p2p_lock:
            self.max_height_to_serve = height
            still_pending = []
            for inv in self.pending_getdata:
                if self._can_serve(self.height_by_hash[inv.hash]):
                    self._send_block(inv)
                else:
                    still_pending.append(inv)
            self.pending_getdata = still_pending

    def serve_pending_heights(self, heights, witness=None):
        """Answer the latest pending request for each height, optionally overriding its witness flag."""
        with p2p_lock:
            heights_to_send = set(heights)
            pending_by_height = {self.height_by_hash[inv.hash]: inv for inv in self.pending_getdata}
            for height in heights:
                self._send_block(pending_by_height[height], witness)
            self.pending_getdata = [inv for inv in self.pending_getdata if self.height_by_hash[inv.hash] not in heights_to_send]

    def _send_block(self, inv, witness=None):
        if inv.type & MSG_TYPE_MASK != MSG_BLOCK:
            return
        block = self.blocks_by_hash[inv.hash]
        if witness is None:
            witness = inv.type & MSG_WITNESS_FLAG and not self.force_no_witness
        if witness:
            self.send_without_ping(msg_block(block))
        else:
            self.send_without_ping(msg_no_witness_block(block))


class FeaturePruneAssumeValidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 12
        self.rpc_timeout = 120

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        # Node 0 only builds the source chain, so skip the per-header block index check there
        self.start_node(0, extra_args=["-checkblockindex=0"])

    def reset_datadir(self, index):
        self.cleanup_folder(self.nodes[index].chain_path)

    def stored_block_bytes(self, node):
        # Unlike flatfile sizes, this also counts writes inside preallocated blk/rev chunks.
        return node.getblockchaininfo()["size_on_disk"]

    def requested_types(self, peers, height):
        with p2p_lock:
            return [
                request_type
                for peer in as_list(peers)
                for request_type in peer.request_types_by_height.get(height, [])
            ]

    def assert_requested(self, peers, height, expected_type):
        self.wait_until(lambda: expected_type in self.requested_types(peers, height), timeout=60)

    def submit_headers(self, node, blocks):
        results = node.batch([node.submitheader.get_request(CBlockHeader(block).serialize().hex()) for block in blocks])
        assert all(result.get("error") is None for result in results)

    def build_source_chain(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        self.log.info("Mine a buried assumevalid chain with a witness spend at the assumevalid height")
        self.generate(wallet, ASSUMEVALID_HEIGHT - 1, sync_fun=self.no_op)
        spent_utxo = wallet.get_utxo(confirmed_only=True)
        spend_tx = wallet.send_self_transfer(from_node=node, utxo_to_spend=spent_utxo)
        assumevalid_hash = self.generate(wallet, 1, sync_fun=self.no_op)[0]
        assert_equal(node.getblockcount(), ASSUMEVALID_HEIGHT)
        self.generate(wallet, BURIAL_BLOCKS, sync_fun=self.no_op)

        block_hashes = node.batch([node.getblockhash.get_request(height) for height in range(1, node.getblockcount() + 1)])
        blocks = [from_hex(CBlock(), result["result"]) for result in node.batch([node.getblock.get_request(block_hash["result"], 0) for block_hash in block_hashes])]
        assumevalid_block = blocks[ASSUMEVALID_HEIGHT - 1]
        assert any(not tx.wit.is_null() for tx in assumevalid_block.vtx[1:])
        return blocks, assumevalid_hash, spend_tx["txid"], spent_utxo["txid"], spent_utxo["vout"]

    def run_test(self):
        blocks, assumevalid_hash, spend_txid, spent_txid, spent_vout = self.build_source_chain()
        block_hashes = [block.hash_hex for block in blocks]
        final_chainwork = self.nodes[0].getblockheader(block_hashes[-1])["chainwork"]
        prune_assumevalid_args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={final_chainwork}"]
        self.nodes[3].assert_start_raises_init_error(
            extra_args=["-pruneassumevalid", "-txindex"],
            expected_msg="Error: Prune mode is incompatible with -txindex.",
        )
        self.log.info("Valid configurations fall back when prune-assumevalid preconditions do not apply")
        fallback_cases = [
            (["-prune=0", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}"], "pruning is disabled", 0),
            (["-pruneassumevalid", "-assumevalid=0"], "assumevalid is disabled", 550),
            (["-prune=1", "-pruneassumevalid", f"-assumevalid={'01' * 32}"], None, 1),
            (["-prune=1234", "-pruneassumevalid", "-assumevalid=0"], "assumevalid is disabled", 1234),
            (["-nopruneassumevalid", f"-assumevalid={assumevalid_hash}"], None, 0),
            (prune_assumevalid_args + ["-blockfilterindex=basic"], "block filter or coin statistics indexing requires stored undo data", 1),
            (prune_assumevalid_args + ["-coinstatsindex"], "block filter or coin statistics indexing requires stored undo data", 1),
        ]
        for args, reason, prune_target in fallback_cases:
            node = self.nodes[3]
            self.start_node(3, extra_args=args)
            info = node.getblockchaininfo()
            assert_equal(info["pruned"], prune_target != 0)
            if prune_target:
                assert_equal(info["automatic_pruning"], prune_target > 1)
                if prune_target > 1:
                    assert_equal(info["prune_target_size"], prune_target * 1024 * 1024)
            if reason is not None:
                assert f"-pruneassumevalid inactive: {reason}" in node.debug_log_path.read_text(encoding="utf-8")
            self.submit_headers(node, blocks)
            before = self.stored_block_bytes(node)
            peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=1))
            peer.send_headers_for_blocks(blocks[-1:])
            self.wait_until(lambda: node.getblockcount() == 1, timeout=60)
            self.assert_requested(peer, 1, MSG_BLOCK | MSG_WITNESS_FLAG)
            assert_greater_than(self.stored_block_bytes(node), before)
            assert_equal(node.getblock(block_hashes[0])["height"], 1)
            self.stop_node(3)
            self.reset_datadir(3)

        self.log.info("Default pruned assumevalid behavior still requests and stores witness blocks")
        self.start_node(2, extra_args=["-prune=1", f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={final_chainwork}"])
        default_node = self.nodes[2]
        self.submit_headers(default_node, blocks)
        default_bytes = self.stored_block_bytes(default_node)
        default_peer = default_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=1))
        default_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: default_node.getblockcount() == 1, timeout=60)
        self.assert_requested(default_peer, 1, MSG_BLOCK | MSG_WITNESS_FLAG)
        assert_greater_than(self.stored_block_bytes(default_node), default_bytes)
        assert_equal(default_node.getblock(block_hashes[0])["height"], 1)


if __name__ == "__main__":
    FeaturePruneAssumeValidTest(__file__).main()
