# SIMD, Assembly, and Portable-Reference Backend Differential

## Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `69`
- Selected goal: `backend-differential` (SIMD, assembly, and portable-reference backend differential)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD: `1dcc2da988ee625fbc5d7d55eb6f894c1103ec52`
- Catalog: `agent-journal/reusable-continuous-agent-goals.md`

## Scope and Hypothesis

This cycle focused on the highest-risk local optimized/reference pair: libsecp256k1's x86_64 assembly-enabled backend versus the portable C backend. The falsifiable hypothesis was that an optimized backend diverges from the portable implementation on a valid boundary vector, malformed public input, signature error status, tweak operation, or deterministic signing result.

The trust boundary is Bitcoin Core's cryptographic library API. The expected contract is identical serialized public keys, signatures, shared protocol-visible key material, return statuses, and validation results for identical inputs and context-randomization seed. The comparison used the same current source tree and the same module configuration in both builds.

## Build Matrix

Both builds used Clang 19.1.7, Debug mode, current `src/secp256k1`, shared-library output, and these modules: recovery, extrakeys, Schnorr, MuSig, ElligatorSwift, and Silent Payments enabled; ECDH disabled to match the current Bitcoin Core unit build; experimental APIs disabled. Valgrind and ctime tests were unavailable/off.

- `/data/my_storage/tmp/backend-differential/auto`: `SECP256K1_ASM=AUTO`, resolved as `x86_64`, with `USE_ASM_X86_64=1`.
- `/data/my_storage/tmp/backend-differential/portable`: `SECP256K1_ASM=OFF`.

The existing `build_unit_clang19` binary was found to be stale relative to the current source, so it was not used as final evidence. A fresh assembly-enabled tree was built before the final comparison.

## Deterministic API Differential

The probe is preserved at `agent-journal/backend_differential_probe.cpp`. It uses fixed context randomization (`32` bytes of `0x42`), fixed xorshift-generated inputs, and `2048` vectors. It records statuses and serialized bytes for secret-key validation, public-key creation/parse/serialization, malformed public-key parsing, ECDSA sign/verify/normalization, recoverable ECDSA sign/parse/recovery, x-only conversion, keypair/Schnorr sign/verify, ordinary and x-only tweaks, tweak checking, and zero/all-ones invalid keys.

The same source was compiled once against each fresh library and run without environment-dependent input:

```text
clang++-19 -std=c++17 -Wall -Wextra -Werror -I src/secp256k1/include agent-journal/backend_differential_probe.cpp -L/data/my_storage/tmp/backend-differential/auto/lib -Wl,-rpath,/data/my_storage/tmp/backend-differential/auto/lib -lsecp256k1 -o /data/my_storage/tmp/backend-differential/probe-auto-fresh -pthread
clang++-19 -std=c++17 -Wall -Wextra -Werror -I src/secp256k1/include agent-journal/backend_differential_probe.cpp -L/data/my_storage/tmp/backend-differential/portable/lib -Wl,-rpath,/data/my_storage/tmp/backend-differential/portable/lib -lsecp256k1 -o /data/my_storage/tmp/backend-differential/probe-portable-fresh -pthread
/data/my_storage/tmp/backend-differential/probe-auto-fresh
/data/my_storage/tmp/backend-differential/probe-portable-fresh
cmp /data/my_storage/tmp/backend-differential/auto-probe-fresh.log /data/my_storage/tmp/backend-differential/portable-probe-fresh.log
```

Both probes returned status 0 and produced exactly:

```text
vectors=2048 failures=0 digest=f50ba90e4d947dde
```

`cmp` reported identical output. This checks output bytes and status values together; it is stronger than comparing only successful test exits.

## Library Test Verification

Both fresh builds were configured and built with `SECP256K1_BUILD_TESTS=ON` and `SECP256K1_BUILD_EXHAUSTIVE_TESTS=ON`. The test command was:

```text
<build>/bin/tests --iterations=2 --seed=0123456789abcdef --jobs=2 --log=1
```

The assembly build completed with status 0 and total execution time `96.332 s`. The portable build completed with status 0 and total execution time `94.473 s`. Both passed field/scalar/group, ECDSA/recovery, x-only/extrakeys, Schnorr, MuSig, ElligatorSwift, Silent Payments, checksum, byte-order, and constant-time helper test groups. No failure or backend mismatch appeared.

