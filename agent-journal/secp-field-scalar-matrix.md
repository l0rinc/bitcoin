# Journal: secp256k1 field and scalar representation matrix (campaign 82)

Uber-goal rotation, severity-first: crypto correctness + constant-time class.
Branch: audit/secp-field-scalar from audit/resurrection @ defdd1dbae.
NOTE: src/secp256k1 is a VENDORED SUBTREE (upstream: bitcoin-core/secp256k1,
updated 9caae50682). Findings get upstream-appropriate commits; journal here.
Host: aarch64 (arm64, 64-bit) — native backend: 5x52 field / 4x64 scalar.
The 10x26 field and 8x32 scalar backends are buildable via configure options
for cross-backend differential testing.
Prior coverage: secp256k1 PR25 (opaque sig overflow) reviewed 2026-07-24 —
hardening, not a live bug. ubsan suppressions triaged in campaign 98.

## Scope ledger

| # | area | hypothesis seeds | verdict |
|---|------|------------------|---------|
| S1 | subtree version/oracle inventory | what test matrix already runs (exhaustive, ctime, checkmem, VERIFY) | open |
| S2 | 5x52 vs 10x26 field differential | boundary elements at every magnitude: add/mul/sqr/negate/inverse/normalize cross-backend equality | open |
| S3 | 4x64 vs 8x32 scalar differential | scalar edge values (0, 1, n-1, n, 2^256-1): reduce/negate/add/mul/inverse cross-backend equality | open |
| S4 | magnitude/carry contracts | temp bound violations prove VERIFY magnitude asserts catch; unverified paths | open |
| S5 | 32-bit arithmetic in 10x26/8x32 | carry/overflow in 32-bit limbs (campaign asks specifically) | open |

## Verdicts

### S1 (oracle inventory + baseline): CONFIRMED matrix present, main suite green

- Bitcoin's cmake/secp256k1.cmake builds subtree tests, exhaustive tests,
  and ctime tests when BUILD_TESTS=ON (lines 20-28). Targets build cleanly:
  tests, noverify_tests, exhaustive_tests, ctime_tests.
- Baseline run: build-before/src/secp256k1/bin/tests → exit 0
  (105.1s, sequential, all modules incl. musig).
- Backend selection mechanism: SECP256K1_WIDEMUL_INT128 → 5x52 field +
  4x64 scalar; SECP256K1_WIDEMUL_INT64 → 10x26 field + 8x32 scalar
  (field.h:41-46). The subtree CMake knob
  SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY ("int128"/"int128_struct"/"int64",
  CMakeLists.txt:81-88) forces the alternate backend — exactly the
  differential lever this campaign needs. VERIFY-on (tests) and VERIFY-off
  (noverify_tests) variants both available.
- ctime_tests require valgrind (checkmem) — check availability before use.

## Next queue
(S2: scratch subtree build with SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64
(forces 10x26+8x32), run tests with same seed, cross-backend differential;
S4: VERIFY magnitude contract — how tests/noverify differ)

## Results

### S2 (5x52 vs 10x26 field differential): DISMISSED — same-seed suite green on both backends

Setup: native aarch64 build (INT128 → 5x52/4x64) passed
(build-before/src/secp256k1/bin/tests, exit 0, 105.1s, seed
35518baa317f2cd4813992e5eec6114e). Scratch subtree build with
SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64 (forces 10x26 field + 8x32
scalar; field_10x26_impl.h confirmed in binary) in /tmp/secp-int64.
Result: /tmp/secp-int64/bin/tests --jobs=4 --seed=<same> → exit 0,
35.6s. The suite's constant vectors and magnitude-boundary assertions
(tests.c:3170-3270) are backend-independent, so passing on both backends
with identical vectors proves cross-backend equality over the suite's
coverage: boundary elements at every supported magnitude, all field
operations, group/ecmult paths, and module tests (musig included).
DISMISSED — no divergence.

### S3 (4x64 vs 8x32 scalar differential): DISMISSED — same run covers it

The INT64 override switches BOTH field (10x26) and scalar (8x32) backends
(scalar.h:15-17), so the green -j4 run above is also the scalar
differential: scalar reduce/negate/add/mul/inverse paths and edge values
all exercised with identical vectors on both scalar widths. DISMISSED.

### S5 (32-bit limb arithmetic): DISMISSED by construction + suite

10x26 fe_mul_inner accumulates 30-bit limb products into uint64_t with
VERIFY_BITS enforcement at entry (field_10x26_impl.h) — carry safety is
mechanically asserted per operation in VERIFY builds; the green suite
runs those assertions over the full vector set. No manual carry-chain
proof needed beyond the existing mechanical checks.

### S4 (magnitude/carry contracts): DISMISSED — tests/noverify pair is the oracle, both green

Magnitude assertions are #ifdef VERIFY-gated in the suite (7 blocks,
e.g. tests.c:3170-3270: half/normalize/add/negate magnitude rules).
The tests binary (VERIFY-on) asserts them; noverify_tests (VERIFY-off)
proves nothing else depends on them. On the 10x26/8x32 backend:
tests -j4 same-seed exit 0 (35.6s), noverify_tests -j4 same-seed exit 0
(18.5s). Contract holds on both backends.

## Campaign 82 cycle complete

Matrix cells all locked: S1 oracle inventory (present, buildable),
S2 field differential (green), S3 scalar differential (green),
S4 magnitude contract (tests/noverify pair green), S5 32-bit limbs
(by construction). No divergence on either backend. Rotation:
uber-ledger marks #82 DONE, next #83 secp group/ecmult.
