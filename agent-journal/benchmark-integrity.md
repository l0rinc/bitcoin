# Campaign #19 — benchmark-integrity

Base: audit/resurrection @ 7ca69f0172 (rotation ledger commit for #18).
Branch: audit/benchmark-integrity. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-27): prevector Clear/Resize/Destructor measured harness overhead — CONFIRMED + FIXED

### Hypothesis
Header-inline micro-benchmarks whose timed regions leave no observable result
are dead-code-elimination candidates: if the operation under test compiles to
dead stores, the benchmark measures nanobench harness overhead, not the
operation. Surveyed bench files lacking `doNotOptimizeAway`
(`grep -rLn doNotOptimizeAway src/bench/*.cpp` — most entries call separate-TU
functions and are safe); `src/bench/prevector.cpp` operates entirely on
header-inline template code in-TU → prime candidate.

### Trust boundary
Measurement integrity only (bench/); no production code involved. Campaign
rule applied: fix the misleading benchmark, no production change.

### Protocol followed
Release build (`build-before`, gcc, -O2), 5 repetitions per configuration,
median + spread recorded, temporary 2x-work and no-op mutants used to prove
(in)sensitivity, objdump to confirm mechanism.

### Baseline (ns/op, median of 5, spread <= 0.14)
| benchmark | ns/op | ins/op | cyc/op |
|---|---|---|---|
| PrevectorClearNontrivial | 15.46 | 123.5 | 37.0 |
| PrevectorClearTrivial | 6.26 | 47.0 | 15.0 |
| PrevectorDestructorNontrivial | 15.58 | 133.0 | 37.3 |
| PrevectorDestructorTrivial | 9.18 | 81.5 | 22.0 |
| PrevectorResizeNontrivial | 7.74 | 61.75 | 18.8 |
| PrevectorResizeTrivial | 3.13 | 23.5 | 7.5 |

### Mutation evidence (2x operations under test, batch-normalized)
- No-op mutant (body emptied): nanobench itself reports
  `:boom: ... (iterations overflow. Maybe your code got optimized away?)` —
  harness can detect a fully empty body; pre-fix ResizeTrivial at 3.13 ns was
  effectively at that floor.
- 2x-work mutant, PRE-fix: ResizeTrivial 3.13 -> 3.13 (+0%),
  ResizeNontrivial 7.74 -> 8.04 (+4%), ClearTrivial 6.26 -> 6.26 (+0%),
  ClearNontrivial 15.46 -> 15.5 (+0%), DestructorTrivial 9.18 -> 12.96 (+41%,
  only the malloc/free pair survives elision), DestructorNontrivial
  15.58 -> 28.00 (+80%).
  => Clear/Resize fully insensitive: CONFIRMED misleading. Destructor
  partially misleading (fill stores elided, only heap alloc measured).

### Fix (commit 138ef3c044)
`ankerl::nanobench::doNotOptimizeAway(t0/t1)` barriers added to
PrevectorClear, PrevectorResize, PrevectorDestructor, placed while the
vectors are FULL (barrier after clear/resize(0) does not help: stores are
semantically dead at that point — verified: ResizeTrivial stayed 3.13).
Semantics: `asm volatile("" : : "r,m"(val) : "memory")` (nanobench.h:1075)
forces the whole object image to be materialized.

### Post-fix verification
- objdump of `Bench::run<PrevectorResize<unsigned char>...>`: two
  `bl memset@plt` inside the timed loop (previously elided).
- 2x-work mutant on the FIXED Resize: per-iteration cycles 28 -> 59
  (cyc/op 7.00 -> 7.38 with batch 4 -> 8; ins/iter 94 -> 190) — now sensitive.
- Post-fix medians: DestructorTrivial 9.18 -> 17.42 ns/op (ins/op 81.5 ->
  153.5); DestructorNontrivial 15.58 -> 23.44; ClearTrivial 5.85;
  ClearNontrivial 16.50; ResizeTrivial 2.93; ResizeNontrivial 8.35.
  (One of 5 runs showed system-wide interference, median unaffected.)
- Note: Clear/Resize ns/op barely moved because the retained memset of 28-29
  bytes is genuinely cheaper than the ~3ns/iteration harness floor on this
  Cortex-A76; the benchmarks are now HONEST (work retained + scales), the
  absolute numbers are just small. Destructor shows the full effect.
- Full suite: all 12 `Prevector.*` benchmarks run clean post-fix.

### Verdict
- CONFIRMED (3 benchmark families misleading, 1 partial). FIXED in
  138ef3c044. Benchmark-only change; no production code touched.
