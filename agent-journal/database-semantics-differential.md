# Database-engine and persistence-semantics differential

## Cycle 272: SQLite transaction-listener abort contract

### Selection and gate

- Exact selector after the Cycle 271 close: `shuf -i 0-98 -n 1` -> `95` (`database-semantics-differential`); no reroll was needed.
- Branch: `uber-cycle-272-database-semantics-differential-20260802`.
- Cycle start HEAD: `6458305c8f59650ed1d0dc44bdb6476ee1632626`.
- `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- `git rev-list --left-right --count HEAD...origin/master`: `1333 45`.
- The tracked/index state and `git diff --check` were clean at entry. Known unrelated untracked probes, catalogs, crash artifacts, caches, and package files were preserved and excluded from staging.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Corrected TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Uber-goal state SHA-256 at the gate: `004926f21ddae890e5f711a3e87a8ac4bd94f57ac92657546e6fcaec9478550b`.
- Storage gate: `/` was full and `/data` had about 14 GiB free; all scratch builds and test temporary directories used `/data/my_storage/tmp`.
- Protected long-running test processes, including PIDs `777094`, `956381`, `1138182`, `1157959`, `1312049`, `1312050`, and `1346200`, were alive and were not touched.

### New cell and exclusions

This cycle excludes Cycle 45's fixed LevelDB iterator-status defect, Cycle 126's broad LevelDB batch/recovery/WAL/MANIFEST/snapshot/comparator/checksum/compaction/sync and v31.1 matrix, Cycle 136's dismissed ordinary `CCoinsViewDB` reader-lifetime cell, and Cycle 185's fixed LevelDB write-sync wrapper test. The new cell is the transaction outcome bridge in the SQLite wallet backend: whether wallet transaction listeners receive a valid abort callback when a transaction is rolled back after database writes have been staged.

### Working hypothesis

`WalletBatch::TxnAbort()` unconditionally invokes every listener's `std::function<void()> on_abort`. Four current wallet call sites used `.on_abort = {}`, which creates an empty function rather than a callable no-op. `DescriptorScriptPubKeyMan::Encrypt()` registers one of those listeners only after successfully staging encrypted records. A later rollback therefore could throw `std::bad_function_call` instead of completing the SQLite transaction abort. The trust boundary is the wallet transaction abstraction, SQLite rollback/semaphore release, and in-memory state that must remain unencrypted after an aborted encryption transaction.

### Independent evidence

1. Static contract trace: `WalletBatch::TxnAbort()` calls `listener.on_abort()` without checking it. `DescriptorScriptPubKeyMan::Encrypt()` registers an empty abort function after all of its database writes succeed, and `CWallet::EncryptWallet()` explicitly calls its rollback routine when a later manager or commit fails. The same empty-function construction existed in transaction listeners for transaction removal, address-book updates, and previously-spent address state.
2. Deterministic pre-fix runtime: the added `scriptpubkeyman_tests/encrypt_descriptor_abort_preserves_state` test creates an in-memory SQLite wallet, starts a transaction, successfully stages `DescriptorScriptPubKeyMan::Encrypt()`, and aborts it. Against the unpatched implementation, `/data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=scriptpubkeyman_tests/encrypt_descriptor_abort_preserves_state --log_level=message --report_level=short --color_output=false --random=272001` exited `201` with `unexpected exception thrown by batch.TxnAbort()`; six state/setup assertions passed and the abort assertion failed.
3. The failure is backend-driven but deterministic: `SQLiteBatch::TxnAbort()` completes `ROLLBACK TRANSACTION`, clears its active-transaction state, and then `WalletBatch` invokes the listener. The exception is therefore a transaction API failure after the database has already rolled back, not a speculative SQLite syntax issue.

### Fix

Replace all four empty abort `std::function` initializers with explicit `[] {}` no-op callbacks. Add the focused regression to verify descriptor encryption remains in the private-key state and that abort returns normally after all encrypted records were staged.

### Verification

- `ninja -C /data/my_storage/tmp/cycle246-wallet test_bitcoin -j2`: passed with GCC 12.2 after the source and test changes.
- `TMPDIR=/data/my_storage/tmp/cycle272-post-fix-tmp /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=scriptpubkeyman_tests/encrypt_descriptor_abort_preserves_state --log_level=message --report_level=short --color_output=false --random=272002`: passed 1 case and 7 assertions.
- `TMPDIR=/data/my_storage/tmp/cycle272-suite-tmp /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=scriptpubkeyman_tests,walletdb_tests --log_level=message --report_level=short --color_output=false --random=272003`: passed 24 cases and 210 assertions.
- `TMPDIR=/data/my_storage/tmp/cycle272-wallet-suite-tmp /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=wallet_tests --log_level=message --report_level=short --color_output=false --random=272004`: passed 26 cases and 220 assertions.
- `git grep -n -E 'on_abort[[:space:]]*=[[:space:]]*\\{\\}' -- src/wallet` returned no empty abort callbacks after the fix, and `git diff --check` passed.

### Verdict and limits

Confirmed and fixed: several SQLite-backed wallet rollback paths could throw `std::bad_function_call` after the database rollback had completed because an empty `std::function` was treated as a no-op callback. The regression uses the in-memory SQLite backend and the descriptor-encryption transaction path; it does not exercise a real power-loss schedule, a commit-time filesystem failure, or a removed historical BDB engine. Those remain separate evidence cells. The changed callbacks are behavior-neutral on successful commits and make abort behavior match the existing explicit no-op callback convention.

### Handoff

The source/test/journal finding commit and the separate state-only close commit must be authored as `Lőrinc <pap.lorinc@gmail.com>`. The next cycle must refresh the gate, draw with the exact selector, and avoid reopening this callback cell without new evidence from a different backend or a commit/restart fault schedule.

## Cycle 185: wrapper partial-write and sync-failure contract

### Selection and gate

- Exact selector after the Cycle 184 close: `shuf -i 0-98 -n 1` -> `95` (`database-semantics-differential`); no reroll was needed.
- Branch: `uber-cycle-185-database-semantics-differential-20260731`.
- Cycle start HEAD: `707d625d557035c78bd0aaecb543639dbea001ef`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- `git rev-list --left-right --count HEAD...origin/master`: `1160 42`.
- Tracked/index state was clean at entry; known unrelated untracked artifacts were preserved and excluded from all staging.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Corrected TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Uber-goal state SHA-256: `b5da487f36059bf682bdd344912745cf5ae55d8f1915586d1cc5997a0040d299`.
- TSV validation: 99 records, four fields each, IDs 0 through 98 exactly once.
- Storage gate: `/` had about 57 MiB free and `/data` about 49 GiB free; disposable test/runtime files must use `/data/my_storage/tmp`.
- Unrelated long-running test processes with PIDs 777094 and 956381 were alive and were not touched.

### New cell and exclusions

This cycle excludes Cycle 45's fixed iterator-status defect, Cycle 126's broad LevelDB batch/recovery/WAL/MANIFEST/snapshot/comparator/checksum/compaction/sync and v31.1 matrix, and Cycle 136's dismissed ordinary `CCoinsViewDB` reader lifetime cell. No alternative RocksDB/Pebble engine is installed in the current environment, so the selected cell is the narrower wrapper contract when LevelDB reports a partial write or sync failure: whether `CDBWrapper::WriteBatch` exposes the engine's failure, whether a failed batch changes visible state, and whether reopening preserves the documented recovery state.

### Working hypothesis

The current `CDBWrapper::WriteBatch` selects LevelDB's synchronous or asynchronous `WriteOptions`, passes the native `WriteBatch` unchanged, and throws through `HandleError`. The hypothesis is that this wrapper boundary either hides a LevelDB partial-write/recovery distinction or leaves callers with a state that contradicts the chainstate/index write protocol. The LevelDB `WriteSyncError` contract is the comparison oracle, not an assumption that failed writes must roll back atomically.

### Verification and result

LevelDB's `DBImpl::Write()` appends the batch to the log, attempts `Sync()` for a synchronous write, and deliberately does not insert the batch into the memtable when that sync fails. It records a background error because the log record may or may not be durable, so subsequent writes fail until the database is reopened. This is also the contract exercised by the vendored `DBTest.WriteSyncError`: a prior key remains readable, the failed key is absent in the live instance, and later writes fail. A failed synchronous write is therefore not a promise of rollback after restart; recovery must tolerate either durable outcome for the failed record.

The wrapper-specific gap was real but was test-only: `dbwrapper_tests` covered iterator read faults and ordinary batches but had no write-side fault oracle. `src/test/dbwrapper_tests.cpp` now supplies a memenv-backed `WriteSyncErrorEnv` which fails only `.log` `Sync()` calls. `dbwrapper_write_sync_error` verifies that `CDBWrapper::WriteBatch(..., true)` throws `dbwrapper_error`, the preexisting key remains readable, the failed key is absent before close, a later write is rejected by LevelDB's recorded background error, and a clean reopen preserves the preexisting key. It does not assert a particular post-reopen value for the failed key because LevelDB explicitly treats that outcome as indeterminate after a sync error.

Evidence:

- The standalone current LevelDB `db_test` completed all 56 DB tests, including `WriteSyncError`, recovery, non-writable filesystem, manifest-write, and compaction-error cases. The standalone fault-injection target completed both tests.
- The current-tree Clang 19 release `test_bitcoin` rebuilt successfully, then `TMPDIR=/data/my_storage/tmp/cycle185-bitcoin-full-runtime .../bin/test_bitcoin --run_test=dbwrapper_tests --log_level=message --report_level=short --color_output=false` passed 16 cases and 2,484 assertions. Five isolated repeats of the new case each passed all 7 assertions.
- The current-tree Clang 19 UBSan target rebuilt successfully. With `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, the focused case and the complete 16-case `dbwrapper_tests` suite passed 7 and 2,484 assertions respectively, with no sanitizer diagnostic.
- `git diff --check` passed. The unrelated long-running PIDs 777094 and 956381 remained alive and untouched.

