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
| 82 | secp-field-scalar-matrix | DONE | 2026-07-27 | both backends green same-seed; tests/noverify pair green |
| 83 | secp-group-ecmult | DONE | 2026-07-27 | parity oracles green both backends + exhaustive order-13 |
| 84 | secp-nonce-session | DONE | 2026-07-27 | all 5 dismissed; zero-before-checks enforcement verified |

## Done since seeding
| 5/52 | boundary/integer | DONE | 2026-07-27 | B1-B4 inclusive-boundary consistent; audit/boundary-integer |
| 62 | rejected-finding-resurrection | DONE | 2026-07-27 | R1 attack on P2.1 FAILED (dismissal strengthened) |
| 56 | stale-pr-resurrection | DONE | 2026-07-27 | 33916 gap confirmed, target queued; scan empty |

## Next-up queue (severity-first)
1. 96 — TODO/FIXME challenge — NEXT
2. 20 — micro-optimization (user-directed interest earlier)
3. then re-rank from accumulated journals

## Handoff
Updated after every rotation. Current: #86, #88, #87, #82, #83, #84,
5/52, 62, 56 DONE.
Technique note for future secp cycles: subtree-only scratch builds with
SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64 + tests/noverify -j4 give a
full cross-backend differential in ~35s on this host. NEXT: #96 on branch
audit/todo-deferred-work.
