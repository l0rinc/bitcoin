# Memory Pressure, OOM, Allocator, and Fragmentation Audit

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
