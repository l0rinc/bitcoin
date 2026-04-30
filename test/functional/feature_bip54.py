#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# Test Consensus Cleanup soft fork (BIP 54)

from test_framework.blocktools import (
    add_witness_commitment,
    create_block,
    create_empty_fork,
    create_tx_with_script,
    MAX_FUTURE_BLOCK_TIME,
    NORMAL_GBT_REQUEST_PARAMS,
)
from test_framework.messages import (
    CBlockHeader,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    SEQUENCE_FINAL,
)
from test_framework.script import (
    CScript,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_DUP,
    OP_ENDIF,
    OP_NOT,
    OP_NOTIF,
    OP_1,
)
from test_framework.script_util import (
    MAX_TX_BIP54_SIGOPS,
    PAY_TO_ANCHOR,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import generate_keypair


class Bip54Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # BIP54 will be activated halfway through the test.
        self.extra_args = [["-vbparams=consensuscleanup:0:3999999999"]]

    def generate_bulk(self, node, count, version=None):
        """Generate a number of blocks at once, controlling the block version."""
        for block in create_empty_fork(node, count, version=version):
            assert_equal(node.submitblock(block.serialize().hex()), None)

    def mine_and_submit(self, node, txs):
        """Mine these transactions into a block and submit it to the provided node, returning its rejection reason if any."""
        template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block = create_block(txlist=txs, tmpl=template)
        add_witness_commitment(block)
        block.solve()
        return node.submitblock(block.serialize().hex())

    def create_prep_tx(self, spk):
        """Create a transaction that spends the full value of a wallet coin to the given scriptPubKey."""
        prevout = self.wallet.get_utxo(confirmed_only=True)
        prevout_sats = int(COIN * prevout["value"])
        prep_tx = CTransaction()
        prep_tx.vin = [CTxIn(COutPoint(int(prevout['txid'], 16), prevout['vout']))]
        prep_tx.vout = [CTxOut(prevout_sats, spk)]
        self.wallet.sign_tx(prep_tx)
        return prep_tx

    def create_spend_tx(self, prev_tx, script_sig):
        """Create a transaction that spends the first output of the provided transaction to the wallet scriptPubKey."""
        return create_tx_with_script(prev_tx, 0, script_sig, amount=prev_tx.vout[0].nValue, output_script=self.wallet.get_output_script())

    def create_txs_many_sigops(self):
        """Create a list of transactions that fits in a single block and violates the BIP54 sigops limit. """
        # A bare scriptPubKey that accounts for 2500 sigops, 20 for each CHECKMULTISIG
        prep_spk = CScript([OP_DUP, OP_NOTIF] + [OP_CHECKMULTISIG] * (MAX_TX_BIP54_SIGOPS // 20) + [OP_ENDIF])
        prep_tx = self.create_prep_tx(prep_spk)

        # One more sigop in the scriptSig to make it exceed the limit
        _, dummy_pubkey = generate_keypair()
        script_sig = CScript([b"", dummy_pubkey, OP_CHECKSIG, OP_NOT])
        tx = self.create_spend_tx(prep_tx, script_sig)

        return [prep_tx, tx]

    def submit_block_many_sigops(self, node):
        """Create and submit a block that violates the BIP54 sigops limit."""
        return self.mine_and_submit(node, self.create_txs_many_sigops())

    def submit_block_many_sigops_split(self, node):
        """Create and submit a block that exceeds the BIP54 sigops limit but in multiple txs."""
        # Get the very same transactions, except we drop the sigop in the last tx..
        txs = self.create_txs_many_sigops()
        last_tx = txs[-1]
        last_tx.vin[0].scriptSig = CScript([OP_1])

        # ..And add it to a separate transaction instead.
        _, dummy_pubkey = generate_keypair()
        prep_spk = CScript([b"", dummy_pubkey, OP_CHECKSIG, OP_NOT])
        prep_tx = self.create_prep_tx(prep_spk)
        tx = self.create_spend_tx(prep_tx, CScript())
        txs += [prep_tx, tx]

        # Now submit the two pairs of transactions in a single block.
        return self.mine_and_submit(node, txs)

    def timewarp_attack(self, node, fork_height):
        """Perform a pseudo Timewarp attack. Pseudo because regtest does not have retargets, so we only do the first period."""
        # Fork from the first block of an existing difficulty adjustment period.
        assert_equal(fork_height % 144, 0)
        fork_header = node.getblockheader(node.getblockhash(fork_height))

        # Record the start time to later mock elapsed time as we mined an entire period of blocks.
        start_time = fork_header["time"]

        height = fork_height + 1
        prev_hash = int(fork_header["hash"], 16)

        def submit_header(ntime):
            """Mine a block with this timestamp on top of the previous one and submit its header."""
            nonlocal height, prev_hash
            block = create_block(prev_hash, height=height, ntime=ntime)
            block.solve()
            node.submitheader(CBlockHeader(block).serialize().hex())
            prev_hash = block.hash_int
            height += 1

        # Now mine all blocks until the very last by holding back timestamps as much as MTP allows.
        ntime = start_time + 1
        while height % 144 < 143:
            submit_header(ntime)
            if height % 6 == 0:
                ntime += 1

        # Pretend it takes us 10 minutes per block interval to mine this difficulty adjustment
        # (including the 144th block below).
        mocked_time = start_time + 143 * 10 * 60
        node.setmocktime(mocked_time)

        # Mine the last block of the period as far in the future as possible.
        submit_header(mocked_time + MAX_FUTURE_BLOCK_TIME)

        # Finally, reset the time back to that of the 5 blocks prior when mining the first block
        # of the next period. If we were not on regtest the difficulty would have been dropped
        # slightly and we would be set to reduce it by 4x at the end of the upcoming difficulty
        # adjustment period. If BIP54 is active this block will be refused by the node.
        assert_equal(height % 144, 0)
        try:
            submit_header(start_time + 144 // 6 + 1)
        finally:
            # We only submit headers because it's sufficient to test the mitigation and so that we don't
            # affect the other tests. Reset the time so those can mine blocks normally.
            node.setmocktime(0)

    def murch_zawy_attack(self, node, fork_height):
        """Perform a pseudo Murch-Zawy attack. Pseudo because regtest does not have retargets, so we only do the first 3 periods."""
        # Expected duration in seconds of a regtest retarget interval (by the incorrect formula).
        interval_expected_sec = 144 * 10 * 60

        # Start on a fresh retarget period, forking from the last block of an existing one.
        assert_equal(fork_height % 144, 143)
        prev_header = node.getblockheader(node.getblockhash(fork_height))

        # The Murch-Zawy attack reduces the difficulty by pushing timestamp forwards and back
        # to the minimum time, exploiting the difficulty reduction floor of /4 and the fact
        # that the elapsed time in a retarget period is allowed to be negative. See for more
        # https://delvingbitcoin.org/t/zawy-s-alternating-timestamp-attack/1062 (especially
        # the "Variant on Zawy's Attack" section).
        # Because timestamps are postdated, blocks can't be submitted until later. Record
        # them in the meantime then submit them all at once.
        blocks = []
        height = fork_height + 1
        prev_hash = int(prev_header["hash"], 16)

        def mine(ntime):
            """Mine a block with this timestamp on top of the previous one, to be submitted later."""
            nonlocal height, prev_hash
            block = create_block(prev_hash, height=height, ntime=ntime)
            block.solve()
            blocks.append(block)
            prev_hash = block.hash_int
            height += 1

        # Mine the first retarget period, holding back the timestamps of all blocks in the
        # period except the very last one.
        ntime = prev_header["time"] + 1
        first_period_start_time = ntime
        while height % 144 < 143:
            mine(ntime)
            if height % 6 == 0:
                ntime += 1

        # The timestamp of the last block of the period is set to maximally reduce difficulty.
        first_period_end_time = first_period_start_time + 4 * interval_expected_sec
        mine(first_period_end_time)

        # This attack also works with a strict timewarp fix. Mine the first block of the
        # next retarget period with a timestamp no less than that of the last block of the
        # previous period.
        second_period_start_time = first_period_end_time
        mine(second_period_start_time)
        ntime += 1

        # Mine the second retarget period, holding back timestamps.
        while height % 144 < 143:
            mine(ntime)
            if height % 4 == 0:
                ntime += 1

        # The timestamp of the last block of the period is again set to maximally reduce difficulty.
        # It must be ratcheted from this period's first block: reusing the previous period's
        # closing timestamp would make this period's elapsed time zero, which the clamp turns
        # into a 4x difficulty increase instead of the intended reduction.
        second_period_end_time = second_period_start_time + 4 * interval_expected_sec
        mine(second_period_end_time)

        # And the first block of the following period with the same time, to respect the timewarp fix.
        mine(second_period_end_time)
        ntime += 1

        # Mine one last full period of blocks, holding back timestamps. This time, include the last
        # block of the period.
        while height % 144 != 0:
            mine(ntime)
            if height % 4 == 0:
                ntime += 1

        # Check that the three periods exercise the retarget clamps claimed below.
        assert_equal(len(blocks), 3 * 144)
        period_timespans = [
            blocks[143].nTime - blocks[0].nTime,
            blocks[287].nTime - blocks[144].nTime,
            blocks[431].nTime - blocks[288].nTime,
        ]
        clamped_timespans = [
            max(interval_expected_sec // 4, min(timespan, 4 * interval_expected_sec))
            for timespan in period_timespans
        ]
        assert_equal(clamped_timespans, [
            4 * interval_expected_sec,
            4 * interval_expected_sec,
            interval_expected_sec // 4,
        ])

        # Now if regtest actually had difficulty adjustment we'd be in a situation where the time
        # has been reset to its minimal value, yet the difficulty is 4x lower than when we started
        # the attack (because it was /4 twice and only *4 once). In an actual Murch-Zawy attack,
        # Mallory would repeat the process until the post-dated blocks can be submitted. A single
        # round is sufficient to demonstrate the attack, so we'll move forward and submit our chain.
        node.setmocktime(second_period_end_time)
        try:
            for block in blocks:
                node.submitheader(CBlockHeader(block).serialize().hex())
        finally:
            # We only submit headers because it's sufficient to test the mitigation and so that we don't
            # affect the other tests. Reset the time so those can mine blocks normally.
            node.setmocktime(0)

    def submit_nontimelocked_cb(self, node):
        """Submit a block with its coinbase transaction not timelocked to its height."""
        template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block = create_block(tmpl=template)
        block.vtx[0].nLockTime -= 1
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        return node.submitblock(block.serialize().hex())

    def submit_final_sequence_cb(self, node):
        """Submit a block with its coinbase timelocked to its height but with a final sequence, which disables the timelock."""
        template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block = create_block(tmpl=template)
        block.vtx[0].vin[0].nSequence = SEQUENCE_FINAL
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        return node.submitblock(block.serialize().hex())

    def submit_block_64byte(self, node):
        """Submit a block that contains a 64-byte (Segwit) transaction."""
        # Create a 64-byte tx that spends a single Segwit input and contains a single p2a output.
        tx = self.create_prep_tx(PAY_TO_ANCHOR)
        assert_equal(len(tx.serialize_without_witness()), 64)

        # Mine a block containing that transaction.
        return self.mine_and_submit(node, [tx])

    def submit_block_64byte_coinbase(self, node):
        """Submit a block that contains a 64-byte coinbase transaction."""
        template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block = create_block(tmpl=template)
        # The pre-padding default coinbase script, which yields a 64-byte coinbase at the heights used in this test.
        block.vtx[0].vout[0].scriptPubKey = CScript([OP_1])
        assert_equal(len(block.vtx[0].serialize_without_witness()), 64)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        return node.submitblock(block.serialize().hex())

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        self.log.info("BIP54 tests before activation")
        # When BIP54 is not active we:
        # - Accept a block with more than 2500 BIP54-sigops in either a single transaction or
        #   split across multiple transactions.
        assert_equal(self.submit_block_many_sigops(node), None)
        assert_equal(self.submit_block_many_sigops_split(node), None)
        # - Can perform Timewarp and Murch-Zawy attacks.
        self.timewarp_attack(node, fork_height=144)
        self.murch_zawy_attack(node, fork_height=143)
        # - Accept a block with its coinbase not timelocked to the block height, or with its timelock disabled.
        assert_equal(self.submit_nontimelocked_cb(node), None)
        assert_equal(self.submit_final_sequence_cb(node), None)
        # - Accept a block containing a 64-byte transaction.
        assert_equal(self.submit_block_64byte(node), None)
        # - Even if that is the coinbase transaction.
        assert_equal(self.submit_block_64byte_coinbase(node), None)

        self.log.info("Activating BIP54")
        # Mine all the way to the very last block of the current retarget period.
        self.generate_bulk(node, 143 - node.getblockcount() % 144)

        # Mine enough signaling blocks to transition to locked in and reach the next period.
        cc_dep = node.getdeploymentinfo()["deployments"]["consensuscleanup"]["bip9"]
        assert_equal(cc_dep["status"], "started")
        signal_bip54 = (1 << 29) | (1 << cc_dep["bit"])
        thresh = cc_dep["statistics"]["threshold"]
        self.generate_bulk(node, thresh, version=signal_bip54)
        self.generate_bulk(node, 144 - thresh + 1)

        # Mine one more retarget period to transition from locked in to active.
        cc_dep = node.getdeploymentinfo()["deployments"]["consensuscleanup"]["bip9"]
        assert_equal(cc_dep["status"], "locked_in")
        self.generate_bulk(node, 144)
        cc_dep = node.getdeploymentinfo()["deployments"]["consensuscleanup"]["bip9"]
        assert_equal(cc_dep["status"], "active")

        self.log.info("BIP54 tests after activation")
        # Once BIP54 is active we:
        # - Refuse blocks with more than 2500 BIP54-sigops in a single transaction but still
        #   accept a block with the same number of BIP54-sigops split in more than one transaction.
        assert_equal(self.submit_block_many_sigops(node), None) # TODO: A transaction with more than 2500 legacy sigops must be invalid
        assert_equal(self.submit_block_many_sigops_split(node), None)
        # - Cannot perform Timewarp and Murch-Zawy attacks
        self.timewarp_attack(node, fork_height=cc_dep["since"]) # TODO: The first retarget block must respect the two-hour timewarp bound
        self.murch_zawy_attack(node, fork_height=cc_dep["since"] - 1) # TODO: Retarget periods must not have a negative duration
        # - Refuse blocks with a coinbase not timelocked to the block height, or with its timelock disabled
        assert_equal(self.submit_nontimelocked_cb(node), None) # TODO: The coinbase must enforce its height timelock
        assert_equal(self.submit_final_sequence_cb(node), None) # TODO: The coinbase must enforce its height timelock
        # - Refuse a block containing a 64-byte transaction
        assert_equal(self.submit_block_64byte(node), None) # TODO: Transactions with exactly 64 non-witness bytes must be invalid
        # - Even if that is the coinbase transaction
        assert_equal(self.submit_block_64byte_coinbase(node), None) # TODO: Transactions with exactly 64 non-witness bytes must be invalid

if __name__ == "__main__":
    Bip54Test(__file__).main()