The raw logs and binaries remain under `/data/my_storage/tmp/backend-differential/` for handoff and independent replay. No fuzz, sanitizer, daemon, profiling, or test process remains running.

## Verdict

**Dismissed as a current backend mismatch; no confirmed finding.** The optimized and portable implementations produced identical deterministic API output/status digests over 2048 vectors, and both passed the same current-source test suite with a fixed seed. No production repair or test change is justified by this cycle.

## Limitations and Rejected Leads

- Only x86_64 and Clang 19 were exercised, in Debug mode. GCC, ARM, 32-bit, big-endian, LTO, and release-only behavior remain unchecked.
- ECDH was disabled in both builds; a future cycle should compare it with a matching module configuration. Compiler and backend timing was not treated as a correctness oracle here.
- The probe exercises public API contracts and valid/invalid parser/status cases, not unsupported object aliasing or arbitrary internal object bytes. Such inputs were correctly excluded because they lack a public contract.
- Existing library tests are implementation-local oracles; the probe supplies the independent cross-backend output comparison. No Bitcoin Core end-to-end workload was needed after the library-level result matched.

## Next Queue

1. Draw another eligible catalog goal and record the exact command/draw before work.
2. Reopen backend differential for GCC/Clang release, ECDH, sanitizer, or 32-bit/cross-architecture coverage when resources allow.
3. Prioritize a distinct high-risk pending cell rather than repeating this passing x86_64 pair.

## Cycle 90: ECDH and release/compiler backend parity

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `69`
- Selected goal: `backend-differential`
- Branch: `uber-cycle-90-backend-differential-20260729`
- HEAD at gate: `89b836342154f71d5d4427dc13864702170eec42`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `2 968`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The tracked worktree passed `git diff --check`; known untracked agent artifacts remain preserved and excluded from cycle commits. No relevant Bitcoin Core, test, fuzz, sanitizer, or benchmark process was running at the gate.

This cycle excludes the existing x86_64 Clang assembly-versus-portable API vector pass, which used ECDH disabled and is recorded above. The new queue is ECDH, release-mode compiler comparisons, and backend configuration combinations that can change optimized/reference behavior without repeating the prior 2048-vector digest.

### Build matrix and differential evidence

Four fresh CMake/Ninja Release builds used the same current source and only enabled the ECDH module, with all other optional modules disabled. The matrix was GCC 12.2.0 and Clang 19.1.7 crossed with `SECP256K1_ASM=OFF` and forced `SECP256K1_ASM=x86_64`. Unit tests and order-13 exhaustive tests were enabled; Valgrind and ctime tests were unavailable/off.

The external probe was compiled with GCC 12 using `-std=c11 -Wall -Wextra -Werror`. It used fixed context randomization and 4096 deterministic vector pairs. For each pair it compared ECDH default SHA256 output, a custom 65-byte hash callback, hash-callback failure, zero and all-ones invalid scalars, malformed and round-trip public-key parsing, and commutative shared-secret output. Every configuration returned:

```text
vectors=4096 failures=0 digest=88259292f10dcdc7
```

`cmp` confirmed identical probe output for GCC reference versus GCC assembly, GCC reference versus Clang reference, and GCC assembly versus Clang assembly. The exact probe source is `/data/my_storage/tmp/cycle90-backend-differential/probes/ecdh_differential.c`; all binaries and build trees are under the same scratch root.

Each `bin/tests --iterations=2 --seed=0123456789abcdef --jobs=2 --log=1` run passed. The ECDH API, generator-basepoint, invalid-scalar, inverse-result, Wycheproof, and context-SHA256 tests all reported `PASSED` in every build. Each `bin/exhaustive_tests` run reported `Exhaustive tests for order 13`, `test count = 2`, and `no problems found`.

### Verdict and handoff

- Dismissed as a current backend/compiler mismatch: no status, serialized output, error behavior, or algebraic relation diverged across the four Release configurations.
- No production source or test change is justified by this cycle.
- Limitations: only x86_64 little-endian hardware was executed; GCC 12 and Clang 19 were the available compilers; the matrix used ECDH-only module composition. ARM, 32-bit, big-endian, full-module composition, LTO, and sanitizer/Valgrind behavior remain open. The prior full-module x86_64 cell is not reopened here.
- Next queue: close the cycle, run a fresh gate, and draw a distinct catalog goal. Reopen this backend cell only for one of the listed compiler, architecture, module, sanitizer, or ECDH-specific boundaries.

