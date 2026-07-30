# Database-engine and persistence-semantics differential

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
