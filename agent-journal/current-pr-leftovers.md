# Current Branch and PR Leftover Sweep

## Cycle 323: Taproot fee-estimation test workaround

### Selection and scope

- Exact selector: `shuf -i 0-122 -n 1` returned `3`.
- Branch: `uber-cycle-323-current-pr-leftovers-20260802`.
- Selection commit: `3a96d968e0`; cycle-start HEAD:
  `3a96d968e0`; cycle-start catalog SHA-256:
  `da5a2650e39932fe39d952c139a8f547fe319819c4f663d71a055f4f6298a153`.
- The prior dedicated current-PR checkpoint was Cycle 293 (`d7705e2474`).
  The new scope was the source/test stack after that checkpoint, including
  wallet/database recovery, fuzz input preservation, filesystem allocation,
  PSBT and descriptor parsing, minisketch bounds, generated secp256k1
  vectors, kernel C ABI validation, build configuration, p2p serialization,
  release differential tests, generated-source checks, CI artifact checks,
  secret ctime coverage, and the Cycle 322 wallet cache fix.
- No merge commits or online PR metadata occur in this local post-checkpoint
  range. Prior findings and unrelated untracked files were excluded; each
  source change was checked for analogous callers, tests, generated/build
  lists, stale comments, and cleanup paths.

### Candidate inventory and dismissed leftovers

- The secp256k1 vector-comment sanitizer covers every current generator that
  emits external comments into C block comments. `test_vectors_musig2_generate.py`
  uses comments only to choose fixed error enums and emits no raw comment text;
  the new helper is included in `Makefile.am` distribution. No generated-file
  omission was found.
- The paired Unix fallback and Windows `AllocateFileRange` changes both have
  preservation tests, and their remaining platform branches use separate
  file-size semantics. No unmodified analogous allocation path or build-list
  omission was established.
- The kernel import-path validation checks both arrays, every element, and
  length-aware path conversion; its C ABI test covers null element and missing
  length arrays. The zero-count nullable-array case is valid by contract and
  did not justify a source change.
- The remaining recent commits have focused regression tests or intentional
  configuration-only behavior. A passing current suite alone did not turn
  execution-only coverage or speculative documentation edits into findings.

### Confirmed leftover: stale Taproot fee-rate workaround

`2ac99aac74` corrected `TRDescriptor::MaxSatisfactionWeight()` and
`MaxSatisfactionElems()` for known script paths and removed the two comments in
`test/functional/wallet_taproot.py` that said `fee_rate=200` compensated for
the old inability to estimate script-path fees. Blame shows both explicit
`fee_rate=200` values were introduced with those comments by
`6efcdf6b7f6`; they remained after the comments were removed. The functional
test therefore retained the behavior workaround while appearing to assert the
post-fix fee path, a partial test migration.

The independent descriptor regression already gives the exact contract oracle:
for `tr(NUMS,pk(NUMS))`, the old implementation returned a 66-byte
key-path-only satisfaction and one witness element, while the corrected code
returns 135 bytes and three elements. A focused functional test should use the
normal estimator rather than force an unrelated 200 sat/vB rate. The smallest
fix removes the explicit fee-rate option from both `sendtoaddress` and
`walletcreatefundedpsbt`; no production behavior changes.

Before the edit, Python compilation passed. After removing both stale
overrides, the rebuilt current wallet-enabled `bitcoind` was exercised by:

```text
python3 test/functional/wallet_taproot.py \
  --configfile=/data/my_storage/tmp/cycle246-wallet/test/config.ini \
  --cachedir=/data/my_storage/tmp/cycle323-wallet-taproot-cache-0405 \
  --tmpdir=/data/my_storage/tmp/cycle323-wallet-taproot-run-0406 \
  --portseed=323 --randomseed=32301 --timeout-factor=2 --loglevel=INFO
```

The run covered all address, `sendtoaddress`, and PSBT cases, including NUMS
internal keys that force script-path satisfaction, and exited 0 with
`Tests successful`. The cached and temporary datadirs were isolated under
`/data/my_storage/tmp`; no protected process was stopped.

