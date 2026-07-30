# Cycle 110: Secret scalar tweak API timing cell

## Identity and scope

- Cycle: `110`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `53`
- Goal: Statistical timing-side-channel campaign
- Slug: `statistical-timing`
- Branch: `uber-cycle-110-statistical-timing-20260729`
- HEAD at cycle start: `32d63a0d1c4f8b22f72142a093e0ce777ee93e0c`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Entry divergence: `origin/master...HEAD = 40 1009`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`

The fresh gate passed `git fetch origin master`, tracked/index cleanliness, `git diff --check`, all catalog hashes, and process checks. PID `777094` (`wallet_tests`, parent `725042`) was preserved. Prior timing cells covered public-key creation/ECDSA, ECDH/Schnorr/MuSig, GCC and Clang backend/optimization variants, ElligatorSwift, and Silent Payments. This cycle excludes those completed caller cells and targets the secret scalar tweak APIs and their keypair wrapper.

## Contract and hypotheses

`secp256k1_ec_seckey_tweak_add` and `secp256k1_ec_seckey_tweak_mul` receive a secret scalar plus a public tweak. Their source path uses fixed-width scalar parsing, addition or multiplication, a constant-time conditional zero on failure, and scalar serialization. `secp256k1_keypair_xonly_tweak_add` loads a secret scalar and its public point, normalizes public-key parity, applies the same secret tweak, and uses variable-time public-point arithmetic on a declassified public key. The keypair API is therefore a boundary control: a timing difference there is not a secret leak without a source/ctime explanation.

Hypothesis H1: secret-class timing separates in the raw scalar tweak APIs under a supported compiler/backend, indicating a compiler or scalar implementation issue. Hypothesis H2: the keypair wrapper shows a class effect only through its public/declassified point path, which should be distinguishable from H1. A reportable finding requires stable repeated separation, identical successful outputs, source-level secret dependence, and an independent ctime/MSan or mutation control. Passing timing statistics remain limited evidence, not proof of constant time.

## Execution plan

Use current-source Clang 19 and GCC 12 Release libraries with `SECP256K1_ASM=AUTO`, plus a current-source Clang 19 `SECP256K1_ASM=OFF` build. The deterministic probe will use valid scalar classes `1`, `2`, `n-1`, and a high non-boundary scalar, a fixed nonzero public tweak, randomized class order, CPU affinity, paired samples, raw `CLOCK_MONOTONIC_RAW` timings, medians/percentiles, Welch statistics, and a same-input control. Each mutating API receives a fresh copy of its key material per sample. Run the official relevant extrakeys/core tests and the ctime target when the available MSan toolchain permits it. Preserve raw outputs, compiler flags, library hashes, and exact commands under `/data/my_storage/tmp/cycle110-statistical-timing/`.

## Evidence ledger

### Builds and measurement

The probe was compiled from `agent-journal/statistical_timing_cycle110_probe.cpp` against:

- current-source Clang 19 Release, `SECP256K1_ASM=AUTO`, using `/data/my_storage/tmp/cycle105-clang19-release`;
- current-source GCC 12 Release, `SECP256K1_ASM=AUTO`, using `/data/my_storage/tmp/cycle105-gcc-release`; and
- a fresh current-source Clang 19 Release build with `SECP256K1_ASM=OFF` at `/data/my_storage/tmp/cycle110-statistical-timing/clang-off`.

The probe pinned execution to one CPU, randomized pair order, timed 10,000 samples per pair with 16 API calls per sample, and repeated each pair three times. It compared four valid secret-key pairs (`1`/high, `1`/`2`, `n-1`/high, and `1`/`n-1`) with a fixed nonzero public tweak. It exercised `secp256k1_ec_seckey_tweak_add`, `secp256k1_ec_seckey_tweak_mul`, `secp256k1_keypair_xonly_tweak_add`, and `secp256k1_pubkey_tweak_add` as a public control. Raw logs and binaries are retained under `/data/my_storage/tmp/cycle110-statistical-timing/`.

### Timing results

The scalar APIs showed no stable class-dependent effect across builds or repetitions. Across all scalar-tweak logs, Welch statistics ranged from approximately `0.03` to `1.19` for tweak-add and `0.06` to `1.64` for tweak-mul; paired statistics changed sign and stayed within approximately `-2.30` to `2.36`. The public `pubkey_tweak_add` control was similarly directionless, with Welch values from about `0` to `0.86` and paired values from about `-1.77` to `1.27`.

`keypair_xonly_tweak_add` showed a repeated negative paired direction for `1` versus high and for `1` versus `n-1`, especially in the Clang runs. The source explains this without a secret-dependent branch: `src/secp256k1/src/modules/extrakeys/main_impl.h:180-200` explicitly declassifies the public half while loading the keypair, and `:275-278` branches on the normalized public point's y parity before applying the secret tweak. Keys `1` and `n-1` produce `G` and `-G`, so this is an expected public/declassified distinction. The Welch statistic was not consistently significant, and the scalar-only APIs did not reproduce the effect.

### Independent ctime control

A fresh Clang 19 MSan build with `SECP256K1_ASM=OFF` and ctime tests enabled was built at `/data/my_storage/tmp/cycle110-statistical-timing/msan`. The restored source passed:

```text
MSAN_OPTIONS='halt_on_error=1:exit_code=86:report_umrs=1:print_summary=1' /data/my_storage/tmp/cycle110-statistical-timing/msan/bin/ctime_tests
status=0
```

As a sensitivity check, removing only `secp256k1_declassify(ctx, &ret, sizeof(ret));` at `main_impl.h:283` caused the same command to exit `86` with an MSan `use-of-uninitialized-value` report at `secp256k1_keypair_xonly_tweak_add`, `main_impl.h:283:9`. Restoring the line and rerunning returned status `0`. The tracked source was verified clean after the mutation. This confirms that the ctime oracle detects a missing public-result declassification; it does not turn the timing observation into a secret leak.

### Functional validation and verdict

The official `tests` and `noverify_tests` passed with seed `0123456789abcdef`, two iterations, and two workers in all three build variants. The Clang AUTO, Clang assembly-off, and GCC AUTO logs recorded total test times of `18.317/9.688`, `18.281/9.526`, and `19.622/10.280` seconds respectively for `tests/noverify_tests`.

**Verdict: dismissed for the scalar timing hypothesis; inconclusive timing-only evidence for the wrapper, with the observed direction classified as the expected public parity/declassification path. No production source defect or source change was found.** Valgrind and dudect were unavailable, and this cycle did not cover ARM, 32-bit, LTO, or a full compiler/optimization matrix. Keep the goal eligible for a future caller, tool, architecture, or compiler cell rather than treating these runs as a proof of constant-time behavior.

### Next queue

1. Reopen only with a distinct evidence source, such as dudect/Valgrind, another compiler or optimization configuration, a non-x86 target, or a new secret-bearing caller.
2. Preserve the scalar probe logs and the MSan mutation log; do not repeat the same API/build cell without new evidence.
3. Draw the next eligible goal after closing this cycle and re-run the full fresh gate.

---

# Statistical Timing Cycle 1

## Identity and Scope

- Cycle: `1`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `53`
- Goal: Statistical timing-side-channel campaign
- Slug: `statistical-timing`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- HEAD at cycle start: `1dcc2da988ee625fbc5d7d55eb6f894c1103ec52`
- Tracked dirty state at cycle start: clean. Agent-owned catalog and journal files were untracked.

The prior finding ledger in `doc/fuzzing-findings.md` contains no confirmed consensus, key-loss, unauthenticated memory-safety, or remotely reachable race finding. Its recent secp-related history includes subtree update `c26d4e2d6f` and the upstream constant-time fixes documented in `src/secp256k1/CHANGELOG.md` for GCC 13.1 and Clang 14+.

## Hypothesis

The current secret-scalar operations might retain a secret-dependent timing path in an optimized or portable backend, especially during public-key generation or ECDSA signing. A reportable candidate would need a stable class separation, a source-level dataflow explanation or independent tooling result, and a reachable secret-bearing caller. A statistical result alone is not proof.

## Source and Contract Review

- `src/secp256k1/src/ctime_tests.c:15-17` requires the memory-checking interface and `:59-62` refuses to run without Valgrind or MSan. The test covers key generation, ECDSA signing, ECDH when enabled, recoverable signing, secret-key tweaks, keypair/Schnorr/MuSig operations, ElligatorSwift, and silent payments. There are 23 secret-memory undefinitions in the current file.
- `src/secp256k1/CMakeLists.txt:142` makes ctime tests conditional; the existing `build_unit_clang19` cache has `SECP256K1_BUILD_CTIME_TESTS=OFF` and `SECP256K1_VALGRIND=AUTO`. Valgrind and dudect are unavailable in this environment.
- `src/secp256k1/src/ecmult_const_impl.h:61-100` selects precomputed points with a fixed full-table loop and `secp256k1_fe_cmov`, avoiding a secret array index. Its fixed-window scalar path is the relevant ECDH/arbitrary-point multiplication contract.
- `src/secp256k1/src/ecmult_gen_impl.h:109-170` recodes the blinded scalar and uses fixed comb loops for generator multiplication. The context's scalar and point offsets are the documented defense against simple power/timing observations.
- `src/secp256k1/src/modules/ecdh/main_impl.h:34-76` uses `secp256k1_ecmult_const` and handles invalid scalar input by constant-time conditional replacement before returning the public validity result.
- `src/secp256k1/src/modules/schnorrsig/main_impl.h:139-185` treats the key-derived public-key parity and nonce point parity as public/declassified values before conditional negation. `src/secp256k1/src/modules/musig/session_impl.h:632-700` has explicit secret cleanup and only branches on loaded/public parity or contract status.
- `src/secp256k1/src/modinv32_impl.h:531-578` uses 20 fixed 30-divstep rounds for `secp256k1_modinv32`; the variable-time inverse is a separate function at `:580-655` and is not the constant-time secret operation under test.
- `src/secp256k1/CONTRIBUTING.md:49` explicitly directs secret operations to the ctime test. The changelog records prior compiler-induced constant-time fixes, so compiler/backend coverage remains an open follow-up rather than an assumed guarantee.

## Measurement

Temporary source: `agent-journal/statistical_timing_probe.cpp`. It uses the public API with two valid secret classes: scalar `1` and 32 bytes of `0x7f`. For each operation it draws a deterministic pseudo-random class order, collects 4,000 samples total (about 2,014 per class), times 32 repeated calls with `CLOCK_MONOTONIC_RAW`, pins the process to CPU 0, and reports per-operation means, medians, and a Welch t statistic. The context is randomized with a fixed seed. The probe measures `secp256k1_ec_pubkey_create` and `secp256k1_ecdsa_sign`; it is a screening experiment, not a dudect replacement.

Builds:

- Existing Clang 19, Debug, Bitcoin CMake build, `SECP256K1_ASM=AUTO`, static library: `/data/my_storage/tmp/statistical-timing/probe-auto`.
- Fresh standalone Clang 19 Release build, `SECP256K1_ASM=OFF`, CMake-reported `-O2`, shared library: `/data/my_storage/tmp/statistical-timing/probe-portable`.

Raw logs:

- `/data/my_storage/tmp/statistical-timing/auto-pinned.log`
- `/data/my_storage/tmp/statistical-timing/portable-pinned.log`

Three pinned runs per variant produced:

| Variant | Operation | Welch t values | Observation |
|---|---|---|---|
| AUTO/Debug | `ec_pubkey_create` | `0.516, -0.388, 1.569` | Direction changes; medians were within about 21 ns. |
| AUTO/Debug | `ecdsa_sign` | `-1.288, -2.792, -0.874` | One larger negative result, but no independent stable replication. |
| ASM-OFF/Release | `ec_pubkey_create` | `0.476, -2.432, 0.236` | Direction changes; medians were within about 16 ns. |
| ASM-OFF/Release | `ecdsa_sign` | `-1.058, 0.338, -2.107` | Direction changes; medians were within about 194 ns in the noisiest run. |

The existing official secp256k1 suite also passed with fixed seed `0123456789abcdef`, one iteration, and two workers: `build_unit_clang19/src/secp256k1/bin/tests` status `0`, total `7.144 s`; `noverify_tests` status `0`, total `4.903 s`. Targeted `ecmult_gen_blind`, `modinv_tests`, `ecdsa_sign_verify`, `test_schnorrsig_sign`, `musig_nonce_test`, and `test_keypair` all passed with two iterations and the same seed.

## Verdict

**Inconclusive; no confirmed finding.** The source review found the expected fixed-window/conditional-move structure, and the paired measurements did not show a stable class-dependent direction across backend variants or repetitions. The occasional t statistic around `2` to `3` is not sufficient evidence because the experiment lacks dudect's calibrated class schedule, repeated independent datasets, compiler matrix, and source/tool confirmation.

## Limitations and Rejected Leads

- Valgrind, dudect, and a usable ctime target are unavailable. The current ctime test was not silently treated as passed.
- Only x86_64 and Clang 19 were measured. The portable build exercised assembly-off code but not GCC, ARM, 32-bit, LTO, or optimized assembly builds.
- Only two valid key classes and two APIs were sampled. ECDH, Schnorr, MuSig, ElligatorSwift, silent payments, nonce callbacks, and failure paths still need a ctime/MSan or equivalent review.
- The probe's timing includes API/setup and scheduler noise and reports aggregate statistics, not raw traces or a calibrated p-value. No statistical result here is a proof of constant time.
- No source change or production defect was found; no repair commit was made. The temporary probe is preserved as the reproducible experiment source.

## Next Queue

1. Re-run ctime tests under MSan if a compatible Clang runtime is available, or under Valgrind after installing the missing tool; retain the exact command and diagnostics.
2. Build Clang and GCC Release variants with assembly on/off and run the same paired probe for ECDH, Schnorr, MuSig, and ElligatorSwift secret APIs.
3. Inspect optimized assembly around `secp256k1_fe_cmov`, `ecmult_const`, `ecmult_gen`, and compiler-sensitive conditional moves; correlate any difference with the historical GCC/Clang fixes.
4. Draw the next eligible goal after recording this cycle. Keep `statistical-timing` eligible for reopening when new compiler, backend, tool, or caller evidence changes these assumptions.

## Reopened Cycle: Fresh Release Backend Timing (draw 53)

This was a distinct reopened hypothesis after the backend differential cycle: current-source optimization might expose a stable secret-class timing difference even though fresh assembly and portable builds returned identical API outputs. Both builds used Clang 19.1.7, `-O2` Release, the same optional modules, and the same current source. One used `SECP256K1_ASM=AUTO` resolved to x86_64 assembly; the other used `SECP256K1_ASM=OFF`. The timing probe was compiled at `-O2` from `statistical_timing_probe.cpp` and run with `taskset -c 2`.

Each repetition used 2,014 samples per class, 32 API calls per sample, fixed xorshift class scheduling, and the same two valid key classes as the earlier screening probe. Raw logs, build trees, and binaries are under `/data/my_storage/tmp/statistical-timing-cycle5/`.

```text
AUTO Release, ec_pubkey_create t: -1.260, 0.288, -1.668
AUTO Release, ecdsa_sign       t:  1.639, -1.164, -0.803
ASM-OFF Release, ec_pubkey_create t: -1.168, -0.528, 0.792
ASM-OFF Release, ecdsa_sign       t:  0.898, -0.764, -1.776
```

The low/high means and medians moved by tens of nanoseconds and changed direction across repetitions. No stable class effect or backend-specific direction was observed. CPU pinning reduced one source of scheduler movement but does not replace calibrated dudect statistics, raw trace analysis, or a compiler/tool proof.

**Reopened-cycle verdict: inconclusive; no confirmed finding.** This fresh Release result does not change the earlier conclusion. The source review and current backend functional tests remain clean, but Valgrind, dudect, ctime/MSan, GCC, 32-bit, ARM, LTO, and ECDH coverage are still absent. Preserve this goal for reopening only when one of those evidence sources becomes available or a new secret-bearing caller is identified.

## Cycle 44: ECDH, Schnorr, and MuSig Boundary Recheck

- Cycle: `44`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `53`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `23472e129886ba9468b5614ef81d65d9772f774d`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`

