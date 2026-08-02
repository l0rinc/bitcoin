# RAII, Smart-Pointer, and Resource-Leak Audit Cycle 73

## Identity and Gate

- Cycle: `73`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `54`
- Goal: `RAII, smart-pointer, and resource-leak audit`
- Slug: `raai-resource-leaks`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Branch: `uber-cycle-73-raii-resource-leaks-20260728`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `4935b3908795d5e2196fbf2b91c667790776cffa`
- `origin/master...HEAD` at the gate: `2 921`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- No relevant daemon, test, fuzz, sanitizer, Valgrind, or profiling process was running at the gate.

## Prior-Work Exclusions

Prior cycles already covered scheduler shutdown ordering, wallet/database cleanup, descriptor/provider ownership, Berkeley wallet migration, kernel wrapper ownership, and selected network queue lifetimes. This cycle must search those journals and history first, then select a distinct resource cell rather than repeating a closed finding.

## Initial Risk Map and Hypotheses

1. A raw operating-system or database handle may escape its owning RAII wrapper on a constructor, initialization, error, cancellation, or callback path, causing a leak or double close.
2. A moved or released smart pointer may leave a second owner, dangling view, or cleanup callback with the wrong lifetime, especially across asynchronous network, wallet, chainstate, and RPC paths.
3. A resource acquired before a lock/thread/callback boundary may not be released when the boundary throws, returns false, is cancelled, or is shut down concurrently.
4. Custom deleters, `unique_ptr::release`, `new`/`delete`, `CloseSocket`, file descriptors, mmap/munmap, LevelDB iterators, SQLite/BDB handles, and thread joins may have asymmetric success/failure contracts.

Primary surfaces are constructors/destructors and manual ownership bridges in `src/net`, `src/node`, `src/wallet`, `src/dbwrapper.cpp`, `src/util`, `src/qt`, and libsecp256k1 context/module wrappers. Exclude intentional process-lifetime singletons and documented ownership transfers unless a caller violates the transfer contract.

## Planned Evidence

- Build a resource ledger from static search, callers, history, and ownership annotations: acquire, owner, transfer, release, failure edges, cancellation edge, and proof of cleanup.
- Prioritize one concrete handle class at a time: sockets, files/mappings, database iterators/transactions, threads/callback registrations, and secure allocations. Use semantic call chains rather than name-only matches.
- Use compiler warnings/static analysis, ASan/LSan or Valgrind where available, deterministic fault injection, and small lifetime counters or test-only deleters. A leak report alone is insufficient if it is a harness or intentional process-lifetime object.
- For every candidate, establish a reachable failing path, minimize the resource count, compare before/after process or handle state, and preserve a regression test. Do not change ownership style without proving the old path leaks or double-releases.

## Cycle 73 Evidence and Verdict

### Candidate ledger

| Candidate | Acquire/owner path | Failure edge | Evidence | Verdict |
| --- | --- | --- | --- | --- |
| LevelDB options and environment in `CDBWrapper` | `GetOptions()` allocated an LRU cache, optional Bloom policy, and `CBitcoinLevelDBLogger`; `LevelDBContext` stored those objects, `pdb`, and an optional `NewMemEnv` as raw pointers | `CDBWrapper` construction could throw from `TryCreateDirectories()` or `HandleError()` before `CDBWrapper::~CDBWrapper()` became active | Clang 19 ASan/LSan standalone probe, a temporary destructor mutation, and the focused unit regression | Confirmed and fixed |
| Other manual file/socket/mapping/database ownership sites | Reviewed prior journals, current `new`/`delete`, `release`, `CloseSocket`, file, mmap, and database call chains in the scoped surfaces | No second reachable leak or double-release with an independent reproducer was established in this cycle | Static review and existing tests only | Inconclusive; queued with named surfaces rather than converted into speculative cleanup |

### Confirmed finding

