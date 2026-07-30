# Rejected-Finding Resurrection and Assumption Attack

## Cycle 157 Selection, Reopened Cell, and Gate

- Exact selector: `shuf -i 0-98 -n 1` -> `62` (`rejected-finding-resurrection`). The dedicated branch is `uber-cycle-157-rejected-finding-resurrection-20260730`; start HEAD was `9f67b9fc0a65ba8599d153cd772db053420a2935`, `origin/master` was `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base was `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and start divergence was `42 1097` (`origin/master...HEAD`).
- The fresh gate fetched `origin` successfully, preserved the catalog/protocol hashes, and found no tracked or staged changes. It also found the unrelated long-running test processes PID `777094` (`wallet_tests`) and PID `956381` (`util_tests`); neither was touched. Existing agent artifacts, `node_modules/`, `package*.json`, and `test/cache/` remained untracked and were preserved.
- This cycle excludes the earlier CoinStatsIndex mismatch dismissal and the Qt stale-state fix. It reopens the Cycle 137 `-seednode=` dismissal from `agent-journal/history-incomplete-fixes.md`, which established only that an empty seed node produces one failed address-fetch attempt and is not retried indefinitely.

## Distinct Hypothesis: Empty Seed Node Suppresses Fallback

The old dismissal missed a separate startup contract. `AppInitMain()` copied raw `-seednode` values into `connOptions.vSeedNodes`, while both network threads independently tested raw `gArgs.GetArgs("-seednode").empty()`. On an empty addrman with `-dnsseed=0 -fixedseeds=1`, an empty or whitespace-only value therefore did three things: it created an invalid empty `ADDR_FETCH` target, marked seed nodes as active for the fixed-seed policy, and delayed fixed-seed insertion until the one-minute timeout. With DNS enabled it also made the DNS thread wait through its 30-second seed-node phase even though no usable seed existed.

Trust boundary: local command-line/configuration input controls startup peer discovery. The input is not a consensus value, but a malformed local setting can delay bootstrap, consume an address-fetch slot, and suppress the fallback intended for an empty peers database.

## Independent Discovery and Verification

Discovery track:

- `src/init.cpp` at the pre-fix gate assigned `connOptions.vSeedNodes = args.GetArgs("-seednode")` without validation. `src/net.cpp::ThreadOpenConnections()` computed `use_seednodes` from raw `gArgs` rather than its already-selected seed span. `CConnman::ThreadDNSAddressSeed()` used the same raw argument check. The open-connection path used the seed span for `add_addr_fetch`, so the raw flag and usable vector could disagree.
- The pre-fix binary at commit `9f67b9fc0a65ba8599d153cd772db053420a2935` was run in an isolated signet datadir with `-listen=0 -discover=0 -dnsseed=0 -fixedseeds=1 -maxconnections=1 -proxy=127.0.0.1:1 -seednode=`. At `2026-07-30T15:03:04Z` it logged `Empty addrman, adding seednode () to addrfetch`, then `trying v2 connection (addr-fetch) to , lastseen=0.0hrs`, followed by the controlled local proxy refusal. The no-seed control on the same binary logged immediate fixed-seed insertion.
- The source history confirms the adjacent empty `-connect` fix (`6f91b89157`) filters invalid values before the retry loop, but no corresponding `-seednode` validation existed. The current option description requires a hostname/address and permits repetition; an empty value has no valid interpretation.

Verification track:

- The fix filters empty and whitespace-only `-seednode` values in `AppInitMain()`, logs `Ignoring empty -seednode value`, and passes only the filtered vector to the network lifecycle. `ThreadOpenConnections()` now derives `use_seednodes` from its filtered span. `ThreadDNSAddressSeed()` receives the same boolean captured at thread start, so valid seed-node behavior remains unchanged and empty input cannot create a DNS delay.
- A direct post-fix regtest run with `-dnsseed=0 -fixedseeds=1 -seednode= -seednode=' ' -proxy=127.0.0.1:1` logged two warnings and, at `2026-07-30T15:08:36Z`, `Adding fixed seeds as -dnsseed=0 ... neither -addnode nor -seednode are provided`; it emitted no empty address-fetch message. Regtest has no fixed seed addresses, so the subsequent `Added 0 fixed seeds` is expected; the gating log is the relevant assertion.
- `CCACHE_DIR=/data/my_storage/tmp/cycle157-ccache cmake --build /data/my_storage/tmp/cycle89-build --target bitcoind -j2` passed. `BITCOIND=/data/my_storage/tmp/cycle89-build/bin/bitcoind test/functional/p2p_seednode.py --configfile=/data/my_storage/tmp/cycle89-build/test/config.ini --cachedir=/data/my_storage/tmp/cycle157-p2p-seednode-cache-2 --tmpdir=/data/my_storage/tmp/cycle157-p2p-seednode-2 --portseed=157` passed all four cases, including empty and whitespace-only values. The adjacent `p2p_dns_seeds.py` run with an isolated tmpdir and port seed 158 passed all cases. `git diff --check` passed.

## Verdict and Fix

**Confirmed and fixed as a distinct recurrence/variant.** The prior finding was correctly limited to the absence of indefinite retries, but its dismissal did not cover the changed caller configuration where an invalid value suppresses fixed-seed fallback and starts an empty fetch. The fix is limited to input filtering and making both network-thread decisions use the filtered seed-node state. Valid repeated seed nodes still enter the existing fetch and DNS-wait paths. No consensus, wallet, key, or remote-network impact was established.