This cycle used a distinct caller/configuration cell rather than repeating the earlier `ec_pubkey_create` and ECDSA screen. The probe covered `secp256k1_ecdh`, `secp256k1_schnorrsig_sign32`, and `secp256k1_musig_nonce_gen_counter` with two valid secret-key classes, paired public inputs, randomized class order, eight calls per timed sample, 10,000 paired trials, three repetitions, and CPU 2 affinity. Current-source Clang 19 Release shared libraries were built once with x86_64 assembly selected and once with `SECP256K1_ASM=OFF`; ECDH, extrakeys, Schnorr, and MuSig were enabled. Raw traces and build logs are under:

- `/data/my_storage/tmp/statistical-timing-cycle44-auto/raw.log`
- `/data/my_storage/tmp/statistical-timing-cycle44-portable/raw.log`
- `agent-journal/statistical_timing_cycle44_probe.cpp`

The initial Welch t-statistics were:

| Backend | ECDH repetitions | Schnorr repetitions | MuSig counter repetitions |
|---|---|---|---|
| AUTO assembly | `0.816, -0.180, -0.659` | `0.661, 0.258, 0.469` | `-0.014, 0.259, 1.779` |
| ASM-OFF portable | `-1.913, 0.527, -1.830` | `-1.726, -1.358, -1.457` | `-0.889, -0.018, -0.018` |