### Verdict and handoff

**Confirmed and fixed as a test leftover:** the two explicit fee-rate
overrides were stale remnants of the defect repaired by `2ac99aac74` and
prevented the functional test from exercising the default estimator. The
test-only source edit and this journal are one self-contained finding. The
next current-PR pass should begin after the state-close and catalog gate, and
should not reopen the dismissed generator, allocation, or kernel-array cells
without a changed contract or new evidence.

## Cycle 293 Selection and Gate

- The fresh post-Cycle-292 gate selected `3` (`current-pr-leftovers`) with the exact selector command `shuf -i 0-98 -n 1`; the selector output was `3`. The dedicated branch is `uber-cycle-293-current-pr-leftovers-20260802`. Start HEAD was `b219fa6352c776ba73a90b4c4d8f76bc78503576`, `origin/master` was `556988790a7f961693a8fd93f73725baea66476a`, merge-base was `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and start divergence was `origin/master...HEAD = 45 1376`.
- The entry tracked/index state SHA256 was `11a06b3ba7f5a55efd4cbaadcda36062c4d5a1530415bcf9bc9fd9b4908836dc`. The catalog, prompt, TSV, and protocol hashes matched the recorded values. Protected processes `777094`, `956381`, `1138182`, `1157959`, `1312049`, `1312050`, and `1346200` remained alive; unrelated untracked agent artifacts were preserved.
- The previous dedicated current-PR cycle was Cycle 151. This cycle therefore inventoried and searched the newer source stack rather than reopening the closed argument, flush, private-broadcast, or HKDF cells. The recent stack includes wallet/database recovery, parser validation, index lifecycle, mempool accounting, IPC cleanup, sanitizer/fuzz harness, build/CI, and secp256k1 state changes. No generated-file, build-list, help-text, or stale-symbol omission was found in the reviewed commits.

## Cycle 293 Candidate: Corrupt Wallet Best-Block State

- Hypothesis: after `c66c73cf74` introduced `BestBlockReadResult`, the remaining boolean `WalletBatch::ReadBestBlock` callers in `CWallet::AttachChain` might treat a corrupt `BESTBLOCK` record as absent and bypass cross-chain wallet-reuse protection.
- The relevant callers at `src/wallet/wallet.cpp:3302` and `:3326` use the boolean wrapper. The migration path already uses `ReadBestBlockResult` and rejects `ERROR`. The direct test harness corrupted a regtest wallet's SQLite `BESTBLOCK` value to `b"\\x01"`, then attempted to load it on a signet node. The exact result was `Wallet file verification failed ... Data is not in recognized format.` The database verifier rejected the malformed record before `AttachChain`, so this shape did not reach the suspected bypass. The existing valid-record cross-chain functional test remains the applicable contract.
- Verdict: **dismissed** for this cycle. A missing-record/ancient-wallet compatibility case is intentionally accepted and was not changed without independent evidence that it should be rejected. No wallet source change is justified.

## Cycle 293 Candidate: TxoSpenderIndex Regenerates a Key for Persisted Entries

- Hypothesis and trust boundary: `f1f5758cd6` changed `TxoSpenderIndex::CustomInit` so a missing SipHash key is rejected when a best-block marker exists or the key itself exists. However, `BaseIndex::Init` passes `nullopt` when `ReadBestBlock()` returns an empty locator. If a crash or partial metadata loss leaves a nonempty index database with spender entries but no `B` marker and no `siphash_key`, the old guard accepts the database and generates a new random key, orphaning all persisted entries. The trust boundary is the on-disk index metadata and LevelDB entry set during index restart/recovery.
- Source trace: `src/index/base.cpp:132-158` reads the locator and calls `CustomInit(nullopt)` when it is absent. `CDBWrapper::IsEmpty()` at `src/dbwrapper.cpp:387` checks whether any LevelDB entry exists. `src/index/txospenderindex.cpp:72-84` previously checked only `block || m_db->Exists("siphash_key")`, despite the comment saying that regeneration is valid only for a new index. Actual spender keys use the `s` prefix and `CDiskTxPos` serialization.
- Independent before-fix reproduction: the new test creates a fresh on-disk `txospenderindex` database, writes a correctly serialized `s`-prefixed `PersistedSpenderKey`, leaves both metadata records absent, and calls `Init()` with the normal `TestChain100Setup` chain. Before the production edit, the focused command
  `/data/my_storage/tmp/cycle273-clang19-ubsan/bin/test_bitcoin --run_test=txospenderindex_tests/txospenderindex_rejects_missing_siphash_key_with_persisted_entries --log_level=message --color_output=false -- -testdatadir=/data/my_storage/tmp/cycle293-testdata`
  failed at `txospenderindex_tests.cpp:266` because `!index.Init()` was false. The first attempt without `-testdatadir` was invalid evidence: the root filesystem was full and the fixture aborted while creating its chain. The rerun placed all test data on `/data/my_storage` and reproduced the incorrect successful initialization.
- Fix: `CustomInit` now rejects when `block || !m_db->IsEmpty()`. Thus a missing key is accepted only for a genuinely empty new index; a nonempty database, including malformed metadata or leftover spender entries, fails closed instead of silently changing the lookup key. This is a two-line production change with no API or serialization migration.
- Regression: `src/test/txospenderindex_tests.cpp` adds `txospenderindex_rejects_missing_siphash_key_with_persisted_entries`, using a real `s`-prefixed disk-position key rather than an arbitrary database record. The test is self-contained and uses a scratch datadir. The modified target built successfully with `env CCACHE_DIR=/data/my_storage/tmp/cycle293-ccache cmake --build /data/my_storage/tmp/cycle273-clang19-ubsan --target test_bitcoin -j2`.
- After-fix validation: `/data/my_storage/tmp/cycle273-clang19-ubsan/bin/test_bitcoin --run_test=txospenderindex_tests --log_level=message --color_output=false -- -testdatadir=/data/my_storage/tmp/cycle293-testdata-all` selected all 5 `txospenderindex_tests` cases and exited 0 with `*** No errors detected`. This includes the new missing-key case, `txospenderindex_rejects_corrupt_siphash_key`, initial synchronization, multibyte transaction offsets, and the reader/reinitialization race. `git diff --check` also passed.
- Verdict: **confirmed and fixed**. The source, regression test, and this journal will be committed together as one self-sufficient finding by `Lőrinc <pap.lorinc@gmail.com>`. No generated or build-list files changed. The remaining current-stack queue is the newer post-Cycle-151 commit inventory; do not reopen the dismissed wallet shape without a distinct state that passes wallet database verification.

## Cycle 151 Completion

- The fresh post-Cycle-150 gate selected `84` (`secp-nonce-session`) with the exact command `shuf -i 0-98 -n 1`; that goal's nonce/session evidence was already closed in Cycle 95, so the required reroll selected `3` (`current-pr-leftovers`). The dedicated branch is `uber-cycle-151-current-pr-leftovers-20260730`; start HEAD was `f818945455c2f8ea768258546c63d8aadf12c729`, `origin/master` was `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base was `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and start divergence was `1086 42`. The gate preserved the unrelated untracked artifacts and PID `777094`.
- The fresh scope was the post-Cycle-141 stack: `d6e42bca9c` (stop after block-file flush failure), `d0d092b296` (cancel stale private-broadcast work), and `26dc79e84d` (cleanse the HKDF context secret). The earlier `6f91b89157` empty-`-connect` migration was rechecked only for analogous leftover sites because its dedicated functional regression already exists; upstream PR #35838 and its Darwin GUI test were already mined in Cycle 146 and had no new evidence.
- For `d6e42bca9c`, the failure path is complete: `FlushChainstateBlockFile()` reports the I/O failure, `FlushStateToDisk()` returns `state.Error()` before `WriteBlockIndexDB()`, chainstate flush/sync, `m_last_flushed_block`, or `ChainStateFlushed`. The Linux virtual-filesystem regression in `chainstate_write_tests.cpp` exercises the real failure and asserts no callback. The absent-cursor assumeutxo path returns success by contract, so it does not bypass the guard.
- For `d0d092b296`, all removal paths now account for `RemoveResult::NumUnstarted()`: stale invalidation, RPC abort, receipt from the network, and private-broadcast disconnect cleanup. The possible snapshot/abort interleaving can add one aggregate retry slot after a transaction is removed, but the implementation explicitly documents this as an allowed vain connection; `ThreadPrivateBroadcast()` consumes the slot on success or retains it for retry on failure, and every cancellation path uses saturating subtraction. No exact-counter invariant or user-visible incorrect result was proven, so no race fix or timing-sensitive test was justified.
- For `26dc79e84d`, the only production caller constructs the HKDF context in BIP324, and the new destructor owns the complete `m_prk` cleanup while copy/move are deleted. The placement-storage regression checks the full object after destruction; source search found no remaining whole-object wipe or analogous secret copy. The focused `crypto_tests,bip324_tests,chainstate_write_tests,net_tests` command selected 65 cases and exited 0 with `*** No errors detected`; the expected flush-failure diagnostic was emitted by the Linux test. The separate `private_broadcast_tests` command selected 10 cases and also exited 0 with no errors.
- Whole-tree searches found no omitted generated/build/help/docs/test references or unresolved applicable review objection. The current-pr source, test, and journal changes were therefore classified as complete. Verdict: **dismissed/no source finding**. No production edit is justified. The next cycle must perform a fresh gate, preserve PID `777094` and unrelated untracked files, and draw exactly `shuf -i 0-98 -n 1`.

## Cycle 141 Completion

- The fresh post-Cycle-140 gate selected `53` with `shuf -i 0-98 -n 1`; Goal 53 (`statistical-timing`) was already closed across the existing ECDH, Schnorr, MuSig, EllSwift, GCC, Clang, and backend cells, with no new tool/compiler/architecture/caller evidence available. The required reroll selected `3` (`current-pr-leftovers`).
- Branch: `uber-cycle-141-current-pr-leftovers-20260730`. Start HEAD: `80b290c227785139a3ae0970164f43763b15b411`; `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1067 40`. The gate preserved the unrelated untracked artifacts, `test/cache/`, and PID `777094`.
- The distinct current-stack hypothesis came from `f1008dcd27` (`rpc: deduplicate descriptor scan objects`): the production helper skips exact duplicate descriptor objects in `scantxoutset`, `scanblocks`, and `getdescriptoractivity`, while its permanent regression was added only to `rpc_scantxoutset.py`. `getdescriptoractivity` already had an equivalent duplicate-address assertion. `rpc_scanblocks.py` had no duplicate-input assertion, so a temporary focused assertion was evaluated as a possible leftover.
- The current `rpc_scanblocks.py` suite passed against `/data/my_storage/tmp/cycle89-build` using `/data/my_storage/tmp/cycle89-build/test/cache` and scratch directory `/data/my_storage/tmp/cycle141-scanblocks-test-20260730-b`. The candidate assertion was then rejected: removing the `scanblocks` dedup guard would still produce the same result because `GCSFilter::ElementSet` deduplicates derived scripts. A result-only test therefore does not kill the production regression; timing thresholds or instrumentation would be flaky or unnecessarily invasive. The prior Cycle 130 resource-exhaustion journal already contains the independent 1-to-200 duplicate-object CPU scaling proof and fixed control.
- Recent source commits were also checked for analogous omitted sites, stale names, generated/build omissions, and missing tests. The current stack has matching behavioral tests for the recent network, wallet, persistence, parser, and optimization changes; header-only include fixes and vector refreshes have no applicable runtime test omission. No new self-sufficient leftover with hard proof was established.
- The temporary test edit was removed. `git diff --check` and the tracked/index dirty-state gate are clean. Verdict: **dismissed/no fix** for this cycle. Next queue: continue with a fresh goal draw; do not reopen exact duplicate descriptor expansion unless a new caller or a non-flaky performance oracle is found.

## Cycle 30 Selection

- Selected index: `3`
- Selected slug: `current-pr-leftovers`
- Selected title: `Current branch and PR leftover sweep`
- Selector: `shuf -i 0-98 -n 1`
- Selection timestamp: `2026-07-28T05:33:37Z`
- Catalog: `agent-goals/REUSABLE_AGENT_GOALS.md`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Gate HEAD: `6ea89f6c40dd94b2e7c1524cd5573430f7e71072`
- Gate merge-base with `origin/master`: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Gate divergence: `origin/master...HEAD = 2 832`
- Gate state: tracked and staged files clean; unrelated untracked catalog/journal artifacts and `test/cache/` preserved; no relevant build, test, daemon, fuzz, sanitizer, or profiling process running.

## Selected Prompt

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/current-pr-leftovers.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Search each current commit for analogous sites, stale names, partial migrations, omitted tests, generated files, build lists, and unresolved review objections.
```

