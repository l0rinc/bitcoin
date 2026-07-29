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