The means were sensitive to rare scheduler-scale outliers while medians were nearly equal. A same-key MuSig control with 5,000 paired trials stayed near zero in paired median and positive-pair fraction, but the original AUTO MuSig class pair showed a small approximately 5 ns median direction. A second 5,000-trial pair using equal one-bit scalar classes (`1` versus `2`) changed direction across backends and repetitions. The timing-only verdict is therefore **inconclusive**: no stable class effect, compiler/backend invariant, or reportable timing finding was established. The full raw traces are retained; no timing change was made.

## Confirmed Constant-Time Boundary Finding

The timing screen identified a distinct adjacent evidence question in the MuSig counter caller. The existing MSan ctime test tainted the raw-secret `musig_nonce_gen` call but left the opaque `secp256k1_keypair` defined before `musig_nonce_gen_counter`. A temporary ctime harness change marked that keypair undefined before the counter call. The unpatched current source then failed:

```text
status=86
MemorySanitizer: use-of-uninitialized-value
secp256k1_pubkey_load ... src/secp256k1/src/secp256k1.c:261:5
secp256k1_musig_nonce_gen_internal ... session_impl.h:404:10
secp256k1_musig_nonce_gen_counter ... session_impl.h:486:11
ctime_tests.c:259:15
```

`secp256k1_musig_nonce_gen_counter` extracts the secret and public components with `secp256k1_keypair_sec` and `secp256k1_keypair_pub`, then passes the copied public key into `secp256k1_musig_nonce_gen_internal`. The internal path calls `secp256k1_pubkey_load`, whose validation branch consumes the tainted public representation. The analogous `secp256k1_keypair_load` helper explicitly declassifies its public component before the same validation contract.