Verdict: no production `CDBWrapper` or LevelDB semantic defect was confirmed. The wrapper already exposes the native failure and its callers use exception-driven abort/replay behavior consistently. The permanent regression test is justified as a missing contract oracle, not as a behavior fix. No alternate RocksDB/Pebble implementation is installed, and the memenv fault cannot model torn sectors or real power loss; those remain future evidence sources.

## Cycle 136: replaceable chainstate wrapper reader contract

### Selection and gate

- Exact selector after the Cycle 135 gate: `shuf -i 0-98 -n 1` -> `95` (`database-semantics-differential`); no reroll was needed.
- Branch: `uber-cycle-136-database-semantics-differential-20260730`.
- Cycle start HEAD: `db6d40b1372d63fea0408c305294784891ef38f9`.
- The prior Cycle 126 campaign already covered the broad CDBWrapper/LevelDB batch, recovery, WAL/MANIFEST, snapshot, iterator, corruption, checksum, comparator, compaction, sync, and v31.1 engine matrix. Cycle 45's iterator-status defect and the later coins cursor lifetime fixes were excluded.

### New cell and hypothesis

The new cell was the lifetime contract for ordinary `CCoinsViewDB` readers while `ResizeCache()` destroys and reopens the replaceable `CDBWrapper`. `GetCoin()`, `HaveCoin()`, `GetBestBlock()`, `GetHeadBlocks()`, `NeedsUpgrade()`, `EstimateSize()`, and `GetDBProperty()` directly dereference `m_db` without taking `m_db_mutex`, while `ResizeCache()` takes that mutex only after requiring `cs_main`.