## Scope Ledger

- Primary source stack: `f91039eb5c` (`node: reject overflowing validation cache arguments`), `93fef76238` (`mempool: reject overflowing cluster size arguments`), and `6ea89f6c40` (cycle-29 state handoff).
- Exclusions from the selected scope: do not reopen the already fixed validation-cache or cluster-size defects as new findings; do not treat cycle-29 journal metadata or unrelated untracked user files as leftovers.
- Candidate classes: analogous argument parsers and downstream arithmetic, stale option names/help/docs, test and fuzz coverage omissions, generated/build/install lists, partial conversions in the same source stack, and review objections attached to the touched paths.
- Required evidence: commit-by-commit file/symbol inventory, whole-tree analogous-site search, history/review comparison, clean-HEAD reproduction for any candidate, and narrow/broad validation for each proven fix.

## Hypotheses

1. The cycle-29 option validation pattern may have analogous unchecked arithmetic in nearby node or mempool argument parsers.
2. The two source fixes may omit tests, help/docs, generated/build manifests, or optional-module references that should have changed with them.
3. Recent branch commits may contain partial migrations or unresolved review objections that are still applicable to the current stack.

## Cycle Status

## Cycle 30 Discovery

### Candidate: overflowing `-prune` byte conversion

- Hypothesis: `ApplyArgsManOptions()` converts a non-negative `-prune` MiB value to bytes with an unchecked unsigned multiplication, so values at or above `2^44` MiB wrap to zero instead of being rejected.
- Trust boundary: local command-line/config input reaches `BlockManager::Options::prune_target`, then `BlockManager` derives `m_prune_mode` from whether the target is nonzero and `chainstate.cpp` reports automatic pruning only for a nonzero non-manual target.
- Contract: the documented `-prune=<n>` value is a MiB target; `1` selects manual pruning and values at least 550 select automatic pruning. An unrepresentable target must not silently become the no-pruning value.
- Source/history: `src/node/blockmanager_args.cpp` still used `uint64_t(nPruneArg) * 1_MiB`; the original `Move ::nPruneTarget into BlockManager` commit `fa721f1cab` introduced the same unchecked multiplication, and the current `-prune` functional test covered negative and below-minimum values but no conversion overflow. The cycle-29 validation-cache fix established the same parser-to-byte conversion pattern and provided the checked-arithmetic precedent.
- Before-fix command: `build_func_clang19/bin/bitcoind -regtest -datadir=/data/my_storage/tmp/current-pr-leftovers-prune-before-cycle30.ZWCHOn -prune=17592186044416 -server=0 -listen=0 -connect=0 -dnsseed=0 -fixedseeds=0 -daemon=1`.
- Before-fix output: the daemon returned `Bitcoin Core starting`, reached `init message: Done loading`, and logged `Setting NODE_NETWORK in non-prune mode`; no initialization error was emitted. Since `2^44 * 2^20 == 0 mod 2^64`, this is the exact wrap-to-zero control, not merely a large allocation attempt. A `-prune=550` control in `/data/my_storage/tmp/current-pr-leftovers-prune-valid-cycle30.f9SZEk` logged `Prune configured to target 550 MiB on disk for block and undo files.` and reached `Done loading`.
- Severity/reachability: local configuration/direct API correctness issue, not a remotely reachable or consensus defect. The wrapped value silently changes the documented pruning mode and can leave a user running unpruned while believing a target was configured.
- Independent verifier plan: add a focused `AppInitParameterInteraction` test, rebuild `bitcoind` and `test_bitcoin`, rerun the same huge command expecting a status-1 explicit error, rerun `-prune=550`, and run the complete node-init and pruning-related unit/functional gates available without the 4 GB functional workload.

