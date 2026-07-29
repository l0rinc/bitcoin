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
