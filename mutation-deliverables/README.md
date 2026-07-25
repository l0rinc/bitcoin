# Mutation-campaign deliverables: mixed P2SH/witness sigop coverage

These files close the two consensus-sigop coverage gaps found by mutation
testing (mutations M3 and W2 in `src/consensus/tx_verify.cpp`):

- **M3** — conditioning the P2SH sigop branch on `!tx.HasWitness()`:
  `if ((flags & SCRIPT_VERIFY_P2SH) && !tx.HasWitness())`
  (the mutation l0rinc demonstrated in bitcoin/bitcoin#35164 review /
  l0rinc/bitcoin#248). Undercounts sigop cost of mixed-input transactions:
  their P2SH sigops are dropped, so an over-limit block can be accepted.
  Consensus-splitting bug.
- **W2** — the symmetric mirror discovered during this campaign: skipping
  the witness sigop loop when the transaction has any P2SH input. Same
  severity, opposite direction: witness sigops of mixed-input transactions
  are dropped. Was not caught by any master test, including the tests added
  by bitcoin/bitcoin#35164 (its mixed case is P2SH-dominated, so the witness
  contribution is not decisive for the rejection).

## Why the gaps existed (root cause)

1. **Structural segregation of the test suites.** Every sigop-limit test
   built either pure-witness or pure-P2SH transactions. The two accounting
   branches in `GetTransactionSigOpCost` interact only for mixed-input
   transactions, and nothing constructed one at the block-limit boundary
   where each contribution is individually decisive.
2. **Fuzzer blind spot.** No fuzz target constructed hash-consistent P2SH
   scriptPubKey/scriptSig pairs. A coverage-guided fuzzer mutating random
   bytes can essentially never produce a scriptSig whose pushed redeem
   script hashes to the scriptPubKey's hash160 (~2^160 work), so the P2SH
   sigop paths were unreachable by fuzzing unless the target *constructs*
   consistent pairs from fuzz input. The witness side was covered (e.g.
   `script.cpp`, `tx_pool.cpp`) only because witness programs need no such
   preimage.
3. **Self-checking anti-pattern.** Where production code is only ever
   compared against itself (or against outcomes it fully determines),
   condition-level mutations are invisible. A differential reference with an
   independent aggregation structure is required.

## Contents

- `fuzz/mixed_sigops.cpp` — new differential fuzz target. Builds mixed
  P2SH + P2WSH transactions (with hash-consistent script pairs derived from
  fuzz input), then compares `GetP2SHSigOpCount` / `GetTransactionSigOpCost`
  against a locally computed reference under all contract-valid flag combos
  (`0`, `P2SH`, `P2SH|WITNESS`; `WITNESS` without `P2SH` is contract-invalid:
  `CountWitnessSigOps` asserts on it at `src/script/interpreter.cpp:2154`).
  The P2SH redeem scripts are bare CHECKSIG chains, which can never parse as
  witness programs, so the P2SH-wrapped-witness path is never ambiguous.
- `seeds/mixed_sigops_seed1` — hand-crafted 69-byte seed decoding to the
  minimal decisive transaction: 1 P2SH input with 3 CHECKSIGs + 1 P2WSH
  input with 2 CHECKSIGs (expected cost 3*4 + 2 = 14). Trips immediately
  under either mutation; also useful as a regression seed.
- `p2p_segwit_witness_decisive.patch` — functional test (applies to
  `test/functional/p2p_segwit.py`, `test_witness_sigops`) adding a
  witness-decisive mixed block: 59 P2SH inputs (69148 sigop-cost, under the
  80000 limit) + 40 P2WSH inputs (11720, individually decisive) = 80868 >
  80000. Catches both M3 and W2, where PR #35164's block_7 catches only M3.
  Self-contained: funds its own outputs in block_7b and preserves the
  OP_TRUE cleanup utxo needed by downstream tests.

## Registration (upstream shape)

Add `mixed_sigops.cpp` to the `add_executable(fuzz ...)` source list in
`src/test/fuzz/CMakeLists.txt` (alphabetical, after `mini_miner.cpp`).
No other build changes are needed.

## Proof log (this campaign, master 3a2c52f9d7, Debug -ftrapv build)

| build | seed run | 1000-input random sweep |
|---|---|---|
| clean master | exit 0 | exit 0 (no false positives) |
| M3 injected | SIGABRT, `mixed_sigops.cpp:108` assert | aborts (found naturally at a structured-random input) |
| W2 injected | SIGABRT, `mixed_sigops.cpp:108` assert | — |

Reproduce: configure with `-DBUILD_FUZZ_BINARY=ON`, build `fuzz` (or a
minimal executable from `fuzz.cpp` + `mixed_sigops.cpp` + `test_fuzz`),
then `FUZZ=mixed_sigops ./bin/fuzz_sigop seeds/mixed_sigops_seed1`.
