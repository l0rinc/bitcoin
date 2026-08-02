# Floating-Point, Sanitizer, and Fuzzer-Exclusion Audit

## Cycle 295: preserve floating-point RPC fuzzer arguments

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `98`
- Selected goal: `float-sanitizer-fuzz-exclusions`
- Branch: `uber-cycle-295-float-sanitizer-fuzz-exclusions-20260802`
- Start HEAD: `e8cd67286f2f8373d0e1a60ed5931e7a243ad45e`
- `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Entry divergence (`origin/master...HEAD`): `45 1380`
- Catalog, prompt, TSV, and protocol hashes matched the fixed gate values.
- The tree had no tracked changes at entry. Existing untracked agent artifacts
  were preserved. The seven protected long-running test processes remained
  alive and untouched.

### Scope and prior-finding exclusions

The previous Goal 98 cycles closed raw IEEE exceptional-value coverage, the
locale-unavailable precondition, policy-estimator non-finite input handling,
the necessary SHA256 SSE4 sanitizer attribute, the overly broad strprintf
fuzzer guards, and bloom integer-overflow suppressions. The current scan
therefore concentrated on remaining sanitizer exclusions, float conversions,
and fuzzer paths that transform numeric values before they reach a production
parser. No consensus or libsecp256k1 production path in the current float
inventory used floating point.

### Confirmed finding: RPC fuzzer rounded generated doubles before JSON parsing

`src/test/fuzz/rpc.cpp` generated scalar RPC floating-point arguments with
`strprintf("%f", value)`. This fixed the representation at six digits after
the decimal point before passing the string to `RPCConvertValues`. The
converter treats the `estimaterawfee` threshold parameter as JSON, and the
RPC implementation rejects thresholds only after parsing when the value is
outside `[0, 1]` (`src/rpc/client.cpp` parameter mapping and `ParseParam`,
`src/rpc/fees.cpp:168`). Thus the fuzzer did not preserve the value it claimed
to generate at a boundary-sensitive RPC interface.

The before-fix converter probe used the same `strprintf` and
`RPCConvertValues` path. Its relevant output was:

```text
0.99999939999999998 -> 0.999999 -> 0.99999899999999997
0.99999950000000004 -> 1.000000 -> 1
1.0000004 -> 1.000000 -> 1
1.2345678899999999 -> 1.234568 -> 1.2345680000000001
4.9406564584124654e-324 -> 0.000000 -> 0
```

The `1.0000004` case is a direct oracle failure: the generated value is
outside the RPC threshold domain, but the fuzzer submitted the rounded value
`1`, which passes the production check. The subnormal case also demonstrates
that `%f` erased a distinct valid numeric input by converting it to zero.

The smallest harness-only fix uses `%g` with
`std::numeric_limits<double>::max_digits10`, which is sufficient to round-trip
every finite `double` through a decimal JSON number. A post-fix scratch probe
using the same converter produced:

```text
0.999999 -> 0.99999939999999998 -> 0.99999939999999998 rejected=0
1 -> 0.99999950000000004 -> 0.99999950000000004 rejected=0
1 -> 1.0000004 -> 1.0000004 rejected=1
1.23457 -> 1.2345678899999999 -> 1.2345678899999999 rejected=1
4.94066e-324 -> 4.9406564584124654e-324 -> 4.9406564584124654e-324 rejected=0
```

This is a fuzz-oracle defect, not a production RPC defect. The standard
`ConsumeFloatingPoint<double>()` helper samples the full finite double range;
an independent million-sample probe found no values in `(-2, 2)`, so the
usual generated distribution rarely reaches the threshold boundary. That
limitation is recorded rather than hidden: the formatting fix preserves
boundary behavior whenever such a value is generated, but this cycle does not
claim that the existing generator efficiently explores that domain. The later
unmerged RPC-target redesign was treated as review precedent only and was not
used as an oracle.

### Verification

- The source change added `<limits>` and replaced the `%f` conversion with
  `strprintf("%.*g", std::numeric_limits<double>::max_digits10, ...)`.
- `git diff --check` passed.
- Rebuild from the current tree:
  `env TMPDIR=/data/my_storage/tmp/cycle295-rpc-build-tmp CCACHE_DIR=/data/my_storage/tmp/cycle295-rpc-ccache ninja -C /data/my_storage/tmp/cycle188-coverage-build fuzz -j2`
  passed all three final steps, including recompilation of `rpc.cpp` and
  relinking `bin/fuzz`.
- Final fixed-run RPC fuzz command:
  `env FUZZ=rpc TMPDIR=/data/my_storage/tmp/cycle295-rpc/runtime-final /data/my_storage/tmp/cycle188-coverage-build/bin/fuzz -runs=10000 -seed=29502 -max_len=4096 -rss_limit_mb=4096 -timeout=5 -artifact_prefix=/data/my_storage/tmp/cycle295-rpc/artifacts-final/ -print_final_stats=1 -verbosity=0 --testdatadir=/data/my_storage/tmp/cycle295-rpc/testdata-final`
  completed 10,000 executions, added 164 units, and reached 1,297 MiB peak
  RSS with no crash, timeout, sanitizer diagnostic, or artifact. It exercised
  the RPC setup, conversion, and multiple scalar-argument paths; the direct
  converter probe supplies the targeted formatting oracle because the random
  run did not reliably select the rare small-double boundary.
- A preceding eight-second seeded run completed 28,509 executions with 352
  new units and no failure. Its initial retry failed only because a typo made
  `TMPDIR` nonexistent; the corrected run used a dedicated `/data` temporary
  directory and completed normally.
- The source was rebuilt after the formatting-only adjustment; no production
  unit-test behavior was changed.

### Verdict and handoff

**Confirmed and fixed:** the RPC fuzz harness rounded generated floating-point
arguments to six fractional digits, allowing invalid boundary values to reach
JSON-RPC code as different valid values and collapsing small values to zero.
The source/evidence commit must include only `src/test/fuzz/rpc.cpp` and this
journal update, authored as `Lőrinc <pap.lorinc@gmail.com>`. The separate uber
state-close commit records the exact source commit and the next Goal 98 queue.

Next Goal 98 cells: construct a deterministic input that selects the float
argument lambda and reaches a boundary-sensitive JSON parameter; then inspect
the remaining production `TrafficGraphWidget` zero-interval path and current
sanitizer/fuzzer exclusions. Revisit the persisted `AddrInfo::nAttempts`
candidate only after these higher-evidence cells.

## Cycle 282: resurrect bloom integer diagnostics

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `98`
- Selected goal: `float-sanitizer-fuzz-exclusions`
- Branch: `uber-cycle-282-float-sanitizer-fuzz-exclusions-20260802`
- Start HEAD: `13000d172edb256d511836811a246632bad0473b`
- `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Entry divergence (`origin/master...HEAD`): `45 1354`
- Uber-state SHA-256 at gate: `d0d00df03c527185ac2950da398fc202b84b17ba7123ecee97b9ec8a308fd0e5`
- Catalog, prompt, TSV, and protocol hashes matched the fixed gate values.
- `git fetch origin master`, tracked/index cleanliness, and `git diff --check` passed at entry. Persistent untracked agent artifacts were preserved. The seven protected long-running test processes remained alive and untouched.

