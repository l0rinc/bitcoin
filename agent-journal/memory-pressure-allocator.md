# Memory Pressure, OOM, Allocator, and Fragmentation Audit

## Cycle 266: PoolResource chunk allocation failure cleanup

- Goal: `74`, `memory-pressure-allocator`; exact selector `shuf -i 0-98 -n 1` -> `74`.
- Branch: `uber-cycle-266-memory-pressure-allocator-20260802`.
- Start HEAD/state close: `46781c48c19553b349ff7f0767d29e2bab5ff478`; `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start `git rev-list --left-right --count HEAD...origin/master`: `1321 45`.
- The tracked/index gate was clean at cycle start. Existing untracked goal, journal, probe, package, crash, and test-cache artifacts were preserved. Protected long-running tests were left running. Scratch state used `/data/my_storage/tmp/cycle266-*`; no default datadir, wallet, key, or production database was used.

### Scope and caller audit

The previous memory handoff left three distinct cells: a fresh chainstate/cache
resize workload, allocation failure outside the closed prevector policy cell,
and a post-commit `DynamicMemoryUsage()` owner review. The fresh cache review
ran first. `Chainstate::ResizeCoinsCaches()` compares only the coin-tip cache
size when deciding whether to call `FlushStateToDisk(IF_NEEDED)` or
`FORCE_FLUSH`; a direct database-only resize would therefore discard cached
coin entries even though the coin-tip budget did not shrink. History from
`f36aaa6392` through `7099e93d0a8` describes reallocation as necessary for a
shrinking in-memory coin map. All current production callers found in
`ActivateSnapshot()` and `MaybeRebalanceCaches()` change both dimensions
together. The existing direct resize and cache-rebalance tests passed, so the
database-only edge remains an uncovered API case rather than a reachable
runtime defect in this cycle.

The allocation-failure review then traced `PoolResource::AllocateChunk()`.
`PoolAllocator` backs `CCoinsMap` and other unordered-map accounting paths, so
chunk rollover is reachable during ordinary cache/graph growth. The old code
allocated an aligned chunk, published its cursor members, and only then called
`m_allocated_chunks.emplace_back()`. If the list-node allocation threw
`std::bad_alloc`, the aligned chunk was neither tracked nor reclaimable by the
resource destructor. The partially published cursor also described memory
outside the tracked chunk list. This is an allocator cleanup defect, distinct
from the repository-wide deliberate fatal `new_handler` policy: the failure
can occur in a component that still exposes and propagates `std::bad_alloc`.

### Deterministic failure proof

`/data/my_storage/tmp/cycle266_pool_allocate_failure_probe.cpp` overrides the
global allocation functions only in the scratch executable. It constructs a
`PoolResource<16, 8>` with a 16-byte first chunk, consumes that chunk, then
fails the next ordinary allocation while allowing the aligned chunk allocation
to succeed. The test catches `std::bad_alloc` from the list-node construction
and counts aligned chunk allocations versus aligned deallocations at resource
destruction.

The pre-fix probe was compiled with:

`c++ -std=c++20 -O0 -g -I src -I /data/my_storage/tmp/cycle243-build/src /data/my_storage/tmp/cycle266_pool_allocate_failure_probe.cpp src/util/check.cpp src/clientversion.cpp -o /data/my_storage/tmp/cycle266_pool_allocate_failure_probe && /data/my_storage/tmp/cycle266_pool_allocate_failure_probe`

It exited `134` at the final allocation/deallocation equality assertion. The
patched source exits `0` with the same deterministic fault schedule. This is
an independent ownership oracle: it does not depend on allocator RSS, pool
statistics, or a test-only production hook.

### Fix and validation

`AllocateChunk()` now inserts the aligned storage pointer into the owned list
before publishing the available-memory cursor. If the list node cannot be
allocated, a catch block calls the matching aligned `operator delete` and
rethrows. Once list insertion succeeds, the remaining initialization is
nothrow and the existing sanity checks remain unchanged.

Validation completed:

- `git diff --check` passed.
- `CCACHE_DISABLE=1 TMPDIR=/data/my_storage/tmp/cycle266-build-tmp cmake --build /data/my_storage/tmp/cycle243-build --target test_bitcoin -j2` passed. The existing two unrelated Clang `txgraph.cpp` unused-member warnings remained.
- `test_bitcoin --run_test=pool_tests,allocator_tests` passed 11 cases and 21,747 assertions.
- `test_bitcoin --run_test=coins_tests,mempool_tests` passed 63 cases and 1,219,050 assertions.
- `test_bitcoin --run_test=validation_chainstate_tests,validation_chainstatemanager_tests,validation_flush_tests` passed 29 cases and 60,104 assertions, including `/data`-backed scratch temp directories and cache rebalancing/restart paths.
- The deterministic pre-fix/fixed scratch probe described above failed before the change and passed after it.

No sanitizer rebuild or full repository test suite was run in this cycle. The
probe validates cleanup of the aligned chunk allocation; it does not attempt to
make every possible `std::list` or aligned allocation failure site observable.
The fix is limited to the chunk ownership transition and does not alter pool
capacity, freelist rounding, or the fatal process-wide OOM policy.

### Verdict and next queue

Confirmed: a `std::bad_alloc` from `m_allocated_chunks.emplace_back()` leaked
the just-allocated aligned pool chunk and left the resource's cursor state
untracked. The ownership-order fix is ready for one self-contained commit with
this journal update.

Next distinct memory cells are (1) a post-fix persistent `/data` chainstate
resize/flush profile that measures allocator retention separately from database
cache ownership, (2) direct `PoolAllocator::allocate()` size multiplication and
constructor-overflow contracts if a reachable standard-container path is
found, and (3) a source/history review of any remaining `DynamicMemoryUsage()`
owner added after Cycle 263. Do not reopen the closed prevector OOM policy,
unbroadcast-set, or TxGraph container-capacity findings without new evidence.

## Cycle 263: retained TxGraph container accounting

- Goal: `74`, `memory-pressure-allocator`; exact selector `shuf -i 0-98 -n 1` -> `74`.
- Branch: `uber-cycle-263-memory-pressure-allocator-20260802`.
- Start HEAD: `fa8f1f6f860076385e83e41d7ebb1665565f0eab`; `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start `git rev-list --left-right --count HEAD...origin/master`: `1313 45`.
- Catalog/prompt/TSV/protocol hashes matched the stable gate: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- The tracked/index gate was clean at cycle start. Existing untracked goal, journal, probe, package, crash, and test-cache artifacts were preserved. Protected long-running tests were left running. Build and test scratch state used `/data/my_storage/tmp/cycle263-*`; no default datadir, wallet, key, or production database was used.

