#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Run a stale-first block proxy for historical reorg replay."""

from argparse import ArgumentParser
from collections import defaultdict
import http.client
import json
from pathlib import Path
import queue
import re
import sys
import threading
import time
from urllib.parse import (
    quote,
    urlsplit,
    urlunsplit,
)


REPO_ROOT = Path(__file__).resolve().parents[1]
FUNCTIONAL_ROOT = REPO_ROOT / "test" / "functional"
if str(FUNCTIONAL_ROOT) not in sys.path:
    sys.path.insert(0, str(FUNCTIONAL_ROOT))

from test_framework.authproxy import (  # noqa: E402
    AuthServiceProxy,
    JSONRPCException,
)
from test_framework.p2p import (  # noqa: E402
    NetworkThread,
    P2P_SERVICES,
)
from test_framework.stale_reorg_proxy import (  # noqa: E402
    HistoricalReorgProxy,
    RPCChainView,
    hash_int_to_hex,
    load_stale_blocks_from_dir,
)

UPDATE_TIP_RE = re.compile(r"UpdateTip: new best=([0-9a-f]{64}) height=(\d+)")
TARGET_VALIDATION_MARKERS = (
    "block is marked invalid",
    "ConnectBlock",
    "ActivateBestChain failed",
    "InvalidChainFound",
    "DisconnectTip",
    "REORGANIZE",
    "Chain reorganization",
    "bad-blk",
    "bad-txns",
)

LISTEN_HOST = "127.0.0.1"
NETWORK = "mainnet"
RAW_CACHE_MIB = 1024
RPC_BLOCK_BATCH_SIZE = 256
RPC_HEADER_BATCH_SIZE = 1024
RPC_HASH_BATCH_SIZE = 4096
TIP_HEIGHT_CACHE_SECONDS = 2.0
ACTIVE_PREFETCH_WINDOW = 1024
ACTIVE_SEND_AHEAD = 256
DEFER_STALE_UNTIL_GETDATA = True
PROGRESS_LOG_SECONDS = 5.0
POLL_SECONDS = 1.0


class ResilientRPCClient:
    """Reconnect-and-retry wrapper around AuthServiceProxy."""

    def __init__(self, rpc_url, *, timeout, retries=12, retry_delay=0.2):
        self._rpc_url = rpc_url
        self._timeout = timeout
        self._retries = retries
        self._retry_delay = retry_delay
        self._rpc = AuthServiceProxy(self._rpc_url, timeout=self._timeout)

    def _reconnect(self):
        self._rpc = AuthServiceProxy(self._rpc_url, timeout=self._timeout)

    @staticmethod
    def _is_retryable_error(exc):
        return isinstance(
            exc,
            (
                BrokenPipeError,
                ConnectionError,
                OSError,
                TimeoutError,
                http.client.HTTPException,
            ),
        )

    def _call(self, method_name, *args, **kwargs):
        delay = self._retry_delay
        attempt = 0
        while True:
            try:
                return getattr(self._rpc, method_name)(*args, **kwargs)
            except JSONRPCException as exc:
                # During startup, bitcoind may still be warming up and return -28.
                # Treat this as transient and wait until the node is ready.
                if isinstance(exc.error, dict) and exc.error.get("code") == -28:
                    time.sleep(delay)
                    delay = min(delay * 2, 2.0)
                    continue
                raise
            except Exception as exc:
                if not self._is_retryable_error(exc):
                    raise
                attempt += 1
                if attempt >= self._retries:
                    raise
                print(
                    f"Transient RPC error on {method_name} "
                    f"(attempt {attempt}/{self._retries}): {exc!r}; reconnecting..."
                )
                self._reconnect()
                time.sleep(delay)
                delay = min(delay * 2, 2.0)
        raise RuntimeError("unreachable")

    def batch(self, rpc_call_list):
        delay = self._retry_delay
        attempt = 0
        while True:
            try:
                return self._rpc.batch(rpc_call_list)
            except JSONRPCException as exc:
                if isinstance(exc.error, dict) and exc.error.get("code") == -28:
                    time.sleep(delay)
                    delay = min(delay * 2, 2.0)
                    continue
                raise
            except Exception as exc:
                if not self._is_retryable_error(exc):
                    raise
                attempt += 1
                if attempt >= self._retries:
                    raise
                print(
                    f"Transient RPC error on batch "
                    f"(attempt {attempt}/{self._retries}): {exc!r}; reconnecting..."
                )
                self._reconnect()
                time.sleep(delay)
                delay = min(delay * 2, 2.0)
        raise RuntimeError("unreachable")

    def __getattr__(self, name):
        if name.startswith("__") and name.endswith("__"):
            raise AttributeError

        def caller(*args, **kwargs):
            return self._call(name, *args, **kwargs)

        return caller