### Scope and prior-finding exclusions

The earlier Goal 98 cycles closed raw IEEE exceptional-value coverage, the
locale-unavailable precondition, policy-estimator non-finite input handling,
the necessary SHA256 SSE4 sanitizer attribute, and the overly broad strprintf
fuzzer guards. This cycle therefore concentrated on current sanitizer
suppression scope and production float-adjacent arithmetic that had not yet
been independently checked. The current tree still had three bloom-specific
`unsigned-integer-overflow` suppressions in
`test/sanitizer_suppressions/ubsan`.

### Confirmed finding: bloom wraparound was hidden by per-symbol suppressions

The current `src/common/bloom.cpp` computed the MurmurHash seed as
`nHashNum * 0xFBA4C795 + nTweak` in `unsigned int`, and formed rolling filter
generation masks as `0 - uint64_t(...)`. These operations intentionally wrap,
but they caused integer sanitizer diagnostics to be suppressed for
`CBloomFilter::Hash`, `CRollingBloomFilter::insert`, and `RollingBloomHash`.
The historical fix `6ea393cd6f5a28d16f86e9c7cddb0912d6124cff` was present in a
separate local history line but was not an ancestor of this cycle branch; the
current source still contained the pre-fix expressions.

The independent reproduction used a fresh Clang 19 build configured with
`-DSANITIZERS=integer`, `-DENABLE_IPC=OFF`, and wallet/GUI/bench disabled. The
baseline `bloom_tests` suite passed in the existing UBSan build. After copying
the repository suppression file to scratch and removing only the three bloom
entries, the focused command

