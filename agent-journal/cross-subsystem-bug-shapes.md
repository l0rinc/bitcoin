# Cross-Subsystem Bug Shapes Cycle 62

## Identity and Gate

- Cycle: `62`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `26`
- Goal: `bug fixed in one subsystem but present in another`
- Slug: `cross-subsystem-bug-shapes`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `387f5a122f1c85655253cd35b47d49140317e8a9`
- `origin/master...HEAD` at the gate: `2 894`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- `goals.tsv` validation: `validated_rows=99 total_lines=100 status=ok`
- No relevant test, fuzz, sanitizer, daemon, or profiling process was running at the gate.

Cycle 48 already closed the wallet-rescan reservation-ordering cell. This cycle must select a distinct historical bug shape and avoid rediscovering that lock/scheduling path.

## Scope and Hypotheses

Mine recent and historical fixes for a concrete structural pattern such as write-before-validation, unchecked durable return, stale accounting, inconsistent boundary validation, partial output mutation, missing cleanup, or cross-layer lifecycle asymmetry. Search analogous parsers, caches, indexes, queues, state machines, and public APIs using semantic structure, not only names.

1. A historical fix may have corrected one implementation family while an analogous caller remains reachable and unprotected.
2. A superficially similar site may be intentionally different because its state, trust boundary, or failure contract differs.
3. A candidate may be a test/documentation gap rather than a production defect.

For every candidate, record the seed fix, structural features, callers, trust boundary, history, tests, and expected contract. Require an independent minimal reproducer or proof of unreachability before changing code. Keep one finding per self-sufficient commit and preserve negative controls.

## Evidence Log

- Historical seed: `b0ce659a98` (`validation: readd redownloaded block candidates`) fixes a shared-object mutation bug in `ChainstateManager::ReceivedBlockTransactions`. `nSequenceId` is part of `CBlockIndexWorkComparator`, while each chainstate owns a separate ordered candidate set. The fix removes a block from every chainstate before changing the shared sequence id and re-adds it through each chainstate's candidate policy.
- Analogous site reviewed: `Chainstate::PreciousBlock` removes and reinserts its target only in the active chainstate while changing the same shared `nSequenceId`. The source shape is real, but it is not enough to justify a fix without a reachable second candidate set.
- Candidate-set contract: `TryAddBlockIndexCandidate` adds a block to a historical chainstate only when it is on that chainstate's `TargetBlock` path. In `SetupSnapshot`, the background chainstate targets the snapshot base at height 110, while the active snapshot tip is at height 210. `PreciousBlock` returns before mutating sequence ids when the requested block has less chain work than the active tip. Therefore a block eligible for active `PreciousBlock` cannot simultaneously be a valid background candidate in this snapshot configuration.
- Controlled regression attempt: two same-work children of the active snapshot tip were added with `ReceivedBlockTransactions`. The active candidate set contained them, but the background set correctly contained neither (`count == 0` before `PreciousBlock`). Making the test pass required manually injecting invalid background membership, which would test a broken invariant rather than production reachability. The initial pointer-based attempt also produced misleading counts because the test retained pre-lookup pointers; it was discarded rather than treated as evidence.
- Negative controls: `bc3db5ef52` restart publication, `997c034412` cursor/cache locking, `6aa5d8d948` compact-block transaction accounting, `c3d9446762` checkqueue completion, `7e19ce200b`/`5311b15727` descriptor PSBT verification, and the cycle-48 wallet reservation ordering were reviewed as distinct seed families. Their analogous current paths were either already covered by the originating fix or had different contracts and no independently reachable omission.
- Verification commands: `git grep -n 'setBlockIndexCandidates' -- src/validation.cpp src/test`; `TMPDIR=/data/my_storage/tmp cmake --build build_unit_clang19 --target test_bitcoin -j4`; and the disposable filtered `validation_chainstatemanager_tests/precious_block_preserves_background_candidate_order` run. The latter was removed after the reachability proof; the worktree returned to tracked/staged cleanliness, and `git diff --check` passed. No production file was changed.

## Verdict

- Dismissed: the historical structural resemblance is real, but the suspected `PreciousBlock` cross-chainstate corruption is unreachable under the current chainstate candidate-set contract. No source or regression commit is justified. Preserve the seed and the failed-but-invalid test model so a future change to snapshot target semantics reopens this cell with a valid reproducer.

## Handoff

- Cycle closed without a source commit. Next run must recheck the gate, draw a fresh full-catalog selector, and choose a distinct unchecked defect shape. Reopen this cell only if a caller can make the same block a valid candidate in both chainstates without violating `TargetBlock` or active-chain work rules.

## Cycle 76: cross-subsystem failure-publication and lifecycle asymmetry

### Selection and gate

- Initial selector: `shuf -i 0-98 -n 1` -> `37` (`build-dead-zones`); rejected because cycle 74 closed the same configuration cell with no changed code, tool, or assumption.
- Retry selector: `shuf -i 0-98 -n 1` -> `26` (`cross-subsystem-bug-shapes`).
- Branch: `uber-cycle-76-cross-subsystem-bug-shapes-20260728`.
- Gate HEAD: `02e0a92ddc33af203f4204848eb6095312f052af`.
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Divergence: `origin/master...HEAD` was `2 930`.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Tracked source was clean; known untracked agent artifacts and `test/cache/` were preserved. No relevant test, fuzz, sanitizer, daemon, or profiling process was running.

### Excluded cells and active hypothesis

Cycle 48's wallet-rescan reservation ordering and cycle 62's chainstate candidate-set mutation are closed and excluded. This cycle mines a different historical defect shape: a failure or lifecycle event may publish state in one subsystem while an analogous caller, cache, index, queue, or wrapper publishes the same conceptual state before validation, after a dropped return, or without symmetric rollback/cleanup.

Start with recent source fixes and review rationale, extract structural features independent of symbol names, then search analogous wallet, persistence, P2P, descriptor, and index paths. A candidate is only reportable when its trust boundary, intended contract, reachable caller, and failure schedule are independently established. Status: active; no source finding claimed yet.
