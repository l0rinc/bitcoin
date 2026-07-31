# Deterministic Simulation and Failure-Schedule Exploration

## Cycle 84 start

- Selected by the uber loop: exact `shuf -i 0-98 -n 1` -> `71` (`deterministic-simulation`).
- Branch: `uber-cycle-84-deterministic-simulation-20260728`.
- Cycle-start HEAD: `c79c8b401b43dd1c367cf81f9508db68bae90372` (`journal: close sanitizer cycle 83`).
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence `2 950`.
- Scope: explore seeded task ordering, time, randomness, network, disk, retries, and shutdown schedules through production logic with explicit state, progress, durability, and resource invariants.
- Exclusions: Cycle 9's generated Silent Payments artifact determinism is a separate generator/vector contract and is recorded in the pre-existing untracked `agent-journal/deterministic-artifacts.md`. Do not repeat the prior full-sync, sanitizer, P2P accounting, filesystem crash, database fault, or scheduler cells unless a new schedule, trust boundary, or first-invalid operation is demonstrated.
- Gate: catalog/protocol/TSV hashes match; tracked source was clean after cycle 83; known untracked agent artifacts and `test/cache` are preserved and excluded. Use `/data/my_storage/tmp` for scratch data because the root filesystem is full. No relevant process was running at initialization.

## Campaign contract

Production code must run through the experiment wherever practical. A schedule is a seed plus an explicit ordered trace of task, clock, randomness, network, disk, retry, and shutdown choices. Every candidate must state invariants for final state, progress, durability, cleanup, and resource bounds before execution. A replay must produce the same decisions and a minimized failing schedule; a passing run is evidence only for the exercised bounded state space.

## Hypotheses

1. A production worker or callback lifecycle has a schedule-dependent state or lifetime bug that ordinary sleeps and nondeterministic thread tests do not expose.
2. A retry, timeout, or clock transition can violate progress or resource bounds when failure and shutdown events are interleaved in a specific order.
3. A persistence boundary can report progress or publish in-memory state before the corresponding durable write/flush/recovery point under a replayable fault schedule.
4. An existing deterministic test or simulation seam models state separately from production logic, so it misses a reachable schedule or asserts only execution rather than the required postcondition.

## Verification protocol

- Inventory existing deterministic hooks, mock clocks, task queues, thread interruptors, fault injectors, socket shims, database environments, and scheduler tests before adding infrastructure. Search journals, history, issues, and review precedent for duplicate schedules.
- Build the smallest production-path schedule first. Record seed, every decision, initial state, environment, exact command, raw trace, final state, progress counters, durable markers, cleanup, and resource peaks. Do not use sleeps as a synchronization oracle.
- Keep discovery and verification independent when practical: replay from a serialized schedule, compare against a reference/state recomputation, run a temporary mutation that should violate the invariant, and shrink any failing sequence while preserving the first invalid transition.
- Classify candidates as source, test/harness, documentation, tool, dependency, or other-project behavior. Commit only a confirmed local root cause with a failing-before/passing-after oracle and one self-contained source/test commit authored as `Lőrinc <pap.lorinc@gmail.com>`.
- If no source defect is found, retain rejected schedules and exact missing coverage in this journal. A cycle close may contain at most one journal-only handoff snapshot.

## Initial queue

- Map existing scheduler, thread interruption, `-dbcrashratio`, database fault, mock-time, socket, and shutdown seams to production callers and current tests.
- Search recent fixes involving retries, disconnects, flush/recovery, chainstate/index state, mempool workers, wallet callbacks, and background compaction for a schedule shape that has not yet been replayed.
- Select the highest-risk uncovered schedule cell, define its invariant table, then implement or reuse the smallest deterministic trace runner without creating a parallel fake implementation.

## Cycle 84 verification: failed network start published live threads

### Worktree reconciliation

The cycle-start snapshot was `c79c8b401b`. Before the source experiment, the existing branch had advanced to `55cfa7a4489a6185dac5207a504b708184611b5e` (`doc: summarize secp256k1 Codex Security triage`) through three pre-existing commits: `0bfcf35cf6`, `7f08743b4a`, and `55cfa7a448`. These commits and the pre-existing untracked files were preserved and are outside this cycle's scope. At the experiment point, `origin/master...HEAD` was `2 954`, with merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.

