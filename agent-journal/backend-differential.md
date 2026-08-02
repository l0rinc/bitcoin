# SIMD, Assembly, and Portable-Reference Backend Differential

## Cycle 301: libminisketch generic and CLMUL implementation differential

### Selection and fresh gate

- Exact selector: `shuf -i 0-100 -n 1` -> `69` (`backend-differential`). The
  previous backend cells covered CRC32C, Bitcoin Core SHA256 dispatch, and
  libsecp256k1 x86_64 portable/assembly under Release, sanitizer, and ThinLTO;
  this cycle therefore selected the remaining vendored Minisketch backend
  matrix rather than repeating those passing cells.
- Branch: `uber-cycle-301-backend-differential-20260802`.
- Cycle-start HEAD: `6078e6c8c33514435957cc5a186be5049d9ad1ac`.
- `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`.
- Merge-base equals `origin/master`; start divergence was `0 1395`.
- The tracked worktree was clean at selection. Existing untracked probes and
  scratch artifacts were preserved and excluded from cycle commits. The seven
  protected test processes were checked and left running.

### Scope and hypothesis

`src/minisketch/src/minisketch.cpp` exposes implementation 0 (generic), and,
when `HAVE_CLMUL` is compiled in and CPUID reports PCLMULQDQ, implementations 1
(CLMUL) and 2 (trinomial CLMUL). The falsifiable hypothesis was that an
optimized field backend diverges from generic behavior for a supported field
size, especially at non-byte-aligned sizes, zero/full capacity, high-bit
truncation, clone/deserialize, decode bounds, or a failed merge. The trust
boundary is the public C/C++ Minisketch API: equal serialized bytes and decoded
set-XOR results are required for equivalent operations; a merge rejected for a
different implementation or field size must leave the destination unchanged.

The existing `src/minisketch/src/test.cpp` already compares implementations
inside its own test oracle. A separate probe was used here so that the new
evidence also exercised the public merge failure contract and an independent
`std::set` model across every field size 2 through 64.

### Build matrix and independent evidence

Two fresh standalone CMake builds used Clang 19.1.7, Release mode, all field
sizes, and tests enabled:

```text
cmake -S src/minisketch -B /data/my_storage/tmp/cycle301-minisketch/generic -G Ninja -DCMAKE_CXX_COMPILER=clang++-19 -DCMAKE_BUILD_TYPE=Release -DMINISKETCH_INSTALL=OFF -DMINISKETCH_BUILD_TESTS=ON -DMINISKETCH_BUILD_BENCHMARK=OFF -DHAVE_CLMUL=OFF -DCMAKE_CXX_FLAGS=-mno-pclmul
cmake -S src/minisketch -B /data/my_storage/tmp/cycle301-minisketch/optimized -G Ninja -DCMAKE_CXX_COMPILER=clang++-19 -DCMAKE_BUILD_TYPE=Release -DMINISKETCH_INSTALL=OFF -DMINISKETCH_BUILD_TESTS=ON -DMINISKETCH_BUILD_BENCHMARK=OFF
cmake --build /data/my_storage/tmp/cycle301-minisketch/generic --target test-noverify test-verify --parallel 4
cmake --build /data/my_storage/tmp/cycle301-minisketch/optimized --target test-noverify test-verify --parallel 4
```

The generic configuration reported CLMUL disabled. The optimized configuration
reported CLMUL enabled and built both CLMUL source families. The preserved
probe `agent-journal/minisketch_backend_cycle301_probe.cpp` constructed every
available implementation for bits 2..64, applied deterministic toggle/add
sequences including truncation and `UINT64_MAX`, compared serialization across
implementations, checked bounded decode against a set-XOR model, cloned every
state, tested capacity-reducing same-implementation merges, and used the C API
to verify rejected cross-implementation and cross-field merges preserve the
destination and source.

Both probes returned status 0 and exactly:

```text
bits=2..64 failures=0 digest=727afd9cb1c74f12
```

The independent probe was compiled against each static library. The optimized
run additionally exercised implementations 1 and 2; its implementation-0
behavior produced the same digest as the generic-only build. The Release
`test-noverify 2` and `test-verify 2` commands passed in both trees with
`All tests successful.`

### Sanitized replay

