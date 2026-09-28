#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test prune-assumevalid IBD mode."""

import copy
import os

from test_framework.blocktools import (
    COINBASE_MATURITY,
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    BlockTransactions,
    BlockTransactionsRequest,
    CBlock,
    CBlockHeader,
    CInv,
    HeaderAndShortIDs,
    MAX_HEADERS_RESULTS,
    MSG_BLOCK,
    MSG_TYPE_MASK,
    MSG_WITNESS_FLAG,
    NODE_NETWORK,
    NODE_NETWORK_LIMITED,
    NODE_WITNESS,
    from_hex,
    msg_block,
    msg_blocktxn,
    msg_cmpctblock,
    msg_getdata,
    msg_getblocktxn,
    msg_headers,
    msg_inv,
    msg_no_witness_block,
    msg_sendcmpct,
)
from test_framework.script import CScript, OP_RETURN
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    p2p_port,
    try_rpc,
)
from test_framework.wallet import MiniWallet


MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16
CACHED_TEST_HEIGHT = 128
DEFAULT_PRUNE_ASSUMEVALID_CACHE_BLOCKS = 10
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


class BlockAnnouncementPeer(P2PInterface):
    def __init__(self):
        super().__init__()
        self.announced_hashes = set()

    def on_cmpctblock(self, message):
        self.announced_hashes.add(message.header_and_shortids.header.hash_int)

    def on_headers(self, message):
        self.announced_hashes.update(header.hash_int for header in message.headers)

    def on_inv(self, message):
        self.announced_hashes.update(inv.hash for inv in message.inv if inv.type == MSG_BLOCK)


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

    def requested_heights(self, peers):
        with p2p_lock:
            return {
                height
                for peer in as_list(peers)
                for height in peer.request_types_by_height
            }

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

    def build_competing_chain(self, prev_block, first_height, final_height):
        blocks = []
        prev_hash = prev_block.hash_int
        block_time = prev_block.nTime
        for height in range(first_height, final_height + 1):
            block_time += 1
            block = create_block(prev_hash, height=height, ntime=block_time)
            block.solve()
            blocks.append(block)
            prev_hash = block.hash_int
        return blocks

    def test_cached_blocks_after_ibd_exit(self, blocks, assumevalid_hash):
        self.log.info("Connect accepted cached children when their parent makes IBD end")
        exit_height = DEFAULT_PRUNE_ASSUMEVALID_CACHE_BLOCKS - 2
        last_cached_height = DEFAULT_PRUNE_ASSUMEVALID_CACHE_BLOCKS - 1
        exit_chainwork = self.nodes[0].getblockheader(blocks[exit_height - 1].hash_hex)["chainwork"]
        self.start_node(6, extra_args=["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}",
                                     f"-minimumchainwork={exit_chainwork}", "-maxtipage=1"])
        node = self.nodes[6]
        self.submit_headers(node, blocks)
        initial_bytes = self.stored_block_bytes(node)
        node.setmocktime(blocks[exit_height - 1].nTime + 1)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0, withheld_heights=[1]))
        with node.assert_debug_log(expected_msgs=[f"Accepted stripped prune-assumevalid block {blocks[last_cached_height - 1].hash_hex}"], timeout=60):
            peer.serve_until_height(last_cached_height)
            peer.send_headers_for_blocks(blocks[-1:])
        peer.sync_with_ping()
        assert_equal(node.getblockcount(), 0)
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        assert_equal(node.submitblock(blocks[2].serialize().hex()), "duplicate")
        assert_equal(self.stored_block_bytes(node), initial_bytes)

        self.log.info("Replace the disconnected peer holding the missing parent")
        peer.peer_disconnect()
        peer.wait_for_disconnect()
        replacement = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        replacement.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(replacement, 1, MSG_BLOCK)
        with node.assert_debug_log(expected_msgs=["Leaving InitialBlockDownload (latching to false)", "-pruneassumevalid inactive: initial block download has ended"]):
            replacement.serve_pending_heights([1])
            self.wait_until(lambda: node.getblockcount() == last_cached_height, timeout=60)
        assert not node.getblockchaininfo()["initialblockdownload"]
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        for result in node.batch([node.getblock.get_request(block.hash_hex) for block in blocks[:last_cached_height]]):
            assert_equal(result["error"], {"code": -1, "message": "Block not available (pruned data)"})

        self.log.info("Refetch outstanding stripped responses in full after IBD ends")
        replacement.serve_until_height(CACHED_TEST_HEIGHT)
        self.wait_until(lambda: node.getblockcount() == CACHED_TEST_HEIGHT, timeout=60)
        for height in range(last_cached_height + 1, CACHED_TEST_HEIGHT + 1):
            assert MSG_BLOCK | MSG_WITNESS_FLAG in self.requested_types(replacement, height)
        assert_greater_than(self.stored_block_bytes(node), initial_bytes)
        self.stop_node(6)

    def test_cached_blocks_after_best_header_change(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Keep the accepted script-skip decision when the best header moves off a cached block")
        self.reset_datadir(6)
        self.start_node(6, extra_args=["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}",
                                     f"-minimumchainwork={final_chainwork}"])
        node = self.nodes[6]
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=CACHED_TEST_HEIGHT))
        peer.send_headers_for_blocks(blocks)
        self.assert_requested(peer, ASSUMEVALID_HEIGHT, MSG_BLOCK)
        peer.serve_pending_heights([ASSUMEVALID_HEIGHT])
        peer.sync_with_ping()
        assert_equal(node.getblockcount(), CACHED_TEST_HEIGHT)
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, assumevalid_hash)

        competing = self.build_competing_chain(blocks[CACHED_TEST_HEIGHT - 1],
                                               first_height=CACHED_TEST_HEIGHT + 1,
                                               final_height=len(blocks) + 1)
        self.submit_headers(node, competing)
        assert_equal(node.getblockchaininfo()["headers"], len(blocks) + 1)
        with node.assert_debug_log(expected_msgs=[
            f"Enabling script verification at block #{ASSUMEVALID_HEIGHT - 1} ({blocks[ASSUMEVALID_HEIGHT - 2].hash_hex}): block not in best header chain.",
            f"Disabling script verification at block #{ASSUMEVALID_HEIGHT} ({assumevalid_hash}).",
        ]):
            peer.serve_pending_heights([ASSUMEVALID_HEIGHT - 1], witness=True)
            self.wait_until(lambda: node.getblockcount() == ASSUMEVALID_HEIGHT, timeout=60)
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, assumevalid_hash)
        self.stop_node(6)

    def test_compact_block_fallback(self, blocks, assumevalid_hash):
        self.log.info("Request a compact block fallback with witness data after the block becomes eligible")
        first_chainwork = self.nodes[0].getblockheader(blocks[0].hash_hex)["chainwork"]
        self.start_node(7, extra_args=["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={first_chainwork}"])
        node = self.nodes[7]
        # Without the assumevalid header, the first block is requested with witness data.
        early_blocks = blocks[:MAX_BLOCKS_IN_TRANSIT_PER_PEER]
        self.submit_headers(node, early_blocks)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        peer.send_without_ping(msg_sendcmpct(announce=False, version=2))
        peer.send_headers_for_blocks(early_blocks)
        self.assert_requested(peer, 1, MSG_BLOCK | MSG_WITNESS_FLAG)
        self.submit_headers(node, blocks[MAX_BLOCKS_IN_TRANSIT_PER_PEER:])

        # A short ID collision makes the node fall back to a full block request.
        compact_block = HeaderAndShortIDs()
        compact_block.initialize_from_block(blocks[0], prefill_list=[0], use_witness=True)
        compact_block.shortids = [0, 0]
        peer.send_and_ping(msg_cmpctblock(compact_block.to_p2p()))
        self.wait_until(lambda: len(self.requested_types(peer, 1)) == 2, timeout=60)
        assert_equal(self.requested_types(peer, 1), [MSG_BLOCK | MSG_WITNESS_FLAG] * 2)
        peer.serve_pending_heights([1])
        self.wait_until(lambda: node.getblockcount() == 1, timeout=60)
        peer.sync_with_ping()
        assert_equal(node.getblock(blocks[0].hash_hex)["height"], 1)
        self.stop_node(7)

        for failure in ["short ID collision", "block transaction reconstruction"]:
            self.log.info(f"Replace an existing stripped request after {failure}")
            self.reset_datadir(7)
            self.start_node(7, extra_args=["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={first_chainwork}"])
            self.submit_headers(node, blocks)
            peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
            peer.send_without_ping(msg_sendcmpct(announce=False, version=2))
            peer.send_headers_for_blocks(blocks[-1:])
            self.assert_requested(peer, 1, MSG_BLOCK)
            compact_block = HeaderAndShortIDs()
            compact_block.initialize_from_block(blocks[0], prefill_list=[] if failure == "block transaction reconstruction" else [0], use_witness=True)
            if failure == "short ID collision":
                compact_block.shortids = [0, 0]
            peer.send_and_ping(msg_cmpctblock(compact_block.to_p2p()))
            if failure == "block transaction reconstruction":
                peer.wait_until(lambda: "getblocktxn" in peer.last_message)
                assert_equal(peer.last_message["getblocktxn"].block_txn_request.to_absolute(), [0])
                incorrect_coinbase = copy.deepcopy(blocks[0].vtx[0])
                incorrect_coinbase.nLockTime ^= 1
                response = msg_blocktxn()
                response.block_transactions = BlockTransactions(blocks[0].hash_int, [incorrect_coinbase])
                peer.send_and_ping(response)
            self.wait_until(lambda: len(self.requested_types(peer, 1)) == 2)
            assert_equal(self.requested_types(peer, 1), [MSG_BLOCK, MSG_BLOCK | MSG_WITNESS_FLAG])

            # The original stripped reply can still arrive after the full fallback request.
            with node.assert_debug_log(expected_msgs=["Ignoring delayed or unsolicited stripped block"]):
                peer.send_and_ping(msg_no_witness_block(blocks[0]))
            assert_equal(node.getblockcount(), 0)
            peer.serve_pending_heights([1])
            self.wait_until(lambda: node.getblockcount() == 1)
            assert_equal(node.getblock(blocks[0].hash_hex)["height"], 1)
            self.stop_node(7)

    def test_presegwit_restart(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Restart omitted pre-SegWit history with the optimization disabled or its anchor changed")
        args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}",
                f"-minimumchainwork={final_chainwork}", f"-testactivationheight=segwit@{ASSUMEVALID_HEIGHT + 1}"]
        self.start_node(8, extra_args=args)
        node = self.nodes[8]
        self.submit_headers(node, blocks)
        initial_bytes = self.stored_block_bytes(node)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=2))
        peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: node.getblockcount() == 2, timeout=60)
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        self.stop_node(8)
        full_args = [arg for arg in args if arg != "-pruneassumevalid"]
        for restart_args in [full_args, [arg for arg in args if not arg.startswith("-assumevalid=")] + ["-assumevalid=0"]]:
            self.start_node(8, extra_args=restart_args)
            assert_equal(node.getblockcount(), 2)
            assert_equal(node.gettxoutsetinfo()["bestblock"], blocks[1].hash_hex)
            assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, blocks[1].hash_hex)
            self.stop_node(8)

        self.log.info("Recover omitted pre-SegWit blocks using the ordinary full reindex path")
        self.start_node(8, extra_args=full_args + ["-reindex"])
        assert_equal(node.getblockcount(), 0)
        self.submit_headers(node, blocks)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=2, force_no_witness=True))
        peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: node.getblockcount() == 2, timeout=60)
        assert_equal(node.getblock(blocks[1].hash_hex)["height"], 2)
        self.restart_node(8, extra_args=full_args)
        assert_equal(node.getblockcount(), 2)
        self.stop_node(8)

    def test_idle_restart_with_updated_assumevalid(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Keep existing full history and normal pruning after a stale node's assumevalid is updated")
        old_height = MAX_BLOCKS_IN_TRANSIT_PER_PEER
        old_chainwork = self.nodes[0].getblockheader(blocks[old_height - 1].hash_hex)["chainwork"]
        old_args = ["-prune=1", f"-minimumchainwork={old_chainwork}"]
        self.start_node(9, extra_args=old_args)
        node = self.nodes[9]
        self.submit_headers(node, blocks[:old_height])
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=old_height))
        peer.send_headers_for_blocks(blocks[:old_height])
        self.wait_until(lambda: node.getblockcount() == old_height, timeout=60)
        assert not node.getblockchaininfo()["initialblockdownload"]
        self.stop_node(9)

        later_time = blocks[-1].nTime + 366 * 24 * 60 * 60
        new_args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}",
                    f"-minimumchainwork={final_chainwork}", f"-mocktime={later_time}"]
        with node.assert_debug_log(expected_msgs=["-pruneassumevalid inactive: existing full block history is available"]):
            self.start_node(9, extra_args=new_args)
        assert node.getblockchaininfo()["initialblockdownload"]
        self.submit_headers(node, blocks)
        old_bytes = self.stored_block_bytes(node)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=old_height + 4))
        peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: node.getblockcount() == old_height + 4, timeout=60)
        for height in range(old_height + 1, old_height + 5):
            assert_equal(self.requested_types(peer, height), [MSG_BLOCK | MSG_WITNESS_FLAG])
            assert_equal(node.getblock(blocks[height - 1].hash_hex)["height"], height)
        assert_greater_than(self.stored_block_bytes(node), old_bytes)
        assert_equal(node.getblock(blocks[old_height - 1].hash_hex)["height"], old_height)
        saved_tip = node.getbestblockhash()
        self.stop_node(9)

        self.log.info("Changing, disabling, or newly learning an anchor does not invalidate committed history")
        competing_anchor = self.build_competing_chain(blocks[0], first_height=2, final_height=2)[0]
        for anchor in ["0", blocks[0].hash_hex, competing_anchor.hash_hex]:
            args = [arg for arg in new_args if not arg.startswith("-assumevalid=")] + [f"-assumevalid={anchor}"]
            self.start_node(9, extra_args=args)
            assert_equal(node.gettxoutsetinfo()["bestblock"], saved_tip)
            node.submitheader(CBlockHeader(competing_anchor).serialize().hex())
            assert_equal(node.getbestblockhash(), saved_tip)
            self.stop_node(9)

    def test_late_stripped_replies(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Ignore a stripped reply after another peer supplies the requested block with witnesses")
        args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}",
                f"-minimumchainwork={final_chainwork}"]
        self.start_node(10, extra_args=args)
        node = self.nodes[10]
        self.submit_headers(node, blocks)
        old_peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=ASSUMEVALID_HEIGHT - 1))
        old_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: node.getblockcount() == ASSUMEVALID_HEIGHT - 1, timeout=120)
        self.assert_requested(old_peer, ASSUMEVALID_HEIGHT, MSG_BLOCK)

        self.log.info("Validate witnesses when a stripped request receives a full block")
        bad_witness_block = copy.deepcopy(blocks[ASSUMEVALID_HEIGHT - 1])
        witness_stack = bad_witness_block.vtx[0].wit.vtxinwit[0].scriptWitness.stack
        witness_stack[0] = witness_stack[0][:-1] + bytes([witness_stack[0][-1] ^ 1])
        with node.assert_debug_log(expected_msgs=["Received mutated block"]):
            old_peer.send_without_ping(msg_block(bad_witness_block))
            old_peer.wait_for_disconnect()
        assert_equal(node.getblockcount(), ASSUMEVALID_HEIGHT - 1)

        old_peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=ASSUMEVALID_HEIGHT - 1))
        old_peer.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(old_peer, ASSUMEVALID_HEIGHT, MSG_BLOCK)
        full_peer = node.add_p2p_connection(P2PInterface())
        full_peer.send_and_ping(msg_block(blocks[ASSUMEVALID_HEIGHT - 1]))
        self.wait_until(lambda: node.getblockcount() == ASSUMEVALID_HEIGHT, timeout=60)
        with p2p_lock:
            old_request = next(inv for inv in old_peer.pending_getdata if inv.hash == blocks[ASSUMEVALID_HEIGHT - 1].hash_int and inv.type == MSG_BLOCK)
        with node.assert_debug_log(expected_msgs=["Ignoring delayed or unsolicited stripped block"]):
            old_peer._send_block(old_request)
            old_peer.sync_with_ping()
        assert_equal(node.getblock(blocks[ASSUMEVALID_HEIGHT - 1].hash_hex)["height"], ASSUMEVALID_HEIGHT)

        self.log.info("Reject a merkle-mutated stripped reply even after its request was canceled")
        bad_merkle_block = copy.deepcopy(blocks[ASSUMEVALID_HEIGHT - 1])
        bad_merkle_block.vtx[0].nLockTime ^= 1
        with node.assert_debug_log(expected_msgs=["Received mutated block"]):
            old_peer.send_without_ping(msg_no_witness_block(bad_merkle_block))
            old_peer.wait_for_disconnect()
        assert_equal(node.getblockcount(), ASSUMEVALID_HEIGHT)
        self.stop_node(10)

    def test_unconnected_cached_restart(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Refetch an unconnected stripped block after restart without prune-assumevalid")
        args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}",
                f"-minimumchainwork={final_chainwork}"]
        self.start_node(11, extra_args=args)
        node = self.nodes[11]
        self.submit_headers(node, blocks)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        peer.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(peer, 2, MSG_BLOCK)
        with node.assert_debug_log(expected_msgs=["Accepted stripped prune-assumevalid block"]):
            peer.serve_pending_heights([2])
            peer.sync_with_ping()
        assert_equal(node.getblockcount(), 0)
        self.stop_node(11)

        full_args = [arg for arg in args if arg != "-pruneassumevalid"]
        self.start_node(11, extra_args=full_args)
        node = self.nodes[11]
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=2))
        peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: node.getblockcount() == 2, timeout=60)
        self.assert_requested(peer, 2, MSG_BLOCK | MSG_WITNESS_FLAG)
        assert_equal(node.getblock(blocks[1].hash_hex)["height"], 2)
        self.stop_node(11)
        self.start_node(11, extra_args=full_args)
        assert_equal(self.nodes[11].getblockcount(), 2)
        self.stop_node(11)

    def test_announcements_after_ibd_exit(self, blocks, assumevalid_hash):
        self.log.info("Do not announce an omitted tip after IBD ends")
        self.reset_datadir(11)
        exit_chainwork = self.nodes[0].getblockheader(blocks[2].hash_hex)["chainwork"]
        args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}",
                f"-minimumchainwork={exit_chainwork}"]
        blocknotify_dir = os.path.join(self.options.tmpdir, "prune_blocknotify")
        os.mkdir(blocknotify_dir)
        args.append(f"-blocknotify=echo > \"{os.path.join(blocknotify_dir, '%s')}\"")
        if self.is_zmq_compiled():
            # Node 10 has stopped and will not restart while this notifier is active.
            args.append(f"-zmqpubrawblock=tcp://127.0.0.1:{p2p_port(10)}")
        self.start_node(11, extra_args=args)
        node = self.nodes[11]
        assert_equal(node.getblockcount(), 0)
        observer = node.add_p2p_connection(BlockAnnouncementPeer())
        observer.send_without_ping(msg_sendcmpct(announce=True, version=2))
        observer.send_and_ping(msg_inv([CInv(MSG_BLOCK, blocks[1].hash_int)]))
        self.submit_headers(node, blocks)
        source = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=3))
        source.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: node.getblockcount() == 3, timeout=60)
        assert not node.getblockchaininfo()["initialblockdownload"]
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, blocks[2].hash_hex)
        assert_equal(node.getblocktemplate({"rules": ["segwit"]})["previousblockhash"], blocks[2].hash_hex)
        node.syncwithvalidationinterfacequeue()
        self.wait_until(lambda: os.path.exists(os.path.join(blocknotify_dir, blocks[2].hash_hex)), timeout=10)
        observer.sync_with_ping()
        with p2p_lock:
            assert blocks[2].hash_int not in observer.announced_hashes
        if self.is_zmq_compiled():
            assert_equal([notification["type"] for notification in node.getzmqnotifications()], ["pubrawblock"])

        self.log.info("Announce the next fully stored block")
        source.serve_until_height(4)
        self.wait_until(lambda: node.getblockcount() == 4, timeout=60)
        self.assert_requested(source, 4, MSG_BLOCK | MSG_WITNESS_FLAG)
        node.syncwithvalidationinterfacequeue()
        self.wait_until(lambda: os.path.exists(os.path.join(blocknotify_dir, blocks[3].hash_hex)), timeout=10)
        observer.sync_with_ping()
        with p2p_lock:
            assert blocks[3].hash_int in observer.announced_hashes
        if self.is_zmq_compiled():
            assert_equal([notification["type"] for notification in node.getzmqnotifications()], ["pubrawblock"])

        self.log.info("Verify stored descendants after refetching an omitted ancestor without undo data")
        source_peer_id = max(peer["id"] for peer in node.getpeerinfo())
        assert_equal(node.getblockfrompeer(blocks[2].hash_hex, source_peer_id), {})
        self.wait_until(lambda: not try_rpc(-1, "Block not available (pruned data)", node.getblock, blocks[2].hash_hex), timeout=60)
        with node.assert_debug_log(expected_msgs=["Block verification stopping at height 3 (no undo data)"]):
            self.restart_node(11, extra_args=args + ["-checkblocks=0", "-checklevel=4"])
        assert_equal(self.nodes[11].getblockcount(), 4)
        assert_equal(self.nodes[11].getblock(blocks[3].hash_hex)["height"], 4)
        self.stop_node(11)

    def test_cache_limit(self, blocks, final_chainwork):
        self.log.info("Bound cached and requested stripped blocks and recover the first missing parent without writing blocks")
        node = self.nodes[3]
        genesis = from_hex(CBlock(), self.nodes[0].getblock(self.nodes[0].getblockhash(0), 0))
        budget_blocks = []
        previous = genesis
        for height in range(1, 501 + BURIAL_BLOCKS):
            padding = create_coinbase(height, script_pubkey=CScript([OP_RETURN, bytes(900_000)])) if height <= DEFAULT_PRUNE_ASSUMEVALID_CACHE_BLOCKS else None
            block = create_block(previous.hash_int, padding, height=height, ntime=blocks[0].nTime + height)
            block.solve()
            budget_blocks.append(block)
            previous = block
        args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={budget_blocks[499].hash_hex}",
                f"-minimumchainwork={final_chainwork}", "-dbcache=4", "-dbbatchsize=1"]
        self.start_node(3, extra_args=args)
        node.setmocktime(budget_blocks[-1].nTime)
        self.submit_headers(node, budget_blocks)
        initial_bytes = self.stored_block_bytes(node)
        with node.assert_debug_log(expected_msgs=["Pausing stripped downloads"], timeout=120):
            peer = node.add_p2p_connection(AssumeValidBlockStore(budget_blocks, max_height_to_serve=0, withheld_heights=[1]))
            peer.serve_until_height(500)
            peer.send_headers_for_blocks(budget_blocks[-1:])
            self.wait_until(lambda: max(self.requested_heights(peer), default=0) == DEFAULT_PRUNE_ASSUMEVALID_CACHE_BLOCKS - 1, timeout=120)
        self.wait_until(lambda: node.getpeerinfo()[0]["inflight"] == [1], timeout=120)
        assert_equal(max(self.requested_heights(peer)), DEFAULT_PRUNE_ASSUMEVALID_CACHE_BLOCKS - 1)
        assert_equal(node.getblockcount(), 0)
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        with p2p_lock:
            assert_equal({request_type for request_types in peer.request_types_by_height.values() for request_type in request_types}, {MSG_BLOCK})

        self.log.info("Use the ordinary stalling timeout to recover the parent while the cache is full")
        staller_id = node.getpeerinfo()[0]["id"]
        with node.assert_debug_log(expected_msgs=[f"Stall started peer={staller_id}"], timeout=60):
            replacement = node.add_p2p_connection(AssumeValidBlockStore(budget_blocks, max_height_to_serve=500), services=NODE_NETWORK)
            replacement.send_headers_for_blocks(budget_blocks[-1:])
            replacement.sync_with_ping()
        # Each late block from the stalling peer restarts its stall timer, so keep advancing time until it is dropped.
        def staller_disconnected():
            node.bumpmocktime(3)
            return not peer.is_connected

        self.wait_until(staller_disconnected)
        self.assert_requested(replacement, 1, MSG_BLOCK)
        self.wait_until(lambda: node.getblockcount() == 500, timeout=120)
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        for height in range(2, 501):
            assert_equal(self.requested_types([peer, replacement], height), [MSG_BLOCK])

        self.log.info("Complete a multi-batch UTXO flush without retaining block or undo files")
        with node.assert_debug_log(expected_msgs=["Writing partial batch", "Writing final batch"]):
            assert_equal(node.gettxoutsetinfo()["bestblock"], budget_blocks[499].hash_hex)
        self.restart_node(3, extra_args=["-prune=1", "-assumevalid=0", "-dbcache=8"])
        assert_equal(node.getblockcount(), 500)
        assert_equal(node.gettxoutsetinfo()["bestblock"], budget_blocks[499].hash_hex)
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, budget_blocks[499].hash_hex)
        self.stop_node(3)

    def test_stored_history_fallback(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Existing full blocks disable stripped fetching even from a non-witness peer")
        self.reset_datadir(3)
        self.start_node(3, extra_args=["-prune=1"])
        node = self.nodes[3]
        node.submitblock(blocks[0].serialize().hex())
        self.stop_node(3)
        args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={final_chainwork}"]
        self.start_node(3, extra_args=args)
        self.submit_headers(node, blocks)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=2), services=NODE_NETWORK)
        peer.send_headers_for_blocks(blocks[-1:])
        peer.sync_with_ping()
        assert_equal(self.requested_types(peer, 2), [])
        witness_peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=2))
        witness_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: node.getblockcount() == 2, timeout=60)
        self.assert_requested(witness_peer, 2, MSG_BLOCK | MSG_WITNESS_FLAG)
        assert_equal(node.getblock(blocks[1].hash_hex)["height"], 2)
        self.stop_node(3)

    def test_wallet_notification_without_witness(self, blocks, assumevalid_hash, spend_txid, final_chainwork):
        if not self.is_wallet_compiled():
            return

        self.log.info("Record the stripped transaction variant in a watch-only wallet")
        self.reset_datadir(7)
        full_wtxid = self.nodes[0].getrawtransaction(spend_txid, True, assumevalid_hash)["hash"]
        full_hex = self.nodes[0].getrawtransaction(spend_txid, False, assumevalid_hash)
        descriptor = MiniWallet(self.nodes[0]).get_descriptor()
        args = ["-disablewallet=0", "-prune=1", "-pruneassumevalid",
                f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={final_chainwork}"]
        self.start_node(7, extra_args=args)
        node = self.nodes[7]
        node.createwallet("watch", disable_private_keys=True, blank=True, load_on_startup=True)
        wallet = node.get_wallet_rpc("watch")
        assert_equal(wallet.importdescriptors([{"desc": descriptor, "timestamp": 0}])[0]["success"], True)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=ASSUMEVALID_HEIGHT - 1))
        peer.send_headers_for_blocks(blocks)
        self.wait_until(lambda: node.getblockcount() == ASSUMEVALID_HEIGHT - 1, timeout=60)
        assert_equal(node.sendrawtransaction(full_hex), spend_txid)
        assert_equal(wallet.gettransaction(spend_txid)["wtxid"], full_wtxid)
        peer.serve_until_height(ASSUMEVALID_HEIGHT)
        self.wait_until(lambda: node.getblockcount() == ASSUMEVALID_HEIGHT, timeout=60)
        transaction = wallet.gettransaction(spend_txid, False, True)
        assert full_wtxid != spend_txid
        assert_equal(transaction["wtxid"], spend_txid)
        assert "txinwitness" not in transaction["decoded"]["vin"][0]
        self.restart_node(7, extra_args=args)
        wallet = self.nodes[7].get_wallet_rpc("watch")
        assert_equal(wallet.gettransaction(spend_txid)["wtxid"], spend_txid)
        assert_raises_rpc_error(-1, "Can't rescan beyond pruned data", wallet.rescanblockchain, 0, 2)
        self.stop_node(7)

    def test_reorg_into_omitted_history(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Use the ordinary fatal disconnect path when a reorg requires omitted history")
        self.reset_datadir(3)
        args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={final_chainwork}"]
        self.start_node(3, extra_args=args)
        node = self.nodes[3]
        self.submit_headers(node, blocks)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=2))
        peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: node.getblockcount() == 2, timeout=60)
        assert_equal(node.gettxoutsetinfo()["bestblock"], blocks[1].hash_hex)
        genesis = from_hex(CBlock(), self.nodes[0].getblock(self.nodes[0].getblockhash(0), 0))
        competing = self.build_competing_chain(genesis, first_height=1, final_height=3)
        for block in competing[:2]:
            assert_equal(node.submitblock(block.serialize().hex()), "inconclusive")
        with node.assert_debug_log(expected_msgs=["Failed to disconnect block"]):
            node.submitblock(competing[2].serialize().hex())
            node.wait_until_stopped(timeout=60, expect_error=True, expected_stderr="Error: A fatal internal error occurred, see debug.log for details: Failed to disconnect block.")
        self.start_node(3, extra_args=["-prune=1", "-assumevalid=0", "-reindex"])
        self.wait_until(lambda: node.getblockcount() == 3, timeout=60)
        assert_equal(node.gettxoutsetinfo()["bestblock"], competing[2].hash_hex)
        self.stop_node(3)

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

        self.log.info("Require full reindex after a crash interrupts an ephemeral chainstate flush")
        crash_args = prune_assumevalid_args + ["-dbbatchsize=1", "-dbcrashratio=1"]
        self.start_node(4, extra_args=crash_args)
        flush_crash_node = self.nodes[4]
        self.submit_headers(flush_crash_node, blocks)
        flush_crash_peer = flush_crash_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=2))
        flush_crash_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: flush_crash_node.getblockcount() == 2, timeout=60)
        try:
            flush_crash_node.gettxoutsetinfo()
        except Exception:
            pass
        flush_crash_node.wait_until_stopped(timeout=60)
        for extra_args in (prune_assumevalid_args, prune_assumevalid_args + ["-assumevalid=0"]):
            flush_crash_node.assert_start_raises_init_error(
                extra_args=extra_args,
                expected_msg="The chainstate flush was interrupted. A full -reindex is required to redownload omitted blocks.\nPlease restart with -reindex to recover.",
            )
        assert "Replaying blocks" not in flush_crash_node.debug_log_path.read_text(encoding="utf-8")
        self.start_node(4, extra_args=prune_assumevalid_args + ["-reindex"])
        assert_equal(flush_crash_node.getblockcount(), 0)
        self.submit_headers(flush_crash_node, blocks)
        reindex_peer = flush_crash_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=MAX_BLOCKS_IN_TRANSIT_PER_PEER))
        reindex_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: flush_crash_node.getblockcount() == MAX_BLOCKS_IN_TRANSIT_PER_PEER, timeout=60)
        for height in range(1, MAX_BLOCKS_IN_TRANSIT_PER_PEER + 1):
            assert_equal(self.requested_types(reindex_peer, height), [MSG_BLOCK])
        self.stop_node(4)

        self.log.info("Store a normally requested historical block instead of caching it ephemerally")
        presegwit_args = prune_assumevalid_args + [f"-testactivationheight=segwit@{ASSUMEVALID_HEIGHT + 1}"]
        self.start_node(5, extra_args=presegwit_args)
        refetch_node = self.nodes[5]
        self.submit_headers(refetch_node, blocks)
        refetch_bytes = self.stored_block_bytes(refetch_node)
        refetch_peer = refetch_node.add_p2p_connection(AssumeValidBlockStore(
            blocks,
            max_height_to_serve=1,
            force_no_witness=True,
        ))
        refetch_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: refetch_node.getblockcount() == 1, timeout=60)
        assert_equal(self.stored_block_bytes(refetch_node), refetch_bytes)
        peer_id = refetch_node.getpeerinfo()[0]["id"]
        assert_equal(refetch_node.getblockfrompeer(block_hashes[0], peer_id), {})
        self.wait_until(lambda: not try_rpc(-1, "Block not available (pruned data)", refetch_node.getblock, block_hashes[0]), timeout=60)
        assert_equal(self.requested_types(refetch_peer, 1)[-1], MSG_BLOCK | MSG_WITNESS_FLAG)
        with refetch_node.assert_debug_log(expected_msgs=["Block verification stopping at height 1 (no undo data)"]):
            self.restart_node(5, extra_args=presegwit_args + ["-checkblocks=0", "-checklevel=4"])
        assert_equal(self.nodes[5].getblock(block_hashes[0])["height"], 1)
        self.stop_node(5)

        self.log.info("Fetch an eligible stripped block from a peer without witness support")
        self.reset_datadir(5)
        self.start_node(5, extra_args=presegwit_args)
        no_witness_node = self.nodes[5]
        no_witness_peer = no_witness_node.add_p2p_connection(
            AssumeValidBlockStore(blocks, max_height_to_serve=2), services=NODE_NETWORK)
        no_witness_peer.send_headers_for_blocks(blocks)
        self.wait_until(lambda: no_witness_node.getblockcount() == 2, timeout=60)
        assert_equal(self.requested_types(no_witness_peer, 2), [MSG_BLOCK])
        self.stop_node(5)

        self.log.info("Sync assumevalid ancestors as stripped ephemeral blocks")
        self.start_node(1, extra_args=[arg for arg in prune_assumevalid_args if arg != "-prune=1"])
        prune_assumevalid_node = self.nodes[1]
        info = prune_assumevalid_node.getblockchaininfo()
        assert_equal(info["pruned"], True)
        assert_equal(info["automatic_pruning"], True)
        assert_equal(info["prune_target_size"], 550 * 1024 * 1024)
        self.submit_headers(prune_assumevalid_node, blocks)
        initial_bytes = self.stored_block_bytes(prune_assumevalid_node)
        prune_assumevalid_node.setmocktime(1_700_000_000)
        out_of_order_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        out_of_order_peer.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(out_of_order_peer, 2, MSG_BLOCK)
        out_of_order_peer.sync_with_ping()
        assert_greater_than_or_equal(DEFAULT_PRUNE_ASSUMEVALID_CACHE_BLOCKS - 1, len(self.requested_heights(out_of_order_peer)))
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)

        self.log.info("Connect out-of-order stripped blocks from the transient cache")
        out_of_order_peer.serve_pending_heights([2])
        out_of_order_peer.sync_with_ping()
        assert_equal(prune_assumevalid_node.getblockcount(), 0)
        assert_equal(len(self.requested_types(out_of_order_peer, 2)), 1)
        out_of_order_peer.serve_pending_heights([1])
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == 2, timeout=60)
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)

        checkpoint = prune_assumevalid_node.gettxoutsetinfo("muhash")
        assert_equal(checkpoint["bestblock"], block_hashes[1])
        prune_assumevalid_node.kill_process()

        self.log.info("Resume at the persisted UTXO height after an unclean shutdown")
        self.start_node(1, extra_args=prune_assumevalid_args)
        prune_assumevalid_node = self.nodes[1]
        self.submit_headers(prune_assumevalid_node, blocks)
        first_restart_height = prune_assumevalid_node.getblockcount()
        assert_equal(first_restart_height, 2)
        assert_equal(prune_assumevalid_node.gettxoutsetinfo("muhash")["muhash"], checkpoint["muhash"])
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)
        crash_height = MAX_BLOCKS_IN_TRANSIT_PER_PEER
        crash_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=crash_height))
        crash_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == crash_height, timeout=120)
        for height in range(first_restart_height + 1, crash_height + 1):
            assert_equal(set(self.requested_types(crash_peer, height)), {MSG_BLOCK})
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)

        self.log.info("Discard unflushed connections and redownload from the persisted height")
        prune_assumevalid_node.kill_process()
        self.start_node(1, extra_args=prune_assumevalid_args)
        assert_equal(prune_assumevalid_node.getblockcount(), 2)
        assert_equal(prune_assumevalid_node.gettxoutsetinfo("muhash")["muhash"], checkpoint["muhash"])
        self.submit_headers(prune_assumevalid_node, blocks)
        recovery_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=crash_height))
        recovery_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == crash_height, timeout=120)
        for height in range(3, crash_height + 1):
            assert_equal(set(self.requested_types(recovery_peer, height)), {MSG_BLOCK})
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)

        self.log.info("Keep the exact UTXO tip across a clean stop without block files")
        assert_equal(prune_assumevalid_node.gettxoutsetinfo()["bestblock"], block_hashes[crash_height - 1])
        self.restart_node(1, extra_args=prune_assumevalid_args)
        prune_assumevalid_node = self.nodes[1]
        assert_equal(prune_assumevalid_node.getblockcount(), crash_height)
        assert_equal(prune_assumevalid_node.gettxoutsetinfo()["bestblock"], block_hashes[crash_height - 1])
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)
        prune_assumevalid_node.kill_process()

        self.start_node(1, extra_args=prune_assumevalid_args)
        prune_assumevalid_node = self.nodes[1]
        self.submit_headers(prune_assumevalid_node, blocks)
        restart_height = prune_assumevalid_node.getblockcount()
        assert_greater_than_or_equal(crash_height, restart_height)
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)
        prune_assumevalid_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=ASSUMEVALID_HEIGHT))
        prune_assumevalid_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == ASSUMEVALID_HEIGHT, timeout=120)

        for height in range(restart_height + 1, ASSUMEVALID_HEIGHT + 1):
            assert_equal(set(self.requested_types(prune_assumevalid_peer, height)), {MSG_BLOCK})
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)
        assert prune_assumevalid_node.gettxout(spend_txid, 0) is not None
        assert prune_assumevalid_node.gettxout(spent_txid, spent_vout) is None
        assert_raises_rpc_error(-1, "Block not available (pruned data)", prune_assumevalid_node.getblock, assumevalid_hash)

        self.log.info("Advertise NODE_NETWORK_LIMITED without serving omitted recent blocks during IBD")
        serving_peer = prune_assumevalid_node.add_p2p_connection(P2PInterface())
        assert_equal(serving_peer.nServices & (NODE_NETWORK | NODE_NETWORK_LIMITED | NODE_WITNESS), NODE_NETWORK_LIMITED | NODE_WITNESS)
        assert prune_assumevalid_node.getblockchaininfo()["initialblockdownload"]
        serving_peer.send_and_ping(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, blocks[ASSUMEVALID_HEIGHT - 1].hash_int)]))
        assert "block" not in serving_peer.last_message
        block_txn_request = msg_getblocktxn()
        block_txn_request.block_txn_request = BlockTransactionsRequest(blocks[ASSUMEVALID_HEIGHT - 1].hash_int, [0])
        serving_peer.send_and_ping(block_txn_request)
        assert "blocktxn" not in serving_peer.last_message

        self.log.info("Resume normal witness download and block storage after the assumevalid height")
        prune_assumevalid_peer.serve_until_height(ASSUMEVALID_HEIGHT + 1)
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == ASSUMEVALID_HEIGHT + 1, timeout=60)
        self.assert_requested(prune_assumevalid_peer, ASSUMEVALID_HEIGHT + 1, MSG_BLOCK | MSG_WITNESS_FLAG)
        assert_greater_than(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)
        assert_equal(prune_assumevalid_node.getblock(block_hashes[ASSUMEVALID_HEIGHT])["height"], ASSUMEVALID_HEIGHT + 1)
        assert_raises_rpc_error(-1, "Block not available (pruned data)", prune_assumevalid_node.getblock, assumevalid_hash)
        serving_peer.send_and_ping(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, blocks[ASSUMEVALID_HEIGHT].hash_int)]))
        assert_equal(serving_peer.last_message["block"].block.hash_int, blocks[ASSUMEVALID_HEIGHT].hash_int)
        block_txn_request.block_txn_request = BlockTransactionsRequest(blocks[ASSUMEVALID_HEIGHT].hash_int, [0])
        serving_peer.send_and_ping(block_txn_request)
        response = serving_peer.last_message["blocktxn"].block_transactions
        assert_equal(response.blockhash, blocks[ASSUMEVALID_HEIGHT].hash_int)
        assert_equal([tx.serialize() for tx in response.transactions], [blocks[ASSUMEVALID_HEIGHT].vtx[0].serialize()])

        self.log.info("Restart safely with historical assumevalid blocks missing from disk")
        with self.nodes[1].assert_debug_log(expected_msgs=[f"Block verification stopping at height {ASSUMEVALID_HEIGHT} (no data)"]):
            self.restart_node(1, extra_args=prune_assumevalid_args + ["-checkblocks=0", "-checklevel=4"])
        prune_assumevalid_node = self.nodes[1]
        assert_equal(prune_assumevalid_node.getblockcount(), ASSUMEVALID_HEIGHT + 1)
        assert prune_assumevalid_node.gettxout(spend_txid, 0) is not None
        assert_raises_rpc_error(-1, "Block not available (pruned data)", prune_assumevalid_node.getblock, assumevalid_hash)

        self.log.info("Restart committed omitted history with different optimization and assumevalid settings")
        for args in [
            [arg for arg in prune_assumevalid_args if arg != "-pruneassumevalid"],
            [arg for arg in prune_assumevalid_args if not arg.startswith("-assumevalid=")] + ["-assumevalid=0"],
            [arg for arg in prune_assumevalid_args if not arg.startswith("-assumevalid=")] + [f"-assumevalid={block_hashes[0]}"],
        ]:
            self.restart_node(1, extra_args=args)
            assert_equal(prune_assumevalid_node.getblockcount(), ASSUMEVALID_HEIGHT + 1)
            assert prune_assumevalid_node.gettxout(spend_txid, 0) is not None
        self.restart_node(1, extra_args=prune_assumevalid_args)

        self.log.info("Continue after restart without rereading blocks pruned by -pruneassumevalid")
        restart_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=ASSUMEVALID_HEIGHT + 2))
        restart_peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == ASSUMEVALID_HEIGHT + 2, timeout=60)
        self.assert_requested(restart_peer, ASSUMEVALID_HEIGHT + 2, MSG_BLOCK | MSG_WITNESS_FLAG)

        self.log.info("Request a competing post-assumevalid block with witness data")
        prev_block = blocks[ASSUMEVALID_HEIGHT + 1]
        competing_blocks = self.build_competing_chain(
            prev_block,
            first_height=ASSUMEVALID_HEIGHT + 3,
            final_height=len(blocks) + 1,
        )
        competing_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(competing_blocks, max_height_to_serve=0, first_height=ASSUMEVALID_HEIGHT + 3))
        competing_peer.send_headers_for_blocks(competing_blocks)
        self.assert_requested(competing_peer, ASSUMEVALID_HEIGHT + 3, MSG_BLOCK | MSG_WITNESS_FLAG)

        self.test_cached_blocks_after_ibd_exit(blocks, assumevalid_hash)
        self.test_cached_blocks_after_best_header_change(blocks, assumevalid_hash, final_chainwork)
        self.test_compact_block_fallback(blocks, assumevalid_hash)
        self.test_presegwit_restart(blocks, assumevalid_hash, final_chainwork)
        self.test_idle_restart_with_updated_assumevalid(blocks, assumevalid_hash, final_chainwork)
        self.test_late_stripped_replies(blocks, assumevalid_hash, final_chainwork)
        self.test_unconnected_cached_restart(blocks, assumevalid_hash, final_chainwork)
        self.test_announcements_after_ibd_exit(blocks, assumevalid_hash)
        self.test_cache_limit(blocks, final_chainwork)
        self.test_stored_history_fallback(blocks, assumevalid_hash, final_chainwork)
        self.test_wallet_notification_without_witness(blocks, assumevalid_hash, spend_txid, final_chainwork)
        self.test_reorg_into_omitted_history(blocks, assumevalid_hash, final_chainwork)


if __name__ == "__main__":
    FeaturePruneAssumeValidTest(__file__).main()
