# Cycle 276: concurrent ThreadPool shutdown ownership

## Fresh gate and selection

- `git fetch origin master` passed before branch creation. The exact selector
  `shuf -i 0-98 -n 1` drew goal `8`, `locking-threading`; no reroll was used.
- Branch: `uber-cycle-276-locking-threading-20260802`.
- Gate HEAD: `ae3e461186f61b061d0edd4350793a273a32cc4c`; `origin/master`:
  `556988790a7f961693a8fd93f73725baea66476a`; merge base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence:
  `1341 45` (`HEAD...origin/master`).
- The tracked/index state was clean, `git diff --check` passed, the state file
  hash was `89d1b4626a852098a80dd70a9d9b2822d4074643436947d0812cab50fb973e0d`,
  and the catalog, prompt, TSV, and protocol hashes matched the persistent
  values. The protected long-running processes were alive and untouched.

## Distinct scope and hypothesis

The exact validation-callback re-registration cell from Cycle 210, the
SignalInterrupt lost-wakeup cell from Cycle 189, the mapport `std::thread`
lifecycle cell from Cycle 163, and the deferred scheduler callback lifetime
cell from Cycle 235 remain excluded. The new cell was the fixed-size
`ThreadPool` controller lifecycle in `src/util/threadpool.h`, focusing on two
controller threads calling `Stop()` concurrently while a worker is still
executing.

The public lifecycle contract requires `Stop()` to be called from a controller
(non-worker) thread and says it waits for all queued work to complete. The
implementation swapped `m_workers` into a local vector before joining. A second
controller therefore saw an empty worker vector, independently set
`m_interrupt`, returned after an empty drain/join, and reset `m_interrupt` to
false while the first stopper was still joining the original worker. The first
worker could then finish its task and return to its wait predicate instead of
exiting. The existing Start-vs-Stop test covered one stopper but did not cover
Stop-vs-Stop ownership.

## Independent pre-fix reproduction

The new `threadpool_tests/concurrent_stop_waits_for_first_stop` test starts one
worker, blocks it on a counting semaphore, starts a first stopper, waits until
that stopper has swapped the worker list, and starts a second stopper. Before
the fix, the second stopper returned during the first stopper's join phase, so
the assertion

`second_stop_finished.wait_for(500ms) == std::future_status::timeout`

failed in the Clang 19 ASan+UBSan build at
`/data/my_storage/tmp/cycle274-asan-wallet/bin/test_bitcoin` with:

`ASAN_OPTIONS=detect_leaks=0 /data/my_storage/tmp/cycle274-asan-wallet/bin/test_bitcoin --run_test=threadpool_tests/concurrent_stop_waits_for_first_stop --report_level=short --catch_system_errors=no`

The test then hung after releasing the worker because the early second Stop
had cleared `m_interrupt`; the first stopper's worker went back to its
condition-variable wait and could not be joined. That intentionally failing
process was terminated with Ctrl-C after the first assertion failure. This is
a direct lifecycle witness, not a timing-only inference.

## Repair and verification

The fix adds `m_stopping`, guarded by `m_mutex`, as shutdown ownership state.
The first controller entering `Stop()` sets it before swapping workers and
keeps it set through queue draining, worker joins, the empty-queue assertion,
and the `m_interrupt` reset. A concurrent controller waits on the existing
condition variable until that state clears and then returns. The final state
transition notifies all waiters. `Start()` remains rejected because
`m_interrupt` stays true for the entire first shutdown. The public comment now
states the concurrent Stop contract explicitly.

Verification results:

- The repaired ASan+UBSan build passed the focused test: 1 case and 4
  assertions. The complete `threadpool_tests` suite passed 18 cases and 461
  assertions.
- An independent Clang 19 UBSan build at
  `/data/my_storage/tmp/cycle273-clang19-ubsan` passed the focused test (1/4)
  and the complete suite (18/461). Its build emitted only the pre-existing
  `-O0` object-size-sanitizer warning.
- A separate Clang 19 ThreadSanitizer build at
  `/data/my_storage/tmp/cycle163-tsan` was rebuilt after reconfiguration and
  passed the focused test (1/4) with
  `TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1`; the complete suite
  passed 18 cases and 462 assertions with no TSan report.
- `git diff --check` passed. No full repository suite was run; the evidence is
  scoped to the changed lifecycle and its direct ThreadPool callers/tests.

## Verdict and handoff

