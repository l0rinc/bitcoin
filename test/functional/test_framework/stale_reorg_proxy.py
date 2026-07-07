#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpers for replaying stale blocks before canonical blocks over P2P."""

from bisect import bisect_left
from collections import OrderedDict, defaultdict
from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
import binascii
import random
import re
import time
import traceback

from test_framework.messages import (
    CBlockHeader,
    CInv,
    MAX_HEADERS_RESULTS,
    MSG_BLOCK,
    MSG_TYPE_MASK,
    msg_headers,
    msg_inv,
    msg_notfound,
)
from test_framework.p2p import (
    P2P_SERVICES,
    P2PInterface,
)
from test_framework.v2_p2p import EncryptedP2PState


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
    header: CBlockHeader
    raw_block: bytes
    path: Path


class RawBlockMessage:
    """P2P `block` message carrying pre-serialized block bytes."""

    msgtype = b"block"

    def __init__(self, payload):
        self.payload = payload

    def serialize(self):
        return self.payload


def load_stale_blocks_from_dir(blocks_dir):
    """Load stale blocks from files named <height>-<hash>.bin."""
    records = []
    for path in sorted(Path(blocks_dir).glob("*.bin")):
        match = STALE_BLOCK_FILE_RE.fullmatch(path.name)
        if match is None:
            continue
        raw_block = path.read_bytes()
        header = CBlockHeader()
        try:
            # Parse only the header to avoid retaining large per-tx structures.
            header.deserialize(BytesIO(raw_block))
        except Exception as exc:
            print(f"Skipping malformed stale block file {path}: {exc!r}", flush=True)
            continue

        expected_hash = match.group("hash")
        if header.hash_hex != expected_hash:
            print(
                "Skipping stale block hash mismatch for {}: expected {}, got {}".format(
                    path, expected_hash, header.hash_hex
                ),
                flush=True,
            )
            continue

        records.append(
            StaleBlockRecord(
                height=int(match.group("height")),
                hash_int=header.hash_int,
                prev_hash_int=header.hashPrevBlock,
                header=header,
                raw_block=raw_block,
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

    def __init__(
        self,
        rpc,
        *,
        block_raw_cache_max_bytes=256 * 1024 * 1024,
        rpc_batch_size=64,
        rpc_header_batch_size=512,
        rpc_hash_batch_size=1024,
        tip_height_cache_seconds=1.0,
    ):
        self.rpc = rpc
        self._active_height_by_hash = {}
        self._block_hash_by_height = {}
        self._header_by_hash = {}
        self._block_raw_by_hash = OrderedDict()
        self._block_raw_cache_bytes = 0
        self._block_raw_cache_max_bytes = int(block_raw_cache_max_bytes)
        self._rpc_batch_size = max(1, int(rpc_batch_size))
        self._rpc_header_batch_size = max(1, int(rpc_header_batch_size))
        self._rpc_hash_batch_size = max(1, int(rpc_hash_batch_size))
        self._raw_cache_evictions = 0
        self._raw_block_size_ema = None
        self._raw_block_size_samples = 0
        self._raw_block_size_recent_max = None
        self._tip_height_cache_seconds = max(0.0, float(tip_height_cache_seconds))
        self._tip_height_cache_value = None
        self._tip_height_cache_at = 0.0

    def _note_raw_block_size(self, raw_len):
        # Track a smoothed estimate so prefetch windows can adapt as blocks grow.
        raw_len = int(raw_len)
        if raw_len <= 0:
            return
        alpha = 0.02
        if self._raw_block_size_ema is None:
            self._raw_block_size_ema = float(raw_len)
        else:
            self._raw_block_size_ema = (1.0 - alpha) * self._raw_block_size_ema + alpha * float(raw_len)
        self._raw_block_size_samples += 1
        if self._raw_block_size_recent_max is None:
            self._raw_block_size_recent_max = float(raw_len)
        else:
            # Decay slowly so a recent large block quickly clamps batch sizing.
            self._raw_block_size_recent_max = max(
                float(raw_len),
                self._raw_block_size_recent_max * 0.99,
            )

    def estimated_raw_block_size(self):
        # Before we have any samples, assume a modest historical mainnet block.
        # This is only used to bound prefetch work.
        if self._raw_block_size_ema is None:
            return 150_000
        est = float(self._raw_block_size_ema)
        if self._raw_block_size_recent_max is not None:
            est = max(est, float(self._raw_block_size_recent_max))
        return max(1, int(est))

    def estimated_raw_cache_capacity_blocks(self, *, fill_ratio=0.80):
        max_bytes = int(self._block_raw_cache_max_bytes)
        if max_bytes <= 0:
            return 0
        est = self.estimated_raw_block_size()
        if est <= 0:
            return 0
        return max(0, int((max_bytes * float(fill_ratio)) // est))

    def raw_cache_stats(self):
        return {
            "entries": len(self._block_raw_by_hash),
            "bytes": self._block_raw_cache_bytes,
            "max_bytes": self._block_raw_cache_max_bytes,
            "evictions": self._raw_cache_evictions,
        }

    def _cache_get_raw(self, block_hash_int):
        raw = self._block_raw_by_hash.get(block_hash_int)
        if raw is None:
            return None
        self._block_raw_by_hash.move_to_end(block_hash_int)
        return raw

    def _cache_put_raw(self, block_hash_int, raw):
        if self._block_raw_cache_max_bytes <= 0:
            return
        raw_len = len(raw)
        if raw_len > self._block_raw_cache_max_bytes:
            # Block too large to cache under the configured cap.
            return
        self._note_raw_block_size(raw_len)
        existing = self._block_raw_by_hash.pop(block_hash_int, None)
        if existing is not None:
            self._block_raw_cache_bytes -= len(existing)
        self._block_raw_by_hash[block_hash_int] = raw
        self._block_raw_cache_bytes += raw_len
        while (
            self._block_raw_cache_bytes > self._block_raw_cache_max_bytes
            and self._block_raw_by_hash
        ):
            _, evicted = self._block_raw_by_hash.popitem(last=False)
            self._block_raw_cache_bytes -= len(evicted)
            self._raw_cache_evictions += 1

    def tip_height(self):
        now = time.monotonic()
        if (
            self._tip_height_cache_value is not None
            and self._tip_height_cache_seconds > 0.0
            and now - self._tip_height_cache_at <= self._tip_height_cache_seconds
        ):
            return self._tip_height_cache_value
        tip = self.rpc.getblockcount()
        self._tip_height_cache_value = tip
        self._tip_height_cache_at = now
        return tip

    def cached_tip(self):
        if not self._block_hash_by_height:
            return None, None
        tip_height = max(self._block_hash_by_height.keys())
        return hash_hex_to_int(self._block_hash_by_height[tip_height]), tip_height

    def get_block_hash(self, height):
        if height not in self._block_hash_by_height:
            block_hash_hex = self.rpc.getblockhash(height)
            self._block_hash_by_height[height] = block_hash_hex
            self._active_height_by_hash[hash_hex_to_int(block_hash_hex)] = height
        return self._block_hash_by_height[height]

    def get_block_hashes_range(self, start_height, end_height):
        if end_height < start_height:
            return []

        heights = list(range(start_height, end_height + 1))
        missing = [height for height in heights if height not in self._block_hash_by_height]
        raw_rpc = getattr(self.rpc, "_rpc", None)
        if missing and raw_rpc is not None and hasattr(self.rpc, "batch"):
            unresolved = []
            for offset in range(0, len(missing), self._rpc_hash_batch_size):
                chunk = missing[offset : offset + self._rpc_hash_batch_size]
                requests = [raw_rpc.getblockhash.get_request(height) for height in chunk]
                responses = self.rpc.batch(requests)
                for height, response in zip(chunk, responses):
                    block_hash_hex = response.get("result")
                    if response.get("error") is not None or block_hash_hex is None:
                        unresolved.append(height)
                        continue
                    self._block_hash_by_height[height] = block_hash_hex
                    self._active_height_by_hash[hash_hex_to_int(block_hash_hex)] = height
            missing = unresolved

        for height in missing:
            self.get_block_hash(height)

        return [self._block_hash_by_height[height] for height in heights]

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

    def get_active_heights_batch(self, block_hash_ints):
        result = {}
        missing = []
        for block_hash_int in block_hash_ints:
            if block_hash_int in self._active_height_by_hash:
                result[block_hash_int] = self._active_height_by_hash[block_hash_int]
            else:
                missing.append(block_hash_int)

        if missing:
            raw_rpc = getattr(self.rpc, "_rpc", None)
            if raw_rpc is not None and hasattr(self.rpc, "batch"):
                unresolved = []
                for offset in range(0, len(missing), self._rpc_header_batch_size):
                    chunk = missing[offset : offset + self._rpc_header_batch_size]
                    requests = [
                        raw_rpc.getblockheader.get_request(hash_int_to_hex(block_hash_int))
                        for block_hash_int in chunk
                    ]
                    responses = self.rpc.batch(requests)
                    for block_hash_int, response in zip(chunk, responses):
                        header = response.get("result")
                        if response.get("error") is not None or header is None:
                            unresolved.append(block_hash_int)
                            continue
                        height = (
                            header["height"]
                            if header.get("confirmations", -1) >= 0
                            else None
                        )
                        self._active_height_by_hash[block_hash_int] = height
                        result[block_hash_int] = height
                missing = unresolved

        for block_hash_int in missing:
            result[block_hash_int] = self.get_active_height(block_hash_int)

        return result

    def get_header(self, block_hash_int):
        if block_hash_int not in self._header_by_hash:
            header_hex = self.rpc.getblockheader(hash_int_to_hex(block_hash_int), False)
            header = CBlockHeader()
            header.deserialize(BytesIO(bytes.fromhex(header_hex)))
            self._header_by_hash[block_hash_int] = header
        return self._header_by_hash[block_hash_int]

    def get_headers_batch(self, block_hash_ints):
        result = {}
        missing = []
        for block_hash_int in block_hash_ints:
            header = self._header_by_hash.get(block_hash_int)
            if header is not None:
                result[block_hash_int] = header
            else:
                missing.append(block_hash_int)

        if not missing:
            return result

        raw_rpc = getattr(self.rpc, "_rpc", None)
        if raw_rpc is not None and hasattr(self.rpc, "batch"):
            unresolved = []
            for offset in range(0, len(missing), self._rpc_header_batch_size):
                chunk = missing[offset : offset + self._rpc_header_batch_size]
                requests = [
                    raw_rpc.getblockheader.get_request(hash_int_to_hex(block_hash_int), False)
                    for block_hash_int in chunk
                ]
                responses = self.rpc.batch(requests)
                for block_hash_int, response in zip(chunk, responses):
                    header_hex = response.get("result")
                    if response.get("error") is not None or header_hex is None:
                        unresolved.append(block_hash_int)
                        continue
                    header = CBlockHeader()
                    header.deserialize(BytesIO(bytes.fromhex(header_hex)))
                    self._header_by_hash[block_hash_int] = header
                    result[block_hash_int] = header
            missing = unresolved

        for block_hash_int in missing:
            result[block_hash_int] = self.get_header(block_hash_int)
        return result

    def get_block_raw(self, block_hash_int):
        cached = self._cache_get_raw(block_hash_int)
        if cached is not None:
            return cached
        block_hex = self.rpc.getblock(hash_int_to_hex(block_hash_int), 0)
        raw = binascii.unhexlify(block_hex)
        self._cache_put_raw(block_hash_int, raw)
        return raw

    def get_blocks_raw_batch(self, block_hash_ints, *, cache_only=False):
        result = {} if not cache_only else None
        missing = []
        for block_hash_int in block_hash_ints:
            cached = self._cache_get_raw(block_hash_int)
            if cached is not None:
                if not cache_only:
                    result[block_hash_int] = cached
            else:
                missing.append(block_hash_int)

        if not missing:
            return {} if cache_only else result

        raw_rpc = getattr(self.rpc, "_rpc", None)
        if raw_rpc is not None and hasattr(self.rpc, "batch"):
            unresolved = []
            batch_size = self._rpc_batch_size if self._rpc_batch_size > 0 else len(missing)
            # Avoid huge JSON-RPC responses once blocks get large by bounding the
            # expected response size. `getblock(verbosity=0)` returns hex, so
            # payload is roughly 2x raw block size.
            raw_budget_bytes = 16 * 1024 * 1024
            est_raw = self.estimated_raw_block_size()
            max_by_size = max(1, raw_budget_bytes // max(1, est_raw))
            batch_size = max(1, min(batch_size, max_by_size))
            for offset in range(0, len(missing), batch_size):
                chunk = missing[offset : offset + batch_size]
                requests = [
                    raw_rpc.getblock.get_request(hash_int_to_hex(h), 0) for h in chunk
                ]
                responses = self.rpc.batch(requests)
                for block_hash_int, response in zip(chunk, responses):
                    if response.get("error") is not None or response.get("result") is None:
                        unresolved.append(block_hash_int)
                        continue
                    raw = binascii.unhexlify(response["result"])
                    self._cache_put_raw(block_hash_int, raw)
                    if not cache_only:
                        result[block_hash_int] = raw
            missing = unresolved

        for block_hash_int in missing:
            raw = self.get_block_raw(block_hash_int)
            if not cache_only:
                result[block_hash_int] = raw

        return {} if cache_only else result


class HistoricalReorgProxy(P2PInterface):
    """Serve stale blocks first, then canonical blocks from an RPC upstream."""

    def __init__(
        self,
        *,
        chain_view,
        stale_blocks,
        max_height=None,
        stale_choice_seed=None,
        defer_stale_until_getdata=False,
        stale_served_hook=None,
        record_all_served_blocks=True,
        record_getdata_requests=True,
        active_prefetch_window=255,
        active_send_ahead=0,
        active_send_ahead_min_batch=16,
        header_follow_window=4096,
    ):
        super().__init__()
        self.chain_view = chain_view
        self.max_height = max_height
        self.defer_stale_until_getdata = defer_stale_until_getdata
        self._stale_served_hook = stale_served_hook
        self._active_prefetch_window = max(0, int(active_prefetch_window))
        self._active_send_ahead = max(0, int(active_send_ahead))
        self._active_send_ahead_min_batch = max(
            1,
            min(int(active_send_ahead_min_batch), self._active_send_ahead)
            if self._active_send_ahead > 0
            else 1,
        )
        self._header_follow_window = max(0, int(header_follow_window))
        self.stale_blocks = list(stale_blocks)
        self.stale_by_parent = group_stale_blocks_by_parent(self.stale_blocks)
        self.stale_by_hash = {stale.hash_int: stale for stale in self.stale_blocks}
        self.announced_stale_headers = set()
        self.served_stale_blocks = set()
        self.disabled_stale_blocks = set()
        self.getdata_requests = []
        self._record_getdata_requests = record_getdata_requests
        self.served_blocks = []  # (kind, height, hash_int)
        self._record_all_served_blocks = record_all_served_blocks
        self._stale_rng = random.Random(stale_choice_seed)
        self._stale_parent_order = {}
        self._stale_subtree_depth_cache = {}
        self._accept_net = None
        self._accept_services = None
        self._accept_supports_v2 = True
        self._stats = defaultdict(int)
        self._pre_getdata_window_counts = defaultdict(int)
        self._last_stats_log_time = time.monotonic()
        self._first_getheaders_at = None
        self._first_getdata_at = None
        self._last_getdata_seen_at = None
        self._highest_active_height_sent = 0
        self._highest_active_height_requested = 0
        self._highest_announced_active_height = 0
        self._highest_active_height_prefetched = 0
        self._pending_stale_hashes = set()
        self._pending_stale_count_by_height = defaultdict(int)
        pending_heights = set()
        for stale in self.stale_blocks:
            if self.max_height is not None and stale.height > self.max_height:
                continue
            self._pending_stale_hashes.add(stale.hash_int)
            self._pending_stale_count_by_height[stale.height] += 1
            pending_heights.add(stale.height)
        self._pending_stale_heights = sorted(pending_heights)

    def _log(self, message):
        print(f"[proxy] {message}", flush=True)

    def _advertised_starting_height(self):
        tip_height = self.chain_view.tip_height()
        if self.max_height is not None:
            return min(tip_height, self.max_height)
        return tip_height

    def _maybe_log_stats(self, *, force=False):
        now = time.monotonic()
        if not force and now - self._last_stats_log_time < 10.0:
            return
        self._last_stats_log_time = now
        raw_cache = self.chain_view.raw_cache_stats()
        self._log(
            "stats getheaders={} getdata={} headers_sent={} stale_headers={} "
            "stale_blocks={} active_blocks={} prefetch_cached={} "
            "active_send_ahead={} stale_inv_nudge={} stale_repeats={} fallback_active_inv={} "
            "req_height={} raw_cache_entries={} raw_cache_mib={:.1f} raw_cache_evict={}".format(
                self._stats["getheaders"],
                self._stats["getdata"],
                self._stats["headers_sent"],
                self._stats["stale_headers"],
                self._stats["stale_blocks"],
                self._stats["active_blocks"],
                self._stats["active_prefetch_cached_blocks"],
                self._stats["active_send_ahead_blocks"],
                self._stats["stale_inv_nudge"],
                self._stats["stale_repeats"],
                self._stats["fallback_active_inv"],
                self._highest_active_height_requested,
                raw_cache["entries"],
                raw_cache["bytes"] / (1024 * 1024),
                raw_cache["evictions"],
            )
        )

    def configure_inbound_reconnect(self, *, net, services=P2P_SERVICES, supports_v2_p2p=True):
        self._accept_net = net
        self._accept_services = services
        self._accept_supports_v2 = supports_v2_p2p
        if supports_v2_p2p:
            self.v2_state = EncryptedP2PState(initiating=False, net=net)
        self.peer_connect_send_version(services)
        if self.on_connection_send_msg is not None:
            self.on_connection_send_msg.nStartingHeight = self._advertised_starting_height()
        self.reconnect = True
        self._log(
            "inbound reconnect enabled net={} supports_v2={} defer_stale_until_getdata={}".format(
                net,
                supports_v2_p2p,
                self.defer_stale_until_getdata,
            )
        )

    def on_close(self):
        # Re-arm handshake state when a new inbound connection reuses this object.
        if self._accept_services is not None:
            self.peer_connect_send_version(self._accept_services)
            if self.on_connection_send_msg is not None:
                self.on_connection_send_msg.nStartingHeight = self._advertised_starting_height()
        if self._accept_supports_v2 and self._accept_net is not None:
            self.v2_state = EncryptedP2PState(initiating=False, net=self._accept_net)
        self._log("peer disconnected, re-arming inbound handshake state")

    def _find_serving_fork(self, locator_hashes, hash_stop):
        best_hash = None
        best_height = -1
        locator_height_cap = None
        if self._stats["getdata"] > 0:
            locator_height_cap = self._highest_active_height_requested + self._header_follow_window

        for locator_hash in locator_hashes:
            stale = self.stale_by_hash.get(locator_hash)
            if stale is not None and (
                stale.hash_int in self.served_stale_blocks
                or stale.hash_int in self.announced_stale_headers
            ):
                if self._next_stale_candidate(stale.hash_int, hash_stop) is not None:
                    if (
                        locator_height_cap is None
                        or stale.height <= locator_height_cap
                    ) and stale.height > best_height:
                        best_hash = stale.hash_int
                        best_height = stale.height

            height = self.chain_view.get_active_height(locator_hash)
            if (
                height is not None
                and (
                    locator_height_cap is None
                    or height <= locator_height_cap
                )
                and height > best_height
            ):
                best_hash = locator_hash
                best_height = height

        if best_hash is not None:
            return best_hash, best_height

        if locator_height_cap is not None:
            tip_height = self.chain_view.tip_height()
            if self.max_height is not None:
                tip_height = min(tip_height, self.max_height)
            if tip_height >= 0:
                capped_height = min(locator_height_cap, tip_height)
                capped_hash = hash_hex_to_int(self.chain_view.get_block_hash(capped_height))
                return capped_hash, capped_height

        cached_tip_hash, cached_tip_height = self.chain_view.cached_tip()
        if cached_tip_hash is not None:
            return cached_tip_hash, cached_tip_height
        genesis_hash = hash_hex_to_int(self.chain_view.get_block_hash(0))
        return genesis_hash, 0

    def _stale_subtree_depth(self, parent_hash):
        if parent_hash in self._stale_subtree_depth_cache:
            return self._stale_subtree_depth_cache[parent_hash]

        best = 0
        for stale in self.stale_by_parent.get(parent_hash, []):
            depth = 1 + self._stale_subtree_depth(stale.hash_int)
            if depth > best:
                best = depth
        self._stale_subtree_depth_cache[parent_hash] = best
        return best

    def _next_stale_candidate(self, parent_hash, hash_stop, *, allow_announced=False):
        # During initial headers pre-sync (before any getdata), stale
        # diversions can trigger repeated pre-sync loops. Defer stale
        # injection until block download begins.
        if self.defer_stale_until_getdata and self._stats["getdata"] == 0:
            return None

        if parent_hash not in self._stale_parent_order:
            ordered = list(self.stale_by_parent.get(parent_hash, []))
            if len(ordered) > 1:
                # Prefer siblings that allow longer stale sequences.
                self._stale_rng.shuffle(ordered)
                ordered.sort(
                    key=lambda rec: (
                        -self._stale_subtree_depth(rec.hash_int),
                        rec.height,
                        rec.hash_int,
                    )
                )
            self._stale_parent_order[parent_hash] = ordered

        for stale in self._stale_parent_order.get(parent_hash, []):
            if stale.hash_int in self.served_stale_blocks:
                continue
            if stale.hash_int in self.announced_stale_headers and not allow_announced:
                # In long-run replay mode, never re-announce the same stale
                # header; repeated diversions can deadlock header sync.
                if self.defer_stale_until_getdata:
                    continue
                # Functional test mode: allow re-announcement once getdata
                # begins so skipped stale headers can still be fetched.
                if self._stats["getdata"] == 0:
                    continue
            if stale.hash_int in self.disabled_stale_blocks:
                continue
            if self.max_height is not None and stale.height > self.max_height:
                continue
            if hash_stop and hash_stop != stale.hash_int:
                continue
            return stale
        return None

    def _next_pending_stale_height(self, min_height):
        index = bisect_left(self._pending_stale_heights, min_height)
        while index < len(self._pending_stale_heights):
            height = self._pending_stale_heights[index]
            if self._pending_stale_count_by_height.get(height, 0) > 0:
                return height
            index += 1
        return None

    def _consume_pending_stale(self, stale):
        if stale.hash_int not in self._pending_stale_hashes:
            return
        self._pending_stale_hashes.remove(stale.hash_int)
        remaining = self._pending_stale_count_by_height.get(stale.height, 0)
        if remaining > 0:
            self._pending_stale_count_by_height[stale.height] = remaining - 1

    def _serve_stale_with_followups(self, stale, *, fallback_active_hash_int=None, source="getdata"):
        self.served_stale_blocks.add(stale.hash_int)
        self._consume_pending_stale(stale)
        self.served_blocks.append(("stale", stale.height, stale.hash_int))
        self.send_without_ping(RawBlockMessage(stale.raw_block))
        self._stats["stale_blocks"] += 1
        self._log(
            "served stale block [{}] height={} hash={} parent={}".format(
                source,
                stale.height,
                hash_int_to_hex(stale.hash_int),
                hash_int_to_hex(stale.prev_hash_int),
            )
        )
        if self._stale_served_hook is not None:
            try:
                self._stale_served_hook(stale, source)
            except Exception as exc:
                self._log(
                    "stale_served_hook failed: height={} hash={} err={!r}".format(
                        stale.height, hash_int_to_hex(stale.hash_int), exc
                    )
                )

        invs = []
        next_stale = self._next_stale_candidate(stale.hash_int, 0)
        if next_stale is not None:
            invs.append(CInv(MSG_BLOCK, next_stale.hash_int))
            self._log(
                "stale continuation candidate: height={} hash={}".format(
                    next_stale.height, hash_int_to_hex(next_stale.hash_int)
                )
            )

        if fallback_active_hash_int is None:
            fallback_active_hash_int = hash_hex_to_int(
                self.chain_view.get_block_hash(stale.height)
            )
        if fallback_active_hash_int != stale.hash_int:
            invs.append(CInv(MSG_BLOCK, fallback_active_hash_int))
            self._stats["fallback_active_inv"] += 1
            self._log(
                "stale fallback: announcing active peer block "
                "height={} hash={}".format(
                    stale.height, hash_int_to_hex(fallback_active_hash_int)
                )
            )

        if next_stale is None:
            next_height = stale.height + 1
            tip_height = self.chain_view.tip_height()
            if self.max_height is not None:
                tip_height = min(tip_height, self.max_height)
            if next_height <= tip_height:
                next_hash_int = hash_hex_to_int(self.chain_view.get_block_hash(next_height))
                if next_hash_int != fallback_active_hash_int:
                    invs.append(CInv(MSG_BLOCK, next_hash_int))

        if invs:
            self.send_without_ping(msg_inv(invs))

    def _maybe_proactive_stale_announce(self, max_requested_height):
        """Inject stale headers during block download even if getheaders is quiet.

        When the target already has headers synced, it may stop asking getheaders
        entirely (getheaders ~= 1). In that mode, stale injection via getheaders
        never triggers. This proactively announces one stale header at a time as
        the peer approaches the corresponding height.
        """
        if self._stats["getdata"] == 0 or max_requested_height is None:
            return

        next_stale_height = self._next_pending_stale_height(max_requested_height)
        if next_stale_height is None:
            return
        if next_stale_height > max_requested_height + 1:
            return

        if next_stale_height <= 0:
            return
        parent_hash_int = hash_hex_to_int(
            self.chain_view.get_block_hash(next_stale_height - 1)
        )
        stale = self._next_stale_candidate(parent_hash_int, 0)
        if stale is None:
            return
        if stale.hash_int in self.announced_stale_headers:
            return

        self.announced_stale_headers.add(stale.hash_int)
        self.send_without_ping(msg_headers([CBlockHeader(stale.header)]))
        self._stats["headers_sent"] += 1
        self._stats["stale_headers"] += 1
        self.send_without_ping(msg_inv([CInv(MSG_BLOCK, stale.hash_int)]))
        self._stats["stale_inv_nudge"] += 1
        self._log(
            "proactive stale announce near download frontier: "
            "requested_height={} stale_height={} stale_hash={} parent={}".format(
                max_requested_height,
                stale.height,
                hash_int_to_hex(stale.hash_int),
                hash_int_to_hex(stale.prev_hash_int),
            )
        )

    def _maybe_follow_active_headers(self, max_requested_height):
        """Advance the peer's active header frontier during getdata-only sync.

        After the last stale range, some peers stop issuing fresh getheaders
        requests while they are still downloading blocks. If we do not extend
        the announced active headers here, block download can stall at the last
        announced height.
        """
        if max_requested_height is None:
            return

        tip_height = self.chain_view.tip_height()
        if self.max_height is not None:
            tip_height = min(tip_height, self.max_height)
        if self._highest_announced_active_height >= tip_height:
            return

        follow_threshold = min(128, MAX_HEADERS_RESULTS // 4)
        if max_requested_height + follow_threshold < self._highest_announced_active_height:
            return

        start_height = max(self._highest_announced_active_height + 1, max_requested_height + 1)
        next_stale_height = self._next_pending_stale_height(start_height)
        end_height = min(tip_height, start_height + MAX_HEADERS_RESULTS - 1)
        if next_stale_height is not None:
            end_height = min(end_height, next_stale_height - 1)
        if end_height < start_height:
            return

        active_hash_hexes = self.chain_view.get_block_hashes_range(start_height, end_height)
        active_hashes = [hash_hex_to_int(block_hash_hex) for block_hash_hex in active_hash_hexes]
        if not active_hashes:
            return

        header_map = self.chain_view.get_headers_batch(active_hashes)
        headers = [header_map[block_hash_int] for block_hash_int in active_hashes]
        self.send_without_ping(msg_headers(headers))
        self._stats["headers_sent"] += len(headers)
        self._highest_announced_active_height = max(
            self._highest_announced_active_height,
            end_height,
        )
        self._log(
            "active header follow count={} range={}..{} next_stale_height={}".format(
                len(headers),
                start_height,
                end_height,
                next_stale_height if next_stale_height is not None else "none",
            )
        )

    def on_getheaders(self, message):
        try:
            self._stats["getheaders"] += 1
            if self._first_getheaders_at is None:
                self._first_getheaders_at = time.monotonic()
            locator_hashes = message.locator.vHave
            hash_stop = message.hashstop
            fork_hash, fork_height = self._find_serving_fork(locator_hashes, hash_stop)

            tip_height = self.chain_view.tip_height()
            if self.max_height is not None:
                tip_height = min(tip_height, self.max_height)

            next_height = fork_height + 1
            if next_height > tip_height:
                if self._stats["getdata"] == 0:
                    self._log(
                        "pre-getdata empty headers fork_height={} tip_height={} "
                        "locators={} hash_stop={}".format(
                            fork_height,
                            tip_height,
                            len(locator_hashes),
                            hash_int_to_hex(hash_stop) if hash_stop else "0",
                        )
                    )
                    # Some peers can keep requesting headers after reaching the
                    # tip without transitioning to block download. Nudge with
                    # the current tip inv to trigger getdata.
                    if tip_height > 0 and self._stats["getheaders"] % 25 == 0:
                        tip_hash_int = hash_hex_to_int(self.chain_view.get_block_hash(tip_height))
                        self.send_without_ping(msg_inv([CInv(MSG_BLOCK, tip_hash_int)]))
                        self._stats["fallback_active_inv"] += 1
                        self._log(
                            "pre-getdata tip nudge: announcing tip height={} hash={}".format(
                                tip_height, hash_int_to_hex(tip_hash_int)
                            )
                        )
                self.send_without_ping(msg_headers([]))
                return

            if self._stats["getdata"] == 0:
                window_key = (fork_height, next_height, tip_height)
                self._pre_getdata_window_counts[window_key] += 1
                repeat = self._pre_getdata_window_counts[window_key]
                if repeat in (1, 5, 10) or repeat % 25 == 0:
                    first_locator = locator_hashes[0] if locator_hashes else 0
                    first_locator_height = (
                        self.chain_view.get_active_height(first_locator)
                        if first_locator
                        else None
                    )
                    self._log(
                        "pre-getdata headers window fork_height={} next_height={} "
                        "tip_height={} repeat={} locators={} first_locator={} "
                        "first_locator_height={} hash_stop={}".format(
                            fork_height,
                            next_height,
                            tip_height,
                            repeat,
                            len(locator_hashes),
                            hash_int_to_hex(first_locator) if first_locator else "0",
                            first_locator_height,
                            hash_int_to_hex(hash_stop) if hash_stop else "0",
                        )
                    )
                    if (
                        self._first_getheaders_at is not None
                        and self._stats["getheaders"] >= 200
                        and self._stats["getheaders"] % 50 == 0
                    ):
                        age = time.monotonic() - self._first_getheaders_at
                        self._log(
                            "pre-getdata stall-watch getheaders={} headers_sent={} age={:.1f}s"
                            .format(
                                self._stats["getheaders"],
                                self._stats["headers_sent"],
                                age,
                            )
                        )

            headers = []
            height = next_height
            parent_hash = fork_hash
            stale_header = None
            stale_was_repeat = False
            announced_active_height = fork_height
            allow_stale_diversion = not (
                self.defer_stale_until_getdata and self._stats["getdata"] == 0
            )
            while height <= tip_height and len(headers) < MAX_HEADERS_RESULTS:
                stale = self._next_stale_candidate(parent_hash, hash_stop)
                if stale is not None:
                    stale_was_repeat = stale.hash_int in self.announced_stale_headers
                    headers.append(CBlockHeader(stale.header))
                    self.announced_stale_headers.add(stale.hash_int)
                    stale_header = stale
                    break
                remaining = MAX_HEADERS_RESULTS - len(headers)
                active_end = min(tip_height, height + remaining - 1)
                if allow_stale_diversion and not hash_stop:
                    next_stale_height = self._next_pending_stale_height(height)
                    if next_stale_height is not None:
                        active_end = min(active_end, next_stale_height - 1)
                if active_end < height:
                    break

                active_hash_hexes = self.chain_view.get_block_hashes_range(height, active_end)
                active_hashes = [hash_hex_to_int(block_hash_hex) for block_hash_hex in active_hash_hexes]
                if hash_stop:
                    try:
                        stop_index = active_hashes.index(hash_stop)
                    except ValueError:
                        stop_index = None
                    if stop_index is not None:
                        active_hashes = active_hashes[: stop_index + 1]

                if not active_hashes:
                    break

                header_map = self.chain_view.get_headers_batch(active_hashes)
                for offset, block_hash_int in enumerate(active_hashes):
                    headers.append(header_map[block_hash_int])
                    parent_hash = block_hash_int
                    announced_active_height = height + offset

                if hash_stop and active_hashes[-1] == hash_stop:
                    break
                height = announced_active_height + 1

            self.send_without_ping(msg_headers(headers))
            self._stats["headers_sent"] += len(headers)
            if announced_active_height > self._highest_announced_active_height:
                self._highest_announced_active_height = announced_active_height
            if stale_header is not None:
                self._stats["stale_headers"] += 1
                self._log(
                    "header diversion fork_height={} fork_hash={} -> stale_height={} stale_hash={} "
                    "repeat={} locators={} hash_stop={}".format(
                        fork_height,
                        hash_int_to_hex(fork_hash),
                        stale_header.height,
                        hash_int_to_hex(stale_header.hash_int),
                        stale_was_repeat,
                        len(locator_hashes),
                        hash_int_to_hex(hash_stop) if hash_stop else "0",
                    )
                )
                # Explicitly announce the stale block so peers request it
                # immediately instead of silently staying on the active branch.
                self.send_without_ping(msg_inv([CInv(MSG_BLOCK, stale_header.hash_int)]))
                self._stats["stale_inv_nudge"] += 1
                self._log(
                    "header diversion nudge: announcing stale block "
                    "height={} hash={}".format(
                        stale_header.height,
                        hash_int_to_hex(stale_header.hash_int),
                    )
                )
                if len(headers) < MAX_HEADERS_RESULTS:
                    tip_height = self.chain_view.tip_height()
                    if self.max_height is not None:
                        tip_height = min(tip_height, self.max_height)
                    next_height = stale_header.height + 1
                    if next_height <= tip_height:
                        next_active_hash = hash_hex_to_int(self.chain_view.get_block_hash(next_height))
                        self.send_without_ping(msg_inv([CInv(MSG_BLOCK, next_active_hash)]))
                        self._stats["fallback_active_inv"] += 1
                        self._log(
                            "header diversion nudge: announcing next active block "
                            "height={} hash={}".format(
                                next_height, hash_int_to_hex(next_active_hash)
                            )
                        )
            self._maybe_log_stats()
        except Exception as exc:
            print(f"on_getheaders failure: {exc!r}", flush=True)
            print(traceback.format_exc(), flush=True)

    def on_getdata(self, message):
        try:
            self._stats["getdata"] += 1
            now = time.monotonic()
            if self._first_getdata_at is None:
                self._first_getdata_at = now
                header_age = 0.0
                if self._first_getheaders_at is not None:
                    header_age = now - self._first_getheaders_at
                self._log(
                    "first getdata received after {:.1f}s (getheaders={} headers_sent={})".format(
                        header_age,
                        self._stats["getheaders"],
                        self._stats["headers_sent"],
                    )
                )
            self._last_getdata_seen_at = now
            active_requests = []
            missing_active = []
            active_candidates = []
            for inv in message.inv:
                if self._record_getdata_requests:
                    self.getdata_requests.append(inv.hash)
                if (inv.type & MSG_TYPE_MASK) != MSG_BLOCK:
                    continue

                stale = self.stale_by_hash.get(inv.hash)
                if stale is not None:
                    if stale.hash_int in self.served_stale_blocks:
                        # If peer asks again for stale block, steer back to active.
                        self.disabled_stale_blocks.add(stale.hash_int)
                        self._consume_pending_stale(stale)
                        self._stats["stale_repeats"] += 1
                        self.send_without_ping(msg_notfound([CInv(inv.type, stale.hash_int)]))
                        active_hash_int = hash_hex_to_int(self.chain_view.get_block_hash(stale.height))
                        if active_hash_int != stale.hash_int:
                            self.send_without_ping(msg_inv([CInv(MSG_BLOCK, active_hash_int)]))
                            self._stats["fallback_active_inv"] += 1
                        self._log(
                            "stale re-requested (likely failed validation), "
                            "sent notfound + active fallback: height={} stale={} active={}".format(
                                stale.height,
                                hash_int_to_hex(stale.hash_int),
                                hash_int_to_hex(active_hash_int),
                            )
                        )
                        continue

                    self._serve_stale_with_followups(stale, source="getdata")
                    continue

                active_candidates.append(inv)

            active_height_map = self.chain_view.get_active_heights_batch(
                [inv.hash for inv in active_candidates]
            )
            for inv in active_candidates:
                height = active_height_map.get(inv.hash)
                if height is None:
                    missing_active.append(inv)
                    continue

                active_requests.append((height, inv.hash))
                if height > self._highest_active_height_requested:
                    self._highest_active_height_requested = height

            if active_requests:
                # Keep getdata responses in ascending height order. This helps
                # stale-first replay behave deterministically, and ensures we
                # don't stream children blocks before parents.
                active_requests.sort(
                    key=lambda item: (item[0] is None, item[0] if item[0] is not None else 0)
                )
                active_hashes = [block_hash_int for _, block_hash_int in active_requests]
                block_map = self.chain_view.get_blocks_raw_batch(active_hashes)
                for height, block_hash_int in active_requests:
                    # Some nodes won't request stale blocks during IBD even if
                    # we announce them. To reliably replay reorgs, inject stale
                    # blocks at their heights right before serving the active
                    # block for that height.
                    if height is not None and height > 0:
                        # Some stale blocks build on other stale blocks (a rare
                        # two-block stale branch). Prefer stales that extend the
                        # active parent first, then fall back to any pending
                        # stale at this height whose parent stale was already
                        # served.
                        while self._pending_stale_count_by_height.get(height, 0) > 0:
                            parent_hash_int = hash_hex_to_int(
                                self.chain_view.get_block_hash(height - 1)
                            )
                            stale = self._next_stale_candidate(
                                parent_hash_int,
                                0,
                                allow_announced=True,
                            )
                            if stale is None or stale.height != height:
                                # Fall back to stale-on-stale children.
                                candidates = []
                                for rec in self.stale_blocks:
                                    if rec.height != height:
                                        continue
                                    if rec.hash_int in self.served_stale_blocks:
                                        continue
                                    if rec.hash_int in self.disabled_stale_blocks:
                                        continue
                                    if rec.hash_int not in self._pending_stale_hashes:
                                        continue
                                    if rec.prev_hash_int in self.served_stale_blocks:
                                        candidates.append(rec)
                                candidates.sort(key=lambda rec: rec.hash_int)
                                stale = candidates[0] if candidates else None
                            if stale is None:
                                break
                            if stale.hash_int not in self.announced_stale_headers:
                                self.announced_stale_headers.add(stale.hash_int)
                                # Announce first so the peer doesn't treat the
                                # incoming block as unexpected.
                                self.send_without_ping(msg_headers([CBlockHeader(stale.header)]))
                                self._stats["headers_sent"] += 1
                                self._stats["stale_headers"] += 1
                                self.send_without_ping(msg_inv([CInv(MSG_BLOCK, stale.hash_int)]))
                                self._stats["stale_inv_nudge"] += 1
                                self._log(
                                    "injected stale announce: height={} hash={} parent={}".format(
                                        stale.height,
                                        hash_int_to_hex(stale.hash_int),
                                        hash_int_to_hex(stale.prev_hash_int),
                                    )
                                )
                            self._serve_stale_with_followups(
                                stale,
                                fallback_active_hash_int=block_hash_int,
                                source="active-getdata",
                            )

                    if self._record_all_served_blocks:
                        self.served_blocks.append(("active", height, block_hash_int))
                    self.send_without_ping(RawBlockMessage(block_map[block_hash_int]))
                    self._stats["active_blocks"] += 1
                    if height is not None and height > self._highest_active_height_sent:
                        self._highest_active_height_sent = height
                    if self._stats["active_blocks"] % 5000 == 0:
                        self._log(
                            "served active block count={} latest_height={} hash={}".format(
                                self._stats["active_blocks"],
                                height,
                                hash_int_to_hex(block_hash_int),
                            )
                        )

                # Optionally stream a bounded number of active blocks ahead to
                # reduce RTT-limited getdata loops. Never cross the next stale
                # height so stale opportunities remain intact.
                max_requested_height = max((h for h, _ in active_requests if h is not None), default=None)
                self._maybe_proactive_stale_announce(max_requested_height)
                self._maybe_follow_active_headers(max_requested_height)
                est_raw = self.chain_view.estimated_raw_block_size()
                raw_cache_max_bytes = self.chain_view.raw_cache_stats()["max_bytes"]
                if self._active_send_ahead > 0 and max_requested_height is not None:
                    tip_height = self.chain_view.tip_height()
                    if self.max_height is not None:
                        tip_height = min(tip_height, self.max_height)
                    # Keep proactive streaming bounded to a fixed window ahead
                    # of what the peer actually requested. Do not grow the
                    # ahead distance cumulatively across getdata rounds.
                    send_start = max(
                        self._highest_active_height_sent + 1,
                        max_requested_height + 1,
                    )
                    send_end = min(
                        max_requested_height + self._active_send_ahead,
                        tip_height,
                        self._highest_announced_active_height,
                    )
                    next_stale_height = self._next_pending_stale_height(send_start)
                    if next_stale_height is not None:
                        send_end = min(send_end, next_stale_height - 1)
                    # Send ahead in small chunks so we don't stall the P2P thread
                    # on very large blocks (common after ~200k on mainnet).
                    send_budget_bytes = min(
                        64 * 1024 * 1024,
                        max(8 * 1024 * 1024, raw_cache_max_bytes // 64),
                    )
                    send_limit_blocks = max(1, send_budget_bytes // max(1, est_raw))
                    send_limit_blocks = min(send_limit_blocks, self._active_send_ahead)
                    if send_limit_blocks > 0:
                        send_end = min(send_end, send_start + send_limit_blocks - 1)
                    send_count = send_end - send_start + 1
                    min_batch = min(self._active_send_ahead_min_batch, max(1, send_limit_blocks))
                    if send_count >= min_batch:
                        send_hash_hexes = self.chain_view.get_block_hashes_range(send_start, send_end)
                        send_hashes = [hash_hex_to_int(block_hash_hex) for block_hash_hex in send_hash_hexes]
                        send_block_map = self.chain_view.get_blocks_raw_batch(send_hashes)
                        for send_height, send_hash_int in zip(
                            range(send_start, send_end + 1),
                            send_hashes,
                        ):
                            if self._record_all_served_blocks:
                                self.served_blocks.append(("active", send_height, send_hash_int))
                            self.send_without_ping(RawBlockMessage(send_block_map[send_hash_int]))
                            self._stats["active_blocks"] += 1
                            self._stats["active_send_ahead_blocks"] += 1
                            if send_height > self._highest_active_height_sent:
                                self._highest_active_height_sent = send_height
                        if send_count >= 32:
                            self._log(
                                "active send-ahead count={} range={}..{} next_stale_height={}".format(
                                    send_count,
                                    send_start,
                                    send_end,
                                    next_stale_height if next_stale_height is not None else "none",
                                )
                            )

                # Push a bounded amount of active blocks ahead to avoid RTT-limited
                # getdata loops by prefetching raw blocks from upstream RPC. Stop
                # before the next stale height so reorg opportunities are preserved.
                if max_requested_height is not None:
                    tip_height = self.chain_view.tip_height()
                    if self.max_height is not None:
                        tip_height = min(tip_height, self.max_height)
                    cap_blocks = self.chain_view.estimated_raw_cache_capacity_blocks()
                    effective_prefetch_window = (
                        min(self._active_prefetch_window, cap_blocks) if cap_blocks > 0 else 0
                    )
                    if effective_prefetch_window > 0:
                        prefetch_start = max(
                            self._highest_active_height_prefetched + 1,
                            max_requested_height + 1,
                        )
                        prefetch_end = min(
                            max_requested_height + effective_prefetch_window,
                            tip_height,
                            self._highest_announced_active_height,
                        )
                        next_stale_height = self._next_pending_stale_height(prefetch_start)
                        if next_stale_height is not None:
                            prefetch_end = min(prefetch_end, next_stale_height - 1)
                        # Bound prefetch so we don't block `on_getdata` for seconds.
                        prefetch_budget_bytes = min(
                            32 * 1024 * 1024,
                            max(4 * 1024 * 1024, raw_cache_max_bytes // 128),
                        )
                        prefetch_limit_blocks = max(1, prefetch_budget_bytes // max(1, est_raw))
                        prefetch_limit_blocks = min(prefetch_limit_blocks, 512, effective_prefetch_window)
                        if prefetch_limit_blocks > 0:
                            prefetch_end = min(prefetch_end, prefetch_start + prefetch_limit_blocks - 1)
                        if prefetch_end >= prefetch_start:
                            prefetch_hash_hexes = self.chain_view.get_block_hashes_range(
                                prefetch_start,
                                prefetch_end,
                            )
                            prefetch_hashes = [
                                hash_hex_to_int(block_hash_hex) for block_hash_hex in prefetch_hash_hexes
                            ]
                            self.chain_view.get_blocks_raw_batch(prefetch_hashes, cache_only=True)
                            self._stats["active_prefetch_cached_blocks"] += len(prefetch_hashes)
                            if prefetch_end > self._highest_active_height_prefetched:
                                self._highest_active_height_prefetched = prefetch_end
                            if len(prefetch_hashes) >= 512:
                                self._log(
                                    "active prefetch cached count={} range={}..{} next_stale_height={}".format(
                                        len(prefetch_hashes),
                                        prefetch_start,
                                        prefetch_end,
                                        next_stale_height if next_stale_height is not None else "none",
                                    )
                                )

            if missing_active:
                self.send_without_ping(msg_notfound(missing_active))
                self._log(
                    "notfound for {} unknown/unsupported block requests; first_hash={}".format(
                        len(missing_active),
                        hash_int_to_hex(missing_active[0].hash),
                    )
                )
            self._maybe_log_stats()
        except Exception as exc:
            print(f"on_getdata failure: {exc!r}", flush=True)
            print(traceback.format_exc(), flush=True)
