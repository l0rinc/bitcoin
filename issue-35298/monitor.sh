#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(git -C "$SCRIPT_DIR/.." rev-parse --show-toplevel 2>/dev/null || (cd "$SCRIPT_DIR/.." && pwd))"
DATADIR="${DATADIR:-/mnt/my_storage/BitcoinData}"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
RUN_BASE="${RUN_BASE:-$REPO_ROOT/issue-35298/runs}"
INTERVAL="${INTERVAL:-60}"
DURATION="${DURATION:-172800}"
DBCACHE="${DBCACHE:-800}"
MAXMEMPOOL="${MAXMEMPOOL:-100}"
TXINDEX="${TXINDEX:-0}"
EXTRA_BITCOIND_ARGS="${EXTRA_BITCOIND_ARGS:-}"
REQUIRE_CHAINSTATE="${REQUIRE_CHAINSTATE:-1}"
WAIT_UNTIL_SYNCED_BEFORE_TIMER="${WAIT_UNTIL_SYNCED_BEFORE_TIMER:-0}"
STOP_WHEN_SYNCED="${STOP_WHEN_SYNCED:-0}"

default_device() {
    local source parent
    source="$(findmnt -n -o SOURCE --target "$DATADIR" 2>/dev/null | head -n1 || true)"
    source="${source#/dev/}"
    if [[ -z "$source" ]]; then
        return 0
    fi
    parent="$(lsblk -no PKNAME "/dev/$source" 2>/dev/null | head -n1 || true)"
    printf '%s\n' "${parent:-$source}"
}

DEVICE="${DEVICE:-$(default_device)}"

BITCOIND="$BUILD_DIR/bin/bitcoind"
BITCOIN_CLI="$BUILD_DIR/bin/bitcoin-cli"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)"
RUN_DIR="$RUN_BASE/$RUN_ID"
LOGFILE="$RUN_DIR/debug.log"
PIDFILE="$RUN_DIR/bitcoind.pid"

mkdir -p "$RUN_DIR"

die() {
    echo "monitor.sh: $*" >&2
    exit 1
}

[[ -x "$BITCOIND" ]] || die "missing bitcoind at $BITCOIND"
[[ -x "$BITCOIN_CLI" ]] || die "missing bitcoin-cli at $BITCOIN_CLI"
if [[ "$REQUIRE_CHAINSTATE" != "0" && ! -d "$DATADIR/chainstate" ]]; then
    die "missing chainstate directory at $DATADIR/chainstate"
fi

if pgrep -a bitcoind >/dev/null; then
    die "a bitcoind process is already running; stop it or set up a separate datadir"
fi

sample_chainstate() {
    local out="$1"
    local ts
    ts="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    if [[ ! -d "$DATADIR/chainstate" ]]; then
        print_zero_chainstate "$out" "$ts"
        return 0
    fi
    find "$DATADIR/chainstate" -maxdepth 1 -type f \( -name '*.ldb' -o -name '*.sst' \) -printf '%s\n' |
        sort -n |
        python3 -c '
import sys
ts = sys.argv[1]
sizes = [int(line) for line in sys.stdin if line.strip()]
if not sizes:
    print(f"{ts}\t0\t0\t0\t0\t0\t0\t0\t0\t0")
    raise SystemExit
n = len(sizes)
small = sum(1 for size in sizes if size < 4 * 1024 * 1024)
large = sum(1 for size in sizes if size >= 24 * 1024 * 1024)
pick = lambda q: sizes[int((n + 1) * q) - 1]
print(f"{ts}\t{n}\t{sum(sizes)}\t{small}\t{large}\t{sizes[0]}\t{pick(0.50)}\t{pick(0.90)}\t{pick(0.99)}\t{sizes[-1]}")
' "$ts" >> "$out"
}

print_zero_chainstate() {
    local out="$1"
    local ts="$2"
    printf "%s\t0\t0\t0\t0\t0\t0\t0\t0\t0\n" "$ts" >> "$out"
}