def parse_args():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        "--upstream-rpc",
        required=True,
        help="Upstream node RPC URL, e.g. http://127.0.0.1:8332",
    )
    parser.add_argument(
        "--upstream-cookie-file",
        default=None,
        help="Optional upstream RPC .cookie file used for authentication",
    )
    parser.add_argument(
        "--target-rpc",
        default=None,
        help="Optional target node RPC URL used for UTXO snapshot output",
    )
    parser.add_argument(
        "--target-cookie-file",
        default=None,
        help="Optional target RPC .cookie file used for authentication",
    )
    parser.add_argument(
        "--stale-blocks-dir",
        default=str(REPO_ROOT / "stale-blocks" / "blocks"),
        help="Directory containing stale block blobs (<height>-<hash>.bin)",
    )
    parser.add_argument(
        "--listen-port",
        type=int,
        default=8338,
        help="Port to bind the proxy listener",
    )
    parser.add_argument(
        "--target-debug-log",
        default=None,
        help="Path to target node debug.log for reorg validation summary",
    )
    return parser.parse_args()


def collect_snapshot(target_rpc):
    snapshot = target_rpc.gettxoutsetinfo("hash_serialized_3", use_index=False)
    return {
        "height": snapshot["height"],
        "bestblock": snapshot["bestblock"],
        "hash_serialized_3": snapshot.get("hash_serialized_3"),
        "txouts": snapshot["txouts"],
        "total_amount": str(snapshot["total_amount"]),
    }

def collect_utxo_hash(rpc):
    info = rpc.gettxoutsetinfo("hash_serialized_3", use_index=False)
    return {
        "height": info["height"],
        "bestblock": info["bestblock"],
        "hash_serialized_3": info.get("hash_serialized_3"),
    }


def print_utxo_hash_comparison(upstream, target):
    match = (
        upstream.get("hash_serialized_3") is not None
        and upstream.get("bestblock") == target.get("bestblock")
        and upstream.get("hash_serialized_3") == target.get("hash_serialized_3")
    )
    print("UTXO set hash comparison (hash_serialized_3)")
    print(
        "  upstream: height={} bestblock={} hash_serialized_3={}".format(
            upstream.get("height"),
            upstream.get("bestblock"),
            upstream.get("hash_serialized_3"),
        )
    )
    print(
        "  target:   height={} bestblock={} hash_serialized_3={}".format(
            target.get("height"),
            target.get("bestblock"),
            target.get("hash_serialized_3"),
        )
    )
    print("  result: {}".format("MATCH" if match else "MISMATCH"))
    return match


def try_collect_snapshot(target_rpc):
    if target_rpc is None:
        return None
    try:
        return collect_snapshot(target_rpc)
    except Exception as exc:
        print(f"Skipping final snapshot collection: {exc!r}")
        return None


def try_getblockcount(target_rpc):
    if target_rpc is None:
        return None
    try:
        return target_rpc.getblockcount()
    except JSONRPCException as exc:
        # Target may still be starting up; treat as transient.
        if isinstance(exc.error, dict) and exc.error.get("code") == -28:
            return None
        raise
    except Exception:
        return None


