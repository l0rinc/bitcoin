# Journal: Bitcoin mempool, package, and eviction-accounting audit (campaign 87)

Uber-goal rotation, severity-first: mempool acceptance/replacement accounting
(DoS + correctness class). Branch: audit/bitcoin-mempool-accounting from
audit/resurrection @ 1e51acacd4.
Prior coverage: goal 89 P2 (orphanage/peer accounting), own fork PRs
238/228/217/209/214 (fee-delta saturation, S8) — don't re-litigate.
Prime fresh surface: cluster mempool (PR 33629, TxGraph in src/txgraph.cpp)
— recently merged, complex accounting, less battle-tested than txmempool.
Tools: unit tests (txgraph_tests, txmempool_tests) + fuzz targets
(txgraph, cluster_linearize) + mempool functional tests on regtest scratch.

## Scope ledger

| # | area | hypothesis seeds | verdict |
|---|------|------------------|---------|
| M1 | TxGraph cluster accounting (new) | oversized/split/merge operations leave count/fee/size accounting inconsistent vs full recompute | open |
| M2 | txmempool ancestor/descendant tracking | removeRecursive/updateForDescendants leaves stale links or wrong cached scores | open |
| M3 | fee deltas (prioritiseTransaction) | mapDeltas vs recomputed entry fee on reorg/reinsert (S8-adjacent, own PRs covered overflow — check accounting-not-overflow) | open |
| M4 | expiry + trimming | expiry boundary removes txs but leaves dependants/indexes inconsistent | open |
| M5 | conflicts/replacement (RBF) | replacement accepted but descendant/conflict sets mis-updated | open |
| M6 | removal for block/reorg reinsertion | DisconnectPool reinsertion leaves duplicates or drops updates | open |

## Verdicts

### M1 (TxGraph cluster accounting, PR 33629): DISMISSED — model-based differential oracle exists and runs

The txgraph fuzz harness (src/test/fuzz/txgraph.cpp) IS the
incremental-vs-full-recompute comparison the campaign asks for:
- SimTxGraph (naive single-DepGraph model, lines 68-180) mirrors every
  operation against real TxGraph and compares after random op sequences.
- Exact accounting checks: tx counts MAIN/TOP vs model (544-545, 725),
  fee/size sums with overflow-aware assertions (53-54, 710-718),
  cluster contents vs model (843), fee-size diagrams vs model-recomputed
  diagrams (715-718, 506-550).
- real->SanityCheck() internal invariant checker invoked repeatedly
  (546, 661).
- Runs continuously in CI + OSS-Fuzz (build status green per E8 check).
No accounting gap found; the new cluster-mempool code ships with the
strongest oracle form. DISMISSED.

### M2 (txmempool ancestor/descendant tracking): DISMISSED — internal consistency checker, exercised post-scenario

### M3 (fee deltas, prioritiseTransaction): DISMISSED — cumulative-from-delta, preserved across reorg by design

mapDeltas lifecycle (txmempool.cpp:731-799):
1. PrioritiseTransaction: saturating CUMULATIVE delta; SetModifiedFee
   derived from the cumulative value, never an intermediate (740-744 —
   saturated intermediates cannot leave stale modified fees; S8 series
   embedded). Zero delta → erased (749-751). Graph fee synced (746).
2. Evict/reinsert: deltas persist in mapDeltas while the tx is absent;
   ApplyDelta (766-776) re-applies on (re)arrival — prioritise-before-
   arrival semantics, intended. Reorg removal (removeForBlock) does not
   erase deltas → DisconnectPool reinsertion restores priority. Intended.
3. ClearPrioritisation: modified fee reset to base fee, graph fee synced,
   delta erased, invariants asserted (778-790).
4. The internal check() (M2) asserts mapDeltas/modified-fee exactness
   post-scenario (91-92). No accounting defect. DISMISSED.

### M4 (expiry + trimming): DISMISSED — staged descendant removal, cluster-complete chunks

Expire (txmempool.cpp:949-965): expired entries collected by entry_time,
then CalculateDescendants stages all descendants before RemoveStaged —
no orphaned descendants. TrimToSize (999+): removes whole graph chunks
(GetWorstMainChunk — cluster-complete by construction), and the rolling
minimum fee floor is raised to removed-rate + incremental relay feerate
(1011-1016) so equal-feerate txs can't immediately re-enter (the
re-admission guard). Rolling halflife decay bounded by the incremental
floor (983-988). Consistent with check() invariants.

### M5 (RBF conflicts): DISMISSED — recursive removal + prioritisation clear

removeConflicts (425-439): each conflicting tx gets ClearPrioritisation
before removeRecursive (graph-authoritative descendants); replacement
itself is validated in MemPoolAccept (policy/rbf.cpp) before conflicts
are removed — replacement validation and conflict cleanup ride on the
same check()-verified structures.

### M6 (removal for block / reorg reinsertion): DISMISSED — re-validation on re-accept, bounded pool

DisconnectedBlockTransactions: bounded (20MB, MAX_DISCONNECTED_TX_POOL_BYTES)
with explicit evicted-vector accounting on overflow (64-66). Newly
confirmed txs are removed from the disconnect pool
(validation.cpp:3143 removeForBlock); remaining txs are re-accepted
through the NORMAL mempool acceptance path (not blind reinsertion —
duplicates and invalid-after-reorg txs handled by regular checks).
UpdateTransactionsFromBlock (validation.cpp:349) updates mempool
descendant state for connected blocks. Consistent by design.

## Campaign 87 cycle complete

All 6 ledger areas locked, all DISMISSED — the accounting oracles are
unusually strong (TxGraph differential fuzz model, CTxMemPool::check()
with exact assertions, cumulative-from-delta fee accounting, cluster-
complete removal). No new defects. Rotation: uber-ledger marks #87 DONE,
next #82/83/84 secp256k1.

CTxMemPool::check() (txmempool.cpp:471+) is a full internal invariant
verifier: randomized-index consistency (34-46), diagram-vs-entry-order
consistency with saturating-fee order-dependence explicitly handled
(51-67 — the S8 fix evidence inline), mapDeltas/modified-fee exactness
(91-92), mapNextTx bidirectional consistency (118-120), duplicate-coin
availability, txgraph SanityCheck + not-oversized asserts (27). Randomly
gated by check_ratio in production; invoked directly after add/remove
scenarios in mempool_tests.cpp (1401, 1461). UpdateForDescendants cached
scores are covered by the mapDeltas/modified-fee assertions. No stale-link
or stale-score candidate found. DISMISSED.

## Next queue
(M1 first: compare TxGraph incremental accounting vs full recompute via the
existing fuzz oracle; check txgraph_tests coverage of split/merge counts)