The minimal fix adds `secp256k1_declassify(ctx, &pubkey, sizeof(pubkey));` immediately after `secp256k1_keypair_pub` in `src/secp256k1/src/modules/musig/session_impl.h`. The ctime regression retains the keypair taint before the counter call. Rebuilding and rerunning the same MSan command after the fix exited `0`; removing only the new declassification reproduced the same status-86 stack, then restoring it passed again. This is a confirmed constant-time memory-boundary defect, separate from the inconclusive timing measurements.

Validation:

- `cmake --build build_unit_clang19 --target test_bitcoin -j2`: passed.
- `build_unit_clang19/bin/test_bitcoin --run_test=crypto_tests --log_level=message`: 23 cases passed.
- `build_unit_clang19/src/secp256k1/bin/tests --seed=0123456789abcdef --iterations=1 --jobs=2`: status `0`.
- `build_unit_clang19/src/secp256k1/bin/noverify_tests --seed=0123456789abcdef --iterations=1 --jobs=2`: status `0`.
- `build_unit_clang19/src/secp256k1/bin/exhaustive_tests`: order `13`, no problems found.
- `git diff --check`: passed.
- MSan after-fix: `/data/my_storage/tmp/statistical-timing-cycle44-keypair-ctime-final.log`, status `0`.
- MSan mutation without the new declassification: `/data/my_storage/tmp/statistical-timing-cycle44-keypair-ctime-mutation.log`, status `86` at `secp256k1_pubkey_load:261`.

