# Finding Deduplication and Recurrence

## Cycle 116 start

- Selected goal: 64, `finding-dedup-recurrence`.
- Branch: `uber-cycle-116-finding-dedup-recurrence-20260729`.
- Base/HEAD at gate: `579b50ca2a929d421b3baac62522b42c54ccabcd`.
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Divergence (`origin/master...HEAD`): 40 commits behind, 1021 commits ahead.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Selector prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Goal TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Exact selector command/result: `shuf -i 0-98 -n 1` -> `64`.
- The fresh gate passed fetch, tracked/index checks, `git diff --check`, catalog hashes, and preservation of PID `777094`. Existing Cycle 52 orphanage, txgraph, parent/workset, and history patch-id fingerprints are excluded unless a changed caller or later recurrence supplies independent evidence.

Initial recurrence hypotheses:

1. `TxDownloadManagerImpl::MempoolRejectedTx` may process the same peer twice when txid and wtxid candidate lookups overlap; the contract may rely on `HaveTxFromPeer` and orphan insertion to make this idempotent, or may double-add parent requests/announcer state.
2. The txid/wtxid pair of `MempoolAcceptedTx`, `ReceivedTx`, `ReceivedNotFound`, and `ForgetTxHash` may leave an incomplete request fingerprint for witness and non-witness transactions, especially when one hash is already COMPLETED or when a response arrives from an unknown peer.
3. The long series of txrequest contract commits may contain branch copies or incomplete variants rather than distinct findings. Patch-id, source-path, trust-boundary, and semantic fingerprints must be counted separately from subject/message repetition.

The first verification pass will compare current source invariants and callers with the existing semantic index, run the focused txrequest/txdownload contracts, and replay the available txdownload/txrequest fuzz oracles. Any temporary mutation must be restored before the cycle close; no source change is justified without an independently observable recurrence or failing oracle.

## Cycle 116 completion

### Current semantic fingerprints

| Fingerprint | Evidence | Verdict |
|---|---|---|
| `txdownload.orphan-candidate.txid-wtxid-overlap` | `MempoolRejectedTx` seeds the candidate vector with the source peer, then appends `GetCandidatePeers` for txid and wtxid. The same peer can therefore occur twice. `MaybeAddOrphanResolutionCandidate` first rejects a peer already represented by `HaveTxFromPeer`; the first successful path adds the orphan and the second path is a no-op. `TxRequestTracker::ReceivedInv` independently deduplicates each `(peer, parent-txid)` pair. | Dismissed as a current defect; the duplicate vector is an intentional caller boundary with idempotent downstream state. |
| `txdownload.hash-pair.lifecycle` | `ReceivedTx` completes txid and, for witness transactions, wtxid requests; `ReceivedNotFound` completes each advertised hash; accepted/confirmed/rejected paths forget both hashes where the contract requires it; unknown-peer responses return before mutating request state. Existing assertions and focused tests check both hash views. | No recurrence found. |
| `history.txrequest.contract-copy` | Stable patch-id grouping of all visible `txrequest: check ...` commits produced six identities: disconnect cleanup `4cf815013ad15608299f00bf99e1a1f56897f034` (16 copies), expired-output overwrite `80abfe9badc6abf02cd7ebe2e481a0493f2e271e` (15), requestable fixed-point `b306b1cdb4c75380bbc0fa72650991ea7036114a` (16), candidate-peer append `c7ca9cd9ea3049d846a721813fabafc2e1a18857` (14), duplicate inventory `cfb40da80d802f5748811aeddebef4a9abad4609` (11), and null-expiry handling `ea135c71b8b39d14cfb07ebc6de76f5ba88d39ad` (14). Subject, branch, and commit-hash repetition is not counted as new evidence. | Six historical contracts, not 86 findings; all were searched before the current candidate. |
| `history.txdownload-contract-copy` | Stable patch-id grouping of the related txdownload/txdownloadman/fuzz contract history produced 14 identities, including orphan-parent dedup, accepted-request erasure, response cleanup, unknown-peer responses, disconnected-orphan cleanup, request issuance, and duplicate-peer state. | Historical copies are indexed once; no current incomplete variant was found in the selected call path. |

### Verification receipts

- The corrected build command `CCACHE_DIR=/data/my_storage/tmp/cycle116-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j4` completed successfully after the temporary test was added and again after it was removed. The initial test invocation omitted the pre-created `TMPDIR` and failed only in fixture setup; after creating `/data/my_storage/tmp/cycle116-tx-tests`, `txrequest_tests,txdownload_tests` passed all 19 cases.
- The temporary `txdownload_tests/overlapping_txid_wtxid_candidates_are_idempotent` case constructed a segwit child announced by one candidate peer under both txid and wtxid. It passed with exactly one unique orphan, two announcements (source plus candidate), one parent request per peer, no child request residue, and empty state after both disconnects. The test was removed and the restored 19-case suite passed again; `src/test/txdownload_tests.cpp` has no diff.
- The source-equivalent deterministic fuzz runner passed both selected target receipts:

  ```text
  FUZZ=txrequest /data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-fuzz-gcc/bin/fuzz src/txrequest.cpp src/txrequest.h src/test/fuzz/txrequest.cpp src/test/txrequest_tests.cpp
  txrequest: succeeded against 4 files in 1s.

  FUZZ=txdownloadman /data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-fuzz-gcc/bin/fuzz src/node/txdownloadman_impl.cpp src/node/txdownloadman_impl.h src/node/txdownloadman.h src/node/txorphanage.cpp src/node/txorphanage.h src/test/fuzz/txdownloadman.cpp src/test/txdownload_tests.cpp
  txdownloadman: succeeded against 7 files in 0s.
  ```

- `git log 579b50ca2a^..HEAD --` over the txrequest, txdownload manager, orphanage, and selected test/fuzz files returned no source history after the gate. `git diff --check` passed, and the temporary source/test diff was clean after restoration. No production or permanent test change is justified.

### Verdict and next queue

The overlapping txid/wtxid candidate path is a duplicate presentation, not a duplicate state: the orphanage's `(wtxid, peer)` unique index and `HaveTxFromPeer` guard prevent repeated announcements, while the request tracker's `(hash, peer)` uniqueness prevents repeated parent requests. The paired hash lifecycle also has no observed omission across response, NOTFOUND, accepted, confirmed, rejected, and disconnect paths. The cycle confirms the historical deduplication index is useful: 86 visible txrequest contract commits and the related copied txdownload contracts must be reported by semantic patch identity, not branch count. No source commit or retained regression test is warranted.

Next unchecked surfaces for this goal are package/mempool cluster-union recurrence after replacement or reorg reinsertion and database snapshot/iterator duplicate-key accounting under restart, excluding the Cycle 52 orphanage/txgraph cells. Do not reopen this txrequest fingerprint unless a new caller, backend, changed source path, or recurrence signal appears.

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
