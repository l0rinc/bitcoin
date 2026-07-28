# Campaign #21 — rebuild-recovery-profile

Base: audit/resurrection @ 26eaf1e179 (rotation ledger commit for #17).
Branch: audit/rebuild-recovery-profile. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): regtest reindex dominated by consistency checks — profiled + hypothesis confirmed

### Workload / environment (recorded per campaign)
- Workload: `-reindex` over a 5000-block empty (coinbase-only) regtest
  chain, 24 MiB datadir, page-cache resident.
- Host: Cortex-A76 (4 cores), 15GB RAM, NVMe (99% full — scratch kept tiny),
  kernel perf 6.8.12, perf_event_paranoid=4 (user-space sampling OK).
- Build: build-before, gcc, CMAKE_BUILD_TYPE=Release (asserts enabled;
  this tree does not pass -DNDEBUG).
- Data prep: fresh datadir, `generatetoaddress 5000` (deterministic empty
  blocks), stop. Stop condition per run: getblockchaininfo = 5000 blocks,
  initialblockdownload=false, then bitcoin-cli stop. /usr/bin/time -v on a
  foreground bitcoind (daemon mode defeats measurement — noted).

### Baseline (3 identical runs)
| run | wall | user | CPU% | peak RSS |
|---|---|---|---|---|
| base1 | 13.73s | 11.98s | 89% | 61 MB |
| base2 | 13.87s | 12.04s | 88% | — |
| base3 | 14.18s | 11.98s | 86% | — |
Median wall 13.87s (spread ±3%); ~2.7 ms/block. Classification: CPU-bound
(user 12.0s vs sys 0.31s), single dominant thread (b-initload).

### Profile attribution (perf record -F 199 -g, b-initload thread)
- 61.95% base_uint<256u>::CompareTo (self)
- 17.86% ChainstateManager::CheckBlockIndex (self)
- 11.24% CBlockIndexWorkComparator::operator()
- ~92% total in block-index consistency/work-comparison machinery;
  sha512 0.59%, sha256 0.08%, LevelDB/obfuscation/memset all <1%.
  I/O, serialization, crypto: negligible.

### Hypothesis and test
H: the wall time is ~all startup block-index consistency machinery, which
runs by default on regtest (`DefaultConsistencyChecks()` true on regtest,
false on mainnet; init.cpp:669), NOT the reindex work itself.
Test: identical rerun with `-checkblockindex=0`, expecting a large wall drop.

| run | wall | user |
|---|---|---|
| nocbi1 | 2.23s | 0.56s |
| nocbi2 | 2.23s | 0.50s |

Movement: wall 13.87 -> 2.23s (6.2x), user 12.0 -> 0.5s (24x). CONFIRMED,
reproducible. Correctness: identical final state (5000 blocks, IBD done,
same chain/datadir) in both modes; the consistency check is a read-only
audit, so skipping it cannot alter the rebuilt index.

### Verdict
- Profile hypothesis CONFIRMED with reproducible metric win
  (configuration-level). No code defect: the consistency check is a
  deliberate regtest default; mainnet default is off.
- Noteworthy magnitude: ~2.4 ms/block of pure index-invariant comparison
  on a perfectly linear chain (arith_uint256 work comparisons dominate) —
  relevant context for any future CheckBlockIndex optimization work
  (e.g. mainnet -checkblockindex=1 runs at 880k+ blocks).

### Exact commands
- prep: `bitcoind -regtest -datadir=/tmp/btc_r21 -daemon -server
  -fallbackfee=0.00001 -dbcache=300`; `bitcoin-cli ... generatetoaddress 5000`
- measure: `/usr/bin/time -v bitcoind -regtest -datadir=... -reindex
  [-checkblockindex=0]` (foreground) + CLI poll to 5000/False + stop
- profile: `perf record -F 199 -g -o /tmp/r21.perf bitcoind ... -reindex`;
  `perf report --stdio --no-children`

### Limitations / leads
- Empty blocks: tx validation, script checks, and UTXO-cache pressure are
  absent by design; this profiles the block-index/chainstate-rebuild path
  only. A tx-heavy chain (or mainnet replay) is a different workload.
- Phase attribution inside the residual 2.23s not split (file read vs
  LevelDB batch write) — small; queued.
- Reproducibility under 3 runs only; no cross-host comparison.

### Next queue for this campaign
- tx-heavy regtest chain reindex (e.g. 2000 blocks × 500 txs): re-run the
  same protocol; expect LevelDB/coins shares to grow.
- Wallet rescan profile (rescanblockchain over the prepared chain).
- assumeutxo snapshot load profile (loadtxoutset) if a scratch snapshot
  can be produced within disk budget.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-28): tx-heavy -reindex-chainstate — secp256k1 ECDSA dominates; script-check parallelism visible; consistency checks negligible at 610 blocks

Base: e56b843bfd (journal commit for #76 cycle-2 on audit/repro-c2;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/rebuild-recovery-c2 (c1 journal carried in the carry
commit). Start state: clean (untracked scratch only).