```text
TMPDIR=/data/my_storage/tmp/cycle282-bloom-int-run UBSAN_OPTIONS="suppressions=/data/my_storage/tmp/cycle282-bloom-int-run/ubsan-no-bloom:halt_on_error=1:print_stacktrace=1:report_error_type=1" /data/my_storage/tmp/cycle282-bloom-int/bin/test_bitcoin --run_test=bloom_tests/bloom_create_insert_serialize --log_level=test_suite --report_level=short --color_output=false
```

stopped at `src/common/bloom.cpp:50` with
`unsigned integer overflow: 2 * 4221880213`, reached from the existing
`CBloomFilter::insert` test. This is a deterministic first-invalid-operation
trace, not a theoretical warning. The same test also verifies the exact
serialized filter bytes, so its oracle is sensitive to seed changes.

The fix computes each hash seed in `uint64_t` and explicitly truncates to the
protocol's 32-bit seed, preserving modulo-2^32 behavior. It replaces the
mask subtraction with conditional all-zero/all-one masks. The three bloom
symbol suppressions were removed. No floating-point contract or fuzzer gate
was changed.

### Verification

- Incremental rebuild after the patch:
  `CCACHE_DIR=/data/my_storage/tmp/cycle282-bloom-ccache TMPDIR=/data/my_storage/tmp/cycle282-bloom-tmp ninja -C /data/my_storage/tmp/cycle282-bloom-int test_bitcoin -j2`: passed, 6/6 steps.
- The same suppression file with only the bloom entries removed was used after
  the patch. The full `bloom_tests` suite passed all 14 cases and 37,687
  assertions under `-fsanitize=integer`, with no bloom diagnostic.
- Existing exact serialization vectors passed, including the two hash-seed
  cases; rolling-filter retention, reset, and false-positive checks also
  passed. This independently checks behavior rather than merely removing a
  warning.
- `git diff --check` passed after the source/suppression edit. No unrelated
  tracked files were changed.

### Verdict and handoff

**Confirmed and fixed:** integer sanitizer coverage for bloom hashing and
rolling generation was hidden by suppressions even though the wraparound was
intentional and could be made explicit without changing behavior. The cycle
source commit must include `src/common/bloom.cpp`, the suppression removal,
and this journal. The next cycle must close the uber state, draw a distinct
goal with the exact selector, and repeat the fresh gate; do not reopen the
already closed float, SHA256, locale, policy-estimator, or strprintf cells
without new evidence.

## Cycle 178 Identity and Fresh Gate

- Draw command: shuf -i 0-98 -n 1
- Draw: 98
- Selected goal: float-sanitizer-fuzz-exclusions (Floating-point edge
  values, sanitizer resurrection, and fuzz-exclusion audit)
- Branch: uber-cycle-178-float-sanitizer-fuzz-exclusions-20260731
- Start HEAD: 0e471c4bf955cb03150b10af2a4dc844b01f371b
- origin/master: 67efced1fc83a0b7215cc1513e7c4754fee0f12f
- Merge-base: a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b
- Divergence (origin/master...HEAD): 42 1141
- Catalog SHA-256: 5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8
- Prompt SHA-256: 10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec
- Goals TSV SHA-256: babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb
- Protocol SHA-256: 954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc
- Uber-state SHA-256 at gate: f01e6011ac059dcd35dad2d8666eeeb90a0c4838f872527e59f93360df35eb55
- Tracked/index state was clean at the gate. Persistent untracked agent
  artifacts, JavaScript scan files, and test/cache were preserved and will
  remain outside cycle commits.
