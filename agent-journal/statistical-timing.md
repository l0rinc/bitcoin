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
