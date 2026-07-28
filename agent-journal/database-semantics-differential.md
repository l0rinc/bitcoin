# Database-engine and persistence-semantics differential

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