sample_diskstats() {
    local out="$1"
    local ts
    ts="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    if [[ -z "$DEVICE" ]]; then
        printf "%s\tmissing_device\n" "$ts" >> "$out"
        return 0
    fi
    awk -v ts="$ts" -v dev="$DEVICE" '$3 == dev {print ts "\t" $0}' /proc/diskstats >> "$out"
}

sample_proc_io() {
    local out="$1"
    local pid="$2"
    local ts
    ts="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    if [[ -r "/proc/$pid/io" ]]; then
        awk -v ts="$ts" '
            BEGIN { printf "%s", ts }
            { gsub(":", "", $1); printf "\t%s=%s", $1, $2 }
            END { printf "\n" }' "/proc/$pid/io" >> "$out"
    else
        printf "%s\tmissing_proc_io=1\n" "$ts" >> "$out"
    fi
}

sample_rpc() {
    local out="$1"
    local ts
    ts="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    if "$BITCOIN_CLI" -datadir="$DATADIR" getblockchaininfo > "$RUN_DIR/.rpc.tmp" 2> "$RUN_DIR/.rpc.err"; then
        python3 -c 'import json,sys; print(json.dumps({"ts": sys.argv[1], "ok": True, "result": json.load(open(sys.argv[2]))}))' \
            "$ts" "$RUN_DIR/.rpc.tmp" >> "$out"
    else
        python3 -c 'import json,sys; print(json.dumps({"ts": sys.argv[1], "ok": False, "error": sys.argv[2]}))' \
            "$ts" "$(tr '\n' ' ' < "$RUN_DIR/.rpc.err")" >> "$out"
    fi
}

node_synced() {
    "$BITCOIN_CLI" -datadir="$DATADIR" getblockchaininfo > "$RUN_DIR/.sync.tmp" 2> "$RUN_DIR/.sync.err" &&
        python3 -c 'import json,sys; info=json.load(open(sys.argv[1])); sys.exit(0 if info.get("initialblockdownload") is False and info.get("blocks") == info.get("headers") else 1)' "$RUN_DIR/.sync.tmp"
}

stop_node() {
    if [[ -f "$PIDFILE" ]]; then
        local pid
        pid="$(cat "$PIDFILE" 2>/dev/null || true)"
        if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
            "$BITCOIN_CLI" -datadir="$DATADIR" stop > "$RUN_DIR/stop.out" 2> "$RUN_DIR/stop.err" || true
            for _ in $(seq 1 120); do
                kill -0 "$pid" 2>/dev/null || return 0
                sleep 1
            done
            echo "bitcoind did not stop within 120s" >> "$RUN_DIR/stop.err"
        fi
    fi
}

trap stop_node INT TERM