## Verdict and Handoff

**Confirmed and fixed:** MuSig counter nonce generation failed the independent MSan constant-time boundary check when its opaque keypair was treated as secret. **Timing-only result:** inconclusive; the release probes found no stable class-dependent timing effect. The source fix and ctime regression are ready for one self-contained commit authored as `Lőrinc <pap.lorinc@gmail.com>`. Valgrind, dudect, GCC, non-x86 execution, LTO, and a full functional suite remain unavailable or out of scope in this environment. Reopen this goal only for one of those distinct evidence cells or a new secret-bearing caller.

## Cycle 46: GCC Compiler, Backend, and Optimization Cell

- Cycle: `46`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `53`
- Goal: statistical timing-side-channel campaign
- Distinct cell: GCC compiler/backend and `-O3` optimization coverage for ECDH, Schnorr signing, and MuSig counter nonce generation. The cycle-44 MuSig ctime declassification cell is closed and was not repeated.
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `9de78654c41bb74a06769c8822f8a777a3b7bfd6`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Entry divergence: `origin/master...HEAD = 2 862`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV SHA-256: `ee094e408ea668c95129c7a9902d4878e9c303bf9e28e1a481c5af46114725bf`
- Tracked and staged state was clean at entry. Existing untracked agent artifacts and `test/cache` were preserved. No relevant test, daemon, fuzz, sanitizer, Valgrind, or profiling process was running.

