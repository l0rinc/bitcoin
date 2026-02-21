# Historical Reorg Proxy

`contrib/historical_reorg_proxy.py` runs a stale-first P2P proxy that can replay
historical stale blocks before serving canonical blocks from an upstream node.

This is intended for exercising reorg behavior on a target node during sync.

## Inputs

- Upstream RPC node: serves canonical headers/blocks (`getblockhash`,
  `getblockheader`, `getblock`).
- Optional upstream `.cookie` file: authenticate RPC without explicit
  `rpcuser`/`rpcpassword`.
- Optional stale block directory: raw files named `<height>-<hash>.bin` (the
  same format used by `bitcoin-data/stale-blocks`).
- Optional target RPC node: used for final UTXO snapshot output.
- Optional target `.cookie` file: authenticate target RPC without explicit
  credentials.
- Optional target `debug.log`: used to validate that served stale tips were
  later reorged out.

## Example

```bash
contrib/historical_reorg_proxy.py \
  --upstream-rpc=http://127.0.0.1:8332 \
  --upstream-cookie-file=/data/upstream/.cookie \
  --target-rpc=http://127.0.0.1:18443 \
  --target-cookie-file=/data/target/.cookie \
  --target-debug-log=/data/target/regtest/debug.log \
  --stale-blocks-dir=./stale-blocks/blocks \
  --listen-host=127.0.0.1 \
  --listen-port=8338 \
  --network=mainnet \
  --snapshot-height=900000 \
  --exit-after-snapshot
```

Start the target node with:

```bash
bitcoind ... -connect=127.0.0.1:8338
```

When `--target-rpc` is set, the proxy prints a final `gettxoutsetinfo`
(`hash_serialized_3`) snapshot on shutdown, and optionally after
`--snapshot-height`.

When `--target-debug-log` is set, the proxy also prints a reorg summary from
`UpdateTip` log lines:

- number of served stale blocks
- number that became tip
- number that were later reorged out
- hashes missing tip or reorg evidence

By default, missing reorg evidence causes a non-zero exit code. Use
`--allow-missing-reorgs` to print the summary without failing.

## Local 300k shallow run

```bash
BASE_DIR="/mnt/my_storage"
SRC_DIR="$BASE_DIR/bitcoin"
SRC_DATA_DIR="$BASE_DIR/BitcoinData"
TGT_DATA_DIR="$BASE_DIR/ShallowBitcoinData"
LOG_DIR="$BASE_DIR/logs"
STALE_DIR="/data/my_storage/stale-blocks/blocks"
```

Terminal 1 (upstream node, RPC with cookie auth):

```bash
"$SRC_DIR/build/bin/bitcoind" \
  -datadir="$SRC_DATA_DIR" \
  -server=1 \
  -rpcbind=127.0.0.1 \
  -rpcallowip=127.0.0.1 \
  -rpcport=19443 \
  -stopatheight=300000 \
  -debuglogfile="$LOG_DIR/upstream-300k.log"
```

Terminal 2 (proxy):

```bash
"$SRC_DIR/contrib/historical_reorg_proxy.py" \
  --upstream-rpc=http://127.0.0.1:19443 \
  --upstream-cookie-file="$SRC_DATA_DIR/.cookie" \
  --stale-blocks-dir="$STALE_DIR" \
  --target-debug-log="$LOG_DIR/shallow-300k.log" \
  --listen-host=127.0.0.1 \
  --listen-port=8338 \
  --network=mainnet
```

Terminal 3 (target shallow/pruned node under test, no `-server=1`):

```bash
"$SRC_DIR/build/bin/bitcoind" \
  -datadir="$TGT_DATA_DIR" \
  -prune=550 \
  -noconnect \
  -addnode=127.0.0.1:8338 \
  -listen=0 \
  -dnsseed=0 \
  -fixedseeds=0 \
  -discover=0 \
  -stopatheight=300000 \
  -debuglogfile="$LOG_DIR/shallow-300k.log"
```

After the target exits at height 300000, stop the proxy with `Ctrl-C` to print
the reorg validation summary.
