# Untrusted-Interface Resource-Exhaustion Variant Analysis

## Cycle 57

- Selected by the uber loop: `shuf -i 0-98 -n 1` -> `7`
- Goal: `resource-exhaustion-variants`
- Started from HEAD: `e1933776ba36f5812fb42e4efc536f208c6e5110`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence at start: `origin/master...HEAD` = `2 882`
- Dirty-state gate: tracked and staged state clean; only the known agent-owned untracked artifacts remain
- Process gate: no relevant build, test, daemon, fuzz, sanitizer, or profiling process running
- Catalog/protocol/TSV hashes: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`

## Scope and Prior Evidence

This cycle mines realistic denial-of-service shapes at public or attacker-influenced interfaces. The required evidence is an explicit bound or a demonstrated accounting failure for CPU, memory, disk, network, descriptors, queue length, retries, or retained state. A large allocation or slow operation alone is not a finding when the protocol or local policy already imposes a documented bound.

The prior ledger already closes the exact duplicate global relay-backlog cell: cycle 54 proved that duplicate ForceRelay transaction announcements could retain repeated wtxids, and commit `4f867fc8a3` deduplicated the global backlog while preserving per-peer trickling. Cycles 29 and 32 also closed several integer-domain option cells (`-maxsigcachesize`, `-limitclustersize`, `-dbcrashratio`, and `-dbbatchsize`). This cycle must therefore seek a different source-to-sink shape or a recurrence in another queue/accounting domain.

## Initial Hypotheses

1. A public P2P message may decode a bounded wire vector but retain or expand an auxiliary queue without applying the same bound, allowing repeated low-cost messages to create unbounded memory or work.
2. A request/response path may bound each message but multiply the bound across peers, retries, or duplicate identities without a global or per-peer conservation rule.
3. A parser may reject an oversized payload after allocating or reserving based on the advertised length, creating a CPU, memory, or descriptor spike even though the final object is rejected.
4. An existing limit may be applied to admission but not to cleanup, disconnect, restart, or permission transitions, allowing retained state to outlive the trust boundary.

## Required Verification

- Inventory the relevant P2P/RPC/mempool/persistence entry points and write an explicit resource equation before testing.
- Trace attacker-controlled fields through decode, allocation, queue insertion, retry, timeout, cleanup, and restart paths; include permission and duplicate-identity variants.
- Prefer deterministic socket/message shims, functional tests, unit-level state models, or low resource limits over uncontrolled network load.
- For each candidate, measure operation count, bytes, allocations/RSS, queue length, retries, and cleanup state at a fixed small input; state the extrapolated bound and its assumptions.
- Search prior findings, history, issues, and reviews before reporting. Use a failing-before regression or equivalent first-invalid-operation proof for any source fix.
- Run narrow then broad validation, and preserve raw traces and minimized transcripts under `/data/my_storage/tmp/cycle57-resource-exhaustion/`.

## Evidence Ledger

To be populated after the source inventory and first deterministic controls. Candidates remain dismissed or inconclusive until their reachability, resource equation, and cleanup behavior are independently checked.

## Handoff

Cycle 57 is in progress. Recheck this journal and `agent-journal/uber-goal-state.md` before committing any source change; the next distinct hypothesis must be selected only after this cycle's evidence is recorded.