**Confirmed and fixed**: concurrent controller-thread `Stop()` calls could
return early and clear the shutdown predicate, leaving the first stopper
deadlocked and permitting `Start()` during an unfinished shutdown. The source,
regression test, and this journal section belong in one independent commit,
authored as `Lőrinc <pap.lorinc@gmail.com>`. The next Goal 8 cycle should use a
new callback, worker, socket, or scheduler lifecycle cell and must not reopen
this Stop ownership race without a changed contract or recurrence evidence.

# Cycle 210: validation callback re-registration boundary

## Fresh gate and selection

- Goal index: 8, `locking-threading`; exact selector `shuf -i 0-98 -n 1` -> `8`, with no reroll.
- Branch: `uber-cycle-210-locking-threading-20260731`.
- Start HEAD: `62cca938b35d3a260fbe57dcf08645e8c30042f7`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1211 42`.
- Catalog, prompt, TSV, and protocol hashes matched the persistent uber-goal
  state. The tracked worktree and index were clean, `git diff --check` passed,
  and the four preserved long-running test processes were alive and untouched.

## Scope and source trace

The prior Goal 8 cells closed the address dump rename race, transaction-download
and V2 transport lock order, normal scheduler shutdown lifetime, callback-owned
peer state, mapport thread lifetime, and the `SignalInterrupt` lost-wakeup
interleaving. The fresh cell was the validation callback registry, specifically
registration and unregistration while `ValidationSignalsImpl::Iterate()` is
dispatching an event.

`CValidationInterface` promises that each subscriber receives events in
generation order and that one callback completes before the next callback for
that subscriber. `Iterate()` protected its list only while selecting and
advancing entries, then released `m_mutex` around the user callback and walked
the live `std::list`. A callback that unregistered itself removed the map entry,
then re-registered itself appended a new list entry. After the old callback
returned, the live iterator reached that new entry and dispatched the same
event again. Repeating the transition could keep the traversal alive
indefinitely. This is distinct from the already-reviewed production lock
serialization: synchronous validation signals are reached under `cs_main`, and
background signals use `SerialTaskRunner`; this cycle did not claim a separate
cross-source concurrent-callback defect.

## Reproducer and fix

Added `validationinterface_tests/register_during_callback_is_deferred` with a
subscriber that unregisters and re-registers itself during the first
`BlockChecked` callback. The pre-fix oracle temporarily removed only the
generation condition from the loop. The test then observed 2 callbacks for the
first event and 3 total after the second, failing with exit code 201. This
proves the duplicate delivery is caused by the live-list traversal rather than
by test scheduling.

Each newly created `ListEntry` now receives a monotonically increasing
generation under `m_mutex`. `Iterate()` snapshots the current generation and
stops before entries registered during that dispatch. The re-registration
remains active and receives the next event, while the existing reference-count
and shared-lifetime behavior is unchanged.

## Verification

- Release build: `env TMPDIR=/data/my_storage/tmp/cycle210-build-tmp CCACHE_DIR=/data/my_storage/tmp/cycle210-ccache cmake --build /data/my_storage/tmp/cycle105-clang19-release --target test_bitcoin -j2` passed.
- Repaired focused suite: `validationinterface_tests`, 7 cases and 43 assertions passed.
- `validation_block_tests`, 8 cases and 3,071 assertions passed. Its first
  invocation used a missing `TMPDIR` and was stopped after fixture setup
  errors; the rerun created the scratch directory and passed.
- Clang 19 TSan rebuild of `/data/my_storage/tmp/cycle163-tsan` passed, and the
  focused validation-interface suite passed 7 cases and 45 assertions with no
  TSan report.
- `git diff --check` passed. No default datadir, wallet, key, or production
  database was used.

## Verdict and handoff

**Confirmed duplicate validation callback on in-flight re-registration; fixed.**
The source, regression test, and this journal section are committed together.
The next Goal 8 cycle should use a fresh callback or worker lifecycle cell. A
separate hypothesis about overlap between synchronous and queued callbacks was
not promoted: synchronous call sites are under `cs_main` and queued events are
serialized, but a full cross-source happens-before guarantee was not established.
Reopen it only with an independent stateful callback witness or new caller.

# Cycle 189 start: SignalInterrupt reset/handler wakeup contract

## Fresh gate and selection

- `git fetch origin master` succeeded. The exact selector was
  `shuf -i 0-98 -n 1` -> `8`, `locking-threading`; no reroll was needed.
- Dedicated branch: `uber-cycle-189-locking-threading-20260731`. Start HEAD:
  `69be8957de541093a7c182b7363e81a4e613f3cd`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `1168 42`.
- Catalog SHA-256:
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
  Prompt SHA-256:
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
  Corrected TSV SHA-256:
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
  Protocol SHA-256:
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
  Uber-goal state SHA-256 at the gate:
  `0bb91377d55760f26624d45e40a9395a5f1c711a820a038b09900dde24e93560`.
- The TSV has one header and 99 four-field records, IDs 0 through 98 exactly
  once. Tracked files were clean at the gate; known unrelated untracked
  artifacts were preserved. PIDs `777094` and `956381` were alive and were
  not touched.

## Distinct cell and working hypothesis

Prior Goal 8 cells closed the `DumpAddresses()` periodic/shutdown rename race,
transaction-download versus mempool and V2 transport lock-order hypotheses,
normal scheduler-lifetime review, callback-owned peer state, and the mapport
`std::thread` lifecycle race. This cycle targets a separate process-wide
interrupt state machine: `util::SignalInterrupt` and the GUI retry path that
calls `SignalInterrupt::reset()` after a shutdown request.

Trust boundary: an asynchronous POSIX SIGTERM/SIGINT handler or Windows
console callback sets the shutdown event while the main/UI thread resets the
event to retry chainstate initialization. The contract is that an interrupt
that arrives after reset begins must remain observable by `operator bool()` or
by a wake token; reset must not lose a shutdown request.

Working hypothesis: on Unix, `reset()` calls `wait()` while `m_flag` remains
true and clears it only afterward. `operator()`, which is intentionally safe
for a signal handler, uses `m_flag.exchange(true)` and writes a pipe token only
when the previous value was false. A signal between reset's token drain and
final store can therefore observe true, write no token, and then be erased by
reset. The relevant production sequence is the SIGTERM handler in
`src/init.cpp`, the `g_shutdown` object, and the retry reset at
`src/init.cpp:1914`; this is distinct from ordinary thread interruption.

## Cycle 189 completion

## Contract and independent source trace

The relevant implementation was unchanged at the gate:

- `SignalInterrupt::operator()` is deliberately usable from the POSIX signal
  handler. It atomically changes `m_flag` from false to true and writes one
  byte to the token pipe only when the prior flag was false. This coalesces
  repeated shutdown requests and avoids condition-variable operations in a
  signal handler.
- Unix `SignalInterrupt::reset()` previously called `wait()` while
  `m_flag` remained true. `wait()` consumed the old pipe token, then reset
  stored false after returning. Windows `reset()` similarly waited under the
  condition-variable helper and stored false after the helper released its
  mutex.
- `src/init.cpp` installs `HandleSIGTERM`/`HandleSIGINT`, whose only action is
  `g_shutdown.operator()()`. `InitContext()` publishes that object to the
  node, and the GUI retry path at `src/init.cpp:1914` calls
  `node.shutdown_signal->reset()` before retrying chainstate initialization.

The lost-event schedule is deterministic at the state-machine level. Start
with one pending shutdown token (`m_flag == true`, one `x` byte in the pipe).
The retry thread enters `reset()` and consumes that byte while leaving the
flag true. A SIGTERM handler then calls `operator()`: its exchange observes
true, so it writes no new byte. Reset stores false. The subsequent
`ShutdownRequested()` check sees false and the shutdown request is lost. A
signal arriving between the old flag check and the token read has the same
outcome. On Windows, a console callback can set the flag after the wait helper
unlocks and before the old reset store clears it. This is a wakeup/lifecycle
defect, not a TSan-detectable data race.

History confirms the contract rather than introducing it: the class was added
as a signal-safe interrupt in `e2d680a32d7`, and its non-throwing reset/operator
API was established by `1d92d89edb`. The current comments explicitly require
signal-handler safety. The prior Goal 8 cells on peer locks, scheduler
lifetime, mapport thread ownership, and address-file serialization do not
cover this process-wide interrupt state machine.

## Fix

On Unix, `reset()` now atomically exchanges the flag to false before draining
the old token. Any signal after that linearization point sees false, publishes
a token, and leaves the flag true; the drain consumes only the old token, so
the new shutdown remains observable. If no interrupt was pending, reset
returns without reading the pipe. On Windows, reset clears the flag while
holding the same mutex as `operator()`, preventing a concurrent console
callback from being cleared after reset's synchronization point. The normal
single-thread reset behavior and return-value handling are preserved.

Added `util_tests/signal_interrupt_reset_drains_pending_event`, which checks
the pending-event drain, false postcondition, and reuse of the interrupt for a
second event. The test intentionally does not attempt to time an actual
signal-handler interleaving; the exact lost-event schedule is established by
the atomic/pipe state trace above, while timing stress would be flaky.

## Verification

- `git diff --check`: passed.
- UBSan/alignment/object-size Clang 19 build:
  `CCACHE_DIR=/data/my_storage/tmp/cycle189-ccache cmake --build /data/my_storage/tmp/cycle106-clang19-ubsan --target test_bitcoin -j2` passed.
- Focused UBSan test with `TMPDIR=/data/my_storage/tmp/cycle189-ubsan-runtime`:
  1 case and 8 assertions passed.
- Full UBSan `util_tests`: 81 cases and 4,002 assertions passed.
- TSan Clang 19 build:
  `CCACHE_DIR=/data/my_storage/tmp/cycle189-ccache cmake --build /data/my_storage/tmp/cycle163-tsan --target test_bitcoin -j2` passed.
- Focused TSan test with
  `TSAN_OPTIONS='halt_on_error=1:exitcode=66:report_signal_unsafe=0'`: 1 case
  and 8 assertions passed with no report.
- Full TSan `util_tests`: 81 cases and 4,002 assertions passed with no report.

The first focused invocation before creating its `TMPDIR` failed in the test
fixture with `filesystem_error: temp_directory_path`; it was an environment
setup failure and was rerun successfully after creating the `/data` directory.
No Windows or signal-handler execution environment was available, so those
branches are covered by source inspection and the platform-specific locking
proof, not by native runtime output.

## Verdict and handoff

**Confirmed interrupt reset lost-wakeup race; fixed locally.** The smallest
correct change is confined to `src/util/signalinterrupt.cpp`, with the focused
contract test in `src/test/util_tests.cpp`. The next locking cycle should
prioritize a fresh worker or callback state machine, and may revisit
SignalInterrupt only with new evidence about pipe failure or reset lifecycle;
do not reopen this lost-event interleaving.

## Cycle 16: locking, threading, and scheduler audit

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

# Cycle 98: locking, threading, and scheduler audit

## Selection and gate

- Goal index: 8, `locking-threading`, selected with `shuf -i 0-98 -n 1` -> `8`.
- Branch: `uber-cycle-98-locking-threading-20260729`.
- HEAD at cycle start: `3eee5aaf5cb803debc96e31268c5d4d56ade1258`.
- `origin/master`: `9b38d077f894d27ea76413b1db1cb040e25dc296`.
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Start divergence: `origin/master...HEAD = 29 985`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- `git fetch origin master`, tracked/index cleanliness, and `git diff --check` passed.
- An unrelated `test_bitcoin --run_test=wallet_tests --log_level=test_suite` child
  (PID 777094, parent Codex PID 725042) had been sleeping in `futex_wait_queue`
  for about 12 minutes at the gate. It was not started or terminated by this
  cycle and is retained as an external process limitation; all new scratch
  paths and process checks must distinguish it from cycle work.

## Scope and exclusions

Map lock ownership, callback contracts, atomics, condition variables, worker
shutdown, object lifetimes, lock ordering, and deterministic schedules. This is
a fresh cell after Cycle 16 and Cycle 35. Exclude the `CConnman::DumpAddresses`
periodic/shutdown rename race, `ForNode`/`ForEachNode` lock-order review,
transaction-download versus mempool lock nesting, V2 transport lock order, and
the normal scheduler-before-destruction shutdown ordering unless new evidence
changes their contract.

Initial queue:

1. Callback-owned peer state: callbacks invoked after peer snapshots or from
   validation/scheduler queues may retain a raw `CNode*` or peer id across a
   lock release, disconnect, or node-list mutation.
2. Socket shutdown and worker lifetime: partial I/O, interrupt, close, and join
   ordering may permit a worker or callback to touch a socket/owner after the
   published shutdown state.
3. Validation and scheduler callbacks: a callback may acquire a manager or
   wallet lock while another path publishes the same state under a reverse
   order, especially during restart and teardown.
4. Atomics and wait predicates: a relaxed flag, condition-variable predicate,
   or callback handoff may permit missed wakeups, stale state, or a progress
   failure under a deterministic schedule.

For each candidate, state the lock graph and lifetime invariant, identify the
first conflicting access or missed happens-before edge, and distinguish a
non-nested sequence from a real cycle. Use barriers or deterministic hooks
instead of sleeps. Preserve the exact schedule and do not claim a race from
static naming alone.

## Evidence ledger

### Candidate review and verdict

#### Scheduler callback lifetimes during shutdown

`src/init.cpp:1502-1515` schedules callbacks that capture `node` and `args`,
`src/init.cpp:1702-1705` captures the fee estimator, `src/init.cpp:2370-2375`
captures BanMan and PeerManager, and `src/net.cpp:3688-3695` captures
CConnman. `src/init.cpp:312-444` stops the chain clients, joins connman and
the background initialization thread, then calls `node.scheduler->stop()`;
only after that join does it reset peerman, connman, banman, and the remaining
node objects. The wallet loader's `WalletContext` remains inside
`node.chain_clients` until after the scheduler join. Thus a callback can run
during the pre-stop cleanup window, but no captured object is destroyed until
the service thread has joined.

The wallet callback is also bounded by `WalletContext::wallets_mutex` when it
copies the wallet list. `UnloadWallets()` removes entries under the same mutex
and waits for the shared wallet reference to be released; the scheduler's
local `shared_ptr` therefore prevents destruction while `MaybeResendWalletTxs`
is using that wallet. `fBroadcastTransactions` is set during wallet creation
and `m_next_resend` is updated by the same periodic callback, so this review
found no concurrent writer. No source change is justified.

#### Peer snapshots, disconnect, and socket shutdown

`CConnman::NodesSnapshot` copies `m_nodes` and increments each node's atomic
reference count while holding `m_nodes_mutex`; its destructor releases those
references. `DisconnectNodes()` closes sockets and moves disconnected nodes to
the deferred list, deleting only after the owner reference and all snapshots
are released. Socket I/O and `CloseSocketDisconnect()` serialize access to the
socket through `m_sock_mutex`. `PeerManagerImpl::GetAllPeers()` copies
`shared_ptr<Peer>` values under `m_peer_mutex`, and `RelayAddress()` retains raw
peer pointers only while that mutex remains held. The callback-owned-state and
partial-I/O hypotheses were not reproducible and remain dismissed.

#### Lock graph and wait predicates

The current relay-budget path has the nested order
`m_inv_to_send_mutex -> m_mempool.cs` and later takes `m_peer_mutex` only after
the mempool scope ends; per-peer inventory state is taken before its bloom
filter in both backlog distribution and `SendMessages()`. A complete search of
the current production call sites found no reverse nested
`m_peer_mutex -> m_inv_to_send_mutex` or bloom-filter inversion. The message
handler tests `flagInterruptMsgProc` and its wake flag under `mutexMsgProc`,
while scheduler stop sets its predicate under `newTaskMutex` before notifying
and joining. Private-broadcast waits use an atomic wait and every increment or
shutdown release notifies it. No missed-wakeup or lock cycle was demonstrated.

#### Independent runtime evidence

- `TMPDIR=/data/my_storage/tmp/cycle98-tests /data/my_storage/tmp/cycle93-build/bin/test_bitcoin --run_test=scheduler_tests --log_level=message --report_level=short`: 4 cases and 27 assertions passed.
- The corresponding `net_tests` run passed 34 cases and 141,323 assertions.
- The combined rerun `--run_test=net_tests,scheduler_tests` passed 38 cases and 140,846 assertions.
- `git diff --check` passed. The unrelated `wallet_tests` process from the
  cycle gate was left untouched and prevented an uncontended wallet-shutdown
  stress run; this is an environment limitation, not a product failure.

## Cycle verdict and handoff

Cycle 98 is a verified dismissal with no production or test change. The next
risk map prioritizes a fresh subsystem selected from the full catalog, with
callback-owned state, platform-specific worker shutdown, and atomic progress
contracts still unchecked outside the reviewed net/scheduler paths. Preserve
the exact source ranges and test commands above to avoid repeating this cell.

# Cycle 163: mapport lifecycle serialization

## Selection and gate

- The fresh gate after Cycle 162 kept the catalog, prompt, and goals TSV hashes
  unchanged. `origin/master` was `67efced1fc83a0b7215cc1513e7c4754fee0f12f`,
  the merge base was `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and the clean
  tracked start was `c7aeed287d4426b0af928137ba41e14cc375b754` with divergence
  `42 1107`.
