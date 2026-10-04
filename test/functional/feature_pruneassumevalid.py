#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test prune-assumevalid IBD mode."""

import copy
import os
import shutil

from test_framework.blocktools import (
    COINBASE_MATURITY,
    create_block,
    create_coinbase,
)
from test_framework.messages import (
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
    msg_cmpctblock,
    msg_generic,
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
from test_framework.test_node import ErrorMatch
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
BURIAL_BLOCKS = 2100


def as_list(peers):
    return peers if isinstance(peers, list) else [peers]


def prepared_child_count(threads):
    """Children prepared while height 1 is withheld, with two bodies per worker"""
    return max(0, 2 * threads - 1)


def pav_args(assumevalid, chainwork, *extra):
    """Arguments for a pruned node that omits eligible blocks"""
    return ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid}", f"-minimumchainwork={chainwork}", *extra]


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
        self.block_download_window = self.config.getint("net", "BLOCK_DOWNLOAD_WINDOW")
        self.assumevalid_height = max(COINBASE_MATURITY + 2, self.block_download_window + 2)

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
        self.wait_until(lambda: expected_type in self.requested_types(peers, height))

    def requested_heights(self, peers):
        with p2p_lock:
            return {
                height
                for peer in as_list(peers)
                for height in peer.request_types_by_height
            }

    def assert_omitted(self, node, block_hash):
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, block_hash)

    def chainwork(self, block):
        return self.nodes[0].getblockheader(block.hash_hex)["chainwork"]

    def sync_from_store(self, node, blocks, height, *, timeout=60, **store_args):
        """Connect a peer serving blocks up to height, announce its chain, and wait until the node is synced to height"""
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=height, **store_args))
        peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: node.getblockcount() == height, timeout=timeout)
        return peer

    def submit_headers(self, node, blocks):
        results = node.batch([node.submitheader.get_request(CBlockHeader(block).serialize().hex()) for block in blocks])
        assert all(result.get("error") is None for result in results)

    def build_source_chain(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        self.log.info("Mine a buried assumevalid chain with a witness spend at the assumevalid height")
        self.generate(wallet, self.block_download_window, sync_fun=self.no_op)
        # The UTXO set of a node synced through the first download window
        window_muhash = node.gettxoutsetinfo("muhash")["muhash"]
        self.generate(wallet, self.assumevalid_height - 1 - self.block_download_window, sync_fun=self.no_op)
        spent_utxo = wallet.get_utxo(confirmed_only=True)
        spend_tx = wallet.send_self_transfer(from_node=node, utxo_to_spend=spent_utxo)
        assumevalid_hash = self.generate(wallet, 1, sync_fun=self.no_op)[0]
        assert_equal(node.getblockcount(), self.assumevalid_height)
        self.generate(wallet, BURIAL_BLOCKS, sync_fun=self.no_op)

        block_hashes = node.batch([node.getblockhash.get_request(height) for height in range(1, node.getblockcount() + 1)])
        blocks = [from_hex(CBlock(), result["result"]) for result in node.batch([node.getblock.get_request(block_hash["result"], 0) for block_hash in block_hashes])]
        assumevalid_block = blocks[self.assumevalid_height - 1]
        assert any(not tx.wit.is_null() for tx in assumevalid_block.vtx[1:])
        return blocks, assumevalid_hash, spend_tx["txid"], spent_utxo["txid"], spent_utxo["vout"], window_muhash

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

    def test_queued_blocks_after_ibd_exit(self, blocks, assumevalid_hash):
        self.log.info("Connect queued children until their parent chain makes IBD end")
        exit_height = self.block_download_window - 2
        last_queued_height = self.block_download_window - 1
        exit_chainwork = self.chainwork(blocks[exit_height - 1])
        self.start_node(6, extra_args=pav_args(assumevalid_hash, exit_chainwork, "-maxtipage=1"))
        node = self.nodes[6]
        self.submit_headers(node, blocks)
        initial_bytes = self.stored_block_bytes(node)
        node.setmocktime(blocks[exit_height - 1].nTime + 1)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0, withheld_heights=[1]))
        with node.assert_debug_log(expected_msgs=[f"Queued prune-assumevalid block {blocks[last_queued_height - 1].hash_hex}"], timeout=60):
            peer.serve_until_height(last_queued_height)
            peer.send_headers_for_blocks(blocks[-1:])
        peer.sync_with_ping()
        assert_equal(node.getblockcount(), 0)
        assert_equal(self.stored_block_bytes(node), initial_bytes)

        self.log.info("Replace the disconnected peer holding the missing parent")
        peer.peer_disconnect()
        peer.wait_for_disconnect()
        replacement = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        replacement.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(replacement, 1, MSG_BLOCK)
        with node.assert_debug_log(expected_msgs=["Leaving InitialBlockDownload (latching to false)", "-pruneassumevalid inactive:"]):
            replacement.serve_pending_heights([1])
            self.wait_until(lambda: node.getblockcount() == exit_height)
        assert not node.getblockchaininfo()["initialblockdownload"]
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        for result in node.batch([node.getblock.get_request(block.hash_hex) for block in blocks[:exit_height]]):
            assert_equal(result["error"], {"code": -1, "message": "Block not available (pruned data)"})

        self.log.info("Refetch queued and outstanding stripped blocks in full after IBD ends")
        # Refill during connection can use every peer slot, so answer two outstanding stripped requests before waiting for full retries.
        replacement.serve_pending_heights([self.block_download_window, self.block_download_window + 1])
        self.assert_requested(replacement, last_queued_height, MSG_BLOCK | MSG_WITNESS_FLAG)
        self.assert_requested(replacement, self.block_download_window, MSG_BLOCK | MSG_WITNESS_FLAG)
        # Store the later full block first so connecting its parent uses disk read-ahead after IBD
        replacement.serve_pending_heights([self.block_download_window])
        replacement.sync_with_ping()
        assert_equal(node.getblockcount(), exit_height)
        replacement.serve_until_height(self.block_download_window)
        self.wait_until(lambda: node.getblockcount() == self.block_download_window)
        for height in range(exit_height + 1, self.block_download_window + 1):
            assert MSG_BLOCK | MSG_WITNESS_FLAG in self.requested_types(replacement, height)
            assert_equal(node.getblock(blocks[height - 1].hash_hex)["height"], height)
        assert_greater_than(self.stored_block_bytes(node), initial_bytes)
        self.stop_node(6)

    def test_queued_blocks_after_best_header_change(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Refetch a queued block with witnesses after the best header moves off it")
        self.reset_datadir(6)
        self.start_node(6, extra_args=pav_args(assumevalid_hash, final_chainwork))
        node = self.nodes[6]
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=self.block_download_window))
        # Let getheaders drive initial sync without overlapping header announcements.
        self.assert_requested(peer, self.assumevalid_height, MSG_BLOCK)
        peer.serve_pending_heights([self.assumevalid_height])
        peer.sync_with_ping()
        assert_equal(node.getblockcount(), self.block_download_window)
        # Preparing a queued body does not mark it accepted in the block index.
        assert_raises_rpc_error(-1, "Block not available (not fully downloaded)", node.getblock, assumevalid_hash)

        competing = self.build_competing_chain(blocks[self.block_download_window - 1],
                                               first_height=self.block_download_window + 1,
                                               final_height=len(blocks) + 1)
        self.submit_headers(node, competing)
        assert_equal(node.getblockchaininfo()["headers"], len(blocks) + 1)
        with node.assert_debug_log(expected_msgs=[
            f"Enabling script verification at block #{self.assumevalid_height - 1} ({blocks[self.assumevalid_height - 2].hash_hex}): block not in best header chain.",
        ]):
            peer.serve_pending_heights([self.assumevalid_height - 1], witness=True)
            self.wait_until(lambda: node.getblockcount() == self.assumevalid_height - 1)
        # Draining before request refill lets this peer immediately request the dropped block with witnesses.
        self.assert_requested(peer, self.assumevalid_height, MSG_BLOCK | MSG_WITNESS_FLAG)
        peer.serve_pending_heights([self.assumevalid_height])
        self.wait_until(lambda: node.getblockcount() == self.assumevalid_height)
        assert_equal(self.requested_types(peer, self.assumevalid_height), [MSG_BLOCK, MSG_BLOCK | MSG_WITNESS_FLAG])
        assert_equal(node.getblock(assumevalid_hash)["height"], self.assumevalid_height)
        self.stop_node(6)

        self.log.info("Refetch a queued child to reorg away from a stored stale-fork tip")
        self.reset_datadir(6)
        self.start_node(6, extra_args=pav_args(assumevalid_hash, final_chainwork))
        self.submit_headers(node, blocks)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        peer.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(peer, 2, MSG_BLOCK)
        with node.assert_debug_log(expected_msgs=[f"Queued prune-assumevalid block {blocks[1].hash_hex}"]):
            peer.serve_pending_heights([2])
            peer.sync_with_ping()
        assert_equal(node.getblockcount(), 0)

        genesis = from_hex(CBlock(), self.nodes[0].getblock(self.nodes[0].getblockhash(0), 0))
        fork = self.build_competing_chain(genesis, first_height=1, final_height=1)[0]
        assert_equal(node.submitblock(fork.serialize().hex()), None)
        assert_equal(node.getbestblockhash(), fork.hash_hex)
        # Free a request slot without making the queued child's parent the active tip
        peer.serve_pending_heights([1], witness=True)
        peer.sync_with_ping()
        assert_equal(node.getbestblockhash(), fork.hash_hex)
        self.assert_requested(peer, 2, MSG_BLOCK | MSG_WITNESS_FLAG)
        peer.serve_pending_heights([2])
        self.wait_until(lambda: node.getbestblockhash() == blocks[1].hash_hex)
        assert_equal(self.requested_types(peer, 2), [MSG_BLOCK, MSG_BLOCK | MSG_WITNESS_FLAG])
        assert_equal(node.getblock(blocks[1].hash_hex)["height"], 2)
        self.stop_node(6)

    def test_compact_block_fallback(self, blocks, assumevalid_hash):
        self.log.info("Request a compact block fallback with witness data after the block becomes eligible")
        first_chainwork = self.chainwork(blocks[0])
        self.start_node(7, extra_args=pav_args(assumevalid_hash, first_chainwork))
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
        self.wait_until(lambda: len(self.requested_types(peer, 1)) == 2)
        assert_equal(self.requested_types(peer, 1), [MSG_BLOCK | MSG_WITNESS_FLAG] * 2)
        peer.serve_pending_heights([1])
        self.wait_until(lambda: node.getblockcount() == 1)
        peer.sync_with_ping()
        self.assert_omitted(node, blocks[0].hash_hex)
        self.stop_node(7)
        self.reset_datadir(7)

    def test_presegwit_restart(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Restart omitted pre-SegWit history with the optimization disabled or its anchor changed")
        args = pav_args(assumevalid_hash, final_chainwork, f"-testactivationheight=segwit@{self.assumevalid_height + 1}")
        self.start_node(8, extra_args=args)
        node = self.nodes[8]
        self.submit_headers(node, blocks)
        initial_bytes = self.stored_block_bytes(node)
        self.sync_from_store(node, blocks, 2)
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        self.stop_node(8)
        full_args = [arg for arg in args if arg != "-pruneassumevalid"]
        for restart_args in [full_args, [arg for arg in args if not arg.startswith("-assumevalid=")] + ["-assumevalid=0"]]:
            self.start_node(8, extra_args=restart_args)
            assert_equal(node.getblockcount(), 2)
            assert_equal(node.gettxoutsetinfo()["bestblock"], blocks[1].hash_hex)
            self.assert_omitted(node, blocks[1].hash_hex)
            self.stop_node(8)

        self.log.info("Recover omitted pre-SegWit blocks using the ordinary full reindex path")
        self.start_node(8, extra_args=full_args + ["-reindex"])
        assert_equal(node.getblockcount(), 0)
        self.submit_headers(node, blocks)
        self.sync_from_store(node, blocks, 2, force_no_witness=True)
        assert_equal(node.getblock(blocks[1].hash_hex)["height"], 2)
        self.restart_node(8, extra_args=full_args)
        assert_equal(node.getblockcount(), 2)
        self.stop_node(8)

    def test_idle_restart_with_updated_assumevalid(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Keep existing history and omit new eligible blocks after a stale node's assumevalid is updated")
        old_height = MAX_BLOCKS_IN_TRANSIT_PER_PEER
        old_chainwork = self.chainwork(blocks[old_height - 1])
        old_args = ["-prune=1", f"-minimumchainwork={old_chainwork}"]
        self.start_node(9, extra_args=old_args)
        node = self.nodes[9]
        self.submit_headers(node, blocks[:old_height])
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=old_height))
        peer.send_headers_for_blocks(blocks[:old_height])
        self.wait_until(lambda: node.getblockcount() == old_height)
        assert not node.getblockchaininfo()["initialblockdownload"]
        self.stop_node(9)

        later_time = blocks[-1].nTime + 366 * 24 * 60 * 60
        new_args = pav_args(assumevalid_hash, final_chainwork, f"-mocktime={later_time}")
        self.start_node(9, extra_args=new_args)
        assert node.getblockchaininfo()["initialblockdownload"]
        self.submit_headers(node, blocks)
        old_bytes = self.stored_block_bytes(node)
        peer = self.sync_from_store(node, blocks, old_height + 4)
        for height in range(old_height + 1, old_height + 5):
            assert_equal(self.requested_types(peer, height), [MSG_BLOCK])
            self.assert_omitted(node, blocks[height - 1].hash_hex)
        assert_equal(self.stored_block_bytes(node), old_bytes)
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
        self.log.info("Discard canceled stripped replies while the parent is missing and after IBD ends")
        first_work = self.chainwork(blocks[0])
        self.start_node(10, extra_args=pav_args(assumevalid_hash, first_work, "-maxtipage=1"))
        node = self.nodes[10]
        node.setmocktime(blocks[-1].nTime)
        self.submit_headers(node, blocks)
        old_peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        old_peer.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(old_peer, 2, MSG_BLOCK)
        replacement = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        # Receiving the child from another peer cancels its original request before the parent arrives.
        replacement.send_and_ping(msg_block(blocks[1]))
        with node.assert_debug_log(expected_msgs=["Ignoring delayed stripped block"]):
            old_peer.send_and_ping(msg_no_witness_block(blocks[1]))
        assert old_peer.is_connected
        assert_equal(node.getblockcount(), 0)
        node.setmocktime(blocks[0].nTime)
        replacement.send_and_ping(msg_block(blocks[0]))
        self.wait_until(lambda: node.getblockcount() == 2)
        assert not node.getblockchaininfo()["initialblockdownload"]
        self.assert_omitted(node, blocks[0].hash_hex)

        self.log.info("Keep a full request when the same peer still owes an earlier stripped reply")
        old_peer_id = min(peer["id"] for peer in node.getpeerinfo())
        replacement_id = max(peer["id"] for peer in node.getpeerinfo())
        # Refetch is allowed now that the omitted parent has connected.
        assert_equal(node.getblockfrompeer(blocks[0].hash_hex, replacement_id), {})
        assert_equal(node.getblockfrompeer(blocks[0].hash_hex, old_peer_id), {})
        self.wait_until(lambda: MSG_BLOCK | MSG_WITNESS_FLAG in self.requested_types(old_peer, 1))
        with node.assert_debug_log(expected_msgs=["Ignoring delayed stripped block"]):
            old_peer.send_and_ping(msg_no_witness_block(blocks[0]))
        assert old_peer.is_connected
        assert 1 in next(peer["inflight"] for peer in node.getpeerinfo() if peer["id"] == old_peer_id)
        old_peer.serve_pending_heights([1])
        old_peer.sync_with_ping()
        assert_equal(node.getblock(blocks[0].hash_hex)["height"], 1)
        assert_equal(node.getblock(blocks[1].hash_hex)["height"], 2)
        # The allowance for the canceled child reply has already been consumed.
        with node.assert_debug_log(expected_msgs=["Received mutated block"]):
            old_peer.send_without_ping(msg_no_witness_block(blocks[1]))
            old_peer.wait_for_disconnect()
        self.stop_node(10)
        self.reset_datadir(10)

        self.log.info("Ignore a stripped reply after another peer supplies the requested block with witnesses")
        args = pav_args(assumevalid_hash, final_chainwork)
        self.start_node(10, extra_args=args)
        self.submit_headers(node, blocks)
        old_peer = self.sync_from_store(node, blocks, self.assumevalid_height - 1, timeout=120)
        self.assert_requested(old_peer, self.assumevalid_height, MSG_BLOCK)

        self.log.info("Validate witnesses when a stripped request receives a full block")
        bad_witness_block = copy.deepcopy(blocks[self.assumevalid_height - 1])
        witness_stack = bad_witness_block.vtx[0].wit.vtxinwit[0].scriptWitness.stack
        witness_stack[0] = witness_stack[0][:-1] + bytes([witness_stack[0][-1] ^ 1])
        with node.assert_debug_log(expected_msgs=["Received mutated block"]):
            old_peer.send_without_ping(msg_block(bad_witness_block))
            old_peer.wait_for_disconnect()
        assert_equal(node.getblockcount(), self.assumevalid_height - 1)

        old_peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=self.assumevalid_height - 1))
        old_peer.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(old_peer, self.assumevalid_height, MSG_BLOCK)
        full_peer = node.add_p2p_connection(P2PInterface())
        full_peer.send_and_ping(msg_block(blocks[self.assumevalid_height - 1]))
        self.wait_until(lambda: node.getblockcount() == self.assumevalid_height)
        with p2p_lock:
            old_request = next(inv for inv in old_peer.pending_getdata if inv.hash == blocks[self.assumevalid_height - 1].hash_int and inv.type == MSG_BLOCK)
        old_peer._send_block(old_request)
        old_peer.sync_with_ping()
        assert old_peer.is_connected
        assert_equal(node.getblockcount(), self.assumevalid_height)
        self.assert_omitted(node, blocks[self.assumevalid_height - 1].hash_hex)

        self.log.info("Reject a merkle-mutated stripped reply even after its request was canceled")
        bad_merkle_block = copy.deepcopy(blocks[self.assumevalid_height - 1])
        bad_merkle_block.vtx[0].nLockTime ^= 1
        with node.assert_debug_log(expected_msgs=["Received mutated block"]):
            old_peer.send_without_ping(msg_no_witness_block(bad_merkle_block))
            old_peer.wait_for_disconnect()
        assert_equal(node.getblockcount(), self.assumevalid_height)
        self.stop_node(10)

    def test_queued_block_restart(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Refetch an unconnected stripped block after restart without prune-assumevalid")
        args = pav_args(assumevalid_hash, final_chainwork)
        self.start_node(11, extra_args=args)
        node = self.nodes[11]
        self.submit_headers(node, blocks)
        peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        peer.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(peer, 2, MSG_BLOCK)
        with node.assert_debug_log(expected_msgs=["Queued prune-assumevalid block"]):
            peer.serve_pending_heights([2])
            peer.sync_with_ping()
        assert_equal(node.getblockcount(), 0)
        self.stop_node(11)

        full_args = [arg for arg in args if arg != "-pruneassumevalid"]
        self.start_node(11, extra_args=full_args)
        peer = self.sync_from_store(node, blocks, 2)
        self.assert_requested(peer, 2, MSG_BLOCK | MSG_WITNESS_FLAG)
        assert_equal(node.getblock(blocks[1].hash_hex)["height"], 2)
        self.stop_node(11)
        self.start_node(11, extra_args=full_args)
        assert_equal(node.getblockcount(), 2)
        self.stop_node(11)

        self.log.info("Do not mistake queued replies for connected history when the block index is ahead of coins")
        self.reset_datadir(11)
        self.start_node(11, extra_args=args)
        self.submit_headers(node, blocks)
        self.sync_from_store(node, blocks, 2)
        self.stop_node(11)
        coins_dir = node.chain_path / "chainstate"
        checkpoint_dir = node.datadir_path / "saved_chainstate"
        shutil.copytree(coins_dir, checkpoint_dir)
        self.start_node(11, extra_args=args)
        self.submit_headers(node, blocks)
        self.sync_from_store(node, blocks, MAX_BLOCKS_IN_TRANSIT_PER_PEER)
        self.stop_node(11)
        # Model durable block-index updates ahead of the last completed coins checkpoint.
        shutil.rmtree(coins_dir)
        shutil.copytree(checkpoint_dir, coins_dir)
        self.start_node(11, extra_args=args)
        assert_equal(node.getblockcount(), 2)
        self.submit_headers(node, blocks)
        peer = self.sync_from_store(node, blocks, 2)
        self.assert_requested(peer, 4, MSG_BLOCK)
        peer.serve_pending_heights([4])
        self.assert_requested(peer, MAX_BLOCKS_IN_TRANSIT_PER_PEER + 3, MSG_BLOCK)
        assert_equal(node.getpeerinfo()[0]["synced_blocks"], 2)
        replacement = self.sync_from_store(node, blocks, 2)
        peer.peer_disconnect()
        peer.wait_for_disconnect()
        # Free one of the surviving peer's in-flight slots for the missing parent.
        replacement.serve_pending_heights([MAX_BLOCKS_IN_TRANSIT_PER_PEER + 4])
        self.assert_requested(replacement, 3, MSG_BLOCK)
        replacement.serve_until_height(4)
        self.wait_until(lambda: node.getblockcount() == 4)
        self.stop_node(11)

    def test_announcements_after_ibd_exit(self, blocks, assumevalid_hash):
        self.log.info("Do not announce an omitted tip after IBD ends")
        self.reset_datadir(11)
        exit_chainwork = self.chainwork(blocks[2])
        args = pav_args(assumevalid_hash, exit_chainwork)
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
        source = self.sync_from_store(node, blocks, 3)
        assert not node.getblockchaininfo()["initialblockdownload"]
        self.assert_omitted(node, blocks[2].hash_hex)
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
        self.wait_until(lambda: node.getblockcount() == 4)
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
        self.wait_until(lambda: not try_rpc(-1, "Block not available (pruned data)", node.getblock, blocks[2].hash_hex))
        with node.assert_debug_log(expected_msgs=["Block verification stopping at height 3 (no undo data)"]):
            self.restart_node(11, extra_args=args + ["-checkblocks=0", "-checklevel=4"])
        assert_equal(node.getblockcount(), 4)
        assert_equal(node.getblock(blocks[3].hash_hex)["height"], 4)
        self.stop_node(11)

    def test_queued_block_rejection(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Keep queued-body parse failures and failed worker checks on the normal recovery paths")
        for malformed in [True, False]:
            self.start_node(3, extra_args=pav_args(assumevalid_hash, final_chainwork))
            node = self.nodes[3]
            self.submit_headers(node, blocks)
            initial_bytes = self.stored_block_bytes(node)
            peer = self.sync_from_store(node, blocks, 0)
            self.assert_requested(peer, 2, MSG_BLOCK)
            reply = msg_no_witness_block(copy.deepcopy(blocks[1]))
            if malformed:
                reply = msg_generic(b"block", reply.serialize()[:-1])
                error = "Cannot deserialize queued block"
            else:
                reply.block.vtx[0].nLockTime ^= 1
                error = "bad-txnmrklroot"
            with node.assert_debug_log(expected_msgs=[f"Queued prune-assumevalid block {blocks[1].hash_hex}"]):
                peer.send_and_ping(reply)
            assert_equal(node.getblockcount(), 0)
            if malformed:
                peer.serve_pending_heights([3])
                peer.sync_with_ping()
            with node.assert_debug_log(expected_msgs=[error], timeout=10):
                assert_equal(node.submitblock(blocks[0].serialize().hex()), None)
            peer.wait_for_disconnect()
            assert_equal(node.getblockcount(), 1)
            replacement = self.sync_from_store(node, blocks, 3)
            if malformed:
                self.assert_requested(replacement, 3, MSG_BLOCK)
            assert_equal(self.stored_block_bytes(node), initial_bytes)
            self.assert_omitted(node, blocks[1].hash_hex)
            self.stop_node(3)
            self.reset_datadir(3)

    def test_window_concurrency(self, blocks, assumevalid_hash, final_chainwork, expected_muhash, threads):
        self.log.info(f"Connect queued blocks behind a held parent with {threads} block preparation threads")
        self.start_node(3, extra_args=pav_args(assumevalid_hash, final_chainwork, f"-blockfetchthreads={threads}"))
        node = self.nodes[3]
        node.setmocktime(blocks[-1].nTime)
        self.submit_headers(node, blocks)
        initial_bytes = self.stored_block_bytes(node)
        peers = []
        for _ in range(6):
            peer = node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0, withheld_heights=[1]))
            peer.send_headers_for_blocks(blocks[-1:])
            peers.append(peer)
        self.wait_until(lambda: [len(peer["inflight"]) for peer in node.getpeerinfo()] == [MAX_BLOCKS_IN_TRANSIT_PER_PEER] * len(peers))
        # In-flight bookkeeping can become visible before getdata reaches the peers.
        self.wait_until(lambda: len(self.requested_heights(peers)) == MAX_BLOCKS_IN_TRANSIT_PER_PEER * len(peers))

        # Leave the critical parent outstanding while filling the ordinary download window.
        # Prepared children free download slots even while their parent is missing.
        prepared_children = prepared_child_count(threads)
        with node.assert_debug_log(expected_msgs=[f"Queued prune-assumevalid block {blocks[self.block_download_window - 1].hash_hex}"], timeout=60):
            for peer in peers:
                peer.serve_until_height(self.block_download_window)
        self.wait_until(lambda: sum(len(peer["inflight"]) for peer in node.getpeerinfo()) == 1 + prepared_children)
        assert_equal(node.getblockcount(), 0)
        self.wait_until(lambda: max(self.requested_heights(peers)) == self.block_download_window + prepared_children)
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        # An RPC-supplied parent must release queued children without another peer message.
        for peer in peers:
            peer.sync_with_ping()
        assert_equal([peer["last_block"] for peer in node.getpeerinfo()], [0] * len(peers))
        assert_equal(node.submitblock(blocks[0].serialize().hex()), None)
        self.wait_until(lambda: node.getblockcount() == self.block_download_window)
        # Peers supplying queued children receive the same recent-block credit as direct senders.
        self.wait_until(lambda: all(peer["last_block"] > 0 for peer in node.getpeerinfo()))
        assert_equal(node.gettxoutsetinfo("muhash")["muhash"], expected_muhash)
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        for height in range(1, self.block_download_window + 1):
            assert_equal(self.requested_types(peers, height), [MSG_BLOCK])
        self.stop_node(3)
        self.reset_datadir(3)

    def test_window_limit(self, blocks, final_chainwork):
        self.log.info("Bound queued and requested stripped blocks by the download window and recover the first missing parent without writing blocks")
        node = self.nodes[3]
        genesis = from_hex(CBlock(), self.nodes[0].getblock(self.nodes[0].getblockhash(0), 0))
        budget_blocks = []
        previous = genesis
        threads = 2
        prepared_children = prepared_child_count(threads)
        window_end = self.block_download_window + prepared_children
        target_height = window_end + 1
        for height in range(1, max(target_height + BURIAL_BLOCKS, len(blocks)) + 1):
            # Exercise a near-limit queued body without padding the entire window.
            padding = create_coinbase(height, script_pubkey=CScript([OP_RETURN, bytes(900_000)])) if height == 2 else None
            block = create_block(previous.hash_int, padding, height=height, ntime=blocks[0].nTime + height)
            block.solve()
            budget_blocks.append(block)
            previous = block
        args = pav_args(budget_blocks[target_height - 1].hash_hex, final_chainwork, "-dbcache=4", "-dbbatchsize=1", f"-blockfetchthreads={threads}")
        self.start_node(3, extra_args=args)
        node.setmocktime(budget_blocks[-1].nTime)
        self.submit_headers(node, budget_blocks)
        initial_bytes = self.stored_block_bytes(node)
        peer = node.add_p2p_connection(AssumeValidBlockStore(budget_blocks, max_height_to_serve=0, withheld_heights=[1]))
        peer.serve_until_height(target_height)
        peer.send_headers_for_blocks(budget_blocks[-1:])
        self.wait_until(lambda: max(self.requested_heights(peer), default=0) == window_end, timeout=120)
        self.wait_until(lambda: node.getpeerinfo()[0]["inflight"] == [1], timeout=120)
        assert_equal(max(self.requested_heights(peer)), window_end)
        assert_equal(node.getblockcount(), 0)
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        with p2p_lock:
            assert_equal({request_type for request_types in peer.request_types_by_height.values() for request_type in request_types}, {MSG_BLOCK})

        self.log.info("Use the ordinary stalling timeout to recover the parent while the download window is full")
        staller_id = node.getpeerinfo()[0]["id"]
        with node.assert_debug_log(expected_msgs=[f"Stall started peer={staller_id}"], timeout=60):
            replacement = node.add_p2p_connection(AssumeValidBlockStore(budget_blocks, max_height_to_serve=target_height))
            replacement.send_headers_for_blocks(budget_blocks[-1:])
            replacement.sync_with_ping()
        # Each late block from the stalling peer restarts its stall timer, so keep advancing time until it is dropped.
        def staller_disconnected():
            node.bumpmocktime(3)
            return not peer.is_connected

        self.wait_until(staller_disconnected)
        self.assert_requested(replacement, 1, MSG_BLOCK)
        self.wait_until(lambda: node.getblockcount() == target_height, timeout=120)
        assert_equal(self.stored_block_bytes(node), initial_bytes)
        for height in range(2, target_height + 1):
            assert_equal(self.requested_types([peer, replacement], height), [MSG_BLOCK])

        self.log.info("Complete a multi-batch UTXO flush without retaining block or undo files")
        with node.assert_debug_log(expected_msgs=["Writing partial batch", "Writing final batch"]):
            assert_equal(node.gettxoutsetinfo()["bestblock"], budget_blocks[target_height - 1].hash_hex)
        self.restart_node(3, extra_args=["-prune=1", "-assumevalid=0", "-dbcache=8"])
        assert_equal(node.getblockcount(), target_height)
        assert_equal(node.gettxoutsetinfo()["bestblock"], budget_blocks[target_height - 1].hash_hex)
        self.assert_omitted(node, budget_blocks[target_height - 1].hash_hex)
        self.stop_node(3)

    def test_reorg_into_omitted_history(self, blocks, assumevalid_hash, final_chainwork):
        self.log.info("Use the ordinary fatal disconnect path when a reorg requires omitted history")
        self.reset_datadir(3)
        args = pav_args(assumevalid_hash, final_chainwork)
        self.start_node(3, extra_args=args)
        node = self.nodes[3]
        self.submit_headers(node, blocks)
        self.sync_from_store(node, blocks, 2)
        assert_equal(node.gettxoutsetinfo()["bestblock"], blocks[1].hash_hex)
        genesis = from_hex(CBlock(), self.nodes[0].getblock(self.nodes[0].getblockhash(0), 0))
        competing = self.build_competing_chain(genesis, first_height=1, final_height=3)
        for block in competing[:2]:
            assert_equal(node.submitblock(block.serialize().hex()), "inconclusive")
        with node.assert_debug_log(expected_msgs=["Failed to disconnect block"]):
            node.submitblock(competing[2].serialize().hex())
            node.wait_until_stopped(expect_error=True, expected_stderr="Error: A fatal internal error occurred, see debug.log for details: Failed to disconnect block.")
        self.start_node(3, extra_args=["-prune=1", "-assumevalid=0", "-reindex"])
        self.wait_until(lambda: node.getblockcount() == 3)
        assert_equal(node.gettxoutsetinfo()["bestblock"], competing[2].hash_hex)
        self.stop_node(3)

    def run_test(self):
        blocks, assumevalid_hash, spend_txid, spent_txid, spent_vout, window_muhash = self.build_source_chain()
        block_hashes = [block.hash_hex for block in blocks]
        final_chainwork = self.chainwork(blocks[-1])
        prune_assumevalid_args = pav_args(assumevalid_hash, final_chainwork)
        self.nodes[3].assert_start_raises_init_error(
            extra_args=["-pruneassumevalid", "-txindex"],
            expected_msg="Error: Prune mode is incompatible with -txindex.",
        )
        self.log.info("Valid configurations fall back when prune-assumevalid preconditions do not apply")
        index_warning = "-pruneassumevalid disabled: block filter and coin statistics indexes require stored undo data."
        fallback_notice = "-pruneassumevalid inactive:"
        fallback_cases = [
            (["-prune=0", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}"], fallback_notice, 0),
            (["-pruneassumevalid", "-assumevalid=0"], fallback_notice, 550),
            (["-prune=1", "-pruneassumevalid", f"-assumevalid={'01' * 32}"], None, 1),
            (["-prune=1234", "-pruneassumevalid", "-assumevalid=0"], fallback_notice, 1234),
            (["-nopruneassumevalid", f"-assumevalid={assumevalid_hash}"], None, 0),
            (prune_assumevalid_args + ["-blockfilterindex=basic"], index_warning, 1),
            (prune_assumevalid_args + ["-coinstatsindex"], index_warning, 1),
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
                assert reason in node.debug_log_path.read_text(encoding="utf-8")
            self.submit_headers(node, blocks)
            before = self.stored_block_bytes(node)
            peer = self.sync_from_store(node, blocks, 1)
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
        default_peer = self.sync_from_store(default_node, blocks, 1)
        self.assert_requested(default_peer, 1, MSG_BLOCK | MSG_WITNESS_FLAG)
        assert_greater_than(self.stored_block_bytes(default_node), default_bytes)
        assert_equal(default_node.getblock(block_hashes[0])["height"], 1)

        self.log.info("Require a full reindex after a crash interrupts a chainstate flush covering omitted blocks")
        crash_args = prune_assumevalid_args + ["-dbbatchsize=1", "-dbcrashratio=1"]
        self.start_node(4, extra_args=crash_args)
        flush_crash_node = self.nodes[4]
        self.submit_headers(flush_crash_node, blocks)
        self.sync_from_store(flush_crash_node, blocks, 2)
        try:
            flush_crash_node.gettxoutsetinfo()
        except Exception:
            pass
        flush_crash_node.wait_until_stopped()
        # Omitted blocks cannot be replayed, so the ordinary replay failure asks for a full reindex.
        for extra_args in (prune_assumevalid_args, prune_assumevalid_args + ["-assumevalid=0"]):
            flush_crash_node.assert_start_raises_init_error(
                extra_args=extra_args,
                expected_msg="Unable to replay blocks.*\nPlease restart with -reindex to recover.",
                match=ErrorMatch.PARTIAL_REGEX,
            )
        self.start_node(4, extra_args=prune_assumevalid_args + ["-reindex"])
        assert_equal(flush_crash_node.getblockcount(), 0)
        self.submit_headers(flush_crash_node, blocks)
        reindex_peer = self.sync_from_store(flush_crash_node, blocks, MAX_BLOCKS_IN_TRANSIT_PER_PEER)
        for height in range(1, MAX_BLOCKS_IN_TRANSIT_PER_PEER + 1):
            assert_equal(self.requested_types(reindex_peer, height), [MSG_BLOCK])
        self.stop_node(4)

        self.log.info("Store an omitted block fetched again with getblockfrompeer")
        presegwit_args = prune_assumevalid_args + [f"-testactivationheight=segwit@{self.assumevalid_height + 1}"]
        self.start_node(5, extra_args=presegwit_args)
        refetch_node = self.nodes[5]
        self.submit_headers(refetch_node, blocks)
        refetch_bytes = self.stored_block_bytes(refetch_node)
        refetch_peer = self.sync_from_store(refetch_node, blocks, 1, force_no_witness=True)
        assert_equal(self.stored_block_bytes(refetch_node), refetch_bytes)
        peer_id = refetch_node.getpeerinfo()[0]["id"]
        assert_equal(refetch_node.getblockfrompeer(block_hashes[0], peer_id), {})
        self.wait_until(lambda: not try_rpc(-1, "Block not available (pruned data)", refetch_node.getblock, block_hashes[0]))
        assert_equal(self.requested_types(refetch_peer, 1)[-1], MSG_BLOCK | MSG_WITNESS_FLAG)
        with refetch_node.assert_debug_log(expected_msgs=["Block verification stopping at height 1 (no undo data)"]):
            self.restart_node(5, extra_args=presegwit_args + ["-checkblocks=0", "-checklevel=4"])
        assert_equal(refetch_node.getblock(block_hashes[0])["height"], 1)
        self.stop_node(5)

        self.log.info("Sync assumevalid ancestors as omitted stripped blocks")
        self.start_node(1, extra_args=[arg for arg in prune_assumevalid_args if arg != "-prune=1"])
        prune_assumevalid_node = self.nodes[1]
        self.submit_headers(prune_assumevalid_node, blocks)
        initial_bytes = self.stored_block_bytes(prune_assumevalid_node)
        prune_assumevalid_node.setmocktime(1_700_000_000)
        out_of_order_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        out_of_order_peer.send_headers_for_blocks(blocks[-1:])
        self.assert_requested(out_of_order_peer, 2, MSG_BLOCK)
        out_of_order_peer.sync_with_ping()
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)

        self.log.info("Connect out-of-order stripped blocks from the queue")
        out_of_order_peer.serve_pending_heights([2])
        out_of_order_peer.sync_with_ping()
        assert_equal(prune_assumevalid_node.getblockcount(), 0)
        assert_equal(len(self.requested_types(out_of_order_peer, 2)), 1)
        out_of_order_peer.serve_pending_heights([1])
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == 2)
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)

        assert_raises_rpc_error(-1, "necessary block data is already pruned", prune_assumevalid_node.dumptxoutset,
                                "omitted-rollback.dat", "rollback", {"rollback": 1})
        checkpoint = prune_assumevalid_node.gettxoutsetinfo("muhash")
        assert_equal(checkpoint["bestblock"], block_hashes[1])

        crash_height = MAX_BLOCKS_IN_TRANSIT_PER_PEER
        for description in ["Resume at the persisted UTXO height after an unclean shutdown",
                            "Discard unflushed connections and redownload from the persisted height"]:
            self.log.info(description)
            prune_assumevalid_node.kill_process()
            self.start_node(1, extra_args=prune_assumevalid_args)
            assert_equal(prune_assumevalid_node.getblockcount(), 2)
            assert_equal(prune_assumevalid_node.gettxoutsetinfo("muhash")["muhash"], checkpoint["muhash"])
            self.submit_headers(prune_assumevalid_node, blocks)
            peer = self.sync_from_store(prune_assumevalid_node, blocks, crash_height, timeout=120)
            for height in range(3, crash_height + 1):
                assert_equal(set(self.requested_types(peer, height)), {MSG_BLOCK})
            assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)

        self.log.info("Keep the exact UTXO tip across a clean stop without block files")
        assert_equal(prune_assumevalid_node.gettxoutsetinfo()["bestblock"], block_hashes[crash_height - 1])
        self.restart_node(1, extra_args=prune_assumevalid_args)
        assert_equal(prune_assumevalid_node.getblockcount(), crash_height)
        assert_equal(prune_assumevalid_node.gettxoutsetinfo()["bestblock"], block_hashes[crash_height - 1])
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)
        prune_assumevalid_node.kill_process()

        self.start_node(1, extra_args=prune_assumevalid_args)
        self.submit_headers(prune_assumevalid_node, blocks)
        restart_height = prune_assumevalid_node.getblockcount()
        assert_greater_than_or_equal(crash_height, restart_height)
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)
        prune_assumevalid_peer = self.sync_from_store(prune_assumevalid_node, blocks, self.assumevalid_height, timeout=120)

        for height in range(restart_height + 1, self.assumevalid_height + 1):
            assert_equal(set(self.requested_types(prune_assumevalid_peer, height)), {MSG_BLOCK})
        assert_equal(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)
        assert prune_assumevalid_node.gettxout(spend_txid, 0) is not None
        assert prune_assumevalid_node.gettxout(spent_txid, spent_vout) is None
        self.assert_omitted(prune_assumevalid_node, assumevalid_hash)

        self.log.info("Advertise NODE_NETWORK_LIMITED without serving omitted recent blocks during IBD")
        serving_peer = prune_assumevalid_node.add_p2p_connection(P2PInterface())
        assert_equal(serving_peer.nServices & (NODE_NETWORK | NODE_NETWORK_LIMITED | NODE_WITNESS), NODE_NETWORK_LIMITED | NODE_WITNESS)
        assert prune_assumevalid_node.getblockchaininfo()["initialblockdownload"]
        serving_peer.send_and_ping(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, blocks[self.assumevalid_height - 1].hash_int)]))
        assert "block" not in serving_peer.last_message
        block_txn_request = msg_getblocktxn()
        block_txn_request.block_txn_request = BlockTransactionsRequest(blocks[self.assumevalid_height - 1].hash_int, [0])
        serving_peer.send_and_ping(block_txn_request)
        assert "blocktxn" not in serving_peer.last_message

        self.log.info("Resume normal witness download and block storage after the assumevalid height")
        prune_assumevalid_peer.serve_until_height(self.assumevalid_height + 1)
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == self.assumevalid_height + 1)
        self.assert_requested(prune_assumevalid_peer, self.assumevalid_height + 1, MSG_BLOCK | MSG_WITNESS_FLAG)
        assert_greater_than(self.stored_block_bytes(prune_assumevalid_node), initial_bytes)
        assert_equal(prune_assumevalid_node.getblock(block_hashes[self.assumevalid_height])["height"], self.assumevalid_height + 1)
        self.assert_omitted(prune_assumevalid_node, assumevalid_hash)
        serving_peer.send_and_ping(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, blocks[self.assumevalid_height].hash_int)]))
        assert_equal(serving_peer.last_message["block"].block.hash_int, blocks[self.assumevalid_height].hash_int)
        block_txn_request.block_txn_request = BlockTransactionsRequest(blocks[self.assumevalid_height].hash_int, [0])
        serving_peer.send_and_ping(block_txn_request)
        response = serving_peer.last_message["blocktxn"].block_transactions
        assert_equal(response.blockhash, blocks[self.assumevalid_height].hash_int)
        assert_equal([tx.serialize() for tx in response.transactions], [blocks[self.assumevalid_height].vtx[0].serialize()])

        self.log.info("Restart safely with historical assumevalid blocks missing from disk")
        with prune_assumevalid_node.assert_debug_log(expected_msgs=[f"Block verification stopping at height {self.assumevalid_height} (no data)"]):
            self.restart_node(1, extra_args=prune_assumevalid_args + ["-checkblocks=0", "-checklevel=4"])
        assert_equal(prune_assumevalid_node.getblockcount(), self.assumevalid_height + 1)
        assert prune_assumevalid_node.gettxout(spend_txid, 0) is not None
        self.assert_omitted(prune_assumevalid_node, assumevalid_hash)

        self.log.info("Restart committed omitted history with different optimization and assumevalid settings")
        for args in [
            [arg for arg in prune_assumevalid_args if arg != "-pruneassumevalid"],
            [arg for arg in prune_assumevalid_args if not arg.startswith("-assumevalid=")] + ["-assumevalid=0"],
            [arg for arg in prune_assumevalid_args if not arg.startswith("-assumevalid=")] + [f"-assumevalid={block_hashes[0]}"],
        ]:
            self.restart_node(1, extra_args=args)
            assert_equal(prune_assumevalid_node.getblockcount(), self.assumevalid_height + 1)
            assert prune_assumevalid_node.gettxout(spend_txid, 0) is not None
        self.restart_node(1, extra_args=prune_assumevalid_args)

        self.log.info("Continue after restart without rereading blocks pruned by -pruneassumevalid")
        restart_peer = self.sync_from_store(prune_assumevalid_node, blocks, self.assumevalid_height + 2)
        self.assert_requested(restart_peer, self.assumevalid_height + 2, MSG_BLOCK | MSG_WITNESS_FLAG)

        self.log.info("Request a competing post-assumevalid block with witness data")
        prev_block = blocks[self.assumevalid_height + 1]
        competing_blocks = self.build_competing_chain(
            prev_block,
            first_height=self.assumevalid_height + 3,
            final_height=len(blocks) + 1,
        )
        competing_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(competing_blocks, max_height_to_serve=0, first_height=self.assumevalid_height + 3))
        self.assert_requested(competing_peer, self.assumevalid_height + 3, MSG_BLOCK | MSG_WITNESS_FLAG)

        self.test_queued_blocks_after_ibd_exit(blocks, assumevalid_hash)
        self.test_queued_blocks_after_best_header_change(blocks, assumevalid_hash, final_chainwork)
        self.test_compact_block_fallback(blocks, assumevalid_hash)
        self.test_presegwit_restart(blocks, assumevalid_hash, final_chainwork)
        self.test_idle_restart_with_updated_assumevalid(blocks, assumevalid_hash, final_chainwork)
        self.test_late_stripped_replies(blocks, assumevalid_hash, final_chainwork)
        self.test_queued_block_restart(blocks, assumevalid_hash, final_chainwork)
        self.test_announcements_after_ibd_exit(blocks, assumevalid_hash)
        self.test_queued_block_rejection(blocks, assumevalid_hash, final_chainwork)
        for threads in [0, 2]:
            self.test_window_concurrency(blocks, assumevalid_hash, final_chainwork, window_muhash, threads)
        self.test_window_limit(blocks, final_chainwork)
        self.test_reorg_into_omitted_history(blocks, assumevalid_hash, final_chainwork)


if __name__ == "__main__":
    FeaturePruneAssumeValidTest(__file__).main()
