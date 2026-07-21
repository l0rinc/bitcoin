#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Generate and benchmark random txindex lookups across block files.

The generator samples active-chain blocks independently from every physical
block file. The benchmark then measures successful and missing
``getrawtransaction`` requests separately over one persistent RPC connection.
"""

import argparse
import base64
import csv
from dataclasses import dataclass
import hashlib
from http.client import HTTPConnection
import json
import math
from pathlib import Path
import random
import re
import statistics
import sys
import time


MAX_BLOCK_SERIALIZED_SIZE = 4_000_000
MAINNET_BIP30_HEIGHTS = {91_722, 91_812, 91_842, 91_880}
NETWORK_MAGIC = {
    "main": bytes.fromhex("f9beb4d9"),
    "regtest": bytes.fromhex("fabfb5da"),
    "signet": bytes.fromhex("0a03cf40"),
    "test": bytes.fromhex("0b110907"),
    "testnet4": bytes.fromhex("1c163f28"),
}


class RPCError(RuntimeError):
    def __init__(self, error):
        super().__init__(f"RPC error {error['code']}: {error['message']}")
        self.error = error


class BitcoinRPC:
    def __init__(self, host, port, cookie_path):
        user, password = cookie_path.read_text(encoding="utf8").strip().split(":", 1)
        auth = base64.b64encode(f"{user}:{password}".encode()).decode()
        self.connection = HTTPConnection(host, port=port, timeout=900)
        self.headers = {
            "Authorization": f"Basic {auth}",
            "Content-Type": "application/json",
        }
        self.request_id = 0

    def _new_request(self, method, params):
        self.request_id += 1
        return {
            "jsonrpc": "2.0",
            "id": self.request_id,
            "method": method,
            "params": [] if params is None else params,
        }

    def request(self, method, params=None):
        request = self._new_request(method, params)
        response = self._send(request)
        if response.get("id") != request["id"]:
            raise RuntimeError("Unexpected JSON-RPC response id")
        return response

    def call(self, method, params=None):
        response = self.request(method, params)
        if response.get("error") is not None:
            raise RPCError(response["error"])
        return response["result"]

    def batch(self, calls):
        requests = [self._new_request(method, params) for method, params in calls]
        responses = {response["id"]: response for response in self._send(requests)}
        results = []
        for request_id in (request["id"] for request in requests):
            response = responses[request_id]
            if response.get("error") is not None:
                raise RPCError(response["error"])
            results.append(response["result"])
        return results

    def _send(self, request):
        body = json.dumps(request, separators=(",", ":"))
        self.connection.request("POST", "/", body=body, headers=self.headers)
        response = self.connection.getresponse()
        payload = response.read()
        if not payload:
            raise RuntimeError(f"Empty RPC response ({response.status} {response.reason})")
        return json.loads(payload)


@dataclass(frozen=True)
class BlockExtent:
    path: Path
    file_number: int
    offset: int
    size: int
    block_hash: str
    height: int


def make_rpc(args):
    cookie_path = args.cookie or args.datadir / ".cookie"
    return BitcoinRPC(args.rpc_host, args.rpc_port, cookie_path)


def xor_data(data, key, offset):
    if not any(key):
        return data
    return bytes(value ^ key[(offset + index) % len(key)] for index, value in enumerate(data))


def block_hash(header):
    return hashlib.sha256(hashlib.sha256(header).digest()).digest()[::-1].hex()


def read_xor_key(blocks_dir):
    try:
        key = (blocks_dir / "xor.dat").read_bytes()
    except FileNotFoundError:
        return bytes(8)
    if len(key) != 8:
        raise RuntimeError(f"Unexpected XOR key length: {len(key)}")
    return key


def get_active_blocks(rpc, tip_height):
    active = {}
    batch_size = 10_000
    for first_height in range(0, tip_height + 1, batch_size):
        last_height = min(first_height + batch_size, tip_height + 1)
        calls = [("getblockhash", [height]) for height in range(first_height, last_height)]
        for height, hash_value in zip(range(first_height, last_height), rpc.batch(calls)):
            active[hash_value] = height
        print(f"Read active-chain hashes through height {last_height - 1}", file=sys.stderr)
    return active


def list_block_files(blocks_dir):
    files = []
    for path in blocks_dir.glob("blk*.dat"):
        match = re.fullmatch(r"blk(\d+)\.dat", path.name)
        if match:
            files.append((int(match.group(1)), path))
    return sorted(files)


def sample_block_file(path, file_number, active_blocks, network_magic, xor_key, count, rng):
    reservoir = []
    eligible = 0
    with path.open("rb") as block_file:
        while True:
            frame_offset = block_file.tell()
            raw_frame = block_file.read(8)
            if len(raw_frame) < 8 or raw_frame == bytes(8):
                break
            frame = xor_data(raw_frame, xor_key, frame_offset)
            if frame == bytes(8):
                break
            if frame[:4] != network_magic:
                block_file.seek(frame_offset + 1)
                continue

            size = int.from_bytes(frame[4:], "little")
            if size <= 80 or size > MAX_BLOCK_SERIALIZED_SIZE:
                block_file.seek(frame_offset + 1)
                continue

            block_offset = block_file.tell()
            raw_header = block_file.read(80)
            if len(raw_header) < 80:
                break
            header = xor_data(raw_header, xor_key, block_offset)
            hash_value = block_hash(header)
            height = active_blocks.get(hash_value)
            if height is not None and height > 0:
                extent = BlockExtent(path, file_number, block_offset, size, hash_value, height)
                eligible += 1
                if len(reservoir) < count:
                    reservoir.append(extent)
                else:
                    replace = rng.randrange(eligible)
                    if replace < count:
                        reservoir[replace] = extent

            block_file.seek(block_offset + size)
    return reservoir


def flip_txid(txid):
    value = bytearray.fromhex(txid)
    value[0] ^= 1
    return value.hex()


def write_corpus(corpus_dir, rows):
    corpus_dir.mkdir(parents=True, exist_ok=True)
    hits_path = corpus_dir / "hits.tsv"
    misses_path = corpus_dir / "misses.txt"
    hot_path = corpus_dir / "hot.txt"
    with hits_path.open("w", encoding="utf8", newline="") as hits_file:
        writer = csv.DictWriter(hits_file, fieldnames=rows[0].keys(), delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)
    with misses_path.open("w", encoding="utf8") as misses_file:
        for row in rows:
            misses_file.write(f"{flip_txid(row['txid'])}\n")
    with hot_path.open("w", encoding="utf8") as hot_file:
        for _ in rows:
            hot_file.write(f"{rows[0]['txid']}\n")
    return hits_path, misses_path, hot_path


def generate(args):
    hits_path = args.corpus_dir / "hits.tsv"
    misses_path = args.corpus_dir / "misses.txt"
    hot_path = args.corpus_dir / "hot.txt"
    if not args.force and (hits_path.exists() or misses_path.exists() or hot_path.exists()):
        raise RuntimeError(f"Corpus already exists in {args.corpus_dir}; use --force to replace it")

    rpc = make_rpc(args)
    chain_info = rpc.call("getblockchaininfo")
    if chain_info["pruned"]:
        raise RuntimeError("Corpus generation requires an unpruned node")
    try:
        network_magic = NETWORK_MAGIC[chain_info["chain"]]
    except KeyError as error:
        raise RuntimeError(f"Unknown chain {chain_info['chain']}") from error

    blocks_dir = args.blocks_dir or args.datadir / "blocks"
    block_files = list_block_files(blocks_dir)
    if not block_files:
        raise RuntimeError(f"No block files found in {blocks_dir}")

    rng = random.Random(args.seed)
    active_blocks = get_active_blocks(rpc, chain_info["blocks"])
    if chain_info["chain"] == "main":
        active_blocks = {
            hash_value: height
            for hash_value, height in active_blocks.items()
            if height not in MAINNET_BIP30_HEIGHTS
        }
    xor_key = read_xor_key(blocks_dir)
    extents = []
    for index, (file_number, path) in enumerate(block_files, start=1):
        extents.extend(sample_block_file(
            path,
            file_number,
            active_blocks,
            network_magic,
            xor_key,
            args.samples_per_file,
            rng,
        ))
        if index % 100 == 0 or index == len(block_files):
            print(f"Scanned {index}/{len(block_files)} block files", file=sys.stderr)

    rng.shuffle(extents)
    if args.max_samples:
        extents = extents[:args.max_samples]

    rows = []
    seen_txids = set()
    for index, extent in enumerate(extents, start=1):
        block = rpc.call("getblock", [extent.block_hash, 1])
        if block["height"] != extent.height:
            raise RuntimeError(f"Height mismatch for block {extent.block_hash}")
        tx_indexes = list(range(len(block["tx"])))
        rng.shuffle(tx_indexes)
        for tx_index in tx_indexes:
            txid = block["tx"][tx_index]
            if txid not in seen_txids:
                break
        else:
            continue
        seen_txids.add(txid)
        rows.append({
            "txid": txid,
            "blockhash": extent.block_hash,
            "height": extent.height,
            "file": extent.file_number,
            "offset": extent.offset,
            "tx_index": tx_index,
            "coinbase": int(tx_index == 0),
        })
        if index % 1_000 == 0 or index == len(extents):
            print(f"Selected transactions from {index}/{len(extents)} blocks", file=sys.stderr)

    if not rows:
        raise RuntimeError("No transactions selected")
    rng.shuffle(rows)
    hits_path, misses_path, hot_path = write_corpus(args.corpus_dir, rows)
    coinbases = sum(row["coinbase"] for row in rows)
    print(f"Wrote {len(rows)} hits ({coinbases} coinbases) to {hits_path}")
    print(f"Wrote {len(rows)} misses to {misses_path}")
    print(f"Wrote {len(rows)} repeated hot lookups to {hot_path}")


def read_hits(path):
    with path.open(encoding="utf8", newline="") as hits_file:
        yield from csv.DictReader(hits_file, delimiter="\t")


def read_txids(path):
    with path.open(encoding="utf8") as txid_file:
        for line in txid_file:
            if txid := line.strip():
                yield txid


def percentile(sorted_values, percent):
    return sorted_values[math.ceil(len(sorted_values) * percent / 100) - 1]


def print_latencies(label, latencies):
    ordered = sorted(latencies)
    lookup = "lookup" if len(ordered) == 1 else "lookups"
    print(
        f"{label}: {len(ordered)} {lookup}, "
        f"mean={statistics.fmean(ordered):.3f} ms, "
        f"p50={percentile(ordered, 50):.3f} ms, "
        f"p90={percentile(ordered, 90):.3f} ms, "
        f"p99={percentile(ordered, 99):.3f} ms, "
        f"max={ordered[-1]:.3f} ms"
    )


def verify(args):
    rpc = make_rpc(args)
    tip_height = rpc.call("getblockcount")
    count = 0
    for row in read_hits(args.hits):
        result = rpc.call("getrawtransaction", [row["txid"], 1])
        if result.get("blockhash") != row["blockhash"]:
            raise RuntimeError(f"Block hash mismatch for transaction {row['txid']}")
        height = tip_height - result.get("confirmations", 0) + 1
        if height != int(row["height"]):
            raise RuntimeError(f"Height mismatch for transaction {row['txid']}")
        count += 1
    print(f"Verified {count} transaction block associations")


def run(args):
    rpc = make_rpc(args)
    entries = list(read_hits(args.corpus)) if args.corpus.suffix == ".tsv" else [
        {"txid": txid} for txid in read_txids(args.corpus)
    ]
    if not entries:
        raise RuntimeError(f"No transactions found in {args.corpus}")
    latencies = []
    by_height_decile = [[] for _ in range(10)]
    tip_height = rpc.call("getblockcount") if "height" in entries[0] else 0
    for entry in entries:
        start = time.perf_counter_ns()
        response = rpc.request("getrawtransaction", [entry["txid"], 0])
        latency = (time.perf_counter_ns() - start) / 1_000_000
        latencies.append(latency)
        error = response.get("error")
        if args.expect == "hit":
            if error is not None or not isinstance(response.get("result"), str):
                raise RuntimeError(f"Expected transaction {entry['txid']} to be found: {error}")
        elif error is None or error.get("code") != -5:
            raise RuntimeError(f"Expected transaction {entry['txid']} to be missing: {error}")
        if tip_height:
            decile = min(int(entry["height"]) * 10 // (tip_height + 1), 9)
            by_height_decile[decile].append(latency)
    if args.quiet:
        return
    print_latencies(args.expect, latencies)
    for decile, values in enumerate(by_height_decile):
        if values:
            print_latencies(f"height decile {decile}", values)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--datadir", type=Path, required=True, help="Network data directory containing .cookie and blocks/"
    )
    parser.add_argument("--blocks-dir", type=Path, help="Override the blocks directory")
    parser.add_argument("--cookie", type=Path, help="Override the RPC cookie path")
    parser.add_argument("--rpc-host", default="127.0.0.1")
    parser.add_argument("--rpc-port", type=int, default=8332)
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate_parser = subparsers.add_parser("generate", help="Generate hit and miss corpora")
    generate_parser.add_argument("--corpus-dir", type=Path, required=True)
    generate_parser.add_argument("--samples-per-file", type=int, default=16)
    generate_parser.add_argument("--max-samples", type=int, default=0)
    generate_parser.add_argument("--seed", type=int, default=35531)
    generate_parser.add_argument("--force", action="store_true")
    generate_parser.set_defaults(func=generate)

    verify_parser = subparsers.add_parser("verify", help="Verify hit block hashes and heights")
    verify_parser.add_argument("--hits", type=Path, required=True)
    verify_parser.set_defaults(func=verify)

    run_parser = subparsers.add_parser("run", help="Run and validate one lookup corpus")
    run_parser.add_argument("--corpus", type=Path, required=True)
    run_parser.add_argument("--expect", choices=("hit", "miss"), required=True)
    run_parser.add_argument("--quiet", action="store_true")
    run_parser.set_defaults(func=run)
    return parser.parse_args()


def main():
    args = parse_args()
    try:
        args.func(args)
    except (OSError, RPCError, RuntimeError, ValueError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