The same all-field matrix was rebuilt with Clang 19, `-O1 -g
-fsanitize=address,undefined -fno-omit-frame-pointer`, once with CLMUL disabled
and once with it enabled. With
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, both sanitized probes
returned the same digest above. The four sanitized VERIFY/non-VERIFY test
commands returned status 0 and `All tests successful.`; no ASan, UBSan,
runtime-error, or sanitizer-summary output was present.

### Verdict and handoff

**Dismissed as a current Minisketch backend correctness defect; no source or
permanent test change is justified.** Generic, CLMUL, and trinomial-CLMUL
implementations agreed on the independent public API/model matrix, including
the failure-state contract, and passed the implementation-local suites under
Release and ASan+UBSan.

Limitations are material: execution was x86_64 little-endian with Clang 19;
there was no ARM/32-bit/big-endian runtime, alternate compiler, timing or
constant-time measurement, and no host execution of a CLMUL-compiled binary on
a CPU without PCLMULQDQ. The last item is now the next queue because
`EnableClmul()` uses a raw CPUID gate and the host advertises the feature.
Scratch builds, logs, and binaries remain under
`/data/my_storage/tmp/cycle301-minisketch/`. Do not repeat this exact
all-field x86_64 matrix without a changed runtime, compiler, architecture, or
fault-injection boundary.

## Cycle 158: ThinLTO portable and x86_64 assembly differential

### Selection and fresh gate

- Exact selector: `shuf -i 0-98 -n 1` -> `69` (`backend-differential`). This is a distinct open cell from the earlier x86_64 CRC32C (Cycle 114), Bitcoin Core SHA256 dispatch (Cycle 100), libsecp Release compiler/assembly (Cycles 90 and 123), and Clang ASan/UBSan assembly/portable (Cycle 133) comparisons. Those cycles did not exercise LTO.
- The dedicated branch is `uber-cycle-158-backend-differential-20260730`; start HEAD was `b79b80b7ea47a2a13fc32e2337c2b96b3cd9eb70`, `origin/master` was `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base was `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and start divergence was `42 1099` (`origin/master...HEAD`).
- The fresh gate passed `git fetch origin --prune`, catalog/protocol hash checks, tracked-worktree cleanliness, and `git diff --check`. The required hashes remained `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`. PID `777094` and PID `956381` were running unrelated unit tests and were preserved.

### Scope and matrix

The hypothesis was that ThinLTO could change an optimized/reference backend contract even when ordinary Release builds agree: serialized public keys, ECDH outputs, ElligatorSwift/BIP324 outputs, invalid-secret status and output behavior, or an internal field/scalar/group invariant could diverge between libsecp256k1 portable arithmetic and the x86_64 assembly backend.

Two fresh standalone libsecp builds used Clang 19.1.7, Release mode, static libraries, `CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON`, all current modules including ECDSA recovery, and tests/exhaustive tests enabled. The only backend difference was `-DSECP256K1_ASM=OFF` versus `-DSECP256K1_ASM=x86_64`. `ninja -t commands` confirmed `-flto=thin` in both compile and link graphs; the assembly graph also contained `USE_ASM_X86_64=1`. Valgrind/ctime tests were disabled because the host lacks the Valgrind headers.

The installed host has no ARM cross compiler, QEMU, or 32-bit linker runtime. A `gcc-12 -m32` link smoke failed only for missing `Scrt1.o`, `crti.o`, `libgcc`, and 32-bit libc; an AArch64 Clang compile failed because the target libc headers are absent. Those are recorded environment limits, not backend findings.

### Test and independent differential evidence

- `cmake --build /data/my_storage/tmp/cycle158-backend-differential/lto-portable --parallel 2` and the corresponding assembly build completed all 10 Ninja actions after enabling recovery.
- `/data/my_storage/tmp/cycle158-backend-differential/lto-portable/bin/tests --iterations=2 --seed=0123456789abcdef --jobs=2 --log=1` passed with total execution time `24.530 seconds`; the assembly command passed in `24.201 seconds`. Both covered ECDH, ECDSA recovery, field/scalar/group arithmetic, extrakeys, Schnorr, MuSig, ElligatorSwift, Silent Payments, byte-order, and constant-time helper groups.
- The matching `noverify_tests` commands passed with captured total execution times `12.035 seconds` (portable) and `11.908 seconds` (assembly). Both exhaustive binaries reported `Exhaustive tests for order 13`, `test count = 2`, `no problems found`, and exit status 0.
- The independent existing probe `agent-journal/backend_differential_cycle123_probe.cpp` was compiled with `clang++-19 -std=c++17 -Wall -Wextra -Werror -flto=thin -fuse-ld=lld-19` against each LTO archive. It covers 512 deterministic key pairs, public-key creation/serialization, default and custom-hash ECDH in both directions, ElligatorSwift creation/decoding, BIP324 EllSwift XDH, and invalid-secret status/output paths. Both probes exited 0 and printed exactly `vectors=512 failures=0 digest=fe288ea1ddb151fb`; the two logs had SHA-256 `c9799f98016b16dd5f4faff7ca8cffd505eea4a75defe60547f371953e64b308`, and `cmp` returned 0.

