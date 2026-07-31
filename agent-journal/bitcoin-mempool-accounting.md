# Bitcoin mempool, package, and eviction-accounting audit

## Cycle 182 start gate

- Cycle: 182
- Selected goal: 87, `bitcoin-mempool-accounting`
- Exact selector: `shuf -i 0-98 -n 1` -> `87`
- Branch: `uber-cycle-182-bitcoin-mempool-accounting-20260731`
- Start HEAD: `689efba2e58fd231eda120b194549f027e255b21`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence (`origin/master...HEAD`): `42 1153`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Corrected TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc`
- Uber-goal state hash at gate: `3280ddae09f116417ccece021a40759d9f9c18053210b3941c4a9c8717f312e7`
- TSV schema check: `records=99 ids_0_to_98=yes`
- Tracked/index state: clean at gate; known agent-owned untracked artifacts are preserved and excluded from commits.
- Storage gate: root filesystem had about 94 MiB free; `/data` had about 49 GiB free. Scratch data must stay under `/data/my_storage/tmp`.
- Preserved processes: PID 777094 (`wallet_tests`) and PID 956381 (`util_tests`) remain unrelated and must not be terminated.

This goal had no existing dedicated journal, so no Goal 87 hypothesis is closed by prior Goal 87 work. This cycle must not reopen the documented orphanage/global deduplicated-accounting cell from Cycle 52, the allocator/prevector and existing mempool-trimming resource cell from Cycle 53, the compact-block provenance recipe from Cycle 120, or unrelated database/accounting findings from Cycle 181. The selected goal is the state-machine contract: acceptance, replacement, ancestor/descendant links, fee/size counters, expiry, trimming, conflicts, reorg removal, and exact graph/accounting symmetry after operation sequences.

Initial hypotheses, ordered for evidence gathering:

1. Incremental ancestor/descendant metadata can diverge from a recomputed graph after a mixed replacement and descendant-removal sequence, especially when a replacement removes transactions that also have descendants.
2. Package acceptance or rejection can leave an entry in a secondary index/counter after the main pool is unchanged, or can update a counter without the corresponding graph edge.
3. Trim, expiry, block removal, and reorg reinsertion may disagree about fee/size/accounting ownership when overlapping ancestor and descendant sets are removed.
4. Cluster/package metadata may have a stale union or membership result after a conflict replacement, but this is only actionable if a deterministic production-path state and an independent oracle demonstrate divergence.

The first experiment will inventory the current `CTxMemPool` contracts and existing fuzz/test helpers, then build a small deterministic state model over valid transaction graphs. The model will compare full recomputation of membership, parent/child edges, ancestor/descendant closures, fees, virtual sizes, and dynamic usage against the pool after each accepted or removing operation. Existing tests will be used as controls; new tests will be added only for a demonstrated mismatch.

## Evidence ledger

| ID | Hypothesis / surface | Status | Evidence and next action |
|---|---|---|---|
| H1 | Replacement plus descendant removal leaves graph or aggregate metadata stale | unchecked | Build a minimal parent/child/conflict sequence and compare incremental state with a recomputed oracle. |
| H2 | Package acceptance/rejection leaves secondary state after the primary pool state is unchanged | unchecked | Inspect package admission rollback and rejection caches; use state snapshots around injected rejection paths. |
| H3 | Trim/expiry/block removal/reorg removal disagree on overlapping-set accounting | unchecked | Map each removal API and test a sequence with shared ancestors, descendants, and conflicts. |
| H4 | Cluster/package union metadata diverges after replacement | unchecked | Check current cluster code and only retain if it is distinct from prior compact-block/orphan accounting work. |

## Commands and results

The start gate and selector are recorded above. Detailed source reads, test commands, raw output, independent verifier results, mutations, verdicts, and handoff will be appended as the cycle proceeds.

## Handoff

No candidate has a verdict yet. Continue with source and test inventory before making a change. Do not claim repository completion.
