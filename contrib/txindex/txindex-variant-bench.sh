#!/usr/bin/env bash
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -euo pipefail

BASE_DIR="${BASE_DIR:-/mnt/my_storage}"
DATA_DIR="${DATA_DIR:-$BASE_DIR/BitcoinData}"
LOG_DIR_BASE="${LOG_DIR_BASE:-$BASE_DIR/logs/txindex-variants}"
CORPUS_DIR="${CORPUS_DIR:-$BASE_DIR/logs/txindex-lookup-corpus}"
BASE_COMMIT="${BASE_COMMIT:-e2dc767418bd2e34bfe41cf3d5e818788b17b35f}"

VARIANT_COMMITS=(
    "3e28ff7509674387509f04c884801379bb6a57c3" # 4-byte prefixes
    "7db35d54b729b4324b6ba266363810ac4868ef52" # 6-byte prefixes
    "d50ce012dcf425acec784988e1005bf4e29c7c4c" # cached next block sequence
    "4ba83b33ebb12389e87ed4c8f33b16d39c2f2449" # cached block sequence mappings
    "bf43e0535abbed652d2e12f4f18a371afa8fc199" # 16 KiB data blocks
    "9cd40a6f63e5cda774d464b1a3e7d5af01e17504" # no block cache
    "7db49be9c712a72efc18fda547c0a12ca0e0c9fd" # reallocated block cache
)
PREFIX_COMMITS=(
    "3e28ff7509674387509f04c884801379bb6a57c3"
    "7db35d54b729b4324b6ba266363810ac4868ef52"
)

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"
original_commit="$(git -C "$repo_root" rev-parse HEAD)"
original_branch="$(git -C "$repo_root" symbolic-ref --quiet --short HEAD || true)"

restore_checkout()
{
    if [[ -n "$original_branch" ]]; then
        git -C "$repo_root" checkout --quiet "$original_branch"
    else
        git -C "$repo_root" checkout --quiet --detach "$original_commit"
    fi
}
trap restore_checkout EXIT

run_benchmark()
{
    local variant_commit="$1"
    local benchmark="$2"
    local subject="$3"
    shift 3

    local variant_id="${variant_commit:0:10}"
    local variant_log_dir="$LOG_DIR_BASE/$variant_id"
    local output_log="$variant_log_dir/$benchmark-output.log"
    local stderr_log="$variant_log_dir/$benchmark-stderr.log"

    mkdir -p "$variant_log_dir"
    git -C "$repo_root" checkout --quiet "$variant_commit"
    if ! git -C "$repo_root" log -1 --format=%b --grep="^${subject}$" |
        env "$@" bash 2>"$stderr_log" |
        tee "$output_log"; then
        if [[ "$benchmark" == "rebuild" ]]; then
            tail -100 \
                "$variant_log_dir/txindex-build-before.log" \
                "$variant_log_dir/txindex-build-after.log" \
                "$stderr_log" 2>/dev/null || true
        else
            tail -100 "$stderr_log" >&2
        fi
        return 1
    fi
}

for variant_commit in "${VARIANT_COMMITS[@]}"; do
    variant_id="${variant_commit:0:10}"
    run_benchmark \
        "$variant_commit" \
        rebuild \
        "contrib: add txindex rebuild benchmark" \
        BASE_DIR="$BASE_DIR" \
        DATA_DIR="$DATA_DIR" \
        LOG_DIR="$LOG_DIR_BASE/$variant_id" \
        BENCH_ROOT="$BASE_DIR/txindex-bench-$variant_id" \
        BEFORE_COMMIT="$BASE_COMMIT" \
        AFTER_COMMIT="$variant_commit"
done

for variant_commit in "${VARIANT_COMMITS[@]}"; do
    variant_id="${variant_commit:0:10}"
    run_benchmark \
        "$variant_commit" \
        lookup \
        "contrib: add txindex lookup benchmark" \
        BASE_DIR="$BASE_DIR" \
        DATA_DIR="$DATA_DIR" \
        LOG_DIR="$LOG_DIR_BASE/$variant_id" \
        BENCH_ROOT="$BASE_DIR/txindex-bench-$variant_id" \
        CORPUS_DIR="$CORPUS_DIR"
done

for variant_commit in "${PREFIX_COMMITS[@]}"; do
    variant_id="${variant_commit:0:10}"
    run_benchmark \
        "$variant_commit" \
        collision \
        "contrib: add txindex collision benchmark" \
        BASE_DIR="$BASE_DIR" \
        DATA_DIR="$DATA_DIR" \
        LOG_DIR="$LOG_DIR_BASE/$variant_id" \
        BENCH_ROOT="$BASE_DIR/txindex-bench-$variant_id"
done

printf "\n=== all txindex variant results ===\n"
for variant_commit in "${VARIANT_COMMITS[@]}"; do
    variant_id="${variant_commit:0:10}"
    for benchmark in rebuild lookup; do
        output_log="$LOG_DIR_BASE/$variant_id/$benchmark-output.log"
        if [[ -f "$output_log" ]]; then
            sed -n "/^=== txindex $benchmark result ===/,\$p" "$output_log"
        fi
    done
done
for variant_commit in "${PREFIX_COMMITS[@]}"; do
    variant_id="${variant_commit:0:10}"
    output_log="$LOG_DIR_BASE/$variant_id/collision-output.log"
    if [[ -f "$output_log" ]]; then
        sed -n '/^=== txindex collision result ===/,$p' "$output_log"
    fi
done