### Verdict and handoff

**Dismissed as a current ThinLTO backend mismatch; no source or permanent test change is justified.** The portable and x86_64 assembly configurations agree on the independent API/status digest and both implementation-local test families under the same Clang 19 ThinLTO settings. The result is an x86_64 little-endian correctness comparison, not evidence for ARM/32-bit/big-endian behavior, GCC LTO, full LTO versus ThinLTO, PGO/BOLT, Valgrind/ctime, or timing/constant-time equivalence. Scratch builds, logs, and probe outputs remain under `/data/my_storage/tmp/cycle158-backend-differential/`; the existing probe source remains untracked and was not staged. The next backend reopening should use a newly available architecture/toolchain or a distinct profile/transform boundary.

## Cycle 133: sanitized assembly and portable backend differential

### Selection and fresh gate

- Selector: exact shuf -i 0-98 -n 1 -> 69 (backend-differential); no reroll was needed because the previous cycle was goal 33.
- Branch: uber-cycle-133-backend-differential-20260730.
- Gate and cycle start HEAD: b3bca20ce48f1c69123bf226f2ed5aa5896f1a2b.
- Gate origin/master: 9611a356035be531d62bfc40879f388d5dc359c4; merge-base: a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b; divergence: 40 1052.
- Tracked state was clean, git diff --check passed, the four catalog/protocol hashes matched, and PID 777094 was preserved. Known unrelated untracked artifacts remain outside the cycle scope.
- Catalog hashes: reusable goals 5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8, random prompt 10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec, goals TSV babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb, uber protocol 954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0.

### Distinct scope and hypothesis

Cycle 123 already compared full-module Release libsecp256k1 builds under GCC 12 and Clang 19 with assembly OFF and x86_64 assembly enabled. This cycle selects the explicitly open sanitizer boundary: current full modules built with Clang 19 ASan+UBSan, comparing portable arithmetic against the x86_64 assembly configuration. The hypothesis is that sanitizer instrumentation or error-path checking exposes an optimized/reference divergence in serialized output, status, invalid-input behavior, or an internal invariant that ordinary Release tests miss.

Both trees used the same current source, Debug, O1, static library output, ECDH/recovery/extrakeys/Schnorr/MuSig/ElligatorSwift/Silent Payments enabled, fixed SECP256K1_ASM=OFF versus SECP256K1_ASM=x86_64, and ASan+UBSan with frame pointers. Each build completed configure and 10/10 Ninja steps. The assembly tree's build graph contains USE_ASM_X86_64=1; the portable tree reports assembly OFF.

### Sanitized library verification

The fixed command was tests --iterations=2 --seed=0123456789abcdef --jobs=2 --log=1, with ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:halt_on_error=1 and UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1.

| Backend | tests | noverify_tests | exhaustive_tests |
|---|---|---|---|
| portable, ASAN+UBSAN | exit 0; 70.584 seconds | exit 0; 35.302 seconds | exit 0; order 13, no problems found |
| x86_64 assembly, ASAN+UBSAN | exit 0; 71.541 seconds | exit 0; 35.278 seconds | exit 0; order 13, no problems found |

Both normal test runs covered the current ECDH, recovery, ECDSA, extrakeys, Schnorr, MuSig, ElligatorSwift, Silent Payments, checksum, byte-order, and constant-time helper groups. The no-VERIFY runs and exhaustive order-13 runs also exited 0. A scan of all six test/probe logs found zero AddressSanitizer, UndefinedBehaviorSanitizer, runtime-error, or sanitizer-summary lines.

### Independent cross-backend probe

