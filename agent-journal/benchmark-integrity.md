# Benchmark integrity journal

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
