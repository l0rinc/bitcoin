# Backport Correctness Audit

## Cycle 144: 30.x #35232 backport batch

### Draw, gate, and scope

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `66`
- Selected goal: `backport-correctness` (Cherry-pick, backport, and release-branch correctness audit)
- Branch: `uber-cycle-144-backport-correctness-20260730`
- Gate HEAD: `354cf3abf41df4c0c843895a17d6b853450e1b0f`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Explicit `git rev-list --left-right --count HEAD...origin/master`: `1072 40` (HEAD-only, origin-only)
- `origin/30.x`: `49faec4f87f5cd19c88db01a82e5c68b087c8227` (`v30.3`)
- The tracked/staged tree and `git diff --check` were clean at the gate. PID `777094` and unrelated untracked artifacts were preserved.

Cycles 36 and 66 covered the 31.x `#35331`/CVE-2024-52911 cells, and Cycle 67 covered the 29.x `#34370` Berkeley wallet cleanup backport. This cycle selected the distinct 30.x `#35232` merge `1fb642efb3ee6212f22302d742f6927981acfa76` and its second-parent batch. The current-master action-reference pinning from Goal 59 was excluded rather than rediscovered.

### Batch inventory and ancestry

The merge has release parent `1d97fda8a1475f0c9c10c5db0a06e1cad5484195` and backport parent `f3320b316e3d6737a4608189c7b92c12ef8a50b5`. Its second-parent chain contains:

```text
c7034f4854 depends: Unset SOURCE_DATE_EPOCH in gen_id
f92bd8b8ba doc: mention -DWITH_ZMQ=ON in BSD build guides
1dba05e7f6 wallet: use outpoint when estimating input size
c913cd9add Disable seek compaction
827f9343c4 ci: switch runners from cirrus to warpbuild
922626571e doc: remove reference to cirrus
f074c479a5 ci: use ubuntu-latest instead of ubuntu-24.04
f3320b316e doc: update release notes for v30.x
```

The risk-bearing implementation changes are the external-input wallet size calculation and the LevelDB read-seek compaction policy. The dependency cache identity, BSD build documentation, release CI runner/cache transition, and release notes were checked as integration surfaces.

The wallet backport patch-id is identical to the master fix `005738e3b846d1ba33f8d4a24f93c238fcf52f88` (`44de1bd989d4df5ca320a7ad87ab2b5e028daa8e`). The seek-compaction patch-id is identical to current master `78714f6d4fdc3b7831bba426bfff55bbe1021e6c` (`74a9d157b962c9b7edd1b5e4fe33b53dadb3913f`). Both patches change the same production/test semantics: pass `CTxIn{outpoint}` to `MaxInputWeight`, and return false from `Version::UpdateStats` while retaining manual/size compactions. The release range passed `git diff --check`.

### Release build and behavior

The detached release worktree `/data/my_storage/tmp/cycle144-release30` was checked out at `v30.3`. A clean GCC 12.2 CMake/Ninja build in `/data/my_storage/tmp/cycle144-release30-build` used wallet and tests enabled, GUI/IPC/ZMQ/USDT disabled, and completed all `489/489` actions while linking `bin/test_bitcoin`.

The focused release command was:

```text
TMPDIR=/data/my_storage/tmp/cycle144-test-tmp /data/my_storage/tmp/cycle144-release30-build/bin/test_bitcoin --run_test=spend_tests,dbwrapper_tests --random=144066 --log_level=test_suite --color_output=false
```

All 12 selected cases passed, including `spend_tests/max_signed_input_size_uses_external_outpoint`, `wallet_duplicated_preset_inputs_test`, `SubtractFee`, and the eight database-wrapper cases. The complete release command was:

```text
TMPDIR=/data/my_storage/tmp/cycle144-test-tmp /data/my_storage/tmp/cycle144-release30-build/bin/test_bitcoin --random=144067 --log_level=message --report_level=short --color_output=false
```

It passed `672/673` executable cases and all `19,788,170/19,788,170` assertions. The one warning was the expected `script_assets_tests/script_assets_test` skip because `DIR_UNIT_TEST_DATA` was unset; no product test failed.