- Preserved unrelated long-running tests: PIDs 777094 and 956381; neither
  was modified.
- Storage gate: approximately 99 MiB free on root and 50 GiB free on /data.

## Cycle 178 Scope and Prior-Finding Exclusions

Goal 98 was previously selected in Cycle 47 and Cycle 99. Cycle 47
confirmed that removing the SHA256 SSE4 no_sanitize(address) attribute
causes a current Clang compile failure and a GCC ASan autodetection crash;
that guard is closed. Cycle 99 fixed the persisted fee-estimator NaN decay
acceptance in commit 513c5e4381 and closed the raw float target's IEEE
vectors, NaN oracle, locale-unavailable precondition, and policy-estimator
invalid-threshold cells. Those findings are evidence seeds, not targets for
repetition unless new source or compiler evidence changes their contract.

This cycle starts a new current-tree pass over sanitizer attributes and
suppression scope, fuzzer gates outside the closed raw-float oracle,
floating-point conversion paths in public/config/RPC/policy interfaces, and
the sanitizer CI target/category matrix. The trust boundaries are
untrusted serialized/config/RPC values, persisted policy data, developer
test inputs, and compiler/tool diagnostics. Floating point must not be
introduced into consensus or secret-dependent cryptographic decisions.

Initial queue:

1. Diff current sanitizer attributes, suppressions, CI exclusions, and
   compiler-version guards against the Cycle 99 inventory; identify a
   target or diagnostic that is currently hidden without an independently
   reproduced tool limitation.
2. Trace fuzzer Assumptions, catches, early returns, clamps, and ignored
   errors around JSON/config/RPC number parsing, serialization, locale
   handling, policy values, and public numeric utilities. Classify each as a
   real precondition or an oracle gap.
3. Inventory production float/double values and conversions not covered by
   the closed raw-float target, prioritizing ordering, hashing, integer
   casts, formatting, persistence, and error-state behavior.
4. Compare the documented sanitizer matrix with generated CI targets and
   build scripts; resurrect one excluded path only when a deterministic
   current compiler/build experiment can prove a missed diagnostic or a
   necessary compatibility guard.

Each candidate needs an expected value domain and failure-state invariant
before testing, a source/history trace, a minimal reproducer or static/tool
proof, and an independent verdict. Preserve raw bits, minimized inputs,
sanitizer traces, suppression rationale, and rejected hypotheses.

## Cycle 178 Findings

### Sanitizer and CI matrix re-check

The tracked sanitizer attributes remain limited to the three previously
identified classes: the x86 SHA256 inline-assembly ASan workaround in
`src/crypto/sha256_sse4.cpp`, the pre-Clang-11 Minisketch CLMUL MSan workaround
in `src/minisketch/src/fields/clmul_common_impl.h`, and benchmark-only integer
and undefined-behavior exclusions in `src/bench/nanobench.h`. The current
suppression files contain only the documented dependency, test-deadlock,
intermittent-race, and intentional-arithmetic entries. No new broad source
attribute, disabled production fuzz target, or suppression masking a current
diagnostic was found.

The CI configurations still cover `address,float-divide-by-zero,integer,undefined`
for the native ASan job; `fuzzer,address,undefined,float-divide-by-zero,integer`
for the native libFuzzer job; separate `memory` jobs for MSan; and `thread` for
TSan. The fuzzer runner uses the qa-assets corpus and a fixed `--empty_min_time`
stop condition. The matrix therefore has no sanitizer-resurrection finding in
this cycle.

### Confirmed finding: strprintf fuzzer guards filtered safe literal text

The current `src/test/fuzz/strprintf.cpp` guards were seeded by history
`cc668d06fb7` and `470e2ac602e` and the upstream tinyformat issue 70. Before
the fix, the first guard counted every digit in the entire format string and
looked for `$` and `*` anywhere. The later guards looked for `c` and `*`
anywhere after a percent sign. Consequently, these safe inputs were discarded
before the floating-point or integer formatting cases even though tinyformat
formatted them successfully with a normal `double`:

```text
digits 1234567 %f    filtered by the old large-digit guard
cash $1 * %f         filtered by the old positional-star guard
literal * %f         filtered by the old variable-width guard
literal c %f         filtered by the old character-conversion guard
```

