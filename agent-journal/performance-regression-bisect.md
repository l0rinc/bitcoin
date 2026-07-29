# Campaign #25 — performance-regression-bisect

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/perf-bisect. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): MemPoolAddTransactions +2.4% over the 2026-07-05..07 check series — bisected to 3ae78dbd25, cost explained, no smallest fix (journal-only)

### Draw
Random draw over the 55-goal eligible pool: raw=4370217508493066240,
index 10 -> #25.

### Range selection (justified)
Fork-authored check/assert series on txgraph/clusterlin hot paths
(2026-07-05..07, 13 commits) — the same class that already produced the
6df3c46012 32% finding in campaign #23 c1. Workload: bench_bitcoin
ComplexMemPool (clean) and MemPoolAddTransactions (regressed), the two
mempool add-path benchmarks. Host: Cortex-A76, build-before Release,
asserts on. Interleaved A/B runs throughout.

### Bisect log (exact)
- Endpoints: P=8382b2af8e (parent of series), T=ac3220508c (series tip).
  ComplexMemPool: P 266.4-269.2 vs T 268.8-271.4 ms — within noise.
  MemPoolAddTransactions (6 interleaved pairs):
  P median 246.5ms, T median 252.5ms => +2.4%; distributions separated.
- 5de135c09d (81-line diagram-contract commit, prime suspect a priori):
  S median 248.2 vs P 248.0 — CLEAN. Suspect rejected by measurement.
- Midpoint 228c637014 (rbf fee diagrams): M median 247.7 — CLEAN.
  => first half of series exonerated.
- 00e7618969: M2 median 247.0 — CLEAN.
- 3ae78dbd25 (collapse saturated disconnected chunks): M3 median 251.3,
  all 4 runs above all 4 M2 runs — REGRESSED.
- 4c3561205b (its parent): M4 median 248.4 — CLEAN.
- Adjacent confirmation M4 vs M3 (4 pairs): 249.3 vs 252.5, separated.
FIRST-BAD: 3ae78dbd25.

### Causal experiments (staged controls)
3ae78dbd25 routes 4 chunk consumers through a new GetChunking(), which
adds (a) one full-cluster IsConnected() walk and (b) per-chunk
IsConnected(chunk.transactions) walks per call, and switches the diagram
path from feerate-only ChunkLinearization to SetInfo-carrying
ChunkLinearizationInfo (bitset construction).
- Control 1 (saturation precheck gating (b), on tip): bench 251.6 vs
  ungated 251.6 — NO recovery. (b) is not the cost.
- Control 2 (control 1 + remove (a)): 250.2 vs 251.6 — ~0.6% recovered.
  (a) costs ~0.6-1%.
- Residual ~1.5% consistent with SetInfo/bitset construction replacing
  feerate-only computation in the diagram path — inherent to the crash
  fix's need for chunk.transactions in the connectivity check.
- Both controls REVERTED after measurement (no proven fix to commit).

### Verdict
- CONFIRMED regression (+2.4% on MemPoolAddTransactions), bisected to a
  single first-bad commit with adjacent-commit confirmation. The commit
  is a deliberate correctness fix (saturated-fee disconnected-chunk
  crash, fuzz-proven; see its message). Master-relative severity: none
  (fork-local commit; no upstream equivalent exists).
- No smallest fix committed: the obvious gates recovered <=1% and were
  reverted per campaign discipline (commit only trivial PROVEN measured
  fixes). The residual is the design cost of carrying chunk transaction
  sets for the connectivity contract.

### Why existing benchmarks missed it
Nobody A/B'd the check series; the cost is split across three small
mechanisms, each near noise individually.

### Limitations / leads
- Fix direction queued, not implemented: a feerate-only fast path for
  the diagram consumers when chunk sums cannot saturate (my gate's
  precheck makes the connectivity bitsets unnecessary exactly then);
  needs the diagram consumer split, ~30 lines, then re-bisect-proof.
- ComplexMemPool showed no regression; block-building paths
  (GetMainStagingDiagrams consumers) not separately profiled.
- Measurements on asserts-on Release (this tree's default); the Assume
  density makes absolute numbers unrepresentative of NDEBUG builds.

### Exact commands / artifacts
- builds: cmake --build build-before -j4 --target bench_bitcoin at each
  commit; binaries /tmp/bench_{P,S,M,M2,M3,M4,T,FIX,MUT}
- measurement: interleaved loops
  `for b in X Y; do /tmp/bench_$b -filter='^MemPoolAddTransactions$'`

### Next queue for this campaign
- The feerate-only fast path (above), with the same A/B harness.
- txindex hashed-keys range (35531 lineage) with an IBD-ish workload.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): regression re-verified at HEAD; consumer analysis caps the fast-path upside

