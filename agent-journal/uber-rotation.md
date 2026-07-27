# Uber-goal rotation ledger

The uber-goal works the 99 campaigns in agent-journal/campaign-goals-99.md
(shared boilerplate + per-goal campaign-focus text). Each turn: pick the next
campaign by the rotation policy, run a bounded cycle per its protocol, commit
journal updates, then record state here and rotate. Campaign prompts are
reconstructed as: shared boilerplate + the goal's campaign-focus section.

## Rotation policy
- Severity-first ordering, then least-recently-run. Skip or defer campaigns
  whose prerequisites are missing on this machine (e.g. dm-flakey for 72,
  KLEE/CBMC for 77 — note as deferred, don't grind).
- A few cycles (typically 1–4 turns) per campaign, then rotate — never park
  on one campaign until the user says so. Bounded slices; exact handoff in
  the campaign's own journal each turn.
- Every finding gets one independent commit as Lőrinc <pap.lorinc@gmail.com>;
  journal-only commits for verdicts. Never claim the tree exhausted.
- Branch: one campaign branch per campaign (audit/<slug>) from the ledger
  base; journals committed there. Reviews/ stays on audit/resurrection.
- The three watch crons keep firing; answer them in their own turns.

## Campaign state
| # | campaign | state | last cycle | notes |
|---|----------|-------|-----------|-------|
| 26 | cross-subsystem-bug-shapes | DONE | 2026-07-27 | S1/S2/S5 mapped; journal on audit/resurrection |
| 33 | external-vulnerability-variants | DONE | 2026-07-27 | E1-E10; 0 local variants; audit/external-vulns |
| 85 | bitcoin-consensus-mutation | DONE | 2026-07-27 | 6 mutants all killed; journal on audit/resurrection |
| 86 | bitcoin-chainstate-symmetry | DONE | 2026-07-27 | all 7 areas locked; C2 defect = own PR 35714; C6 caveat noted |
| 89 | bitcoin-p2p-accounting | DONE | 2026-07-27 | P1-P7 dismissed; journal on audit/resurrection |
| 97 | cpp-defect-taxonomy | DONE | 2026-07-27 | all 18 classes + leftovers; audit/cpp-taxonomy |
| 98 | float-sanitizer-fuzz-exclusions | DONE | 2026-07-27 | 1 finding (99d98861fc); audit/float-sanitizer |
| (all others) | — | PENDING | — | unrun |
| 88 | bitcoin-wallet-recovery | DONE | 2026-07-27 | 6 dismissed; W4 finding fixed (0e7a8fabb5) |
| 87 | bitcoin-mempool-accounting | DONE | 2026-07-27 | all 6 dismissed (strong existing oracles) |

## Next-up queue (severity-first)
1. 82 — secp256k1 field/scalar representation matrix — NEXT
2. 83 — secp256k1 group/ecmult formula parity
3. 84 — secp256k1 nonce/signing/Schnorr/MuSig state machine
4. 5/52 — boundary/integer (shared lineage)
5. 62 — rejected-finding resurrection (feeds on accumulated journals)
6. 56 — stale PR resurrection
7. 96 — TODO/FIXME challenge
8. 20 — micro-optimization (user-directed interest earlier)
9. then re-rank from accumulated journals

## Handoff
Updated after every rotation. Current: #86, #88, #87 DONE. NEXT: #82
secp256k1 field/scalar matrix — note: secp256k1 is a VENDORED SUBTREE
(src/secp256k1), changes belong upstream (bitcoin-core/secp256k1);
audit locally, fix via subtree-appropriate commits, journal locally.
