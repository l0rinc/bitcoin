# Bitcoin mempool, package, and eviction-accounting audit

## Cycle 271: retained empty TxGraph memory trips stale zero-usage assertion

### Selection and gate

- Exact selector after the Cycle 270 state close: `shuf -i 0-98 -n 1` -> `87` (`bitcoin-mempool-accounting`); distinct evidence remained open, so no reroll was needed.
- Branch: `uber-cycle-271-bitcoin-mempool-accounting-20260802`.
- Gate HEAD was `2b2e79c78cc2516e36d25e780ba25fc043e5adee`; fetched `origin/master` was `556988790a7f961693a8fd93f73725baea66476a`; merge-base was `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `1331 45` (`HEAD...origin/master`).
- The tracked/index state was clean and `git diff --check` passed at entry. Existing untracked agent/user artifacts, package metadata, `node_modules/`, `test/cache/`, crash files, and profiling output were preserved and excluded from staging.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Corrected TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Uber-goal state SHA-256 at the unchanged gate state: `fdd687b1b3fc690dccc796d9d7db7e432d571caad9bfb6020f96630d987d2825`.
- Protected long-running processes `777094`, `956381`, `1138182`, `1157959`, `1312049`, `1312050`, and `1346200` were alive and untouched. Root scratch space was exhausted during the cycle (`/` reported 0 bytes free); all disposable builds and test data were kept under `/data/my_storage/tmp`, which had 14 GiB available at the evidence check.

### Prior cells and scope

The existing Goal 87 ledger's Cycle 182 independent graph/accounting oracle was searched. Its diamond graph/removal sequence and the recorded H1-H4 results were not reopened. The later history and state ledger were also checked for the already-fixed orphanage/global deduplicated accounting, allocator/prevector, compact-block provenance, package test-accept cache cleanup, reorg dependency repair, conflict index, trim no-spends output, randomized index, block-builder, fee-saturation, and retained-memory accounting cells.

The distinct cell for this cycle is the interaction between retained `TxGraph` allocations and the empty-graph accounting invariant. Commits `c516b42ffdee007ecabcb0a406904026a2494c31` and `48652bffd6eb9d21db69402449b42d8be4979a01` intentionally added retained entry, cluster-container, removal-buffer, and unlinked-index allocations to `GetMainMemoryUsage()`. The function still had the older assertion `(usage == 0) == (m_main_clusterset.m_txcount == 0)`, which became invalid once empty graphs were allowed to retain capacity.

### Working hypothesis and pre-fix evidence

After a graph has held and removed many transactions, its main transaction count can be zero while its owned vectors and indexes retain allocated memory. A later mempool `DynamicMemoryUsage()` query must report that memory without aborting. The old assertion treated the zero-count state as proof that usage must be zero, so normal cache/mempool setup and RPC/eviction accounting could terminate a debug/assume-on-failure process after graph churn.

The failure was reproduced independently in a fresh process with the current tree before this cycle's fix:

```text
mkdir -p /data/my_storage/tmp/cycle271-package-accounting-isolated
TMPDIR=/data/my_storage/tmp/cycle271-package-accounting-isolated \
  /data/my_storage/tmp/cycle270-minisketch-build/bin/test_bitcoin \
  --run_test=txpackage_tests/package_test_accept_preserves_coins_cache \
  --random=271002 --log_level=test_suite --report_level=detailed --color_output=false
```

The command exited `201` with:

```text
txgraph.cpp:3789 virtual size_t (anonymous namespace)::TxGraphImpl::GetMainMemoryUsage(): Assertion `(usage == 0) == (m_main_clusterset.m_txcount == 0)' failed.
unknown location(0): fatal error: in "txpackage_tests/package_test_accept_preserves_coins_cache": signal: SIGABRT
```

The broader pre-fix control also reached this first assertion, then produced cascading fixture/argument-registration failures; it was stopped rather than treating those secondary failures as independent findings. The direct package test failure is sufficient evidence because it is deterministic in a clean process and occurs before the package is submitted.

