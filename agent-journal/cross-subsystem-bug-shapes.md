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

### Candidate 1: `dumptxoutset` temporary snapshot rename

Historical seed `0654511e1b` fixed settings-file publication after stream write/close failures. The first analogue was `dumptxoutset` in `src/rpc/blockchain.cpp:3181`, which calls `fs::rename(temppath, path)` without an explicit error-code check after creating a temporary UTXO snapshot. The apparent shape was: produce a durable artifact, publish it by rename, and return a success object.

The contract was checked against `std::filesystem`: the overload used by `fs::rename` throws `std::filesystem::filesystem_error` on failure. This is materially different from the settings path's boolean `RenameOver` contract. A current-source daemon was run on a scratch regtest datadir with 110 generated blocks. An `LD_PRELOAD` probe intercepted rename calls whose source ended in `.incomplete` and returned `EIO`. The command was:

```text
LD_PRELOAD=/data/my_storage/tmp/cross-subsystem-cycle76/librename_failure.so build_func_clang19/bin/bitcoind -regtest -datadir=/data/my_storage/tmp/cross-subsystem-cycle76/node -daemon -listen=0 -discover=0 -dnsseed=0 -rpcuser=cycle76 -rpcpassword=cycle76 -fallbackfee=0.0001
build_cli_clang19/bin/bitcoin-cli -regtest -datadir=/data/my_storage/tmp/cross-subsystem-cycle76/node -rpcuser=cycle76 -rpcpassword=cycle76 dumptxoutset rename-failure.dat latest
```

The RPC returned status 1 with `filesystem error: cannot rename: Input/output error`, and the final output path did not exist. The probe source is `agent-journal/rename_failure_preload.c`; the temporary node and shared object are outside the repository under `/data/my_storage/tmp/cross-subsystem-cycle76/`. This is dismissed: the exception is already converted to an RPC error, and adding a redundant explicit check would not improve the publication contract. The daemon was stopped and no process remains.

### Candidate 2: fee metadata on alternate package failure edges

Historical seed `e4ba0726dc` fixed a package precheck path that constructed a plain `MempoolAcceptResult::Failure` for `TX_RECONSIDERABLE`, dropping the effective feerate and fee-calculation wtxids required by the result contract. The current `MempoolAcceptResult` failure constructor now has an `Assume` rejecting that state, so every call site was checked rather than relying on the constructor alone.

The audit covered all `MempoolAcceptResult::Failure` call sites in `src/validation.cpp`. The remaining precheck and replacement paths branch to `FeeFailure` for `TX_RECONSIDERABLE`; max-feerate, ephemeral-spend, policy-script, and consensus-script paths assign non-reconsiderable states before constructing `Failure`. `SubmitPackage`'s consensus-script failure is explicitly an impossible postcondition after policy checks and is classified as package error. Package-level TRUC/RBF/cluster failures either return no per-transaction result or use a package policy state. The existing result-shape checks in `src/test/util/txmempool.cpp` and `src/test/txpackage_tests.cpp` enforce the same distinction.

Verdict: dismissed. No second reachable source path can produce a metadata-less `TX_RECONSIDERABLE` result on the current tree, so a duplicate fix or test would add no evidence. The analogous contract remains covered by the historical regression and constructor assumption.

### Candidate 3: compact-block announcement after a block read failure

Historical seed `6fbcd16491` changed the `GETBLOCKTXN` handler from `assert(ret)` to a logged peer disconnect after `ReadBlock` fails, because pruning is not the only failure mode: corruption, truncation, and ordinary I/O errors remain possible after releasing `cs_main`. The same conceptual operation exists in `PeerManagerImpl::SendMessages` at `src/net_processing.cpp:6346`: when a high-bandwidth peer has one queued header announcement and no cached compact block, the node reads the block and still executes `assert(ret)`.

This is a distinct caller path with the same block-storage trust boundary. It is reachable after a peer handshake and `SENDCMPCT(1, 2)`, `UpdatedBlockTip()` queues a single active-chain block for header relay, and the cached recent compact block does not match. A deterministic regression now uses the on-disk genesis block, queues its announcement, removes the corresponding block file, and calls `SendMessages`. The production fix checks `ReadBlock`, logs the failure, marks the peer for disconnect, and returns before compact-block construction instead of asserting.

The focused test passed on the restored fix. A deliberate mutation replacing the new guard with `assert(ret);` aborted with status 134 and the expected `Assertion 'ret' failed` diagnostic. The restored fix then passed the focused normal test and the full 32-case `net_tests` suite. A Clang 19 TSan build passed, followed by the focused regression and full 32-case `net_tests` suite under `TSAN_OPTIONS=halt_on_error=1:exitcode=99`, with no diagnostics.

Verdict: confirmed and fixed. The historical `GETBLOCKTXN` regression is an independent control for the same failure class; this caller had the same unhandled storage failure and now has a regression oracle.
