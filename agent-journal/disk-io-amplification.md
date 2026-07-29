# Campaign #24 — disk-io-amplification

Base: 889d13d80c (journal commit for URGENT.md #67-c1 item on
audit/release-diff; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/disk-io. Start state: clean (untracked
scratch only).

## Cycle 1 (2026-07-28): regtest IBD write composition — ~2.0x byte amplification, 8.5x space preallocation; no pathological growth

### Draw
Random draw over the 58-goal pool (39 pending + 19 CYCLE-1; #67
excluded as just-cycled): raw=15028562080738234013, seed masked to
63 bits (5805190043883458205), index 1 -> #24.

### Method
Two cells on build-before (HEAD 889d13d80c), regtest, isolated
datadirs, fixed -connect peer, /proc/<pid>/io rchar/wchar (logical
bytes; read_bytes/write_bytes are useless here — buffered writes
never hit them). Sampling via the node's own -pid file (pgrep races
lost the pid twice — recorded as harness lesson).

### Cell 1: 1000 empty blocks
- blocks dir: 17.8MB = 16MB blk00000.dat posix_fallocate
  preallocation + ~0.29MB block data + 1MB rev00000.dat prealloc
  + undo + ~0.4MB index.
- Space amplification: 17.8MB / 0.29MB payload ≈ 61x on tiny regtest
  chains — bounded by design (files fill to 128MB on mainnet;
  preallocation is 16MB chunks).
- fs_helpers.cpp:233 fallback zero-fill TODO: MOOT on Linux
  (posix_fallocate path taken; fallback matters only without it).

### Cell 2: 310 blocks x 100 P2PK self-transfers (~20k txs)
- Server: blocks 17.88MB, chainstate 218.8KB (pre-shutdown, un-
  compacted). Client after sync + clean shutdown: blocks 17.87MB,
  chainstate 29.4KB (UTXO set stays ~100-300 entries — self-transfers
  consolidate).
- wchar during sync: 2,498 -> 3,596,728 = 3.59MB logical writes.
  rchar 0 -> 1.24MB.
- Composition: ~1.87MB block data + ~1MB undo + ~0.4MB index +
  ~0.03MB chainstate + WAL residue.
- Byte-write amplification: 3.59MB / 1.87MB payload ≈ 1.9-2.0x,
  dominated by undo data (spend-heavy blocks) and the block index.
  Space amplification at this scale: 17.87MB / 1.87MB ≈ 9.6x
  (preallocation again).

### Verdict
- DISMISSED: no pathological write amplification on the measured
  paths. ~2x byte amplification (undo+index+WAL) and bounded
  preallocation are both intended design; chainstate growth is
  minimal when the UTXO set stays small, and LevelDB compaction is
  clean at shutdown (218.8KB -> 29.4KB for the same logical set).
- Harness lessons recorded: (1) read_bytes/write_bytes in /proc/io
  miss buffered writes — use rchar/wchar; (2) pgrep-by-cmdline races
  the daemon — sample via -pid file; (3) MiniWallet cluster limit:
  100 intra-round chained self-transfers hit too-large-cluster —
  tolerate-or-break (this fork's cluster policy works as intended).

### Exact commands
- /tmp/btc24_run.sh (empty-chain cell), /tmp/btc24_mw.py (tx-heavy
  server chain, MiniWallet RAW_P2PK, PortSeed 317, rpcport 19804)
- sync + sample: bitcoind -connect=127.0.0.1:28811 -pid=<path> -daemon;
  grep rchar/wchar /proc/$(cat pid)/io before/after; du -sb
  blocks chainstate blocks/index

### Limitations / queue
- Pruning mode (undo retention, blk file rotation/delete) unmeasured —
  queued.
- LevelDB WAL vs SST split over a longer tx-heavy run (compaction
  pressure) queued — needs a UTXO-GROWING chain (fan-out outputs,
  not self-transfers) to stress the coins DB.
- dbcache flush cadence (periodic vs shutdown) not isolated.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