### Hypothesis and Contract

GCC optimization or the x86_64 assembly/portable backend could expose a secret-class timing difference in the fixed-window scalar paths despite the earlier Clang release screen. The relevant public callers were `secp256k1_ecdh`, `secp256k1_schnorrsig_sign32`, and `secp256k1_musig_nonce_gen_counter`; the same-key MuSig operation was a null control. The source contract remains that secret scalar selection and multiplication are fixed-window/conditional-move operations, while input validation and explicitly declassified public values may branch.

The local tool inventory was GCC 12.2.0, Clang 14 plus the existing Clang 19 installation, CMake 3.25.1, and Ninja 1.11.1. Valgrind, dudect, QEMU, and cross GCC toolchains were unavailable. The GCC 13.1 constant-time regression documented in `src/secp256k1/CHANGELOG.md` was treated as a reason to test the compiler matrix, not as an oracle that the current code was vulnerable.

### Builds and Measurement

The preserved `agent-journal/statistical_timing_cycle44_probe.cpp` was compiled against isolated shared libraries. Each probe used CPU affinity, deterministic class scheduling, randomized context blinding, three repetitions, 4,000 samples per class, 16 API calls per sample, raw `CLOCK_MONOTONIC_RAW` traces, and key modes `0` (`1` versus `0x7f...7f`) and `1` (`1` versus `2`). The exact output files are:

- `/data/my_storage/tmp/statistical-timing-cycle46-gcc-auto/raw-mode0.log`
- `/data/my_storage/tmp/statistical-timing-cycle46-gcc-auto/raw-mode1.log`
- `/data/my_storage/tmp/statistical-timing-cycle46-gcc-off/raw-mode0.log`
- `/data/my_storage/tmp/statistical-timing-cycle46-gcc-off/raw-mode1.log`
- `/data/my_storage/tmp/statistical-timing-cycle46-gcc-o3-auto/raw-mode0.log`
- `/data/my_storage/tmp/statistical-timing-cycle46-gcc-o3-auto/raw-mode1.log`
- `/data/my_storage/tmp/statistical-timing-cycle46-gcc-o3-off/raw-mode0.log`
- `/data/my_storage/tmp/statistical-timing-cycle46-gcc-o3-off/raw-mode1.log`

Release builds used GCC `-O2` with `SECP256K1_ASM=AUTO` and `OFF`. Separate Debug-configured builds set `CMAKE_C_FLAGS_DEBUG=-O3`, again with assembly `AUTO` and `OFF`; CMake reported the expected `-O3` compile option. The probe was pinned to CPU 2 or 3 and executed one build at a time to avoid deliberate cross-run contention.

| Build | Key mode | Welch t range over 12 API/repetition samples | Maximum absolute t |
|---|---:|---:|---:|
| GCC `-O2`, AUTO | 0 | `-0.498 .. 1.267` | `1.267` |
| GCC `-O2`, AUTO | 1 | `-0.609 .. 0.315` | `0.609` |
| GCC `-O2`, OFF | 0 | `-0.459 .. 1.141` | `1.141` |
| GCC `-O2`, OFF | 1 | `-1.120 .. 0.413` | `1.120` |
| GCC `-O3`, AUTO | 0 | `-1.055 .. 1.025` | `1.055` |
| GCC `-O3`, AUTO | 1 | `-0.646 .. 0.664` | `0.664` |
| GCC `-O3`, OFF | 0 | `-1.125 .. 0.486` | `1.125` |
| GCC `-O3`, OFF | 1 | `-0.764 .. 0.624` | `0.764` |

