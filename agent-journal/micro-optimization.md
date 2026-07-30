# Goal 20: Simple micro-optimization discovery and proof

## Cycle 140: `BufferedFile::FindByte` contiguous scan

- Branch: `uber-cycle-140-micro-optimization-20260730`
- Start HEAD: `c8c855c410dcfbf2d32d7a526a1fd2e2b92bdb1a`
- Scope: the contiguous byte-search portion of `BufferedFile::FindByte` in `src/streams.h`.
- Prior evidence: the repository's existing `FindByte` benchmark is a 200-byte scan with the match at the final byte. A GCC 12.2 `RelWithDebInfo` build (`-O2 -g`) measured approximately 73 ns/op, 659 instructions/op, 84 cycles/op, 264 branches/op, and 0.4% branch misses. Five external `hyperfine` runs were 107.6--107.7 ms for the benchmark process. CPU frequency scaling was enabled, so the measurement is directional rather than a portable absolute. The original benchmark used `setup()`, which forces one timed iteration per epoch; the harness now resets the position inside `run()` so repeated operations have identical preconditions.
- Production reachability: block validation uses `FindByte` while scanning `blk*.dat` for the network magic byte; buffered-file fuzzing and stream tests exercise the same contract.
- Hypothesis: replacing only the contiguous `std::find` with libc's `memchr` will reduce scan instructions and branches without changing buffer refill, transfer-limit, EOF, or exact-position behavior.
- Required proof: compare identical builds and pinned CPU runs, inspect hardware counters, run the focused stream tests and full `streams_tests`, and retain the pre/post benchmark artifacts. Reject the change if the result is within noise or if semantic coverage is insufficient.

## Evidence ledger

| Status | Candidate | Evidence |
| --- | --- | --- |
| confirmed | `std::find` to `memchr` for one contiguous buffer segment | Semantics passed; source and benchmark commit pending |

## Cycle 140 result

The benchmark was corrected to reset the buffered stream inside each timed iteration. With the same GCC 12.2 `RelWithDebInfo` build (`-O2 -g`, hardening enabled), CPU pinned to core 2, and five base/candidate pairs in each ordering:

- Base first: base was 47.87--49.97 ns/op, 644 instructions/op, 38.6--40.3 cycles/op, and 263 branches/op. Candidate was 12.41--12.78 ns/op, 162 instructions/op, 10.0--10.3 cycles/op, and 30 branches/op.
- Candidate first: candidate was 12.41--12.59 ns/op, 162 instructions/op, 10.0--10.1 cycles/op, and 30 branches/op. Base was 47.86--47.99 ns/op, 644 instructions/op, 38.6--38.7 cycles/op, and 263 branches/op.
- The host's powersave governor changed wall-clock frequency between runs; hardware counters remained stable. The candidate reduced the measured scan work by about 75% in instructions, cycles, and branches.

Validation:

- `cmake --build /data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-gcc --target bench_bitcoin -j4`: passed for the candidate.
- `cmake --build /data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-gcc --target test_bitcoin -j4`: passed; existing unrelated warnings remained in `httpserver_tests.cpp` and `util_tests.cpp`.
- `test_bitcoin --run_test=streams_tests --log_level=test_suite`: 27 cases passed, including boundary, EOF, failure-position, exact-match, and randomized buffered-file cases.
- `FUZZ=buffered_file ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 .../fuzz ... -seed=140 -runs=2000 -print_final_stats=1`: 2,000 runs, 53 new units, no sanitizer diagnostics, peak RSS 1462 MiB.
- `git diff --check`: passed.

Implementation notes:

- `memchr` is limited to the already computed contiguous segment, so refill, rewind, transfer-limit, EOF, and exact-position behavior remain outside the changed operation.
- `std::byte` is converted to `unsigned char` before the C library search; every byte value is representable.
- The first candidate compile correctly rejected passing a `std::vector` iterator to `memchr`; the final code uses `DataBuffer::data()` and compiled cleanly.
- The header change caused the expected broad rebuild because `BufferedFile` is defined in `streams.h`; no additional source helper or public contract change was introduced.

Artifacts: `/data/my_storage/tmp/cycle140-bench/findbyte-base-fixed.json`, `/data/my_storage/tmp/cycle140-bench/findbyte-candidate-fixed.json`, `/data/my_storage/tmp/cycle140-bench/interleaved-*.json`, `/data/my_storage/tmp/cycle140-bench/reverse-*.json`, and `/data/my_storage/tmp/cycle140-bench/buffered_file-corpus/`.

## Handoff

After the candidate measurement, either commit one self-contained source/test/journal change with exact results or record a dismissed hypothesis and close the cycle without a source change.
