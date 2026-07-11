#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test prune-assumevalid IBD mode."""

from test_framework.blocktools import (
    COINBASE_MATURITY,
    create_block,
)
from test_framework.messages import (
    BlockTransactions,
    CBlock,
    CBlockHeader,
    HeaderAndShortIDs,
    MSG_BLOCK,
    MSG_TYPE_MASK,
    MSG_WITNESS_FLAG,
    from_hex,
    msg_block,
    msg_blocktxn,
    msg_cmpctblock,
    msg_headers,
    msg_no_witness_block,
    msg_sendcmpct,
)
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    try_rpc,
)
from test_framework.wallet import MiniWallet


MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16
PRUNE_ASSUMEVALID_REQUEST_PEERS = 9
PRUNE_ASSUMEVALID_INITIAL_REQUESTS = PRUNE_ASSUMEVALID_REQUEST_PEERS * MAX_BLOCKS_IN_TRANSIT_PER_PEER
ASSUMEVALID_HEIGHT = max(COINBASE_MATURITY + 2, PRUNE_ASSUMEVALID_INITIAL_REQUESTS + 2)
BURIAL_BLOCKS = 2100


class AssumeValidBlockStore(P2PInterface):
    def __init__(self, blocks, max_height_to_serve, first_height=1, withheld_heights=(), force_no_witness=False):
        super().__init__()
        self.blocks = blocks
        self.first_height = first_height
        self.blocks_by_hash = {block.hash_int: block for block in blocks}
        self.height_by_hash = {block.hash_int: height for height, block in enumerate(blocks, start=first_height)}
        self.max_height_to_serve = max_height_to_serve
        self.withheld_heights = set(withheld_heights)
        self.force_no_witness = force_no_witness
        self.pending_getdata = []
        self.request_types_by_height = {}

    def send_headers_for_blocks(self, blocks):
        for start in range(0, len(blocks), 2000):
            self.send_without_ping(msg_headers([CBlockHeader(block) for block in blocks[start:start + 2000]]))

    def on_getheaders(self, message):
        start = 0
        for locator_hash in message.locator.vHave:
            if locator_hash in self.height_by_hash:
                start = self.height_by_hash[locator_hash] - self.first_height + 1
                break
        headers = []
        for block in self.blocks[start:]:
            headers.append(CBlockHeader(block))
            if block.hash_int == message.hashstop or len(headers) == 2000:
                break
        self.send_without_ping(msg_headers(headers))

    def on_getdata(self, message):
        for inv in message.inv:
            height = self.height_by_hash.get(inv.hash)
            if height is None:
                continue
            self.request_types_by_height.setdefault(height, []).append(inv.type)
            if height <= self.max_height_to_serve and height not in self.withheld_heights:
                self._send_block(inv)
            else:
                self.pending_getdata.append(inv)

    def serve_until_height(self, height):
        self.max_height_to_serve = height
        still_pending = []
        for inv in self.pending_getdata:
            if self.height_by_hash[inv.hash] <= self.max_height_to_serve:
                self._send_block(inv)
            else:
                still_pending.append(inv)
        self.pending_getdata = still_pending

    def serve_pending_heights(self, heights):
        heights_to_send = set(heights)
        pending_by_height = {self.height_by_hash[inv.hash]: inv for inv in self.pending_getdata}
        for height in heights:
            self._send_block(pending_by_height[height])
        self.pending_getdata = [inv for inv in self.pending_getdata if self.height_by_hash[inv.hash] not in heights_to_send]

    def _send_block(self, inv):
        if inv.type & MSG_TYPE_MASK != MSG_BLOCK:
            return
        block = self.blocks_by_hash[inv.hash]
        if inv.type & MSG_WITNESS_FLAG and not self.force_no_witness:
            self.send_without_ping(msg_block(block))
        else:
            self.send_without_ping(msg_no_witness_block(block))


class FeaturePruneAssumeValidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 7
        self.rpc_timeout = 120

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        self.start_node(0)

    def block_file_sizes(self, node):
        return {
            path.name: path.stat().st_size
            for pattern in ("blk*.dat", "rev*.dat")
            for path in sorted(node.blocks_path.glob(pattern))
        }

    def requested_types(self, peers, height):
        if not isinstance(peers, list):
            peers = [peers]
        with p2p_lock:
            return [
                request_type
                for peer in peers
                for request_type in peer.request_types_by_height.get(height, [])
            ]

    def assert_requested(self, peers, height, expected_type):
        self.wait_until(lambda: expected_type in self.requested_types(peers, height), timeout=60)

    def requested_heights(self, peers):
        if not isinstance(peers, list):
            peers = [peers]
        with p2p_lock:
            return {
                height
                for peer in peers
                for height, request_types in peer.request_types_by_height.items()
                if request_types
            }

    def submit_headers(self, node, blocks):
        for block in blocks:
            node.submitheader(CBlockHeader(block).serialize().hex())

    def build_source_chain(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        self.log.info("Mine a buried assumevalid chain with a witness spend at the assumevalid height")
        self.generate(wallet, ASSUMEVALID_HEIGHT - 1, sync_fun=self.no_op)
        spend_tx = wallet.send_self_transfer(from_node=node, confirmed_only=True)
        assumevalid_hash = self.generate(wallet, 1, sync_fun=self.no_op)[0]
        spent_prevout = spend_tx["tx"].vin[0].prevout
        assert_equal(node.getblockcount(), ASSUMEVALID_HEIGHT)
        self.generate(wallet, BURIAL_BLOCKS, sync_fun=self.no_op)

        blocks = [
            from_hex(CBlock(), node.getblock(node.getblockhash(height), False))
            for height in range(1, node.getblockcount() + 1)
        ]
        assumevalid_block = blocks[ASSUMEVALID_HEIGHT - 1]
        assert any(not tx.wit.is_null() for tx in assumevalid_block.vtx[1:])
        return blocks, assumevalid_hash, spend_tx["txid"], format(spent_prevout.hash, "064x"), spent_prevout.n

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

    def run_test(self):
        blocks, assumevalid_hash, spend_txid, spent_txid, spent_vout = self.build_source_chain()
        block_hashes = [block.hash_hex for block in blocks]
        final_chainwork = self.nodes[0].getblockheader(block_hashes[-1])["chainwork"]
        prune_assumevalid_args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={final_chainwork}"]
        fallback_args = ["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}", "-minimumchainwork=0"]
        snapshot = self.nodes[0].dumptxoutset("utxos.dat", rollback=200)

        self.log.info("Reject -pruneassumevalid without pruning or without an active assumevalid hash")
        self.nodes[1].assert_start_raises_init_error(
            extra_args=["-pruneassumevalid", f"-assumevalid={assumevalid_hash}"],
            expected_msg="Error: -pruneassumevalid requires pruning. Please restart with -prune.",
        )
        self.nodes[1].assert_start_raises_init_error(
            extra_args=["-prune=1", "-pruneassumevalid", "-assumevalid=0"],
            expected_msg="Error: -pruneassumevalid requires a non-zero assumevalid block. Please set -assumevalid to a block hash.",
        )
        self.nodes[1].assert_start_raises_init_error(
            extra_args=prune_assumevalid_args + ["-blockfilterindex=basic"],
            expected_msg="Error: -pruneassumevalid is incompatible with -blockfilterindex because the index requires block and undo data.",
        )
        self.nodes[1].assert_start_raises_init_error(
            extra_args=prune_assumevalid_args + ["-coinstatsindex"],
            expected_msg="Error: -pruneassumevalid is incompatible with -coinstatsindex because the index requires block and undo data.",
        )

        self.log.info("Reject -pruneassumevalid startup with an existing assumeutxo snapshot")
        self.start_node(3, extra_args=["-prune=1"])
        snapshot_node = self.nodes[3]
        self.stop_node(3)
        chainstate_snapshot_path = snapshot_node.chain_path / "chainstate_snapshot"
        chainstate_snapshot_path.mkdir()
        with open(chainstate_snapshot_path / "base_blockhash", "wb") as f:
            f.write(bytes.fromhex(assumevalid_hash)[::-1])
        snapshot_node.assert_start_raises_init_error(
            extra_args=["-prune=1", "-pruneassumevalid", f"-assumevalid={assumevalid_hash}"],
            expected_msg="-pruneassumevalid is incompatible with assumeutxo snapshots. Please restart without -pruneassumevalid or remove the snapshot chainstate.\nPlease restart with -reindex or -reindex-chainstate to recover.",
        )

        self.log.info("Require full reindex after a crash interrupts an ephemeral chainstate flush")
        crash_args = prune_assumevalid_args + ["-dbbatchsize=1", "-dbcrashratio=1"]
        self.start_node(4, extra_args=crash_args)
        flush_crash_node = self.nodes[4]
        self.submit_headers(flush_crash_node, blocks)
        flush_crash_peer = flush_crash_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=2))
        flush_crash_peer.send_headers_for_blocks(blocks)
        self.wait_until(lambda: flush_crash_node.getblockcount() == 2, timeout=60)
        try:
            flush_crash_node.gettxoutsetinfo()
        except Exception:
            pass
        flush_crash_node.wait_until_stopped(timeout=60)
        flush_crash_node.assert_start_raises_init_error(
            extra_args=prune_assumevalid_args,
            expected_msg="The interrupted chainstate flush requires blocks that were not written by -pruneassumevalid. A full -reindex is required to redownload them.\nPlease restart with -reindex to recover.",
        )
        self.start_node(4, extra_args=prune_assumevalid_args + ["-reindex"])
        assert_equal(flush_crash_node.getblockcount(), 0)
        self.stop_node(4)

        self.log.info("Store a normally requested historical block instead of caching it ephemerally")
        presegwit_args = prune_assumevalid_args + [f"-testactivationheight=segwit@{ASSUMEVALID_HEIGHT + 1}"]
        self.start_node(5, extra_args=presegwit_args)
        refetch_node = self.nodes[5]
        self.submit_headers(refetch_node, blocks)
        refetch_sizes = self.block_file_sizes(refetch_node)
        refetch_peer = refetch_node.add_p2p_connection(AssumeValidBlockStore(
            blocks,
            max_height_to_serve=1,
            force_no_witness=True,
        ))
        refetch_peer.send_headers_for_blocks(blocks)
        self.wait_until(lambda: refetch_node.getblockcount() == 1, timeout=60)
        assert_equal(self.block_file_sizes(refetch_node), refetch_sizes)
        peer_id = refetch_node.getpeerinfo()[0]["id"]
        assert_equal(refetch_node.getblockfrompeer(block_hashes[0], peer_id), {})
        self.wait_until(lambda: not try_rpc(-1, "Block not available (pruned data)", refetch_node.getblock, block_hashes[0]), timeout=60)
        assert_equal(self.requested_types(refetch_peer, 1)[-1], MSG_BLOCK | MSG_WITNESS_FLAG)
        self.stop_node(5)

        self.log.info("Reuse the queued witness mode for a compact-block fallback")
        self.start_node(1, extra_args=fallback_args)
        prune_assumevalid_node = self.nodes[1]
        self.submit_headers(prune_assumevalid_node, blocks)
        prune_assumevalid_node.setmocktime(1_700_000_000)

        fallback_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
        fallback_peer.send_and_ping(msg_sendcmpct(announce=False, version=2))
        fallback_peer.send_headers_for_blocks(blocks)
        self.assert_requested(fallback_peer, 1, MSG_BLOCK)

        compact_block = HeaderAndShortIDs()
        compact_block.initialize_from_block(blocks[0], prefill_list=[], use_witness=True)
        fallback_peer.send_without_ping(msg_cmpctblock(compact_block.to_p2p()))
        fallback_peer.wait_until(lambda: "getblocktxn" in fallback_peer.last_message, timeout=60)

        genesis = from_hex(CBlock(), self.nodes[0].getblock(self.nodes[0].getblockhash(0), False))
        alternate_headers = self.build_competing_chain(genesis, first_height=1, final_height=len(blocks) + 1)
        alternate_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(alternate_headers, max_height_to_serve=0))
        alternate_peer.send_headers_for_blocks(alternate_headers)
        alternate_peer.sync_with_ping()

        wrong_blocktxn = msg_blocktxn()
        wrong_blocktxn.block_transactions = BlockTransactions(blocks[0].hash_int, [blocks[1].vtx[0]])
        fallback_peer.send_without_ping(wrong_blocktxn)
        self.wait_until(lambda: len(self.requested_types(fallback_peer, 1)) >= 2, timeout=60)
        assert_equal(self.requested_types(fallback_peer, 1)[-1], MSG_BLOCK)

        fallback_peer.send_without_ping(msg_no_witness_block(blocks[0]))
        fallback_peer.sync_with_ping()
        assert_equal(prune_assumevalid_node.getblockcount(), 0)
        prune_assumevalid_node.invalidateblock(alternate_headers[0].hash_hex)
        alternate_peer.peer_disconnect()
        alternate_peer.wait_for_disconnect()
        fallback_peer.peer_disconnect()
        fallback_peer.wait_for_disconnect()
        self.stop_node(1)
        self.log.info("Recover out-of-order stripped blocks after the peer holding their parent disconnects")
        self.start_node(6, extra_args=prune_assumevalid_args)
        peer_loss_node = self.nodes[6]
        self.submit_headers(peer_loss_node, blocks)
        peer_loss_sizes = self.block_file_sizes(peer_loss_node)
        peer_loss_node.setmocktime(1_700_000_000)
        gap_peer = peer_loss_node.add_p2p_connection(AssumeValidBlockStore(
            blocks,
            max_height_to_serve=ASSUMEVALID_HEIGHT,
            withheld_heights={1},
        ))
        gap_peer.send_headers_for_blocks(blocks)
        self.wait_until(lambda: max(self.requested_heights(gap_peer), default=0) >= ASSUMEVALID_HEIGHT, timeout=60)
        gap_peer.sync_with_ping()
        assert_equal(peer_loss_node.getblockcount(), 0)
        assert_equal(self.block_file_sizes(peer_loss_node), peer_loss_sizes)
        gap_peer.peer_disconnect()
        gap_peer.wait_for_disconnect()

        recovery_peer = peer_loss_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=1))
        recovery_peer.send_headers_for_blocks(blocks)
        self.assert_requested(recovery_peer, 1, MSG_BLOCK)
        self.wait_until(lambda: peer_loss_node.getblockcount() == ASSUMEVALID_HEIGHT, timeout=60)
        assert_equal(self.block_file_sizes(peer_loss_node), peer_loss_sizes)
        self.stop_node(6)

        self.log.info("Sync assumevalid ancestors as stripped ephemeral blocks")
        self.start_node(1, extra_args=prune_assumevalid_args)
        prune_assumevalid_node = self.nodes[1]
        assert_raises_rpc_error(
            -32603,
            "Unable to load UTXO snapshot: -pruneassumevalid is incompatible with assumeutxo snapshots. Please restart without -pruneassumevalid to load a snapshot.",
            prune_assumevalid_node.loadtxoutset,
            snapshot["path"],
        )
        self.submit_headers(prune_assumevalid_node, blocks)
        initial_sizes = self.block_file_sizes(prune_assumevalid_node)
        prune_assumevalid_node.setmocktime(1_700_000_000)

        prune_assumevalid_peers = [
            prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=0))
            for _ in range(PRUNE_ASSUMEVALID_REQUEST_PEERS)
        ]
        for peer in prune_assumevalid_peers:
            peer.send_headers_for_blocks(blocks[-1:])
        self.wait_until(lambda: len(self.requested_heights(prune_assumevalid_peers)) == PRUNE_ASSUMEVALID_INITIAL_REQUESTS, timeout=60)
        for peer in prune_assumevalid_peers:
            peer.sync_with_ping()
        pre_crash_prune_assumevalid_peers = prune_assumevalid_peers
        assert_equal(
            self.requested_heights(pre_crash_prune_assumevalid_peers),
            set(range(1, PRUNE_ASSUMEVALID_INITIAL_REQUESTS + 1)),
        )
        assert_equal(self.block_file_sizes(prune_assumevalid_node), initial_sizes)

        self.log.info("Connect out-of-order stripped blocks from the transient cache")
        out_of_order_peer = prune_assumevalid_peers[0]
        out_of_order_peer.serve_pending_heights([2])
        out_of_order_peer.sync_with_ping()
        assert_equal(prune_assumevalid_node.getblockcount(), 0)
        assert_equal(len(self.requested_types(pre_crash_prune_assumevalid_peers, 2)), 1)
        out_of_order_peer.serve_pending_heights([1])
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == 2, timeout=60)
        assert_equal(self.block_file_sizes(prune_assumevalid_node), initial_sizes)

        prune_assumevalid_node.kill_process()

        self.log.info("Restart safely after an unclean shutdown in the prune-assumevalid region")
        self.start_node(1, extra_args=prune_assumevalid_args)
        prune_assumevalid_node = self.nodes[1]
        self.submit_headers(prune_assumevalid_node, blocks)
        first_restart_height = prune_assumevalid_node.getblockcount()
        assert first_restart_height <= 2
        assert_equal(self.block_file_sizes(prune_assumevalid_node), initial_sizes)
        crash_height = MAX_BLOCKS_IN_TRANSIT_PER_PEER
        crash_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=crash_height))
        crash_peer.send_headers_for_blocks(blocks)
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == crash_height, timeout=120)
        for height in range(first_restart_height + 1, crash_height + 1):
            assert self.requested_types(crash_peer, height)
            assert all(request_type == MSG_BLOCK for request_type in self.requested_types(crash_peer, height))
        assert_equal(self.block_file_sizes(prune_assumevalid_node), initial_sizes)
        prune_assumevalid_node.kill_process()

        self.start_node(1, extra_args=prune_assumevalid_args)
        prune_assumevalid_node = self.nodes[1]
        self.submit_headers(prune_assumevalid_node, blocks)
        restart_height = prune_assumevalid_node.getblockcount()
        assert restart_height <= crash_height
        assert_equal(self.block_file_sizes(prune_assumevalid_node), initial_sizes)
        prune_assumevalid_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=ASSUMEVALID_HEIGHT))
        prune_assumevalid_peers = [prune_assumevalid_peer]
        prune_assumevalid_peer.send_headers_for_blocks(blocks)
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == ASSUMEVALID_HEIGHT, timeout=120)

        for height in range(1, ASSUMEVALID_HEIGHT + 1):
            if height > restart_height:
                assert self.requested_types(prune_assumevalid_peer, height)
                assert all(request_type == MSG_BLOCK for request_type in self.requested_types(prune_assumevalid_peer, height))
        assert_equal(self.block_file_sizes(prune_assumevalid_node), initial_sizes)
        assert prune_assumevalid_node.gettxout(spend_txid, 0) is not None
        assert prune_assumevalid_node.gettxout(spent_txid, spent_vout) is None
        assert_raises_rpc_error(-1, "Block not available (pruned data)", prune_assumevalid_node.getblock, assumevalid_hash)

        self.log.info("Reject a reorg below a connected stripped block without aborting")
        reorg_prev_tip = prune_assumevalid_node.getbestblockhash()
        reorg_blocks = self.build_competing_chain(
            blocks[ASSUMEVALID_HEIGHT - 2],
            first_height=ASSUMEVALID_HEIGHT,
            final_height=ASSUMEVALID_HEIGHT + 1,
        )
        with prune_assumevalid_node.assert_debug_log(expected_msgs=["Cannot disconnect -pruneassumevalid block"], timeout=60):
            for block in reorg_blocks:
                prune_assumevalid_node.submitblock(block.serialize().hex())
        assert_equal(prune_assumevalid_node.getbestblockhash(), reorg_prev_tip)
        assert_equal(prune_assumevalid_node.getblockcount(), ASSUMEVALID_HEIGHT)
        prune_assumevalid_node.getblockchaininfo()
        prune_assumevalid_node.invalidateblock(reorg_blocks[-1].hash_hex)

        self.log.info("Resume normal witness download and block storage after the assumevalid height")
        for peer in prune_assumevalid_peers:
            peer.serve_until_height(ASSUMEVALID_HEIGHT + 1)
        self.wait_until(lambda: prune_assumevalid_node.getblockcount() == ASSUMEVALID_HEIGHT + 1, timeout=60)
        self.assert_requested(prune_assumevalid_peers, ASSUMEVALID_HEIGHT + 1, MSG_BLOCK | MSG_WITNESS_FLAG)
        assert self.block_file_sizes(prune_assumevalid_node) != initial_sizes
        assert_equal(prune_assumevalid_node.getblock(block_hashes[ASSUMEVALID_HEIGHT])["height"], ASSUMEVALID_HEIGHT + 1)
        assert_raises_rpc_error(-1, "Block not available (pruned data)", prune_assumevalid_node.getblock, assumevalid_hash)

        self.log.info("Restart safely with historical assumevalid blocks missing from disk")
        self.restart_node(1, extra_args=prune_assumevalid_args)
        prune_assumevalid_node = self.nodes[1]
        assert_equal(prune_assumevalid_node.getblockcount(), ASSUMEVALID_HEIGHT + 1)
        assert prune_assumevalid_node.gettxout(spend_txid, 0) is not None
        assert_raises_rpc_error(-1, "Block not available (pruned data)", prune_assumevalid_node.getblock, assumevalid_hash)

        self.log.info("Reject restart without -pruneassumevalid after stripped blocks were connected")
        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(
            extra_args=["-prune=1", f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={final_chainwork}"],
            expected_msg="This chainstate contains blocks pruned by -pruneassumevalid. Please restart with -pruneassumevalid or rebuild the database using -reindex.\nPlease restart with -reindex to recover.",
        )
        self.start_node(1, extra_args=prune_assumevalid_args)
        prune_assumevalid_node = self.nodes[1]

        self.log.info("Continue after restart without rereading blocks pruned by -pruneassumevalid")
        restart_peer = prune_assumevalid_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=ASSUMEVALID_HEIGHT + 2))
        restart_peer.send_headers_for_blocks(blocks)
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

        self.log.info("Default pruned assumevalid behavior still requests and stores witness blocks")
        self.start_node(2, extra_args=["-prune=1", f"-assumevalid={assumevalid_hash}", f"-minimumchainwork={final_chainwork}"])
        default_node = self.nodes[2]
        self.submit_headers(default_node, blocks)
        default_sizes = self.block_file_sizes(default_node)
        default_peer = default_node.add_p2p_connection(AssumeValidBlockStore(blocks, max_height_to_serve=1))
        default_peer.send_headers_for_blocks(blocks)
        self.wait_until(lambda: default_node.getblockcount() == 1, timeout=60)
        self.assert_requested(default_peer, 1, MSG_BLOCK | MSG_WITNESS_FLAG)
        assert self.block_file_sizes(default_node) != default_sizes
        assert_equal(default_node.getblock(block_hashes[0])["height"], 1)


if __name__ == "__main__":
    FeaturePruneAssumeValidTest(__file__).main()
