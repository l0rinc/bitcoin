#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
"""Ensure suppressed transactions do not consume the global relay budget."""

from decimal import Decimal

from p2p_filter import P2PBloomFilter
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import CInv, MSG_TX, MSG_WTX, msg_feefilter, msg_filterclear, msg_inv, msg_version
from test_framework.p2p import P2PInterface, P2P_SERVICES, P2P_SUBVERSION, P2P_VERSION
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


SEND_RATE = 1
BUCKET_CAP = SEND_RATE * 30


class TxRelayRateLimitKnownTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f"-txsendrate={SEND_RATE}", "-peerbloomfilters=1"]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        node.setmocktime(1_700_000_000)
        self.generate(wallet, COINBASE_MATURITY + 2 * (BUCKET_CAP + 1) + 10)

        # A local submission with no eligible relay peer must not spend either
        # global budget or leave a backlog that cannot be delivered.
        before_no_peer = node.getnetworkinfo()["inv_buckets"]
        no_peer_tx = wallet.create_self_transfer()
        wallet.sendrawtransaction(from_node=node, tx_hex=no_peer_tx["hex"])
        after_no_peer = node.getnetworkinfo()["inv_buckets"]
        for direction in ("inbound", "outbound"):
            assert_equal(after_no_peer[direction], before_no_peer[direction])

        # A relay=0 peer still gets a TxRelay object when NODE_BLOOM is offered,
        # but it must not consume global budget before enabling relay with a
        # filter message.
        non_relay_peer = node.add_p2p_connection(
            P2PInterface(), send_version=False, wait_for_verack=False
        )
        version_without_relay = msg_version()
        version_without_relay.nVersion = P2P_VERSION
        version_without_relay.strSubVer = P2P_SUBVERSION
        version_without_relay.nServices = P2P_SERVICES
        version_without_relay.relay = 0
        non_relay_peer.send_without_ping(version_without_relay)
        non_relay_peer.wait_for_verack()
        assert_equal(node.getpeerinfo()[0]["relaytxes"], False)
        before_non_relay = node.getnetworkinfo()["inv_buckets"]
        non_relay_tx = wallet.create_self_transfer()
        wallet.sendrawtransaction(from_node=node, tx_hex=non_relay_tx["hex"])
        after_non_relay = node.getnetworkinfo()["inv_buckets"]
        for direction in ("inbound", "outbound"):
            assert_equal(after_non_relay[direction], before_non_relay[direction])
        assert_equal(non_relay_peer.message_count["inv"], 0)

        # Enabling a nonmatching BIP37 filter makes the peer relay-enabled, but
        # it still cannot receive an announcement for the filtered transaction.
        non_relay_peer.send_and_ping(P2PBloomFilter.watch_filter_init)
        before_filtered = node.getnetworkinfo()["inv_buckets"]
        filtered_tx = wallet.create_self_transfer()
        wallet.sendrawtransaction(from_node=node, tx_hex=filtered_tx["hex"])
        node.syncwithvalidationinterfacequeue()
        after_filtered = node.getnetworkinfo()["inv_buckets"]
        for direction in ("inbound", "outbound"):
            assert_equal(after_filtered[direction], before_filtered[direction])
        assert_equal(non_relay_peer.message_count["inv"], 0)

        # A peer's fee filter also makes it ineligible for a particular announcement.
        non_relay_peer.send_and_ping(msg_filterclear())
        non_relay_peer.send_and_ping(msg_feefilter(100000))
        before_fee_filtered = node.getnetworkinfo()["inv_buckets"]
        fee_filtered_tx = wallet.create_self_transfer(fee_rate=Decimal("0.00000100"))
        wallet.sendrawtransaction(from_node=node, tx_hex=fee_filtered_tx["hex"])
        node.syncwithvalidationinterfacequeue()
        after_fee_filtered = node.getnetworkinfo()["inv_buckets"]
        for direction in ("inbound", "outbound"):
            assert_equal(after_fee_filtered[direction], before_fee_filtered[direction])
        assert_equal(non_relay_peer.message_count["inv"], 0)
        non_relay_peer.peer_disconnect()
        non_relay_peer.wait_for_disconnect()

        outbound_peer = node.add_outbound_p2p_connection(
            P2PInterface(), p2p_idx=0, connection_type="outbound-full-relay"
        )
        legacy_peer = node.add_outbound_p2p_connection(
            P2PInterface(wtxidrelay=False), p2p_idx=1, connection_type="outbound-full-relay"
        )
        node.bumpmocktime(10)
        outbound_peer.sync_with_ping()
        legacy_peer.sync_with_ping()

        # Exercise the separate outbound bucket with the same known-filter
        # contract before adding inbound peers.
        for _ in range(BUCKET_CAP):
            tx = wallet.create_self_transfer()
            inv = msg_inv([CInv(MSG_WTX, int(tx["wtxid"], 16))])
            outbound_peer.send_and_ping(inv)
            legacy_peer.send_and_ping(msg_inv([CInv(MSG_TX, int(tx["txid"], 16))]))
            wallet.sendrawtransaction(from_node=node, tx_hex=tx["hex"])
            node.syncwithvalidationinterfacequeue()

        outbound_peer.sync_with_ping()
        bucket = node.getnetworkinfo()["inv_buckets"]["outbound"]
        assert_equal(bucket["backlog"], 0)
        assert_equal(bucket["count_tok"], BUCKET_CAP)
        unknown_outbound = wallet.create_self_transfer()
        wallet.sendrawtransaction(from_node=node, tx_hex=unknown_outbound["hex"])
        node.syncwithvalidationinterfacequeue()
        bucket = node.getnetworkinfo()["inv_buckets"]["outbound"]
        assert_equal(bucket["backlog"], 0)
        assert_equal(bucket["count_tok"], BUCKET_CAP - 1)
        assert_equal(outbound_peer.message_count["inv"], 0)
        assert_equal(legacy_peer.message_count["inv"], 0)

        peers = [node.add_p2p_connection(P2PInterface()) for _ in range(2)]
        for peer in peers:
            peer.sync_with_ping()

        # Tell both peers about each transaction before submitting it locally.
        # The node records those INV hashes in each peer's known filter, while
        # P2PInterface deliberately does not answer the resulting GETDATA.
        for _ in range(BUCKET_CAP):
            tx = wallet.create_self_transfer()
            inv = msg_inv([CInv(MSG_WTX, int(tx["wtxid"], 16))])
            for peer in peers:
                peer.send_and_ping(inv)
            wallet.sendrawtransaction(from_node=node, tx_hex=tx["hex"])

        for peer in peers:
            peer.sync_with_ping()
            assert_equal(peer.message_count["inv"], 0)

        bucket = node.getnetworkinfo()["inv_buckets"]["inbound"]
        assert_equal(bucket["backlog"], 0)
        assert_equal(bucket["count_tok"], BUCKET_CAP)

        # This transaction is unknown to both peers and should consume one
        # token and enter each peer's ordinary per-peer queue immediately.
        unknown = wallet.create_self_transfer()
        wallet.sendrawtransaction(from_node=node, tx_hex=unknown["hex"])
        bucket = node.getnetworkinfo()["inv_buckets"]["inbound"]
        assert_equal(bucket["backlog"], 0)
        assert_equal(bucket["count_tok"], BUCKET_CAP - 1)
        for peer in peers:
            assert_equal(peer.message_count["inv"], 0)


if __name__ == "__main__":
    TxRelayRateLimitKnownTest(__file__).main()