The release `depends/gen_id` was run with the required `SHA256SUM=sha256sum`, GCC/binutils command variables, and `SOURCE_DATE_EPOCH` values `1` and `2000000000`. Both release runs returned the same digest `a6e2b19f185c77c7b74c6d812539cbb35053c1f875ba04f48821db20f5278f3f`; current master returned the same digest. The first attempt omitted `SHA256SUM` and produced `line 108: -: command not found`; that was a harness setup error and was discarded.

### Standalone LevelDB follow-up

The release tree's own LevelDB project was configured independently:

```text
cmake -S /data/my_storage/tmp/cycle144-release30/src/leveldb -B /data/my_storage/tmp/cycle144-leveldb-build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DLEVELDB_BUILD_TESTS=ON -DLEVELDB_BUILD_BENCHMARKS=OFF
cmake --build /data/my_storage/tmp/cycle144-leveldb-build --target autocompact_test db_test -j2
```

The build failed in `util/comparator.cc` through `util/no_destructor.h:24`:

```text
error: 'is_standard_layout_v' is not a member of 'std'
```

The release `src/leveldb/CMakeLists.txt` sets `CMAKE_CXX_STANDARD 11`, while its `NoDestructor` header uses the C++17 variable template `std::is_standard_layout_v`. This is a real release-branch standalone-LevelDB build defect, but it is not a defect introduced by `#35232`: `src/leveldb/CMakeLists.txt` and `src/leveldb/util/no_destructor.h` are byte-identical between the batch's release parent `1fb642efb3^1` and `origin/30.x`. The current-master LevelDB update `5fe0615f7ab868223ea3c0ba8d44d5b84cd01074` changes the standalone project to C++17. A separate current-master LevelDB build completed `47/47` actions; `autocompact_test` passed both `ReadAll` and `ReadHalf`, and `db_test` passed `GetDoesNotTriggerSeekCompaction`.

Report-ready release repro:

1. Check out `origin/30.x` at `49faec4f87`.
2. Run the two CMake commands above.
3. Observe the C++11 `std::is_standard_layout_v` compile error.
4. Apply the CMake C++17 correction from `5fe0615f7ab` or its minimal equivalent and rerun; the current-master control passes.

This adjacent release issue is recorded for a release-branch report, not committed on the current master-based branch because current master already contains the correction. The floating action references visible on `origin/30.x` were also excluded as the already-covered Goal 59 supply-chain cell.

### Verdict

**Dismissed as a defect in the 30.x #35232 backport batch; no current-tree source or test change is justified.** The release batch's wallet, LevelDB, dependency identity, and release unit behavior match the intended master contracts. A pre-existing standalone LevelDB CMake defect remains a report-ready release-branch finding outside the selected batch.

### Limitations and next queue

- No Windows, non-x86, sanitizer, or real power-loss execution was performed for `v30.3`.
- The detached release build did not run the full functional migration matrix; the complete release unit binary supplied the broad local control.
- The standalone LevelDB failure is release-only and pre-existing at the batch parent; it should not be represented as a failed cherry-pick of `#35232`.
- If Goal 66 repeats, choose another distinct 28.x/29.x/30.x batch or upgrade/downgrade behavior cell rather than reopening `#35232`.
- Draw the next catalog goal only after recording the fresh gate and preserving all unrelated artifacts.

## Cycle 67

### Draw and scope

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `66`
- Selected goal: `backport-correctness` (Cherry-pick, backport, and release-branch correctness audit)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Cycle gate HEAD: `f08d9749c2949a3ff6b91d87525d42486a483c36`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` was `2 906` at the gate.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The tracked/staged state was clean with only known untracked agent artifacts; no relevant process was running.

Cycle 36 audited the 31.x `#35331` batch, and cycle 66's earlier goal-66 audit covered the 31.x CVE-2024-52911 lifetime backport. This cycle excludes both cells and selects the distinct 29.x `#34370` wallet/Berkeley DB backport family, with the 28.x rebased form as an independent patch comparison.