Before the fix, `GetOptions()` transferred three raw allocations directly into a local `leveldb::Options`. `LevelDBContext` had no destructor, while `CDBWrapper::~CDBWrapper()` called `Close()` only for fully constructed objects. A deterministic constructor failure therefore skipped cleanup. The minimal trigger uses a regular file as the parent of the requested database path, so `TryCreateDirectories()` throws after options allocation and before `DB::Open()`:

```text
CDBWrapper({.path = "/dev/null/bitcoin-raii-cycle73", .cache_bytes = 1 << 20})
```

The old-source Clang 19 ASan/LSan probe caught the expected filesystem error and reported `4064 byte(s) leaked in 19 allocation(s)`, including the direct LRU cache, Bloom policy, logger, and indirect cache buckets. The same probe against the fixed source caught the same exception and produced no leak report. Replacing only `~LevelDBContext() { Close(); }` with `~LevelDBContext() = default;` restored the exact 4064-byte leak, establishing that the destructor is causally required rather than the result of probe setup.

The fix makes the three `GetOptions()` allocations local `unique_ptr`s until all allocations succeed, initializes raw context fields to null, gives `LevelDBContext` an idempotent `Close()` plus destructor, and makes `CDBWrapper::Close()` delegate to that owner. Cleanup order is database, filter policy, logger, block cache, then custom environment. The new `dbwrapper_constructor_failure_cleanup` test creates an uncreatable child path and verifies the public exception contract and parent cleanup.

### Commands and validation

- `cmake --build build_unit_clang19 --target test_bitcoin -j2` passed after the fix was restored.
- `build_unit_clang19/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=test_suite --report_level=short` passed: 14 cases, 2475 assertions.
- `/data/my_storage/tmp/sanitizer-analysis-matrix-cycle26/asan-unit-clang/bin/test_bitcoin --run_test=dbwrapper_tests --detect_memory_leaks --log_level=test_suite --report_level=short` passed: 14 cases, 2475 assertions, no leak diagnostic.
- The standalone probe in `agent-journal/raai_cycle73_dbwrapper_probe.cpp` was compiled and linked against the Clang 19 ASan/UBSan LevelDB build. With `ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:allocator_may_return_null=1'`, `LSAN_OPTIONS='detect_leaks=1:report_objects=1'`, and `UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'`, the fixed run exited 0 with the expected filesystem error and no leak output.
- A selected block/index/flush dependent-suite attempt was stopped after the host root filesystem reported `Disk space is too low!`; subsequent cascading assertions included `cs_args` double-lock state from the aborted fixture. The raw log is `/data/my_storage/tmp/raai-cycle73-dependent-tests.log`. The dedicated dbwrapper normal and sanitized suites are the authoritative validation for this finding; no process remains running.
- `git diff --check` passed. The temporary source mutation was restored before closing the cycle.

### Commit and handoff

- Source/test/probe/journal commit: `548d8cc8f8` (`dbwrapper: clean up resources on constructor failure`).
- Post-commit validation: normal and Clang ASan/LSan `dbwrapper_tests` each passed 14 cases and 2475 assertions with no leak diagnostic; `git diff --check` passed.
- Raw evidence: `/data/my_storage/tmp/raai-cycle73-dbwrapper-before.log`, `/data/my_storage/tmp/raai-cycle73-dbwrapper-after-asan.log`, `/data/my_storage/tmp/raai-cycle73-dbwrapper-mutated.log`, `/data/my_storage/tmp/raai-cycle73-dbwrapper-after-asan-tests.log`, and `/data/my_storage/tmp/raai-cycle73-dependent-tests.log`.
- Remaining resource cells: socket and callback cancellation, file/mapping failure paths, database iterator/transaction ownership, and secure allocation cleanup. Re-check the deduplication ledger before selecting one in the next cycle.
- Verdict: cycle 73 confirmed one reachable resource leak and fixed it; no repository-completion claim is made.

# RAII, Smart-Pointer, and Resource-Leak Audit Cycle 246

## Identity and Gate

