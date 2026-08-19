#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Manual witness-decoding memory probe."""

import argparse
import sys

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE
from test_framework.messages import (
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    MAX_PROTOCOL_MESSAGE_LENGTH,
    msg_generic,
    ser_compact_size,
)
from test_framework.p2p import P2PInterface
from test_framework.test_shell import TestShell


STACK_ITEMS = 3_999_933


class RepeatedEmptyWitness(CTxInWitness):
    def is_null(self):
        return False

    def serialize(self):
        return ser_compact_size(STACK_ITEMS) + bytes(STACK_ITEMS)


parser = argparse.ArgumentParser(add_help=False)
parser.add_argument("--case", choices=("control", "witness"), required=True)
args, framework_args = parser.parse_known_args()
sys.argv[1:] = framework_args

test = TestShell()
try:
    test.setup(setup_clean_chain=True, extra_args=[["-dbcache=16", "-maxmempool=5", "-persistmempool=0", "-checkblockindex=0"]])
    node = test.nodes[0]
    test.generatetoaddress(node, 1, ADDRESS_BCRT1_UNSPENDABLE)
    assert not node.getblockchaininfo()["initialblockdownload"]
    peer = node.add_p2p_connection(P2PInterface())
    tx = CTransaction()
    tx.vin, tx.vout = [CTxIn()], [CTxOut()]
    tx.wit.vtxinwit = [RepeatedEmptyWitness()]
    payload = tx.serialize_with_witness()
    assert len(payload) == MAX_PROTOCOL_MESSAGE_LENGTH
    peer.send_and_ping(msg_generic(b"tx" if args.case == "witness" else b"memcontrol", payload))
finally:
    test.shutdown()