### Fix and independent verification

The invariant now preserves the valid direction only: a nonempty main graph must have nonzero accounted usage. Empty graphs may have nonzero usage because retained allocations are intentionally included. A direct regression in `src/test/txgraph_tests.cpp` creates 1,024 entries, removes them all, confirms `GetTransactionCount(MAIN) == 0`, releases the external refs, and requires `GetMainMemoryUsage() > 0` without an abort.

- Incremental isolated CMake Debug/Clang 19 rebuild of `test_bitcoin` passed after changing `src/txgraph.cpp` and the TxGraph test.
- The new `txgraph_memory_usage_allows_retained_empty_graph` case passed 2/2 assertions with seed `271003`.
- The previously failing `txpackage_tests/package_test_accept_preserves_coins_cache` case passed 5/5 assertions with seed `271004`.
- The fixed combined control `--run_test=mempool_tests,txgraph_tests,txpackage_tests,rbf_tests --random=271005` passed 75 cases and 3,166/3,166 assertions.
- `git grep` confirmed `GetMainMemoryUsage()` feeds `CTxMemPool::DynamicMemoryUsage()`, RPC `getmempoolinfo` usage, eviction thresholds, chainstate cache sizing, and fuzz/state-machine checks; this is therefore an accounting-invariant fix rather than a test-only workaround.
- No wallet, key, default datadir, protected process, or root `/tmp` scratch was used. The pre-fix abort and post-fix controls provide independent failing-before/passing-after evidence; no sanitizer rebuild was attempted because the current Clang build was already available and the defect is an explicit assertion transition.

Verdict: confirmed and fixed local mempool/TxGraph accounting defect. The retained allocation accounting was correct; the stale zero-usage equivalence assertion was not. The fix does not change eviction policy or graph membership, and it leaves nonempty-graph sanity checking intact.

Limitations: no full sanitizer rebuild or fuzz-worker run was performed in this cycle, and root filesystem exhaustion prevented use of default temporary paths. The selected focused controls cover the affected accounting and package/RBF/removal paths; `/data` scratch remains available. The next queue is to exercise retained-empty accounting through the RPC/cache-resize and eviction paths, then continue package rejection and replacement state transitions without repeating this exact assertion cell.

## Cycle 271 handoff

The source and state commits for this cycle will be recorded after the fix and journal are validated. The next cycle must perform a fresh gate, draw exactly one selector from `0..98`, and create a new dedicated branch; do not claim repository completion.

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
| H1 | Replacement plus descendant removal leaves graph or aggregate metadata stale | not confirmed in tested sequence | The independent diamond/removal oracle passed; keep broader replacement sequences in the queue. |
| H2 | Package acceptance/rejection leaves secondary state after the primary pool state is unchanged | unchecked | Inspect package admission rollback and rejection caches; use state snapshots around injected rejection paths. |
| H3 | Trim/expiry/block removal/reorg removal disagree on overlapping-set accounting | unchecked | Map each removal API and test a sequence with shared ancestors, descendants, and conflicts. |
| H4 | Cluster/package union metadata diverges after replacement | oracle gap fixed; no defect in tested sequence | The independent connected-component check covers clusters for the representative graph; broader replacement coverage remains useful. |

## Commands and results

The start gate and selector are recorded above. Detailed source reads, test commands, raw output, independent verifier results, mutations, verdicts, and handoff will be appended as the cycle proceeds.

## Cycle 182 finding: independent mempool graph/accounting oracle

### Contract and trust boundary

The relevant contract is that the live `CTxMemPool` graph and its aggregate
queries agree with the raw transaction-input graph after every accepted,
prioritized, recursively removed, block-removed, and block-reinserted state.
For an entry, ancestors and descendants include the entry itself; their
counts, virtual-size sums, and modified-fee sums must be exact. A cluster is
the undirected connected component, so it also includes cousins connected
through a shared parent or child. The trust boundary is the production
`CTxMemPool`/`TxGraph` integration: the oracle must not reuse the graph
implementation whose wiring it is checking.

