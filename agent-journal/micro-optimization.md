# Goal 20: Simple micro-optimization discovery and proof

## Cycle 161: HexStr direct-store hypothesis dismissed

- Selector and branch: the fresh gate drew goal 84 first, which was already closed by Cycle 95; the required reroll `shuf -i 0-98 -n 1` drew `20` (`micro-optimization`). The branch is `uber-cycle-161-micro-optimization-20260730`.
- Start state: HEAD `41fb712ad28f701dec477ea9d5245c1ff1a4e4f3`, `origin/master` `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and divergence `42 1103`. Cycle 140's `BufferedFile::FindByte`/`memchr` optimization was explicitly excluded.
- Scope: `HexStr(std::span<const uint8_t>)` in `src/crypto/hex_base.cpp`, whose existing 512-byte lookup table copied two characters per input byte with `std::memcpy`. The proposed change assigned `it[0]` and `it[1]` directly. Existing `util_HexStr` tests cover empty spans, all three accepted span types, and every byte value.

### Cycle 161 evidence

- Build: GCC 12.2 `RelWithDebInfo` (`-O2 -g`, hardening enabled, `REDUCE_EXPORTS=ON`) in `/data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-gcc`; `bench_bitcoin` and `test_bitcoin` built successfully. The candidate and base benchmark executable hashes were respectively `bb9613003638a64048f025a1fbe3afcf2b6e39fa207ca7841c6177bdf42c9717` and `7dd4aeb9290f66a09fb4e190302fdb2a83dafffe238630ca82b1085082379ca2`.
- Baseline before editing: five CPU-2-pinned `HexStrBench` runs at `-min-time=500` measured approximately `0.55 ns/byte`, `6.00 instructions/byte`, and `0.44 cycles/byte` for a 4,000,000-byte batch. CPU frequency scaling was enabled, so wall time is directional; the hardware counters were stable.
- Interleaved base-first measurements, five pairs, used the exact command `taskset -c 2 .../bench_bitcoin -filter='HexStrBench' -min-time=500 -output-json=...`. Base averaged `0.002207324` seconds, `24,000,403.926` instructions, and `1,774,230.198` cycles per epoch. The direct-store candidate averaged `0.003073632` seconds, `36,000,404.723` instructions, and `2,467,150.864` cycles. Per 4,000,000-byte batch this is about `6.00` versus `9.00` instructions/byte and `0.44` versus `0.62` cycles/byte.
- Reversed-order measurements, five pairs, produced the same direction: base averaged `0.002196625` seconds, `24,000,403.918` instructions, and `1,766,416.644` cycles; candidate averaged `0.003095499` seconds, `36,000,405.163` instructions, and `2,484,963.980` cycles. Branch counts stayed near 4,000,097 in both variants, so the extra work is in the generated stores/loads rather than a changed loop boundary.
- The candidate binary passed `mkdir -p /data/my_storage/tmp/cycle161-micro-optimization/test-tmp && TMPDIR=/data/my_storage/tmp/cycle161-micro-optimization/test-tmp .../test_bitcoin --run_test=util_tests/util_HexStr --random=161021 --log_level=test_suite`: one case, no errors. An earlier broad `util_tests` attempt was invalid because its explicitly supplied `TMPDIR` did not exist and entered an unrelated long-running path; only that newly started PID was stopped, while persistent unrelated PIDs 777094 and 956381 were preserved.

### Cycle 161 verdict

Dismissed. Direct character stores are semantically equivalent on the focused oracle but are a reproducible performance regression in both execution orders. The source was restored byte-for-byte; `git diff --check` is clean and no source/test commit is warranted. The next goal-20 cycle should select a different measured hot operation, search this journal before probing it, and retain these HexStr measurements as a closed negative result.

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