def parse_update_tip_events(debug_log_path, start_offset=0):
    path = Path(debug_log_path)
    if not path.exists():
        return []

    events = []
    with path.open("r", encoding="utf-8", errors="replace") as log_file:
        if start_offset > 0:
            file_size = path.stat().st_size
            if file_size >= start_offset:
                log_file.seek(start_offset)

        for lineno, line in enumerate(log_file, start=1):
            match = UPDATE_TIP_RE.search(line)
            if match is None:
                continue
            events.append(
                {
                    "lineno": lineno,
                    "hash": match.group(1),
                    "height": int(match.group(2)),
                }
            )
    return events


def scan_target_debug_events(debug_log_path, start_offset=0, last_tip=None):
    path = Path(debug_log_path)
    if not path.exists():
        return start_offset, last_tip, []

    file_size = path.stat().st_size
    offset = start_offset if 0 <= start_offset <= file_size else 0
    events = []
    tip = last_tip

    with path.open("r", encoding="utf-8", errors="replace") as log_file:
        log_file.seek(offset)
        for line in log_file:
            stripped = line.rstrip("\n")
            match = UPDATE_TIP_RE.search(stripped)
            if match is not None:
                new_hash = match.group(1)
                new_height = int(match.group(2))
                if tip is not None:
                    prev_height, prev_hash = tip
                    if new_hash != prev_hash and new_height <= prev_height:
                        events.append(
                            {
                                "kind": "reorg",
                                "line": stripped,
                                "prev_height": prev_height,
                                "prev_hash": prev_hash,
                                "new_height": new_height,
                                "new_hash": new_hash,
                            }
                        )
                tip = (new_height, new_hash)
                continue

            if any(marker in stripped for marker in TARGET_VALIDATION_MARKERS):
                events.append({"kind": "validation", "line": stripped})

        new_offset = log_file.tell()

    return new_offset, tip, events


def validate_reorgs_from_events(served_stale, update_tip_events):
    """Validate that each served stale tip later gets replaced."""
    positions_by_hash = defaultdict(list)
    for idx, event in enumerate(update_tip_events):
        positions_by_hash[event["hash"]].append(idx)

    per_block = []
    for stale in sorted(served_stale, key=lambda item: (item["height"], item["hash"])):
        stale_hash = stale["hash"]
        stale_height = stale["height"]
        positions = positions_by_hash.get(stale_hash, [])

        connected_as_tip = bool(positions)
        reorged_out = False
        saw_disconnect_phase = False
        replaced_by = None

        if connected_as_tip:
            first_pos = positions[0]
            for event in update_tip_events[first_pos + 1 :]:
                if event["height"] < stale_height:
                    saw_disconnect_phase = True
                if event["hash"] != stale_hash and event["height"] >= stale_height:
                    reorged_out = True
                    replaced_by = {
                        "hash": event["hash"],
                        "height": event["height"],
                        "lineno": event["lineno"],
                    }
                    break

        per_block.append(
            {
                "hash": stale_hash,
                "height": stale_height,
                "connected_as_tip": connected_as_tip,
                "reorged_out": reorged_out,
                "saw_disconnect_phase": saw_disconnect_phase,
                "replaced_by": replaced_by,
            }
        )

    connected = [block for block in per_block if block["connected_as_tip"]]
    reorged = [block for block in per_block if block["reorged_out"]]
    missing_tip = [block for block in per_block if not block["connected_as_tip"]]
    missing_reorg = [
        block
        for block in per_block
        if block["connected_as_tip"] and not block["reorged_out"]
    ]
    saw_disconnect = [block for block in per_block if block["saw_disconnect_phase"]]

    return {
        "served_stale": len(served_stale),
        "update_tip_events": len(update_tip_events),
        "connected_as_tip": len(connected),
        "reorged_out": len(reorged),
        "with_disconnect_phase": len(saw_disconnect),
        "missing_tip": missing_tip,
        "missing_reorg": missing_reorg,
        "details": per_block,
    }


