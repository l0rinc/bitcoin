# Campaign #23 — perf-flamegraph-investigation

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/perf-flamegraph. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): ComplexMemPool — 32% is an ungated fork verification loop — CONFIRMED + FIXED (83f9989a68)

### Draw
Random draw over the 64-goal eligible pool: raw=1662843009503595786,
index 10 -> #23.

### Workload / environment
- Benchmark: ComplexMemPool (src/bench/mempool_stress.cpp:127): timed
  region = removeForBlock(10k txs) + re-add + UpdateTransactionsFromBlock
  (10k hashes) per iteration. Representative reorg/mempool hot path.
- Host: Cortex-A76, 4 cores, perf_event_paranoid=4 (user sampling OK).
  Build: build-before, Release, asserts enabled (no -DNDEBUG).
- Baseline (3 runs): 269.0 / 268.5 / 267.2 ms, err% <= 0.7, IPC ~0.95.

### Profile attribution (perf record -F 199 -g; dwarf retry)
Self-time leaders: memcmp 10.6-12.3% (txid comparisons in mapTx/
mapNextTx lookups), TxGraphImpl::CompareMainTransactions 5.3-6.0%,
sha512::Transform 4.1-5.3%, UpdateTransactionsFromBlock 4.2-4.6%,
sha512+CSHA256 ~7.7%, allocator ~11%, cluster_linearize machinery ~12%,
ChaCha20 (bench det_rand) 2.4%.

IMPORTANT ATTRIBUTION CAVEAT (method note for future cycles):
sha512::Transform is NOT hot-path cost. Static chase: the only mempool-
adjacent SHA512 users are RNGState::MixExtract (FastRandomContext
seeding) and mempool_stress.cpp:53 GetRandHash() — the latter runs in
the bench SETUP (50k tx creations outside bench.run). perf records the
whole process; nanobench total (3.01s) includes setup. Dwarf stacks
(--call-graph dwarf,16384) could not unwind this binary past leaf
frames; fp stacks attribute only ~0.6% to the RandomSeed path, so
sha512 self-time is largely setup artifact. Treat any whole-process
profile of nanobench binaries as setup-contaminated; next time gate
capture on the timed region (scratch harness or -D delay calibration).

### The defect (mechanism)
Fork commit 6df3c46012 ("mempool: check reorg dependency repair", a
fuzz-oracle addition by the campaign author) appended a verification
loop to CTxMemPool::UpdateTransactionsFromBlock (production reorg
repair path, called on every disconnect during reorgs): per surviving
child of each reintroduced tx, an unconditional
m_txgraph->GetAncestors(Level::MAIN) cluster walk, result checked by
Assume(). Unconditional = runs in every build, asserts or not;
O(B x cluster) per reorg block. Upstream master does not contain it
(git show origin/master:src/txmempool.cpp | grep -c = 0) — fork-local.

### Experiment (staged clean / mutation / repaired, order documented)
1. Baseline above (clean, loop active).
2. Mutation: loop commented out (scratch) -> 181.6 / 185.0 / 182.1 ms.
3. Repaired: loop gated `if constexpr (G_ABORT_ON_FAILED_ASSUME)` —
   the exact idiom the adjacent HasDescendants cross-check
   (txmempool.cpp, same file, ~10 lines below) already uses for the
   same class of verification -> 181.3 / 181.5 / 181.5 ms.
Gated == mutation within noise: the gate captures the entire cost.
-32.4% median (268.5 -> 181.4 ms) on the workload.

### Fix + verification
Commit 83f9989a68. Oracle preserved in fuzz builds (G_FUZZING_BUILD)
and ABORT_ON_FAILED_ASSUME builds; production cost removed.
- test_bitcoin --run_test=mempool_tests: No errors detected (includes
  UpdateTransactionsFromBlockRestoresChildAncestry — tests the repair
  behavior itself, not the Assume).
- test/functional/mempool_reorg.py: Tests successful.
- build_fuzz (BUILD_FOR_FUZZING=ON): txmempool.cpp.o rebuilt clean —
  the gated branch compiles where active. Byte-identical loop body, so
  the fuzz oracle is unchanged where enabled.

