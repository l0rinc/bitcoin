#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Run a stale-first block proxy for historical reorg replay."""

from argparse import ArgumentParser
from collections import defaultdict
import json
from pathlib import Path
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

from test_framework.authproxy import AuthServiceProxy  # noqa: E402
from test_framework.p2p import NetworkThread  # noqa: E402
from test_framework.stale_reorg_proxy import (  # noqa: E402
    HistoricalReorgProxy,
    RPCChainView,
    hash_int_to_hex,
    load_stale_blocks_from_dir,
)

UPDATE_TIP_RE = re.compile(r"UpdateTip: new best=([0-9a-f]{64}) height=(\d+)")


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
        "--listen-host",
        default="127.0.0.1",
        help="Host to bind the proxy listener",
    )
    parser.add_argument(
        "--listen-port",
        type=int,
        default=8338,
        help="Port to bind the proxy listener",
    )
    parser.add_argument(
        "--network",
        choices=["mainnet", "testnet4", "signet", "regtest"],
        default="mainnet",
        help="P2P network magic to use",
    )
    parser.add_argument(
        "--max-height",
        type=int,
        default=None,
        help="Optional maximum active-chain height to serve",
    )
    parser.add_argument(
        "--snapshot-height",
        type=int,
        default=None,
        help="If --target-rpc is set, emit UTXO snapshot once this height is reached",
    )
    parser.add_argument(
        "--poll-seconds",
        type=float,
        default=5.0,
        help="Polling interval for --snapshot-height checks",
    )
    parser.add_argument(
        "--exit-after-snapshot",
        action="store_true",
        help="Exit immediately after printing snapshot data",
    )
    parser.add_argument(
        "--target-debug-log",
        default=None,
        help="Path to target node debug.log for reorg validation summary",
    )
    parser.add_argument(
        "--allow-missing-reorgs",
        action="store_true",
        help="Do not return non-zero if stale blocks lack reorg evidence in debug.log",
    )
    return parser.parse_args()


def collect_snapshot(target_rpc):
    snapshot = target_rpc.gettxoutsetinfo("hash_serialized_3", use_index=False)
    return {
        "height": snapshot["height"],
        "bestblock": snapshot["bestblock"],
        "hash_serialized_3": snapshot.get("hash_serialized_3"),
        "txouts": snapshot["txouts"],
        "total_amount": snapshot["total_amount"],
    }


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
    if cookie_file is None:
        return rpc_url

    cookie_text = Path(cookie_file).read_text(encoding="utf-8").strip()
    if ":" not in cookie_text:
        raise ValueError(f"Malformed cookie file (missing user:pass): {cookie_file}")

    cookie_user, cookie_pass = cookie_text.split(":", 1)
    normalized_url = rpc_url if "://" in rpc_url else f"http://{rpc_url}"
    split = urlsplit(normalized_url)

    # If credentials already exist in URL, keep them.
    if split.username is not None:
        return normalized_url

    netloc = f"{quote(cookie_user, safe='')}:{quote(cookie_pass, safe='')}@{split.netloc}"
    return urlunsplit((split.scheme, netloc, split.path, split.query, split.fragment))


def main():
    args = parse_args()
    debug_log_offset = 0

    upstream_rpc_url = rpc_url_with_cookie(args.upstream_rpc, args.upstream_cookie_file)
    upstream_rpc = AuthServiceProxy(upstream_rpc_url, timeout=300)

    target_rpc = None
    if args.target_rpc:
        target_rpc_url = rpc_url_with_cookie(args.target_rpc, args.target_cookie_file)
        target_rpc = AuthServiceProxy(target_rpc_url, timeout=300)

    stale_dir = Path(args.stale_blocks_dir)
    stale_blocks = []
    if stale_dir.exists():
        stale_blocks = load_stale_blocks_from_dir(stale_dir)
    else:
        print(f"Warning: stale blocks directory does not exist: {stale_dir}")

    print(f"Loaded {len(stale_blocks)} stale blocks from {stale_dir}")

    if args.target_debug_log:
        debug_log_path = Path(args.target_debug_log)
        if debug_log_path.exists():
            debug_log_offset = debug_log_path.stat().st_size
        print(
            f"Tracking debug log for reorg validation: {debug_log_path} "
            f"(start byte={debug_log_offset})"
        )

    proxy = HistoricalReorgProxy(
        chain_view=RPCChainView(upstream_rpc),
        stale_blocks=stale_blocks,
        max_height=args.max_height,
    )
    proxy.peer_connect_helper("0", 0, args.network, 1)
    proxy.reconnect = False

    network_thread = NetworkThread()
    network_thread.start()

    ready = threading.Event()

    def on_listen(addr, port):
        print(f"Proxy listening on {addr}:{port}")
        print(f"Connect target node with: -connect={addr}:{port}")
        ready.set()

    NetworkThread.listen(proxy, on_listen, addr=args.listen_host, port=args.listen_port)
    if not ready.wait(timeout=10):
        raise RuntimeError("Timed out while waiting for proxy listener startup")

    exit_code = 0
    snapshot_printed = False
    try:
        while True:
            if target_rpc and args.snapshot_height is not None:
                target_height = target_rpc.getblockcount()
                if target_height >= args.snapshot_height:
                    print(json.dumps(collect_snapshot(target_rpc), indent=2, sort_keys=True))
                    snapshot_printed = True
                    if args.exit_after_snapshot:
                        break
                    args.snapshot_height = None
            time.sleep(args.poll_seconds)
    except KeyboardInterrupt:
        pass
    finally:
        if target_rpc and not snapshot_printed:
            print(json.dumps(collect_snapshot(target_rpc), indent=2, sort_keys=True))

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
                if args.allow_missing_reorgs:
                    print(
                        "Reorg validation did not fully pass, but continuing "
                        "due to --allow-missing-reorgs."
                    )
                else:
                    print("Reorg validation failed.")
                    exit_code = 1

        network_thread.close(timeout=10)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
