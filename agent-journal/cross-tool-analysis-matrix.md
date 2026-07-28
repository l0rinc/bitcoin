# Campaign #36 — cross-tool-analysis-matrix

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/cross-tool. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): UBSan full unit suite (gcc, -fsanitize=undefined) — PENDING RESULT

### Draw
Random draw over the 37-goal eligible pool: raw=5725662729127340569,
index 12 -> #36.

### Matrix inventory (this host)
- Compilers: gcc 13 (build-before), clang 18.
- Sanitizers run before this session: ASan+UBSan+fuzzer on 7
  security-critical fuzz targets (build_fuzz,
  SANITIZERS=undefined,address,fuzzer; 7000+ runs, zero reports);
  ASan/LSan unit suite; valgrind memcheck on coins_tests (0 errors);
  clang static analyzer (#12, 1 FP documented).
- Suppressions: ubsan file's `-fsanitize=undefined` section EMPTY (no
  blind zones); `-fsanitize=integer` section lists dependency paths
  (boost/c++/leveldb/minisketch/secp256k1) — integer sanitizer is
  intentionally noisy there, not a project blind zone.
- UNTESTED intersection chosen for cycle 1: full test_bitcoin under
  -fsanitize=undefined (gcc, RelWithDebInfo) — the unit-suite x UBSan
  cell the fuzz-only runs never covered (wallet/validation edge paths).

### Method
build-ubsan: -DSANITIZERS=undefined, ninja test_bitcoin, run full
suite; minimize and independently confirm any report before
classifying (project bug vs test bug vs dependency issue).

### Result
Exactly ONE report in the full UBSan suite: streams.cpp:102
'null pointer passed as argument 1, which is declared to never be
null'. Suite-by-suite scan isolated net_tests; halt_on_error stack:
CaptureMessageToFile (net.cpp:4324, -capturemessages) on an
empty-payload message (verack) -> AutoFile::write(span{}) ->
fwrite(nullptr, 1, 0). Formally UB (nonnull contract applies at size
zero), glibc-benign, invisible to non-instrumented builds. Upstream
master carries the identical pattern (verified).
FIX (22aa75a2eb): empty-span early return in AutoFile::read/write/
write_buffer + streams_tests/autofile_empty_span_io regression test.
VERIFIED: UBSan full suite re-run zero reports; gcc suite green.
Failing-before/passing-after complete. build-ubsan removed after use
(disk; recreate with -DSANITIZERS=undefined).

### Verdict
- CONFIRMED one real UB (trivial severity: debug-feature path,
  benign in practice, upstream-applicable). FIXED and verified.
- Matrix cell filled: full unit suite x -fsanitize=undefined (gcc) =
  1 finding; all other cells per inventory remain clean.
- No blind zones found in the suppressions files (undefined section
  empty; integer section is dependency-scoped by design).

### Limitations
- Only the demonstrated trigger path was regression-tested; sibling
  empty-span I/O is now guarded at the same chokepoint but has no
  dedicated caller-path proof (none reachable in the suite).
- clang-18 and _GLIBCXX_ASSERTIONS cells not run this cycle.

### Next queue for this campaign
- clang-18 unit-suite differential (gcc/clang optimizer-visible class).
- _GLIBCXX_ASSERTIONS container-bounds cell (gcc).
- TSan on the scheduler/net threads subset (lock-order noise expected;
  requires curated suppressions).

## Rotation note
One bounded cycle in progress; rotation per uber-goal policy follows
the result.
