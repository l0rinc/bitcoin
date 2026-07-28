# Assertion, Assume, and Invariant Reachability Audit

## Cycle 68

### Draw and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `2`
- Selected goal: `assertion-invariant-audit`
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Cycle gate HEAD: `0c8c5ad2d8ffcf1468d46339608efd42d37fb1c1`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `HEAD...origin/master` was `908 2` at the gate.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Tracked and staged state was clean except known untracked agent artifacts; no relevant process was running.

### Scope and initial hypothesis

The prior AddrMan `Select` assertion-oracle correction, the MuSig argument-ordering fix, and earlier lock/lifetime cells are excluded. This cycle opens the distinct release-reachable question of whether an `Assume`, `assert`, `CHECK_NONFATAL`, or `VERIFY` statement is being used as the only guard for data arriving through RPC, configuration, network, persisted state, fuzz entry points, or optional-module boundaries.

The falsifiable hypothesis is that at least one current assumption is reachable with an invalid or merely unusual production input, or that a release-disabled assertion removes a check required by a later dereference, cast, indexing operation, state transition, or public output contract. The trust boundary includes malformed serialized data, rejected-but-parsed API values, partially initialized objects, restart/recovery state, and callers compiled with different debug/VERIFY/configuration settings.

### Status

Cycle 68 is in progress. Candidate inventory and prioritization are open; no production source has been changed.

### Method

For each candidate, record the exact invariant, all callers, the build mode in which it is active, and the first operation that relies on it. Compare source, history, tests, docs, fuzz harnesses, and release configuration. Try to falsify the invariant with boundary values, malformed persisted/network inputs, error returns, and lifecycle transitions. A candidate cannot become a finding without a failing-before proof, first-invalid-operation trace, mutation-sensitive regression, or a rigorous caller/dataflow proof.

## Next queue

1. Inventory non-test `Assume`, `assert`, `CHECK_NONFATAL`, `VERIFY`, and unreachable branches, then rank by untrusted reachability and consequence.
2. Verify the highest-risk release and optional-module cells with focused tests and one independent mutation or negative control.
3. Lock each candidate as confirmed, dismissed, or inconclusive before selecting the next distinct invariant.