Hypothesis: a production caller could reach one of those methods without `cs_main` or without the cursor's `m_db_mutex` protection, allowing a concurrent cache resize to use or destroy `m_db` and produce a use-after-free, invalid LevelDB access, or inconsistent persistence result.

### Contract and caller trace

- `Chainstate::CoinsDB()` asserts and declares `EXCLUSIVE_LOCKS_REQUIRED(::cs_main)`. `Chainstate::ResizeCoinsCaches()` also requires `cs_main` before calling `CoinsDB().ResizeCache()`.
- Startup `NeedsUpgrade()` runs inside `CompleteChainstateInitialization()`, which is explicitly `EXCLUSIVE_LOCKS_REQUIRED(::cs_main)`.
- Chainstate `BatchWrite()`, `GetHeadBlocks()`, `GetBestBlock()`, and ordinary coin lookups route through the chainstate/cache paths protected by `cs_main`; the RPC, REST, and node-interface direct lookup paths all acquire `cs_main` before touching the active chainstate.
- `gettxoutsetinfo` obtains the DB reference under `cs_main`. `kernel::ComputeUTXOStats()` reacquires `cs_main` while creating the cursor and reading its best block, then keeps the cursor alive through the complete scan. The cursor owns `m_db_mutex`, so a concurrent resize may wait but cannot reset the database under the scan. `EstimateSize()` is called before that cursor is destroyed.
- Snapshot preparation holds `cs_main` across the flush, statistics, and cursor construction. `Cursor()` and `CompactFullAsync()` have explicit `!m_db_mutex` contracts and runtime assertions, and `ResizeCache()` holds the mutex while replacing `m_db`.
- The only current `GetDBProperty()` caller is the coins unit test. `EstimateSize()` is used by the coinstats path and test/fuzz storage helpers; no unprotected production caller was found.