The existing independent 512-vector probe agent-journal/backend_differential_cycle123_probe.cpp was compiled with Clang 19 and ASan+UBSan against each static library. It covers public-key creation and serialization, default and custom-hash ECDH in both directions, EllSwift create/decode, BIP324 EllSwift XDH in both directions, and invalid-secret status/output paths.

Both probe processes exited 0 and printed exactly:

    vectors=512 failures=0 digest=fe288ea1ddb151fb

cmp returned 0 for the two logs. The logs have identical SHA256 c9799f98016b16dd5f4faff7ca8cffd505eea4a75defe60547f371953e64b308. This independent output/status comparison agrees with the library test suites and does not depend on the implementation's own expected-value tables.

### Verdict and handoff

The bounded hypothesis is dismissed for a new repository defect. The ASan+UBSan portable and x86_64 assembly backends produced identical probe output/status and passed the same normal, no-VERIFY, and exhaustive test suites without sanitizer diagnostics. No production or permanent test change is justified.

Scratch artifacts and logs are under /data/my_storage/tmp/cycle133-secp-sanitized-off and /data/my_storage/tmp/cycle133-secp-sanitized-asm. The result is an x86_64 little-endian Clang 19 sanitizer comparison; ARM32/cross architecture, 32-bit, big-endian, LTO/PGO, Valgrind/ctime, and timing equivalence remain open. Do not repeat the prior Release compiler/assembly cell without one of those changed evidence sources.

## Cycle 114: CRC32C SSE4.2 and portable backend differential

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `69`
- Selected goal: `backend-differential`
- Branch: `uber-cycle-114-backend-differential-20260729`
- HEAD at gate: `6051c419bbb2a1a12915f2840e0b07a635394002`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `40 1017`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The tracked worktree passed the gate; known untracked agent artifacts remain preserved and excluded from cycle commits. PID `777094` is the unrelated pre-existing wallet test process and must remain untouched.

This cycle excludes the prior libsecp256k1 x86_64 assembly-versus-portable comparison, the Cycle 90 ECDH/compiler matrix, and the Cycle 100 Bitcoin Core SHA256 dispatch comparison. The distinct unchecked cell is bundled CRC32C: its runtime API dispatches to an SSE4.2 implementation when the configured build includes that object and the CPU advertises the instruction, otherwise it uses `ExtendPortable`.

### Scope and hypothesis

The falsifiable hypothesis is that the CRC32C SSE4.2 implementation diverges from the portable implementation at an alignment, short-input, block-threshold, fragmented-extension, nonzero-seed, or large-input boundary. The trust boundary is the CRC32C library API used by persistence/database code; equal output for identical bytes and extension seeds is required, including the C ABI wrappers and empty input.

The planned matrix is two fresh x86_64 builds from the same source: one with SSE4.2 compiler detection enabled and one with it disabled. The probe will use deterministic boundary vectors, repeated pseudo-random vectors, nonzero extension seeds, and fragmented updates. Bundled CRC32C unit and C API tests will run in both builds. Any divergence will be minimized and independently checked before considering a production change.

### Build matrix and differential evidence

The standalone CMake project was configured twice from the current source with tests and benchmarks disabled, `CRC32C_USE_GLOG=OFF` because the optional vendored `third_party/glog` directory is absent, and `HAVE_ARM64_CRC32C=OFF`:

```text
cmake -S src/crc32c -B /data/my_storage/tmp/cycle114-backend-differential/cmake-reference -G Ninja -DCMAKE_BUILD_TYPE=Release -DCRC32C_BUILD_TESTS=OFF -DCRC32C_BUILD_BENCHMARKS=OFF -DCRC32C_INSTALL=OFF -DCRC32C_USE_GLOG=OFF -DHAVE_SSE42=OFF -DHAVE_ARM64_CRC32C=OFF -DCMAKE_CXX_FLAGS=-mno-sse4.2
cmake -S src/crc32c -B /data/my_storage/tmp/cycle114-backend-differential/cmake-sse42 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCRC32C_BUILD_TESTS=OFF -DCRC32C_BUILD_BENCHMARKS=OFF -DCRC32C_INSTALL=OFF -DCRC32C_USE_GLOG=OFF -DHAVE_SSE42=ON -DHAVE_ARM64_CRC32C=OFF
cmake --build /data/my_storage/tmp/cycle114-backend-differential/cmake-reference --parallel 4
cmake --build /data/my_storage/tmp/cycle114-backend-differential/cmake-sse42 --parallel 4
```