### Scope and ownership audit

This cycle continued the retained-container queue after Cycle 238's fix for
`TxGraphImpl::m_entries` capacity. The remaining main-graph ownership ledger
was:

- `m_cluster_usage` charges each live Cluster's dynamic data and one
  `sizeof(std::unique_ptr<Cluster>)` slot, but `ClusterSet::m_clusters` uses
  `pop_back()` and retains vector capacity after Cluster deletion.
- `m_main_clusterset.m_to_remove` is cleared after removals, but its capacity
  is retained. `m_unlinked` is also cleared after `Compact()` while retaining
  the allocation created by Ref destruction.
- `m_entries` was already covered by `memusage::DynamicUsage(m_entries)` from
  Cycle 238. The set-like `m_main_chunkindex` was already counted, and
  `m_main_chunkindex_discarded` is released by `ClearShrink()` before this
  query. Staging allocations and temporary group-operation state remain
  outside the documented main-graph estimate.

The source audit confirmed that `GetMainMemoryUsage()` counted only live
Cluster objects, the shared Entry vector, and the live chunk-index nodes. It
had no term for the three retained main-graph vector allocations above. This
underreported memory after transaction churn even when the live transaction
count and Entry-vector capacity were held constant.

### Reproduction and independent controls

The first regression constructs two graphs with the same retained Entry and
unlinked-index capacity by adding and aborting 2048 staging transactions. One
graph then keeps one main transaction. The other adds and removes 1024 main
transactions before keeping one. The latter retains a large per-quality
Cluster vector and a main removal-vector allocation, while its counted live
state is otherwise equivalent. Before the fix both reported `163992` bytes;
the fixed code reported `172224` versus `200912`.

The second regression constructs two graphs with 2048 live main transactions.
It removes and compacts 1024 transactions in only one graph, then adds 1024
replacement transactions. This keeps live count, Entry capacity, Cluster
vector capacity, and chunk-index cardinality aligned while retaining the
cleared removal and unlinked buffers only in the churned graph. Before the fix
both reported `442384` bytes; the fixed code makes the churned value larger.

As an independent mutation check, temporarily removing the new accounting
terms from `src/txgraph.cpp` and rebuilding made both regression assertions
fail: the cluster-container case reported `[163992 <= 163992]` and the
removal-buffer case reported `[442384 <= 442384]`. Restoring the terms made
both cases pass. This confirms the tests are sensitive to the intended
allocation classes rather than merely to live transaction count.

### Fix and validation

`GetMainMemoryUsage()` now counts retained `m_to_remove` and `m_unlinked`
vector allocations. It also walks the main per-quality Cluster vectors and
charges `DynamicUsage(capacity)` minus the raw pointer slots already included
by each live Cluster's `TotalMemoryUsage()`. This preserves the existing
allocator-rounding model without double-counting live vector elements.

Validation completed:

- `CCACHE_DISABLE=1 TMPDIR=/data/my_storage/tmp/cycle263-clean-build-tmp
  cmake --build /data/my_storage/tmp/cycle243-build --target test_bitcoin -j2`
  passed. The existing build emitted only two unrelated Clang warnings about
  unused member functions.
- `test_bitcoin --run_test=txgraph_tests --log_level=message
  --report_level=short --color_output=false` passed 26 cases and 617
  assertions.
- `test_bitcoin --run_test=mempool_tests --log_level=message
  --report_level=short --color_output=false` passed 26 cases and 1013
  assertions, using `TMPDIR=/data/my_storage/tmp`.
- `git diff --check` passed after the source and test edits.

No sanitizer build, full repository test suite, or production RSS measurement
was run in this cycle. The result is an allocator-model accounting fix, not a
claim that `GetMainMemoryUsage()` is an exact process RSS measurement.

### Verdict and next queue

Confirmed: retained main TxGraph container allocations were omitted from the
reported main memory usage. The fix and focused regressions are ready for one
self-contained commit with this journal update.