The code therefore uses two compatible protections: `cs_main` excludes ordinary chainstate operations from cache replacement, while `m_db_mutex` extends the lifetime for asynchronous compaction and persistent cursors that intentionally outlive the initial `cs_main` section.

### Snapshot-size side check

The long coinstats scan can observe a LevelDB database that advances after its cursor snapshot was created. Its `nDiskSize` is calculated from `GetApproximateSizes()` after the scan, not from a LevelDB snapshot. This is not a demonstrated contract violation: the RPC describes it as the estimated current chainstate disk size, and the functional test explicitly treats `disk_size` as nondeterministic across equivalent calls. The counts, hash, and block identity remain tied to the cursor snapshot.

### Verification

- `mkdir -p /data/my_storage/tmp/cycle136-test-tmp` was required before the test runner could create its temporary directory; the initial setup issue was corrected without changing the repository.
- `TMPDIR=/data/my_storage/tmp/cycle136-test-tmp /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=coins_tests,dbwrapper_tests,coinstatsindex_tests --log_level=test_suite --color_output=false` passed 53 selected cases and ended with `*** No errors detected`.
- The run included persistent `coins_db_resize_cursor`, malformed first-key rejection, LevelDB layout/compaction, the complete current dbwrapper error/iterator/reopen suite, and both coinstats initial-sync/unclean-shutdown cases.
- The focused source/history trace and the existing persistent resize test independently cover the two lifetime mechanisms. No sanitizer trace, failing-before behavior, or engine-specific semantic divergence was found for this new cell.

### Verdict and limits

Dismissed for this cycle. No source or permanent test change is justified. The unannotated methods depend on the enclosing chainstate contract, and the one long-running storage path that releases `cs_main` retains the DB through its cursor. No alternate RocksDB/Pebble engine is installed, and no real power-loss device schedule was available; those remain future evidence sources rather than findings.

### Handoff

The next cycle must refresh the gate, rerun the exact selector, and choose a goal other than this already-covered reader-lifetime cell. Preserve the distinction between the fixed cursor/resize defect and this dismissed ordinary-reader contract audit.

## Cycle 126: LevelDB batch, recovery, and engine-version differential

### Selection and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `95` (`database-semantics-differential`).
- Branch: `uber-cycle-126-database-semantics-differential-20260730`.
- Cycle start HEAD: `418c28820de8ad27e53df358c58951f24a3162d3`.
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `40 1041` (`origin/master...HEAD`).
- The fresh gate fetched `origin master`, passed tracked/index cleanliness and `git diff --check`, matched the catalog/prompt/TSV hashes, and preserved PID `777094` with its Codex parent `725042`.

### Scope and exclusions

Cycle 45 fixed the Bitcoin wrapper's loss of LevelDB iterator read errors. This cycle excluded that path and selected the queued batch/recovery ordering, sync durability, comparator, and engine-version cells. The hypotheses were that a current or v31.1 engine would disagree on write-batch ordering, snapshots/iterators, WAL/MANIFEST recovery, corruption handling, filters, compaction, or sync-related semantics used by `CDBWrapper`, or that the wrapper's model/fault harness would expose a current invariant violation.

