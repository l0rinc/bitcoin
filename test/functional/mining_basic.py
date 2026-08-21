#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mining RPCs

- getmininginfo
- getblocktemplate
- submitblock

mining_template_verification.py tests getblocktemplate in proposal mode"""

import copy
from decimal import Decimal

from test_framework.blocktools import (
    create_coinbase,
    get_witness_script,
    NORMAL_GBT_REQUEST_PARAMS,
    TIME_GENESIS_BLOCK,
    REGTEST_N_BITS,
    REGTEST_TARGET,
    nbits_str,
    target_str,
)
from test_framework.key import ECKey
from test_framework.messages import (
    BLOCK_HEADER_SIZE,
    CBlock,
    CBlockHeader,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    DEFAULT_BLOCK_RESERVED_WEIGHT,
    from_hex,
    MAX_BLOCK_WEIGHT,
    MAX_SEQUENCE_NONFINAL,
    MINIMUM_BLOCK_RESERVED_WEIGHT,
    ser_uint256,
    WITNESS_SCALE_FACTOR,
)
from test_framework.p2p import P2PDataStore
from test_framework.script import (
    sign_input_segwitv0,
)
from test_framework.script_util import (
    keys_to_multisig_script,
    script_to_p2wsh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    get_fee,
    JSONRPCException,
)
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)


DIFFICULTY_ADJUSTMENT_INTERVAL = 144
MAX_FUTURE_BLOCK_TIME = 2 * 3600
MAX_TIMEWARP = 600
VERSIONBITS_TOP_BITS = 0x20000000
VERSIONBITS_DEPLOYMENT_TESTDUMMY_BIT = 28
DEFAULT_BLOCK_MIN_TX_FEE = 1 # default `-blockmintxfee` setting [sat/kvB]

class MiningTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            [],
            [],
            ["-fastprune", "-prune=1"]
        ]
        self.setup_clean_chain = True

    def mine_chain(self):
        self.log.info('Create some old blocks')
        for t in range(TIME_GENESIS_BLOCK, TIME_GENESIS_BLOCK + 200 * 600, 600):
            self.nodes[0].setmocktime(t)
            self.generate(self.wallet, 1, sync_fun=self.no_op)
        mining_info = self.nodes[0].getmininginfo()
        assert_equal(mining_info['blocks'], 200)
        assert_equal(mining_info['currentblocktx'], 0)
        assert_equal(mining_info['currentblockweight'], DEFAULT_BLOCK_RESERVED_WEIGHT)

        self.log.info('test blockversion')
        self.restart_node(0, extra_args=[f'-mocktime={t}', '-blockversion=1337'])
        self.connect_nodes(0, 1)
        assert_equal(1337, self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)['version'])
        self.restart_node(0, extra_args=[f'-mocktime={t}'])
        self.connect_nodes(0, 1)
        assert_equal(VERSIONBITS_TOP_BITS + (1 << VERSIONBITS_DEPLOYMENT_TESTDUMMY_BIT), self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)['version'])
        self.restart_node(0)
        self.connect_nodes(0, 1)

    def test_fees_and_sigops(self):
        self.log.info("Test fees and sigops in getblocktemplate result")
        node = self.nodes[0]

        # Generate a coinbases with p2pk transactions for its sigops.
        wallet_sigops = MiniWallet(node, mode=MiniWalletMode.RAW_P2PK)
        self.generate(wallet_sigops, 1, sync_fun=self.no_op)

        # Mature with regular coinbases to prevent interference with other tests
        self.generate(self.wallet, 100, sync_fun=self.no_op)

        # Generate three transactions that must be mined in sequence
        #
        #      tx_a (1 sat/vbyte)
        #        |
        #        |
        #      tx_b (2 sat/vbyte)
        #        |
        #        |
        #      tx_c (3 sat/vbyte)
        #
        tx_a = wallet_sigops.send_self_transfer(from_node=node,
                                                fee_rate=Decimal("0.00001"))
        tx_b = wallet_sigops.send_self_transfer(from_node=node,
                                                fee_rate=Decimal("0.00002"),
                                                utxo_to_spend=tx_a["new_utxo"])
        tx_c = wallet_sigops.send_self_transfer(from_node=node,
                                                fee_rate=Decimal("0.00003"),
                                                utxo_to_spend=tx_b["new_utxo"])

        # Generate transaction without sigops. It will go first because it pays
        # higher fees (100 sat/vbyte) and descends from a different coinbase.
        tx_d = self.wallet.send_self_transfer(from_node=node,
                                              fee_rate=Decimal("0.00100"))

        block_template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block_template_txs = block_template['transactions']

        block_template_fees = [tx['fee'] for tx in block_template_txs]
        assert_equal(block_template_fees, [
            tx_d["fee"] * COIN,
            tx_a["fee"] * COIN,
            tx_b["fee"] * COIN,
            tx_c["fee"] * COIN
        ])
        # verify that coinbasevalue field is set to claim full block reward (subsidy + fees)
        expected_block_reward = create_coinbase(
            height=int(block_template["height"]), fees=sum(block_template_fees)).vout[0].nValue
        assert_equal(block_template["coinbasevalue"], expected_block_reward)

        block_template_sigops = [tx['sigops'] for tx in block_template_txs]
        assert_equal(block_template_sigops, [0, 4, 4, 4])

        # Clear mempool
        self.generate(self.wallet, 1, sync_fun=self.no_op)

    def test_blockmintxfee_parameter(self):
        self.log.info("Test -blockmintxfee setting")
        self.restart_node(0, extra_args=['-minrelaytxfee=0', '-persistmempool=0'])
        node = self.nodes[0]

        # test default (no parameter), zero and a bunch of arbitrary blockmintxfee rates [sat/kvB]
        for blockmintxfee_sat_kvb in (DEFAULT_BLOCK_MIN_TX_FEE, 0, 5, 10, 50, 100, 500, 1000, 2500, 5000, 21000, 333333, 2500000):
            blockmintxfee_btc_kvb = blockmintxfee_sat_kvb / Decimal(COIN)
            if blockmintxfee_sat_kvb == DEFAULT_BLOCK_MIN_TX_FEE:
                self.log.info(f"-> Default -blockmintxfee setting ({blockmintxfee_sat_kvb} sat/kvB)...")
            else:
                blockmintxfee_parameter = f"-blockmintxfee={blockmintxfee_btc_kvb:.8f}"
                self.log.info(f"-> Test {blockmintxfee_parameter} ({blockmintxfee_sat_kvb} sat/kvB)...")
                self.restart_node(0, extra_args=[blockmintxfee_parameter, '-minrelaytxfee=0', '-persistmempool=0'])
            assert_equal(node.getmininginfo()['blockmintxfee'], blockmintxfee_btc_kvb)

            # submit one tx with exactly the blockmintxfee rate, and one slightly below
            tx_with_min_feerate = self.wallet.send_self_transfer(from_node=node, fee_rate=blockmintxfee_btc_kvb, confirmed_only=True)
            assert_equal(tx_with_min_feerate["fee"], get_fee(tx_with_min_feerate["tx"].get_vsize(), blockmintxfee_btc_kvb))
            if blockmintxfee_sat_kvb >= 10:
                lowerfee_btc_kvb = blockmintxfee_btc_kvb - Decimal(10)/COIN  # 0.01 sat/vbyte lower
                assert_greater_than(blockmintxfee_btc_kvb, lowerfee_btc_kvb)
                assert_greater_than_or_equal(lowerfee_btc_kvb, 0)
                tx_below_min_feerate = self.wallet.send_self_transfer(from_node=node, fee_rate=lowerfee_btc_kvb, confirmed_only=True)
                assert_equal(tx_below_min_feerate["fee"], get_fee(tx_below_min_feerate["tx"].get_vsize(), lowerfee_btc_kvb))
            else:  # go below zero fee by using modified fees
                tx_below_min_feerate = self.wallet.send_self_transfer(from_node=node, fee_rate=blockmintxfee_btc_kvb, confirmed_only=True)
                node.prioritisetransaction(tx_below_min_feerate["txid"], 0, -11)

            # check that tx below specified fee-rate is neither in template nor in the actual block
            block_template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
            block_template_txids = [tx['txid'] for tx in block_template['transactions']]

            # Unless blockmintxfee is 0, the template shouldn't contain free transactions.
            # Note that the real block assembler uses package feerates, but we didn't create dependent transactions so it's ok to use base feerate.
            if blockmintxfee_btc_kvb > 0:
                for txid in block_template_txids:
                    tx = node.getmempoolentry(txid)
                    assert_greater_than(tx['fees']['base'], 0)

            self.generate(self.wallet, 1, sync_fun=self.no_op)
            block = node.getblock(node.getbestblockhash(), verbosity=2)
            block_txids = [tx['txid'] for tx in block['tx']]

            assert tx_with_min_feerate['txid'] in block_template_txids
            assert tx_with_min_feerate['txid'] in block_txids
            assert tx_below_min_feerate['txid'] not in block_template_txids
            assert tx_below_min_feerate['txid'] not in block_txids

            # Restart node to clear mempool for the next test
            self.restart_node(0)

    def test_timewarp(self):
        self.log.info("Test timewarp attack mitigation (BIP94)")
        node = self.nodes[0]
        self.restart_node(0, extra_args=['-test=bip94'])

        self.log.info("Mine until the last block of the retarget period")
        blockchain_info = self.nodes[0].getblockchaininfo()
        n = DIFFICULTY_ADJUSTMENT_INTERVAL - blockchain_info['blocks'] % DIFFICULTY_ADJUSTMENT_INTERVAL - 2
        t = blockchain_info['time']

        for _ in range(n):
            t += 600
            self.nodes[0].setmocktime(t)
            self.generate(self.wallet, 1, sync_fun=self.no_op)

        self.log.info("Create block two hours in the future")
        self.nodes[0].setmocktime(t + MAX_FUTURE_BLOCK_TIME)
        self.generate(self.wallet, 1, sync_fun=self.no_op)
        assert_equal(node.getblock(node.getbestblockhash())['time'], t + MAX_FUTURE_BLOCK_TIME)

        self.log.info("First block template of retarget period can't use wall clock time")
        self.nodes[0].setmocktime(t)
        # The template will have an adjusted timestamp, which we then modify
        tmpl = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_greater_than_or_equal(tmpl['curtime'], t + MAX_FUTURE_BLOCK_TIME - MAX_TIMEWARP)
        # mintime and curtime should match
        assert_equal(tmpl['mintime'], tmpl['curtime'])

        block = CBlock()
        block.nVersion = tmpl["version"]
        block.hashPrevBlock = int(tmpl["previousblockhash"], 16)
        block.nTime = tmpl["curtime"]
        block.nBits = int(tmpl["bits"], 16)
        block.nNonce = 0
        block.vtx = [create_coinbase(height=int(tmpl["height"]))]
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        assert_equal(node.getblocktemplate(template_request={
            'data': block.serialize().hex(),
            'mode': 'proposal',
            **NORMAL_GBT_REQUEST_PARAMS,
        }), None)

        bad_block = copy.deepcopy(block)
        bad_block.nTime = t
        bad_block.solve()
        assert_raises_rpc_error(-25, 'time-timewarp-attack', lambda: node.submitheader(hexdata=CBlockHeader(bad_block).serialize().hex()))

        self.log.info("Test timewarp protection boundary")
        bad_block.nTime = t + MAX_FUTURE_BLOCK_TIME - MAX_TIMEWARP - 1
        bad_block.solve()
        assert_raises_rpc_error(-25, 'time-timewarp-attack', lambda: node.submitheader(hexdata=CBlockHeader(bad_block).serialize().hex()))

        bad_block.nTime = t + MAX_FUTURE_BLOCK_TIME - MAX_TIMEWARP
        bad_block.solve()
        node.submitheader(hexdata=CBlockHeader(bad_block).serialize().hex())

    def test_pruning(self):
        self.log.info("Test that submitblock stores previously pruned block")
        prune_node = self.nodes[2]
        self.generate(prune_node, 400, sync_fun=self.no_op)
        pruned_block = prune_node.getblock(prune_node.getblockhash(2), verbosity=0)
        pruned_height = prune_node.pruneblockchain(400)
        assert_greater_than_or_equal(pruned_height, 2)
        pruned_blockhash = prune_node.getblockhash(2)

        assert_raises_rpc_error(-1, 'Block not available (pruned data)', prune_node.getblock, pruned_blockhash)

        result = prune_node.submitblock(pruned_block)
        assert_equal(result, "inconclusive")
        assert_equal(prune_node.getblock(pruned_blockhash, verbosity=0), pruned_block)


    def send_transactions(self, utxos, fee_rate, target_vsize):
        """
        Helper to create and send transactions with the specified target virtual size and fee rate.
        """
        for utxo in utxos:
            self.wallet.send_self_transfer(
                from_node=self.nodes[0],
                utxo_to_spend=utxo,
                target_vsize=target_vsize,
                fee_rate=fee_rate,
            )

    def verify_block_template(self, expected_tx_count, expected_weight):
        """
        Create a block template and check that it satisfies the expected transaction count and total weight.
        """
        response = self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        self.log.info(f"Testing block template: contains {expected_tx_count} transactions, and total weight <= {expected_weight}")
        assert_equal(len(response["transactions"]), expected_tx_count)
        total_weight = sum(transaction["weight"] for transaction in response["transactions"])
        assert_greater_than_or_equal(expected_weight, total_weight)

    def test_block_max_weight(self):
        self.log.info("Testing default and custom -blockmaxweight startup options.")

        LARGE_TXS_COUNT = 10
        LARGE_VSIZE = int(((MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT) / WITNESS_SCALE_FACTOR) / LARGE_TXS_COUNT)
        HIGH_FEERATE = Decimal("0.0003")

        # Ensure the mempool is empty
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        # Generate UTXOs and send 10 large transactions with a high fee rate
        utxos = [self.wallet.get_utxo(confirmed_only=True) for _ in range(LARGE_TXS_COUNT + 4)] # Add 4 more utxos that will be used in the test later
        self.send_transactions(utxos[:LARGE_TXS_COUNT], HIGH_FEERATE, LARGE_VSIZE)

        # Send 2 normal transactions with a lower fee rate
        NORMAL_VSIZE = int(2000 / WITNESS_SCALE_FACTOR)
        NORMAL_FEERATE = Decimal("0.0001")
        self.send_transactions(utxos[LARGE_TXS_COUNT:LARGE_TXS_COUNT + 2], NORMAL_FEERATE, NORMAL_VSIZE)

        # Check that the mempool contains all transactions
        self.log.info(f"Testing that the mempool contains {LARGE_TXS_COUNT + 2} transactions.")
        assert_equal(len(self.nodes[0].getrawmempool()), LARGE_TXS_COUNT + 2)

        # Verify the block template includes only the 10 high-fee transactions
        self.log.info("Testing that the block template includes only the 10 large transactions.")
        self.verify_block_template(
            expected_tx_count=LARGE_TXS_COUNT,
            expected_weight=MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT,
        )

        # Test block template creation with custom -blockmaxweight
        custom_block_weight = MAX_BLOCK_WEIGHT - 2000
        # Reducing the weight by 2000 units will prevent 1 large transaction from fitting into the block.
        self.restart_node(0, extra_args=[f"-blockmaxweight={custom_block_weight}"])

        self.log.info("Testing the block template with custom -blockmaxweight to include 9 large and 2 normal transactions.")
        self.verify_block_template(
            expected_tx_count=11,
            expected_weight=MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT - 2000,
        )

        # Ensure the block weight does not exceed the maximum
        self.log.info(f"Testing that the block weight will never exceed {MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT}.")
        self.restart_node(0, extra_args=[f"-blockmaxweight={MAX_BLOCK_WEIGHT}"])
        self.log.info("Sending 2 additional normal transactions to fill the mempool to the maximum block weight.")
        self.send_transactions(utxos[LARGE_TXS_COUNT + 2:], NORMAL_FEERATE, NORMAL_VSIZE)
        self.log.info(f"Testing that the mempool's weight matches the maximum block weight: {MAX_BLOCK_WEIGHT}.")
        assert_equal(self.nodes[0].getmempoolinfo()['bytes'] * WITNESS_SCALE_FACTOR, MAX_BLOCK_WEIGHT)

        self.log.info("Testing that the block template includes only 10 transactions and cannot reach full block weight.")
        self.verify_block_template(
            expected_tx_count=LARGE_TXS_COUNT,
            expected_weight=MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT,
        )

        self.log.info("Test -blockreservedweight startup option.")
        # Lowering the -blockreservedweight by 4000 will allow for two more transactions.
        self.restart_node(0, extra_args=["-blockreservedweight=4000"])
        self.verify_block_template(
            expected_tx_count=12,
            expected_weight=MAX_BLOCK_WEIGHT - 4000,
        )

        self.log.info("Test that node will fail to start when user provide invalid -blockreservedweight")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=[f"-blockreservedweight={MAX_BLOCK_WEIGHT + 1}"],
            expected_msg=f"Error: -blockreservedweight ({MAX_BLOCK_WEIGHT + 1}) exceeds consensus maximum block weight ({MAX_BLOCK_WEIGHT})",
        )

        self.log.info(f"Test that node will fail to start when user provide -blockreservedweight below {MINIMUM_BLOCK_RESERVED_WEIGHT}")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=[f"-blockreservedweight={MINIMUM_BLOCK_RESERVED_WEIGHT - 1}"],
            expected_msg=f"Error: -blockreservedweight ({MINIMUM_BLOCK_RESERVED_WEIGHT - 1}) is lower than minimum safety value of ({MINIMUM_BLOCK_RESERVED_WEIGHT})",
        )

        self.log.info("Test that node will fail to start when user provide invalid -blockmaxweight")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=[f"-blockmaxweight={MAX_BLOCK_WEIGHT + 1}"],
            expected_msg=f"Error: -blockmaxweight ({MAX_BLOCK_WEIGHT + 1}) exceeds consensus maximum block weight ({MAX_BLOCK_WEIGHT})",
        )

        self.log.info("Test that node will fail to start when -blockmaxweight is lower than -blockreservedweight")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=[f"-blockmaxweight={DEFAULT_BLOCK_RESERVED_WEIGHT - 1}"],
            expected_msg=f"Error: -blockreservedweight ({DEFAULT_BLOCK_RESERVED_WEIGHT}) exceeds -blockmaxweight ({DEFAULT_BLOCK_RESERVED_WEIGHT - 1})",
        )

    def test_witness_heavy_multisig_blocks(self):
        self.log.info("Mine blocks from standard P2WSH multisig mempool transactions")
        self.log.info("Each measured block contains many independent one-input, one-output monetary spends")
        self.log.info("Each spend has one real multisig check and is accepted into the mempool before block selection")
        node = self.nodes[0]
        if not node.running:
            self.start_node(0)
        self.wallet.rescan_utxos(include_mempool=False)

        spend_amount_sat = 100_000
        spend_fee_sat = 1_000
        funding_batch_size = 250
        standard_p2wsh_stack_items_limit = 100
        standard_p2wsh_stack_item_size_limit = 80
        standard_p2wsh_script_size_limit = 3_600
        p2wsh_policy_summary = lambda stack_items, max_item_size, script_size: f"{stack_items} non-script witness stack items, largest item {max_item_size} bytes, witness script {script_size:,} bytes"
        segwit_ratio = lambda serialized_size, stripped_size: serialized_size / stripped_size
        size_summary = lambda serialized_size, stripped_size, ratio, weight: f"{serialized_size:,} witness-inclusive bytes / {stripped_size:,} stripped bytes = {ratio:.2f}x, weight {weight:,}/{MAX_BLOCK_WEIGHT:,}"
        compact_size_len = lambda count: 1 if count < 253 else 3 if count <= 0xffff else 5 if count <= 0xffffffff else 9

        def make_keys(multisig_key_count):
            keys = []
            for private_key_number in range(1, multisig_key_count + 1):
                key = ECKey()
                key.set(secret=private_key_number.to_bytes(length=32, byteorder='big'), compressed=True)
                keys.append(key)
            return keys

        def make_funding_tx(script_pubkey, output_count):
            utxo = self.wallet.get_utxo(confirmed_only=True)
            tx = CTransaction()
            tx.vin = [CTxIn(COutPoint(int(utxo["txid"], 16), utxo["vout"]))]
            tx.vout = [CTxOut(spend_amount_sat, script_pubkey) for _ in range(output_count)]
            input_value_sat = int(utxo["value"] * COIN)
            change_sat = input_value_sat - output_count * spend_amount_sat - output_count * spend_fee_sat
            assert_greater_than(change_sat, 0)
            tx.vout.append(CTxOut(change_sat, self.wallet.get_output_script()))
            self.wallet.sign_tx(tx)
            return tx

        def make_multisig_spend(outpoint, witness_script, keys, required_signatures):
            tx = CTransaction()
            tx.vin = [CTxIn(outpoint, b'')]
            tx.vout = [CTxOut(spend_amount_sat - spend_fee_sat, self.wallet.get_output_script())]
            tx.wit.vtxinwit = [CTxInWitness()]
            tx.wit.vtxinwit[0].scriptWitness.stack = [witness_script]
            for key in reversed(keys[:required_signatures]):
                sign_input_segwitv0(tx, 0, witness_script, spend_amount_sat, key)
            signatures = tx.wit.vtxinwit[0].scriptWitness.stack[:-1]
            # CHECKMULTISIG has a historical dummy stack item before the signatures.
            tx.wit.vtxinwit[0].scriptWitness.stack = [b''] + signatures + [witness_script]
            return tx

        def candidate_result(txids):
            try:
                self.generateblock(node, output=self.wallet.get_address(), transactions=txids, submit=False, sync_fun=self.no_op)
                return True, ""
            except JSONRPCException as e:
                assert "TestBlockValidity failed" in e.error["message"]
                return False, e.error["message"]

        def largest_valid_prefix(txids):
            # This finds the exact full-block boundary quickly: N spends fit, N + 1 does not.
            all_fit, _ = candidate_result(txids)
            assert_equal(all_fit, False)
            low = 0
            high = len(txids)
            while low < high:
                mid = (low + high + 1) // 2
                fits, _ = candidate_result(txids[:mid])
                if fits:
                    low = mid
                else:
                    high = mid - 1
            next_fits, reject_reason = candidate_result(txids[:low + 1])
            assert_equal(next_fits, False)
            return low, reject_reason

        def mine_mempool_txids(txids):
            mempool = set(node.getrawmempool())
            remaining = [txid for txid in txids if txid in mempool]
            while remaining:
                fits, _ = candidate_result(remaining)
                to_mine = remaining
                if not fits:
                    count, _ = largest_valid_prefix(remaining)
                    assert_greater_than(count, 0)
                    to_mine = remaining[:count]
                mined = self.generateblock(node, output=self.wallet.get_address(), transactions=to_mine, sync_fun=self.no_op)
                assert_equal(node.getbestblockhash(), mined["hash"])
                mempool = set(node.getrawmempool())
                remaining = [txid for txid in remaining if txid in mempool]

        existing_mempool = node.getrawmempool()
        if existing_mempool:
            self.log.info(f"Setup: mining {len(existing_mempool)} existing mempool transaction(s) before the demo")
            mine_mempool_txids(existing_mempool)
            self.wallet.rescan_utxos(include_mempool=False)

        cases = [
            ("small-2-of-2", 2, 2),
            ("small-3-of-5", 3, 5),
            ("custody-20-of-20", 20, 20),
        ]

        for name, required_signatures, multisig_key_count in cases:
            multisig_name = f"{required_signatures}-of-{multisig_key_count}"
            keys = make_keys(multisig_key_count)
            witness_script = keys_to_multisig_script([key.get_pubkey().get_bytes() for key in keys], k=required_signatures)
            witness_script_size = len(witness_script)
            sigops_per_spend = witness_script.GetSigOpCount(True)
            assert_equal(sigops_per_spend, multisig_key_count)
            script_pubkey = script_to_p2wsh_script(witness_script)
            script_pubkey_size = len(script_pubkey)
            sample_tx = make_multisig_spend(COutPoint(1, 0), witness_script, keys, required_signatures)
            sample_spend_weight = sample_tx.get_weight()
            candidate_output_count = (MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT) // sample_spend_weight
            candidate_output_count += max(25, candidate_output_count // 10)
            funding_batches = []
            outpoints = []
            for first_output in range(0, candidate_output_count, funding_batch_size):
                output_count = min(funding_batch_size, candidate_output_count - first_output)
                funding_tx = make_funding_tx(script_pubkey, output_count)
                funding_batches.append(funding_tx)
                outpoints.extend(COutPoint(funding_tx.txid_int, vout) for vout in range(output_count))

            self.log.info(f"{name}: each measured transaction spends {spend_amount_sat:,} sat and pays {spend_fee_sat:,} sat fee")
            self.log.info(f"{name}: each P2WSH output is {script_pubkey_size} bytes, committing to a {witness_script_size:,}-byte {multisig_name} script")
            self.log.info(f"{name}: per spend: one {multisig_name} check, {required_signatures} signatures, {sigops_per_spend} sigops, weight about {sample_spend_weight:,}")
            self.log.info(f"{name}: setup represents {len(outpoints):,} prior P2WSH deposits created in {len(funding_batches)} standard batched funding transaction(s)")
            self.log.info(f"{name}: the measured block below contains only the later spend transactions, not the setup deposits")
            assert_greater_than_or_equal(standard_p2wsh_script_size_limit, witness_script_size)

            funding_txids = []
            for index, funding_tx in enumerate(funding_batches):
                tx_hex = funding_tx.serialize().hex()
                if index == 0:
                    accept = node.testmempoolaccept([tx_hex])[0]
                    assert_equal(accept["allowed"], True)
                funding_txids.append(node.sendrawtransaction(tx_hex))
            funding_block = self.generateblock(node, output=self.wallet.get_address(), transactions=funding_txids, sync_fun=self.no_op)
            assert_equal(node.getbestblockhash(), funding_block["hash"])
            self.wallet.rescan_utxos(include_mempool=False)

            txids = []
            for outpoint in outpoints:
                spend = make_multisig_spend(outpoint, witness_script, keys, required_signatures)
                stack = spend.wit.vtxinwit[0].scriptWitness.stack
                non_script_items = stack[:-1]
                assert_greater_than_or_equal(standard_p2wsh_stack_items_limit, len(non_script_items))
                assert_greater_than_or_equal(standard_p2wsh_stack_item_size_limit, max(len(item) for item in non_script_items))
                tx_hex = spend.serialize().hex()
                if not txids:
                    accept = node.testmempoolaccept([tx_hex])[0]
                    assert_equal(accept["allowed"], True)
                    self.log.info(f"{name}: mempool accepts the sample spend: {p2wsh_policy_summary(len(non_script_items), max(len(item) for item in non_script_items), witness_script_size)}")
                txids.append(node.sendrawtransaction(tx_hex))
                if len(txids) % 500 == 0:
                    self.log.info(f"{name}: submitted {len(txids):,} standard multisig spends to the mempool")

            assert_equal(set(txids).issubset(set(node.getrawmempool())), True)
            accepted_count, reject_reason = largest_valid_prefix(txids)
            assert_greater_than(accepted_count, 0)
            assert_greater_than(len(txids), accepted_count)
            self.log.info(f"{name}: largest candidate block uses {accepted_count:,} mempool spends")
            self.log.info(f"{name}: adding one more standard spend is rejected by block validation: {reject_reason}")

            spend_block = self.generateblock(node, output=self.wallet.get_address(), transactions=txids[:accepted_count], sync_fun=self.no_op)
            assert_equal(node.getbestblockhash(), spend_block["hash"])
            block_hex = node.getblock(spend_block["hash"], verbosity=0)
            block = from_hex(CBlock(), block_hex)
            serialized_size = len(bytes.fromhex(block_hex))
            stripped_size = len(block.serialize(with_witness=False))
            block_size_ratio = segwit_ratio(serialized_size, stripped_size)
            block_weight = block.get_weight()
            self.log.info(f"{name}: mined and accepted a valid block with {accepted_count:,} standard mempool {multisig_name} P2WSH spends")
            self.log.info(f"{name}: each spend executes one {multisig_name} multisig check and moves real value")
            self.log.info(f"{name}: serialized block size with witness: {serialized_size:,} bytes")
            self.log.info(f"{name}: same block stripped of witness data: {stripped_size:,} bytes")
            self.log.info(f"{name}: witness-inclusive block is {block_size_ratio:.2f}x the stripped block size, while still under the {MAX_BLOCK_WEIGHT:,} weight limit at {block_weight:,} weight")
            self.log.info(f"{name}: summary: mined and accepted block {spend_block['hash']} has {size_summary(serialized_size, stripped_size, block_size_ratio, block_weight)}")
            assert_greater_than(MAX_BLOCK_WEIGHT, block_weight)
            assert_greater_than(serialized_size, stripped_size)
            mine_mempool_txids(txids[accepted_count:])
            self.wallet.rescan_utxos(include_mempool=False)

        self.log.info("Simulate full blocks across the full single-CHECKMULTISIG size range")
        self.log.info("A single CHECKMULTISIG is capped at 20 pubkeys, so this sweeps 1-of-1 through 20-of-20")
        empty_block_template = self.generateblock(node, output=self.wallet.get_address(), transactions=[], submit=False, sync_fun=self.no_op)
        empty_block_hex = empty_block_template["hex"]
        empty_block = from_hex(CBlock(), empty_block_hex)
        empty_serialized_size = len(bytes.fromhex(empty_block_hex))
        empty_stripped_size = len(empty_block.serialize(with_witness=False))
        empty_weight = empty_block.get_weight()

        def simulate_full_block(sample_tx):
            sample_serialized_size = len(sample_tx.serialize())
            sample_stripped_size = len(sample_tx.serialize_without_witness())
            sample_weight = sample_tx.get_weight()

            def simulated_sizes(spend_count):
                tx_count_size_delta = compact_size_len(1 + spend_count) - compact_size_len(1)
                serialized_size = empty_serialized_size + tx_count_size_delta + spend_count * sample_serialized_size
                stripped_size = empty_stripped_size + tx_count_size_delta + spend_count * sample_stripped_size
                weight = empty_weight + 4 * tx_count_size_delta + spend_count * sample_weight
                return serialized_size, stripped_size, weight

            low = 0
            high = MAX_BLOCK_WEIGHT // sample_weight + 1
            while low < high:
                mid = (low + high + 1) // 2
                if simulated_sizes(mid)[2] <= MAX_BLOCK_WEIGHT:
                    low = mid
                else:
                    high = mid - 1
            assert_greater_than_or_equal(MAX_BLOCK_WEIGHT, simulated_sizes(low)[2])
            assert_greater_than(simulated_sizes(low + 1)[2], MAX_BLOCK_WEIGHT)
            return low, *simulated_sizes(low)

        self.log.info("curve columns: multisig, witnessScript bytes, spends that fit, serialized bytes, stripped bytes, ratio, weight")
        for multisig_key_count in range(1, 21):
            keys = make_keys(multisig_key_count)
            witness_script = keys_to_multisig_script([key.get_pubkey().get_bytes() for key in keys], k=multisig_key_count)
            sample_tx = make_multisig_spend(COutPoint(multisig_key_count, 0), witness_script, keys, multisig_key_count)
            spend_count, serialized_size, stripped_size, weight = simulate_full_block(sample_tx)
            self.log.info(
                f"curve {multisig_key_count:02d}-of-{multisig_key_count:02d}: "
                f"script {len(witness_script):>4,} B, {spend_count:>5,} spends, "
                f"serialized {serialized_size:>9,} B, stripped {stripped_size:>8,} B, "
                f"{segwit_ratio(serialized_size, stripped_size):>5.2f}x, weight {weight:>9,}; next same spend rejected"
            )

    def test_height_in_locktime(self):
        self.log.info("Sanity check generated blocks have their coinbase timelocked to their height.")
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        block = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)
        assert_equal(block["tx"][0]["locktime"], block["height"] - 1)
        assert_equal(block["tx"][0]["vin"][0]["sequence"], MAX_SEQUENCE_NONFINAL)

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.mine_chain()

        self.log.info('getmininginfo')
        mining_info = node.getmininginfo()
        assert_equal(mining_info['blocks'], 200)
        assert_equal(mining_info['chain'], self.chain)
        assert 'currentblocktx' not in mining_info
        assert 'currentblockweight' not in mining_info
        assert_equal(mining_info['bits'], nbits_str(REGTEST_N_BITS))
        assert_equal(mining_info['target'], target_str(REGTEST_TARGET))
        # We don't care about precision, round to avoid mismatch under Valgrind:
        assert_equal(round(mining_info['difficulty'], 10), Decimal('0.0000000005'))
        assert_equal(mining_info['next']['height'], 201)
        assert_equal(mining_info['next']['target'], target_str(REGTEST_TARGET))
        assert_equal(mining_info['next']['bits'], nbits_str(REGTEST_N_BITS))
        assert_equal(round(mining_info['next']['difficulty'], 10), Decimal('0.0000000005'))
        assert_equal(round(mining_info['networkhashps'], 5), Decimal('0.00333'))
        assert_equal(mining_info['pooledtx'], 0)

        self.log.info("getblocktemplate: Test default witness commitment")
        txid = int(self.wallet.send_self_transfer(from_node=node)['wtxid'], 16)
        tmpl = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)

        # Check that default_witness_commitment is present.
        assert 'default_witness_commitment' in tmpl
        witness_commitment = tmpl['default_witness_commitment']

        # Check that default_witness_commitment is correct.
        witness_root = CBlock.get_merkle_root([ser_uint256(0),
                                               ser_uint256(txid)])
        script = get_witness_script(witness_root, 0)
        assert_equal(witness_commitment, script.hex())

        # Mine a block to leave initial block download and clear the mempool
        self.generatetoaddress(node, 1, node.get_deterministic_priv_key().address)
        tmpl = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        self.log.info("getblocktemplate: Test capability advertised")
        assert 'proposal' in tmpl['capabilities']
        assert 'coinbasetxn' not in tmpl

        next_height = int(tmpl["height"])
        coinbase_tx = create_coinbase(height=next_height)
        # sequence numbers must not be max for nLockTime to have effect
        coinbase_tx.vin[0].nSequence = 2**32 - 2

        block = CBlock()
        block.nVersion = tmpl["version"]
        block.hashPrevBlock = int(tmpl["previousblockhash"], 16)
        block.nTime = tmpl["curtime"]
        block.nBits = int(tmpl["bits"], 16)
        block.nNonce = 0
        block.vtx = [coinbase_tx]
        block.hashMerkleRoot = block.calc_merkle_root()

        self.log.info("getblocktemplate: segwit rule must be set")
        assert_raises_rpc_error(-8, "getblocktemplate must be called with the segwit rule set", node.getblocktemplate, {})

        self.log.info("getblocktemplate: result should set the right rules")
        assert_equal(['csv', '!segwit', 'taproot'], self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)['rules'])

        self.log.info("submitblock: Test block decode failure")
        assert_raises_rpc_error(-22, "Block decode failed", node.submitblock, block.serialize()[:-15].hex())

        self.log.info("submitblock: Test empty block")
        assert_equal('high-hash', node.submitblock(hexdata=CBlock().serialize().hex()))

        self.log.info('submitheader tests')
        assert_raises_rpc_error(-22, 'Block header decode failed', lambda: node.submitheader(hexdata='xx' * BLOCK_HEADER_SIZE))
        assert_raises_rpc_error(-22, 'Block header decode failed', lambda: node.submitheader(hexdata='ff' * (BLOCK_HEADER_SIZE-2)))

        missing_ancestor_block = copy.deepcopy(block)
        missing_ancestor_block.hashPrevBlock = 123
        assert_raises_rpc_error(-25, 'Must submit previous header', lambda: node.submitheader(hexdata=super(CBlock, missing_ancestor_block).serialize().hex()))

        block.nTime += 1
        block.solve()

        def chain_tip(b_hash, *, status='headers-only', branchlen=1):
            return {'hash': b_hash, 'height': 202, 'branchlen': branchlen, 'status': status}

        assert chain_tip(block.hash_hex) not in node.getchaintips()
        node.submitheader(hexdata=block.serialize().hex())
        assert chain_tip(block.hash_hex) in node.getchaintips()
        node.submitheader(hexdata=CBlockHeader(block).serialize().hex())  # Noop
        assert chain_tip(block.hash_hex) in node.getchaintips()

        bad_block_root = copy.deepcopy(block)
        bad_block_root.hashMerkleRoot += 2
        bad_block_root.solve()
        assert chain_tip(bad_block_root.hash_hex) not in node.getchaintips()
        node.submitheader(hexdata=CBlockHeader(bad_block_root).serialize().hex())
        assert chain_tip(bad_block_root.hash_hex) in node.getchaintips()
        # Should still reject invalid blocks, even if we have the header:
        assert_equal(node.submitblock(hexdata=bad_block_root.serialize().hex()), 'bad-txnmrklroot')
        assert_equal(node.submitblock(hexdata=bad_block_root.serialize().hex()), 'bad-txnmrklroot')
        assert chain_tip(bad_block_root.hash_hex) in node.getchaintips()
        # We know the header for this invalid block, so should just return early without error:
        node.submitheader(hexdata=CBlockHeader(bad_block_root).serialize().hex())
        assert chain_tip(bad_block_root.hash_hex) in node.getchaintips()

        bad_block_lock = copy.deepcopy(block)
        bad_block_lock.vtx[0].nLockTime = 2**32 - 1
        bad_block_lock.hashMerkleRoot = bad_block_lock.calc_merkle_root()
        bad_block_lock.solve()
        assert_equal(node.submitblock(hexdata=bad_block_lock.serialize().hex()), 'bad-txns-nonfinal')
        assert_equal(node.submitblock(hexdata=bad_block_lock.serialize().hex()), 'duplicate-invalid')
        # Build a "good" block on top of the submitted bad block
        bad_block2 = copy.deepcopy(block)
        bad_block2.hashPrevBlock = bad_block_lock.hash_int
        bad_block2.solve()
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(hexdata=CBlockHeader(bad_block2).serialize().hex()))

        # Should reject invalid header right away
        bad_block_time = copy.deepcopy(block)
        bad_block_time.nTime = 1
        bad_block_time.solve()
        assert_raises_rpc_error(-25, 'time-too-old', lambda: node.submitheader(hexdata=CBlockHeader(bad_block_time).serialize().hex()))

        # Should ask for the block from a p2p node, if they announce the header as well:
        peer = node.add_p2p_connection(P2PDataStore())
        peer.wait_for_getheaders(timeout=5, block_hash=block.hashPrevBlock)
        peer.send_blocks_and_test(blocks=[block], node=node)
        # Must be active now:
        assert chain_tip(block.hash_hex, status='active', branchlen=0) in node.getchaintips()

        # Building a few blocks should give the same results
        self.generatetoaddress(node, 10, node.get_deterministic_priv_key().address)
        assert_raises_rpc_error(-25, 'time-too-old', lambda: node.submitheader(hexdata=CBlockHeader(bad_block_time).serialize().hex()))
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(hexdata=CBlockHeader(bad_block2).serialize().hex()))
        node.submitheader(hexdata=CBlockHeader(block).serialize().hex())
        node.submitheader(hexdata=CBlockHeader(bad_block_root).serialize().hex())
        assert_equal(node.submitblock(hexdata=block.serialize().hex()), 'duplicate')  # valid

        self.test_fees_and_sigops()
        self.test_blockmintxfee_parameter()
        self.test_block_max_weight()
        self.test_witness_heavy_multisig_blocks()
        self.test_timewarp()
        self.test_pruning()
        self.test_height_in_locktime()


if __name__ == '__main__':
    MiningTest(__file__).main()
