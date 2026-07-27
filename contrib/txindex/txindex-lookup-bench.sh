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
HYPERFINE_RUNS="${HYPERFINE_RUNS:-10}"
SAMPLES_PER_FILE="${SAMPLES_PER_FILE:-16}"

repo_root="$(git rev-parse --show-toplevel)"
tool="$repo_root/contrib/txindex/txindex-lookup-bench.py"
build_before="$BENCH_ROOT/build-before"
build_after="$BENCH_ROOT/build-after"
index_before="$BENCH_ROOT/index-before"
index_after="$BENCH_ROOT/index-after"
live_index="$DATA_DIR/indexes/txindex"
corpus_dir="${CORPUS_DIR:-$LOG_DIR/txindex-lookup-corpus}"

for command in hyperfine python3; do
    command -v "$command" >/dev/null || {
        echo "Missing required command: $command" >&2
        exit 1
    }
done
if [[ ! -x "$build_before/bin/bitcoind" || ! -x "$build_after/bin/bitcoind" ||
      ! -d "$index_before" || ! -d "$index_after" ]]; then
    echo "Missing before/after builds or txindex snapshots in $BENCH_ROOT." >&2
    echo "Run the txindex rebuild benchmark first." >&2
    exit 1
fi
if [[ -e "$live_index" ]]; then
    echo "$live_index already exists; move it aside or rerun the rebuild benchmark." >&2
    exit 1
fi

mkdir -p "$LOG_DIR"

active_pid=""
active_cli=""
active_snapshot=""

stop_node()
{
    if [[ -n "$active_pid" ]]; then
        "$active_cli" -datadir="$DATA_DIR" -rpcport="$RPC_PORT" stop >/dev/null 2>&1 || {
            kill "$active_pid" 2>/dev/null || true
        }
        wait "$active_pid" 2>/dev/null || true
        active_pid=""
    fi
    if [[ -n "$active_snapshot" && -d "$live_index" ]]; then
        if [[ -e "$active_snapshot" ]]; then
            echo "Refusing to overwrite $active_snapshot while restoring txindex." >&2
            return 1
        fi
        mv "$live_index" "$active_snapshot"
    fi
    active_cli=""
    active_snapshot=""
}

cleanup()
{
    set +e
    stop_node
}
trap cleanup EXIT INT TERM

start_node()
{
    local label="$1"
    local build_dir="$2"
    local snapshot="$3"
    local log_file="$LOG_DIR/txindex-lookup-$label.log"
    local daemon="$build_dir/bin/bitcoind"

    if [[ ! -d "$snapshot" || -e "$live_index" ]]; then
        echo "Cannot activate $snapshot as $live_index." >&2
        return 1
    fi
    mv "$snapshot" "$live_index"
    active_snapshot="$snapshot"
    active_cli="$build_dir/bin/bitcoin-cli"
    : >"$log_file"
    "$daemon" \
        -datadir="$DATA_DIR" \
        -rpcport="$RPC_PORT" \
        -txindex=1 \
        -connect=0 \
        -printtoconsole=0 \
        -debuglogfile="$log_file" &
    active_pid=$!

    while kill -0 "$active_pid" 2>/dev/null; do
        if "$active_cli" -datadir="$DATA_DIR" -rpcport="$RPC_PORT" getblockcount >/dev/null 2>&1 &&
           grep -Fq "txindex is enabled at height" "$log_file" 2>/dev/null; then
            return 0
        fi
        sleep 1
    done
    wait "$active_pid" || true
    active_pid=""
    echo "bitcoind exited before txindex was ready; tail of $log_file:" >&2
    tail -100 "$log_file" >&2 || true
    return 1
}