### Candidate review: other argument conversions

- `src/node/caches.cpp` already uses `SaturatingLeftShift` for `-dbcache`; no analogous unchecked shift remains there.
- `src/node/mempool_args.cpp` now uses `CheckedMul` for `-maxmempool` and cycle-30 inherited the checked cluster-size path; existing tests cover those contracts.
- `src/init.cpp` validates `-maxsendbuffer` and `-maxreceivebuffer` before their later conversion; cycle-10 tests cover both paths.
- `src/node/coins_view_args.cpp` assigns debug-only `-dbbatchsize` directly to an unsigned field and accepts signed `-dbcrashratio` values. These are separate debug-option domain questions, not omissions from the cycle-29 source stack; no production-facing arithmetic failure was proven in this cycle and they remain queued as lower-priority hypotheses.
- No stale `maxsigcachesize`/`limitclustersize` names, generated-file references, or build-list omissions were found. `src/init.cpp`, RPC help, existing functional tests, and focused unit targets all reference the current names. The cycle-29 commits are self-contained and do not require generated artifacts.

### Cycle 30 verification status

## Cycle 30 Verification

- Source edit: `src/node/blockmanager_args.cpp` now uses `CheckedMul<uint64_t>` for the non-negative MiB-to-byte conversion and returns `-prune is too large (got ... MiB)` on overflow. The manual `-prune=1` sentinel and ordinary minimum check remain unchanged.
- Regression edit: `src/test/node_init_tests.cpp` adds `init_rejects_overflowing_prune_argument`. The first run caught an unrelated `-listen=0`/`-listenonion=1` fixture failure; setting `-listenonion=0` made the oracle specific. The focused test then passed 1 assertion and printed the expected prune error; the complete `node_init_tests` suite passed 3 cases and 5 assertions.
- Repaired huge control: `build_func_clang19/bin/bitcoind -regtest -datadir=/data/my_storage/tmp/current-pr-leftovers-prune-after-cycle30.B8LZk1 -prune=17592186044416 -server=0 -listen=0 -listenonion=0 -connect=0 -dnsseed=0 -fixedseeds=0 -daemon=0` exited status 1 with `Error: -prune is too large (got 17592186044416 MiB)` and left no daemon running.
- Repaired normal control: `-prune=550` in `/data/my_storage/tmp/current-pr-leftovers-prune-valid-after-cycle30.Ps6Bsd` reached `init message: Done loading` and logged `Prune configured to target 550 MiB on disk for block and undo files.`
- Boundary control: `-prune=17592186044415` (`2^44-1` MiB) reached `Done loading` and logged the exact target in `/data/my_storage/tmp/current-pr-leftovers-prune-boundary-cycle30.AkQSL4`; `-prune=17592186044416` is the first overflowing value. The boundary daemon was terminated and the process gate is clean.
- Broader validation: `blockmanager_tests` passed 11 cases/117 assertions; `validation_flush_tests` passed 3 cases/58,001 assertions; `mempool_tests` passed 24 cases/423 assertions. `cmake --build build_unit_clang19 --target test_bitcoin -j2` and `cmake --build build_func_clang19 --target bitcoind -j2` both passed. `git diff --check` passed.
- Commit discipline: commit `5b2e4f5a63` (`node: reject overflowing prune target arguments`) as one self-sufficient change authored by `Lőrinc <pap.lorinc@gmail.com>`, containing the source fix, focused regression, and this journal. No generated files or build-list changes were required. The user-owned untracked `test/cache/` and catalog artifacts remain untouched.