The independent probe command was:

```text
g++ -std=c++20 -O0 -I src -x c++ -o /data/my_storage/tmp/cycle178-float-strprintf-probe - <<'EOF'
...tinyformat probe over the four inputs and historical crash forms...
EOF
/data/my_storage/tmp/cycle178-float-strprintf-probe
```

It reported `actual-safe=1` for each literal-text input while the applicable
old guard reported `old-before=1` or `old-after=1`. The historical large-width,
positional-star, `%c`, and `*` seeds were kept as exclusions.

The fix adds `InspectFormatString`, a small parser matching tinyformat's
percent escaping, positional argument prefix, flags, width, precision,
length modifiers, and conversion position. Large numeric fields and
positional variable-width fields remain excluded before all type cases;
variable width/precision and the actual `c` conversion remain excluded only
from the floating/integer cases that can trigger the known UB. Literal `$`,
`*`, `c`, and digits no longer suppress unrelated format coverage. Production
callers audited in `src/index/base.cpp`, `src/netbase.cpp`, and wallet logging
use `ConstevalFormatString`; this is a fuzz-harness oracle/coverage defect,
not an attacker-controlled production format-string path.

### Verification

- `git diff --check`: passed.
- Clang fuzz build:
  `CCACHE_DIR=/data/my_storage/tmp/cycle178-fuzz-ccache TMPDIR=/data/my_storage/tmp/cycle178-fuzz-tmp cmake --build /data/my_storage/tmp/cycle131-build-libfuzzer --target fuzz -j2`: passed; the changed target compiled and linked.
- Clang ASan/UBSan fuzz run:
  `FUZZ=str_printf ASAN_OPTIONS=abort_on_error=1:detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz -max_total_time=5 -rss_limit_mb=768 -max_len=64 /data/my_storage/tmp/cycle178-fuzz-seeds`: exit 0, `Done 15664 runs in 6 second(s)`, peak RSS 469 MiB, coverage 1738, no sanitizer report.
- Focused unit suite:
  `/data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=util_string_tests --log_level=test_suite --report_level=short --color_output=false`: exit 0, 4 cases and 192 assertions passed. The suite was run with ASan/UBSan abort options enabled.
- GCC fuzz build:
  `CCACHE_DIR=/data/my_storage/tmp/cycle178-gcc-ccache TMPDIR=/data/my_storage/tmp/cycle178-gcc-tmp cmake --build /data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-fuzz-gcc --target fuzz -j2`: exit 0; all 282 build/link steps completed. Two unrelated pre-existing `-Woverloaded-virtual` warnings appeared in `http_request.cpp` and `pcp.cpp` with that non-Werror configuration.
- GCC direct corpus run:
  `FUZZ=str_printf /data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-fuzz-gcc/bin/fuzz /data/my_storage/tmp/cycle178-fuzz-seeds/literal-digits /data/my_storage/tmp/cycle178-fuzz-seeds/literal-symbols /data/my_storage/tmp/cycle178-fuzz-seeds/literal-star /data/my_storage/tmp/cycle178-fuzz-seeds/literal-c /data/my_storage/tmp/cycle178-fuzz-seeds/large-width /data/my_storage/tmp/cycle178-fuzz-seeds/positional-star /data/my_storage/tmp/cycle178-fuzz-seeds/variable-width`: exit 0, `str_printf: succeeded against 7 files in 0s.`
- Historical danger seeds were separately executed by the Clang fuzzer; large-width, positional-star, `%c`, and variable-width inputs all completed without `ERROR`, `runtime error`, or `SUMMARY` output.

### Cycle 178 verdict and handoff

**Confirmed and fixed:** the strprintf fuzz harness used whole-string
substring/count checks for format-specifier hazards, dropping legitimate
literal-text cases from floating-point and integer exploration. The new
specifier-aware inspection retains the historical crash/UB exclusions and
restores safe literal cases. No new sanitizer resurrection or production
floating-point defect was confirmed. The source change and this evidence must
be committed together; after the commit, close the cycle gate and draw a
distinct next goal/cell rather than repeating the closed findings above.

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
