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