Source/test changes are ready for one self-contained commit. The exact old and new startup traces, proxy isolation, and functional-suite results above are the handoff for review; future work should test other malformed multi-value startup options against their fallback gates rather than reopening this cell without new evidence.

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

## Cycle 37 Selection and Gate

- Selected index: `62`
- Selected slug: `rejected-finding-resurrection`
- Selected title: `Rejected-finding resurrection and assumption attack`
- Selector: `shuf -i 0-98 -n 1`
- Cycle gate HEAD: `ebe09a67153fe09ba67b03b0cc49f01fa84e2381`
- Gate origin/master: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Gate merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Gate divergence: `origin/master...HEAD = 2 843`
- Gate state: tracked and staged files clean; agent/catalog artifacts and `test/cache/` remain untracked and were preserved; no relevant process was running.

## Cycle 37 Distinct Cell

The cycle-33 CoinStats `CustomAppend()` mismatch cell was excluded. The distinct reopened cell was the deferred `GetNodeStateStats()` failure contract from `error-path-state.md`, attacked through the Qt peer-detail caller and its changed refresh path.

### Hypothesis

When node-state statistics become unavailable after a previous successful refresh, `RPCConsole::updateDetailWidget()` may leave the old state-only values visible instead of representing the unavailable state. The trust boundary is a local node-state/GUI synchronization boundary: `TRY_LOCK(::cs_main, lockMain)` can legitimately fail, and peer selection/data refresh can invoke the UI update while the same peer remains selected.

### Evidence and Reproduction

- `NodeImpl::getNodesStats()` initializes each tuple with `fNodeStateStatsAvailable=false` and default `CNodeStateStats`, then assigns the real state only when `cs_main` is acquired. A busy `cs_main` therefore produces a valid node row whose state statistics are unavailable.
- `PeerTableModel::refresh()` emits `dataChanged` after rebuilding the rows. `RPCConsole` connects that signal directly to `updateDetailWidget()`, so an unavailable refresh updates the selected peer detail pane without requiring a selection change.
- Before this cycle, `updateDetailWidget()` assigned `timeoffset`, services, sync/common height, ping wait, address relay/accounting, and relay-transaction fields only inside `if (stats->fNodeStateStatsAvailable)`. The false path performed no writes. Qt labels retain their previous text, so a true-to-false refresh left the prior peer-state values visible while the adjacent general fields had already been refreshed.
- `git show 1b0db7b984 -- src/qt/rpcconsole.cpp` and line history show that the peer-detail implementation has had explicit unavailable-state handling in its history, while the current nine-field false path has no equivalent clearing behavior. The current comment only explains why the fetch can fail; it does not establish that stale values are an acceptable display contract.
- The minimal deterministic state transition is: select a peer, run one successful refresh to populate all nine fields, make `cs_main` unavailable for the next `getNodesStats()` call, emit `dataChanged`, and call `updateDetailWidget()` for the unchanged selection. The old code retains all nine old strings; the new `else` branch assigns `ts.na` to each state-only field.

### Fix

`src/qt/rpcconsole.cpp` now explicitly assigns `ts.na` to all nine node-state-only labels when `fNodeStateStatsAvailable` is false: time offset, services, sync height, common height, ping wait, address relay enabled, addresses processed, addresses rate limited, and relay transactions. This is the smallest caller-side fix and preserves the intentional `GetNodeStateStats()` false result.

### Verification

- Configured a clean Qt 6 build at `/data/my_storage/tmp/build-gui-cycle37` with `BUILD_GUI=ON`, `BUILD_GUI_TESTS=ON`, `BUILD_TESTS=ON`, wallet/IPC/ZMQ/USDT disabled, and Debug type; configuration succeeded.
- `cmake --build /data/my_storage/tmp/build-gui-cycle37 --target bitcoin-qt test_bitcoin-qt -j2` completed all 453 Ninja actions and linked both targets.
- `QT_QPA_PLATFORM=minimal /data/my_storage/tmp/build-gui-cycle37/bin/test_bitcoin-qt --log_level=message --report_level=short` passed AppTests (3), OptionTests (6), URITests (3), and RPCNestedTests (3); all tests passed with no failures, skips, or blacklisted cases.
- `build_unit_clang19/bin/test_bitcoin --run_test=net_tests/get_node_state_stats_overwrites_reused_output --log_level=message --report_level=short` passed 1 case and 4 assertions, preserving the adjacent API reset contract.
- `git diff --check` passed.

There is no existing Qt test that constructs a selected peer and forces the `TRY_LOCK` false path, so this cycle does not claim a direct widget-level regression test. The state-flow proof is deterministic from the production caller chain, and the current Qt suite provides build/application regression coverage. A future GUI-test expansion can exercise the exact lock-busy transition if the test harness gains a controllable peer-state fixture.

## Cycle 37 Verdict and Handoff

**Confirmed and fixed.** The API-level false result remains intentional; the resurrected defect was stale user-visible state in the Qt caller. The source fix and this cycle's journal/state handoff are committed together. The next cycle must run a fresh gate, exclude this fixed Qt state-refresh cell, draw another goal from the validated catalog, and continue the risk-map loop.
