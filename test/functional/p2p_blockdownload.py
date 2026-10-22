#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
"""Test configurable download windows and retaining full blocks until connection."""

from test_framework.blocktools import add_witness_commitment, create_block, create_coinbase
from test_framework.messages import CBlockHeader, CTxOut, msg_block, msg_headers, msg_no_witness_block
from test_framework.p2p import P2PDataStore, p2p_lock
from test_framework.script import CScript, OP_RETURN
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, try_rpc


class DownloadPeer(P2PDataStore):
    def on_getheaders(self, message):
        pass

    def on_getdata(self, message):
        self.getdata_requests.extend(inv.hash for inv in message.inv)


class BlockDownloadTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        for window, memory, prune in [(1, 64, 0), (4, 0, 0), (4, 1, 0), (32, 64, 550)]:
            self.log.info(f"Download window {window}, pending full-block memory {memory} MiB, prune target {prune} MiB")
            args = [f"-blockdownloadwindow={window}", f"-blockdownloadmemory={memory}", f"-prune={prune}", "-blockfetchthreads=8", "-assumevalid=0"]
            self.restart_node(0, extra_args=args)
            height = node.getblockcount()
            tip = node.getbestblockhash()
            block_time = node.getblock(tip)['time']
            blocks = []
            for offset in range(1, window + 2):
                coinbase = create_coinbase(height + offset)
                # Two pending blocks exceed a 1 MiB budget together, but each fits by itself
                if offset in (2, 3):
                    coinbase.vout.append(CTxOut(0, CScript([OP_RETURN, b'x' * 700_000])))
                block = create_block(int(tip, 16), coinbase, ntime=block_time + offset)
                block.solve()
                blocks.append(block)
                tip = block.hash_hex

            peer = node.add_p2p_connection(DownloadPeer())
            peer.send_and_ping(msg_headers([CBlockHeader(block) for block in blocks]))
            peer.wait_until(lambda: len(peer.getdata_requests) == min(window, 16))
            with p2p_lock:
                assert_equal(peer.getdata_requests, [block.hash_int for block in blocks[:min(window, 16)]])

            if window > 1:
                for offset in (2, 3):
                    queued = memory > 0 and (offset == 2 or memory > 1)
                    block = blocks[offset - 1]
                    expected = f"Queued full block {block.hash_hex}" if queued else f"received block {block.hash_hex}"
                    with node.assert_debug_log([expected]):
                        peer.send_and_ping(msg_block(block))
                    if queued:
                        assert_raises_rpc_error(-1, "Block not available (not fully downloaded)", node.getblock, block.hash_hex)
                    else:
                        assert_equal(node.getblock(block.hash_hex, 0), block.serialize().hex())
                assert_equal(node.getblockcount(), height)

                if window > 16:
                    # Farther blocks use disk even when there is room in the byte budget
                    peer.wait_until(lambda: blocks[16].hash_int in peer.getdata_requests)
                    peer.send_and_ping(msg_block(blocks[16]))
                    assert_equal(node.getblock(blocks[16].hash_hex, 0), blocks[16].serialize().hex())
                else:
                    peer.sync_with_ping()
                    with p2p_lock:
                        assert blocks[window].hash_int not in peer.getdata_requests

                # A restart redownloads queued blocks and retains the ones already written
                self.restart_node(0, extra_args=args)
                assert_equal(node.getblockcount(), height)
                peer = node.add_p2p_connection(DownloadPeer())
                peer.send_and_ping(msg_headers([CBlockHeader(block) for block in blocks]))
                peer.wait_until(lambda: blocks[0].hash_int in peer.getdata_requests)
                with p2p_lock:
                    assert_equal(blocks[1].hash_int in peer.getdata_requests, memory > 0)
                    assert_equal(blocks[2].hash_int in peer.getdata_requests, memory > 1)
                if memory > 0:
                    peer.send_and_ping(msg_block(blocks[1]))
                    assert_raises_rpc_error(-1, "Block not available (not fully downloaded)", node.getblock, blocks[1].hash_hex)

            # Connect from the gap onwards and verify that ordinary mode persists every block
            for offset, block in enumerate(blocks, start=1):
                if node.getblockcount() < height + offset:
                    peer.wait_until(lambda: block.hash_int in peer.getdata_requests)
                    peer.send_and_ping(msg_block(block))
                    self.wait_until(lambda: node.getblockcount() >= height + offset)
                assert_equal(node.getblock(block.hash_hex, 0), block.serialize().hex())
            self.restart_node(0, extra_args=args)
            assert_equal(node.getbestblockhash(), blocks[-1].hash_hex)
            assert_equal(node.verifychain(4, len(blocks)), True)

        self.log.info("A queued full-block reply missing witnesses must not invalidate its header")
        height = node.getblockcount()
        tip = node.getbestblockhash()
        block_time = node.getblock(tip)['time']
        parent = create_block(int(tip, 16), height=height + 1, ntime=block_time + 1)
        parent.solve()
        child = create_block(parent.hash_int, height=height + 2, ntime=block_time + 2)
        add_witness_commitment(child)
        child.solve()
        peer = node.add_p2p_connection(DownloadPeer())
        headers = msg_headers([CBlockHeader(parent), CBlockHeader(child)])
        peer.send_and_ping(headers)
        peer.wait_until(lambda: child.hash_int in peer.getdata_requests)
        with node.assert_debug_log([f"Queued full block {child.hash_hex}"]):
            peer.send_and_ping(msg_no_witness_block(child))
        with node.assert_debug_log(["mutated queued block"]):
            peer.send_without_ping(msg_block(parent))
            peer.wait_for_disconnect()
        assert_equal(node.getblockcount(), height + 1)
        peer = node.add_p2p_connection(DownloadPeer())
        peer.send_and_ping(headers)
        peer.wait_until(lambda: child.hash_int in peer.getdata_requests)
        peer.send_and_ping(msg_block(child))
        assert_equal(node.getbestblockhash(), child.hash_hex)
        assert_equal(node.getblock(child.hash_hex, 0), child.serialize().hex())

        self.log.info("Persist queued full blocks when a competing branch connects first")
        height = node.getblockcount()
        fork_parent = create_block(child.hash_int, height=height + 1, ntime=block_time + 3)
        fork_parent.solve()
        fork_child = create_block(fork_parent.hash_int, height=height + 2, ntime=block_time + 4)
        fork_child.solve()
        peer.send_and_ping(msg_headers([CBlockHeader(fork_parent), CBlockHeader(fork_child)]))
        peer.wait_until(lambda: fork_child.hash_int in peer.getdata_requests)
        peer.send_and_ping(msg_block(fork_child))
        assert_raises_rpc_error(-1, "Block not available (not fully downloaded)", node.getblock, fork_child.hash_hex)
        alternate_parent = create_block(child.hash_int, height=height + 1, ntime=block_time + 10)
        alternate_parent.solve()
        alternate_child = create_block(alternate_parent.hash_int, height=height + 2, ntime=block_time + 11)
        alternate_child.solve()
        node.submitheader(CBlockHeader(alternate_parent).serialize().hex())
        assert_equal(node.submitblock(alternate_child.serialize().hex()), "inconclusive")
        assert_equal(node.submitblock(alternate_parent.serialize().hex()), None)
        assert_equal(node.getbestblockhash(), alternate_child.hash_hex)
        self.wait_until(lambda: not try_rpc(-1, "Block not available (not fully downloaded)", node.getblock, fork_child.hash_hex))
        assert_equal(node.getblock(fork_child.hash_hex, 0), fork_child.serialize().hex())


if __name__ == '__main__':
    BlockDownloadTest(__file__).main()
