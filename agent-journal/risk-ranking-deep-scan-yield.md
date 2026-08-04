# #113 — Risk ranking, deep-scan stopping, and marginal-yield audit

## Cycle 1 (2026-08-04, draw r192 raw=706951372696882673) — draw-loop yield measurement + one protocol amendment

First cycle (genuinely pending per the cycle-348 manifest).

MEASUREMENT (ledger-parse, r151-r190, 40 rounds on the post-rebase
base): 29 rounds produced evidence (1 adoption w/ failing-before/
passing-after, 2 conformance/determinism proofs, 1 journal-integrity
repair, 1 catalog manifest, 1 transplant execution, 1 Release-gap
closure, 1 API coverage-gap test, 1 backend differential, 1 lock
discipline verification, plus review-dismissals with recorded
evidence); 11 reopen-FAIL. Yield ~72% evidence-bearing.

VERDICT on the stopping rule: the 20-round NP halt + resume sweep
behaved exactly as designed (halted at static state; the halt sweep
caught the upstream advance + qa-assets move within the same
session). The reopen-first ranking is validated: every resume-
condition cycle produced multi-cycle work.

YIELD DEFECT FOUND + AMENDMENT: repeat draws waste slots — #74 hit at
r154 AND r159 (5 apart), #19 at r172 AND r183 (11 apart), #51 at
r161 AND r191 (30 apart, still static), #36 at r158/r188-area. A
reopen-FAIL goal re-drawn within a short window re-runs the same
static check. PROTOCOL AMENDMENT (adopted starting now): a goal that
just returned reopen-FAIL goes on a 25-draw cooldown unless a fresh
trigger (upstream advance, remote move, new finding) explicitly
reopens it. Cooldown set recorded in the ledger's round log.
Expected effect: FAIL rate drops as static goals self-exclude.

Limitations: 40-round sample, post-rebase era (unusually
target-rich); the FAIL rate will rise when upstream goes quiet —
the cooldown is exactly the mitigation.
