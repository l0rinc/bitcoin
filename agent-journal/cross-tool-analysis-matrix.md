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

## Cycle 2 (2026-07-28): clang-18 unit-suite differential — IN PROGRESS marker, results below

Base: c51d41c8fc (journal commit for #60 cycle-1 on
audit/reviewer-preference; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/cross-tool-c2 (c1 journal carried in the
carry commit). Start state: clean (untracked scratch only).

### Draw (with pool-repair)
A draw over the legacy incremental pool landed on #61 (CYCLE-3) —
raw=2149655188711527484, idx 16/27 — and was DISCARDED: the pool had
silently carried stale CYCLE-2+ entries (4, 28, 61). Pool rebuilt from
the ledger handoff per the documented rule (41 pending + 23 CYCLE-1 =
64; EXHAUSTED/QUEUE-COMPLETE/deferred #72/#77 excluded; #60 excluded
as just-cycled). Redraw: raw=9923708442630681006, seed masked to 63
bits (700336405775905198), idx 46/64 -> #36. The repair is recorded in
uber-rotation.md (POOL-REPAIR NOTE).

### Hypothesis (queued cell from c1)
H: clang-18 (1.1.3... 18.1.3) exposes optimizer/warning-visible
divergence in the unit suite that gcc-13.3 misses on identical source
and flags (this tree: the fork's Assume-hardened validation/P2P code +
the #22-c2 bloom clean-flag patch + #1-c1 comment fix).

### Design (differential isolation verified)
- build-clang: Ninja, Release -O2, clang-18/clang++-18 18.1.3,
  options mirrored from build-before (wallet/IPC/bench/kernel ON,
  ZMQ/USDT OFF, ccache ON). CMAKE_CXX_FLAGS empty on both sides;
  CMAKE_CXX_FLAGS_RELEASE=-O2 on both — compiler is the ONLY variable.
- build-before (gcc 13.3.0) provides the same-source baseline: HEAD
  c51d41c8fc, worktree clean.
- Cells: (a) build warning differential (count + classification);
  (b) full unit suite pass/fail differential; (c) on any divergence:
  minimize and classify (project bug vs test bug vs toolchain).

### Progress log
- configure: 21.9s, clean.
- build: 536 edges for bin/test_bitcoin (cold ccache for clang);
  warnings through edge ~121: ZERO.

### Results (differential complete)
Build: 536 edges, 25.2 min, exit 0, 4 warnings (gcc side: 0).
Suite: clang-18 test_bitcoin full suite GREEN (147.9s, "No errors
detected"); gcc-13.3 same-source baseline (HEAD c51d41c8fc, freshly
relinked) GREEN. No behavioral divergence.

### Warning triage (the actual differential yield)
1. signet.cpp:71 -Wunneeded-internal-declaration
   (IsSignetToSpendScriptSig): fork-added helper (c3e4b74ec5) whose
   only caller sits inside AssumeCreatedSignetTxs' `if constexpr
   (G_ABORT_ON_FAILED_ASSUME)` block (signet.cpp:83-88). In non-fuzz
   builds the call is discarded, so the function is never emitted.
   BY DESIGN — clang-only diagnostic of the fork's fuzz-only Assume
   pattern. NOT dead code (emitted and exercised in BUILD_FOR_FUZZING).
2. txgraph.cpp:41,57 -Wunneeded-member-function
   (CheckedFeePerWeightSum::Add/AssumeMatches): same class — fork
   saturation-check helpers called only inside `if constexpr
   (G_ABORT_ON_FAILED_ASSUME)` blocks (txgraph.cpp:3379-3393,
   3484-3499; struct from 3ae78dbd25). BY DESIGN.
3. test/blockmanager_tests.cpp:539 -Wthread-safety-analysis: writing
   CBlockIndex::nStatus (upstream GUARDED_BY(::cs_main) annotation,
   chain.h:137) while constructing unshared test indexes before
   WriteBatchSync. Single-threaded setup, object not yet published —
   benign upstream test pattern; gcc has no thread-safety analysis at
   all, so the class is inherently clang-only. NO ACTION.
Systematic note: 3 of 4 warnings are the SAME interaction — clang's
-Wunneeded-* flags helpers referenced only from discarded
G_ABORT_ON_FAILED_ASSUME constexpr blocks. Any future clang CI on
this fork will see one warning per such helper; the fix, if ever
wanted, is [[maybe_unused]] on the helpers, not deletion.

### Verdict
- DISMISSED: clang-18 finds no optimizer-visible defect in the unit
  suite on this source; both compilers green.
- Matrix cell filled: full unit suite x {gcc-13.3, clang-18.1.3} x
  -O2 Release (asserts on) = 0 behavioral divergences, 4 clang-only
  warnings all triaged benign/by-design (3 fork fuzz-only-helper
  class, 1 upstream test-annotation class).
- Hypothesis H refuted for the suite; the differential's yield is the
  warning-class map above.

### Exact commands
- cmake -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18
  (options mirrored from build-before; verified CMAKE_CXX_FLAGS and
  CMAKE_CXX_FLAGS_RELEASE=-O2 identical both sides)
- ninja -C build-clang bin/test_bitcoin (log /tmp/btc36_clang_build.log)
- build-clang/bin/test_bitcoin --log_level=test_suite
- ninja -C build-before bin/test_bitcoin && build-before/bin/test_bitcoin
- triage: grep -n callers + git log -S per symbol (see above)

### Limitations / queue
- clang build dir deleted after use (231 MB scratch; recreate with
  the cmake line above, ~25 min).
- clang UBSan cell (c1 ran gcc UBSan) and _GLIBCXX_ASSERTIONS cell
  still open; TSan subset still open.
- Warning-as-error differential (CMAKE_COMPILE_WARNING_AS_ERROR=ON)
  would surface the 4 warnings as failures in CI — noted for the
  fork's CI-parity campaign (#47) queue, not fixed here (cosmetic).
- Functional tests under clang not run (unit-only cell per queue).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted (3
cells open).