- The exact selector sequence was `58`, `62`, `7`, `8`: goals 58, 62, and 7
  were closed cells, so each was rerolled as required. Goal 8 was accepted
  because its prior risk map left platform-specific worker shutdown and atomic
  progress contracts open. The branch is
  `uber-cycle-163-locking-threading-20260730`.
- The persistent wallet test process (PID 777094), the unrelated util test
  process (PID 956381), and all unrelated untracked artifacts were preserved.

## Scope and hypothesis

The reviewed net/scheduler cells from Cycles 16, 35, and 98 remain excluded.
This cycle selected the cross-thread mapport lifecycle boundary:

`OptionsModel::setOption(MapPortNatpmp)` calls `node().mapPort()` from the GUI
thread (`src/qt/optionsmodel.cpp`), while core shutdown calls `StopMapPort()`
from `Shutdown()` (`src/init.cpp`). Both paths reached the global
`g_mapport_thread` in `src/mapport.cpp` without a mutex. `StartThreadMapPort()`
assigned that `std::thread`, while `StopMapPort()` and
`InterruptMapPort()` called `joinable()` and, respectively, `join()` or the
interrupt object. `CThreadInterrupt` makes the interrupt flag safe, but does
not make concurrent access to the `std::thread` object safe.

The expected contract is that enable, disable, interrupt, and join operations
are serialized as one lifecycle state machine. In particular, disabling must
hold the lifecycle exclusion across both interrupt and join; locking those two
public calls separately would leave a window in which another enable could
start a thread after the interrupt check and before the join.

