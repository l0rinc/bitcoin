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

## Cycle 3 (2026-07-31): clang-18 UBSan full unit suite — zero reports; {gcc,clang} x UBSan matrix consistent; DISMISSED

### Draw
RE-RANK draw 134 over the 8-cell re-harvested queue: raw=
16057275024291464013, masked 6833902987436688205 -> idx 5 -> clang
UBSan cell (campaign #36; the harvest shorthand mislabeled it #47 —
#47 is ci-parallelism; corrected here).

### Hypothesis
H: clang-18 -fsanitize=undefined over the full unit suite reports
zero NEW undefined-behavior findings beyond c1's fixed streams.cpp:102
class — the {gcc-13.3, clang-18.1.3} x UBSan matrix is consistent.

### Method
cmake -B build-clang-ubsan -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
-DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18
-DSANITIZERS=undefined; ninja bin/test_bitcoin; UBSAN_OPTIONS=
print_stacktrace=1:halt_on_error=0 full suite run.
WORKFLOW TRAP (recorded): configuring the build dir while the
worktree was on agent/all-findings, then switching to the older
feature branch, left a stale generate rule
(xswiftec_inv_test_vectors.csv.h, #108 machinery) that failed the
first build ([375/537] GenerateHeaderFromRaw: source csv absent on
this branch). Repair: reconfigure after checkout; rebuild completed
(158 remaining edges). Lesson: configure AFTER the branch checkout,
not before.

### Result
- Suite: 1128 test cases, rc=0, "No errors detected",
  grep -c 'runtime error' = 0 (log /tmp/btc36c3_suite.log).
- POSITIVE CONTROL: nm shows 117 __ubsan_* symbols linked (incl.
  __ubsan_handle_type_mismatch_v1, _out_of_bounds_abort) — the binary
  is genuinely instrumented; a silent no-op build is excluded.
- Verdict: DISMISSED. Matrix cell filled: full unit suite x
  -fsanitize=undefined x clang-18.1.3 = 0 reports, matching the gcc
  cell post-22aa75a2eb. No clang-only UB surface in the suite.

### Exact commands
- cmake/ninja lines above; suite: UBSAN_OPTIONS=print_stacktrace=1:
  halt_on_error=0 build-clang-ubsan/bin/test_bitcoin (rc=0, 1128)
- control: nm build-clang-ubsan/bin/test_bitcoin | grep -c __ubsan
  -> 117

### Limitations / queue
- build-clang-ubsan removed after the run (disk at 100%, 1.5 GB
  reclaimed; recreate with the cmake line above, ~40 min cold).
- Remaining open cells: _GLIBCXX_ASSERTIONS, TSan subset,
  functional-suite-under-clang, warning-as-error CI note (c2).

## Rotation note
Cycle 3 complete; rotating per uber-goal policy. Not exhausted (3
open cells).

## Cycle 4 (2026-07-31): _GLIBCXX_ASSERTIONS full unit suite — 1128 cases, zero checked-container violations; DISMISSED

### Draw
RE-RANK draw 147 over the 5-cell queue: raw=12275778598515865734,
masked 3052406561661089926 -> idx 1 -> #36 _GLIBCXX_ASSERTIONS
(c2 queue). Branch: audit/cross-tool-c4 from 0ec6d5e210.

### Method
cmake -B build-glibcxx -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
-DCMAKE_CXX_FLAGS='-D_GLIBCXX_ASSERTIONS'; reconfigure AFTER the
branch checkout (the c3 stale-configure trap applies — caught it
this time); ninja bin/test_bitcoin (536 edges, ~34 min cold);
full suite.

### Result
- Suite: 1128 test cases, rc=0, "No errors detected", zero
  assertion/abort lines (log /tmp/btc36c4_suite.log).
- POSITIVE CONTROL: CMAKE_CXX_FLAGS cached with the define
  (CMakeCache.txt) and `ninja -t commands` shows
  -D_GLIBCXX_ASSERTIONS in the actual compile line — ninja's
  non-verbose log omits commands, so grep of the build log is NOT
  a flag-presence check (it printed 0; -t commands is the control).
- Verdict: DISMISSED. Matrix cell filled: full unit suite x
  gcc-13.3 x checked libstdc++ = 0 violations (no out-of-bounds
  vector/string access, no invalid iterator use anywhere the
  suite reaches).

### Limitations / queue
- build-glibcxx removed after the run (disk; recreate with the
  cmake line above, ~35 min cold).
- Remaining open cells: TSan subset, functional-suite-under-clang,
  warning-as-error CI note (c2).

## Rotation note
Cycle 4 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 5 (2026-08-01): TSan concurrency subset — suite green; 2 warnings = the suite's own intentional lock inversions; DISMISSED

### Draw
RE-RANK draw 156 over the 6-cell pool: raw=15079885369332575445,
masked 5856513332477799637 -> idx 1 -> #36 TSan subset (c2
queue). Branch: audit/cross-tool-c5 from 1796cff899.

### Method
cmake -B build-tsan -G Ninja RelWithDebInfo -DSANITIZERS=thread;
ninja bin/test_bitcoin; TSAN_OPTIONS=halt_on_error=0 subset run:
checkqueue_tests, threadpool_tests, scheduler_tests, sync_tests,
cuckoocache_tests, headers_sync_chainwork_tests.
BUILD NOTE: the in-tree capnp/kj fails to LINK under TSan
(kj::_::runCatchingExceptions undefined at final link — the
external project's objects don't carry the symbol set the
TSan-instrumented ipc objects expect); sidestepped with
-DENABLE_IPC=OFF for this subset (IPC has no TSan-relevant unit
coverage here; recorded as a build-system limitation, not
triaged further this cycle).

### Result
- Suite: all 6 concurrency suites green ("No errors detected").
- 2 TSan warnings, BOTH lock-order-inversion inside
  TestPotentialDeadLockDetected (sync_tests.cpp:18,84-99) — the
  test that DELIBERATELY inverts lock order to exercise the
  DEBUG_LOCKORDER throw ("potential deadlock detected: mutex1 ->
  mutex2 -> mutex1", the exact string it asserts). TSan reports
  the intentional inversion — by-design test constructs, not
  defects. Zero data-race reports anywhere.
- Verdict: DISMISSED. Matrix cell filled: concurrency suites x
  gcc-13.3 TSan (aarch64) = 0 real findings.

### Exact commands
- cmake/ninja/suite lines above; log /tmp/btc36c5_suite.log;
  TSan report excerpts in-cycle above.

### Limitations / queue
- build-tsan removed after the run (disk hit 535 MB free; 3.9 GB
  reclaimed; recreate with the cmake line, ~40 min cold).
- Full-suite TSan not run (subset per queue; the IPC link failure
  would also gate it).
- Remaining #36 cells: functional-suite-under-clang, warning-as-
  error CI note (c2).

## Rotation note
Cycle 5 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 6 (2026-08-01, draw 169, raw=13595585722515773213, masked 4372213685660997405, idx 1/4): functional-suite-under-clang 7/7 = gcc baseline; warning-as-error CI gap DISMISSED; campaign COMPLETE

### Draw
RE-RANK draw 169 over the 4-cell pool: raw=13595585722515773213,
masked 4372213685660997405 -> idx 1 -> #36 functional-under-clang
(c2 queue). Branch: audit/cross-tool-c6 from 1796cff899-lineage
(audit/cross-tool-c5 tip).

### H1: clang-18-built bitcoind diverges behaviorally from gcc-13.3
on the functional suite (optimizer/codegen-visible). DISMISSED.
- Build: cmake -B build-clang-func -G Ninja Release,
  clang-18/clang++-18 18.1.x, -g0 (disk), ENABLE_WALLET/IPC off,
  tests off; bitcoind+bitcoin-cli only; ccache 58% hit.
  Version line: v31.99.0-7599fd612942 (HEAD journal lineage).
- Runner quirk recorded: test_runner.py hardcodes
  test/config.ini (its --configfile only passes through); point
  test/config.ini at each build's config in turn, delete after.
- Subset (consensus/P2P core, MiniWallet-raw only):
  feature_block, p2p_compactblocks, rpc_blockchain (v1+v2),
  feature_reindex, p2p_segwit, feature_csv_activation.
- clang run: ALL 7 Passed, 217s accumulated, 90s wall
  (/tmp/btc36c6). gcc build-before same-day baseline: ALL 7
  Passed, 217s, 89s wall (/tmp/btc36c6g). Per-test times within
  1s. Zero behavioral divergence, zero clang-only failures.

### H2 (c2 queue, warning-as-error CI note): clang-only warnings
can slip CI. DISMISSED — with a fork-hygiene corollary.
- ci/test/03_test_script.sh:118: BITCOIN_CONFIG_ALL carries
  -DCMAKE_COMPILE_WARNING_AS_ERROR=ON for EVERY job; ci.yml runs
  clang jobs (asan, fuzz, fuzz_with_msan, iwyu, msan, tidy, tsan).
  No silent-warning channel exists; the literal -Werror in
  00_setup_env_native_previous_releases.sh is only because that
  job pins an older CMake lacking the option (comment at :18).
- Corollary (fork CI hygiene, not a Core defect): the 3 benign
  c2 -Wunneeded-* helpers (signet.cpp:83, txgraph.cpp:41/57) fire
  only when FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION/
  ABORT_ON_FAILED_ASSUME is absent (src/util/check.h:21-35) and
  that macro is per-target, so the non-fuzz clang jobs would see
  them under Werror; remedy if wanted is [[maybe_unused]] on the
  helpers (c2 already recorded). No fix commit: benign by design,
  out of minimal-defect scope.

### Verdict
DISMISSED both cells. #36 matrix complete: gcc/clang unit
differential (c2), UBSan (c3), _GLIBCXX_ASSERTIONS (c4), TSan
(c5), functional-under-clang + Werror CI note (c6).

### Campaign #36: COMPLETE

### Exact commands
- cmake/ninja lines above; test_runner subset lines above;
  test/config.ini copy-delete dance recorded.

### Limitations
- Subset, not the full functional suite (disk- and time-bounded;
  the 6 tests were chosen for consensus/P2P density).
- build-clang-func kept for now (Release -g0, ~small); delete on
  next disk squeeze. clang version string: Ubuntu clang 18.1.x.

## Cycle 339 (2026-08-04) — r158 #36: delta-code coverage matrix (post-rebase inventory)

Reopen PASS (193+26 upstream commits = new code since the last matrix
cycle). Coverage of the inherited delta code on this host:

- ASan Debug (build-after, -O0 -ftrapv): unit battery 229 cases /
  1.29M assertions green incl. crypto/blockencodings/coins/http/
  torcontrol/rpc/descriptor/chainstatemanager suites.
- UBSan+ASan+fuzzer (build_fuzz, RelWithDebInfo): smoke 2000 runs x
  {partially_downloaded_block, http_request, torcontrol, rpc, tx_pool,
  coins_view, integer} — all DONE, zero artifacts.
- Functional (ASan bitcoind): interface_http, feature_index_prune,
  p2p_getdata, p2p_compactblocks — Tests successful (incl. upstream's
  new empty-getblocktxn subtest).
- Release (NDEBUG): NOT RUN on the new tree — build-before is stale
  (pre-rebase) and disk is at 780M free (no new tree possible).
  Assert-erasure divergence is therefore UNVERIFIED for the delta code.
  Condition to close: disk headroom for a release tree (or reuse of
  build-before after a clean).
- clang-tidy/cppcheck/cbmc/klee: absent on host (rechecked).

Deferred-scope record per the matrix goal: delta code is covered by
3 of 5 verifier forms; release-NDEBUG and static analysis remain
open conditions (blocked by disk/tooling, not by choice).