if [[ ! -f "$corpus_dir/hits.tsv" || ! -f "$corpus_dir/misses.txt" ||
      ! -f "$corpus_dir/hot.txt" ]]; then
    if [[ -e "$corpus_dir" ]]; then
        echo "$corpus_dir is incomplete; remove it before regenerating the corpus." >&2
        exit 1
    fi
    echo "Generating the shared lookup corpus with the before binary."
    start_node corpus "$build_before" "$index_before"
    python3 "$tool" --datadir "$DATA_DIR" --rpc-port "$RPC_PORT" generate \
        --corpus-dir "$corpus_dir" \
        --samples-per-file "$SAMPLES_PER_FILE"
    stop_node
fi

benchmark_version()
{
    local label="$1"
    local build_dir="$2"
    local snapshot="$3"
    local json_file="$LOG_DIR/txindex-lookup-$label.json"
    local csv_file="$LOG_DIR/txindex-lookup-$label.csv"
    local -a common=(python3 "$tool" --datadir "$DATA_DIR" --rpc-port "$RPC_PORT")
    local hits_command misses_command hot_command

    echo
    echo "Benchmarking $label lookups with $snapshot"
    start_node "$label" "$build_dir" "$snapshot"
    "${common[@]}" verify --hits "$corpus_dir/hits.tsv"

    echo "First post-restart passes:"
    "${common[@]}" run --corpus "$corpus_dir/hits.tsv" --expect hit
    "${common[@]}" run --corpus "$corpus_dir/misses.txt" --expect miss
    "${common[@]}" run --corpus "$corpus_dir/hot.txt" --expect hit

    printf -v hits_command "%q " "${common[@]}" run --quiet \
        --corpus "$corpus_dir/hits.tsv" --expect hit
    printf -v misses_command "%q " "${common[@]}" run --quiet \
        --corpus "$corpus_dir/misses.txt" --expect miss
    printf -v hot_command "%q " "${common[@]}" run --quiet \
        --corpus "$corpus_dir/hot.txt" --expect hit

    hyperfine \
        --warmup 1 \
        --runs "$HYPERFINE_RUNS" \
        --shell bash \
        --sort command \
        --command-name "$label-random-hits" "$hits_command" \
        --command-name "$label-misses" "$misses_command" \
        --command-name "$label-repeated-hot-hit" "$hot_command" \
        --export-csv "$csv_file" \
        --export-json "$json_file"
    stop_node
}

benchmark_version before "$build_before" "$index_before"
benchmark_version after "$build_after" "$index_after"
trap - EXIT INT TERM

hits_count=$(($(wc -l <"$corpus_dir/hits.tsv") - 1))
paste -d, \
    <(tail -n +2 "$LOG_DIR/txindex-lookup-before.csv") \
    <(tail -n +2 "$LOG_DIR/txindex-lookup-after.csv") |
    awk -F, -v lookups="$hits_count" '
        BEGIN {
            print "\n=== txindex lookup result ==="
            printf "corpus: %d lookups per workload\n", lookups
            printf "%-22s %13s %13s %13s %13s  %s\n",
                "workload", "before total", "after total",
                "before/lookup", "after/lookup", "comparison"
        }
        {
            workload = $1
            gsub(/^"|"$/, "", workload)
            sub(/^before-/, "", workload)
            gsub(/-/, " ", workload)
            before = $2
            after = $10
            if (after < before) {
                comparison = sprintf("after uses %.1f%% less wall time",
                    100 * (before - after) / before)
                ++after_wins
            } else if (after > before) {
                comparison = sprintf("after uses %.1f%% more wall time",
                    100 * (after - before) / before)
            } else {
                comparison = "same wall time"
            }
            printf "%-22s %10.4f s %10.4f s %10.3f ms %10.3f ms  %s\n",
                workload, before, after, before * 1000 / lookups,
                after * 1000 / lookups, comparison
        }
        END {
            printf "Conclusion: the post-change txindex had lower wall time in %d of %d workloads.\n",
                after_wins, NR
        }
    '
printf "Raw hyperfine data:\n  %s\n  %s\n" \
    "$LOG_DIR/txindex-lookup-before.json" \
    "$LOG_DIR/txindex-lookup-after.json"