Next distinct memory cells are (1) a fresh `/data`-backed chainstate/cache
resize and flush workload, (2) allocation-failure behavior outside the closed
prevector/OOM policy cell, and (3) a source/history review of any remaining
`DynamicMemoryUsage()` owner added after this commit. Do not reopen the fixed
Entry, unbroadcast-set, or TxGraph container-capacity omissions without new
evidence.

## Cycle 170 start

- Goal: `74`, `memory-pressure-allocator`; exact selector `shuf -i 0-98 -n 1` -> `74`.
- Branch: `uber-cycle-170-memory-pressure-allocator-20260730`.
- Start HEAD: `27a40f68419d290b117f499e3d9e6c4120a9f26f`; `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `git rev-list --left-right --count HEAD...origin/master`: `1121 42`.
- Catalog/prompt/TSV/protocol hashes: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Fresh gate: `git diff --check` passed. Existing untracked artifacts remain preserved. Unrelated test processes `777094` (`wallet_tests`) and `956381` (`util_tests`) were present and must not be stopped. Root storage remains constrained; use `/data/my_storage/tmp/cycle170-*` and do not claim a full-suite result unless it actually runs.
- Scope: continue the existing memory queue without reopening Cycle 53's prevector OOM policy cell or Cycle 88's receive-accounting cell. Prioritize (1) mempool/package admission and eviction with exact retained-byte and RSS accounting, (2) chainstate/index cache resize and rebuild retention after flush, then (3) `DynamicMemoryUsage()` omitted-capacity or double-counting candidates with an independent recomputation or mutation oracle.
- Required evidence: deterministic workload and trust boundary; independent retained-state/byte oracle; peak and post-workload RSS or allocator evidence separated from harness/sanitizer reservation; failure/cleanup/retry behavior where applicable; and a failing-before/passing-after regression or equivalent mutation result before any source commit.

### Cycle 170 evidence and verdict

#### Confirmed: unbroadcast transaction nodes were omitted from mempool usage

The primary trust boundary is a locally submitted wallet/RPC transaction that is
accepted into the resident mempool and then added to `m_unbroadcast_txids` by
`BroadcastTransaction`. The set is protected by the mempool mutex, owns one
`std::set` tree node for each tracked transaction, is removed on broadcast or
transaction removal, and is copied by `GetUnbroadcastTxs()`. Its allocation is
therefore retained Bitcoin-owned mempool state, not a transient RPC result.

Before this cycle, `CTxMemPool::DynamicMemoryUsage()` accounted for the indexed
transaction estimate, `mapNextTx`, `mapDeltas`, `txns_randomized`, the main
transaction graph, and `cachedInnerUsage`, but not `m_unbroadcast_txids`.
`getmempoolinfo` reports this same method as `usage`, and the chainstate cache
budget uses it to calculate unused mempool space. The omission meant the usage
estimate and shared cache budget were low by the set allocation while a local
transaction remained unbroadcast. This is not a claim that one late broadcast
registration bypasses every admission check: `BroadcastTransaction` adds the
set entry after acceptance. It is a confirmed retained-memory accounting error
for subsequent reporting, trimming, and cache-budget decisions.

History establishes that this was an incomplete accounting extension rather than
an intentional exclusion. Commit `89eeb4a333` ("[mempool] Track \"unbroadcast\"
transactions", 2020) added the set and its insertion/removal paths after the
existing accounting formula, without changing `DynamicMemoryUsage()`. Commit
`a7ebe48b94` later exposed `unbroadcastcount` in RPC and the 0.21 release notes,
while the `usage` description remained "Total memory usage for the mempool".
No later history searched in this cycle added a corresponding usage term.

The fix adds `memusage::DynamicUsage(m_unbroadcast_txids)` to the formula. The
regression test inserts one resident transaction, records usage before and after
`AddUnbroadcastTx`, compares the delta with the independent
`memusage::DynamicUsage(std::set<Txid>)` oracle, then removes the txid and checks
that usage returns exactly to the baseline. On this 64-bit build the one-node
oracle is 80 bytes; that is the allocator model used by the project, not an RSS
claim. A large local submission set therefore creates an equally large omitted
estimate before the fix (for example, 100,000 nodes model 8,000,000 bytes).

The exact pre-fix mutation was removal of only the new accounting term. The
incremental GCC build passed, but the focused regression failed with
`after - before == expected` reported as `[0 != 80]` (exit code 201). Restoring
the term and rebuilding made the focused test pass with 1 case and 2
assertions. The full GCC `mempool_tests` suite then passed 25 cases and 425
assertions. An existing independent Clang 19 Debug build with
`-fsanitize=undefined,alignment,object-size` was rebuilt for the changed
production/test objects; its full mempool suite also passed 25 cases and 425
assertions with `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

Commands and scratch locations:

- GCC RelWithDebInfo build: `/data/my_storage/tmp/cycle170-mempool-build`,
  with scratch ccache `/data/my_storage/tmp/cycle170-ccache`.
- Focused run: `env TMPDIR=/data/my_storage/tmp/cycle170-mempool-run
  /data/my_storage/tmp/cycle170-mempool-build/bin/test_bitcoin
  --run_test=mempool_tests/MempoolUnbroadcastMemoryUsage
  --log_level=test_suite --report_level=short --color_output=false`.
- Full GCC run: the same binary with `--run_test=mempool_tests
  --log_level=message --report_level=short --color_output=false`.
