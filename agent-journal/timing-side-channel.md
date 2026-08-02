# Campaign #53 — timing-side-channel

Base: 767e6d78fc (journal commit for #103 cycle-1 on
audit/finding-composition; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/timing-channel. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): AES-256-CBC padding timing — no significant leak (Welch t = 1.53 / 1.69 / -1.14 over 3x30k interleaved runs)

### Draw
Random draw over the 43-goal pool (26 pending + 17 CYCLE-1; #103
excluded as just-cycled): raw=286597859504303150, index 11 -> #53.

### Cell selection
#45 c1 verified ctaes + the CBC padding check constant-time BY CODE
READ (aes.cpp:111-127: full 16-iteration scan, padsize mask, no OOB).
This cycle takes the empirical half of the same boundary: a dudect-
style statistical test, since the campaign treats a code argument as
supporting evidence, not proof.

### Harness (/tmp/btc53_dudect.cpp, self-contained)
Two classes, identical ciphertext length (1008 B): (A) valid PKCS
padding, (B) padding broken by flipping the last byte. Interleaved
randomized order, 30000 Decrypt calls of
AES256CBCDecrypt::Decrypt (production object code: aes.cpp + ctaes,
linked against build-before's libbitcoin_crypto/util.a, -O2, same
host/compiler as the node), wall time per call, Welch's t-test.

### Results (three independent runs)
| run | meanA (valid) | meanB (invalid) | Welch t |
|---|---|---|---|
| 1 | 88467.2 ns | 88444.4 ns | 1.53 |
| 2 | 88339.6 ns | 88324.0 ns | 1.69 |
| 3 | 88513.9 ns | 88528.9 ns | -1.14 |
All far below the |t|>4.5 lead threshold; sign flips between runs
(noise-dominated). The invalid class is never consistently slower or
faster.

### Verdict
- DISMISSED (empirical): no measurable padding-validity timing
  channel on this host/toolchain — consistent with the constant-time
  code analysis (#45 c1). The Vaudenay boundary holds at
  implementation level for AES-256-CBC decrypt.
- Method note: ctaes bit-slicing is ~88ns/byte here (1008 B ≈ 88 µs),
  so the padding-check cost (<0.1% of the call) would be visible only
  if it branched — it doesn't.

### Exact commands
- g++ -O2 -std=c++20 -I src -o /tmp/btc53_dudect /tmp/btc53_dudect.cpp
  build-before/lib/libbitcoin_crypto.a build-before/lib/libbitcoin_util.a
- /tmp/btc53_dudect (x3)

### Limitations / queue
- Single host (Cortex-A76); a second microarchitecture would
  strengthen the negative — queued if a second host appears.
- Amplified leakage via repeated-decrypt oracles (network-level
  Vaudenay) requires decryption-with-victim-key — not applicable to
  Bitcoin's encrypt-only-RPC posture (no decryption oracle exposed).
- secp ctime tests (SECP256K1_BUILD_CTIME_TESTS, valgrind-backed)
  unrun this cycle — valgrind availability unverified; queued.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-08-02, draw 205, raw=3235443014574466965 (63-bit), idx 17/28): secp256k1 ctime_tests under valgrind — 0 errors from 0 contexts; constant-time property CONFIRMED at the library level

### Cell
The c1 queue's secp ctime cell: SECP256K1_BUILD_CTIME_TESTS is a
valgrind-backed suite that poisons secrets and fails on any
secret-dependent branch/address — the library-level constant-
time check.

### Evidence
- Host check: valgrind present (/usr/bin/valgrind) — c1's
  'availability unverified' resolved.
- Subtree build: cmake -B /tmp/secp-ctime (Release,
  CTIME_TESTS=ON, others OFF; Valgrind_WORKS success) — 7-edge
  build, seconds (the session's subtree technique).
- Run: valgrind --error-exitcode=42 ./bin/ctime_tests ->
  'ERROR SUMMARY: 0 errors from 0 contexts' + clean heap
  (1 alloc/1 free). The suite's secret-dependence checks all
  pass on this host/toolchain (gcc 13.3, Cortex-A76).

### Verdict
CONFIRMED (negative, second form): the constant-time property
holds at library level under valgrind's dynamic taint analysis,
complementing c1's statistical dudect negative on AES-CBC and
the #45 code-read. No timing-leak signal on any axis tested.

### Exact commands
- cmake/ninja lines above; valgrind run above.

### Limitations / queue
- Single microarchitecture (c1 note stands: a second host would
  strengthen both negatives).
- ctime_tests covers the library's own suites; Core-side usage
  patterns (nonce function choice etc.) are covered by the #45
  family code-reads, not valgrind.
