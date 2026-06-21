#!/usr/bin/env bash
set -euo pipefail

BTC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

UPSTREAM_DATADIR="${UPSTREAM_DATADIR:-/mnt/my_storage/BitcoinData}"
TARGET_DATADIR="${TARGET_DATADIR:-/mnt/my_storage/ShallowBitcoinDataLatest}"
STALE_BLOCKS_DIR="${STALE_BLOCKS_DIR:-/mnt/my_storage/stale-blocks/blocks}"

UPSTREAM_RPC_PORT="${UPSTREAM_RPC_PORT:-8332}"
PROXY_PORT="${PROXY_PORT:-8348}"
TARGET_RPC_PORT="${TARGET_RPC_PORT:-19454}"

WINDOW_NAME="${WINDOW_NAME:-stale-reorg}"
SESSION_NAME="${SESSION_NAME:-stale-reorg}"

COMPILE="${COMPILE:-1}"
CCACHE_DISABLE="${CCACHE_DISABLE:-1}"

if [[ "$COMPILE" == "1" ]]; then
  echo "[build] configuring + compiling for current commit..."
  (
    cd "$BTC_DIR" && \
    cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    CCACHE_DISABLE="$CCACHE_DISABLE" cmake --build build -j"$(nproc)" --target bitcoind bitcoin-cli
  )
fi

mkdir -p "$TARGET_DATADIR"

UPSTREAM_CMD="cd '$BTC_DIR' && build/bin/bitcoind -datadir='$UPSTREAM_DATADIR' -server=1 -noconnect -rpcport='$UPSTREAM_RPC_PORT' -listen=0 -printtoconsole=1"
PROXY_CMD="cd '$BTC_DIR' && python3 -u contrib/historical_reorg_proxy.py --upstream-rpc=http://127.0.0.1:$UPSTREAM_RPC_PORT --upstream-cookie-file='$UPSTREAM_DATADIR/.cookie' --target-rpc=http://127.0.0.1:$TARGET_RPC_PORT --target-cookie-file='$TARGET_DATADIR/.cookie' --stale-blocks-dir='$STALE_BLOCKS_DIR' --listen-port=$PROXY_PORT"
TARGET_CMD="cd '$BTC_DIR' && build/bin/bitcoind -datadir='$TARGET_DATADIR' -server=1 -rpcport='$TARGET_RPC_PORT' -prune=10000 -dbcache=2000 -assumevalid=0 -connect=127.0.0.1:$PROXY_PORT -listen=0 -printtoconsole=1"

if [[ -n "${TMUX:-}" ]]; then
  tmux new-window -n "$WINDOW_NAME" -c "$BTC_DIR"
  TARGET_WIN="{last}"
else
  tmux new-session -d -s "$SESSION_NAME" -n "$WINDOW_NAME" -c "$BTC_DIR"
  TARGET_WIN="$SESSION_NAME:0"
fi

tmux split-window -h -t "$TARGET_WIN"
tmux split-window -h -t "$TARGET_WIN"
tmux select-layout -t "$TARGET_WIN" even-horizontal

tmux send-keys -t "${TARGET_WIN}.0" "$UPSTREAM_CMD" C-m
tmux send-keys -t "${TARGET_WIN}.1" "$PROXY_CMD" C-m
tmux send-keys -t "${TARGET_WIN}.2" "$TARGET_CMD" C-m

echo "[ok] launched 3 panes:"
echo "  pane 0: upstream bitcoind"
echo "  pane 1: historical reorg proxy (auto UTXO compare)"
echo "  pane 2: target bitcoind (assumevalid=0, prune=10000, dbcache=2000)"

if [[ -z "${TMUX:-}" ]]; then
  exec tmux attach -t "$SESSION_NAME"
fi
