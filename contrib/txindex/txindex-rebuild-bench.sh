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
JOBS="${JOBS:-$(nproc)}"

repo_root="$(git rev-parse --show-toplevel)"
before_commit="${BEFORE_COMMIT:-}"
after_commit="${AFTER_COMMIT:-}"

if [[ -z "$before_commit" || -z "$after_commit" ]]; then
    echo "Set BEFORE_COMMIT and AFTER_COMMIT to the commits to compare." >&2
    exit 1
fi

before_commit="$(git rev-parse "$before_commit^{commit}")"
after_commit="$(git rev-parse "$after_commit^{commit}")"
build_before="$BENCH_ROOT/build-before"
build_after="$BENCH_ROOT/build-after"
index_before="$BENCH_ROOT/index-before"
index_after="$BENCH_ROOT/index-after"
live_index="$DATA_DIR/indexes/txindex"
size_log="$LOG_DIR/txindex-rebuild-sizes.tsv"
result_csv="$LOG_DIR/txindex-rebuild.csv"

if [[ "$BASE_DIR" != /* || "$DATA_DIR" != /* || "$LOG_DIR" != /* || "$BENCH_ROOT" != /* ]]; then
    echo "BASE_DIR, DATA_DIR, LOG_DIR, and BENCH_ROOT must be absolute paths." >&2
    exit 1
fi
if [[ "$BASE_DIR" == "/" || "$DATA_DIR" == "/" || "$BENCH_ROOT" == "/" ||
      "$live_index" != "$DATA_DIR/indexes/txindex" ]]; then
    echo "Refusing unsafe benchmark paths." >&2
    exit 1
fi
if [[ ! -d "$DATA_DIR/blocks" || ! -d "$DATA_DIR/chainstate" ]]; then
    echo "Expected an existing blockchain in $DATA_DIR." >&2
    exit 1
fi
for command in cmake git hyperfine ninja; do
    command -v "$command" >/dev/null || {
        echo "Missing required command: $command" >&2
        exit 1
    }
done

mkdir -p "$BENCH_ROOT" "$LOG_DIR" "$DATA_DIR/indexes"

if ! git -C "$repo_root" diff --quiet ||
   ! git -C "$repo_root" diff --cached --quiet; then
    echo "Commit or stash tracked changes before running the benchmark." >&2
    exit 1
fi

original_commit="$(git -C "$repo_root" rev-parse HEAD)"
original_branch="$(git -C "$repo_root" symbolic-ref --quiet --short HEAD || true)"

restore_checkout()
{
    if [[ -n "$original_branch" ]]; then
        if [[ "$(git -C "$repo_root" symbolic-ref --quiet --short HEAD || true)" == "$original_branch" ]]; then
            return
        fi
        git -C "$repo_root" checkout --quiet "$original_branch"
    else
        if [[ "$(git -C "$repo_root" rev-parse HEAD)" == "$original_commit" ]]; then
            return
        fi
        git -C "$repo_root" checkout --quiet --detach "$original_commit"
    fi
}

build_version()
{
    local label="$1"
    local build_dir="$2"
    local commit="$3"
    echo "Building $label at $commit"
    git -C "$repo_root" checkout --quiet --detach "$commit"
    rm -rf -- "$build_dir"
    cmake -S "$repo_root" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        >"$LOG_DIR/txindex-build-$label.log"
    cmake --build "$build_dir" -j "$JOBS" --target bitcoind bitcoin-cli \
        >>"$LOG_DIR/txindex-build-$label.log"
}

trap restore_checkout EXIT
build_version before "$build_before" "$before_commit"
build_version after "$build_after" "$after_commit"
restore_checkout
trap - EXIT

before_daemon="$build_before/bin/bitcoind"
before_cli="$build_before/bin/bitcoin-cli"
after_daemon="$build_after/bin/bitcoind"
after_cli="$build_after/bin/bitcoin-cli"

stop_existing_node()
{
    local cli
    for cli in "$before_cli" "$after_cli"; do
        if "$cli" -datadir="$DATA_DIR" -rpcport="$RPC_PORT" getblockcount >/dev/null 2>&1; then
            echo "Stopping the node currently using $DATA_DIR"
            "$cli" -datadir="$DATA_DIR" -rpcport="$RPC_PORT" stop >/dev/null
            while "$cli" -datadir="$DATA_DIR" -rpcport="$RPC_PORT" getblockcount >/dev/null 2>&1; do
                sleep 1
            done
            return
        fi
    done
}

wait_index()
{
    local pid="$1"
    local log_file="$2"
    while kill -0 "$pid" 2>/dev/null; do
        if grep -Fq "txindex is enabled at height" "$log_file" 2>/dev/null; then
            return 0
        fi
        sleep 5
    done
    wait "$pid" || true
    echo "bitcoind exited before txindex finished; tail of $log_file:" >&2
    tail -100 "$log_file" >&2 || true
    return 1
}

run_index()
{
    local label="$1"
    local daemon="$2"
    local cli="$3"
    local snapshot="$4"
    local log_file="$5"
    local pid

    : >"$log_file"
    "$daemon" \
        -datadir="$DATA_DIR" \
        -rpcport="$RPC_PORT" \
        -txindex=1 \
        -connect=0 \
        -printtoconsole=0 \
        -debuglogfile="$log_file" &
    pid=$!
    if ! wait_index "$pid" "$log_file"; then
        kill "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
        return 1
    fi
    "$cli" -datadir="$DATA_DIR" -rpcport="$RPC_PORT" stop >/dev/null || {
        kill "$pid" 2>/dev/null || true
    }
    wait "$pid"
    if [[ ! -d "$live_index" ]]; then
        echo "$label did not create $live_index" >&2
        return 1
    fi
    mv "$live_index" "$snapshot"
}

stop_existing_node

# Only the txindex database and prior benchmark snapshots are removed. Blocks,
# chainstate, wallets, and other indexes remain untouched.
rm -rf -- "$live_index" "$index_before" "$index_after"

export DATA_DIR RPC_PORT live_index
export -f run_index wait_index
printf -v prepare_command 'rm -rf -- %q' "$live_index"
printf -v before_command '%q ' run_index before "$before_daemon" "$before_cli" \
    "$index_before" "$LOG_DIR/txindex-rebuild-before.log"
printf -v after_command '%q ' run_index after "$after_daemon" "$after_cli" \
    "$index_after" "$LOG_DIR/txindex-rebuild-after.log"

cpu="$(lscpu | sed -n 's/^Model name:[[:space:]]*//p' | head -1)"
filesystem="$(df -T "$DATA_DIR" | awk 'NR == 2 {print $2}')"
printf "txindex rebuild benchmark | before=%s | after=%s | host=%s | architecture=%s | threads=%s | cpu=%s | filesystem=%s\n" \
    "$before_commit" "$after_commit" "$(hostname)" "$(uname -m)" "$(nproc)" "$cpu" "$filesystem" |
    tee "$LOG_DIR/txindex-rebuild-host.txt"

hyperfine \
    --runs 1 \
    --warmup 0 \
    --shell bash \
    --sort command \
    --show-output \
    --prepare "$prepare_command" \
    --command-name before "$before_command" \
    --command-name after "$after_command" \
    --export-csv "$result_csv" \
    --export-json "$LOG_DIR/txindex-rebuild.json"

before_bytes="$(du -sb "$index_before" | awk '{print $1}')"
after_bytes="$(du -sb "$index_after" | awk '{print $1}')"
before_human="$(du -sh "$index_before" | awk '{print $1}')"
after_human="$(du -sh "$index_after" | awk '{print $1}')"
{
    printf "label\tbytes\thuman\tpath\n"
    printf "before\t%s\t%s\t%s\n" "$before_bytes" "$before_human" "$index_before"
    printf "after\t%s\t%s\t%s\n" "$after_bytes" "$after_human" "$index_after"
} | tee "$size_log"

awk \
    -F, \
    -v before_commit="$before_commit" \
    -v after_commit="$after_commit" \
    -v before_bytes="$before_bytes" \
    -v after_bytes="$after_bytes" \
    -v before_human="$before_human" \
    -v after_human="$after_human" '
    NR > 1 {
        gsub(/^"|"$/, "", $1)
        seconds[$1] = $2
    }
    BEGIN {
        print "\n=== txindex rebuild result ==="
        printf "before commit: %s\nafter commit:  %s\n", before_commit, after_commit
    }
    END {
        printf "%-24s %16s %16s\n", "measurement", "before", "after"
        printf "%-24s %13.2f s %13.2f s\n",
            "rebuild wall time", seconds["before"], seconds["after"]
        printf "%-24s %16s %16s\n", "txindex size", before_human, after_human
        printf "%-24s %16s %16s\n", "txindex bytes", before_bytes, after_bytes
        if (seconds["after"] < seconds["before"]) {
            time_result = sprintf("%.1f%% less wall time (%.2fx speedup)",
                100 * (seconds["before"] - seconds["after"]) / seconds["before"],
                seconds["before"] / seconds["after"])
        } else if (seconds["after"] > seconds["before"]) {
            time_result = sprintf("%.1f%% more wall time",
                100 * (seconds["after"] - seconds["before"]) / seconds["before"])
        } else {
            time_result = "the same wall time"
        }
        if (after_bytes < before_bytes) {
            size_result = sprintf("%.1f%% smaller", 100 * (before_bytes - after_bytes) / before_bytes)
        } else if (after_bytes > before_bytes) {
            size_result = sprintf("%.1f%% larger", 100 * (after_bytes - before_bytes) / before_bytes)
        } else {
            size_result = "the same size"
        }
        printf "Conclusion: the post-change rebuild used %s and its txindex is %s.\n",
            time_result, size_result
    }
' "$result_csv"
printf "Databases kept at:\n  %s\n  %s\n" "$index_before" "$index_after"
