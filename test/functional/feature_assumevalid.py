#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test logic for skipping validation checks on old blocks.

Test logic for skipping validation checks on blocks which we've assumed
valid (https://github.com/bitcoin/bitcoin/pull/9484)

We build a chain that includes a duplicate coinbase, too many P2SH sigops, and
invalid P2SH script spends:

    0:        genesis block
    1:        block 1 with coinbase transaction output.
    2-100:    bury that block so the coinbase transaction output can be spent
    101:      spend the block 1 coinbase output to P2SH outputs
    102:      a block containing a duplicate coinbase and a transaction
              spending enough P2SH outputs to exceed the sigops limit. The
              P2SH spends also fail script validation.
    103-2202: bury the bad block with just over two weeks' worth of blocks
              (2100 blocks)

Start a few nodes:

    - node0 has no -assumevalid parameter. Try to sync to block 2202. It will
      reject block 102 and only sync as far as block 101
    - node1 has -assumevalid set to the hash of block 102. Try to sync to
      block 2202. node1 will sync all the way to block 2202.
    - node2 has -assumevalid set to the hash of block 102. Try to sync to
      block 200. node2 will reject block 102 since it's assumed valid, but it
      isn't buried by at least two weeks' work.
    - node3 has -assumevalid set to the hash of block 102. Feed a longer
      competing headers-only branch so block #1 is not on the best header chain.
    - node4 has -assumevalid set to the hash of block 102. Submit an alternative
      block #1 that is not part of the assumevalid chain.
    - node5 has -assumevalid set to the hash of block 102. Submit blocks after
      block 102 to show that sigops and BIP30 checks are enforced again.
    - node6 starts with no -assumevalid parameter. Reindex to hit
      "assumevalid hash not in headers" and "below minimum chainwork".
"""

import copy

from test_framework.blocktools import (
    COINBASE_MATURITY,
    MAX_BLOCK_SIGOPS,
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    CBlockHeader,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    msg_block,
    msg_headers,
)
from test_framework.p2p import P2PInterface
from test_framework.script import (
    CScript,
    OP_CHECKSIG,
    OP_TRUE,
    sign_input_legacy,
)
from test_framework.script_util import script_to_p2sh_script
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet_util import generate_keypair


class BaseNode(P2PInterface):
    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in new_blocks]
        self.send_without_ping(headers_message)


class AssumeValidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 7
        self.rpc_timeout = 120
        self.bip34_arg = "-testactivationheight=bip34@2203"
        self.extra_args = [[self.bip34_arg] for _ in range(self.num_nodes)]

    def setup_network(self):
        self.add_nodes(self.num_nodes, self.extra_args)
        # Start node0. We don't start the other nodes yet since
        # we need to pre-mine a block with invalid transactions
        # so we can pass in the block hash as assumevalid.
        self.start_node(0)

    def run_test(self):
        # Build the blockchain
        self.tip = int(self.nodes[0].getbestblockhash(), 16)
        self.block_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time'] + 1

        self.blocks = []

        # Get a pubkey for the coinbase TXO
        coinbase_key, coinbase_pubkey = generate_keypair()

        p2sh_sigops_per_input = 500
        p2sh_sigop_inputs = MAX_BLOCK_SIGOPS // p2sh_sigops_per_input + 1
        redeem_script = CScript([OP_CHECKSIG] * p2sh_sigops_per_input)
        p2sh_script = script_to_p2sh_script(redeem_script)

        # Create the first block with a coinbase output to our key
        height = 1
        block = create_block(self.tip, create_coinbase(height, coinbase_pubkey), ntime=self.block_time)
        self.blocks.append(block)
        self.block_time += 1
        block.solve()
        # Save the coinbase for later
        self.block1 = block
        self.tip = block.hash_int
        height += 1

        # Bury the block 100 deep so the coinbase output is spendable
        p2sh_funding_tx = None
        for _ in range(100):
            txlist = []
            if height == COINBASE_MATURITY + 1:
                p2sh_funding_tx = CTransaction()
                p2sh_funding_tx.vin.append(CTxIn(COutPoint(self.block1.vtx[0].txid_int, 0)))
                for _ in range(2 * p2sh_sigop_inputs):
                    p2sh_funding_tx.vout.append(CTxOut(1, p2sh_script))
                sign_input_legacy(p2sh_funding_tx, 0, self.block1.vtx[0].vout[0].scriptPubKey, coinbase_key)
                txlist.append(p2sh_funding_tx)
            block = create_block(self.tip, height=height, ntime=self.block_time, txlist=txlist)
            block.solve()
            self.blocks.append(block)
            self.tip = block.hash_int
            self.block_time += 1
            height += 1
        assert p2sh_funding_tx is not None

        tx = CTransaction()
        for output_index in range(p2sh_sigop_inputs):
            tx.vin.append(CTxIn(COutPoint(p2sh_funding_tx.txid_int, output_index), CScript([redeem_script])))
        tx.vout.append(CTxOut(1, CScript([OP_TRUE])))

        duplicate_coinbase = copy.deepcopy(self.blocks[1].vtx[0])
        block102 = create_block(self.tip, coinbase=duplicate_coinbase, ntime=self.block_time, txlist=[tx])
        self.block_time += 1
        block102.solve()
        self.blocks.append(block102)
        self.tip = block102.hash_int
        self.block_time += 1
        height += 1

        post_assumevalid_sigops_tx = CTransaction()
        for output_index in range(p2sh_sigop_inputs, 2 * p2sh_sigop_inputs):
            post_assumevalid_sigops_tx.vin.append(CTxIn(COutPoint(p2sh_funding_tx.txid_int, output_index), CScript([redeem_script])))
        post_assumevalid_sigops_tx.vout.append(CTxOut(1, CScript([OP_TRUE])))
        post_assumevalid_sigops = create_block(self.tip, height=height, ntime=self.block_time, txlist=[post_assumevalid_sigops_tx])
        post_assumevalid_sigops.solve()
        post_assumevalid_bip30 = create_block(self.tip, coinbase=copy.deepcopy(self.blocks[2].vtx[0]), ntime=self.block_time)
        post_assumevalid_bip30.solve()

        # Bury the assumed valid block 2100 deep
        for _ in range(2100):
            block = create_block(self.tip, height=height, ntime=self.block_time)
            block.solve()
            self.blocks.append(block)
            self.tip = block.hash_int
            self.block_time += 1
            height += 1
        block_1_hash = self.blocks[0].hash_hex

        self.start_node(1, extra_args=[self.bip34_arg, f"-assumevalid={block102.hash_hex}"])
        self.start_node(2, extra_args=[self.bip34_arg, f"-assumevalid={block102.hash_hex}"])
        self.start_node(3, extra_args=[self.bip34_arg, f"-assumevalid={block102.hash_hex}"])
        self.start_node(4, extra_args=[self.bip34_arg, f"-assumevalid={block102.hash_hex}"])
        self.start_node(5, extra_args=[self.bip34_arg, f"-assumevalid={block102.hash_hex}"])
        self.start_node(6)

        # nodes[0]
        self.log.info("Send blocks to node0. Block 102 will be rejected.")
        p2p0 = self.nodes[0].add_p2p_connection(BaseNode())
        p2p0.send_header_for_blocks(self.blocks[0:2000])
        p2p0.send_header_for_blocks(self.blocks[2000:])
        with self.nodes[0].assert_debug_log(expected_msgs=[
            f"Enabling script, BIP30, and sigops checks at block #1 ({block_1_hash}): assumevalid=0 (always verify).",
        ]):
            p2p0.send_and_ping(msg_block(self.blocks[0]))
        with self.nodes[0].assert_debug_log(expected_msgs=[
            "Block validation error: bad-txns-BIP30",
        ]):
            for i in range(1, 103):
                p2p0.send_without_ping(msg_block(self.blocks[i]))
            p2p0.wait_for_disconnect()
            assert_equal(self.nodes[0].getblockcount(), COINBASE_MATURITY + 1)
            assert_equal(next(filter(lambda x: x["hash"] == self.blocks[-1].hash_hex, self.nodes[0].getchaintips()))["status"], "invalid")

        # nodes[1]
        self.log.info("Send all blocks to node1. All blocks will be accepted.")
        p2p1 = self.nodes[1].add_p2p_connection(BaseNode())
        p2p1.send_header_for_blocks(self.blocks[0:2000])
        p2p1.send_header_for_blocks(self.blocks[2000:])
        with self.nodes[1].assert_debug_log(expected_msgs=[
            f"Disabling script, BIP30, and sigops checks at block #1 ({self.blocks[0].hash_hex}).",
        ]):
            p2p1.send_and_ping(msg_block(self.blocks[0]))
        with self.nodes[1].assert_debug_log(expected_msgs=[
            f"Enabling script, BIP30, and sigops checks at block #103 ({self.blocks[102].hash_hex}): block height above assumevalid height.",
        ]):
            for i in range(1, 2202):
                p2p1.send_without_ping(msg_block(self.blocks[i]))
            # Syncing 2200 blocks can take a while on slow systems. Give it plenty of time to sync.
            p2p1.sync_with_ping(timeout=960)
            assert_equal(self.nodes[1].getblockcount(), 2202)

        # nodes[2]
        self.log.info("Send blocks to node2. Block 102 will be rejected.")
        p2p2 = self.nodes[2].add_p2p_connection(BaseNode())
        p2p2.send_header_for_blocks(self.blocks[0:200])
        with self.nodes[2].assert_debug_log(expected_msgs=[
            f"Enabling script, BIP30, and sigops checks at block #1 ({block_1_hash}): block too recent relative to best header.",
        ]):
            p2p2.send_and_ping(msg_block(self.blocks[0]))
        with self.nodes[2].assert_debug_log(expected_msgs=[
            "Block validation error: bad-txns-BIP30",
        ]):
            for i in range(1, 103):
                p2p2.send_without_ping(msg_block(self.blocks[i]))
            p2p2.wait_for_disconnect()
            assert_equal(self.nodes[2].getblockcount(), COINBASE_MATURITY + 1)
            assert_equal(next(filter(lambda x: x["hash"] == self.blocks[199].hash_hex, self.nodes[2].getchaintips()))["status"], "invalid")

        # nodes[3]
        self.log.info("Send two header chains, and a block not in the best header chain to node3.")
        best_hash = self.nodes[3].getbestblockhash()
        tip_block = self.nodes[3].getblock(best_hash)
        second_chain_tip, second_chain_time, second_chain_height = int(best_hash, 16), tip_block["time"] + 1, tip_block["height"] + 1
        second_chain = []
        for _ in range(150):
            block = create_block(second_chain_tip, height=second_chain_height, ntime=second_chain_time)
            block.solve()
            second_chain.append(block)
            second_chain_tip, second_chain_time, second_chain_height = block.hash_int, second_chain_time + 1, second_chain_height + 1
        p2p3 = self.nodes[3].add_p2p_connection(BaseNode())
        p2p3.send_header_for_blocks(second_chain)
        p2p3.send_header_for_blocks(self.blocks[0:103])
        with self.nodes[3].assert_debug_log(expected_msgs=[
            f"Enabling script, BIP30, and sigops checks at block #1 ({block_1_hash}): block not in best header chain.",
        ]):
            p2p3.send_and_ping(msg_block(self.blocks[0]))
            assert_equal(self.nodes[3].getblockcount(), 1)

        # nodes[4]
        self.log.info("Send a block not in the assumevalid header chain to node4.")
        genesis_hash = self.nodes[4].getbestblockhash()
        genesis_time = self.nodes[4].getblock(genesis_hash)['time']
        alt1 = create_block(int(genesis_hash, 16), height=1, ntime=genesis_time + 2)
        alt1.solve()
        p2p4 = self.nodes[4].add_p2p_connection(BaseNode())
        p2p4.send_header_for_blocks(self.blocks[0:103])
        with self.nodes[4].assert_debug_log(expected_msgs=[
            f"Enabling script, BIP30, and sigops checks at block #1 ({alt1.hash_hex}): block not in assumevalid chain.",
        ]):
            p2p4.send_and_ping(msg_block(alt1))
            assert_equal(self.nodes[4].getblockcount(), 1)

        # nodes[5]
        self.log.info("Send blocks through assumevalid to node5, then reject invalid blocks after assumevalid.")
        p2p5 = self.nodes[5].add_p2p_connection(BaseNode())
        p2p5.send_header_for_blocks(self.blocks[0:2000])
        p2p5.send_header_for_blocks(self.blocks[2000:])
        with self.nodes[5].assert_debug_log(expected_msgs=[
            f"Disabling script, BIP30, and sigops checks at block #1 ({self.blocks[0].hash_hex}).",
        ]):
            for i in range(102):
                p2p5.send_without_ping(msg_block(self.blocks[i]))
            p2p5.sync_with_ping(timeout=960)
            assert_equal(self.nodes[5].getblockcount(), 102)
        with self.nodes[5].assert_debug_log(expected_msgs=[
            f"Enabling script, BIP30, and sigops checks at block #103 ({post_assumevalid_sigops.hash_hex}): block height above assumevalid height.",
            "Block validation error: bad-blk-sigops",
        ]):
            p2p5.send_without_ping(msg_block(post_assumevalid_sigops))
            p2p5.wait_for_disconnect()
            assert_equal(self.nodes[5].getblockcount(), 102)
        p2p5 = self.nodes[5].add_p2p_connection(BaseNode())
        with self.nodes[5].assert_debug_log(expected_msgs=[
            "Block validation error: bad-txns-BIP30",
        ]):
            p2p5.send_without_ping(msg_block(post_assumevalid_bip30))
            p2p5.wait_for_disconnect()
            assert_equal(self.nodes[5].getblockcount(), 102)

        # nodes[6]
        self.log.info("Reindex to hit specific assumevalid gates (no races with header downloads/chainwork during startup).")
        p2p6 = self.nodes[6].add_p2p_connection(BaseNode())
        p2p6.send_header_for_blocks(self.blocks[0:200])
        p2p6.send_without_ping(msg_block(self.blocks[0]))
        self.wait_until(lambda: self.nodes[6].getblockcount() == 1)
        with self.nodes[6].assert_debug_log(expected_msgs=[
            f"Enabling script, BIP30, and sigops checks at block #1 ({block_1_hash}): assumevalid hash not in headers.",
        ]):
            self.restart_node(6, extra_args=[self.bip34_arg, "-reindex-chainstate", "-assumevalid=1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"])
            assert_equal(self.nodes[6].getblockcount(), 1)
        with self.nodes[6].assert_debug_log(expected_msgs=[
            f"Enabling script, BIP30, and sigops checks at block #1 ({block_1_hash}): best header chainwork below minimumchainwork.",
        ]):
            self.restart_node(6, extra_args=[self.bip34_arg, "-reindex-chainstate", f"-assumevalid={block102.hash_hex}", "-minimumchainwork=0xffff"])
            assert_equal(self.nodes[6].getblockcount(), 1)


if __name__ == '__main__':
    AssumeValidTest(__file__).main()
