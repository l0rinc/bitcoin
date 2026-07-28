# Finding Deduplication and Recurrence

## Cycle 52

- Goal: `64`, `finding-dedup-recurrence`.
- Branch: `fuzz-contract-cluster-oracles-20260709`.
- Base: `origin/master` `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`.
- Gate HEAD: `b4bc6f881a745feb9debf8f860e8a5b9a458ad3f`.
- Selector: `shuf -i 0-98 -n 1` -> `64`.
- Scope: transaction-graph and orphanage deduplicated accounting under repeated announcements, duplicate parent references, removal, recurrence, and public projections.
- Closed cells excluded: wallet reservation ordering, taproot/x-only descriptor inference, LevelDB iterator status, and the prior historical finding cells.

The index key used in this cycle is:

`(source path, trust boundary, defect shape, source-to-sink relation, affected version, reproducer/hash, semantic summary)`.

An identical subject, branch, or commit message is not a distinct finding. A recurrence is distinct only when the same semantic shape is reachable through a changed path or after a later refactor; an incomplete variant must retain the original finding link instead of creating a duplicate entry.

## Semantic Index

| Fingerprint | Evidence | Verdict |
|---|---|---|
| `orphanage.wtxid-global-vs-announcement-accounting` | `src/node/txorphanage.cpp`: `ByWtxid` stores one entry per `(wtxid, peer)`, while `m_unique_orphan_usage`, `m_unique_orphans`, input links, and rounded-input scores update only for `brand_new`. `SanityCheck()` reconstructs both views. | Current implementation matches the contract. |
| `orphanage.public-view.wtxid-group-and-announcers` | `GetOrphanTransactions()` groups announcements by wtxid and exports the announcer set; the `txorphanage_sim` fuzzer compares that view against an independent model. Fix/oracle lineage includes `7db0ef2cb5`. | Closed; rechecked this cycle. |
| `orphanage.workset.unique-and-requeue` | `AddChildrenToWorkSet()` suppresses already reconsiderable wtxids and `GetTxToReconsider()` consumes the flag; unit cases cover idempotence and requeue after consumption. | Closed; rechecked this cycle. |
| `txdownload.unique-parent-before-request` | `GetUniqueParents()` sorts and uniques parent txids before request construction; the `aa961c8dec` history/test mutation demonstrated duplicate-parent sensitivity. | Closed for this path; no new caller variant found. |
| `txdownload.wtxid-peer-accounting` | Connection/disconnection and orphan-announcement paths assert per-peer wtxid state; `c748c04934` added the relevant peer accounting contract. | Closed; no recurrence found. |
| `txgraph.same-cluster-ref-union` | `GetConflicts()`, `GroupClusters()`, ancestor/descendant unions, and cluster lookup deduplicate references before exposing results. Current `txgraph_tests` and `txgraph` fuzz contracts cover the same-cluster duplicate path. | Closed for current callers. |
| `txgraph.saturated-fee-aggregation` | Historical production fixes `32ed8c5596` and `fb7bfd05ac` cover distinct overflow/saturation shapes; current tests include equal-feerate and saturated chunk controls. | Confirmed historical findings, not a new recurrence. |
| `history.ref-contract-repeat` | `txorphanage: check orphan fuzz contracts` has 16 visible commits but only two patch ids: `4cc5cf12c13115decf57332149c1ce305f1a60a1` (2 refs) and `88a86735c8d3b8e8979fc6df0957d3ef8bc21839` (14 refs). `txgraph: check trim return ref contracts` has 16 visible commits with one patch id, `96b7d45d75212543141b71f9f1b72e0ed844c7b0`. | Duplicate branch copies, counted once per patch identity. |

## Deterministic Evidence

### Focused unit contracts

Command:

`env TMPDIR=/data/my_storage/tmp/cycle52-dedup build_unit_clang19/bin/test_bitcoin --run_test=orphanage_tests,txdownload_tests,txgraph_tests --catch_system_error=no --log_level=test_suite`

Result: exit 0; orphanage, transaction-download, and transaction-graph suites completed; the log ends with `*** No errors detected`. The orphanage-only post-restore control completed all 12 cases with no errors. `git diff --check` passed after source restoration.

### Fuzz replays

- `txorphanage_sim`: 3,000 runs, `cov 4893`, `ft 27069`, corpus `987/230Kb`, average 53 executions/sec, peak RSS 67 MB, exit 0. This target independently models duplicate wtxids, duplicate txids with distinct witnesses, parent links, peer/global accounting, workset state, eviction, and block removal.
- `txorphan_protected`: 2,000 runs, `cov 3320`, `ft 17792`, corpus `491/2281Kb`, average 8 executions/sec, peak RSS 97 MB, exit 0. No sanitizer or assertion diagnostic.
- `txorphan`: the 686-file corpus contains inputs up to 1,044,557 bytes. The deterministic run was interrupted at a real resource boundary after 659 executed units, zero new units, one execution/sec average, slowest unit 16 seconds, and peak RSS 127 MB. No diagnostic was emitted. The raw log is `/data/my_storage/tmp/cycle52-dedup/txorphan.log`; the blocker is the CPU cost of the large corpus unit, not an observed failure.

### Oracle mutation

A temporary source mutation added a second `m_unique_orphan_usage += iter->GetMemUsage()` in the non-`brand_new` announcement branch. After rebuilding `test_bitcoin`, `orphanage_tests` aborted at `src/node/txorphanage.cpp:765`:

`Assertion calculated_dedup_usage == m_unique_orphan_usage failed.`

The mutation therefore falsified the cached global dedup invariant. The source was restored with `apply_patch`, the unit target was rebuilt, and the clean orphanage suite passed 12 cases. The mutation log is `/data/my_storage/tmp/cycle52-dedup/orphanage-mutation.log`; the restored log is `/data/my_storage/tmp/cycle52-dedup/orphanage-after-restore.log`.

## Verdict

No new source defect was confirmed. The current orphanage representation keeps per-announcement accounting separate from per-wtxid accounting, and its independent public-view/fuzz model plus `SanityCheck()` detect the tested overcounting mutation. Existing transaction-graph and parent/workset dedup contracts also survived the focused suites and available fuzz replays. No production commit is justified.

The cycle is a journal-only handoff. Do not count the repeated branch copies as new findings, and do not reopen the closed fingerprints above without a new caller, contract, or recurrence after a source change.

## Next Queue

1. `txrequest` txid/wtxid aliasing across request, response, timeout, and disconnect removal, excluding the already-closed orphan peer-accounting cell.
2. Duplicate transaction-id presentation across disconnected/reorged chainstate views, with separate public RPC and persistence fingerprints.
3. Package/mempool cluster union deduplication across replacement, eviction, and reorg reinsertion.
4. Database snapshot/iterator duplicate-key and deletion accounting under restart, excluding the closed iterator-status propagation finding.

Retain the raw logs under `/data/my_storage/tmp/cycle52-dedup/`. The next cycle must repeat the branch/base/dirty/process/catalog gate, select a fresh goal with `shuf -i 0-98 -n 1`, and search this semantic index before testing.