### Initial hypothesis

The falsifiable hypothesis is that the 29.x `#34370` backport (`115172ceb8`, including `7475d134f6`) omitted a prerequisite or incorrectly adapted Berkeley wallet file ownership and cleanup rules. A later rebased commit (`29abedc97b`, `Rebased-From: 7475d134f6`) is present on `origin/28.x`; it will be compared for semantic drift rather than treated as proof by itself.

The trust boundary is a legacy Berkeley wallet directory containing a single wallet database, BDB environment files, backup/lock files, and unrelated user files or subdirectories. The relevant contract is that migration, backup, and cleanup may remove only files proven to belong exclusively to the wallet, while preserving unrelated data and handling multi-database environments conservatively.

### Status

Cycle 67 is complete. The candidate is dismissed as a current backport defect; no production source has been changed.

### Ancestry and semantic comparison

- `origin/29.x` at `3fc0865963` contains the original `7475d134f6` Berkeley wallet file-list change through merge `115172ceb8`; `origin/28.x` contains the later rebased `29abedc97b`, whose metadata records `Rebased-From: 7475d134f6`.
- The isolated source hunks in `src/wallet/bdb.cpp` and `src/wallet/bdb.h` are semantically identical. The differing blob indexes are the only difference between the source-side patch views, and both isolated patches have patch-id `212ea0ca53a6439bfae9d61a4f0e50fcd183e5ed`.
- The 29.x merge also contains the expected migration backup-location, cleanup, error-name, and functional-test follow-ups. No missing generated file, build-list entry, guard, or prerequisite was found.
- `BerkeleyDatabase::Files()` conservatively enumerates the single-wallet environment, its recognized files, and `database/log.*`; it falls back to the wallet path when the environment is shared or enumeration fails. The active BDB environment removes its temporary `database` directory during normal shutdown, so the exploratory log-prefix lead did not establish a release-specific data-loss contract.

### Release verification

The detached v29.4 worktree `/data/my_storage/tmp/backport-correctness-cycle67-29x` was built with Berkeley DB 4.8 enabled in `/data/my_storage/tmp/backport-correctness-cycle67-29x-build`. The `test_bitcoin` and `bitcoin-wallet` targets completed all `482/482` build actions; `bitcoind` and `bitcoin-cli` also rebuilt successfully. The focused release unit command selected wallet, wallet database/load, filesystem, and related cases: 26 cases were selected or reported in the run, including 17 wallet, 2 wallet-database, 2 wallet-load, and 5 filesystem cases, with `*** No errors detected`.

The legacy-wallet functional control passed with a fixed seed and exercised invalid dumps, unnamed and named wallets, multiple non-directory wallets, preservation of `db.log` and unrelated `test.dat`, cleanup, chainless conflicts, large records, and BDB parser comparison. Its first invocation failed before product startup because `bitcoin-cli` had not yet been built; after building that target, the corrected invocation exited 0 and reported `Tests successful`. `wallet_migration.py` exited 77 with the framework's documented skip because previous-release binaries were unavailable; this is an execution limitation, not product evidence.

### Verdict

**Dismissed as a current backport defect; no confirmed finding.** The 29.x implementation matches the independently rebased 28.x patch, preserves the intended single-wallet ownership boundary, and passes the available BDB-enabled unit and functional controls. No production or test repair commit is justified by this cycle.

### Limitations and rejected leads

- The full wallet migration matrix could not run because the test requires a v28 previous-release node that is not installed in the detached build environment.
- Windows, non-x86, sanitizer, and injected active multi-database crash schedules were not run for this release tree.
- The temporary `database/log.*` exploration did not prove a defect: the directory is an active BDB environment area and is removed by normal environment shutdown, while the functional test independently preserved unrelated root files and `db.log`.
- The initial missing-`bitcoin-cli` functional invocation was a harness setup error and was excluded from the verdict.

### Next queue

1. Recheck the repository gate and draw the next eligible catalog goal with the exact selector command.
2. If goal 66 repeats, choose a distinct 28.x/29.x/30.x backport, migration fixture, or release-branch behavior cell rather than reopening this patch.
3. Keep upgrade/downgrade, filesystem fault injection, and non-x86 release execution as separate evidence cells.

## Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `66`
- Selected goal: `backport-correctness` (Cherry-pick, backport, and release-branch correctness audit)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD: `1dcc2da988ee625fbc5d7d55eb6f894c1103ec52`
- Catalog: `agent-journal/reusable-continuous-agent-goals.md`

## Scope and Hypothesis

The cycle targeted the security-sensitive backport of the precomputed transaction-data lifetime fix. The falsifiable hypothesis was: the 31.x backport of CVE-2024-52911's `txsdata` lifetime correction omitted a prerequisite or resolved the surrounding `ConnectBlock` code incorrectly, leaving a release-branch use-after-free or a materially different validation contract.

The trust boundary is an invalid block reaching `Chainstate::ConnectBlock` with threaded script checks enabled. `CCheckQueueControl<CScriptCheck>` retains pointers into `txsdata`; local objects are destroyed in reverse construction order, so `txsdata` must be constructed before `control` and remain in scope until `control` completes.

## Repository and Release Evidence

- `origin/31.x` is at tag `v31.1`, commit `9be056a8a72b624dae9623b2f7bded92c2a21c91`.
- `v31.0` is the parent release commit `6574cb40869...`; `v31.0..v31.1` contains the backport merge `24bfb2c8be`.
- The backport commit is `0cedd6abf22866103ea852edb871d463f7ba1222`. Its message identifies PR `#35210`, says it backports `#35209`, records `Rebased-From: 1ed799fb21db51a12cbd5579420a61b9b5b3ee7d`, and includes the review ACK from `fanquake` in the merge metadata.
- The master fix is `1ed799fb21db51a12cbd5579420a61b9b5b3ee7d`; its merge is `aa1d0d7cd7`. Both fixes change only `src/validation.cpp`.

The pre-fix `v31.0` source has `control` at line 2511 and `txsdata` at line 2514. The release source after the backport has `txsdata` at line 2518 and `control` at line 2519. Current `origin/master` has the same order at lines 2526 and 2527. The isolated diffs for `0cedd6ab` and `1ed799fb` are identical: insert the vector declaration before `control` and remove its old declaration. Both isolated `git diff --check` runs were clean. No prerequisite, generated file, build-list update, guard, or test-file change is required by this hunk.

The historical source of the bug is also explicit in the backport message: the earlier `492e1f09943f...` logging refactor changed failure paths so the queue could outlive the vector, and the issue was reported as CVE-2024-52911. That history supports the lifetime contract; it does not indicate a defect in the 31.x backport.

## Verification

The disposable worktree `/data/my_storage/tmp/backport-31x-cve-control` was checked out at `v31.1`. A clean GCC 12 CMake/Ninja configuration was created at `/data/my_storage/tmp/build-backport-31x-cve-control` with tests enabled and GUI/CLI/utilities/kernel library/IPC/ZMQ/USDT disabled:

```text
cmake -S /data/my_storage/tmp/backport-31x-cve-control -B /data/my_storage/tmp/build-backport-31x-cve-control -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTS=ON -DBUILD_GUI=OFF -DBUILD_CLI=OFF -DBUILD_TX=OFF -DBUILD_UTILS=OFF -DBUILD_WALLET=OFF -DBUILD_KERNEL_LIB=OFF -DENABLE_IPC=OFF -DWITH_ZMQ=OFF -DWITH_USDT=OFF
cmake --build /data/my_storage/tmp/build-backport-31x-cve-control --target test_bitcoin -j2
```

The target completed all `496/496` build actions with status 0, including `src/CMakeFiles/bitcoin_node.dir/validation.cpp.o`. The release validation block suite then passed all three cases with a fixed Boost shuffle seed:

```text
/data/my_storage/tmp/build-backport-31x-cve-control/bin/test_bitcoin --run_test=validation_block_tests --random=123456 --log_level=test_suite
```

