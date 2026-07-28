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

## Active State

- Status: in progress
- Current focus: constructor/error/cancellation ownership asymmetry across sockets, files, database iterators, callback registrations, and secure resources.
- Next action: search the existing journal/history ledger, inventory manual ownership sites, then choose the highest-risk unchecked cell.