### Why existing benchmarks/tests missed it
ComplexMemPool exercises UpdateTransactionsFromBlock but nobody had
diffed against a loop-disabled control; unit/functional oracles assert
correctness, not cost.

### Verdict
- CONFIRMED performance defect (fork-local; master-relative severity:
  none for upstream, measurable reorg slowdown for the fork). FIXED
  with measured 32% workload win and full regression battery.

### Limitations
- Full fuzz-binary link + tx_pool run with the gate in place not done
  (build cost exceeds the bounded cycle); compile-level evidence only.
- memcmp/txid-comparison share (~11%) and TxGraph::CompareMainTransactions
  (~6%) remain the top production costs — hypotheses for future cycles,
  not trivial fixes (hashed-index equality design, cluster ordering).
- Whole-process profile setup contamination quantified only by static
  chase, not by a timed-region-gated capture.

### Exact commands / artifacts
- `perf record -F 199 -g -o /tmp/r23.perf ./build-before/bin/bench_bitcoin -filter='^ComplexMemPool$'`
- `perf record -F 199 --call-graph dwarf,16384 -o /tmp/r23d.perf ...`
- `perf report --stdio --no-children -i /tmp/r23{,d}.perf`
- artifacts: /tmp/r23.perf, /tmp/r23d.perf (regenerate if needed)

### Next queue for this campaign
- Timed-region-gated capture method (scratch driver binary running only
  bench.run bodies, or -D delay calibration) — fixes the attribution hole.
- EvictionProtection*/ConnectBlockAll profiles (different hot paths).
- TxGraph::CompareMainTransactions / memcmp share: rank fix hypotheses.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): c1 stack backport + independent re-verification (−32% confirmed at HEAD)

### Draw
Random draw over the 11-goal eligible pool (10 pending + 1 CYCLE-1,
#40 excluded as just-cycled): raw=5886040343995211851, index 0 ->
#23. Ledger had NO row; audit/perf-flamegraph holds a complete c1
(fix 83f9989a68 + journal 23cad7b1b0, 2026-07-28) stranded
off-lineage — the #66 problem again. Branch: audit/perf-flamegraph-c2
from e83ffe70a8 (#40 c1 bookkeeping).

### Backport
Cherry-picks: fix as 93c29aac55, journal as f356018b10
(uber-rotation.md conflict resolved ours; c1 row restored in
uber-goal-state.md).

### Verification at HEAD
- cmake --build build-before --target test_bitcoin clean;
  test_bitcoin --run_test=mempool_tests -> No errors detected
  (includes UpdateTransactionsFromBlockRestoresChildAncestry).
- ComplexMemPool bench at HEAD: FIRST run measured 278.5 ms/op —
  ~baseline, because I had only rebuilt test_bitcoin and ran a STALE
  bench_bitcoin binary (process lesson: rebuild the exact binary
  whose behavior you measure). After --target bench_bitcoin rebuild:
  184.7 ms/op (ins/op 607.9M -> 444.1M, -27%), matching c1's
  repaired 181.4 median within ~1.8% (build/noise drift) and
  refuting the stale 278.5. Effect confirmed: ~-32% vs the
  268.5 baseline.

### Verdict
c1 fix verified in lineage: the gate compiles to zero code in this
Release-with-asserts build (G_ABORT_ON_FAILED_ASSUME false), the
oracle stays active in fuzz/abort builds, and the measured cost
disappears. No new defect work this cycle (backport + independent
re-verification is the bounded cell; the c1 queue's heavier profile
cells — EvictionProtection*/ConnectBlockAll captures,
CompareMainTransactions hypotheses — carry forward).

### Exact commands
- git cherry-pick 83f9989a68 23cad7b1b0 -> 93c29aac55, f356018b10
- test_bitcoin --run_test=mempool_tests
- bench_bitcoin --filter=ComplexMemPool --min-time=2000 (stale:
  278.5; rebuilt: 184.7 ms/op)

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