### Draw
Random draw over the 54-goal pool (33 pending + 21 CYCLE-1 after
POOL-REPAIR 2; #76 excluded as just-cycled): raw=9147548957504681430,
index 36 -> #21. This was also re-rank queue item 1 ("tx-heavy
reindex").

### Workload
610-block regtest chain: 210 MiniWallet funding blocks (RAW_P2PK,
real ECDSA signatures) + 400 blocks x 100 self-transfer txs (1 input
each) + coinbases = ~40,400 non-coinbase txs, ~40,400 CHECKSIG
operations. Chain built in 8.6 min via a standalone MiniWallet script
(/tmp/btc21_mw.py).

### Harness lessons (recorded; they invalidated 5 earlier attempts)
1. Plain -reindex on a synced datadir is block-INDEX-only — cheap and
   NOT revalidation. The validation cell needs -reindex-chainstate.
2. Completion detection must not poll state that is true PRE-wipe:
   getblockchaininfo=610/False matches before the wipe starts, so a
   naive poll stops the node instantly (three runs measured startup,
   not revalidation). getblockcount does not reliably dip either
   (revalidation of 610 blocks takes ~7s wall). Correct gate:
   log-based ('Wiping LevelDB' -> 'UpdateTip.*height=610 version'),
   and note the SOURCE datadir's stale debug.log already contains
   height=610 lines (count NEW lines or gate on the wipe marker
   first; my final runs used sleep-gated foreground stops and
   /usr/bin/time user-time as the gate-independent metric).
3. Wallet-RPC tx generation is ~0.1-26 tx/s with silent failure modes
   (fresh-regtest fee estimation REQUIRES -fallbackfee; avoid_reuse
   against a single destination stalls). MiniWallet
   (test/functional/test_framework/wallet.py) does ~80 tx/s with no
   wallet DB; direct use of this fork's framework needs: pathlib
   datadir, Binaries(paths=...), PortSeed.n set, explicit -regtest in
   extra_args, explicit -rpcport matching rpc_port(0), and
   called_by_framework=True through generate() (fork-added mining-RPC
   guard), and get_utxos(..., mark_as_spent=False) for probes.

### Results (-reindex-chainstate, -checkblockindex=0, default dbcache)
| run | user | note |
|---|---|---|
| manual | 6.75s | wall 8.1s (sleep-gated stop) |
| mw1/mw2 | 6.85/6.82s | user valid; wall polluted by blind-window gate |
| rep1/rep2 | 6.78/6.77s | user valid |
| checks-on | 6.78s | same CPU; consistency checks add nothing measurable at 610 blocks |
Reproducible: 6.75-6.85s user across 6 runs. ~0.17 ms CPU per tx.

### Profile attribution (perf record -F 199 -g, whole process)
- ~85%+ in secp256k1 EC math: gej_double 37.3%, gej_add_ge_var 25.7%,
  fe_sqrt 12.8%, gej_add_zinv_var 7.5% (all four threads: b-initload
  + b-scriptch.00/01/02 — script-check parallelism working as
  designed; the three worker threads show similar shares each).
- Everything else below the 2% print limit (LevelDB, serialization,
  hashing, coins cache all minor at this scale).
c1's empty-block profile (92% block-index consistency machinery) is
fully inverted: with real transactions, signature verification IS the
reindex cost, and CheckBlockIndex (610 blocks) is noise — c1's
domination is an empty-chain/large-n artifact, not a general property
(compare: c1 5000 empty blocks = 13.9s checks-on vs 2.2s off; here
610 tx-heavy blocks = 6.78s user either way).

### Verdict
- CONFIRMED (profile shape, as queued): tx-heavy reindex CPU is
  ECDSA-dominated; no new bottleneck found. The perf fix space is
  secp256k1 verification speed (already heavily optimized; batch
  verification not applicable to historical P2PK), not bookkeeping.
- No code defect. Harness lessons 1-3 are the durable yield.

### Exact commands
- /tmp/btc21_mw.py (MiniWallet chain build; framework API notes above)
- /usr/bin/time -v bitcoind -regtest -datadir=<copy> -reindex-chainstate
  [-checkblockindex=0] -rpcport=28513, sleep-gated stop
- perf record -F 199 -g on one revalidation; perf report --no-children
  --percent-limit=2 -g none

### Limitations / queue
- Wall-time series has gate overhead; user-CPU is the robust metric
  this cycle. A progress-series wall profile (blocks/s vs height)
  queued.
- 100-tx blocks all single-input P2PK; multi-input/SegWit/Taproot
  mixes change the EC/hash shares — queued.
- dbcache sensitivity (small vs default) untested — queued.
- All scratch removed (/tmp/btc21_{mw,dst,txsrc}, perf data);
  scripts kept in /tmp (btc21_mw.py, btc21_run.sh — log-gated version
  has the stale-log caveat documented above).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