Directions changed between repetitions and backend/optimization variants. Medians and 95th percentiles tracked across classes, and the deterministic sink was identical for every paired build/class run: `229` for mode 0 and `164` for mode 1. No run failed an API result check.

The optimized object was also inspected with `objdump`. `secp256k1_ecmult_const.part.0` had the same `0x9fa` text size in both GCC `-O2` builds and sizes `0x1115` (AUTO) and `0x1067` (OFF) in the GCC `-O3` builds. The disassembly retained the call into the fixed-window multiplication routine and its loop structure; the visible conditional branches were around public validation, callbacks, and loop/control mechanics. This is corroborating code review, not a proof of constant time.

### Independent Correctness Validation

Both GCC `-O2` test builds were configured and rebuilt with `SECP256K1_BUILD_TESTS=ON`, assembly `AUTO` or `OFF`, and exhaustive/benchmark/ctime tests disabled because the latter tools are unavailable. With seed `0123456789abcdef`, `--iterations=1`, and `--jobs=2`, all four binaries passed:

- GCC AUTO `tests`: exit `0`, 18.589 seconds.
- GCC AUTO `noverify_tests`: exit `0`, 9.588 seconds.
- GCC OFF `tests`: exit `0`, 18.875 seconds.
- GCC OFF `noverify_tests`: exit `0`, 9.500 seconds.

The CMake builds, probe executions, official tests, and `git diff --check` completed without a source or test failure. No production source or test change was justified.

### Verdict and Handoff

**Inconclusive; no confirmed timing finding.** The compiler/backend/optimization cell produced no stable class separation, output divergence, or source-level secret-dependent branch evidence. Passing statistics remain limited evidence and do not replace dudect or ctime/Valgrind analysis. Valgrind, dudect, GCC 13+, LTO, non-x86 execution, and full ctime coverage remain unavailable. The next eligible timing work is only a genuinely new tool, compiler, architecture, or secret-bearing caller cell; otherwise draw the next catalog goal. Raw traces and build trees are retained under `/data/my_storage/tmp/statistical-timing-cycle46-*`.

## Cycle 71: ElligatorSwift XDH and Silent Payments caller cell

- Cycle: `71`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `53`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `9eb6b4ad1b66f030f0019b759d0c8017993cf63b`
- Branch: `uber-cycle-71-statistical-timing-20260728`
- Entry divergence: `origin/master...HEAD = 2 917`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Tracked and staged state was clean at entry except known untracked agent artifacts and `test/cache`; no relevant daemon, test, fuzz, sanitizer, Valgrind, or profiling process was running.

### Distinct scope and contract

Cycles 1 and 5 screened `ec_pubkey_create`/ECDSA under Clang; cycle 44 screened ECDH, Schnorr, and MuSig callers and found the separate MuSig ctime declassification defect; cycle 46 screened GCC `-O2`/`-O3` and x86_64 assembly/portable variants. This cycle therefore targets module callers not covered by those timing experiments: `secp256k1_ellswift_xdh`, `secp256k1_silentpayments_sender_create_outputs`, `secp256k1_silentpayments_recipient_label_create`, and `secp256k1_silentpayments_recipient_scan_outputs`.

The ElligatorSwift header promises constant time in `seckey32` for XDH and in `seckey32`/`auxrnd32` for `ellswift_create`; it explicitly labels encode/decode variable-time, so encode/decode are excluded. Silent Payments ctime coverage marks scan and input secrets undefined while public keys, summaries, and outputs are defined. The measurement result alone cannot prove a secret leak because some paths deliberately declassify validity or derived public output; a candidate requires a stable differential result plus source/ctime/MSan evidence.

### Initial execution plan

Build current source with Clang 19 at Release `-O2`, x86_64 assembly `AUTO` and `OFF`, enabling ElligatorSwift, extra keys, MuSig, Schnorr, and Silent Payments. Use a deterministic CPU-pinned paired probe with two valid scalar classes, randomized class order, fixed public peers/recipients, raw monotonic timing, medians/percentiles, Welch statistics, and a same-input control where the secret class is held constant. Preserve raw traces and exact build/toolchain metadata. Run the official module tests and the available ctime/MSan target or document why it cannot run.

### Measurements and independent checks

