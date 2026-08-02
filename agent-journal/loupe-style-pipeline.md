# Campaign #63 — loupe-style-pipeline

Base: 0bd9b11894 (journal commit for URGENT.md #24-c1 item on
audit/disk-io; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/loupe-pipeline. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): four-stage pipeline on the fee-estimator lead — CONFIRMED + FIXED; estimator waste eliminated from zero-state IBD

### Draw
Random draw over the 57-goal pool (38 pending + 19 CYCLE-1; #24
excluded as just-cycled): raw=17607707707087155929, seed masked to 63
bits (8384335670232380121), index 18 -> #63. Lead taken from URGENT.md
(⚪ fee-estimator UpdateMovingAverages per-block cost, #22 c2 queue).

### Stage 1 — SCOUT (this agent, no fix yet)
Lead: TxConfirmStats::UpdateMovingAverages = 21.7% of post-bloom-fix
regtest IBD CPU (#22 c2 perf). Scout read: processBlock
(block_policy_estimator.cpp:730) has no is_ibd gate (unlike the bloom
reset family); call path ConnectTip -> removeForBlock
(validation.cpp:3144) -> signal (txmempool.cpp:461) -> processBlock,
all unconditional. Scout hypothesis: with zero tracked state the
bucket sweeps are pure waste (decay of zeros is zero).

### Stage 2 — VERIFIER (independent subagent, clean HEAD, no fix seen)
- CONFIRMED claim 1: no gate anywhere; upstream master structurally
  identical (fetched raw).
- CONFIRMED claim 2 (reproduced): UpdateMovingAverages = 20.43% of
  1279 samples (perf -F 199 -g, two-node regtest IBD, 20000 empty
  blocks, b-scheduler); processBlock total 20.93%; ClearCurrent 0.41%.
- REFINED claim 3: naive skip-if-empty predicate is UNSAFE —
  removeTx()'s failAvg branch can dirty stats without
  firstRecordedHeight, and Read() restores nonzero averages with
  firstRecordedHeight==0. Exact-skip requires an explicit dirty flag
  cleared by Record(), the removeTx failAvg branch, and Read().
- Bonus: fork's own estimator hardening (6 commits) reviewed;
  debunked a small-sample NodeClock artifact; noted perf host had
  auto-lowered perf_event_max_sample_rate to 1 (fixed, documented).

### Stage 3 — FIXER (minimal, behavior-identical by construction)
TxConfirmStats::m_all_zero (init true): cleared in Record() (after
the blocksToConfirm<1 early-return), in the removeTx failAvg branch,
and conservatively at the end of Read(); UpdateMovingAverages()
returns early when set. Decay of all-zero doubles is bit-identical
zero, so no observable state changes (nBestSeenHeight, ClearCurrent,
processBlockTx, firstRecordedHeight, persistence all untouched).
19 insertions, 1 file. Fixes the fork's mission fit: the ~38.4k
double multiplies per block x3 stats are skipped until the first
relevant datum exists (all of IBD for a fresh node; the estimator
tracks only mempool-seen transactions).

### Stage 4 — REVIEWER (rerun failing-before/passing-after)
- Before (verifier, clean HEAD): UpdateMovingAverages 20.43% of IBD
  samples; user CPU 1.31-1.37s (5000-block runs, #22 c2 lineage).
- After (this branch, patched): UpdateMovingAverages 0 samples (below
  report floor) in the same 5000-block P2P IBD; top symbol is BIP324
  sha512 7.15%; user CPU 0.89s / wall 1.63s (-34% user vs the
  1.31-1.37s post-bloom baseline); client tip == server tip
  (4b4f49d6...e437).
- Functional control: feature_fee_estimation.py (real estimates with
  real transactions) — TESTS SUCCESSFUL.
- No dedicated estimator unit suite exists (functional coverage only)
  — recorded as a test-gap note, not expanded this cycle.

### Verdict
- CONFIRMED + FIXED (performance): zero-state per-block estimator
  waste eliminated with a provably behavior-identical fast path;
  measured -34% user CPU on the 5000-block regtest IBD profile, 0
  residual UpdateMovingAverages samples.
- Master-relative severity: low (test-network/young-chain IBD CPU;
  mainnet IBD pays the same waste until the first mempool-relevant
  block but the absolute cost is small). Not correctness.

### Exact commands
- verifier subagent report (agent-0): code + upstream raw fetch +
  two-node IBD perf (vfy63_* scratch)
- fix: edit src/policy/fees/block_policy_estimator.cpp (m_all_zero)
- build: ninja -C build-before bin/test_bitcoin bin/bitcoind
- profile: server 5000 empty blocks; client perf record -F 199 -g
  -connect=127.0.0.1:28971 -listen=0 -checkblockindex=0
- control: python3 test/functional/feature_fee_estimation.py
  --configfile=build-before/test/config.ini --tmpdir=/tmp/btc63_func

### Limitations / queue
- ClearCurrent not gated (0.41% — not worth the map-emptiness
  invariant analysis per verifier).
- The verifier's FlatFileSeq::Open 12.79% (per-block file-handle
  churn) observation goes to the #24 queue.
- generatetoaddress throughput decay (108 -> 19 blocks/s by height
  20000, wallet-growth vs write path) unpinned — queued as a scout
  lead for #21/#24.
- No estimator unit suite (coverage via functional tests only) —
  queued as a #19/#9 test-gap item.

## Rotation note
Cycle 1 complete with a confirmed fix; rotating per uber-goal policy.
Not exhausted (pipeline composition pass over accumulated findings
queued).

## Cycle 4 (2026-07-30): FlatFileSeq::Open churn MEASURED — 20.7% inclusive (13.2% genuine page IO + ~7.5% fopen machinery); design cost, not a defect

### Draw
Re-harvested-queue draw (seed_raw=3781142207025897645, masked
same, n=5, idx=0) -> flatfileseq-open-churn -> #63 (fourth cycle;
c1 queue cell "the verifier's FlatFileSeq::Open 12.79% observation
goes to the #24 queue" — #24 since COMPLETE; the measurement lands
here). Branch: audit/loupe-pipeline-c4 from 7f7fac141b (#41 c3
journal tip).

### Hypothesis
Per-block file-handle churn (a fresh FILE* per block read) could be
the dominant non-validation cost in read-heavy block workloads —
with a handle cache as the obvious repair.

### Mechanism map
FlatFileSeq (src/flatfile.h:41-96) is minimal: Open returns a new
FILE* per call (fopen rb/rb+, fseek to pos), callers close per
read; ReadRawBlock -> OpenBlockFile per block (blockstorage.cpp
:1138-1151), AutoFile RAII closes at scope end. Upstream-identical
design (open-per-read).

### Experiment
2001-block regtest chain, stop, then
strace -c -f -e trace=openat,close + perf record -F 199 -g on
bitcoind -reindex-chainstate (reads every block):
- strace: 3,527 openat (104 errors) / 3,521 close, 0.0745 s total
  syscall time (12 us openat, 8 us close) over the ~36 s reindex —
  ~0.2% syscall share; every handle closed (no leak).
- perf: FlatFileSeq::Open = 20.71% INCLUSIVE of the reindex CPU,
  decomposed as fseek -> _IO_file_seekoff -> read 13.18% (first-
  touch page fetches — genuine IO a handle cache cannot remove)
  plus ~7.5% fopen/fopen_internal machinery (the avoidable churn).
  The verifier's 12.79% observation reproduces within this family.

### Verdict
DISMISSED (characterized, design cost): the churn is real (one
fopen+fclose per block read) but the measured AVOIDABLE share is
~7.5% of this workload's CPU; the dominant 13% is genuine
first-touch page IO. A handle cache would also drop most fseeks
on sequential reads but not the page fetches; upstream-identical
behavior, not a defect, not queued for local repair. If an
upstream handle-cache discussion ever opens, the measured numbers
here (20.7% inclusive, 7.5% avoidable, 3527 opens / 2001 blocks)
are the reference.

### Exact commands
- setup: createwallet + generatetoaddress 2001 (16 s, ~125 blk/s)
- strace -c -f -e trace=openat,close -o /tmp/ffs_strace.txt --
  perf record -F 199 -g -o /tmp/ffs_reindex.perf --
  bitcoind -regtest -datadir=/tmp/ffs_root/node0 -reindex-chainstate
- perf report --stdio [--no-children] (numbers above)

### Limitations / queue
- Empty regtest blocks: validation cost is minimal here, so the
  open share is INFLATED relative to a mainnet-scale reindex
  (where validation dominates); the 7.5% avoidable figure is the
  upper bound, the composition (fopen vs fseek/read) is the
  transferable part.
- generatetoaddress throughput decay (108 -> 19 blocks/s by
  height 20000) remains the sibling unpinned observation.

## Rotation note
Four cycles; estimator waste (fixed), churn (characterized).

## Cycle 5 (2026-07-30): generatetoaddress decay attribution — NOT reproduced: both curves FLAT to 20k; wallet = constant ~2x factor; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=5564503156500269060, masked
same, n=4, idx=0) -> generatetoaddress-decay -> #63 (fifth cycle;
c1 queue cell "generatetoaddress throughput decay (108 -> 19
blocks/s by height 20000, wallet-growth vs write path) unpinned").
Branch: audit/loupe-pipeline-c5 from 94e708cd8c (#63 c4 tip).

### Hypothesis
The loupe observation's decay could be wallet-growth (per-block
wallet scan cost rising with tx history) or write-path growth
(block-file/chainstate costs rising with height).