Result: exit status 0, `3` test cases, `*** No errors detected`. The disposable worktree, build directory, and all associated processes were removed after verification. No fuzz, sanitizer, daemon, or profiling process remains.

## Verdict

**Dismissed as a current backport defect; no confirmed finding.** The release patch exactly preserves the intended lifetime ordering, matches master, compiles in the actual `v31.1` tree, and passes the focused validation block tests. No production change or repair commit is justified by this cycle.

## Limitations and Rejected Leads

- The focused test suite does not reproduce the historical freed-memory read under ASan or a deliberately threaded invalid-block schedule; it verifies the release tree's normal validation behavior and buildability.
- This cycle checked the 31.x CVE backport only. The 28.x, 29.x, and 30.x backports, later backport batches, and upgrade/downgrade behavior remain separate hypotheses.
- No live GitHub review thread was fetched in this cycle; evidence is from local release refs, merge metadata, commit messages, source comparisons, and execution.
- Whole-range whitespace checks were deliberately not used as evidence because the release range contains unrelated historical whitespace. Only the isolated fix patch was checked.

## Next Queue

1. Draw another eligible catalog goal and record the exact command/draw before work.
2. Reopen this goal for the 28.x/29.x/30.x CVE backports or a high-risk backport batch such as `#35331`, comparing prerequisite ancestry and release-branch tests.
3. Compare one adjacent release pair for undocumented behavioral drift, including restart and upgrade/downgrade fixtures, without treating expected policy changes as defects.

## Cycle 36

### Draw and scope

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `66`
- Selected goal: `backport-correctness` (Cherry-pick, backport, and release-branch correctness audit)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Cycle gate HEAD: `1162eaecac8dbfc69eab31c7229c231eb1fbfdae`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` was `2 842` at the gate.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV validation: `validated_rows=100 total_lines=100`; the 99 goal rows were parsed with four literal-tab fields.
- The previous tracked/staged state was clean. Existing user and agent untracked artifacts, including `test/cache/`, were preserved.

The earlier goal-66 journal covered the 31.x CVE-2024-52911 lifetime fix. Its next queue named the 31.x `#35331` batch as a separate cell, so this cycle did not repeat the `txsdata` review. The falsifiable hypothesis was: the `#35331` release batch omitted a prerequisite/follow-up or introduced a conflict-resolution error in proxy routing, private broadcast, LevelDB compaction, settings persistence, MuSig validation, wallet sizing, or related tests and generated/build files.

### Ancestry and patch inventory

- `origin/31.x` is final v31.1 at `9be056a8a72b624dae9623b2f7bded92c2a21c91`.
- The batch merge is `efde623463cf194dda8407271f4dd136d054bc9f`, with release parent `dbaf0f4fd3e6e469727977f37ceb4066f2ef6656` and source-side parent `c058c29831a930f966180c38dfeab4635b8b86fc`.
- The merge metadata lists the intended backports: `#34953`, `#35228`, `#35279`, the LevelDB subtree portion of `#35313`, `#35316`, `#35378`, `#35348`, `#35384`, `#35408`, `#35410`, `#35430`, `#35447`, and `#35465`.
- The second-parent chain was enumerated. It contains the PSBT test, wallet outpoint sizing, MuSig empty-list validation, CI/build updates, LevelDB seek-compaction disable, GCC/SSE4 ASan handling, proxy override/private-broadcast fixes, settings write-error handling, and background chainstate compaction commits. The final release merge contains 46 changed files, 860 insertions, and 324 deletions; `git diff --check dbaf0f4... efde6234...` was clean.
- Rebased-From and patch-id comparisons classified the individual backports as exact matches where source context remained stable, or intentional context/subtree differences where current master had moved. The only unavailable historical object was the original LevelDB commit object `6bfdb6093b...`; the release subtree, merge metadata, current master subtree, and in-tree LevelDB tests were available and compared.

### Static contract audit

