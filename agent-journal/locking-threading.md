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
