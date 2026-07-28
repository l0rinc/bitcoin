# Rejected-Finding Resurrection and Assumption Attack

## Cycle 33 Selection and Gate

- Selected index: `62`
- Selected slug: `rejected-finding-resurrection`
- Selected title: `Rejected-finding resurrection and assumption attack`
- Selector: `shuf -i 0-98 -n 1`
- Selection timestamp: `2026-07-28T06:10:05Z` (cycle-gate timestamp recorded immediately after selection)
- Catalog: `agent-goals/REUSABLE_AGENT_GOALS.md`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Gate HEAD: `8dbe7cd4cf035b0c9478e0bf099579c92bac52a6`
- Gate origin/master: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Gate merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Gate divergence: `origin/master...HEAD = 2 838`
- Gate state: tracked and staged files clean; agent/catalog artifacts and `test/cache/` remain untracked and were preserved; no relevant process was running.

## Selected Focus

Revisit dismissed findings and their assumptions, attack changed callers, configurations, versions, and input domains, and either confirm recurrence or preserve the dismissal rationale.

## Scope and Prior Dismissal

The first reopened cell was the cycle-22 `CoinStatsIndex::CustomAppend` finding in `agent-journal/error-path-state.md`. `CustomAppend()` increments `m_total_subsidy` before returning false when its in-memory current hash does not match the previous-block hash supplied by `interfaces::BlockInfo`. The earlier dismissal treated that mismatch as an internal invariant violation: `BaseIndex` is supposed to rewind or reject non-contiguous transitions before dispatching the append, and `BaseIndex::ProcessBlock()` treats a false append as a fatal index error.

The scope was intentionally limited to changed caller and lifecycle assumptions. The current branch contains the 2025 recovery-code removal (`54dc34ec227`, `index: Remove unused coinstatsindex recovery code`), current index synchronization/restart changes, and the DB-only UTXO cursor migration. No unrelated wallet, P2P, or public-input cell was reopened.

## Hypothesis

The old dismissal would be invalid if a current production caller could deliver a non-contiguous `BlockInfo` to `CoinStatsIndex::CustomAppend()` without first rewinding the index, or if the failed append's in-memory subsidy mutation could be observed or committed during the resulting shutdown.

Trust boundary: validated chainstate notifications and index restart state are internal node state. The mismatch is not directly attacker-controlled; a caller would first need to violate the block-index/index-tip relationship or corrupt the internal index state.

## Caller and History Audit

- `kernel::MakeBlockInfo()` in `src/kernel/chain.cpp` derives `BlockInfo::prev_hash` from `CBlockIndex::pprev`, not from an independently supplied block header. It also derives the height and block hash from the same block index.
- `BaseIndex::ProcessBlock()` is the only production caller of the virtual `CustomAppend()` interface (`git grep -n CustomAppend` found no other caller). It constructs the `BlockInfo`, supplies block and undo data, and calls `FatalErrorf()` when the append returns false.
- `BaseIndex::Sync()` walks `NextSyncBlock()` from the current index tip. If a next block does not directly follow the current tip, it calls `Rewind()` to the common ancestor before `ProcessBlock()`.
- `BaseIndex::BlockConnected()` rejects a notification that does not connect to an ancestor of the current best index. For a valid reorg it calls `Rewind(best_block_index, pindex->pprev)` before dispatching the append. `CoinStatsIndex::RevertBlock()` updates `m_current_block_hash` to the reorg ancestor after restoring its state.
- `BaseIndex::Init()` loads the locator and the corresponding CoinStats DB entry together. `CoinStatsIndex::CustomInit()` validates the persisted MuHash and restores `m_current_block_hash` from the selected locator entry before a non-genesis append.
- The 2025 recovery removal explicitly says the hash-key fallback was unnecessary because BaseIndex rewinds before `CustomAppend()` and could not recover correctly from that path. Current history contains no later change that adds another append caller or permits out-of-order index construction.
- The current branch's restart and pre-genesis fixes change pointer publication and empty-chain handling, but do not bypass the append sequencing checks.

## Failure-State Audit

`m_total_subsidy` is a live member used while constructing the next `DBVal`; it is not read by `LookUpStats()`, which reads the persisted height/hash entry. The mismatch return occurs before any CoinStats height entry write, before `m_current_block_hash` is updated, and before the `DBVal` is constructed. `CustomCommit()` writes only `DB_MUHASH`; `BaseIndex::Sync()` returns immediately after the failed `ProcessBlock()`, and `BaseIndex::Stop()` unregisters the index and joins the sync thread without committing a failed append.

`BaseIndex::FatalErrorf()` calls `node::AbortNode()`, which sets the exit status and requests shutdown. The shutdown path flushes already queued validation callbacks before unregistering and destroying indexes, so a forced mismatch could leave additional in-memory failed calls. Each such call still returns before a DB write or best-block update, and shutdown destroys the index after the queue drains. This is an internal transient mutation during an already fatal shutdown, not a persisted or caller-visible index result.

## Verification

Static checks:

- `git grep -n CustomAppend -- ':!agent-*' ':!test/cache/*'` showed only the BaseIndex virtual, `BaseIndex::ProcessBlock()`, and the four concrete overrides; no alternate production dispatch exists.
- `git grep -n ProcessBlock\( src` showed the index dispatch confined to `BaseIndex::Sync()` and `BaseIndex::BlockConnected()`; both paths were inspected above.
- `git grep -n 'm_total_subsidy\|CustomCommit(' src/index/coinstatsindex.cpp src/index/base.cpp` confirmed the mutation, DBVal construction, reload, and commit boundaries.
- `git blame` and `git log -L` confirmed the fatal false-return behavior predates the 2025 recovery removal, while the recovery removal's commit message documents the same BaseIndex sequencing invariant.

Runtime controls on current HEAD:

- `build_unit_clang19/bin/test_bitcoin --run_test=coinstatsindex_tests,baseindex_tests --catch_system_error=no --log_level=test_suite`: 3 cases passed, including normal initial sync, unclean shutdown/reload, and no-commit-ahead-of-flush behavior.
- `build_unit_clang19/bin/test_bitcoin --run_test=txindex_tests/txindex_block_until_synced_before_genesis_activation,blockfilter_index_tests,txospenderindex_tests --catch_system_error=no --log_level=test_suite`: 1 selected case passed. The other requested suites/cases were disabled by the test filter; no error was reported.
- The cycle-22 isolated normal and ASan/UBSan fuzz replays remain valid supporting controls: identical 128 qa-assets seeds completed 129 runs each with exit code 0 and no crash artifact. They did not reach the internal mismatch branch, so they are not treated as direct proof of that branch's behavior.

## Verdict

**Dismissed again as a production defect; preserve as an internal-contract hardening lead.** The current callers still enforce contiguous transitions or terminate before a mismatched append can be used. The mutation is real in source, but it happens before an internal fatal return, is not persisted or exposed through the public lookup path, and the index is unregistered/destroyed during shutdown. No source change or regression test is justified without a new caller, a nonfatal error contract, a shutdown-lifecycle change, or a reproducible state where the mismatch is reached through supported node behavior.

## Handoff

No source change was made. Raw source/history evidence is in the commands recorded above; current unit artifacts are under the existing `build_unit_clang19` tree. The next cycle must run a fresh gate, draw a distinct eligible catalog goal, and should not reopen this CoinStats cell unless a new append caller, restart/persistence path, or nonfatal shutdown behavior appears.