The matched builds were configured in `/data/my_storage/tmp/statistical-timing-cycle71/auto` and `/data/my_storage/tmp/statistical-timing-cycle71/off` with Clang 19.1.7, `RelWithDebInfo`, `-O2`, static libraries, all relevant modules enabled, and `SECP256K1_ASM=AUTO` or `OFF`. The probe source is `agent-journal/statistical_timing_cycle71_probe.cpp`; it pins to CPU 2, randomizes class order, uses 1,500 samples per class and four calls per sample, records raw `CLOCK_MONOTONIC_RAW` samples, and writes complete traces to:

- `/data/my_storage/tmp/statistical-timing-cycle71/auto.log`
- `/data/my_storage/tmp/statistical-timing-cycle71/off.log`

The repeated Welch statistics were:

| Operation | AUTO repetitions | ASM-OFF repetitions |
|---|---:|---:|
| Ellswift create | `-202.796, -277.246, -162.903` | `-226.116, -304.722, -256.284` |
| Ellswift XDH | `-0.037, 0.347, -0.224` | `-0.428, 0.760, 0.549` |
| Silent sender outputs | `1.161, 0.458, 0.743` | `1.440, 1.196, 1.826` |
| Silent recipient label | `0.496, -0.373, 0.533` | `0.226, 0.471, -0.073` |
| Silent recipient scan | `6.826, 6.022, 6.170` | `8.183, 7.355, 9.448` |
| Same-key scan control | `0.519, -0.311, -0.526` | `0.080, -0.392, 0.587` |

The large Ellswift-create separation is expected because the API documents the resulting public key as variable-time. XDH, sender output creation, and label creation showed no stable class effect. Scan showed a repeatable approximately 1.2 microsecond low/high mean difference, while the same-key control was near zero. This is not a secret-control-flow finding: the scan API explicitly declassifies the hash-derived output tweak before variable-time public-key arithmetic, because callers branch on a found public output. The probe also held a deliberately nonmatching transaction output fixed rather than generating a matching transaction separately for each secret, so its scan timing is a screening observation of declassified public-output work, not a valid payment-recognition oracle or a proof of leakage.

The AUTO and ASM-OFF module suites both passed with the exact commands:

```text
bin/tests --seed=0123456789abcdef --iterations=1 --jobs=2
bin/noverify_tests --seed=0123456789abcdef --iterations=1 --jobs=2
```

All four commands exited `0`; logs are `/data/my_storage/tmp/statistical-timing-cycle71/auto-tests.log`, `/data/my_storage/tmp/statistical-timing-cycle71/off-tests.log`, and the corresponding console capture. No Valgrind or dudect binary was available.

### MSan ctime boundary control

A separate Clang 19 MSan build was configured in `/data/my_storage/tmp/statistical-timing-cycle71/msan` with `SECP256K1_ASM=OFF`, all ElligatorSwift/Silent Payments dependencies enabled, and `SECP256K1_BUILD_CTIME_TESTS=ON`. The restored source passed:

```text
MSAN_OPTIONS='halt_on_error=1:exit_code=86:report_umrs=1:print_summary=1' bin/ctime_tests
```

The pass is recorded in `msan-ctime.log` and `msan-ctime-restored.log`. As an independent sensitivity control, the `secp256k1_declassify(ctx, hash_ser, sizeof(hash_ser))` call in `secp256k1_silentpayments_create_output_tweak` was temporarily removed. The rebuilt ctime test then exited `86` at `secp256k1_silentpayments_create_output_pubkey` line 160, called from the sender path at line 325 and `ctime_tests.c:327`; the raw report is `msan-ctime-mutated.log`. Restoring the declassification and rebuilding returned ctime to exit `0`. The mutation was not committed.

Source review confirms that `secp256k1_ellswift_xdh` uses `secp256k1_ecmult_const_xonly` for the secret scalar, clears the scalar and shared coordinate, and has no new secret-dependent branch in the valid-key path. Silent Payments uses constant-time `ecmult_const` for the shared secret and clears its secret material. The validity return and output-tweak/public-output boundaries are explicitly declassified. The independent MSan pass and the targeted failure after removing the boundary match those contracts.

### Cycle 71 verdict and handoff

**No new source defect; timing-only scan signal dismissed as a declassified public-output path.** The caller cell is closed without a production or test change. The probe and raw logs remain available for a future cycle only if a new API contract, compiler/backend, architecture, or caller setup changes the assumptions. The next uber cycle must draw a distinct goal from the full catalog.