Both configurations built successfully. Their generated headers contained `HAVE_SSE42 0` and `HAVE_SSE42 1`, respectively. The host advertises `sse4_2`; disassembly showed `_ZN6crc32c11ExtendSse42EjPKhm` in the accelerated archive and no corresponding implementation symbol in the reference archive.

The independent probe is preserved at `agent-journal/crc32c_backend_probe.cpp`. It implements a bitwise CRC32C oracle and checks lengths from zero through 32768 bytes, eight alignment offsets, zero/FF/incrementing/pseudo-random patterns, five extension seeds, eight fragmentation schedules, the C++ and C APIs, and direct portable/SSE routines. It ran against both CMake-produced libraries:

```text
vectors=5280 api_checks=54912 portable_checks=5280 backend_checks=5280 failures=0 api_digest=87e7750709b157c7 portable_digest=5135e2bd6ec53932 backend_digest=5135e2bd6ec53932
vectors=5280 api_checks=54912 portable_checks=5280 backend_checks=5280 failures=0 api_digest=87e7750709b157c7 portable_digest=5135e2bd6ec53932 backend_digest=5135e2bd6ec53932
```

The self-contained bundled `src/crc32c/src/crc32c_capi_unittest.c` was compiled and linked against each archive; both returned status 0 and printed `All tests passed`. The upstream GoogleTest target could not be configured because this checkout has neither `third_party/googletest` nor `third_party/glog`; this is an external checkout limitation, not a test failure. The Bitcoin Core tree has no `leveldb_tests` test case to run under the current unit binary.

### Sanitizer verification