### Wrapper evidence

- The current Clang 19 Release `test_bitcoin` passed `dbwrapper_tests`: 14 cases, 2,475 assertions. The v31.1 Clang 19 Release scratch binary passed its common wrapper suite: 9 cases, 2,411 assertions. The extra current cases are the post-v31.1 cleanup and error-contract regressions; no common case failed.
- The existing deterministic `dbwrapper` fuzz harness was exercised with a fixed scratch seed and returned `dbwrapper: succeeded against 1 files in 0s.` Its model covers bytewise serialized-key ordering, point reads, existence, batches with overwrite/delete ordering, clear/reuse, reopens, optional forced compaction, iterators, deserialization failures, size estimates, obfuscation, and controlled background compaction. No failure or model mismatch was produced.
- Source review covered `CDBBatch::WriteImpl`/`EraseImpl`, `CDBWrapper::WriteBatch`, `fSync` selection, `CompactFull`, custom `testing_env` ownership, `CDBIterator` snapshots, prefix ranges, chainstate partial-flush markers, and all production LevelDB key prefixes. The apparent obfuscation-metadata collision is a reserved internal key namespace: production LevelDB callers use explicit ASCII prefixes, and the existing malformed-key test exercises reopening behavior; no reachable production collision was established.

### Cross-version LevelDB verification

- The vendored current LevelDB source was configured standalone with Clang 19 Debug and all tests enabled in `/data/my_storage/tmp/cycle126-leveldb-tests`. The complete CTest suite passed `30/30` tests in 92.76 seconds. This includes `db_test` (56 internal tests), recovery, fault injection, corruption, write batches, snapshots, comparators, tables, filters, environment, and platform-file tests.
- The v31.1 LevelDB source was first configured unmodified. Its own CMake selected C++11 while `util/no_destructor.h` used `std::is_standard_layout_v`, so the standalone build failed before runtime tests. This is a historical release build mismatch, not a database behavior result. The v31.1 scratch copy was then configured with C++17 and the missing placement-new declaration only to run the engine tests; the Bitcoin v31.1 superproject build already supplies a newer language mode. The complete adjusted CTest suite also passed `30/30` in 96.90 seconds.
- `git diff v31.1..HEAD -- src/leveldb` shows only source-backed maintenance differences: standalone CMake language-mode handling, initialization of `manifest_size`, the POSIX mmap limit, Windows handle move assignment, standard fallthrough annotations, and the `<new>` include. The runtime suite found no behavior divergence attributable to these changes.
- The focused current and v31.1 `db_test`, `recovery_test`, `fault_injection_test`, and `corruption_test` runs independently passed 56, 7, 2, and 12 internal tests on each version. No batch ordering, recovery, checksum, compaction, comparator, snapshot, or sync-related discrepancy appeared.

### Verdict and limits

Dismissed for this cycle. The Bitcoin wrapper tests, deterministic model harness, and complete cross-version LevelDB test matrices found no current persistence-semantics defect or unexplained engine drift. No production or permanent test change was justified.

The v31.1 standalone CMake mismatch is recorded as a historical build/configuration limitation rather than patched in the repository. No alternative engine such as RocksDB or Pebble was installed, no real power-loss device fault was available, and the custom fuzz binary was a deterministic file runner rather than a mutation engine. Future database cycles should target real process-kill/partial-filesystem schedules, a second installed engine, or a newly observed wrapper contract change. The exact scratch paths and test logs are retained for recurrence checks.

## Cycle 45

### Selection and gate

- Selected by the exact selector `shuf -i 0-98 -n 1`: goal `95`, `database-semantics-differential`.
- Goal prompt source: `agent-goals/goals.tsv` row `95` and `agent-journal/reusable-continuous-agent-goals.md`.
- Initial HEAD: `bf1711a9cf9838194e684e261ddf5e60f336db07`.
- Merge-base with `origin/master`: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Divergence at entry: `origin/master...HEAD = 2 860`.
- Entry tracked and staged state: clean. Untracked agent journals, probes, catalog files, and existing test cache artifacts were preserved.
- `origin/master` was fetched before selection and remained `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`.

### Scope and prior-art separation

