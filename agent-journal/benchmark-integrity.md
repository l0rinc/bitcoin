# Benchmark integrity journal

## Cycle 205 - benchmark-integrity

Branch: `uber-cycle-205-benchmark-integrity-20260731`

### Fresh gate and scope

- The exact selector `shuf -i 0-98 -n 1` returned `19` after the Cycle 204 state close. This cycle uses the dedicated branch `uber-cycle-205-benchmark-integrity-20260731`.
- Gate timestamp: `2026-07-31T10:56:19Z`. Start HEAD is `cf314d5b8517ea4219ed2a7066929b590be2d750`; `origin/master` is `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base is `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence is `1200 42`.
- The pre-cycle uber-goal state SHA-256 is `02093eddc7211c32ac80179e366f2c885827d155590e9f1e5cf4e009ec282cec`. Catalog, prompt, goals TSV, and protocol hashes remain `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- `git diff --check` passed, `src/net_processing.cpp` had no tracked diff, and protected processes `777094`, `956381`, `1138182`, and `1157959` were alive. Known untracked agent/build artifacts remain outside this cycle's scope.
- Prior Cycle 169 cells are excluded: the `WriteBlockBench` persistent-growth defect was fixed in `f02853e248`; `MuHashPrecompute` compiler-elision concern, SHA-256 fixture mutation, and `FindByte` result-oracle concern were independently dismissed. This cycle must select a different benchmark-contract cell.
- Scratch data, logs, builds, and temporary mutations belong under `/data/my_storage/tmp/cycle205-*`; never use a default datadir, wallet, key, or production database.

### Working hypotheses

- H1: benchmark wrappers that call `bench.run()` without `doNotOptimizeAway`, an output assertion, or a state-changing operation may report a plausible timing while the optimizer removes or changes the claimed work. Inventory and verify one high-risk wrapper with assembly, counters, and a temporary mutation.
- H2: `batch()` and `unit()` metadata may be missing or semantically wrong for fixed-size and multi-operation benchmarks, producing misleading per-operation or per-byte results even when the timed code is correct.
- H3: setup outside `bench.run()` may intentionally or accidentally warm caches, reuse mutable fixtures, or perform the first operation only outside the timed region. Compare a selected stateful benchmark's cold/warm and per-epoch setup contracts.
- H4: benchmark result export may retain only the last result for repeated `run()` calls or asymptote points, causing output files to misrepresent what was measured. Verify the runner contract against nanobench result ownership and an instrumented multi-run benchmark.

### Initial evidence map

1. `src/bench/bench.cpp`, `src/bench/bench.h`, and `src/bench/nanobench.h`: runner setup, result collection, `batch`, `unit`, warmup, per-epoch setup, and compiler barriers.
2. `src/bench/*.cpp`: inventory timed lambdas, result sinks, mutable fixture reuse, batch declarations, and output units.
3. Recent benchmark history and prior journal entries: distinguish known fixes from new contract gaps.

### Cycle 205 evidence log and verdict

H1 is confirmed as a benchmark output-integrity defect. On the pre-fix binary `/data/my_storage/tmp/cycle169-bench-build2/bin/bench_bitcoin`, this command:

```text
TMPDIR=/data/my_storage/tmp/cycle205-asymptote /data/my_storage/tmp/cycle169-bench-build2/bin/bench_bitcoin -filter=MempoolCheckEphemeralSpends -asymptote=10,20,40 -min-time=1 -testdatadir=/data/my_storage/tmp/cycle205-asymptote/datadir -output-json=/data/my_storage/tmp/cycle205-asymptote/results.json
```

printed measured rows for complexity values 10, 20, and 40 and printed the complexity fit over all three `Bench::results()`, but the JSON contained `results=1` with only `complexityN=40`. The runner's `benchmarkResults.push_back(bench.results().back())` was therefore inconsistent with both the asymptote console output and the `-output-json` help text claiming all benchmark results. The raw pre-fix log and JSON are under `/data/my_storage/tmp/cycle205-asymptote/`; the JSON SHA-256 is `706788b4f946546131a952e70a727523da7357e17bc0a5f4c694a22e161bfde4`.

The fix in `src/bench/bench.cpp` appends the complete result vector in measurement order. The rebuilt binary at `/data/my_storage/tmp/cycle169-bench-build2/bin/bench_bitcoin` then produced `result_count=3`, `complexity=[10, 20, 40]`, and three matching benchmark names in `/data/my_storage/tmp/cycle205-asymptote-fixed2/results.json`. The same binary exported three CSV data rows for the same invocation; the CSV SHA-256 is `4d4ac35202342f7c592af849d190944819b66c4fd4f59c57ffb42c947f9d30b8`. The console still printed all three rows and the same complexity fit.

The same defect affected ordinary multi-run benchmark functions. `src/bench/merkle_root.cpp` calls `run()` once for `MerkleRoot` and once for `MerkleRootWithMutation`; after the fix, `/data/my_storage/tmp/cycle205-merkle/results.json` contained `result_count=2` with both names and unit `leaf` (SHA-256 `92e9e9904110c64642f3cfc1b7ecb6e162a8a6a2187e83d0020f4620d943dd09`). The focused functional regression is `test/functional/tool_bench_output.py`; it uses a clean zero-node setup, binds `TMPDIR` to the framework scratch directory, checks both asymptote complexity values and repeated-run names, and passed with random seed `205019` at `/data/my_storage/tmp/cycle205-functional-output4`.

The first direct functional invocation failed before the benchmark because the minimal build had no `bitcoind`; changing the test to `setup_clean_chain=True` removed the cached-chain prerequisite. A second invocation failed because the parent shell supplied a nonexistent `TMPDIR`; the test now sets it to its existing framework directory. These were harness-environment failures, not source findings.

H2 remains an unchecked batch/unit inventory item. The current static inventory found explicit batch/unit metadata for byte, block, element, selection, key, job, and script workloads, with no changed source justified yet. H3 remains excluded where it overlaps Cycle 169's fixed `WriteBlockBench` state-growth defect. H4 is resolved by the runner fix; no separate production benchmark operation was changed. The missing-result regression is independently demonstrated by the pre-fix output and the post-fix JSON assertion.

### Cycle 205 required proof and next queue

- Required proof for H1 is complete: failing-before export count, source-level fix, passing-after JSON and CSV counts, a repeated-run held-out case, and automated functional assertions over three distinct asymptote points plus both MerkleRoot variants.
- Run `git diff --check`, rebuild `bench_bitcoin`, rerun the focused functional test, and inspect the resulting diff before committing. A narrow `-output-csv` check should confirm that the same complete result vector is exported by both structured formats.
- Next distinct cell: finish H2 by checking batch/unit metadata against the operation actually performed, prioritizing a benchmark with a nontrivial loop or an asymptote point whose unit changes under setup. Do not reopen Cycle 169's WriteBlock, MuHashPrecompute, SHA fixture, or FindByte cells.

### Cycle 205 close

- Evidence commit `2b9d5d91fa271ba87816ac3646658066eb34867b` (`bench: export all asymptote results`) is authored as `Lőrinc <pap.lorinc@gmail.com>`. It changes only result aggregation, adds the focused functional test and test-runner registration, and includes this journal.
- `cmake --build /data/my_storage/tmp/cycle169-bench-build2 --target bench_bitcoin -j2` completed successfully after the final source edit. The focused functional test passed with seed `205019`; the selected `MempoolCheckEphemeralSpends|MerkleRoot -sanity-check` also exited 0. CSV export produced three data rows. No full suite or all-benchmark performance run was claimed because the root filesystem remains critically full and the cycle only requires the narrow runner validation.
- Final source-commit HEAD is `2b9d5d91fa271ba87816ac3646658066eb34867b`; direct divergence is `1201 42`. Protected PIDs remained alive and no cycle-owned process remains. The state-only uber-goal close is intentionally separate and will record the next queue: H2 batch/unit semantics, with the completed runner-export cell excluded.


## Cycle 169 - benchmark-integrity

Branch: `uber-cycle-169-benchmark-integrity-20260730`

### Fresh gate and scope

- Goal selected with exact selector output `19` after the Cycle 168 close.
- Base/HEAD and dirty state were recorded before branch creation; the worktree has only the repository's pre-existing untracked agent artifacts and test/build artifacts.
- Focus is benchmark names, setup, timed regions, batching, units, input realism, cache and I/O state, allocation and compiler-elision barriers, fixture reuse, result validation, and reproducibility.
- Scratch data and logs belong under `/data/my_storage/tmp/cycle169-*`; no default datadir, wallet, key, or production database is used.

### Initial evidence map

1. `src/bench/crypto_hash.cpp`: inspect `MuHashPrecompute`, the 32-byte SHA-256 fixtures, and neighboring optimization barriers.
2. `src/bench/readwriteblock.cpp`: determine whether repeated `WriteBlock` calls measure a stable operation or append indefinitely to a benchmark file.
3. `src/bench/streams_findbyte.cpp`: verify the timed search result and reset state on each iteration.
4. `src/bench/bench.cpp`, `src/bench/bench.h`, and `src/bench/nanobench.h`: verify setup/timed boundaries, result collection, batch units, and compiler-elision behavior.

### Working hypotheses

- H1, high priority: `MuHashPrecompute` may benchmark no meaningful constructor work because its temporary result is discarded and no `doNotOptimizeAway` barrier or observable state update is present. Verify with optimized assembly, benchmark counters, and an independent guarded comparison.
- H2: SHA256 32-byte benchmarks overwrite their input with the previous digest, so only the first iteration uses the declared zero fixture. Determine whether this changes the measured operation or only the input bytes while preserving the same fixed-size cost.
- H3: `WriteBlockBench` may include persistent file growth and page-cache state from every prior iteration. Determine whether this is an intentional append-throughput benchmark or an unstable mismatch with its name/contract.
- H4: `FindByte` may omit a result assertion, but check its state reset and historical rationale before treating that as an oracle defect.

### Evidence ledger

Confirmed: H3 is confirmed. `WriteBlockBench` calls `WriteBlock` repeatedly against one `BlockManager`; the cursor and `m_blockfile_info` grow, `FlatFileSeq::Allocate` preallocates 16 MiB chunks, and each 128 MiB rollover flushes the prior file. Five pinned 200 ms current-release runs produced medians of 783.6, 750.6, 869.3, 744.6, and 740.9 us/op; nanobench marked three as unstable. Each scratch datadir grew to 160-208 MiB, with 128 MiB-plus block files. This violates the benchmark help contract that each call of `run()` should have the same preconditions and makes the result depend on the run's rollover position.

The fix uses nanobench's untimed per-epoch setup to destroy and recreate `TestingSetup` before each measured write. Five pinned 200 ms post-fix runs measured 928.7, 921.8, 978.5, 940.2, and 1058.4 us/op; instruction and branch counts were constant at about 3,787,702 and 776,153, and every final scratch datadir contained exactly one 16 MiB `blk00000.dat`. The remaining wall-time variation is the real storage allocation cost, not an in-loop file rollover or accumulated cursor state. `-sanity-check` passed and produced one block file.

Dismissed: H1 is dismissed for the supported current release-like build. `MuHashPrecompute` had no explicit result barrier, but five pinned runs measured 871.7 ns/op, exactly 7,102 instructions and about 703 cycles per constructor. GCC 12.2 generated a timed lambda containing a direct call to `MuHash3072::MuHash3072`; the constructor was not elided. LTO or a different toolchain would be a separate campaign, not evidence against this build.

Dismissed: H2's 32-byte SHA-256 fixture is overwritten by its digest after the first iteration, but every iteration still hashes exactly 32 bytes and the benchmark has no data-dependent work in this path. No measurable or semantic benchmark defect was demonstrated; changing the fixture would be speculative.

Dismissed: H4's `FindByte` benchmark does not assert `GetPos()` itself, but `BufferedFile::FindByte` has an `Assume` postcondition that the target byte was found, and the timed loop resets the position before each search. The historical setup-reset issue was already fixed in commit `2c9e5faf84`; no distinct oracle gap remains here.

Dismissed/inconclusive: none yet.

Unrelated leads: historical `streams_findbyte` changes already addressed timed setup and realistic buffer traversal; do not duplicate that work without a new regression.

### Required proof and next queue

For H3, the narrow fix is to use nanobench's untimed per-epoch setup to recreate `TestingSetup` before each measured write. Validate that the source builds, `-sanity-check` succeeds, five release-like runs have comparable one-write measurements, and each run leaves one block-sized file rather than a growing sequence. Continue with H2 and H4 only after validating this fix. H1's exact JSON, logs, and disassembly are under `/data/my_storage/tmp/cycle169-bench/`.

Validation artifacts: `/data/my_storage/tmp/cycle169-bench/build-current.log`, `build-fix.log`, `build-delay.log`, `build-clean-after-mutation.log`, `write-block-sanity.log`, `write-block-final-sanity.log`, `write-block-delay.json`, `write-block-*.json`, `write-block-fixed-*.json`, `muhash-precompute-*.json`, `muhash-mul-*.json`, and the corresponding raw logs. A temporary 1 ms delay inside the timed lambda measured 2,034.1 us/op versus about 940 us/op without it, proving the harness detects a deliberate slowdown. The mutation was removed, the clean post-removal release build passed, final `-sanity-check` passed, and the final scratch datadir again contained one 16 MiB block file. `git diff --check` is clean.

Record exact commands, compiler/flags, CPU and affinity, benchmark filters, data paths, source/history links, verdicts, limitations, and the next distinct hypothesis after every experiment.

### Cycle close

- Source/journal commit: `f02853e248` (`bench: reset WriteBlock benchmark state`), authored as `Lőrinc <pap.lorinc@gmail.com>`.
- Post-commit `cmake --build /data/my_storage/tmp/cycle169-bench-build2 --target bench_bitcoin -j2` exited 0; the post-commit focused `WriteBlockBench -sanity-check` exited 0 and left one 16 MiB `blk00000.dat`.
- The full unit suite was not run because `/` remains at 99% capacity after the prior cycle's disk guard; no full-suite result is claimed. Persistent unrelated test processes `777094` and `956381` were preserved, and no owned process remains.
- Close gate after `git fetch origin master`: HEAD `f02853e248dfb20385bc42c416e83315763a40ad`, `origin/master` `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, divergence `42 1120`; `git diff --check` passed. Catalog/prompt/TSV/protocol hashes were `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- The next exact selector command `shuf -i 0-98 -n 1` returned `74` (`memory-pressure-allocator`). Next run: create its fresh branch, preserve this benchmark finding as closed, and start from the existing memory-pressure queue without reopening Cycle 53's prevector OOM cell.

## Cycle 218: pool benchmark batch/unit contract

### Fresh gate and scope

- The exact selector command `shuf -i 0-98 -n 1` returned `19`, selecting `benchmark-integrity`. The dedicated branch is `uber-cycle-218-benchmark-integrity-20260731`.
- The pre-cycle gate recorded start HEAD `57d047def7915f50070e52c94e8a20fb41f08704`, `origin/master` `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and divergence `1223 42`. Tracked status and `git diff --check` were clean.
- Catalog, prompt, goals TSV, and protocol hashes were `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`. The pre-cycle state hash was `2517e87a87524d1ae1f233dad34b8f08403dd780cd1d5523e68c46816cb909f6`.
- Protected processes `777094`, `956381`, `1138182`, and `1157959` were alive and were not touched. The root filesystem had about 6.7 MiB free; `/data` had about 34 GiB free. All cycle scratch paths are under `/data/my_storage/tmp/cycle218-*`.
- Cycle 205's multi-result export fix and Cycle 169's WriteBlock, MuHashPrecompute, SHA fixture, and FindByte cells remain excluded. This cycle continued the queued batch/unit audit.

### Inventory and candidate

- The static command `git grep -n -E '\\.(batch|unit)\\(' -- src/bench` found explicit metadata in the benchmark inventory. The default-unit batch sites are `asmap.cpp`, `pool.cpp`, and four prevector helpers; the other batch sites explicitly name byte, block, selection, key, job, script, leaf, cost, number, or similar units.
- The selected candidate is `BenchFillClearMap` in `src/bench/pool.cpp`, introduced by history commit `b8401c3281978beed6198b2f9782b6a8dd35cbd7`. Its timed lambda performs exactly 5,000 `map[rng()]` inserts and then one `map.clear()`. The runner configured `batch_size` as 5,000 but left nanobench's unit at its default `op`, so structured output and the table reported `ns/op` and `ins/op` even though the normalization is explicitly per inserted element and includes the amortized clear cost.
- The pre-change control used `/data/my_storage/tmp/cycle169-bench-build2/bin/bench_bitcoin -filter='PoolAllocator_.*' -min-time=50 -output-json=/data/my_storage/tmp/cycle218-pool/results.json` with `TMPDIR=/data/my_storage/tmp/cycle218-pool`. The JSON records showed `unit: "op"` and `batch: 5000`; a shell assertion for `unit: "insert"` failed as expected. The console showed `ns/op` and `ins/op` for both variants.

### Fix and verification

- The narrow source fix changes the shared helper to call `bench.batch(batch_size).unit("insert").minEpochIterations(10)`. The timed operation, batch size, setup, allocator comparison, and clear behavior are unchanged; only the exported measurement unit is made explicit.
- `CCACHE_DIR=/data/my_storage/tmp/cycle218-ccache TMPDIR=/data/my_storage/tmp/cycle218-build cmake --build /data/my_storage/tmp/cycle169-bench-build2 --target bench_bitcoin -j2` completed all 57 steps and linked `bin/bench_bitcoin`. A first build attempt failed before source compilation because ccache tried to create `/root/.cache/ccache/tmp` on the full root filesystem; rerouting ccache resolved that environment issue.
- The post-change command `/data/my_storage/tmp/cycle169-bench-build2/bin/bench_bitcoin -filter='PoolAllocator_.*' -min-time=50 -output-json=/data/my_storage/tmp/cycle218-pool-fixed/results.json` with `TMPDIR=/data/my_storage/tmp/cycle218-pool-fixed` exited 0. It printed `ns/insert` and `insert/s`; both JSON records contain `unit: "insert"` and `batch: 5000`. The measured rows were 40.57 ns/insert and 24.42 ns/insert, with the repository's CPU powersave/turbo instability warning.
- The focused sanity control `/data/my_storage/tmp/cycle169-bench-build2/bin/bench_bitcoin -filter='PoolAllocator_.*' -sanity-check` with `TMPDIR=/data/my_storage/tmp/cycle218-sanity` exited 0.
- The functional command `python3 test/functional/tool_bench_output.py --configfile=/data/my_storage/tmp/cycle169-bench-build2/test/config.ini --tmpdir=/data/my_storage/tmp/cycle218-functional-01 --randomseed=218019` exited 0 and reported `Tests successful`. It checked the existing asymptote and repeated-run export behavior, plus both pool benchmark names, exact `insert` units, and exact 5,000 batches.
- This is a confirmed benchmark-interface defect, not a production runtime defect: the old structured artifact proves the prior contract, and the new structured-output assertions fail on that artifact and pass on the rebuilt binary. No online PR was used as an oracle; the source/history evidence was local.

### Limitations and next queue

- The host remained CPU-frequency unstable, so timing values are diagnostic only; this cycle's finding is based on the deterministic unit/batch metadata and regression assertions, not on a performance comparison. The full benchmark suite and full unit suite were not run because the root filesystem is critically full. The build was wallet-disabled, which is irrelevant to the selected non-wallet benchmark.
- Next distinct cells are the remaining default-unit batch sites, especially `asmap.cpp` and the prevector helpers, followed by return-value barriers in benchmark wrappers. Do not reopen the pool label, Cycle 205 result aggregation, or Cycle 169 cells without new evidence.