- The release `ccb991` API tightening removed the default `proxy_override` argument. Every `OpenNetworkConnection` caller found by `git grep` supplies an explicit value: a proxy, an item-held optional, `std::nullopt`, or the fuzzer's optional. There was no omitted caller in `src/net.cpp`, `src/rpc/net.cpp`, `src/test/fuzz/connman.cpp`, or the header declaration.
- The release `66377` private-broadcast guard rejects direct connections and the fuzzer now exercises both the private-broadcast optional proxy and random proxy paths. The current master implementation retains the same semantic guard with evolved logging and proxy types.
- `d61687` rejects an empty MuSig public-key list before parsing and the release `bip328_tests` coverage is present. The current master source retains the same guard.
- `ca00827` matches current master for settings write failures: it checks the stream after writing and after close, reports read errors, and updates the functional coverage. The focused settings functional test passed.
- The chainstate compaction backport keeps `m_db_mutex` around database replacement and the asynchronous full compaction, waits for outstanding work in `CCoinsViewDB` destruction, and waits explicitly in the layout unit test. `ResizeCache()` and background compaction use the same mutex; the production caller holds `cs_main` as required. The later master history has no corrective revert for the removed single-chainstate guard. Later commits `703a671fbc` and `0868c85fd5` add fuzz concurrency coverage and rename the asynchronous wrapper to `CompactFullAsync()`, respectively, supporting the intended concurrent lifecycle rather than identifying a release defect.
- The LevelDB backport disables automatic seek compaction by making `Version::UpdateStats` return false and updates the corresponding seek-compaction tests while preserving explicit full compaction. No whitespace or generated-file issue was found in the release-range patch.

### Verification

The detached release worktree `/data/my_storage/tmp/backport-correctness-cycle36` was checked out at v31.1. The clean GCC 12 RelWithDebInfo CMake/Ninja build at `/data/my_storage/tmp/build-backport-correctness-cycle36` completed `test_bitcoin` in `496/496` build actions and linked `bitcoind` in four actions. The focused unit commands passed:

```text
coins_tests,settings_tests,bip328_tests,private_broadcast_tests,spend_tests: 24 cases, 1,217,435 assertions
net_tests: 17 cases, 144,457 assertions
validation_flush_tests: 1 case, 57,996 assertions
```

The build-tree functional runs `p2p_private_broadcast_retry_v1.py --randomseed=12345` and `feature_settings.py --randomseed=12345` both exited 0 with `Tests successful`. The SOCKS handler emitted expected `ConnectionResetError` teardown traces after intentional peer closes; the test result and cleanup were successful. The first invocation from the source worktree used an unsupported `--srcdir` argument and stopped before starting a node; that was classified as a harness invocation error, then the build-tree script was run successfully.

The complete release unit binary then ran all 735 selected cases and exited 0:

```text
734 test cases out of 740 passed
1 test case out of 740 passed with warnings
5 test cases out of 740 skipped
26275230 assertions out of 26275230 passed
```

The warning was the documented unset `DIR_UNIT_TEST_DATA` skip in `script_assets_tests`; it did not indicate a product failure. A standalone release LevelDB `autocompact_test` build, using the release subtree and its built static libraries, ran `LEVELDB_TESTS=AutoCompact` and passed both `ReadAll` and `ReadHalf` with stable size output across 100 iterations each.

### Verdict and limitations

**Dismissed as a current backport defect; no confirmed finding.** The release batch preserves the intended API, state, compaction, and test contracts; all available release unit and selected functional controls passed, including the complete unit binary. No production or test repair commit is justified.

Remaining limitations are 28.x/29.x/30.x and other release batches, Windows and non-x86 execution, sanitizer coverage of the detached release tree, actual dm-flakey/ENOSPC power-loss schedules, and full upgrade/downgrade fixtures. The missing original LevelDB object prevents a direct patch-id comparison for that one historical commit, but the release subtree behavior and upstream tests were independently checked. These are queued as new cells rather than treated as evidence against the current batch.

### Next queue

1. Draw another catalog goal with the exact selector command and record the gate before work.
2. If goal 66 is drawn again, inspect a distinct 28.x/29.x/30.x backport or release batch; do not reopen `#35331` without new source or test evidence.
3. Preserve the release/upgrade differential and crash-consistency cells for a future draw.
