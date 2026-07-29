# Floating-Point, Sanitizer, and Fuzzer-Exclusion Audit

## Cycle 47

- Goal: `98`, `float-sanitizer-fuzz-exclusions`
- Selector: `shuf -i 0-98 -n 1` -> `98`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- HEAD at cycle start: `e75edc5632349f7e63901cb136077af07e90893c`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Entry divergence: `origin/master...HEAD = 2 863`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV SHA-256: `ee094e408ea668c95129c7a9902d4878e9c303bf9e28e1a481c5af46114725bf`

The tracked and staged worktree was clean at entry. Existing untracked agent artifacts and `test/cache` were preserved. No relevant daemon, test, fuzz, sanitizer, Valgrind, or profiling process was running. The only untracked crash file created during this cycle was an empty libFuzzer artifact from the intentional oracle mutation; it was removed after the mutation was restored.

## Scope and Prior-Finding Check

The three linked passes were reviewed against the existing sanitizer matrix, `doc/fuzzing-findings.md`, the float/locale/policy-estimator fuzz targets, sanitizer suppression history, and the current CI sanitizer configurations. Earlier fee-estimator NaN and negative-threshold findings were treated as closed and were not reported again. The existing `float` target already covers raw IEEE encodings, signed zero, subnormals, infinities, max finite values, quiet/signaling NaNs, NaN payload canonicalization, locale-independent conversion, and arbitrary raw-bit decoding.

Active source-level sanitizer exemptions are limited to the SHA256 SSE4 inline assembly, the Clang-before-11 Minisketch CLMUL MSan workaround, and benchmark-only nanobench timing/counter helpers. The UBSan, TSan, LSan, and Valgrind suppression files were checked for current scope and historical rationale. The current sanitizer CI uses ASan, integer/undefined, float-divide-by-zero, MSan, and TSan configurations; no new broad suppression or disabled production target was identified.

## Sanitizer Resurrection: SHA256 SSE4

`src/crypto/sha256_sse4.cpp` carries the `no_sanitize("address")` attribute on `Transform()` with a comment covering both compiler contracts. History identifies `fedeff7f201df0206eefb744ad125e44a63a3ea0` as the GCC extension of the earlier Clang-only workaround. The source-level hypothesis was falsifiable: removing only the attribute should either expose a currently fixed problem or reproduce the documented compiler/runtime failure.

The current Clang 19 ASan fuzz build was mutated by removing only the guard. `cmake --build build_fuzz_asan_clang19 --target fuzz -j2` failed while compiling `sha256_sse4.cpp`, with repeated `expected relocatable expression` diagnostics from the inline assembly at the `shl $0x6,%2` site. Restoring the guard rebuilt the target successfully. A current-source Clang ASan/UBSan `SHA256AutoDetect(USE_SSE4)` and `SHA256AutoDetect(USE_ALL)` probe then exited 0 with:

```text
sse4(1way);sse41(4way)
x86_shani(1way;2way)
```

The GCC side was tested independently without rebuilding the full dependency graph. A current-source GCC 12.2 ASan/UBSan object with the guard removed compiled successfully, but the linked probe failed exactly during the self-test:

```text
AddressSanitizer:DEADLYSIGNAL
SEGV ...
#0 sha256_sse4::Transform(...) src/crypto/sha256_sse4.cpp:52
#1 SelfTest ... src/crypto/sha256.cpp:538
#2 SHA256AutoDetect(...) src/crypto/sha256.cpp:688
SUMMARY: AddressSanitizer: SEGV src/crypto/sha256_sse4.cpp:52
```

Compiling the current source with the guard and linking the same probe exited 0 with the two output lines above. A full `build_fuzz_ipc_asan_gcc --target fuzz` refresh was started to cross-check the existing GCC tree, but it was interrupted at 21% after several minutes because it was rebuilding the entire dependency graph; this is a resource limitation, not a source failure. The standalone current-source test is the authoritative GCC result. Verdict: **confirmed necessary workaround; no production change**. The attribute must not be removed or broadened into a blind suppression; the comment and guard match independently reproduced compiler and runtime failures.

## Floating-Point Edge Pass

The focused unit and fuzz paths were exercised:

- `build_unit_clang19/bin/test_bitcoin --run_test=serfloat_tests --log_level=test_suite`: 3 cases passed, including raw encoding contracts and NaN canonicalization.
- `build_unit_clang19/bin/test_bitcoin --run_test=policyestimator_tests --log_level=test_suite`: 6 cases passed, including invalid threshold and corrupted estimate-file vectors.
- `build_unit_clang19/bin/test_bitcoin --run_test=util_tests --log_level=test_suite`: 78 cases passed, including money formatting/parsing and locale-independent formatting.
- `FUZZ=float ... build_fuzz_asan_clang19/bin/fuzz -runs=100000 -seed=98047 -max_len=4096`: exit 0, no ASan/UBSan diagnostic, `cov 352`, `ft 414`, corpus `20/162` bytes, approximately 5,000 executions/second, RSS 1427 MiB.