- Cycle: `246`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `54`
- Goal: `RAII, smart-pointer, and resource-leak audit`
- Slug: `raai-resource-leaks`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc`
- Goal TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Branch: `uber-cycle-246-raii-smart-pointer-resource-leaks-20260731`
- Base: `origin/master` at `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `5eeab93b203c2b6264a1b5e4ebd8865a99b6bb07`
- `origin/master...HEAD` at the gate: `42 1275`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts, probes, package files, crash files, and `test/cache/` were preserved.
- Several unrelated sanitizer and test processes were already running in protected build directories. They were not stopped or rebuilt; the current IPC and wallet validation used independent scratch builds.

## Prior-Work Exclusions and Risk Map

The Cycle 73 LevelDB `CDBWrapper` constructor leak was already fixed and searched before this cycle. Scheduler shutdown, wallet/database cleanup outside the selected SQLite constructor path, descriptor/provider ownership, Berkeley migration, kernel wrapper ownership, DynSock stack ownership, and selected network queue lifetimes were also excluded from repetition by their prior journals. The remaining high-value cells were socket/callback cancellation, file/mapping failure paths, database iterator/transaction ownership, and secure allocation cleanup.

The cycle ledger covered four distinct ownership boundaries:

1. `mp::ConnectStream` released a loop-thread-owned `Connection` before proxy construction had completed.
2. `ProxyClientBase` registered a cleanup callback capturing `this` before all constructor operations had succeeded.
3. `SQLiteBatch::SetupSQLStatements` could leave earlier raw `sqlite3_stmt*` handles prepared when a later statement failed.
4. `mp::SpawnProcess` created a socketpair before a pre-fork callback and argv construction that could throw.

## Confirmed Findings

### IPC proxy construction released a connection too early

`ConnectStream` previously passed `connection.release()` through the successful proxy construction path. If allocation or `ProxyClient` construction threw after the Cap'n Proto connection had been created, the raw connection and owned socket escaped. A deterministic descriptor replay with the old code kept the descriptor count at `fd_before=17 fd_after=17` after the injected failure, while the corrected ownership path returned `fd_before=17 fd_after=16` and exit status 0. The final fix keeps the `unique_ptr` until proxy construction succeeds and resets it on the event-loop thread in the catch path. Allocation-failure injection under ASan produced no report after the fix.

Commit: `a8621425b454e9f2e85efffd43b92fa67d89ff22` (`ipc: retain connection ownership during proxy construction`).

### Proxy cleanup callback captured a dead object on constructor failure

`ProxyClientBase` registered a `Connection` sync-cleanup callback capturing `this`, then performed an allocating `emplace_front` and `Sub::construct(*this)`. If either operation threw, the object storage was released without running the destructor, but the callback remained and later dereferenced the dead object during connection teardown. The old same-thread ASan replay reported a heap-use-after-free in the generated `foo.capnp.h` callback path, through `proxy-io.h` and `Connection::~Connection`. The fixed rollback removes the callback via `loop.sync`, satisfying `Connection::removeSyncCleanup`'s event-loop-thread requirement. The exact injected registration test then passed under ASan with one test and no sanitizer report.

Commit: `c180456b34b3c4d00bcbbd538e8a034757ae3ad2` (`ipc: unregister cleanup callback on proxy construction failure`).

### SQLite batch construction leaked partially prepared statements

`SQLiteBatch` initialized raw statement members to null and prepared them sequentially in `SetupSQLStatements`. On an authorizer denial of a later `SQLITE_INSERT` prepare, the constructor threw after an earlier statement had been prepared. Because destructors do not run for an object whose constructor throws, the earlier statement remained registered on the database. The focused pre-fix test reported `statement_count == 0` as `[1 != 0]`. The constructor now calls the existing non-throwing `Close()` cleanup path before rethrowing. The same test passed afterward with all three assertions passing.

The permanent regression is `sqlite_batch_constructor_failure_releases_statements` in `src/wallet/test/wallet_tests.cpp`. The source/test commit is `5bc4c07ce0554aed3c0d1ee5f15423f8db93f71a` (`wallet: clean up SQLite batch construction failures`).

