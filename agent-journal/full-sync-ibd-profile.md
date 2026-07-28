# Full Sync, IBD, Import, and End-to-End Profile Cycle 61

## Identity and Gate

- Cycle: `61`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `22`
- Goal: `full sync, IBD, import, and end-to-end profiling`
- Slug: `full-sync-ibd-profile`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `3d67541b54019711263e742be691e83da57cd0ec`
- `origin/master...HEAD` at the gate: `2 892`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- `goals.tsv` validation: `validated_rows=99 total_lines=100 status=ok`
- No relevant test, fuzz, sanitizer, daemon, or profiling process was running at the gate.

The previous profiling campaigns covered rebuild/recovery, disk amplification, perf/flamegraphs, and recent regression bisection. This cycle therefore selects a distinct end-to-end local import/IBD profile and must separate workload phases before proposing any optimization.

## Scope and Hypotheses

1. A reproducible local block import or bounded IBD workload may have a dominant phase that is obscured by aggregate wall time or network setup.
2. A phase may spend measurable work on avoidable logging, serialization, disk I/O, cache misses, or scheduler overhead rather than validation itself.
3. Any apparent win may be a harness, cache-warmth, data-size, or compiler artifact rather than a production improvement.

Use scratch datadirs and fixed source data. Pin the exact commit, build flags, cache/prune/index settings, stop height/range, storage path, and process environment. Do not use default datadirs, wallets, keys, or production state. Change one factor at a time and require repeated measurements plus matching final chainstate/results before considering a source change.

## Evidence Log

- Pending: inventory available deterministic block fixtures and existing profile artifacts, then run the smallest representative import/validation workload.

## Verdict

- Pending.

## Handoff

- Pending completion. Preserve raw timings, counters, command lines, data hashes, and the next distinct phase hypothesis.