def print_reorg_summary(summary):
    print("Reorg validation summary")
    print(f"  Served stale blocks:       {summary['served_stale']}")
    print(f"  UpdateTip log events:      {summary['update_tip_events']}")
    print(f"  Connected as tip:          {summary['connected_as_tip']}")
    print(f"  Reorged out:               {summary['reorged_out']}")
    print(f"  With disconnect evidence:  {summary['with_disconnect_phase']}")

    if summary["missing_tip"]:
        print("  Missing tip activations:")
        for block in summary["missing_tip"]:
            print(f"    {block['height']}-{block['hash']}")

    if summary["missing_reorg"]:
        print("  Missing reorg evidence:")
        for block in summary["missing_reorg"]:
            print(f"    {block['height']}-{block['hash']}")


def rpc_url_with_cookie(rpc_url, cookie_file):
    normalized_url = rpc_url if "://" in rpc_url else f"http://{rpc_url}"
    if cookie_file is None:
        return normalized_url

    cookie_text = Path(cookie_file).read_text(encoding="utf-8").strip()
    if ":" not in cookie_text:
        raise ValueError(f"Malformed cookie file (missing user:pass): {cookie_file}")

    cookie_user, cookie_pass = cookie_text.split(":", 1)
    split = urlsplit(normalized_url)

    # If credentials already exist in URL, keep them.
    if split.username is not None:
        return normalized_url

    netloc = f"{quote(cookie_user, safe='')}:{quote(cookie_pass, safe='')}@{split.netloc}"
    return urlunsplit((split.scheme, netloc, split.path, split.query, split.fragment))


