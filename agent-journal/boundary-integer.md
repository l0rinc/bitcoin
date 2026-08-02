# Journal: boundary-condition/off-by-one + integer audit (campaigns 5/52)

Uber-goal rotation. Branch: audit/boundary-integer from audit/resurrection
@ a70a5bca7a. Shared lineage: #5 (boundary off-by-one) + #52 (integer
overflow/narrowing/signedness/division).
Prior coverage (don't redo): goal 85 consensus boundary mutants (maturity,
seqlock, witness — all killed); campaign 98 ubsan integer suppressions;
campaign 97 D2/D9/D10; S6/S8 shapes (bloom zero-div, fee-delta saturate).
Fresh surfaces prioritized: NEW code bounds (cluster mempool, TRUC/v3
packages), then goal-85 leftovers (IsFinalTx LOCKTIME_THRESHOLD mixing).

## Scope ledger

| # | area | boundary table hypothesis | verdict |
|---|------|---------------------------|---------|
| B1 | TRUC/v3 package limits | TRUC_CHILD_MAX_WEIGHT, package count/size limits — off-by-one at exactly-limit vs limit+1 | open |
| B2 | cluster mempool size/count limits | -maxmempool/-limitcluster sizes: accepted at limit vs rejected at limit+1 | open |
| B3 | IsFinalTx LOCKTIME_THRESHOLD mixing | nLockTime < threshold compares HEIGHT, >= compares TIME — boundary at exactly 500000000 | open |
| B4 | MiniMiner/cluster cutoff bounds | mini-miner truncation at target feerate boundary | open |

## Verdicts

### B1 (TRUC/v3 package limits): DISMISSED — inclusive limits, consistent across paths, exact-boundary tested

Boundary table (truc_policy.cpp + mempool_truc.py):
- vsize: 9999/10000/10001 → accept/accept/reject. Rejection is vsize >
  TRUC_MAX_VSIZE in both the package path (73, Assume ≤) and single path
  (202). Consistent, and matches BIP431 "must be within" (inclusive).
- child vsize: 999/1000/1001 → accept/accept/reject (> TRUC_CHILD_MAX_VSIZE,
  line 93).
- ancestors: 1 parent + self = 2 (at TRUC_ANCESTOR_LIMIT) accepted;
  2 parents + self = 3 rejected (78/84, count includes self per constant
  docs, truc_policy.h:24-27).
- Functional coverage of limit±1: mempool_truc.py:62-112 (limit+1 rejected,
  limit-3 accepted). static_assert specialization guard (64).

### B3 (IsFinalTx LOCKTIME_THRESHOLD mixing): DISMISSED — exact spec boundary

tx_verify.cpp:21 `(int64_t)tx.nLockTime < ((int64_t)tx.nLockTime <
LOCKTIME_THRESHOLD ? nBlockHeight : nBlockTime)`. Boundary table:
nLockTime = 499999999 → HEIGHT compare; 500000000 → TIME compare;
500000001 → TIME compare. Matches BIP65 exactly (< 500M = height,
≥ 500M = timestamp). Casts are to int64_t on both sides — no signedness
mixing. (Closes goal-85's queued leftover F.)

### B2 (cluster mempool limits): DISMISSED — inclusive boundary, unit + functional pairs

Chain: CheckMemPoolPolicyLimits (validation.cpp:1358 etc.) →
CTxMemPool::ChangeSet::CheckMemPoolPolicyLimits (txmempool.cpp:1233) →
TxGraph::IsOversized(TOP). Boundary encoded in tests: at exactly 64 the
cluster is accepted (mempool_cluster.py:119-125 — replacement 64+1-1=64
passes with maxfeerate=0), at 65 rejected (100/117 "too-large-cluster").
Unit pairs txgraph_tests.cpp:100/105, 158/163 (IsOversized true/false at
boundary states). Inclusive-limit semantics consistent everywhere.

### B4 (MiniMiner target-feerate cutoff): DISMISSED — inclusive, consistent, saturation-flagged

Selection vs verification both use inclusive ≥ target:
mini_miner.cpp:318-323 (package selection assumes ancestor_package_fee >=
target) and 353-365 (m_selected_packages_meet_target=false iff exact fee
< target — exactly-at-target MEETS). Aggregate verification assumes
m_total_fees >= target only while the exact-sum/saturation flags hold
(379-386 — S8 order-dependence handled inline).

## Campaigns 5/52 cycle complete

B1 TRUC limits, B2 cluster limits, B3 locktime threshold, B4 miniminer
cutoff — all inclusive-boundary consistent, all DISMISSED with exact
boundary tables and test evidence. Prior coverage of consensus boundaries
(goal 85) and integer wrap/shift classes (98/97) closes the remaining
lineage. Rotation: uber-ledger marks 5/52 DONE, next #62.

## Next queue
(B1 first: read TRUC limit constants + enforcement sites vs BIP431 text;
then B3 leftover)