## Cycle 30 Verdict and Next Queue

The `-prune` leftover is **confirmed and fixed**. The cycle-29 argument-validation pattern had not been propagated to the block-manager parser, and the clean daemon control demonstrated a user-visible mode inversion. The remaining debug-only `-dbbatchsize` unsigned assignment and `-dbcrashratio` signed domain are separate lower-priority option-contract hypotheses; they were not treated as confirmed without an independent failure contract and are queued for a future goal draw. No stale names, generated-file omissions, build-list omissions, or unresolved applicable review objections remain in the cycle-29 stack.

## Cycle 32 Selection and Gate

- The first selector draw was `48` (`property-oracle-expansion`), but that goal's transaction/block/filter property cell was already closed in cycle 28. It was recorded as an ineligible same-cell repeat rather than reopened without new evidence.
- Rerunning the exact selector `shuf -i 0-98 -n 1` produced `3`, `current-pr-leftovers`, for the distinct debug-option cells explicitly queued by cycle 30.
- Gate HEAD: `b601d20988ba73564a51e079f95b61c8f033ed44`; merge-base with `origin/master`: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD = 2 835`; tracked and staged state clean; no relevant process running. Catalog, prompt, protocol, and TSV validation hashes are recorded in `uber-goal-state.md`.
- Scope excludes the already-fixed `-prune`, `-maxsigcachesize`, and `-limitclustersize` findings. The first unchecked hypothesis is the signed domain of `-dbcrashratio`; `-dbbatchsize` remains a separate follow-up.

## Cycle 32 Candidate 1: Negative `-dbcrashratio` Reaches `randrange`

- Hypothesis: `ReadCoinsViewArgs()` copies any parsed signed integer into `CoinsViewOptions::simulate_crash_ratio` without checking its sign or the destination `int` range. A negative value reaches `FastRandomContext::randrange`, whose contract requires `range > 0`, allowing a debug build to abort during a chainstate write/replay.
- Trust boundary: local command-line/config input enters `src/node/coins_view_args.cpp`, is stored in the chainstate options, and is later used by `CCoinsViewDB::BatchWrite()` after a partial LevelDB batch.
- Contract: `-dbcrashratio=0` disables simulation; a positive value means a `1/ratio` exit probability. Negative values and values outside the `int` representation are not valid ratios and must be rejected before chainstate initialization.
- Before-fix setup: a scratch regtest daemon was started with `-dbcrashratio=-1 -dbbatchsize=1`, mined one block to create a replayable partial flush, then was restarted in the same scratch datadir with `-debug=coindb`. The restart reached `[coindb] Writing partial batch of 0.00 MiB` and exited status 134 with `random.h:257 ... Assertion `range > 0' failed.` The raw debug log is `/data/my_storage/tmp/current-pr-leftovers-dbcrash-cycle32.pMxtsM/regtest/debug.log`; no daemon remained.
- Independent mechanism proof: `FastRandomContext::randrange(int)` calls `Assume(range > 0)` before deriving `maxval`; `ReadCoinsViewArgs()` previously accepted `-1` and the chainstate path called `rng.randrange(m_options.simulate_crash_ratio)` when the nonzero ratio was used. This is a direct invalid-domain path, not a timing-only or theoretical concern.
- Fix: `ReadCoinsViewArgs()` now returns `util::Result<void>`, rejects negative ratios, rejects values above `std::numeric_limits<int>::max()`, and casts only after validation. `ApplyArgsManOptions()` propagates the result before chainstate construction.
- Regression: `validation_chainstatemanager_tests/chainstatemanager_args` now checks `0`, `24`, `-1`, and `2147483648`; the rebuilt focused test passed 1 case and 46 assertions. The rebuilt `bitcoind` rejects the original scratch command before opening chainstate with `Error: -dbcrashratio must be non-negative (got -1)`, and rejects the upper-domain case with `Error: -dbcrashratio is too large (got 2147483648)`.
- Verdict: **confirmed and fixed**. The smallest self-contained change is committed separately as `node: validate chainstate debug arguments` by `Lőrinc <pap.lorinc@gmail.com>`. The negative `-dbbatchsize` narrowing remains an independent candidate and is not covered by this commit.