## Independent verification

The new `pcp_tests/mapport_lifecycle_is_serialized` test installs a null socket
factory, uses a two-party `std::barrier`, and concurrently executes 32 enable
and disable operations. It leaves a final disable operation to clean up any
thread started by the final enable.

Against the original source, the dedicated Clang 19 ThreadSanitizer build
(`cmake -S . -B /data/my_storage/tmp/cycle163-tsan -G Ninja` with
`-fsanitize=thread`, `-DENABLE_IPC=OFF`, and `-DWITH_CCACHE=OFF`) ran:

`TSAN_OPTIONS='halt_on_error=1:exitcode=66:report_signal_unsafe=0' TMPDIR=/tmp /data/my_storage/tmp/cycle163-tsan/bin/test_bitcoin --run_test=pcp_tests/mapport_lifecycle_is_serialized --random=163007 --log_level=message --report_level=short --color_output=false`

It exited 66 with a ThreadSanitizer report. The write was the
`std::thread::operator=` in `StartThreadMapPort()` (`src/mapport.cpp:134` in
the pre-fix binary), and the concurrent read was `std::thread::joinable()` in
`StopMapPort()` (`src/mapport.cpp:157`); TSan identified the object as the
global `g_mapport_thread`. This is a direct first-conflicting-access trace,
not a naming or scheduling inference.

