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
