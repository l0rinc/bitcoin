# Journal: locking, threading, and scheduler audit (campaign 8)

Uber-goal rotation. Branch: audit/locking-threading from audit/resurrection
@ 88c57f8638. Prior coverage (don't redo): goal 97 D12/D13 layers
(-Wthread-safety, DEBUG_LOCKORDER, TSan CI, atomic discipline), goal 89 P6
disconnect teardown, f344e8102c index race.
Fresh surfaces: scheduler lifecycle, callbacks under locks, RMW splits,
cancellation, destruction while work remains.

## Scope ledger

| # | area | hypothesis seeds | verdict |
|---|------|------------------|---------|
| T1 | CScheduler lifecycle | stop during task execution; destruction with queued work; restart after stop; task added after stop requested | open |
| T2 | ValidationInterface queue | SyncWithValidationInterfaceQueue vs registration/removal during drain; deadlock vs missed callback | open |
| T3 | callbacks under locks | ValidationSignals invoking arbitrary callbacks — lock inversions documented? | open |
| T4 | atomics RMW splits | check-then-act on atomic state in scheduler/connman (non-annotated paths) | open |

## Verdicts

### T1 (CScheduler lifecycle): DISMISSED — one-shot contract, coherent guards

- stop() (scheduler.h:79-84): sets stopRequested, notifies, JOINS the
  service thread — serviceQueue unlocks around f() (scheduler.cpp:56-61),
  so stop blocks until the current task finishes; no mid-task interruption.
- Destructor (16-19): asserts nThreadsServicingQueue==0 and, if
  StopWhenDrained was used, an empty queue — destruction with work is only
  possible after stop(), which intentionally abandons pending tasks.
- schedule() after stop: accepted but never runs (no shouldStop check at
  insert, 71-78) — one-shot scheduler contract; matches the single
  process-lifetime instance. Multi-thread service safe (erase-under-lock
  53-54, exit notify 68). f() exceptions are fail-loud (62-65 rethrow).

### T2 (ValidationInterface queue): DISMISSED — pinned lifetime, FIFO drain

- Callback lifetime is pinned through drain by the shared_ptr ListEntry +
  invocation count (validationinterface.cpp:88-97) — the #18338 fix;
  unregister during a callback erases from the map but the in-flight
  invocation completes safely.
- SyncWithValidationInterfaceQueue (149-158) waits FIFO — sees all prior
  queued work. BOUNDARY NOTED: calling Sync from INSIDE a validation
  callback self-deadlocks (the waited task can't run until the callback
  returns) — caller-side contract violation, not a library defect.
- Callbacks NEVER run under cs_main by design (ENQUEUE_AND_LOG_EVENT
  queues everything to the task runner). T3 subsumed — dismissed.

### T4 (atomics read-modify-write splits): DISMISSED — one-way flags + pinned lifetimes

Check-then-act on fDisconnect and friends is safe: the flag is one-way
(false→true only, no ABA), and the acted-on CNode is lifetime-pinned by
the m_nodes shared_ptr snapshot (net.cpp NodesSnapshot) — worst case is
one extra no-op iteration on a dying peer. The 37 relaxed-atomic sites
were classified in goal 97 D12 (counters/gauges/mock clocks/monotonic
caches; none publishes a payload).

## Campaign 8 cycle complete

All 4 ledger areas dismissed. The threading lattice holds: one-shot
scheduler with coherent stop semantics, pinned-callback validation queue
with FIFO sync, callbacks never under cs_main, safe one-way atomic flags.
Rotation: uber-ledger marks #8 DONE, next #15.

## Next queue
(T1 first: read CScheduler Stop/StopRequested/AreThreadsServicingQueue +
destructor path)

## Cycle 356 (2026-08-04, r190) — delay-queue mutex discipline: complete

Draw r190 (raw=9092658967074186760 -> #8). Reopen hook: the
delay-queue subsystem added m_inv_to_send_mutex-guarded global state
(the two InvToSendBuckets).

Guard walk (all access sites): GetInfo LOCKs at :2010;
ProcessInvBacklog LOCKs at :2452 (covers the heartbeat reads, the
extraction, the per-peer drain loop, and the no-connection clears);
InitiateTxBroadcastToAll LOCKs at :2528; ctor inits are
single-threaded. Every public entry carries the negative annotation
(!m_inv_to_send_mutex). Lock ORDER: m_inv_to_send_mutex is
ACQUIRED_BEFORE(m_mempool.cs) (:1219); the only nesting is
m_inv_to_send_mutex -> tx_relay->m_tx_inventory_mutex
(ProcessInvBacklog's drain loop), and no path takes them in reverse
(ProcessInvBacklog's SendMessages call site at :6187 holds no
inventory lock; GetInfo/InitiateTxBroadcastToAll are lock-free at
entry). No deadlock shape, no unguarded access. DISMISSED.