## Cycle 100: Core SHA256 optimized/reference dispatch differential

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `69`
- Selected goal: `backend-differential`
- Branch: `uber-cycle-100-backend-differential-20260729`
- HEAD at gate: `37ab5177e146f6c1038733d124157f67c23543d9`
- `origin/master`: `9b38d077f894d27ea76413b1db1cb040e25dc296`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `29 991`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Tracked/index state passed the clean gate and `git diff --check`; known untracked agent artifacts remain preserved and excluded from cycle commits. PID `777094` is an unrelated pre-existing wallet test process sleeping in `futex_wait_queue`; it was not touched.

This cycle excludes the prior x86_64 Clang libsecp assembly-versus-portable API pass and the Cycle 90 ECDH/release/compiler matrix. The distinct target is Bitcoin Core's current `src/crypto/sha256.cpp` dispatch between the standard transform and selectable SSE4, AVX2, and SHA-NI paths, including the `SHA256D64` multi-block dispatch.

### Planned evidence

Use the current GCC RelWithDebInfo integrated test build at `/data/my_storage/tmp/cycle89-build`, with a temporary test-only differential case. For `STANDARD`, `USE_SSE4`, `USE_AVX2`, `USE_SHANI`, `USE_SSE4_AND_AVX2`, `USE_SSE4_AND_SHANI`, and `USE_ALL`, call `SHA256AutoDetect` and compare identical deterministic messages at padding and block boundaries, fragmented writes, and `SHA256D64` batch sizes. The standard mode is the reference for this public API comparison; the existing self-tests and full crypto suite remain additional oracles. If all modes match, remove the temporary case and retain only this evidence in the handoff journal. If a mismatch occurs, preserve the smallest reproducer and independently verify it before considering a production fix.

### Limitations and next queue

The installed current toolchain is GCC 12.2.0 and Clang 14.0.6; the existing Clang 19 artifacts in the earlier journal are historical scratch evidence, not current Cycle 100 results. This cycle does not establish ARM, 32-bit, big-endian, LTO, or sanitizer behavior. A passing x86_64 dispatch comparison does not prove timing equivalence or constant-time behavior. After closure, draw a fresh eligible catalog goal and choose a distinct unchecked evidence cell.

### Differential execution and verdict

A temporary test-only case was added to `src/test/crypto_tests.cpp`, built in the current GCC 12 RelWithDebInfo tree, and removed after verification. It called `SHA256AutoDetect` with all seven public masks (`STANDARD`, `USE_SSE4`, `USE_AVX2`, `USE_SHANI`, both pairwise combinations, and `USE_ALL`). Each mode processed deterministic messages of lengths 0, 1, 55, 56, 63, 64, 65, 127, 128, 129, 1024, and 4096 bytes using both one-shot and 17-byte-fragmented writes. It also compared `SHA256D64` for every batch size from 0 through 64 against `CHash256` and against the `STANDARD` result.

The focused command was:

```text
TMPDIR=/data/my_storage/tmp/cycle100-backend-differential /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=crypto_tests/sha256_backend_differential_cycle100 --log_level=test_suite
```

It ran one case and exited 0 with `*** No errors detected`. The full `crypto_tests` suite then ran 24 cases and exited 0 with `*** No errors detected`. `git diff --check` passed, and `git diff --quiet -- src/test/crypto_tests.cpp` confirmed the temporary probe left no tracked source change.

**Verdict:** dismissed as a current core SHA256 backend mismatch. No output divergence, status divergence, padding-boundary error, fragmented-write error, or `SHA256D64` batch mismatch was observed. No production repair or permanent test is justified by this cycle.

**Evidence limits:** this is an executed x86_64 GCC comparison only. The result does not cover ARM, 32-bit, big-endian, LTO, sanitizer builds, compiler-generated differences under another toolchain, or timing/constant-time equivalence. The unrelated pre-existing PID `777094` remained untouched. Next action is to close the cycle and draw a fresh catalog goal; backend work should return only for one of the explicitly untested architecture/toolchain/sanitizer cells.
