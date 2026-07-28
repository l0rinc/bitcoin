# Cross-Subsystem Bug Shapes Cycle 62

## Identity and Gate

- Cycle: `62`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `26`
- Goal: `bug fixed in one subsystem but present in another`
- Slug: `cross-subsystem-bug-shapes`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `387f5a122f1c85655253cd35b47d49140317e8a9`
- `origin/master...HEAD` at the gate: `2 894`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- `goals.tsv` validation: `validated_rows=99 total_lines=100 status=ok`
- No relevant test, fuzz, sanitizer, daemon, or profiling process was running at the gate.

Cycle 48 already closed the wallet-rescan reservation-ordering cell. This cycle must select a distinct historical bug shape and avoid rediscovering that lock/scheduling path.

## Scope and Hypotheses

Mine recent and historical fixes for a concrete structural pattern such as write-before-validation, unchecked durable return, stale accounting, inconsistent boundary validation, partial output mutation, missing cleanup, or cross-layer lifecycle asymmetry. Search analogous parsers, caches, indexes, queues, state machines, and public APIs using semantic structure, not only names.

1. A historical fix may have corrected one implementation family while an analogous caller remains reachable and unprotected.
2. A superficially similar site may be intentionally different because its state, trust boundary, or failure contract differs.
3. A candidate may be a test/documentation gap rather than a production defect.

For every candidate, record the seed fix, structural features, callers, trust boundary, history, tests, and expected contract. Require an independent minimal reproducer or proof of unreachability before changing code. Keep one finding per self-sufficient commit and preserve negative controls.

## Evidence Log

- Pending: select a seed defect shape from history, build a semantic site ledger, and verify the highest-risk analogous path.

## Verdict

- Pending.

## Handoff

- Pending completion. Record seed provenance, candidate paths, reachability evidence, exact test commands, and the next distinct bug-shape cell.