### SpawnProcess leaked both socketpair descriptors before fork

`SpawnProcess` created two raw descriptors, then invoked `fd_to_args` and `MakeArgv` before `fork()`. An exception from the callback therefore bypassed every close path. The new libmultiprocess test reproduced the old behavior exactly: `CountOpenFds()` changed from 4 to 6 after the callback threw, with no child process created. Local `ScopedFd` guards now own both descriptors until the parent/child transfer points, and the existing close/error behavior remains in place after `fork()`. The full standalone mptest suite passes after the change.

The source/test commit is `9105c2db16c102bf11245f4306e73203c600d78d` (`ipc: close SpawnProcess descriptors on setup failure`).

## Validation

- `cmake --build /data/my_storage/tmp/cycle246-mp-base --target mptest -j2` passed after both IPC source changes.
- `ctest --test-dir /data/my_storage/tmp/cycle246-mp-base --output-on-failure` passed: one test target, all tests passed, including the pre-fork exception FD regression and all existing IPC tests.
- `/data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=sqlite_batch_constructor_failure_releases_statements --log_level=test_suite --report_level=short --color_output=false` passed with `TMPDIR=/data/my_storage/tmp`: one case, three assertions.
- The same wallet test against the pre-fix SQLite source failed with one remaining prepared statement, proving the regression is sensitive to the old behavior.
- `cmake --build /data/my_storage/tmp/cycle246-wallet --target test_bitcoin -j2` passed after the SQLite fix. The initial `-fanalyzer` wallet build was killed by host memory pressure while compiling an unrelated file; it was not used as evidence.
- The standalone libmultiprocess build and test target were separate from protected long-running builds. No protected process was stopped or rebuilt.
- `git diff --check` passed before each source commit. The two IPC commits, SQLite commit, and this journal close are independent commits intended to build and test alone.

## Residual Queue and Verdict

Remaining cells are file/mapping failure paths, further database iterator/transaction ownership, secure allocation cleanup, and the `echoipc`/spawn lifecycle paths that were inspected but not independently reproduced this cycle. In particular, `echoipc`'s `init.release()` callback registration and the post-fork parent-close failure path need a separate fault-injection harness before any change is justified. Re-check the finding index and prior journals before selecting one.

Verdict: Cycle 246 confirmed and fixed four independent reachable resource-lifetime defects. This is a finite evidence-backed cycle, not a repository-completion claim. The next run must select a new unchecked hypothesis from the accumulated map.

# RAII, Smart-Pointer, and Resource-Leak Audit Cycle 278

## Identity and Gate