- Independent UBSan run: `/data/my_storage/tmp/cycle106-clang19-ubsan/bin/test_bitcoin`
  with the same full mempool selector and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
- The required `git diff --check` passed. No default datadir, wallet, key, or
  production database was used.

#### Dismissed: chainstate cache resize and allocator retention

The source audit traced `CCoinsViewCache::DynamicMemoryUsage()` to the
`CCoinsMap` pool-resource allocation plus `cachedCoinsUsage`. `CCoinsViewCache::Flush`
can call `ReallocateCache()` after the map is empty, and that routine destroys
and reconstructs both the map and its memory resource specifically to release
allocator-retained chunks. `Chainstate::ResizeCoinsCaches()` flushes with
`FORCE_FLUSH` when the coinstip cache shrinks and keeps the cache when it grows.
Current tests explicitly cover upsize retention, downsize eviction, exact map
and coin usage recomputation, and the documented fact that clearing a pool-backed
map does not necessarily reduce its dynamic usage. The existing rebuild and
cache profile evidence in prior cycles also found no reproducible state,
retention, or accounting defect. This cell remains queued for a fresh
`/data`-backed repeated workload, but no source change is justified from the
current audit.

#### Dismissed: other dynamic-usage omissions or double counting

The transaction graph documentation explicitly excludes staging operations,
block builders, queued operations, and temporary caches from its main-graph
estimate; those are not resident committed mempool state and were not treated
as defects. The coins cache has an independent recomputation in both production
sanity checks and `coins_tests`; disconnected-transaction accounting includes
its inner usage, map, and list ownership and asserts an empty destructor state.
The network receive accounting cell was already closed in Cycle 88 and was not
reopened. No second omission survived the source, ownership, and independent
oracle review in this cycle.

#### Limitations and next queue

Evidence was executed on x86_64 Linux with GCC 12.2 RelWithDebInfo and Clang
19.1.7 Debug UBSan. The full current-tree unit suite was not run because `/`
had about 111 MiB free and was at 99%; the focused production suite and the
independent sanitizer mempool suite completed. Existing unrelated test
processes 777094 (`wallet_tests`) and 956381 (`util_tests`) were preserved.

Next distinct cells:

1. Exercise package admission and eviction with a fixed operation sequence,
   comparing staged changeset peak ownership with an independent allocation
   ledger and reported usage; do not reopen the unbroadcast-set omission.
2. Run repeated chainstate cache resize/flush cycles on `/data` scratch state,
   separating allocator retention from cache ownership and RSS noise.
3. Search other `DynamicMemoryUsage()` implementations for newly added owned
   containers, using history and a temporary mutation or recomputation oracle.

## Cycle 53

- Goal: `74`, `memory-pressure-allocator`.
- Branch: `fuzz-contract-cluster-oracles-20260709`.
- Base: `origin/master` `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`.
- Gate HEAD: `b1b13507a7d128c45211657708565af00d00d7ab`.
- Selector: `shuf -i 0-98 -n 1` -> `74`.
- Scope: heap/RSS, allocation lifetime, fragmentation, cache and queue bounds, allocator-failure behavior, and graceful failure under deterministic pressure.
- Host limits: 15 GiB RAM, no swap, `/data` had 40 GiB free at the start and `/` was 100% full with about 69 MiB available. All scratch state used `/data/my_storage/tmp/cycle53-prevector-oom`; no default datadir, wallet, key, or production database was used.

## Inventory and Prior Evidence

The source inventory found:

- `src/support/allocators/secure.h` uses `LockedPoolManager`; a failed locked allocation returns `nullptr` and is converted to `std::bad_alloc`.
- `src/test/allocator_tests.cpp` exercises finite-arena exhaustion, locked-pool exhaustion, invalid sizes, reuse, disjoint allocations, and memory-stat restoration.
- `src/prevector.h` uses `malloc`/`realloc` and asserts allocation success. The comment at lines 140-142 is an explicit 2017 FIXME explaining that these C allocators bypass `new_handler` and that the current behavior is intentional termination rather than a throwing allocator.
- `src/serialize.h` applies `ReadCompactSize`'s `MAX_SIZE` of `0x02000000` (33,554,432) and reads byte `prevector` data in blocks of roughly 5 MiB. The block size limits a single read, but the total object remains bounded by `MAX_SIZE` and normal process memory policy.
- `src/init.cpp:new_handler_terminate()` installs `std::terminate` as the new handler and documents immediate termination on OOM to avoid continuing in a possibly inconsistent chain state. This is the relevant process-wide policy for a daemon, and it explains why `src/rpc/protocol.h`'s `RPC_OUT_OF_MEMORY` is currently not used as a recovery path.
- Existing closed memory cells were excluded: cache-arithmetic overflow and cache-resize propagation, orphanage global/per-announcement accounting, wallet failure-state cleanup, pool-resource contracts, and vector-deque allocation/retention contracts.

The main hypothesis was that `prevector` was an inconsistent boundary: an untrusted serialized size could make `malloc` or `realloc` return null, and the assertion could either abort a daemon or leave an invalid pointer in an assertion-disabled build. The independent policy check was whether Bitcoin Core deliberately treats process-wide OOM as fatal, making a local `std::bad_alloc` conversion incorrect for chainstate and validation callers.

