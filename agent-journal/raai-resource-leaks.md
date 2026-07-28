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