{
    echo "run_id=$RUN_ID"
    echo "datadir=$DATADIR"
    echo "build_dir=$BUILD_DIR"
    echo "device=$DEVICE"
    echo "interval=$INTERVAL"
    echo "duration=$DURATION"
    echo "dbcache=$DBCACHE"
    echo "maxmempool=$MAXMEMPOOL"
    echo "txindex=$TXINDEX"
    echo "extra_bitcoind_args=$EXTRA_BITCOIND_ARGS"
    echo "require_chainstate=$REQUIRE_CHAINSTATE"
    echo "wait_until_synced_before_timer=$WAIT_UNTIL_SYNCED_BEFORE_TIMER"
    echo "stop_when_synced=$STOP_WHEN_SYNCED"
    echo "start_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$RUN_DIR/run.env"

git -C "$REPO_ROOT" rev-parse HEAD > "$RUN_DIR/git-head.txt"
git -C "$REPO_ROOT" status --short > "$RUN_DIR/git-status.txt"
"$BITCOIND" --version > "$RUN_DIR/bitcoind-version.txt"
uname -a > "$RUN_DIR/uname.txt"
df -h "$DATADIR" > "$RUN_DIR/df-start.txt"
findmnt --target "$DATADIR" > "$RUN_DIR/findmnt.txt"
cp /proc/mounts "$RUN_DIR/proc-mounts.txt"

printf "ts\tfiles\tbytes\tsmall_lt4MiB\tlarge_ge24MiB\tmin\tp50\tp90\tp99\tmax\n" > "$RUN_DIR/chainstate.tsv"
printf "ts\traw_diskstats_fields\n" > "$RUN_DIR/diskstats.tsv"
printf "ts\tproc_io_fields\n" > "$RUN_DIR/proc_io.tsv"
: > "$RUN_DIR/blockchaininfo.jsonl"

sample_chainstate "$RUN_DIR/chainstate.tsv"
sample_diskstats "$RUN_DIR/diskstats.tsv"

CMD=(
    "$BITCOIND"
    -datadir="$DATADIR"
    -daemonwait
    -pid="$PIDFILE"
    -server=1
    -nosettings
    -txindex="$TXINDEX"
    -dbcache="$DBCACHE"
    -maxmempool="$MAXMEMPOOL"
    -debug=leveldb
    -debug=bench
    -debug=coindb
    -debuglogfile="$LOGFILE"
    -printtoconsole=0
)

if [[ -n "$EXTRA_BITCOIND_ARGS" ]]; then
    # shellcheck disable=SC2206
    EXTRA_ARGS=($EXTRA_BITCOIND_ARGS)
    CMD+=("${EXTRA_ARGS[@]}")
fi

printf '%q ' "${CMD[@]}" > "$RUN_DIR/start-command.sh"
printf '\n' >> "$RUN_DIR/start-command.sh"
"${CMD[@]}" > "$RUN_DIR/daemon.out" 2> "$RUN_DIR/daemon.err"

PID="$(cat "$PIDFILE")"
echo "$PID" > "$RUN_DIR/pid.txt"
sample_proc_io "$RUN_DIR/proc_io.tsv" "$PID"
sample_rpc "$RUN_DIR/blockchaininfo.jsonl"

if [[ "$WAIT_UNTIL_SYNCED_BEFORE_TIMER" != "0" ]]; then
    echo "waiting for steady state before starting duration at $(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "$RUN_DIR/monitor.log"
    until node_synced; do
        sleep "$INTERVAL"
        if ! kill -0 "$PID" 2>/dev/null; then
            echo "bitcoind exited while waiting for sync at $(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "$RUN_DIR/monitor.log"
            break
        fi
        sample_chainstate "$RUN_DIR/chainstate.tsv"
        sample_diskstats "$RUN_DIR/diskstats.tsv"
        sample_proc_io "$RUN_DIR/proc_io.tsv" "$PID"
        sample_rpc "$RUN_DIR/blockchaininfo.jsonl"
    done
    echo "steady-state timer starts at $(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "$RUN_DIR/monitor.log"
fi

end=$((SECONDS + DURATION))
while (( SECONDS < end )); do
    if [[ "$STOP_WHEN_SYNCED" != "0" ]] && node_synced; then
        echo "node synced at $(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "$RUN_DIR/monitor.log"
        break
    fi
    sleep "$INTERVAL"
    if ! kill -0 "$PID" 2>/dev/null; then
        echo "bitcoind exited at $(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "$RUN_DIR/monitor.log"
        break
    fi
    sample_chainstate "$RUN_DIR/chainstate.tsv"
    sample_diskstats "$RUN_DIR/diskstats.tsv"
    sample_proc_io "$RUN_DIR/proc_io.tsv" "$PID"
    sample_rpc "$RUN_DIR/blockchaininfo.jsonl"
done

stop_node
df -h "$DATADIR" > "$RUN_DIR/df-end.txt"
echo "end_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "$RUN_DIR/run.env"
