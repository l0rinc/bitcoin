#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpers for replaying stale blocks before canonical blocks over P2P."""

from collections import defaultdict
from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
import re

from test_framework.messages import (
    CBlock,
    CBlockHeader,
    CInv,
    MAX_HEADERS_RESULTS,
    MSG_BLOCK,
    MSG_TYPE_MASK,
    from_hex,
    msg_block,
    msg_headers,
    msg_inv,
)
from test_framework.p2p import P2PInterface


STALE_BLOCK_FILE_RE = re.compile(r"^(?P<height>\d+)-(?P<hash>[0-9a-f]{64})\.bin$")


def hash_int_to_hex(hash_int):
    return f"{hash_int:064x}"


def hash_hex_to_int(hash_hex):
    return int(hash_hex, 16)


@dataclass
class StaleBlockRecord:
    height: int
    hash_int: int
    prev_hash_int: int
    block: CBlock
    path: Path


def load_stale_blocks_from_dir(blocks_dir):
    """Load stale blocks from files named <height>-<hash>.bin."""
    records = []
    for path in sorted(Path(blocks_dir).glob("*.bin")):
        match = STALE_BLOCK_FILE_RE.fullmatch(path.name)
        if match is None:
            continue
        block = CBlock()
        with path.open("rb") as block_file:
            block.deserialize(block_file)

        expected_hash = match.group("hash")
        if block.hash_hex != expected_hash:
            raise ValueError(
                "Stale block hash mismatch for {}: expected {}, got {}".format(
                    path,
                    expected_hash,
                    block.hash_hex,
                )
            )

        records.append(
            StaleBlockRecord(
                height=int(match.group("height")),
                hash_int=block.hash_int,
                prev_hash_int=block.hashPrevBlock,
                block=block,
                path=path,
            )
        )

    records.sort(key=lambda rec: (rec.height, rec.hash_int))
    return records


def group_stale_blocks_by_parent(stale_blocks):
    stale_by_parent = defaultdict(list)
    for stale_block in stale_blocks:
        stale_by_parent[stale_block.prev_hash_int].append(stale_block)
    for stale_list in stale_by_parent.values():
        stale_list.sort(key=lambda rec: (rec.height, rec.hash_int))
    return stale_by_parent


class RPCChainView:
    """Lazy RPC-backed access to active-chain headers and blocks."""

    def __init__(self, rpc):
        self.rpc = rpc
        self._active_height_by_hash = {}
        self._block_hash_by_height = {}
        self._header_by_hash = {}
        self._block_by_hash = {}

    def tip_height(self):
        return self.rpc.getblockcount()

    def get_block_hash(self, height):
        if height not in self._block_hash_by_height:
            self._block_hash_by_height[height] = self.rpc.getblockhash(height)
        return self._block_hash_by_height[height]

    def get_active_height(self, block_hash_int):
        if block_hash_int in self._active_height_by_hash:
            return self._active_height_by_hash[block_hash_int]

        try:
            header = self.rpc.getblockheader(hash_int_to_hex(block_hash_int))
        except Exception:
            self._active_height_by_hash[block_hash_int] = None
            return None

        height = header["height"] if header.get("confirmations", -1) >= 0 else None
        self._active_height_by_hash[block_hash_int] = height
        return height

    def get_header(self, block_hash_int):
        if block_hash_int not in self._header_by_hash:
            header_hex = self.rpc.getblockheader(hash_int_to_hex(block_hash_int), False)
            header = CBlockHeader()
            header.deserialize(BytesIO(bytes.fromhex(header_hex)))
            self._header_by_hash[block_hash_int] = header
        return self._header_by_hash[block_hash_int]

    def get_block(self, block_hash_int):
        if block_hash_int not in self._block_by_hash:
            block_hex = self.rpc.getblock(hash_int_to_hex(block_hash_int), 0)
            self._block_by_hash[block_hash_int] = from_hex(CBlock(), block_hex)
        return self._block_by_hash[block_hash_int]


class HistoricalReorgProxy(P2PInterface):
    """Serve stale blocks first, then canonical blocks from an RPC upstream."""

    def __init__(self, *, chain_view, stale_blocks, max_height=None):
        super().__init__()
        self.chain_view = chain_view
        self.max_height = max_height
        self.stale_blocks = list(stale_blocks)
        self.stale_by_parent = group_stale_blocks_by_parent(self.stale_blocks)
        self.stale_by_hash = {stale.hash_int: stale for stale in self.stale_blocks}
        self.served_stale_blocks = set()
        self.getdata_requests = []
        self.served_blocks = []  # (kind, height, hash_int)

    def _find_active_fork(self, locator_hashes):
        for locator_hash in locator_hashes:
            height = self.chain_view.get_active_height(locator_hash)
            if height is not None:
                return locator_hash, height
        genesis_hash = hash_hex_to_int(self.chain_view.get_block_hash(0))
        return genesis_hash, 0

    def _next_stale_candidate(self, parent_hash, hash_stop):
        for stale in self.stale_by_parent.get(parent_hash, []):
            if stale.hash_int in self.served_stale_blocks:
                continue
            if self.max_height is not None and stale.height > self.max_height:
                continue
            if hash_stop and hash_stop != stale.hash_int:
                continue
            return stale
        return None

    def on_getheaders(self, message):
        locator_hashes = message.locator.vHave
        hash_stop = message.hashstop
        fork_hash, fork_height = self._find_active_fork(locator_hashes)

        tip_height = self.chain_view.tip_height()
        if self.max_height is not None:
            tip_height = min(tip_height, self.max_height)

        next_height = fork_height + 1
        if next_height > tip_height:
            self.send_without_ping(msg_headers([]))
            return

        headers = []
        height = next_height
        parent_hash = fork_hash
        while height <= tip_height and len(headers) < MAX_HEADERS_RESULTS:
            stale = self._next_stale_candidate(parent_hash, hash_stop)
            if stale is not None:
                headers.append(CBlockHeader(stale.block))
                break

            block_hash_int = hash_hex_to_int(self.chain_view.get_block_hash(height))
            headers.append(self.chain_view.get_header(block_hash_int))
            parent_hash = block_hash_int
            if hash_stop and block_hash_int == hash_stop:
                break
            height += 1

        self.send_without_ping(msg_headers(headers))

    def on_getdata(self, message):
        for inv in message.inv:
            self.getdata_requests.append(inv.hash)
            if (inv.type & MSG_TYPE_MASK) != MSG_BLOCK:
                continue

            stale = self.stale_by_hash.get(inv.hash)
            if stale is not None:
                self.served_stale_blocks.add(stale.hash_int)
                self.served_blocks.append(("stale", stale.height, stale.hash_int))
                self.send_without_ping(msg_block(stale.block))

                # Short headers batches (well below MAX_HEADERS_RESULTS) may not
                # trigger another getheaders immediately. Announce the next
                # active block to prompt further headers sync from this point.
                next_height = stale.height + 1
                tip_height = self.chain_view.tip_height()
                if self.max_height is not None:
                    tip_height = min(tip_height, self.max_height)
                if next_height <= tip_height:
                    next_hash_int = hash_hex_to_int(self.chain_view.get_block_hash(next_height))
                    self.send_without_ping(msg_inv([CInv(MSG_BLOCK, next_hash_int)]))
                continue

            height = self.chain_view.get_active_height(inv.hash)
            self.served_blocks.append(("active", height, inv.hash))
            self.send_without_ping(msg_block(self.chain_view.get_block(inv.hash)))