### Experiment (driver /tmp/gta_decay.py; two sequential regtest
nodes, 20 chunks x 1000 blocks each, empty blocks)
- A (wallet-full, uses_wallet=True): generates to its own wallet
  address; the wallet sees every coinbase (~20k wallet txs by
  20k).
- B (wallet-less): same address string, no wallet loaded — the
  write path alone.
- Harness lessons: this fork's TestNode defaults
  uses_wallet=False (-disablewallet); generatetoaddress needs
  called_by_framework=True.

### Results (blk/s per 1000-block chunk)
- A (wallet-full): 1243.2 @ 1k -> 1267.9 @ 20k — FLAT
  (range 1243-1314).
- B (wallet-less): 2617.5 @ 1k -> 2568.6 @ 20k — FLAT
  (range 2527-2617).
- Wallet factor: ~2.02x constant throughput cost (block scanning
  is per-block constant, NOT history-dependent — the growing
  20k-tx wallet does NOT slow it down).

### Verdict
DISMISSED (not reproduced / pinned): the decay does not exist
under this shape — both curves are flat to height 20000, and a
growing 20k-transaction wallet does not degrade throughput. The
loupe observation (108 -> 19 blk/s) is environment/shape-specific
(the author's environment/block content), not an inherent node
behavior at this scale; the transferable facts are the ~2x
constant wallet factor and the flat curves. Wallet-growth is
excluded as the mechanism by direct A/B; write-path growth is
excluded by the flat wallet-less curve.

### Exact commands
- python3 /tmp/gta_decay.py (full chunk table in the log above).

### Limitations / queue
- Empty blocks: tx-bearing blocks change the per-block work
  profile (validation/UTXO growth) — the loupe environment may
  have had that shape; not re-tested here (the queue question was
  wallet-vs-write-path, both excluded).
- banlist.dat archaeology remains the last family artifact.

## Rotation note
Five cycles; estimator waste fixed, churn characterized, decay
pinned (absent under this shape).

## Cycle 6 (2026-08-02, draw 195, raw=13100020088567363976, masked 3876648051712588168, idx 28/38): banlist.dat archaeology — legacy/corrupt/missing are three DISTINCT loud paths; corrupt arm fault-injected live; DISMISSED

### Map (addrdb.cpp CBanDB::Read :155-183 + banman.cpp LoadBanlist :31-48)
- Legacy banlist.dat (pre-23 format) present: explicit WARNING
  'banlist.dat ignored ... can only be read by version 22.x'
  (the #41 c6 contract) — loud, no silent skip.
- banlist.json missing: Read=false -> INFO 'Recreating the
  banlist database' (fresh-start line; INFO not warning — the
  file is expected-absent on first run).
- banlist.json corrupt JSON: LogWarning 'Cannot load banlist ...
  does not contain valid JSON ... may be caused by a crash,
  power loss, full disk, or storage error ... fixed by removing
  the file' — DISTINCT from fresh-start, names the remediation.
- Valid-JSON-bad-content: runtime_error -> LogWarning 'Cannot
  parse banlist ...' — third distinct line.
- Bans are transient by design (default 24h), so the recreate-
  on-failure data loss is bounded; upstream-identical paths.

### Live fault injection (/tmp/btc63c6)
Garbage banlist.json -> node start: [warning] 'Cannot load
banlist ... does not contain valid JSON ...' followed by
'Recreating the banlist database' — exact lines above; node
starts clean. Daemon stopped after.

### Verdict
DISMISSED: no silent banlist loss class — corruption is warned
loudly with remediation text, the legacy file has its own
warning, and only the expected-absent case is INFO-level. The
family's last artifact is closed.

### Exact commands
- sed/grep line refs above; bitcoind -datadir=/tmp/btc63c6 run
  (log lines above); bitcoin-cli stop.

### Limitations / queue
- Settings-file warning text is shared with other settings.json
  consumers (same loudness class, fine).
- #63 artifacts closed: estimator waste fixed, churn
  characterized, decay pinned, banlist mapped.
