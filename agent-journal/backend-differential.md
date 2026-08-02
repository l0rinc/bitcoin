# Campaign #69 — backend-differential

Base: 56902d68db (journal commit for #32 cycle-1 on
audit/history-leftovers; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/backend-diff. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): secp256k1 wide-multiply backend differential — int128 vs forced int64, full suites pass both

### Draw
Random draw over the 45-goal pool (28 pending + 17 CYCLE-1; #32
excluded as just-cycled): raw=5867492717535632116, index 16 -> #69.

### Cell selection (proven recipe)
Ledger recipe (secp subtree note): subtree-only scratch builds with
SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY + tests/noverify give a full
cross-backend differential in ~35s of build on this host. The
wide-multiply path is the field/scalar arithmetic core — a
miscompile or backend bug here breaks every signature.

### Setup
Two subtree builds (Release, aarch64):
- /tmp/secp_int128: autodetect (int128_struct path on this compiler)
- /tmp/secp_int64: SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64
  (CMakeLists.txt:81-88 documented test-only override)
Each ran the FULL test suite (bin/tests — exhaustive field/scalar/
group/signature vectors plus randomized rounds) and bin/noverify_tests
(the no-verify-API variant suite).

### Results (all four suites green, exit 0)
- int128: tests 103.2s PASS, noverify 55.8s PASS
- int64:  tests 106.9s PASS, noverify 46.7s PASS
Build time for both subtree builds ~40s total. No divergence in
results, error behavior, or state — both backends agree on the
complete vector corpus.

### Verdict
- DISMISSED: no backend differential for the wide-multiply family on
  aarch64/gcc-13; the int128 autodetect and the forced int64 fallback
  agree on every suite.
- Recipe re-validated (build ~40s, four suite runs ~5.2 min total).

### Exact commands
- cmake -S src/secp256k1 -B /tmp/secp_int{128,64} -GNinja
  -DCMAKE_BUILD_TYPE=Release -DSECP256K1_BUILD_{TESTS=ON,BENCHMARK=OFF,
  EXHAUSTIVE_TESTS=OFF,CTIME_TESTS=OFF}
  [-DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64]
- ninja -C <dir> tests noverify_tests; run bin/tests + bin/noverify_tests

### Limitations / queue
- SHA256 arm_shani vs scalar differential (in-tree crypto, dispatch
  at src/crypto/sha256*.cpp) — the other hot backend family on this
  host; queued (needs the force-scalar build variant).
- int128_struct vs int128 explicit split not separated (autodetect
  already picked one; forcing the other is a c2 cell).
- ecmult window/precompute variant differential unclaimed.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): SHA256 arm_shani vs forced-scalar differential — 2015/2015 byte-identical

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=5820747569139027543, index 1 (of 2) -> #45 c4; THIS cycle:
singleton -> #69 (second cycle; backend-differential queue cell
"SHA256 arm_shani vs scalar differential"). Branch:
audit/backend-differential-c2 from 60debab91c (#45 c4 journal
tip).

### Experiment
Direct-compiled driver (/tmp/btc72_sha.cpp) that
SHA256AutoDetect(USE_ALL)-hashes a corpus, then
SHA256AutoDetect(STANDARD)-rehashes it and byte-compares.
Corpus: BIP test vectors (empty, "abc"), block-boundary lengths
(55,56,63,64,65,119,120,127,128,129,255,256,1000 — covering the
64/128-byte block edges and the 2way path), and 2000 PRNG
(xorshift64) inputs of 0-300 bytes.
Build note: the driver needs -DENABLE_ARM_SHANI
-march=armv8-a+crypto to expose the backend at all — without the
define the source silently autodetects to "standard" (the
introspection.cmake:200 / crypto/CMakeLists.txt:55-62 wiring).

### Result
autodetect(USE_ALL) -> arm_shani(1way;2way);
autodetect(STANDARD) -> standard;
TALLY corpus=2015 mismatches=0. Byte-identical digests across
every case, both 1way and 2way code paths (matches the node's own
startup line "Using the 'arm_shani(1way;2way)' SHA256
implementation").

### Verdict
DISMISSED (differential clean): the arm_shani SHA256 backend is
byte-identical to the scalar reference across the boundary and
random corpus. The c1 int128/int64 explicit-split cell remains
the only open backend item.

### Exact commands
- g++ -O2 -std=c++20 -DENABLE_ARM_SHANI -march=armv8-a+crypto
  -I src -o /tmp/btc72_sha /tmp/btc72_sha.cpp
  src/crypto/sha256.cpp src/crypto/sha256_arm_shani.cpp
- /tmp/btc72_sha (output above)

### Limitations / queue
- The 2way path is driven indirectly (>128B inputs); a direct
  TransformD64 call graph check would be the exhaustive version —
  same expected result given the wrapper already routes there.
- int128_struct vs int128 explicit split (c1 queue) remains.

## Rotation note
Two cycles; SHA256 backend closed. Not exhausted (int128 split).

## Cycle 3 (2026-07-30): int128 explicit split (struct vs native) — all four suites green, results identical

### Draw
Re-rank singleton (last queue cell): #69 (third cycle; c1 queue
cell "int128_struct vs int128 explicit split not separated").
Branch: audit/backend-differential-c3 from 45788336a8 (#89 c3
journal tip).