## Cycle 32 Candidate 2: Negative `-dbbatchsize` Disables Partial Batching

- Hypothesis: `ReadCoinsViewArgs()` assigns a signed `GetIntArg("-dbbatchsize")` directly to unsigned `CoinsViewOptions::batch_write_bytes`. `-1` therefore becomes `UINT64_MAX`, so a value outside the option's non-negative byte domain silently disables partial database batches.
- Trust boundary and contract: this is a debug-only local option, but its help text calls it the maximum database write batch size in bytes. A negative input must not become an enormous valid-looking limit or alter crash-recovery/write-amplification behavior.
- Before-fix paired controls: two fresh regtest daemons used `-dbcrashratio=0`, `-debug=coindb`, and identical one-block workloads. `/data/my_storage/tmp/current-pr-leftovers-dbbatch-negative-cycle32.R1LzXL` used `-dbbatchsize=-1` and logged only `Writing final batch ...` before `Committed 1 changed transaction outputs`; `/data/my_storage/tmp/current-pr-leftovers-dbbatch-positive-cycle32.UhTPqC` used `-dbbatchsize=1` and logged `Writing partial batch ...` followed by `Writing final batch ...`. Both completed and shut down cleanly, with no process left running. The difference is the direct observable effect of the unsigned narrowing, not an allocation failure.
- Independent type proof: `CoinsViewOptions::batch_write_bytes` is `uint64_t`, `GetIntArg()` returns `int64_t`, and the old assignment converted `-1` modulo 2^64. `CCoinsViewDB::BatchWrite()` compares the batch size with that maximum, so the partial-batch branch cannot fire for ordinary data.
- Fix: validate `-dbbatchsize` as non-negative before casting to `uint64_t`, while retaining zero as a valid debug value for flushing at every positive-sized batch. The existing result-returning `ReadCoinsViewArgs()` path reports the error before chainstate construction.
- Regression: `chainstatemanager_args` now checks `0`, `1`, and `-1`; after rebuilding the focused test passed 1 case and 51 assertions. The invalid daemon command returns `Error: -dbbatchsize must be non-negative (got -1 bytes)` before opening the scratch chainstate.
- Verdict: **confirmed and fixed**. This is a separate self-contained commit from the crash-ratio fix, authored by `Lőrinc <pap.lorinc@gmail.com>`. No generated files or build-list changes are needed.

## Cycle 32 Broad Verification

- `cmake --build build_unit_clang19 --target test_bitcoin -j2` and `cmake --build build_func_clang19 --target bitcoind -j2` passed after the two fixes.
- `validation_chainstatemanager_tests/chainstatemanager_args` passed 1 case and 51 assertions after both fixes; the full `validation_chainstatemanager_tests` suite passed 22 cases and 2079 assertions.
- The combined `validation_chainstatemanager_tests,validation_flush_tests,mempool_tests` run passed 49 cases and 60,503 assertions.
- The existing `feature_dbcrash.py` functional attempt was made with an isolated tmpdir and fixed port seed, but failed before selected database write paths with `StopIteration` in `MiniWallet.get_utxo()` while preparing its first 1000-output transfer after coinbase-maturity generation. The test framework cleaned all four dangling nodes; no product assertion or `-dbbatchsize`/`-dbcrashratio` path was reached. This is recorded as a harness/setup blocker, not as a regression from these changes.
- `git diff --check` passed and no test, daemon, build, or fuzz process remains running. The functional artifacts remain under `/data/my_storage/tmp/current-pr-leftovers-feature-dbcrash-cycle32` for handoff.