## Baseline and Deterministic OOM Evidence

Baseline command:

`env TMPDIR=/data/my_storage/tmp/cycle53-prevector-oom build_unit_clang19/bin/test_bitcoin --run_test=prevector_tests,streams_tests --catch_system_error=no --log_level=test_suite`

Result: exit 0. The three prevector cases and the selected stream cases passed; the complete selected invocation ran 30 cases and ended with `*** No errors detected`.

A disposable probe included `prevector.h`, set `RLIMIT_AS` to 128 MiB, and attempted a 200 MiB resize. It was compiled with:

`c++ -std=c++20 -O0 -g -I src agent-journal/prevector_oom_probe.cpp -o /data/my_storage/tmp/cycle53-prevector-oom/prevector_oom_probe`

The direct-allocation mode exited 134 with:

`prevector_oom_probe: src/prevector.h:148: ... Assertion 'new_indirect' failed.`

The growth/reallocation mode first created a 1 MiB prevector, set the same address-space limit, and attempted a 200 MiB resize. It exited 134 with:

`prevector_oom_probe: src/prevector.h:144: ... Assertion '_union.indirect_contents.indirect' failed.`

The second mode was designed to check that the old contents remained valid if a failed `realloc` were converted to an exception. The current implementation does not reach that contract because it terminates at the assertion. The probe was deleted after the experiment with no production or test source left modified.

This is a real, reproducible fatal allocation boundary, but it is not a new defect by itself. The daemon's explicit OOM policy is also fatal, and changing only `prevector` to throw would create a different policy for a low-level container used by scripts, transactions, network messages, and validation state. The network message loop catches `std::exception` only for expected deserialization failures and calls `Assume` on other exceptions. The RPC boundary maps generic exceptions to `RPC_MISC_ERROR`, not `RPC_OUT_OF_MEMORY`. A partial conversion would therefore be neither a complete graceful-failure design nor a demonstrated correctness improvement.

## Fuzz and Memory-Accounting Evidence

The qa-assets prevector corpus contains 650 files, 1,959,728 bytes total, with a 72,952-byte maximum input.

- Normal target:
  `env TMPDIR=/data/my_storage/tmp/cycle53-prevector-oom FUZZ=prevector build_fuzz_libfuzzer_clang19/bin/fuzz -runs=5000 /data/my_storage/tmp/qa-assets/fuzz_corpora/prevector`
  exited 0 after 56 seconds. It reached 925 coverage counters, 5,225 feature points, a 306-unit/354 KiB corpus, and reported 1,558 MiB RSS. No assertion, sanitizer, or crash diagnostic occurred.
- ASan target:
  `env TMPDIR=/data/my_storage/tmp/cycle53-prevector-oom FUZZ=prevector build_fuzz_asan_clang19/bin/fuzz -runs=3000 /data/my_storage/tmp/qa-assets/fuzz_corpora/prevector`
  exited 0 after 238 seconds. It reached 2,883 coverage counters, 17,535 feature points, a 309-unit/358 KiB corpus, and reported 1,558 MiB RSS. No ASan diagnostic occurred. The high fixed RSS is attributed to the sanitizer/fuzzer process reservation and was not treated as production container usage.
- Combined focused memory/accounting command:
  `env TMPDIR=/data/my_storage/tmp/cycle53-prevector-oom build_unit_clang19/bin/test_bitcoin --run_test=allocator_tests,pool_tests,net_tests,mempool_tests,coins_tests --catch_system_error=no --report_level=short`
  exited 0. It passed 102 cases and 1,401,414 assertions. This covered finite arena exhaustion, locked-pool failure, pool reuse, network queued-message accounting, mempool trimming/accounting, and coin-cache usage invariants.

## Verdict

No new source defect was confirmed. The prevector probe demonstrates a real process-terminating OOM path, but current history and `new_handler_terminate()` establish that process termination is the repository's deliberate chain-safety policy. A throwing prevector would require a coordinated contract for validation, peer-message, RPC, and top-level error handling, plus a safe rule for partially mutated deserializations; no such contract exists in this cycle. The normal and sanitizer fuzz runs and the independent allocator/accounting suite found no memory corruption, bound bypass, accounting drift, or cleanup failure.

No production commit is justified. This cycle is a journal-only handoff. Do not reopen the prevector assertion as a source finding without a new caller contract, a release-build state-corruption demonstration, or a coordinated OOM policy change supported by project history.

## Rejected Hypotheses

1. `prevector` allocation failure should be converted locally to `std::bad_alloc`. Rejected as incomplete and inconsistent with the daemon's fatal OOM policy.
2. The 5 MiB deserialization block size prevents memory exhaustion. Rejected as an overstatement: it bounds each read but not the total `MAX_SIZE` allocation. No exploitable or new defect was demonstrated because that total is already the explicit serialization bound and OOM policy is fatal.
3. The 1,558 MiB fuzzer RSS proves a production container leak. Rejected; the normal and sanitizer processes reported the same fixed fuzzer reservation while corpus size stayed below 360 KiB, and no production workload or live daemon was involved.

## Next Queue