### Contract and hypothesis

`CConnman::Start` is a public lifecycle boundary. For an invalid combination of `m_use_addrman_outgoing=true` and a nonempty `m_specified_outgoing`, the contract is:

| Observable | Required result |
| --- | --- |
| return value | `false` |
| thread lifecycle | no network thread is joinable or running after return |
| side effects | no listen setup, address-publication state, semaphore publication, or worker startup caused by the rejected start |
| cleanup | the owner can destroy or retry the connection manager without first repairing a partially started instance |

The falsifiable hypothesis was that the old check violated the thread and side-effect rows because it followed `threadSocketHandler`, optional DNS startup, and `threadOpenAddedConnections` creation. The ordinary node-init path normally derives `m_use_addrman_outgoing=false` when `-connect` is supplied, but direct callers of `CConnman::Start` can still pass the conflicting `Options` pair.

### Source and history evidence

`git grep -n -E 'm_use_addrman_outgoing|specific connections' src/net.cpp src/net.h src/init.cpp src/test` located the only `Start` rejection and its test/fuzz option callers. In the pre-fix source, `Start` called `Init`, skipped listening in the test schedule, initialized the I2P/anchor and address state, created semaphores, reset interrupts, started `threadSocketHandler`, skipped DNS because the test setup forces `-dnsseed=0`, started `threadOpenAddedConnections`, and only then rejected the conflicting options. `StopThreads` could join those handles, but a caller receiving `false` had already observed a partially started manager.

`git log --oneline --all -S'm_use_addrman_outgoing && !connOptions.m_specified_outgoing.empty()' -- src/net.cpp` identified `352d582ba2` (`Add vConnect to CConnman::Options`) as the originating change. `git blame` places the historical rejection after the thread creation block. The fix retains the same bilingual UI error and return value, but evaluates it immediately after `Init` and before `InitBinds` or any other `Start` side effect.

### Deterministic reproduction

The test uses the production `CConnman::Start` path and the existing test `CScheduler`; it does not use sleeps or a fake worker implementation. Initial state is a fresh `RegTestingSetup` connection manager with `m_msgproc` set to the real `PeerManager`, `fListen=false`, `bind_on_any=false`, `m_i2p_accept_incoming=false`, and the test setup's DNS seeding disabled. The input schedule is:

```text
options: m_use_addrman_outgoing=true, m_specified_outgoing=["127.0.0.1"]
call CConnman::Start
observe return value and all seven thread handles
if the old implementation started a thread: Interrupt, Stop, and join it
assert return=false and no handle is joinable
```

The old source was built and run with:

```text
TMPDIR=/data/my_storage/tmp cmake --build /data/my_storage/tmp/cycle84-build --target test_bitcoin -j2
TMPDIR=/data/my_storage/tmp /data/my_storage/tmp/cycle84-build/bin/test_bitcoin --run_test=net_tests/connman_start_rejects_conflicting_options_before_threads --log_level=test_suite
```

The old run exited `201` at `src/test/net_tests.cpp:391`: `!started` passed, while `!threads_started` failed. This is the first invalid lifecycle observation, not a timing inference: the test reads the actual `std::thread::joinable()` state through the existing test friend.

The repaired source was rebuilt with the same build command. The focused test then exited `0` with both assertions passing, and the complete networking suite passed:

```text
TMPDIR=/data/my_storage/tmp /data/my_storage/tmp/cycle84-build/bin/test_bitcoin --run_test=net_tests --log_level=message --report_level=detailed
```

Result: 34/34 `net_tests` cases and 150,899/150,899 assertions passed. `git diff --check` passed. The old failure was reproduced independently before the source move, and the passing-after regression depends on the moved production guard rather than an altered expectation.

### Verdict, fix, and limits

Verdict: **confirmed local lifecycle/source defect**, reachable by any caller that supplies the conflicting public `Options`; the normal command-line construction path makes it uncommon, so this is a correctness and shutdown-integrity issue rather than a consensus or remote high-severity finding. The smallest repair is one source move, plus `ConnmanTestMsg::AnyThreadJoinablePublic()` and the focused regression. No unrelated cleanup or new simulation infrastructure was added.

