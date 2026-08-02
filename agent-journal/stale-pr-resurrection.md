# Journal: stale PR critical-fix resurrection audit (campaign 56)

Uber-goal rotation. Branch: audit/stale-pr-resurrection from
audit/resurrection @ f112f38d99. Seed: reviews/2026-07-25-closed-pr-revival-candidates.md
(18 closed PRs evaluated; 1 YES, 1 MAYBE).

## Verdicts

### PR 33916 (fuzz: wallet TransactionCanBeBumped target) — GAP STANDS, revival path confirmed

- Still closed-unmerged (updated 2026-07-24); no bump/RBF fuzz target
  exists in tree (grep src/test/fuzz + src/wallet/test/fuzz for
  TransactionCanBeBumped → zero hits). Fund-loss-adjacent coverage gap.
- The flagged blocker — UBSan "downcast" at package_eval.cpp:213 — is
  HARNESS-MOCK-ONLY: line 213 today is `LOCK(tx_pool.cs)` on a REAL
  CTxMemPool (constructed at 196-207), and the same harness runs that
  exact line in CI fuzz jobs daily without issue. A master-live downcast
  here would fail every CI fuzz run. The original hit came from the
  mock-mempool variant the review already said to drop.
- Revival path (unchanged from review): non-mocked mempool (as
  package_eval does), direct add-tx helpers for descendant state.
- QUEUED (not done this turn): writing the target is a new-harness
  artifact, not a minimal fix — scheduled as a follow-up cycle.

### PR 35740 (http linger-close after parse errors) — still MAYBE

Still closed-unmerged (2026-07-22). Protocol-robustness improvement, not
critical; no new review. Stands as low-priority revival for anyone
willing to own it.

### Fresh scan (closed-unmerged since 2026-07-25): nothing to revive

- 35816 "test: Add P2SH sigop counting test for segwit blocks" — closed
  by author as DUPLICATE of own open PR 35164 (P2SH sigop coverage in
  test_witness_sigops). Coverage exists via the canonical PR; no revival.
- 35815 — spam (".").

## Next queue
(write the TransactionCanBeBumped fuzz target as a dedicated cycle
(per revival path); then rotate per ledger: #96 TODO/FIXME challenge)