The fuzzer's hard-coded exceptional-value cases are reachable and have a sensitive oracle. A temporary mutation changed the canonical-NaN assertion from `reencoded == DOUBLE_CANONICAL_NAN` to `reencoded == 0`. With the same ASan/UBSan build and seed `98047`, a one-run execution failed immediately at `float.cpp:34` with the minimized empty input artifact. The mutation was restored, the fuzz target rebuilt, and the 100,000-run stress pass above completed cleanly.

The locale target has one deliberate early return when the selected locale is unavailable; it is an environment precondition, not a floating-point exclusion. The policy-estimator target has an explicit invalid-result return for an unavailable estimate; its postconditions and state-preservation assertions remain active. No `Assume`, sanitizer catch, or special-value clamp in the selected float paths was found to discard a valid production domain.

## Verdict and Handoff

**Confirmed:** removing the SHA256 SSE4 ASan exemption still causes a Clang compile failure and a GCC runtime SEGV in the autodetection self-test; the existing guard is necessary. **No new floating-point or fuzzer-exclusion defect:** current raw exceptional-value coverage, locale behavior, policy-estimator invalid inputs, sanitizer diagnostics, and oracle sensitivity all passed. No production or test source change is justified. The full GCC dependency rebuild remains incomplete because of cost; Valgrind, dudect, and several static tools remain unavailable. The next cycle must draw a distinct goal/cell and repeat the entry gate, prior-finding search, and handoff update.

## Cycle 99: floating-point, sanitizer, and fuzzer-exclusion audit

### Selection and gate

- Goal index: `98`, `float-sanitizer-fuzz-exclusions`, selected with exact
  `shuf -i 0-98 -n 1` -> `98` after Cycle 98 closed goal 8; no reroll was
  needed because the draw was distinct from the just-closed goal.
- Branch: `uber-cycle-99-float-sanitizer-fuzz-exclusions-20260729`.
- HEAD at cycle start: `b5525c94fe6c3c14c1f498c04c0a9e8d27a84025`.
- `origin/master`: `9b38d077f894d27ea76413b1db1cb040e25dc296`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Start divergence: `origin/master...HEAD = 29 989`.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Fresh `git fetch origin master`, tracked/index cleanliness, and
  `git diff --check` passed. Existing untracked artifacts remain preserved
  and outside cycle commits.
- The unrelated `test_bitcoin --run_test=wallet_tests --log_level=test_suite`
  child PID 777094, parent Codex PID 725042, remained active from the prior
  gate. It was not started or terminated by this cycle and is an external
  process limitation.

### Scope and prior-finding exclusions

Inventory exceptional floating inputs, sanitizer attributes/suppressions and
CI exclusions, fuzzer `Assume`/catch/early-return gates, and failure-state
contracts. This is a fresh follow-up to Cycle 47. Exclude its confirmed
SHA256 SSE4 `no_sanitize("address")` necessity, the existing raw IEEE float
target's exceptional-value vectors and NaN oracle, the locale-unavailable
environment return, and policy-estimator invalid-threshold behavior unless a
new source change or independent reachability evidence changes their contract.

Initial queue:

1. Current-tree sanitizer attributes and suppressions added or widened after
   the earlier audit, especially target-specific exclusions that hide a
   production path or a new compiler/architecture failure.
2. Fuzzer gates around float parsing, JSON/config/RPC conversion, locale
   handling, serialization, and policy/fee code; determine whether each gate
   is a valid precondition or discards a reachable production state.
3. Float/double values in current public interfaces and conversion helpers,
   including ordering, hashing, integer casts, formatting, and error outputs.
4. Sanitizer CI matrix and excluded targets/categories, with a minimal
   resurrection build or mutation proving whether a missing diagnostic is
   actionable.

For each candidate, record the mathematical/value domain and expected
failure-state invariant before testing. Preserve exact raw bits, minimized
inputs, sanitizer traces, suppression history, and mutation results. Do not
introduce floating point into consensus or secret-dependent cryptography.

### Evidence ledger

#### Sanitizer and fuzzer-exclusion inventory

The current-tree inventory found three distinct classes of exclusions:

- `src/crypto/sha256_sse4.cpp` keeps the previously audited
  `no_sanitize("address")` attribute on the inline-assembly transform. Cycle
  47 already reproduced both the Clang compile failure and the GCC runtime
  failure when it was removed; this cycle excluded it as a closed finding.
- `src/minisketch/src/fields/clmul_common_impl.h` conditionally applies
  `no_sanitize("memory")` only for Clang versions before 11 when MSan is
  active, because those compilers cannot reason through the CLMUL intrinsic.
  The branch is inactive on the installed Clang 14 and is a vendored,
  compiler-specific compatibility guard rather than a current production
  path exclusion. No source change was justified.
