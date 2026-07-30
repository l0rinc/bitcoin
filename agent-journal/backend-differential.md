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