1. Measure network receive/send queue limits under fragmented, duplicate, and stalled peer workloads; compare tracked `GetMemoryUsage()` against actual retained buffers and pause/disconnect thresholds.
2. Stress mempool/package admission and eviction with fixed transaction sequences, checking peak RSS against `DynamicMemoryUsage()` and the configured `-maxmempool` bound.
3. Exercise chainstate and index cache resize/rebuild cycles with `/data`-backed scratch state, recording RSS, allocator retention, and post-flush release; exclude the already-closed cache failure-propagation path.
4. Audit `DynamicMemoryUsage()` implementations for omitted retained capacity or double-counted ownership, using a temporary mutation or independent recomputation oracle before considering a source fix.

Raw fuzz and test output was retained in the tool transcript; the scratch directory is `/data/my_storage/tmp/cycle53-prevector-oom`. The next cycle must repeat the branch/base/dirty/process/catalog gate, select a fresh goal with `shuf -i 0-98 -n 1`, and continue immediately.

## Cycle 88 start

- Goal: `74`, `memory-pressure-allocator`.
- Branch: `uber-cycle-88-memory-pressure-allocator-20260729`.
- Selector: exact `shuf -i 0-98 -n 1` -> `74`.
- Cycle-start HEAD: `0ab48d44a6e17500b05bf76a781254451192f65e`; `origin/master` is
  `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base is
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` is
  `2 964`.
- Catalog/protocol/manifest hashes match the prior gate. No relevant process is
  running. Known unrelated untracked agent/user artifacts and `test/cache/`
  remain outside this cycle.
- Prior Cycle 53's `prevector` allocation/OOM cell is closed and excluded.

### Scope and contract

This cycle audits retained network receive/send buffers and their memory
accounting under a local, deterministic peer workload. The target is not a
generic maximum-RSS comparison: establish the ownership and lifetime of each
buffer, the configured per-peer/global bounds, the pause/disconnect behavior,
and the relationship between `GetMemoryUsage()` or equivalent counters and
actual retained allocations.

Use scratch datadirs and a loopback-only harness. Exercise fragmented and
coalesced messages, duplicate/stalled peers, partial writes, backpressure,
disconnect/reconnect, and shutdown while bytes are queued. Record exact input
sizes, peer counts, queue limits, process RSS/heap counters, queue/accounting
values, and cleanup after each phase. Do not use public peers or an existing
datadir.

### Initial queue

1. Trace `CNode`/transport send and receive buffers, `CConnman` limits, pause
   thresholds, and `GetMemoryUsage()` callers. Identify whether capacity,
   queued payloads, allocator slack, and shared ownership are counted once.
2. Run a fixed local P2P transcript with fragmented headers/payloads, stalled
   readers/writers, duplicates, reconnects, and forced disconnects. Compare
   per-peer and global counters with an independent byte ledger and RSS/heap
   samples at each schedule point.
3. Repeat with multiple peers and the smallest message/queue limits that
   amplify any mismatch. Separate expected kernel socket buffers and fuzzer/
   harness memory from Bitcoin-owned retained state.
4. If a mismatch remains, inject the smallest relevant code mutation or use a
   recomputation oracle, then prove before/after behavior and cleanup. Do not
   change a bound or accounting formula based only on allocator noise.

Required evidence for a source finding: a deterministic sequence and exact
source-to-sink trace, an independent retained-byte or state oracle, a
failing-before/passing-after regression or equivalent mutation result, and
narrow/broad validation. If no source finding is justified, record the
measured limits, rejected hypotheses, raw artifact paths, and next unchecked
queue cell.

### Cycle 88 evidence and verdict

The source and history audit covered `CNode::ReceiveMsgBytes`,
`MarkReceivedMsgsForProcessing`, `PollMessage`, `GenerateWaitSockets`,
`SocketHandlerConnected`, `CConnman::PushMessage`, `SocketSendData`,
`V1Transport`, and `V2Transport`. The relevant ownership boundaries are:

- V1 rejects payloads above `MAX_PROTOCOL_MESSAGE_LENGTH` (4,000,000 bytes)
  before allocation. Its partial receive vector grows only to the received
  bytes plus at most 256 KiB, capped at the message size.
- V2 limits packet contents to the same protocol maximum plus its type
  encoding, limits the handshake/garbage states to 4,111 bytes, and reserves
  at most 256 KiB ahead of received packet data. Ciphertext and decoded
  contents are not retained across more than one application packet; the
  decoded buffer is cleared after extraction or rejection.
- `-maxreceivebuffer` is applied to the completed-message process queue, not
  to the transport's single partial packet. This is consistent with the
  2016 process-queue design rationale in `c6e8a9bcff`, while the transport
  hard caps address the separate per-connection deserialization allocation.
  The help text's broad phrase "receive buffer" is imprecise, but history and
  callers do not establish a changed user-facing contract, so no source or
  documentation change is justified in this cycle.
- The production socket reads at most 64 KiB before handing completed messages
  to the process queue. A message can exceed the flood threshold by its own
  bounded size, and a partial transport buffer is outside that threshold, but
  neither path is unbounded or bypasses its corresponding pause/cap rule.
- Send accounting includes queued serialized messages and the transport's
  current send buffer. The omitted transport fields are fixed-size or bounded
  handshake state by explicit design; the recent send-queue contract tests and
  assertions cover the non-empty queue transition and are outside this new
  receive-focused cell.

The focused accounting hypothesis was that `CNetMessage::GetMemoryUsage()`
double-counts its inline `DataStream` while `m_msg_process_queue_size` omits
the owning `std::list` node, potentially allowing the configured flood limit
to understate retained memory. A disposable layout probe compiled from the
current headers and linked with `src/support/cleanse.cpp` reported:

`sizeof_cnet_message=80`, `sizeof_datastream=32`,
`malloc_usage_cnet_message=96`

and, for payload sizes 0, 1, 65,536, and 1,048,576:

`get_memory_usage_formula` = `list_node_and_payload` = respectively
112, 144, 65,664, and 1,048,704 bytes.

Thus the apparent nested-object overcount exactly compensates for the list
node allocation on this supported 64-bit build, including vector allocation
rounding. A 32-bit build was not available (`bits/c++config.h` is missing for
`-m32`), so no portability claim is made. The probe object and source were
temporary and are not retained. No deterministic runtime mismatch, cleanup
failure, or bound bypass was demonstrated; no production commit is justified.

Exact evidence commands included:

- `git show d22a234ed2 -- src/net.cpp src/net.h`
- `git show 297c888997 -- src/net.cpp`
- `git show c6e8a9bcff -- src/net.cpp src/net.h`
- `c++ -std=c++20 -O2 -Isrc ...; /data/my_storage/tmp/cycle88-memory/probe`
- `c++ -m32 -std=c++20 -O2 -Isrc -c ...` (toolchain unavailable)

Verdict: dismissed for current supported host and current contracts. This is
a journal-only cycle close. Do not reopen the same receive-accounting cell
unless a 32-bit/alternative-allocator result, a changed transport contract,
or a concrete runtime retained-memory discrepancy supplies new evidence.

### Cycle 88 next queue

1. Stress mempool/package admission and eviction with fixed transaction
   sequences, comparing peak RSS with `DynamicMemoryUsage()` and
   `-maxmempool` while checking removal symmetry.
2. Exercise chainstate and index cache resize/rebuild cycles with scratch
   state, measuring allocator retention and post-flush release; exclude the
   already-closed cache failure-propagation path.
3. Audit `DynamicMemoryUsage()` implementations for omitted retained
   capacity or double-counted ownership using an independent recomputation
   oracle and a temporary mutation.

### Cycle 170 completion

- Branch: `uber-cycle-170-memory-pressure-allocator-20260730`. Start HEAD was
  `27a40f68419d290b117f499e3d9e6c4120a9f26f`; the source/test/journal commit
  is `a84b27f5c21b2c6b1cf7607a0699f0c002aa0651`, authored as
  `Lőrinc <pap.lorinc@gmail.com>`, with subject `mempool: account for
  unbroadcast transaction memory`.
- The confirmed defect was the omitted `m_unbroadcast_txids` tree-node memory
  in `CTxMemPool::DynamicMemoryUsage()`. The permanent regression compares the
  before/after delta with `memusage::DynamicUsage(std::set<Txid>)` and checks
  exact removal symmetry. Removing only the production term made the focused
  test fail with `[0 != 80]` and exit code 201; restoring it passed. The
  post-commit GCC rebuild with
  `CCACHE_DIR=/data/my_storage/tmp/cycle170-ccache` exited 0, the focused test
  passed 1 case and 2 assertions, and the full `mempool_tests` suite passed 25
  cases and 425 assertions. The independent Clang 19 UBSan/alignment/object-
  size run also passed 25 cases and 425 assertions. An earlier post-commit
  build wrapper without `CCACHE_DIR` failed only because `/root/.cache/ccache/tmp`
  was absent; it was rerun successfully with the scratch cache and is not a
  source failure.
- The chainstate cache and other dynamic-usage cells were dismissed for this
  cycle with source, history, existing recomputation tests, and documented
  ownership exclusions. The next memory queue remains package admission and
  eviction, repeated chainstate resize/flush retention, and newly added owned
  containers in other usage functions. Do not reopen the fixed unbroadcast
  cell, Cycle 53's prevector OOM policy, or Cycle 88's receive accounting
  without a distinct contract or new evidence.
- Close gate after `git fetch origin master`: HEAD was
  `a84b27f5c21b2c6b1cf7607a0699f0c002aa0651`, `origin/master` was
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base was
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and
  `git rev-list --left-right --count HEAD...origin/master` returned
  `1122 42`. `git diff --check` passed. The only worktree changes after the
  source commit were this handoff and the state ledger; known untracked goal,
  journal, package, and test-cache artifacts were preserved. PIDs 777094
  (`wallet_tests`) and 956381 (`util_tests`) remained alive. `/` had 110 MiB
  free at 99% and `/data` had 50 GiB free at 95%; no full current-tree suite
  result is claimed.
- Catalog/prompt/TSV/protocol hashes at the gate were respectively
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Verdict: confirmed and fixed. The first next selector command returned `53`
  (`statistical-timing`), which was rerolled because its existing timing cells
  are closed; the required reroll returned `80` (`fuzz-engine-differential`).
  A separate state-only close commit follows. The next run must perform a
  fresh gate and open `uber-cycle-171-fuzz-engine-differential-20260730`.

## Cycle 238 start

- Goal: `74`, `memory-pressure-allocator`.
- Selector: exact `shuf -i 0-98 -n 1` -> `74`.
- Branch: `uber-cycle-238-memory-pressure-allocator-20260731`.
- Cycle-start HEAD: `92738468b840942ce0c327aef20e80915365b046`; `origin/master` is
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base is
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `git rev-list --left-right
  --count HEAD...origin/master` returned `1259 42` before this cycle's
  source/test/journal changes.