- Cycle: `278`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `54`
- Goal: `RAII, smart-pointer, and resource-leak audit`
- Slug: `raai-resource-leaks`
- Branch: `uber-cycle-278-raii-resource-leaks-20260802`
- Base: `origin/master` at `556988790a7f961693a8fd93f73725baea66476a`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `8957ad563f60845eaff97b5a7e3a406e97d664e3`
- `origin/master...HEAD` at the gate: `45 1345`
- Uber-goal state SHA-256 at the gate: `1ff34f7ceee52a933c41361f136d8e2f67f9090f9f40e7e20f9af7beeee2dbae`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goal TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc`
- The tracked/index state and `git diff --check` were clean at entry. Existing untracked agent artifacts, crash files, package files, `node_modules/`, `test/cache/`, and prior probes were preserved and excluded.
- Protected long-running tests with PIDs `777094`, `956381`, `1138182`, `1157959`, `1312049`, `1312050`, and `1346200` were alive and were not touched. All new builds and test data used `/data/my_storage/tmp`.

## Scope and Prior-Work Exclusions

Cycle 73's LevelDB constructor cleanup and Cycle 246's IPC connection, proxy-callback, SQLite statement, and `SpawnProcess` descriptor fixes were searched first. The remaining map included file/mapping paths, database iterator/transaction ownership, secure allocation cleanup, and kernel C/C++ wrapper failure paths. The current cycle selected the unindexed logging callback and wrapper ownership boundary rather than reopening those findings.

## Working Hypothesis

`btck::Logger` in `src/kernel/bitcoinkernel_wrapper.h` released its `std::unique_ptr<T>` before `btck_logging_connection_create()` proved that a logging connection existed. The C API explicitly returns null on construction failure, so the wrapper could lose the callback object. The native `LoggingConnection` constructor also registered the callback before allocating its iterator and before completing logging setup; an exception in that interval could leave a callback pointing at a caller-owned object that the failed wrapper construction subsequently destroyed.

## Independent Discovery and Verification

The ownership path was traced from `Logger::Logger` through `btck_logging_connection_create()` to `Handle::create()` and `LoggingConnection`. The old wrapper passed `log.release()` into the C API. The old native constructor removed the callback and invoked the user-data deleter only for the `StartLogging()` false branch; allocation or later logging exceptions had no rollback. The public header promised ownership to the created connection but did not define failure ownership.

A deterministic public C API failure was produced by setting the internal logger to file output with a path whose parent did not exist. A `CountingLog` object and destroy callback showed the pre-fix behavior invoked the destroy callback before `btck_logging_connection_create()` returned null (`destructions == 1`), while the corrected contract leaves the object with the caller (`destructions == 0` until the caller deletes it). The same test also verifies that no callback remains registered after failure.

The fix passes `log.get()` to the C API and calls `release()` only after successful base construction. `LoggingConnection` now initializes its user-data pointer, wraps setup in a catch block, removes the registered callback on every exception, and leaves failed-call user data with the caller. The public header documents that `user_data_destroy_callback` runs only after successful connection creation and is not called when null is returned. The regression is `logging_connection_failure_retains_user_data` in `src/test/kernel/test_kernel.cpp`.

For independent pre-fix evidence, the same test was built after temporarily restoring the original constructor. It failed at the ownership assertion with `[1 != 0]`; after restoring the fix it passed all four assertions. This establishes sensitivity to the old behavior rather than an execution-only coverage addition.

## Validation

- Clean Clang 19 Debug kernel-library build: `/data/my_storage/tmp/cycle278-kernel-build`, `cmake --build ... --target test_kernel -j2` passed.
- Focused fixed test: `test_kernel --run_test=logging_connection_failure_retains_user_data --random=278005` passed 1 case and 4 assertions.
- Existing logging regression: `test_kernel --run_test=logging_tests --random=278002` passed.
- Full fixed kernel suite with `TMPDIR=/data/my_storage/tmp/cycle278-test-tmp`: `test_kernel --random=278006` passed 20 cases and 3723 assertions. The first run using the host `/tmp` was not used as evidence because the root filesystem was full and several tests could not create chainstate data.
- Clang 19 ASan+UBSan build: `/data/my_storage/tmp/cycle278-kernel-asan`, `cmake --build ... --target test_kernel -j2` passed.
- ASan+UBSan focused run with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1` passed 1 case and 4 assertions with no sanitizer or leak diagnostic.
- `git diff --check` passed after the final source/test edits. No protected process was stopped or rebuilt.

## Commit and Residual Queue

- Source/test commit: `56c7ce1254` (`kernel: preserve logging callback ownership on failure`), containing the wrapper, native constructor rollback, public contract, and focused regression.
- The test exercises deterministic logging-start failure, not allocator failure at the iterator or `LoggingConnection` allocation sites. The exception rollback is established by code-path analysis and the public failure test; an allocation-failure interposer remains a useful future kernel-wrapper cell.
- Remaining queue: file/mapping failure paths, database iterator/transaction ownership, secure allocation cleanup, IPC `ListenConnections` raw-fd transfer, and allocation-failure coverage for C API wrappers. Search this entry and the deduplication journal before selecting any of them.

Verdict: confirmed one kernel C/C++ ownership and callback-cleanup defect and fixed it with the smallest compatible transfer/rollback change. This is a finite evidence-backed cycle, not a repository-completion claim. Continue with a fresh gate and exact random selection.
