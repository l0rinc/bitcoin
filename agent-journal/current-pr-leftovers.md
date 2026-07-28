# Current Branch and PR Leftover Sweep

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