### Draw
Random draw over the 5-goal eligible pool (4 pending + 1 CYCLE-1,
#44 excluded as just-cycled): raw=3460624873609219110, index 0 ->
#25 (second cycle). Branch: audit/perf-bisect-c2 from 523b55482c
(#44 c1 bookkeeping). Journal pulled forward from dd3941d62f (c1,
side branch only; ledger row restored).

### Re-verification at HEAD (no rebuild needed — bench binary current)
MemPoolAddTransactions at HEAD: 250.75 ms/op (err 0.2%) — matches
c1's regressed T median 252.5 within noise, clearly above the P
baseline 246.5. The +2.4% persists at HEAD (mechanism unchanged).

### New information: GetChunking consumer split caps the fix upside
The c1-queued "feerate-only fast path" assumed diagram consumers
could skip SetInfo/bitsets. Enumerating all four GetChunking
consumers (txgraph.cpp):
- :1470 AppendChunkFeerates — uses chunk.feerate ONLY. Feerate-only.
- :1175 Updated — chunk.feerate + transactions.Count() (Count is
  derivable without SetInfo, but the current shape reads SetInfo).
- :1485 AppendTrimData — iterates chunk bitsets (chunk[cluster_idx],
  chunk.Count()) — NEEDS SetInfo.
- :3007 sanity check — reads linchunking[chunk_num].transactions —
  NEEDS SetInfo (assert-builds only, but must keep working there).
So 2 of 4 consumers (incl. the always-on sanity path in assert
builds) require the bitsets regardless: a feerate-only variant would
still be constructed alongside SetInfo unless call sites are split
by need — a deeper change than c1's ~30-line estimate, and the win
is capped by AppendChunkFeerates' share of the workload. The
saturation DETECTION (IsConnected walks, ~0.6-1%) must also be kept
in any feerate path (it feeds the collapse decision), so the
addressable slice is only the SetInfo construction (~1.5%) times
the feerate-only consumer share.

### Verdict
Journal-only, per campaign discipline (commit only trivial PROVEN
measured fixes): the regression is real, located, and explained;
the remaining fix is non-trivial with a capped upside and a
correctness-sensitive saturation contract. NOT forced this cycle.

### Exact commands
- build-before/bin/bench_bitcoin --filter='^MemPoolAddTransactions$'
  --min-time=2500 -> 250,751,045 ns/op (HEAD, 2026-07-29)
- reads: txgraph.cpp:1139-1154 (GetChunking), consumers :1175,
  :1470, :1485, :3007

### Limitations / queue
- The A/B harness for a future fast-path attempt: c1's interleaved
  loop + this cycle's HEAD number as the control anchor.
- txindex hashed-keys IBD-ish workload cell (c1 queue) untouched.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 3 (2026-07-29): txindex in-tree baseline with an IBD-ish tx-heavy workload (35531 lineage reference)

### Draw
Re-rank draw over a rebuilt 10-cell queue:
raw=7506511565727394747, index 7 -> #25 (third cycle; c1 queue cell
"txindex hashed-keys range (35531 lineage) with an IBD-ish
workload"). Branch: audit/perf-bisect-c3 from ce2832dc3f (#50 c2
bookkeeping).

### Setup check
The fork does NOT carry the hashed-keys txindex in-tree
(src/index/txindex.cpp has no SipHash/prefix-key code; the design
lives on l0rinc/txindex_optimization and upstream PR 35531, open).
So the cell measures the IN-TREE baseline the design attacks.

### Measurement (regtest, MiniWallet OP_TRUE chain, 410 blocks /
~41.4k txs — the same harness as #21 c3)
- Chain build: 68s (no signatures).
- txindex catch-up from 0 to 410 with -txindex=1: 3.7s wall
  (~11.2k txs/s indexed — same ballpark as the validation path
  itself, ~11.3k txs/s from #21 c3).
- Index size: ~1-3 MB (du -sm floor 1; 32-byte txid key + position
  value per tx, upstream format).
- Consistency: #38 c2's ~3s empty-block interrupt-build bound.

### Verdict
Journal-only baseline: in-tree txindex costs ~25-70 bytes/tx and
runs at validation-path speed on this workload; the 35531 lineage
(12-byte key + empty value, verified in reviews/2026-07-25-
pr-35531-txindex-hashed-keys.md) targets a ~2.7x key shrink whose
mainnet-scale effect (~66GB -> ~26GB per the review) regtest
cannot validate. No defect; the baseline numbers are the durable
yield for future comparisons.

### Exact commands
- /tmp/btc25_mw.py (chain build, OP_TRUE mode)
- bitcoind -regtest -datadir=/tmp/btc25_mw -txindex=1; getindexinfo
  gating; du -sm .../indexes/txindex

### Limitations / queue
- No A/B against the branch (its 2025 base would need a separate
  build; the fork author's adoption/upstream-tracking decision).
- Lookup-path cost (FindTx collision walk) unmeasured — natural
  collisions are ~0 at 41k txs.
- Scratch chain removed (/tmp/btc25_mw).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