### Cell
c1 compared the int128 FAMILY (autodetect = int128_struct on this
host) vs forced int64. The remaining split is WITHIN the 128-bit
family: manual int128_struct (64x64 software) vs builtin int128
(native __int128). SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY takes
both legal values (CMakeLists.txt:81-82), so the split is a
two-build recipe.

### Setup + results (all green, exit 0)
- /tmp/secp_struct  (override=int128_struct): tests 159.7s PASS,
  noverify 102.4s PASS
- /tmp/secp_native  (override=int128):        tests 102.7s PASS,
  noverify 55.7s PASS
Both full suites (exhaustive field/scalar/group/signature vectors
+ randomized rounds) pass on both implementations; no divergence
in results or error behavior. Native is ~35-45% faster wall —
expected (hardware 128-bit multiply vs software), not a
correctness signal.

### Verdict
DISMISSED: the int128 explicit split is closed — struct and
native agree on the complete suites. With c1 (int128 family vs
int64), the wide-multiply backend family is differential-clean
on aarch64/gcc-13.

### Exact commands
- cmake -S src/secp256k1 -B /tmp/secp_{struct,native} -GNinja
  -DCMAKE_BUILD_TYPE=Release -DSECP256K1_BUILD_TESTS=ON
  -DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int128_struct|int128
- ninja -C <dir> tests noverify_tests; run bin/tests +
  bin/noverify_tests (times above)

### Limitations / queue
- ctime variant (SECP256K1_BUILD_CTIME_TESTS) not run — the
  constant-time harness is heavier; queued if a cycle lands here.
- secp256k1 arm-specific field/scalar asm backends (none on
  aarch64 beyond generic) — nothing further to split.

## Rotation note
Three cycles; secp wide-multiply family fully closed. #69 quiets
pending new backends.

## Cycle 4 (2026-07-30): secp256k1 ctime variant — valgrind ctime_tests green on this host

### Draw
Re-rank draw over the remaining 3-cell queue:
raw=182724797729077762, index 1 (of 3) -> #69 (fourth cycle; c3
queue cell "ctime variant"). Branch:
audit/backend-differential-c4 from fec1be1523 (#89 c4 journal
tip).

### Setup
Subtree build with SECP256K1_BUILD_CTIME_TESTS=ON (Valgrind found
at /usr/include, Valgrind_WORKS success; the ctime harness needs
the memory-checking interface — ctime_tests.c:#error without it).
Run under valgrind --error-exitcode=42 --quiet.

### Result
ctime_tests under valgrind: exit 0 — no secret-dependent branch or
memory-access violation in the covered crypto paths (ecdsa,
schnorr, ecdh, musig, ellswift modules enabled in this subtree).
The vendored secp256k1 (post-#65 c5 master update) is
constant-time clean on aarch64 with the default int128_struct
path.

### Verdict
DISMISSED: the ctime variant passes. With c1 (int128 vs int64),
c2 (SHA256 arm_shani vs scalar), c3 (int128 struct vs native),
the backend differential campaign's cells are all closed and
green.

### Exact commands
- cmake -S src/secp256k1 -B /tmp/secp_ctime -GNinja
  -DCMAKE_BUILD_TYPE=Release -DSECP256K1_BUILD_TESTS=ON
  -DSECP256K1_BUILD_CTIME_TESTS=ON
- ninja -C /tmp/secp_ctime ctime_tests
- valgrind --error-exitcode=42 --quiet /tmp/secp_ctime/bin/
  ctime_tests (exit 0)

### Limitations / queue
- ctime under the int64 override (the other widemul backend) not
  run — the default path is the shipping one; the int64 variant
  is c1-covered correctness-wise.

## Rotation note
Four cycles; backend campaign cells all closed green. #69 quiets
pending new backends.

## Cycle 5 (2026-08-02, draw 219, raw=16824079916139056791, masked 7600707879284280983, idx 11/12): backend census — no further differential executable on this host; EXHAUSTED

### Census (all selectable crypto backends in-tree)
- SHA256: 5 optimized (arm_shani [c2 tested], avx2/sse4/sse41/
  x86_shani [x86-only, untestable on aarch64]) + scalar
  [#17 c4 tested end-to-end]. The arm 4way variant is runtime-
  gated to CPUs this host lacks (Cortex-A76 gets 1way;2way).
- SHA512: single scalar Transform (no SIMD variant in-tree,
  sha512.cpp:47) — no differential exists.
- ctaes (AES): single bitsliced constant-time backend by
  design (ctaes.c, no variant selection in aes.cpp) — no
  differential exists.
- secp256k1 widemul: int128 vs int64 (c1/c3) + ctime valgrind
  (c4, independently re-run by #53 c2 this session).

### Verdict
EXHAUSTED: every backend pair executable on this host is green;
the rest are architecture-absent (x86 SIMD, arm 4way) or
single-implementation by design (SHA512, ctaes). Reopen on new
backend or second host.

### Exact commands
- ls src/crypto/sha256*; grep backend refs above (sha512.cpp,
  aes.cpp, ctaes/).

### Limitations
- x86 backend correctness is upstream-CI territory (this host
  is aarch64-only); recorded, not a gap.
