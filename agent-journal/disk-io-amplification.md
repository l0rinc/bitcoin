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

## Cycle 2 (2026-07-29): UTXO-GROWING fan-out chain — linear coins-DB growth, no superlinear write amplification; trace-log confound isolated

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=3510230010931241503, index 3 (of 5) -> #24 (second cycle; c1
queue cell "UTXO-GROWING chain (fan-out outputs)"). Branch:
audit/disk-io-c2 from 05eb46be09 (#60 c6 journal tip).

### Hypothesis
On a chain that GROWS the UTXO set every block (fan-out to
unspendable outputs), LevelDB/coins-DB write volume grows
superlinearly (WAL+compaction rewrites against an ever-larger DB) —
falsifiable via per-100-block write deltas: a rising slope confirms,
a constant slope dismisses.

### Setup
build-before HEAD-lineage node, regtest, isolated datadir
(/tmp/btc24_fan, PortSeed 319). 700 blocks x 8 txs x 20 outputs to a
FIXED external P2PK script (secp256k1 generator G — no known private
key, provably never re-spent => net UTXO growth) + change back to
MiniWallet (RAW_P2PK). Final txouts=112,810 (112,000 fan-out).
Sampling: /proc/<pid>/io rchar/wchar + du per 100 blocks; clean
shutdown; post-stop du.

### Results (wchar in bytes; chainstate/blocks du)
- start (h=110): wchar=682,837; chainstate=8,339; blocks=17,840,221
- per-100-block wchar deltas (MB): 5.86, 8.74, 7.07, 7.08, 5.95,
  7.10, 7.10 — CONSTANT slope while the UTXO set grew 16k -> 112k.
- per-100-block chainstate du deltas (KB): 1214, 691, 891, 891,
  1278, 891, 891 — linear ~8.9 KB/block (~160 new UTXOs x ~55 B).
- end (h=810): wchar=49,584,963 (delta 48.9 MB); chainstate
  6,757,655 (~60 B/UTXO); blocks 17,971,980 (payload fit the 16 MB
  blk prealloc; real block bytes ~6.1 MB); blocks/index 146,180.
- POST-STOP chainstate 6,758,045 (+390 bytes) — no shutdown rewrite
  storm; WAL fully checkpointed.
- LevelDB layout at end: 5 .ldb files + tiny live WAL; mid-run
  snapshot showed 2 compacted SSTs + rotating 51 KB WAL at h=393.

### Decomposition / amplification
- wchar delta 48.9 MB INCLUDES debug.log (framework default
  loglevel=trace): final debug.log 26.8 MB (~33 KB/block; estimated
  in-window share ~23 MB) — a harness artifact, not node behavior.
- Node-behavior writes ~25.8 MB vs logical payload ~14.7 MB
  (blocks 6.1 + undo 0.5 + index 0.1 + coins inserts ~8.0) =>
  ~1.8x amplification, consistent with c1's ~2x (undo-dominated
  there, coins-WAL/compaction here). Naive incl-logs figure is 3.3x.
- FALSIFIED: no rising slope, no quadratic LevelDB behavior; growth
  is linear in blocks and UTXOs, compaction healthy, shutdown clean.

### Verdict
DISMISSED: the UTXO-growing cell shows bounded linear write
amplification (~1.8x ex-logs), same design class as c1. No defect.
The interesting measurable is the HARNESS confound (below).

### Harness lessons (recorded)
1. tail-buffered background pipes hide intermediate SAMPLE output —
   sample the datadir independently or redirect with line buffering.
2. Framework default args run loglevel=trace; debug.log dominates
   wchar on long runs (26.8 MB vs 22 MB real) — subtract it or pass
   quiet args when wchar is the metric.