- Deserialize/FillVector* not implicated (stream consumption / heap growth
  are real observable work; not mutation-tested this cycle).

### Limitations
- Single machine/compiler (Cortex-A76, gcc Release). Elision is
  compiler-dependent; clang may differ. The barrier fix is compiler-neutral.
- Other bench files without doNotOptimizeAway were triaged by TU analysis,
  not mutation-tested — queued below.

### Next queue for this campaign
- Check bench units/batching claims (e.g. `unit("byte")` vs actual batch).
- logging.cpp: check whether the timed region includes formatting setup.
- Wallet benches: fixture reuse across iterations (cache state realism).

## Cycle 2 (2026-07-28): header-inline bench mutation-sweep — all sensitive, no new elision

Sweep of the cycle-1 queue's candidates for the same dead-store shape.
Mechanistic rule that emerged: elision needs the measured work to be a
dead store to a LAMBDA-LOCAL object; benchmark state captured by
reference into nanobench's std::function setup/run lambdas escapes
dead-store elimination (opaque escape), so most benches are naturally
protected.

- streams_findbyte (FindByte): PRIME SUSPECT (header-inline
  BufferedFile::FindByte; found-position result never read, bench.setup
  resets pos unconditionally). Baseline 148.00 ns/op (3 runs, identical).
  2x-scan-size mutant (file_size 200 -> 400): 259.00 ns/op (+75%,
  ~proportional given fixed Fill/SetPos overhead). SENSITIVE — the bf
  object is captured into both setup and run lambdas, so its stores
  escape DSE. Reverted -> 148.00. DISMISSED.
- obfuscation (ObfuscationBench): already has doNotOptimizeAway(data)
  (line 22) and mutates data per call. Safe by inspection. DISMISSED.
- lockedpool (BenchLockedPool): Arena alloc/free mutate internal maps;
  later branch decisions depend on that state — not elidable by
  construction. DISMISSED.
- rollingbloom: CRollingBloomFilter::contains result discarded, but the
  call is out-of-line (common/bloom.cpp) — cross-TU calls are assumed to
  have side effects, cannot be elided. DISMISSED.
- gcs_filter (GCSBlockFilterGetHash et al.): out-of-line calls
  (blockfilter.cpp) or in-loop construction — safe. DISMISSED.
- strencodings (HexStrBench): barrier present (line 21), confirmed cycle 1.
  DISMISSED.

Verdict: no additional misleading benchmarks in the cycle-1 queue.
Journal-only cycle; prevector fix (138ef3c044) remains the sole finding.
Commands: `cmake --build build-before -j4 --target bench_bitcoin`;
`./build-before/bin/bench_bitcoin -filter='FindByte'` (baseline/mutant/
revert).

### Next queue for this campaign
- (exhausted for now — see cycle 3)

## Cycle 3 (2026-07-28): units/batching claims + logging timed region — all honest

- checkqueue (58): batch(BATCH_SIZE*BATCHES).unit("job") — loop submits
  exactly BATCHES x BATCH_SIZE jobs per iteration. HONEST.
- cluster_linearize (85): batch(total_cost).unit("cost") — run recomputes
  the cost of the same 100 Linearize calls and ASSERTS it equals
  total_cost; the batch claim is self-verifying. HONEST (exemplary).
- asmap (24): batch(addrs.size()) — one GetMappedAS per address per
  iteration. HONEST.
- coin_selection (112): batch(NUM_TARGETS=10).unit("selection") — targets
  vector built to NUM_TARGETS; one selection per target. HONEST.
- base58/bech32/bip324_ecdh/checkblock/connectblock: batch matches the
  processed bytes/items; bip324 batch(1).unit("ecdh") trivial. HONEST.
- HexStrBench (c1): batch(data.size()).unit("byte") on the 4M-weight
  buffer. HONEST.
- logging.cpp: TestingSetup (full node context) constructed OUTSIDE
  bench.run; only log() is timed (line 28). Setup not in the timed
  region. HONEST.
- Wallet benches: fixture-per-benchmark reuse is the standard design
  (measurement stability), not a defect. DISMISSED.

### Verdict
- DISMISSED. Campaign queue exhausted: c1 fixed the prevector elision
  (138ef3c044), c2 mutation-swept remaining header-inline benches (all
  sensitive), c3 cleared units/batching + timed-region composition.
  Only finding remains the cycle-1 fix.

### Next queue for this campaign
- New benches in incoming PRs (pr-watch): check for barriers/batch
  claims at arrival.

## Rotation note
Three bounded cycles complete; queue exhausted per evidence above (not
claimed on the whole area — new benches arrive continuously).