def main():
    args = parse_args()
    debug_log_offset = 0
    live_debug_offset = 0
    last_tip = None

    upstream_rpc_url = None
    while upstream_rpc_url is None:
        try:
            upstream_rpc_url = rpc_url_with_cookie(args.upstream_rpc, args.upstream_cookie_file)
        except FileNotFoundError:
            time.sleep(0.2)
    upstream_rpc = ResilientRPCClient(upstream_rpc_url, timeout=300)

    target_rpc = None
    stale_force_queue = None
    stale_force_thread = None
    forced_stale_hashes = set()
    compare_height = None

    def ensure_target_rpc():
        nonlocal target_rpc
        if not args.target_rpc:
            return None
        if target_rpc is not None:
            return target_rpc
        try:
            target_rpc_url = rpc_url_with_cookie(args.target_rpc, args.target_cookie_file)
        except FileNotFoundError:
            return None
        target_rpc = ResilientRPCClient(target_rpc_url, timeout=300)
        return target_rpc

    stale_dir = Path(args.stale_blocks_dir)
    stale_blocks = []
    if stale_dir.exists():
        stale_blocks = load_stale_blocks_from_dir(stale_dir)
    else:
        print(f"Warning: stale blocks directory does not exist: {stale_dir}")

    print(f"Loaded {len(stale_blocks)} stale blocks from {stale_dir}")

    if args.target_rpc:
        compare_height = upstream_rpc.getblockcount()
        print(f"Tracking final sync target height={compare_height}")

    if args.target_debug_log:
        debug_log_path = Path(args.target_debug_log)
        if debug_log_path.exists():
            debug_log_offset = debug_log_path.stat().st_size
            live_debug_offset = debug_log_offset
        print(
            f"Tracking debug log for reorg validation: {debug_log_path} "
            f"(start byte={debug_log_offset})"
        )

    chain_view = RPCChainView(
        upstream_rpc,
        block_raw_cache_max_bytes=RAW_CACHE_MIB * 1024 * 1024,
        rpc_batch_size=RPC_BLOCK_BATCH_SIZE,
        rpc_header_batch_size=RPC_HEADER_BATCH_SIZE,
        rpc_hash_batch_size=RPC_HASH_BATCH_SIZE,
        tip_height_cache_seconds=TIP_HEIGHT_CACHE_SECONDS,
    )
    print(
        "Proxy tuning: raw_cache_mib={} rpc_block_batch={} rpc_header_batch={} "
        "rpc_hash_batch={} tip_cache_s={} prefetch_window={} send_ahead={}".format(
            RAW_CACHE_MIB,
            RPC_BLOCK_BATCH_SIZE,
            RPC_HEADER_BATCH_SIZE,
            RPC_HASH_BATCH_SIZE,
            TIP_HEIGHT_CACHE_SECONDS,
            ACTIVE_PREFETCH_WINDOW,
            ACTIVE_SEND_AHEAD,
        )
    )

    stale_served_hook = None
    if args.target_rpc:
        stale_force_queue = queue.SimpleQueue()

        def stale_served_hook(stale, source):
            stale_force_queue.put((stale, source))

    proxy = HistoricalReorgProxy(
        chain_view=chain_view,
        stale_blocks=stale_blocks,
        defer_stale_until_getdata=DEFER_STALE_UNTIL_GETDATA,
        stale_served_hook=stale_served_hook,
        record_all_served_blocks=False,
        record_getdata_requests=False,
        active_prefetch_window=ACTIVE_PREFETCH_WINDOW,
        active_send_ahead=ACTIVE_SEND_AHEAD,
    )
    proxy.peer_connect_helper("0", 0, NETWORK, 1)
    proxy.configure_inbound_reconnect(
        net=NETWORK,
        services=P2P_SERVICES,
        supports_v2_p2p=False,
    )

    network_thread = NetworkThread()
    network_thread.start()

    ready = threading.Event()

    def on_listen(addr, port):
        print(f"Proxy listening on {addr}:{port}")
        print(f"Connect target node with: -connect={addr}:{port}")
        ready.set()

    NetworkThread.listen(proxy, on_listen, addr=LISTEN_HOST, port=args.listen_port)
    if not ready.wait(timeout=10):
        raise RuntimeError("Timed out while waiting for proxy listener startup")

    if args.target_rpc:

        def worker():
            nonlocal forced_stale_hashes
            while True:
                item = stale_force_queue.get()
                if item is None:
                    return
                stale, source = item
                stale_hash_hex = hash_int_to_hex(stale.hash_int)
                if stale_hash_hex in forced_stale_hashes:
                    continue

                rpc = None
                while rpc is None:
                    rpc = ensure_target_rpc()
                    if rpc is None:
                        time.sleep(0.2)
                        continue
                    height = try_getblockcount(rpc)
                    if height is None:
                        rpc = None
                        time.sleep(0.2)

                forced = False
                for attempt in range(1, 9):
                    try:
                        rpc.preciousblock(stale_hash_hex)
                        forced = True
                        break
                    except JSONRPCException as exc:
                        msg = None
                        code = None
                        if isinstance(exc.error, dict):
                            code = exc.error.get("code")
                            msg = exc.error.get("message")
                        if code == -28:
                            time.sleep(min(0.25 * attempt, 2.0))
                            continue
                        if msg and "Block not found" in msg:
                            try:
                                submit_res = rpc.submitblock(stale.raw_block.hex())
                                if submit_res is not None:
                                    print(
                                        f"[proxy] submitblock returned {submit_res!r} for stale {stale.height}-{stale_hash_hex}",
                                        flush=True,
                                    )
                            except Exception as submit_exc:
                                print(
                                    f"[proxy] submitblock failed for stale {stale.height}-{stale_hash_hex}: {submit_exc!r}",
                                    flush=True,
                                )
                            time.sleep(min(0.25 * attempt, 2.0))
                            continue
                        print(
                            f"[proxy] preciousblock failed for stale {stale.height}-{stale_hash_hex}: {exc!r}",
                            flush=True,
                        )
                        break
                    except Exception as exc:
                        print(
                            f"[proxy] preciousblock transport failure for stale {stale.height}-{stale_hash_hex}: {exc!r}",
                            flush=True,
                        )
                        time.sleep(min(0.25 * attempt, 2.0))
                if forced:
                    forced_stale_hashes.add(stale_hash_hex)
                    try:
                        best = rpc.getbestblockhash()
                    except Exception:
                        best = None
                    print(
                        "[proxy] forced precious stale [{}] height={} hash={} best={}".format(
                            source,
                            stale.height,
                            stale_hash_hex,
                            best if best is not None else "?",
                        ),
                        flush=True,
                    )

        stale_force_thread = threading.Thread(target=worker, name="stale-precious-worker", daemon=True)
        stale_force_thread.start()

    exit_code = 0
    snapshot_printed = False
    last_height_log_time = 0.0
    last_target_height = None
    target_height = None
    try:
        while True:
            snapshot_rpc = ensure_target_rpc()
            if snapshot_rpc is not None:
                target_height = try_getblockcount(snapshot_rpc)
                now = time.monotonic()
                if (
                    last_target_height is None
                    or (target_height is not None and target_height != last_target_height)
                    or now - last_height_log_time >= PROGRESS_LOG_SECONDS
                ):
                    if target_height is not None:
                        print(f"[target-progress] height={target_height}")
                        last_target_height = target_height
                        last_height_log_time = now

            if args.target_debug_log:
                live_debug_offset, last_tip, live_events = scan_target_debug_events(
                    args.target_debug_log,
                    start_offset=live_debug_offset,
                    last_tip=last_tip,
                )
                for event in live_events:
                    if event["kind"] == "reorg":
                        print(
                            "[target-reorg] prev_height={} prev_hash={} -> "
                            "new_height={} new_hash={}".format(
                                event["prev_height"],
                                event["prev_hash"],
                                event["new_height"],
                                event["new_hash"],
                            )
                        )
                    else:
                        print(f"[target-validation] {event['line']}")

            if compare_height is not None and snapshot_rpc is not None and target_height is not None:
                if target_height >= compare_height:
                    print("Computing UTXO set hashes (hash_serialized_3)...")
                    snapshot = collect_snapshot(snapshot_rpc)
                    upstream = collect_utxo_hash(upstream_rpc)
                    target = {
                        "height": snapshot.get("height"),
                        "bestblock": snapshot.get("bestblock"),
                        "hash_serialized_3": snapshot.get("hash_serialized_3"),
                    }
                    if not print_utxo_hash_comparison(upstream, target):
                        exit_code = 1
                    print(json.dumps(snapshot, indent=2, sort_keys=True))
                    snapshot_printed = True
                    break
            time.sleep(POLL_SECONDS)
    except KeyboardInterrupt:
        pass
    finally:
        if not snapshot_printed:
            snapshot_rpc = ensure_target_rpc()
            snapshot = try_collect_snapshot(snapshot_rpc)
            if snapshot is not None:
                print(json.dumps(snapshot, indent=2, sort_keys=True))

        if args.target_debug_log:
            served_stale = []
            seen_stale_hashes = set()
            for kind, height, block_hash_int in proxy.served_blocks:
                if kind != "stale":
                    continue
                if block_hash_int in seen_stale_hashes:
                    continue
                seen_stale_hashes.add(block_hash_int)
                served_stale.append(
                    {
                        "height": int(height),
                        "hash": hash_int_to_hex(block_hash_int),
                    }
                )

            update_tip_events = parse_update_tip_events(
                args.target_debug_log,
                start_offset=debug_log_offset,
            )
            summary = validate_reorgs_from_events(served_stale, update_tip_events)
            print_reorg_summary(summary)

            if summary["missing_tip"] or summary["missing_reorg"]:
                print("Reorg validation failed.")
                exit_code = 1

        network_thread.close(timeout=10)
        if stale_force_queue is not None:
            stale_force_queue.put(None)
        if stale_force_thread is not None:
            stale_force_thread.join(timeout=10)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