The fix adds a file-scope `GlobalMutex`, keeps the worker-start helper private,
and splits interrupt/stop into internal helpers. All three public lifecycle
entry points lock the mutex, while `StartMapPort(false)` keeps the same lock
across interrupt and join. `StopMapPortInternal()` also interrupts before
joining, so the separate shutdown calls remain safe if a GUI enable happens
after the earlier `InterruptMapPort()` observed no worker. The worker never
takes this lifecycle mutex, so joining while it is held cannot create a lock
cycle.

The same TSan command with seed `163014` against the fixed binary exited 0:
one test case and one assertion passed with no sanitizer report. The fixed
Clang 19 UBSan/alignment/object-size binary was rebuilt with:

`cmake --build /data/my_storage/tmp/cycle106-clang19-ubsan --target test_bitcoin -j2`

The following runs all exited 0:

- `pcp_tests`, seed `163015`: 13 cases, 213 assertions.
- `net_tests`, seed `163016`: 36 cases, 146816 assertions.
- `denialofservice_tests`, seed `163017`: 5 cases, 90 assertions.
- `scheduler_tests`, seed `163018`: 4 cases, 27 assertions.

The original source also passed the UBSan lifecycle test, which is expected
because UBSan does not diagnose a C++ data race; the TSan before/after result
is the independent regression proof. Initial TSan configuration attempts were
blocked by the installed Cap'n Proto 0.9.2/Clang 19 C++20 incompatibility and
a broken ccache symlink; IPC and ccache were disabled in the successful build.

## Verdict and handoff

Cycle 163 confirmed and fixed a GUI-versus-shutdown mapport lifecycle race.
The source/test change and this journal belong in one independent commit. The
next open mapport cell is platform-specific behavior of the worker's network
operations during shutdown; do not reopen the fixed `std::thread` lifecycle
race unless its ownership or call-thread contract changes. Continue by
checking the source commit in isolation, then update the uber-goal state and
perform a fresh gate before the next exact selector draw.