The test covers the invalid configuration and proves the failed-start no-thread invariant. It does not prove all possible `Start` failure paths are side-effect-free, nor does this cycle cover Clang/TSan or an active network start. Those remain separate queue cells. No process remains running and the next cycle must be selected after the finding commit.

## Cycle 235 Completion: Goal 71, deferred scheduler task after runner destruction

- Exact selector: `shuf -i 0-98 -n 1` -> `71` (`deterministic-simulation`); no reroll.
  Branch: `uber-cycle-235-deterministic-simulation-20260731`. Cycle-start HEAD
  was `55a36c79e3a02aadfbf2509bfcb68cb1e45739b2`; `origin/master` was
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base was
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `42 1253`.
  The fresh gate passed, catalog/prompt/TSV/protocol hashes were unchanged,
  and protected PIDs `777094`, `956381`, `1138182`, and `1157959` remained
  alive. The previous Goal 71 scheduler cell from Cycle 84, which concerned
  `CConnman::Start` publishing worker threads before rejecting options, was
  excluded. This cycle exercises a distinct `SerialTaskRunner` post-flush
  lifetime schedule.
- Contract and schedule: construct a `CScheduler` with no service thread;
  insert one callback into a `SerialTaskRunner`; call `flush()`, which runs the
  callback directly but leaves the already queued `ProcessQueue` wrapper in the
  scheduler; destroy the runner; then start `serviceQueue()` and schedule its
  stop callback. Required invariants are exactly one user-callback execution,
  no callback or dereference after runner destruction, safe scheduler reuse,
  and one bounded deferred wrapper. The old wrapper captured raw `this`, so the
  later scheduler service dereferenced the destroyed runner.
- Source evidence: `SerialTaskRunner::MaybeScheduleProcessQueue()` scheduled a
  raw-`this` lambda, while `flush()` drains the runner directly and has no way
  to remove a wrapper already held by `CScheduler`. The public runner lifetime
  is bound to the scheduler, but the scheduler can outlive a runner in this
  supported no-service-thread flush sequence. Production shutdown normally
  stops scheduler processing before destroying validation signals; that order
  does not make the direct schedule safe for independently used runners.
- Independent reproduction: the temporary regression passed normally before
  the source fix, so ordinary execution did not expose the stale callback. The
  same pre-fix test in the ASan/UBSan build reported
  `AddressSanitizer: stack-use-after-scope` in `SerialTaskRunner::ProcessQueue`
  through `CScheduler::serviceQueue()`, with the runner's stack scope already
  ended. This is a first-invalid-operation sanitizer trace, not a sleep-based
  timing inference.
- Fix and regression: `SerialTaskRunner` now owns a shared atomic liveness
  token. The deferred scheduler wrapper retains the token and checks it with
  acquire semantics before dereferencing `this`; the destructor publishes
  false with release semantics. The permanent regression is
  `scheduler_tests/serial_task_runner_flush_then_scheduler_restarts` and
  asserts the callback count remains one after the scheduler services the
  stale wrapper.
- Validation: normal and ASan/UBSan `test_bitcoin` rebuilds completed with
  isolated `/data/my_storage/tmp` build and run directories. The focused test
  passed 2 assertions in both builds. The complete `scheduler_tests` plus
  `validationinterface_tests` selection passed 5 scheduler cases, 7
  validation-interface cases, and 72 assertions in both builds. The initial
  combined-suite attempt failed before fixture setup because its `TMPDIR`
  parents did not exist; the corrected isolated-directory rerun passed. The
  earlier deterministic `validation_block_reorg` and `dbwrapper` fuzz controls
  also passed 200 runs each. `git diff --check` passed.
- Verdict: **confirmed local scheduler lifetime defect and fixed**. The token
  closes the schedule where `flush()` is used while scheduler service threads
  are stopped, which is the documented precondition. It does not claim that
  arbitrary concurrent destruction while a scheduler callback is actively
  executing is safe; that would require a separate active-callback ownership
  contract and is outside this minimal fix. The next cycle must choose a fresh
  goal and must not repeat this exact deferred-runner schedule without new
  evidence.
