# Cycle 16: locking, threading, and scheduler audit

## Selection and gate

- Goal index: 8, `locking-threading`, selected with `shuf -i 0-98 -n 1`.
- Branch: `fuzz-contract-cluster-oracles-20260709`.
- Gate HEAD: `6d05b1cb6b7b06d25cda19be4ca22de010af3554`.
- Fresh upstream base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`.
- Worktree: no modified tracked files at investigation start; existing agent journals, probes, and `test/cache/` were preserved.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

## Scope map

Reviewed `m_nodes_mutex`, `m_reconnections_mutex`, `mutexMsgProc`,
`NetEventsInterface::g_msgproc_mutex`, the per-node socket/send/receive/message
locks, scheduler callbacks, worker shutdown, `ForNode`/`ForEachNode` callbacks,
and persistence calls reachable from network shutdown.

The `ForNode` and `ForEachNode` call sites consistently use the declared order
when callbacks access peer-manager state: callers hold `cs_main`, then the
callback acquires `m_nodes_mutex`. `DisconnectNodes()` accumulates reconnections
outside `m_reconnections_mutex` while holding `m_nodes_mutex`, and node deletion
happens after the node-list lock is released. `ThreadMessageHandler()` holds the
global message-processing lock while taking a node snapshot, but the snapshot
keeps references and releases them before node deletion. No reproducible lock
cycle was found in these paths.

Normal shutdown deliberately calls `CConnman::Stop()` before stopping the
`CScheduler`, because scheduled callbacks still reference the live connman. The
ordering keeps the object alive, but it leaves the scheduler able to run while
`StopNodes()` performs its shutdown persistence work.

## Confirmed finding

`CConnman::Start()` schedules `DumpAddresses()` every 15 minutes. `StopNodes()`
also calls `DumpAddresses()` before deleting nodes. Both paths reach
`SerializeFileDB("peers", peers.dat, addrman)`. `AddrMan::Serialize()` protects
the in-memory address tables with its own mutex, but `SerializeFileDB()` creates
a random 16-bit temporary filename, commits the file, and renames it into place
without a process-level write lock.

A valid schedule is therefore:

1. The scheduler serializes an older addrman snapshot and pauses before its final
   rename.
2. Shutdown serializes a newer snapshot and renames it into `peers.dat`.
3. The periodic writer resumes and renames the older snapshot over the newer one.

The final file is syntactically valid, but can lose addresses learned after the
periodic snapshot. Concurrent calls also have a 1-in-65536 temporary-name
collision opportunity. This is a persistence ordering defect, not an AddrMan
memory race; the source/dataflow proof is deterministic even though triggering
the exact filesystem timing is nondeterministic.

History search found no existing guard for this pair of callers. `banman.cpp`
and `node/mempool_persist.cpp` already use function-local dump mutexes for the
same temporary-file/rename pattern, which is the local precedent used here.

## Fix

Added a function-local `Mutex` in `CConnman::DumpAddresses()` so periodic and
shutdown peer-address dumps cannot overlap. This preserves the existing
shutdown order, file format, atomic rename behavior, and AddrMan locking.

## Verification

- `git diff --check`: passed.
- `cmake --build build_unit_clang19 --target test_bitcoin -j2`: passed.
- `build_unit_clang19/bin/test_bitcoin --run_test=net_tests --log_level=message --report_level=short`: 31/31 cases and 151,368/151,368 assertions passed.
- `cmake --build build_unit_tsan_clang19 --target test_bitcoin -j2`: passed.
- `build_unit_tsan_clang19/bin/test_bitcoin --run_test=net_tests --log_level=message --report_level=short`: 31/31 cases and 147,558/147,558 assertions passed.
- `build_unit_tsan_clang19/bin/test_bitcoin --run_test=net_tests/connman_stop_nodes_resets_network_connection_counts --log_level=test_suite --report_level=short`: 1 case and 4/4 assertions passed.
- Full `build_unit_clang19/bin/test_bitcoin --log_level=message --report_level=short` reached completion but was not clean: unrelated mempool saturation, validation signal ordering, and missing external-signer-support cases failed/aborted. No failure referenced `DumpAddresses()` or the changed code.

## Closed hypotheses

- `ForNode` callback lock inversion: dismissed after all production call sites
  and annotations were traced; the peer-manager callbacks use the established
  `cs_main -> m_nodes_mutex` order.
- Scheduler callback use-after-free: dismissed for normal node shutdown because
  `connman->Stop()` precedes `scheduler->stop()` and `connman` is reset only
  after the scheduler joins.
- Startup incompatibility failure leaving unjoined workers: inconclusive as a
  standalone defect; the normal initialization failure path invokes
  `Shutdown()`, which interrupts and joins connman workers. It remains a review
  surface for a future lifecycle cycle.

## Handoff

The code fix and this journal must be committed together. Next cycle should
re-check the fresh base and dirty state, then draw a new goal without repeating
the closed hypotheses. Preserve the full-suite failure details as residual
environment/test evidence rather than attributing them to this change.

# Cycle 35: locking, threading, and scheduler audit

## Selection and gate

- Goal index: 8, `locking-threading`, selected with `shuf -i 0-98 -n 1`.
- This is a distinct follow-up cell from cycle 16: the earlier `DumpAddresses()`
  persistence race is closed and was not reopened. The cycle-35 scope was
  transaction-download/inventory versus mempool locking, V2 transport lock
  nesting, and scheduler callback lifetime.
- Gate HEAD: `722f659965ab807b7effc2b2dca20951ccf9d79f`.
- Fresh upstream base: `origin/master` at
  `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`.
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence was
  `origin/master...HEAD = 2 841`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Corrected catalog validation reported `validated_rows=100 total_lines=100`.
- Tracked/staged state was clean at the gate; existing agent artifacts and
  `test/cache/` were preserved. No relevant test, fuzz, sanitizer, daemon, or
  build process remained after verification.

## Scope and static evidence

The current declarations put `m_tx_download_mutex` and
`m_inv_to_send_mutex` before `m_mempool.cs`. `ProcessInvBacklog()` follows the
declared inventory-to-mempool order, but releases the mempool lock before it
gets the peer snapshot and per-peer inventory locks. `ProcessValidTx()` holds
the transaction-download lock while calling short-lived mempool accessors and
does not retain `m_mempool.cs` across `InitiateTxBroadcastToAll()`. The INV/TX
message paths use `cs_main -> m_tx_download_mutex`, and no production path
held `m_mempool.cs` while acquiring either transaction-download or global
inventory state. The apparent reverse edge was therefore a non-nested sequence,
not a lock cycle.

V2 transport annotations and implementations agree: receive processing may
take `m_recv_mutex` before `m_send_mutex`, while send-only and receive-only
operations take one side. Socket I/O takes the short `m_sock_mutex` scope and
does not retain it while transport callbacks process bytes. `ForEachNode()` and
reconnection handling retained their already documented orders and were not
reopened as cycle-16 hypotheses.

Scheduler inspection confirmed that production shutdown stops connman before
stopping the scheduler, and the scheduler joins its service thread before
`PeerManagerImpl` or `CConnman` are destroyed. The test-only callback that calls
`scheduler.stop()` runs on a thread that is not registered as the scheduler's
service thread; it is not evidence of a production self-join path.

## Independent runtime evidence

- Rebuilt `build_unit_tsan_clang19/bin/test_bitcoin` with TSan, lock-order
  diagnostics, and `-Wthread-safety` after CMake regeneration.
- `build_unit_tsan_clang19/bin/test_bitcoin --run_test=net_tests --log_level=message --report_level=short` passed 31 enabled cases and 135,024/135,024 assertions; 1,159 cases were skipped.
- With the repository TSan suppressions and `halt_on_error=1`,
  `build_unit_tsan_clang19/bin/test_bitcoin --run_test=scheduler_tests --log_level=message --report_level=short` passed 4 enabled cases and 27/27 assertions; 1,186 cases were skipped.
- The custom fuzz driver was invoked with one existing
  `process_message` corpus seed:
  `TSAN_OPTIONS='suppressions=/data/my_storage/bitcoin/test/sanitizer_suppressions/tsan:halt_on_error=1:second_deadlock_stack=1' FUZZ=process_messages build_fuzz_tsan_clang19/bin/fuzz /data/my_storage/tmp/qa-assets/fuzz_corpora/process_message/af3035aea427dc17f1f45866ad8c1641a13e8b81`
  and returned `process_messages: succeeded against 1 files in 0s.` with no
  TSan report.
- An earlier attempt using `-runs=100` was classified as a harness invocation
  error: this build's `src/test/fuzz/fuzz.cpp` accepts files/directories or
  stdin, so the flag was treated as an input path. It did not exercise the
  target and is not a product finding.
- `git diff --check` passed. No race, deadlock, lock-order, or lifetime defect
  was reproduced.

## Verdict and handoff

The transaction/mempool, V2 transport, and production scheduler-lifetime
hypotheses are dismissed for this cycle. No source change is justified. The
remaining locking risk map prioritizes lock contracts around callback-owned
peer state, test-only scheduler lifecycle assumptions, and platform-specific
socket shutdown schedules, while excluding the closed `DumpAddresses()` and
`ForEachNode()` cells. The next cycle must re-check branch/base/HEAD, dirty
state, processes, catalog hashes, journals, history, and review precedent,
then draw a new goal from the full validated catalog.