### Source and test inventory

`CTxMemPool` maintains `mapTx`, `mapNextTx`, randomized membership, fee and
usage totals, and a `TxGraph`. `UpdateTransactionsFromBlock()` repairs
parent-child dependencies after block removal/reinsertion, while entry
destruction unlinks graph references. Existing ancestry tests and the
`tx_pool` fuzz target exercise useful direct and API-level properties, but
the transitive checks compare `GetTransactionAncestry()` with
`CalculateAncestorData()`, both of which use `TxGraph`. They did not
independently recompute closures and clusters from the transaction inputs.

The new `CheckMempoolGraphAccountingModel()` in
`src/test/mempool_tests.cpp` builds independent parent and child maps from
`mapTx`, computes transitive closures and undirected components, and compares
them with `CalculateAncestorData()`, `CalculateDescendantData()`, and
`GetCluster()`. `MempoolGraphAccountingStateMachine` drives a diamond graph
(parent, two siblings, merging child, and unrelated transaction), fee
prioritization, recursive removal, re-addition, block removal, dependency
repair, and merge removal. This is a behavior oracle, not an execution-only
coverage test.

### Evidence and verdict

- The normal focused run passed 1 case and 588 assertions:
  `TMPDIR=/data/my_storage/tmp/cycle182-controls /data/my_storage/tmp/cycle170-mempool-build/bin/test_bitcoin --run_test=mempool_tests/MempoolGraphAccountingStateMachine --random=182087 --log_level=test_suite --report_level=short --color_output=false`.
- The normal combined controls passed 49 cases and 1,624 assertions for
  `mempool_tests,txgraph_tests` with seed `182089`; the separate controls
  passed 25/425 and 23/611 for mempool and TxGraph respectively.
- A disposable mutation removed the `TxGraph::AddDependency` call in
  `UpdateTransactionsFromBlock()`. The focused test then failed 28 of 564
  assertions, including ancestor counts/sizes/fees of `3/227/40000` versus
  `4/259/50000`, descendant counts/sizes/fees of `1/32/10000` versus
  `4/259/50000`, and cluster mismatches. The mutation was restored and the
  clean focused test returned to 588/588.
- The first oracle draft incorrectly modeled a cluster as ancestors union
  descendants. It failed on the diamond's cousins; changing it to an
  independent undirected connected-component traversal matched the documented
  `TxGraph` cluster contract. This is useful evidence that the oracle is
  semantically independent rather than copied from the implementation.
- Clang 19 UBSan (`-fsanitize=undefined,alignment,object-size`) passed the
  focused case with 588/588 assertions and no diagnostic:
  `/data/my_storage/tmp/cycle106-clang19-ubsan/bin/test_bitcoin --run_test=mempool_tests/MempoolGraphAccountingStateMachine --random=182090 --log_level=message --report_level=short --color_output=false`.

Verdict: confirmed test/oracle gap, fixed by the focused independent model
and state-machine regression. No production mempool defect was demonstrated
in the clean source. H1 is not confirmed by this sequence; H4's cluster
relation is now covered for this representative graph. The change is not a
production behavior change.

### Remaining cells and limitations

H2 remains open: package acceptance/rejection and secondary-state rollback
were not independently modeled here. H3 remains open for the broader
trim/expiry/block/reorg overlap and memory-accounting sequences. The new test
does not replace fuzzing, does not run the package-admission fuzzer, and does
not cover every eviction policy or expiry clock path. Existing fuzzer and
suite controls remain relevant. No sanitizer, tool, or production source
suppression was added.

### Handoff

Next queue: build a failure-injection or snapshot oracle around package
rejection and replacement; then exercise trim, expiry, block removal, and
reorg reinsertion with independent membership, graph, and accounting
recomputation. Preserve the exact seeds, scratch paths, and unrelated PIDs
from the cycle gate. Do not claim repository completion.
