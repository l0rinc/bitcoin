# Backport Correctness Audit

## Cycle 165: 28.x #35214 backport batch

### Draw, gate, and scope

- Draw sequence: exact `shuf -i 0-98 -n 1` returned `33`, `82`, `98`, `49`, then `66`; goals 33, 82, 98, and 49 were excluded because their journals record closed cells. Goal 66 was retained because its queue explicitly leaves distinct 28.x/29.x/30.x release batches and upgrade/backport cells.
- Selected goal: `backport-correctness` (Cherry-pick, backport, and release-branch correctness audit).
- Branch: `uber-cycle-165-backport-correctness-20260730`.
- Gate HEAD: `ac5dd3ed2a448417504149650b901457c24e8690`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `1111 42` (HEAD-only, origin-only).
- Selected release ref: `origin/28.x` at `de328509029d36b0541ddf25700ac19a0de6a5c8` (`v28.4` plus `#35213`). The batch under audit is merge `2023de53f073717b28c7d6c87790d221b59c81ab` (`#35214`), whose second-parent commits are `2d3edd9640` (CI seccomp), `2c5242d24f` (Boost multi-index compatibility), and release-note commit `c1c2184f45`.
- Prior exclusions: Cycles 36 and 66 covered 31.x `#35331`/CVE-2024-52911, Cycle 67 covered 29.x `#34370`, and Cycle 144 covered 30.x `#35232`; Cycle 165 uses the distinct 28.x batch and its 28.x-specific source/configuration context.
- The tracked tree and `git diff --check` were clean at the gate. The known untracked agent artifacts, `node_modules/`, `package*.json`, `test/cache/`, and unrelated PIDs `777094` and `956381` were preserved and are outside this cycle's scope.

### Initial hypotheses and evidence plan

1. The Boost 1.91 `multi_index` backport may omit a 28.x container or alter an index/type contract while fixing compilation. Compare the release patch with its source-side fix, compile the release tree against the available system Boost and a newer Boost where possible, and run the mempool/transaction-request/miner paths plus a behavior-sensitive differential probe.
2. The seccomp backport may apply the `--security-opt seccomp=unconfined` capability to the wrong release jobs or fail to pass it through `02_run_container.py`. Trace the exact CI job selection and Docker argument construction, compare the release commit with all source-side variants, and validate the generated command without launching an untrusted or unavailable CI container.
3. The batch may have an omitted prerequisite, wrong ancestry, or release-note/configuration mismatch. Compare each changed path with the source PR and current master, use patch-id/semantic correspondence where meaningful, and inspect release-branch tests and build manifests.

No conclusion is drawn from the commit message alone. A source change requires a deterministic release-branch reproducer, an independent verifier or mutation/reference result, and narrow plus broad validation. A CI-only mismatch may be recorded without a production fix when the exact external runner/container behavior is unavailable.

### Release source and ancestry audit

The 28.x tree contains exactly three Boost.MultiIndex containers: the modified-transaction set in `src/node/miner.h`, the mempool transaction set in `src/txmempool.h`, and the announcement index in `src/txrequest.cpp`. The older 28.x `src/txorphanage.cpp` uses `std::map` and is not an omitted MultiIndex site. Current master has since moved and redesigned the orphanage as `src/node/txorphanage.cpp`, which explains why the source-side fix `0bc9d354df` touches that file while its 28.x rebased form `2c5242d24f` touches the release-specific miner container instead. The release patch preserves all index tags, extractors, ordering, iterator aliases, and element types; it only inlines `indexed_by<...>` as the second template argument to `multi_index_container`.

The release commit's `Rebased-From: 0bc9d354df` metadata, the source-side history, and the release-tree inventory agree. No changed production site or prerequisite was missing. `origin/28.x:depends/packages/boost.mk` pins Boost 1.81.0, while the host has Boost 1.74.0; the system build therefore checks backward compatibility, and the Boost 1.91-specific compilation claim remains an explicit unavailable environment cell rather than an inferred pass.

### 28.x build and behavioral verification

The initial CMake configure was rejected because this release uses Autotools and has no top-level `CMakeLists.txt`; it did not modify the release source. After `./autogen.sh`, the isolated configure command was:

```text
/data/my_storage/tmp/cycle165-backport-28/configure --without-gui --disable-zmq --disable-wallet --disable-bench --enable-tests
```

It completed successfully with GCC 12.2.0, Boost 1.74.0, tests enabled, and the vendored libsecp256k1 tests enabled. The first `make -j2` stopped before compilation because `/root/.cache/ccache/tmp` did not exist; rerunning with `CCACHE_DIR=/data/my_storage/tmp/cycle165-ccache make -j2` completed the entire configured graph with exit 0, including `bitcoind`, `bitcoin-cli`, `bitcoin-tx`, `bitcoin-util`, `test_bitcoin`, and the aggregate fuzz driver.

Focused release tests passed:

