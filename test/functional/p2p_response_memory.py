#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Manual large-response memory probe."""

import argparse
import socket
import sys
import time

from test_framework.blocktools import add_witness_commitment, create_block, create_coinbase
from test_framework.messages import (
    CInv,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    MAGIC_BYTES,
    MSG_BLOCK,
    MSG_WITNESS_FLAG,
    NODE_NETWORK,
    NODE_WITNESS,
    hash256,
    msg_getdata,
    msg_verack,
    msg_version,
)
from test_framework.p2p import P2P_SUBVERSION, P2P_VERSION
from test_framework.script import CScript, OP_2DROP, OP_TRUE
from test_framework.script_util import script_to_p2wsh_script
from test_framework.test_shell import TestShell
from test_framework.util import assert_equal, p2p_port


INPUTS = 15
STACK_ITEMS = 400
STACK_ITEM_SIZE = 520


def frame(message):
    payload = message.serialize()
    return (
        MAGIC_BYTES["regtest"]
        + message.msgtype.ljust(12, b"\x00")
        + len(payload).to_bytes(4, "little")
        + hash256(payload)[:4]
        + payload
    )


def recv_exact(peer, length):
    data = b""
    while len(data) < length:
        chunk = peer.recv(length - len(data))
        if not chunk:
            raise ConnectionError("peer disconnected during handshake")
        data += chunk
    return data


def recv_message(peer):
    header = recv_exact(peer, 24)
    assert_equal(header[:4], MAGIC_BYTES["regtest"])
    payload_length = int.from_bytes(header[16:20], "little")
    payload = recv_exact(peer, payload_length)
    assert_equal(header[20:24], hash256(payload)[:4])
    return header[4:16].rstrip(b"\x00")


def connect_stalled_peer(port):
    peer = socket.socket()
    peer.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
    peer.settimeout(5)
    peer.connect(("127.0.0.1", port))

    version = msg_version()
    version.nVersion = P2P_VERSION
    version.nServices = NODE_NETWORK | NODE_WITNESS
    version.strSubVer = P2P_SUBVERSION
    peer.sendall(frame(version))
    sent_verack = False
    while True:
        command = recv_message(peer)
        if command == b"version" and not sent_verack:
            peer.sendall(frame(msg_verack()))
            sent_verack = True
        if command == b"verack":
            return peer


def create_large_block(node, test):
    tip = int(node.getbestblockhash(), 16)
    block_time = node.getblock(node.getbestblockhash())["time"] + 1
    first_block = create_block(tip, height=1, ntime=block_time)
    first_block.solve()
    assert_equal(node.submitblock(first_block.serialize().hex()), None)
    test.generate(node, 100)

    witness_script = CScript([OP_2DROP] * (STACK_ITEMS // 2) + [OP_TRUE])
    funding = CTransaction()
    funding.vin = [CTxIn(COutPoint(first_block.vtx[0].txid_int, 0))]
    funding.vout = [CTxOut(COIN, script_to_p2wsh_script(witness_script)) for _ in range(INPUTS)]

    height = node.getblockcount() + 1
    tip = int(node.getbestblockhash(), 16)
    block_time = node.getblock(node.getbestblockhash())["time"] + 1
    funding_block = create_block(tip, create_coinbase(height), ntime=block_time, txlist=[funding])
    funding_block.solve()
    assert_equal(node.submitblock(funding_block.serialize().hex()), None)

    response = CTransaction()
    response.vin = [CTxIn(COutPoint(funding.txid_int, index)) for index in range(INPUTS)]
    response.vout = [CTxOut((INPUTS - 1) * COIN, CScript([OP_TRUE]))]
    for _ in range(INPUTS):
        witness = CTxInWitness()
        witness.scriptWitness.stack = [bytes(STACK_ITEM_SIZE)] * STACK_ITEMS + [witness_script]
        response.wit.vtxinwit.append(witness)

    height += 1
    response_block = create_block(
        funding_block.hash_int,
        create_coinbase(height),
        ntime=block_time + 1,
        txlist=[response],
    )
    add_witness_commitment(response_block)
    response_block.solve()
    assert response_block.get_weight() <= 4_000_000
    assert_equal(node.submitblock(response_block.serialize().hex()), None)
    return response_block


parser = argparse.ArgumentParser(add_help=False)
parser.add_argument("--case", choices=("control", "getdata"), required=True)
parser.add_argument("--peers", type=int, default=32)
parser.add_argument("--hold-seconds", type=int, default=5)
args, framework_args = parser.parse_known_args()
sys.argv[1:] = framework_args

test = TestShell()
peers = []
try:
    test.setup(
        setup_clean_chain=True,
        extra_args=[["-dbcache=16", "-maxmempool=5", "-persistmempool=0", "-checkblockindex=0", "-v2transport=0"]],
    )
    node = test.nodes[0]
    block = create_large_block(node, test)
    print(f"block_bytes={len(block.serialize())} peers={args.peers} case={args.case}")

    peers = [connect_stalled_peer(p2p_port(0)) for _ in range(args.peers)]
    if args.case == "getdata":
        request = frame(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, block.hash_int)]))
        for peer in peers:
            peer.sendall(request)
    time.sleep(args.hold_seconds)
finally:
    for peer in peers:
        peer.close()
    test.shutdown()