The cycle compared the Bitcoin `CDBWrapper` contract with the vendored LevelDB iterator and database contracts, then used the wrapper's `DBParams::testing_env` hook for an engine-neutral fault experiment. RocksDB, Pebble, and other alternative database binaries or headers were not installed in the environment, so no alternative engine was treated as an oracle.

Prior cycle 17 measured LevelDB disk and index I/O without finding a production defect. Prior dbwrapper work already covers snapshot stability, exhausted iterator access, malformed obfuscation keys, and serialization-exception classification. This cycle targeted a distinct cell: LevelDB iterator I/O status after `Valid()` becomes false.

### Contract and hypothesis

LevelDB's `src/leveldb/include/leveldb/iterator.h` says `Valid()` only reports whether the iterator is positioned at an entry and separately requires callers to inspect `status()` for errors. `src/leveldb/db/db_iter.cc` preserves the underlying iterator status. Bitcoin's `CDBWrapper::ReadImpl()` and `ExistsImpl()` route non-NotFound LevelDB statuses through `HandleError`, but `CDBIterator::Valid()` previously returned only `iter->Valid()` and discarded `iter->status()`.

Hypothesis: a checksum or read failure while an index/chainstate iterator is positioned would be converted into ordinary end-of-iteration. Callers such as `CCoinsViewDBCursor`, `BlockTreeDB::LoadBlockIndexGuts`, and index range walkers can then return a missing/incomplete result instead of surfacing database corruption.

### Independent evidence

1. Static contract comparison: LevelDB exposes the error through `Iterator::status()`, while the Bitcoin wrapper exposed no status and did not call it. The direct key/value read path already establishes the local policy of throwing `dbwrapper_error` for non-NotFound LevelDB errors.
2. Deterministic runtime fault: `src/test/dbwrapper_tests.cpp` adds `IteratorReadErrorEnv`, backed by `leveldb::NewMemEnv`. The test writes a real entry, calls `CompactFull()` so the entry is in an SSTable, enables a wrapper that returns `Status::Corruption("injected iterator read failure")` for subsequent random reads, seeks to the first entry, and checks that `Valid()` throws `dbwrapper_error` after the fix.
3. Before/after and mutation proof: before the production change the new test failed with `exception dbwrapper_error expected but not raised` (exit status 201). After the change it passed. Restoring the one-line old implementation reproduced the same failure; restoring the status check passed again. The existing full dbwrapper suite also passed, covering ordinary exhaustion and all existing iterator behavior.

### Fix

`CDBIterator::Valid()` now checks the underlying LevelDB status whenever the iterator is invalid and calls the existing `HandleError()` path for non-OK status. Clean exhaustion still returns `false`; corruption and I/O failure now remain distinguishable from exhaustion. The test target receives the vendored LevelDB include directory so the fault-injection test can use the existing `Env` and memenv APIs.

### Commands and results

- `git fetch origin master`: passed; `origin/master` unchanged.
- `cmake --build build_unit_clang19 --target test_bitcoin -j2`: passed after the test include path and production fix.
- `build_unit_clang19/bin/test_bitcoin --run_test=dbwrapper_tests/dbwrapper_iterator_read_error --log_level=message --catch_system_error=no`: passed after the fix.
- `build_unit_clang19/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=message --catch_system_error=no`: passed, 12 test cases.
- Mutation with `CDBIterator::Valid()` reverted to `return m_impl_iter->iter->Valid()`: targeted test failed with the expected missing exception, exit status 201.
- `git diff --check`: passed.

### Verdict and limitations

Verdict: confirmed local persistence/error-propagation defect, fixed. The experiment uses an injected LevelDB corruption status rather than a damaged on-disk table and does not compare a second database engine because none is available. It proves the wrapper boundary and preserves the upstream LevelDB status contract; a future cycle may add a real table-corruption functional test or compare an installed alternative backend if the environment changes.

### Commit and handoff

The source/test change is committed independently as `dbwrapper: preserve iterator read errors`; the journal/state handoff is in a separate journal commit. Both use the required author identity. The next cycle must re-read this journal, avoid repeating iterator status, and select a distinct database semantic such as batch/recovery ordering, sync durability, or comparator behavior before drawing the next goal.