- `test_bitcoin --run_test=mempool_tests,miner_tests,txrequest_tests --random=165001`: 8 cases, 118,948 assertions.
- `test_bitcoin --run_test=net_tests,txpackage_tests,txvalidation_tests,validation_block_tests,validation_chainstate_tests,validation_tests --random=165002`: 36 cases, 195,437 assertions.
- `test_bitcoin --random=165003`: 565/566 cases, 20,071,853/20,071,853 assertions. The sole warning was the known unset `DIR_UNIT_TEST_DATA` skip for `script_assets_test`; no assertion failed.

The release file-based fuzz driver initially rejected `-runs=1000` as an input path; the corrected deterministic corpus contained zero bytes, `src/txrequest.cpp`, and `src/test/fuzz/txrequest.cpp`. `env FUZZ=txrequest src/test/fuzz/fuzz <corpus>` exited 0 with `txrequest: succeeded against 3 files in 0s.` No fuzz, sanitizer, daemon, or profiling process remains running.

### Boost 1.91 before/after compile control

The pinned Boost 1.91.0 source archive was downloaded from `https://archives.boost.io/release/1.91.0/source/boost_1_91_0.tar.bz2`, verified as `de5e6b0e4913395c6bdfa90537febd9028ea4c0735d2cdb0cd9b45d5f51264f5`, and used only from `/data/my_storage/tmp/`. A fresh 28.x configure with `CPPFLAGS=-I/data/my_storage/tmp/boost-1.91/boost_1_91_0` completed successfully. The patched release compiled `node/libbitcoin_node_a-miner.o`, `libbitcoin_node_a-txmempool.o`, and `libbitcoin_node_a-txrequest.o` with GCC 12 and Boost 1.91 headers, exit 0.

An independent pre-fix worktree at the immediate `#35214` release parent `b110304705` was configured identically. Its three corresponding object targets failed with exit 2. The first diagnostics were:

```text
src/txmempool.h:332:64: error: invalid use of incomplete type 'struct boost::multi_index::indexed_by<...>'
boost/multi_index/detail/node_type.hpp:40:66: error: static assertion failed: detail::is_index_list<IndexSpecifierList>::value
src/node/miner.h:101:68: error: invalid use of incomplete type 'struct boost::multi_index::indexed_by<...>'
```

The failure log also contained the expected `mp_rename_impl`/`mp_size_impl` errors and downstream missing iterator members. This is a clean failing-before/passing-after control for the exact compatibility claim; no production behavior change is needed beyond the type declaration rewrite.

### CI seccomp verification

The backport `2d3edd9640` adds `CI_CONTAINER_CAP=--security-opt seccomp=unconfined` to `ci/test/00_setup_env_i686_centos.sh`, `ci/test/00_setup_env_i686_multiprocess.sh`, and `ci/test/00_setup_env_win64.sh`. All three files are selected by the 28.x `.github/workflows/ci.yml` matrix. Sourcing each file in an isolated shell produced the expected container name, image, and capability. `bash -n` passed for all three environment files and `ci/test/02_run_container.sh`; the script's Docker invocation at line 76 expands `$CI_CONTAINER_CAP` immediately after `--cap-add LINUX_IMMUTABLE`, so shell tokenization produces `--security-opt seccomp=unconfined --rm ...` for each job. The Windows-cross job includes Wine32 and the 32-bit Linux jobs execute i686 binaries, so the release-specific broader assignment has a concrete socket-call compatibility rationale; no wrong-job or dropped-argument defect was demonstrated.

Docker and Podman are not installed on this host, so image pull, actual container startup, and i686/Windows execution could not be independently run. A pure command-token control confirmed that removing the environment value would remove the security option, while the committed value is present for every affected release job. This is a CI execution limitation, not evidence of a source defect.

### Candidate ledger and verdict

| Candidate | Classification | Verdict |
|---|---|---|
| The 28.x Boost fix omits a release-specific MultiIndex site or changes an index contract | Release source/backport | Dismissed; source inventory, ancestry comparison, complete build, focused suites, full unit suite, and fuzz corpus passed |
| The Boost fix breaks the older supported/system Boost path | Release build compatibility | Dismissed; Boost 1.74 full build/tests and Boost 1.91 changed-object compile passed |
| The seccomp workaround is assigned to the wrong jobs or is lost before `docker run` | CI configuration/backport | Dismissed; workflow selection, isolated environment sourcing, shell syntax, and argument expansion all match the intended jobs |
| The release-note/configuration batch lacks a prerequisite or has an unverified semantic conflict | Integration/backport | Dismissed; commit ancestry and changed-path audit found no missing prerequisite; no runtime behavior change is claimed for documentation-only hunks |

**Cycle verdict: dismissed; no confirmed current backport defect and no production or permanent test change justified.**

### Limitations and handoff

This cycle did not execute Docker 29.4.2, native i686, or Windows/Wine. The 28.x release's own CI and dependency containers remain the authoritative follow-up for those cells. The next distinct goal-66 cells are the 28.x `#35213` validation-lifetime integration, a 29.x release batch other than Cycle 67's `#34370`, or an upgrade/downgrade/backport fixture with new source evidence. The next uber cycle must perform a fresh gate, draw with the exact selector command, and preserve this dismissal without treating the unavailable external cells as passed.

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