3. Standalone TestNode needs initialize_datadir() (writes
   bitcoin.conf with the matching rpcport + stderr/stdout dirs);
   an -rpcport extra_arg must equal rpc_port(index) for the chosen
   PortSeed — c1's 19804 worked only because PortSeed 317 maps to
   it (PortSeed 319 -> 19828). Three failed starts diagnosed:
   missing stderr/stdout dirs, then the port mismatch (node healthy,
   client ECONNREFUSED on the wrong port).
4. Pure-python sign_tx caps harness throughput ~0.4 blk/s at 8
   tx/block — the 1500-block plan was re-scoped to 700 (112k UTXOs
   still gave 5 SST files and a full compaction cycle).

### Exact commands
- python3 /tmp/btc24_fanout.py (this cycle's script; K=20, 8 tx/blk,
  G-point P2PK outputs, samples per 100 blocks)
- mid-run: du -sb chainstate blocks; grep rchar/wchar /proc/$pid/io;
  ls chainstate/*.ldb|*.log

### Limitations / queue
- Pruning-mode cell (undo retention, blk rotation/delete) still
  unmeasured — next cell if a cycle lands here.
- dbcache flush cadence not isolated (periodic vs shutdown).
- rchar stayed tiny (3.7 MB) — read-side not stressed by this cell.

## Rotation note
Two cycles; UTXO-growth cell closed. Not exhausted (pruning cell).

## Cycle 3 (2026-07-29): pruning-mode retention + rotation probes — machinery clean; below-prune invalidateblock behavior is upstream-identical design

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=16574797430435475050, masked 7351425393580699242, index 0
(of 2) -> #24 (third cycle; c2 queue cell "pruning-mode undo
retention / blk rotation"). Branch: audit/disk-io-c3 from
421f669e51 (#49 c5 journal tip).

### Cell 1: automatic trigger — DEFERRED-BY-SCALE (floor verified in code)
-prune=N has a 550 MiB MIN_DISK_SPACE_FOR_BLOCK_FILES floor with NO
-fastprune exemption (blockmanager_args.cpp:31 — byte-identical to
origin/master). At the measured ~17 KB/bulk-block, auto-triggering
needs ~33k blocks (~14 h) — out of cycle bounds. The -fastprune
help text ("lower minimum prune height") refers to the 16 KiB file
chunking, NOT the disk floor — mildly misleading, upstream-
identical, recorded only.

### Cell 2: manual pruning machinery (-prune=1 + pruneblockchain) — CLEAN
Setup: 510 bulk blocks (num_outputs=120 fan-out, ~16 KiB blk
files), then probes:
- pruneblockchain(200) -> 199 (FILE granularity; pruneheight=200;
  blk files 134 -> 104; getblock(199) "Block not available (pruned
  data)", getblock(200) served — boundary exact).
- pruneblockchain(400) -> 220: capped by MIN_BLOCKS_TO_KEEP=288
  (tip 510 - 288 = 222, file-rounded 220). Over-window
  pruneblockchain(410) silently no-ops at 220.
- invalidateblock within retained window (by 10): disconnect +
  reconsider green.
- Restart: pruneheight persists.

### Cell 3: below-prune invalidateblock (h=100 < pruneheight 221) — UPSTREAM DESIGN, not a defect
Observed: RPC returns SUCCESS; disconnects 249->221, then
DisconnectTip fails to read pruned block 220
("ReadRawBlock FlatFilePos(nFile=-1)" in debug.log), blocks
100-220 marked BLOCK_FAILED_VALID without disconnection, tip
settles at 220 (the first data-bearing block). Mechanism:
DisconnectTip's read failure returns false WITHOUT marking state
invalid (validation.cpp:3006-3008); InvalidateBlock's loop returns
false; the RPC wrapper (rpc/blockchain.cpp:1726-1745) ignores the
bool and keys only on state — state stays valid, ActivateBestChain
confirms the boundary, RPC returns VNULL. This is UPSTREAM-
IDENTICAL (wrapper byte-identical; the mark-invalid-immediately
design at validation.cpp:3706-3711 exists precisely so pruned
nodes can invalidateblock and still start). Recorded as a
behavioral characterization, not a finding: the RPC reports success
on a partially completed operation BY DESIGN (the alternative left
nodes unstartable).

### Harness lessons (recorded)
- RAW_P2PK + target_vsize = broken signatures (create_self_transfer
  pads OP_RETURN outputs AFTER the SIGHASH_ALL signature — NULLFAIL
  "Signature must be zero"); use num_outputs for bulk instead.
  (Most in-tree users are signature-less OP_TRUE mode, so the
  combination is untested upstream too — framework quirk, not a
  node bug.)
- Standalone TestNode PortSeed must match the one used at
  initialize_datadir time (rpcport is baked into bitcoin.conf).

### Exact commands
- python3 /tmp/btc24_prune.py (510-block builder, -fastprune
  -prune=550), /tmp/btc24_prune2.py (probes, -prune=1),
  /tmp/btc24_prune3.py + inline below-prune probe
- code: blockmanager_args.cpp:18-40, validation.cpp:3616-3735,
  validation.cpp:3000-3015, rpc/blockchain.cpp:1726-1745

### Verdict
DISMISSED: pruning retention/rotation/boundary behaviors are
correct and upstream-identical; the one surprising behavior
(below-prune invalidateblock "success") is deliberate upstream
design. No local defect. Automatic-threshold behavior (550 MiB
floor) deferred by scale with the floor code-verified.

### Limitations / queue
- dbcache flush cadence isolation still open (c1 queue).
- Crash-during-prune recovery (unlink vs index ordering) untested
  dynamically on this tree — see #49 c4's static analysis of the
  prune-assumevalid variant.

## Rotation note
Three cycles; pruning cell closed. Not exhausted (flush cadence).

## Cycle 4 (2026-07-29): dbcache flush cadence isolation — scantxoutset flushes every block in a MiniWallet harness; dbcache size irrelevant under per-block scans

### Draw
Re-rank singleton (last queue cell): #24 (fourth cycle; c1 queue
cell "dbcache flush cadence isolation"). Branch:
audit/disk-io-c4 from 6ae07306cb (#50 c7 journal tip).

### Experiment
Same 400-block fan-out chain (64k unspendable G-point outputs,
seeded random.seed(0x24) for txid determinism per #49 c5) on two
arms: -dbcache=450 (default) vs -dbcache=4 (pressure).
Oracle: flush-mode histogram from debug.log + final muhash +
wall time.

### Results
- IDENTICAL flush profiles on both arms: 405 FORCE_SYNC +
  2 FORCE_FLUSH; ZERO periodic flushes; ZERO "cache full" events.
- IDENTICAL outcomes: muhash a6f0dd3eff64cb46657585d0fc4e1e2abe5
  2e4d6ae43a5e2f12bdaa18a1a805e, txouts=64510, wall 538s vs 533s.
- Flush correctness across 405 forced flushes: confirmed (identical
  final UTXO set).

### Mechanism (the isolation answer)
MiniWallet.generate() rescans utxos after EVERY block; rescan calls
scantxoutset; scantxoutset does ForceFlushStateToDisk(false)
(rpc/blockchain.cpp:2446, flushing the coins cache before taking
the cursor). So a rescan-per-block harness forces a full coins-
cache flush per block; the cache never accumulates and -dbcache
size is irrelevant in this regime. Periodic flushes (50-70min
lineage interval) never fire in a 9-minute run; pressure flushes
never fire because each block flushes first.

### Verdict
DISMISSED (cadence question answered): in rescan-driving harnesses
the flush cadence is dominated by scantxoutset-driven FORCE_SYNC,
not by the dbcache size or the periodic schedule. Flush-state
correctness over 405 cycles confirmed identical. The scantxoutset
flush behavior is upstream code (adjacent to the 🔴 resize-race
item's lock region — noted as context, no new issue: the flush is
taken under cs_main BEFORE the cursor lock, the ordering the
e049f064e1 fix relies on).

### Exact commands
- python3 /tmp/btc24_fanout2.py 450 c450; ... 4 c4 (seeded variant
  of the c2 script; RESULT lines above)
- flush histogram: grep "Writing chainstate" debug.log |
  grep -oE "flush mode=.*" | sort | uniq -c
- rpc/blockchain.cpp:2430-2448 (scantxoutset flush site)

### Limitations / queue
- Pressure-flush dynamics (large=1/critical=1) need a no-rescan
  harness (custom utxo tracking without scantxoutset) — queued if
  a cycle lands here.
- #24's remaining cells: none open beyond this queue.

## Rotation note
Four cycles; cadence cell closed. #24 queue-empty.

## Cycle 5 (2026-07-29): cache-pressure flush dynamics — LARGE-tier flushes fire once the coins+mempool budget is actually clamped; state identical

### Draw
Re-rank draw over the remaining 4-cell queue:
raw=18394428872774206570, masked 9171056835919430762, index 2
(of 4) -> #24 (fifth cycle; c4 queue cell "pressure-flush with a
no-rescan harness"). Branch: audit/disk-io-c5 from ebf762be45
(#9 c5 journal tip).

### Harness (the c4 masking removed)
No-rescan fan-out: generatetodescriptor directly, manual utxo
list, direct sendrawtransaction — no scantxoutset, so no per-block
forced flush. Side observation: removing the rescan also cut the
400-block wall time 538s -> 38s (the per-block scantxoutset cost
~90% of the c4 runtime).

### Budget composition (why naive -dbcache=4 never pressures)
GetCoinsCacheSizeState (validation.cpp:2720-2737): nTotalSpace =
max_coins_cache_size_bytes + max(mempool_max - mempool_usage, 0)
— the coins cache borrows the mempool's headroom. With default
maxmempool=300, dbcache=4 gives ~304 MB effective budget: 400 and
800-block runs (64k/128k UTXOs) showed ZERO pressure flushes.
Clamping -maxmempool=5 as well (~9 MB budget) fires them.

### Results (800 blocks, 128,910 UTXOs, seeded-deterministic)
- control (-dbcache=450 -maxmempool=300): 2 FORCE_SYNC only.
- pressure (-dbcache=4 -maxmempool=5): 2 FORCE_SYNC + 3 PERIODIC
  flushes with large=1 (fCacheLarge: cache within 10%/10MiB of the
  budget — the "flush early while idle" tier; no critical tier
  reached).
- IDENTICAL outcomes: muhash 03201ac1cc1ee264757e13e1e2040c09a5
  472f2709d6292b4176689ca41800c1, txouts=128910, wall 76s both.
  Pressure flushing at this scale costs no measurable wall time.

### Verdict
DISMISSED: flush-pressure dynamics are correct and cheap; the
budget-composition rule (coins cache borrows mempool headroom) is
the missing piece for any future cache-pressure experiment.
Flush cadence never affects final state (three independent A/Bs
this campaign: c4 rescan regime, c5 no-rescan both budgets).

### Exact commands
- python3 /tmp/btc24_norescan.py 450 nr850b 800 300; ... 4 nr84b
  800 5 (seed 0x25; RESULT lines above)
- validation.cpp:2712-2737 (budget), :2800-2805 (LARGE/CRITICAL
  gates), :3036/:3133 (IF_NEEDED per-block call sites)

### Limitations / queue
- The CRITICAL tier (IF_NEEDED per-block, "must write now") still
  not directly observed — needs a single-block jump over the
  budget (huge tx flood), queued nicety.
- #24's cells: write composition, UTXO-growth, pruning, flush
  cadence+pressure all closed. QUEUE-EMPTY.

## Rotation note
Five cycles; pressure cell closed. Campaign queue-empty.

## Cycle 6 (2026-07-30): CRITICAL tier directly observed — single-block jump fires IF_NEEDED critical=1 flush; DISMISSED

### Draw
Rebuilt-queue draw (seed_raw=2063430805213321808, masked same,
n=3, idx=2) -> dbcache-pressure-critical -> #24 (sixth cycle; c5
queue cell "CRITICAL tier not directly observed"). Branch:
audit/disk-io-amplification-c6 from af63419c7b (#108 c5 tip).

### Hypothesis
A single block whose cache usage leapfrogs the LARGE band straight
over the CRITICAL threshold could expose a broken gate (no flush,
or flush next block), letting the coins cache overshoot the
configured budget.

### Budget math (verified in-tree)
-dbcache=4 -> block_tree 0.5 / coins_db 1.8 / coins tip 1.8 MiB
(kernel/caches.h:29-36); -maxmempool=5 honored (startup log "plus
up to 4.8 MiB of unused mempool space"). nTotalSpace = 1.8 +
max(0, 4.8 - mempoolUsage) (validation.cpp:2720-2736); LARGE =
max(0.9*total, total-10MiB) (validation.h:566-571). CRITICAL logs
"Cache size (%s) exceeds total space (%s)" at :2731; flushes log
mode+flags via BCLog::COINDB at :2810; empty_cache flushes call
CoinsTip().Flush(), periodic-only writes call .Sync() (:2847-2849).

### Experiment (driver /tmp/btc24_crit.py; TestNode, PortSeed 318)
Regtest node -dbcache=4 -maxmempool=5 -debug=coindb; approach: 5
blocks x 30 fan-out txs x 200 outputs (confirmed_only spends —
v1's unconfirmed spends died at the 64-cluster cap, silently
under-fueling the approach); jump: 1 block x 42 txs x 500 outputs
= 19,502 outputs in one block.

### Results
- Pre-jump: 24,410 outputs, est cache 3.34 MB (below LARGE ~5.2).
- JUMP block: cache logged 6,450,432 B vs total 5,785,120 B (total
  shrunk by the ~0.9 MB in-flight mempool, as predicted) ->
  EXACTLY ONE "exceeds total space" line (validation.cpp:2731),
  at the jump block.
- Same block: "Writing chainstate to disk: flush mode=IF_NEEDED,
  prune=0, large=0, critical=1" — the critical-tier flush fired
  immediately; chainstate du +928 kB (the fresh entries written).
- Node continued mining (tip advanced), shutdown clean.
- Accounted actual = 146.9 B/entry (6,450,432/43,912) vs the c4
  steady-state model 136.7 — +7%, consistent with all-dirty flagged
  entries + growth-edge bucket overallocation; model direction
  consistent with c4 (no under-accounting).
- RSS unusable as a flush signal at this scale (+38 MB at jump,
  +114 MB after two EMPTY blocks — glibc arena retention from the
  21k-output tx deserialization swamps it; recorded as method
  guidance: use the coindb flush lines + chainstate du).

### Verdict
DISMISSED (defensive tier confirmed working): the CRITICAL gate
classifies correctly at a real single-block budget jump and the
IF_NEEDED critical=1 flush writes immediately — no overshoot, no
next-block delay, no broken-gate path. The c5 observation gap is
closed; #24 remains QUEUE-EMPTY.

### Exact commands
- python3 /tmp/btc24_crit.py (full output above); greps: 'exceeds
  total space' (1 hit), 'Writing chainstate to disk' (mode/critical
  flags); budget reads: kernel/caches.h:29-36, validation.cpp
  :2720-2736, :2798-2810, :2847-2849, validation.h:566-571.

### Limitations / queue
- The FORCE_SYNC lines in the tail are shutdown flushes (normal).
- v1 attempt's anomaly (cluster-throttled approach + RSS
  contamination) preserved above as method guidance; v1 datadir
  deleted, /tmp/btc24_crit.py is the v2 driver.

## Rotation note
Six cycles; every queued cell including the CRITICAL-tier nicety is
now closed with direct evidence. Campaign #24 COMPLETE.