- The catalog, random prompt, TSV, and uber-goal protocol hashes matched the
  stable gate: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
  The tracked/index gate was clean, and the protected long-running jobs were
  alive. Existing untracked goal, journal, package, crash, and test-cache
  artifacts were preserved outside this cycle.

### Scope and exclusions

This cycle continued the memory-pressure campaign after the closed prevector
OOM policy (Cycle 53), transport receive-buffer accounting (Cycle 88), and
the fixed unbroadcast-set accounting omission (Cycle 170). The initial queue
was package admission/eviction, changeset lifetime, expiry, persistence, and
retained container capacity. The package submission contract deliberately
permits temporary over-limit state, but requires a final `LimitMempoolSize()`
after the changeset is cleared.

The relevant ownership ledger was:

- `CTxMemPool::DynamicMemoryUsage()` counts map nodes, `mapNextTx`, fee
  deltas, the randomized transaction vector, the unbroadcast set, main
  `TxGraph` usage, and cached entry-owned allocations.
- `removeUnchecked()` removes unbroadcast membership, updates the randomized
  vector, decrements cached entry usage, and erases the map entry. The
  existing `mempool_tests` accounting and `txgraph` invariants cover these
  removal transitions.
- `ChangeSet` owns temporary staged entries, ancestor caches, and staging
  graph state. `Apply()` clears its staging containers; its destructor aborts
  staging. Package submission skips trimming only while this bounded temporary
  state exists, then clears it and trims before returning.
- `TxGraphImpl::GetMainMemoryUsage()` counted cluster objects and the chunk
  index, but represented `m_entries` as `sizeof(Entry) * live_count`. The
  vector is shared by main and staging and `Compact()` removes dead elements
  with `pop_back()` without reducing vector capacity. Therefore a churned
  graph could retain allocator memory that the mempool limit did not see.

### Package and lifecycle evidence

The source/history audit covered `ChangeSet::StageAddition`,
`StageRemoval`, `Apply`, `ClearSubPackageState`, `AcceptSingleTransaction`,
`AcceptPackage`, `LimitMempoolSize`, `TrimToSize`, and `removeUnchecked`.
Package policy remains bounded by `MAX_PACKAGE_COUNT = 25` and
`MAX_PACKAGE_WEIGHT = 404000`. The source contract and current test behavior
show that package submission may temporarily exceed `-maxmempool`, but the
final trim occurs after staging is gone and reports transactions that were
evicted. No package staging leak or post-trim limit bypass was found.

The deterministic runs passed:

- `txpackage_tests`: 15 cases, 302 assertions.
- pre-fix `mempool_tests`: 26 cases, 1013 assertions.
- `mempool_package_limits.py`, `mempool_limit.py`,
  `mempool_packages.py`, `mempool_package_rbf.py`, and `rpc_packages.py`.
  The limit test covered temporary package overage, immediate eviction, and
  the final `bytes < maxmempool` contract.
- `mempool_expiry.py`: default and custom expiry, recursive parent/child
  removal, and independent transaction retention.
- `mempool_persist.py`: persistence/reload and fee-delta preservation.

The post-fix `mempool_limit.py` rerun also passed. A concurrent post-fix
`mempool_tests` attempt was stopped after the fixture reported the known full
root filesystem condition; it did not produce a source failure. No claim is
made for a post-fix full mempool unit suite under that storage condition.

### Confirmed finding and independent reproduction

The focused `txgraph_tests/txgraph_memory_usage_accounts_for_retained_entries`
test builds a fresh one-entry graph and a second graph that adds 1024 entries,
removes them, clears their `Ref` objects, and adds one live entry. On the old
code both graphs reported `216` bytes (`churned_usage > fresh_usage` failed as
`216 <= 216`). This is a direct before/after test of the public memory-usage
estimate and does not depend on RPC transaction construction.

The fix replaces the live-count approximation with
`memusage::DynamicUsage(m_entries)`, which uses the vector's retained
capacity. The allocation is shared by main and staging and remains owned by
the graph, so it must be included even though staging-only cluster structures,
queued operations, and temporary caches remain outside this estimate. After
the fix the same test passed with 1 case and 2 assertions. The complete
`txgraph_tests` suite passed 24 cases and 613 assertions, and the fixed daemon
target rebuilt successfully. This closes a real memory-limit accounting
defect: repeated transaction churn could leave retained `TxGraph::Entry`
storage invisible to `-maxmempool` and `getmempoolinfo.usage`.

### Cycle 238 verdict and next queue

Verdict: confirmed and fixed in one source/test/journal commit. The package,
expiry, persistence, and removal-symmetry cells are dismissed for this cycle;
do not reopen them without a changed contract or new retained-state evidence.

Next unchecked cells:

1. Audit retained capacity in the remaining main-graph container indexes and
   other newly added `DynamicMemoryUsage()` implementations, separating core
   ownership from intentionally excluded staging/temporary state.
2. Exercise repeated chainstate/cache resize and flush cycles with a safe
   scratch filesystem once storage permits, comparing reported usage with an
   independent ownership ledger.
3. Recheck allocation-failure handling only with a distinct non-terminating
   contract; do not reopen the prevector fatal-allocation policy.