- `src/bench/nanobench.h` has integer/undefined sanitizer attributes around
  benchmark measurement and timing helpers. These functions are benchmark
  infrastructure, not production operations; removing the attributes would
  report arithmetic in the measurement framework rather than a repository
  behavior. No source change was justified.

The active UBSan, TSan, LSan, and Valgrind suppression files were inspected.
The current CI configurations still request ASan, float-divide-by-zero,
integer, undefined, MSan, and TSan coverage. No new broad suppression,
disabled production target, or fuzzer catch that hides a valid input was
found. The locale fuzzer's unavailable-locale return remains an explicit
environment precondition, and the policy-estimator fuzzer's deserialization
return stops only after marking its input invalid.

#### Confirmed finding: non-finite persisted fee-estimator decay

The recent fee-estimator hardening commits `c32fa272dd` (corrupt vector
rejection), `4e1e3c2a99` (failed-read atomicity), and `70ccf49cd9` (invalid
threshold rejection) were used as the history seed. No prior journal entry
covered the scalar `decay` field.

`TxConfirmStats::Read` decodes the persisted IEEE-754 value at
`src/policy/fees/block_policy_estimator.cpp:477`. Before this cycle it
accepted the value only when `decay <= 0` and `decay >= 1` were both false.
For a quiet NaN, both comparisons are false, so a corrupt current-format
`fee_estimates.dat` could install a NaN decay into one of the three estimator
statistics objects. Subsequent moving-average updates multiply counters by
that NaN, and a later write reaches the existing finite-vector `Assume`; fee
queries can also operate on poisoned estimator state. This is a local
persisted-file corruption/tampering path, not a remote or consensus path, but
it violates the parser's stated `(0, 1)` decay contract.

The independent regression harness extended the existing current-format file
writer in `src/test/policyestimator_tests.cpp` and exercised `-infinity`, NaN,
and `+infinity` decay encodings. With the production check still unchanged,
the exact command below exited 201 and identified the new assertion at test
line 454:

```text
CCACHE_DISABLE=1 cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j4
TMPDIR=/data/my_storage/tmp/cycle99-decay-before /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=policyestimator_tests/reject_corrupt_fee_estimate_file_vectors --log_level=test_suite --catch_system_errors=no --color_output=false
```

Key failure:

```text
./test/policyestimator_tests.cpp(454): error: check !ReadEstimatorFile(corrupt_path, estimator_path) has failed
*** 1 failure is detected in the test module "Bitcoin Core Test Suite"
```

The minimal production fix is the existing domain check plus an explicit
finiteness check:

```cpp
if (!std::isfinite(decay) || decay <= 0 || decay >= 1)
```

The test now rejects all three non-finite encodings while retaining the
existing valid-file and malformed-vector cases. The failure-before run was
restored to this source before validation, so the regression is sensitive to
the production guard rather than only testing the new fixture writer.

#### Verification

- `git diff --check`: passed.
- `CCACHE_DISABLE=1 cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j4`: passed.
- `TMPDIR=/data/my_storage/tmp/cycle99-decay-after /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=policyestimator_tests/reject_corrupt_fee_estimate_file_vectors --log_level=test_suite --catch_system_errors=no --color_output=false`: exit 0; all cases passed.
- `TMPDIR=/data/my_storage/tmp/cycle99-policy-tests /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=policyestimator_tests --log_level=test_suite --catch_system_errors=no --color_output=false`: exit 0; all 6 policy-estimator cases passed.
- `TMPDIR=/data/my_storage/tmp/cycle99-full-unit /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --log_level=message --catch_system_errors=no --color_output=false`: exit 0; all 1,208 unit cases completed with `*** No errors detected`.
- A dedicated Clang libFuzzer/ASan/UBSan configure was attempted in
  `/data/my_storage/tmp/cycle99-fuzz-build`. It stopped before generation at
  `cmake/module/CheckCXXFeatures.cmake:32`: installed Clang 14 lacks the
  aggregate class-template-argument-deduction feature required by this
  checkout. No sanitizer claim is made from that unavailable build.

#### Verdict and handoff

**Confirmed and fixed:** non-finite `decay` in a current-format fee-estimator
file bypassed the previous comparisons and could poison persisted estimator
state. The parser now rejects all non-finite values before installing the
temporary statistics object, and the regression test proves the old behavior
failed and the new behavior passes. The fix is limited to the parser and its
focused test; no file-format version change is needed.

No other current sanitizer resurrection or fuzzer-exclusion candidate met
the evidence bar. The installed compiler limitation prevented a fresh
libFuzzer/sanitizer run; the unrelated PID 777094 wallet test remained active
and was not touched. Next cycle must draw a distinct goal/cell, repeat the
entry gate and prior-finding search, and retain this finding as a closed
recurrence seed.
