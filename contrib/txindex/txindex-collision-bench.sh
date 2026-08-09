#!/usr/bin/env bash
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -euo pipefail

BASE_DIR="${BASE_DIR:-/mnt/my_storage}"
DATA_DIR="${DATA_DIR:-$BASE_DIR/BitcoinData}"
LOG_DIR="${LOG_DIR:-$BASE_DIR/logs}"
BENCH_ROOT="${BENCH_ROOT:-$BASE_DIR/txindex-bench}"
RPC_PORT="${RPC_PORT:-8332}"

repo_root="$(git rev-parse --show-toplevel)"
build_after="$BENCH_ROOT/build-after"
index_after="$BENCH_ROOT/index-after"
live_index="$DATA_DIR/indexes/txindex"
binary="$BENCH_ROOT/txindex-collision-count"
result="$LOG_DIR/txindex-collisions.txt"
compiler="${CXX:-c++}"

if [[ ! -d "$index_after" ]]; then
    if [[ -d "$live_index" ]]; then
        index_after="$live_index"
    else
        echo "No post-change txindex found. Run the rebuild benchmark first." >&2
        exit 1
    fi
fi
if [[ ! -f "$build_after/src/libleveldb.a" || ! -f "$build_after/src/libcrc32c.a" ]]; then
    echo "Missing the LevelDB build in $build_after; run the rebuild benchmark first." >&2
    exit 1
fi
if [[ -x "$build_after/bin/bitcoin-cli" ]] &&
   "$build_after/bin/bitcoin-cli" -datadir="$DATA_DIR" -rpcport="$RPC_PORT" \
       getblockcount >/dev/null 2>&1; then
    echo "Stop the node using $DATA_DIR before scanning LevelDB." >&2
    exit 1
fi
for command in "$compiler" hyperfine; do
    command -v "$command" >/dev/null || {
        echo "Missing required command: $command" >&2
        exit 1
    }
done

mkdir -p "$BENCH_ROOT" "$LOG_DIR"
"$compiler" \
    -std=c++20 \
    -O3 \
    -DNDEBUG \
    -I"$repo_root/src/leveldb/include" \
    "$repo_root/contrib/txindex/txindex-collision-count.cpp" \
    "$build_after/src/libleveldb.a" \
    "$build_after/src/libcrc32c.a" \
    -pthread \
    -o "$binary"

printf -v count_command '%q ' "$binary" "$index_after"
printf -v result_path '%q' "$result"
count_command+=" >$result_path 2>&1"

hyperfine \
    --runs 1 \
    --warmup 0 \
    --shell bash \
    --show-output \
    --command-name collisions "$count_command" \
    --export-json "$LOG_DIR/txindex-collisions.json"

printf "\n=== txindex collision result ===\n"
cat "$result"