The same direct source matrix was compiled with GCC 12 using `-fsanitize=address,undefined -fno-omit-frame-pointer`, then run with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1`. Both sanitized binaries returned status 0 with the same `vectors=5280`, zero-failure, and `api_digest=87e7750709b157c7` result. No sanitizer report appeared.

### Verdict and handoff

**Dismissed as a current CRC32C backend mismatch; no confirmed finding.** The public dispatch, C ABI, portable implementation, and direct SSE4.2 implementation agree with the independent bitwise oracle across the selected boundary and fragmentation matrix. No production repair or permanent test change is justified by this cycle.

Limitations: execution was x86_64 little-endian with GCC 12. ARM64, 32-bit, big-endian, other compilers, and faulted CPU feature detection remain open. The upstream GoogleTest suite requires missing vendored dependencies. The optimized/reference comparison is a correctness result, not timing or constant-time evidence.

Next queue: close this cycle, preserve the probe and scratch logs under `/data/my_storage/tmp/cycle114-backend-differential/`, run a fresh gate, and draw a new catalog goal. Reopen backend differential only for an untested architecture, compiler, sanitizer, or module boundary.

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

## Cycle 123: full-module Release compiler and assembly differential

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `69`
- Selected goal: `backend-differential` (SIMD, assembly, and portable-reference backend differential)
- Branch: `uber-cycle-123-backend-differential-20260730`
- HEAD at gate: `89e43c2c06262bbaf22664729f6c8e8b409200f3`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `1035 40`

The clean tracked gate passed after the Cycle 122 close commit; known untracked agent artifacts remain preserved and excluded from the cycle. Prior backend cycles covered x86_64 Debug libsecp assembly versus portable APIs, an ECDH-only Release compiler matrix, Core SHA256 dispatch, and CRC32C SSE4.2 versus portable behavior. ARM32 execution is unavailable because this environment has no ARM cross-compiler or QEMU. This cycle therefore selected the remaining full-module Release composition/compiler cell rather than repeating those results.

The hypothesis was that x86_64 assembly and portable backends, or GCC and Clang Release code, could diverge only when all current modules were composed together. The trust boundary was the libsecp256k1 public API, with deterministic serialized output, return status, and invalid-input behavior required to match. The independent probe covered ECDH and EllSwift; the library's current MuSig and Silent Payments tests supplied module-specific verification.

## Cycle 123 result

The four planned trees configured and built successfully. Each summary reported ECDH, recovery, extrakeys, Schnorr, MuSig, ElligatorSwift, and Silent Payments enabled, Release `-O2`, tests and exhaustive tests enabled, ctime/benchmark/examples disabled, and the requested compiler/backend:

```text
/data/my_storage/tmp/cycle123-backend-differential/{gcc-off,gcc-asm,clang-off,clang-asm}
GCC 12.2.0 x SECP256K1_ASM=OFF
GCC 12.2.0 x SECP256K1_ASM=x86_64 (USE_ASM_X86_64=1)
Clang 19.1.7 x SECP256K1_ASM=OFF
Clang 19.1.7 x SECP256K1_ASM=x86_64 (USE_ASM_X86_64=1)
```

The identical seeded test command was run in all four trees:

```text
<tree>/bin/tests --iterations=2 --seed=0123456789abcdef --jobs=2 --log=1
```

All test groups passed. Total execution times were 43.514 seconds (GCC portable), 43.056 seconds (GCC assembly), 41.386 seconds (Clang portable), and 41.147 seconds (Clang assembly). The results included ECDH API/generator/Wycheproof/context-hash cases, ECDSA and recovery, Schnorr, all MuSig tests including nonce and vector cases, the refreshed `ellswift_xdh_test_vectors_tests`, EllSwift round trips and XDH correctness, and all Silent Payments tests. Each `<tree>/bin/exhaustive_tests` run also passed order 13 with `test count = 2` and `no problems found`.

The first independent probe link attempt failed in all four cells because the standalone `tests` target compiles the implementation into `tests.c` and does not build the shared library. This was a reproducible harness setup issue, not a source failure. After explicitly building each `secp256k1` target, the probe linked successfully. The probe source is preserved at `agent-journal/backend_differential_cycle123_probe.cpp`; it exercises 512 deterministic key pairs through public-key serialization, default and custom-hash ECDH in both directions, EllSwift create/decode, BIP324 EllSwift XDH in both directions, and invalid-secret return status/output paths.

The Clang 19-built probe returned exactly this output for every library:

```text
vectors=512 failures=0 digest=fe288ea1ddb151fb
```

`cmp` matched all four logs. Their identical SHA256 was `c9799f98016b16dd5f4faff7ca8cffd505eea4a75defe60547f371953e64b308`. Rebuilding the same probe with GCC 12 and running it against all four libraries produced the same output and digest. The external probe therefore agrees across compiler-built libraries and independently checks the current EllSwift and ECDH paths beyond the shared test executable.

**Verdict:** dismissed as a current full-module Release backend/compiler mismatch. No output, status, error-contract, ECDH, EllSwift, MuSig, Silent Payments, or exhaustive algebraic divergence was observed. No production repair or permanent test change is justified.

Limitations: execution remains x86_64 little-endian; no ARM cross-compiler/QEMU, 32-bit target, big-endian target, LTO/PGO, Valgrind, or sanitized library build was available. The external probe does not independently implement MuSig or Silent Payments semantics, which were covered by their dedicated current-source test groups. The first link failure and the missing Valgrind dependency are recorded rather than suppressed.

Next queue: close this cycle, preserve the four build trees, probe, and raw logs under `/data/my_storage/tmp/cycle123-backend-differential/`, run a fresh gate, and draw another goal. Reopen backend differential only for ARM/cross-architecture, sanitizer-built libraries, LTO/PGO, or a new backend/module change.

## Cycle 249: bitset and cluster-linearization backend differential

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `69`
- Selected goal: `backend-differential` (SIMD, assembly, and portable-reference backend differential)
- Branch: `uber-cycle-249-backend-differential-20260801`
- HEAD at gate: `27aa0368bd3adf774dff3ba9aa06fa8037aed1c9`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `42 1285`

The tracked clean gate, repository hashes, disk/process checks, and required branch state passed. The six protected earlier test processes remained alive. Prior Goal 69 cells were excluded: libsecp256k1 assembly versus portable API checks, sanitized assembly/portable checks, CRC32C SSE4.2 versus portable behavior, Bitcoin Core SHA256 dispatch, full-module compiler/assembly composition, and earlier ECDH/compiler backend comparisons. No relevant source files changed after those cells; this cycle selected the current Bitcoin Core `util/bitset.h` backend family used by cluster linearization.

### Hypothesis and target

The hypothesis was that equivalent bitset representations could disagree at limb boundaries, in set/test/clear operations, or while driving cluster linearization. The trust boundary was production graph/accounting state represented by the bitset and the resulting transaction ordering, chunking, and costs. The distinct comparison covered `IntBitSet<uint64_t>` against `MultiIntBitSet<uint64_t,1>`, `MultiIntBitSet<uint32_t,2>`, and `MultiIntBitSet<uint8_t,8>` for 64 positions. `src/test/fuzz/bitset.cpp` compares those representations with an independent `std::bitset` model across 16/32/48/64/96/128/192/256-bit cases. `src/test/fuzz/cluster_linearize.cpp` target `clusterlin_backend_equivalence` compares production `BitSet<64>` linearization with all four representations while parsing graph inputs and exercising topological mode, costs, and chunking.

The initial deterministic corpus contained 16 bitset inputs (5589 bytes) and 10 cluster inputs (241 bytes). The combined corpus manifest SHA256 was `8a23e3289130b61ba51b9707f2317cf6b8346386b9906ecc3b89aab00517683`. The libFuzzer runs intentionally used copies only for this cycle, but libFuzzer appends new units to its input directory; therefore the post-run directory counts are not treated as an independent initial-corpus measurement.

### Independent execution

The current Clang 19 ASan+UBSan libFuzzer binary was `/data/my_storage/tmp/cycle248-build-libfuzzer-minisketch/bin/fuzz`; only journal/state commits occurred since its build. The exact target runs used fixed seeds `24901` and `24902`, `-max_total_time=30`, `-max_len=4096`, `-rss_limit_mb=4096`, and `-timeout=2`.

| target | runs | final coverage | new units | peak RSS | result |
| --- | ---: | ---: | ---: | ---: | --- |
| `bitset` | 3047 | `cov: 18284`, `ft: 63843`, corpus `876/489Kb` | 882 | 389 MB | exit 0, no artifact |
| `clusterlin_backend_equivalence` | 13188 | `cov: 15091`, `ft: 35458`, corpus `292/6998b` | 319 | 677 MB | exit 0, no artifact |

The existing `cluster_linearize_tests` unit control ran 19 cases in `/data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin` and exited with `*** No errors detected`. This was a pre-existing current-source-equivalent build; the relevant production/test files had no later source commits.

An optimized Clang 19 AFL++ persistent target from `/data/my_storage/tmp/cycle131-build-afl19d/bin/fuzz` was run with one worker for 15 seconds, `AFL_NO_FORKSRV=1`, and copies of the initial corpora. The no-forkserver mode is recorded because the default forkserver mode reproducibly aborted on the high-capacity bitset seed with `Unable to request new process from fork server (OOM?)`; the same seed passed in no-forkserver mode. This is classified as an AFL/toolchain-mode limitation, not a production failure.

| target | execs | exec/s | corpus | found | edges | stability | crashes/hangs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `bitset` | 243 | 15.18 | 30 | 15 | 3530 | 100.00% | 0/0 |
| `clusterlin_backend_equivalence` | 202 | 13.27 | 20 | 10 | 4021 | 100.00% | 0/0 |

All 30 optimized AFL bitset queue files and all 20 optimized AFL cluster queue files were replayed through the Clang 19 ASan+UBSan libFuzzer oracle. The bitset replay completed 31 runs with zero new units and 277 MB peak RSS; the cluster replay completed 21 runs with zero new units and 278 MB peak RSS. Both exited 0 with no sanitizer artifact. Raw logs and scratch inputs are preserved under `/data/my_storage/tmp/cycle249-*`, including `cycle249-backend-corpus`, `cycle249-afl-*`, `cycle249-oracle-backend`, and the corresponding replay logs.

### Verdict and limits

**Verdict:** dismissed as a current bitset/backend-equivalence defect. The independent `std::bitset` model, production cluster-linearization comparison, current unit control, optimized AFL++ exploration, and sanitizer oracle replay produced no output, status, accounting, ordering, chunking, crash, hang, or sanitizer divergence. No production repair or permanent test change is justified.

The evidence is x86_64-only and does not establish ARM, 32-bit, big-endian, LTO/PGO, or timing equivalence. The unit control used a pre-existing equivalent build, while the fuzz targets exercised current source with only journal/state commits since compilation. The AFL forkserver failure was not hidden and was independently reproduced as a mode-specific resource/toolchain issue. The libFuzzer corpus directory growth was accounted for rather than misreported as a fresh corpus result. Next action is to close the cycle and draw a fresh catalog goal; reopen this backend cell only for a source change or new architecture, compiler, sanitizer, or optimization evidence.
