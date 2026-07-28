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

- Existing cycle-14 evidence was coinbase-only chainstate reindexing; existing cycle-17 evidence was index persistence and write amplification. This cycle uses the same deterministic 20,000-block regtest source only as a transport peer, so it measures real P2P header/block scheduling without reusing a local `-reindex-chainstate` path. The source tip is `05d0f52afcec58b38babfe37d622c1ad418726f26657bd2a9b5562407e5f17d0` and the source datadir is 29,135,782 bytes.
- Build: `/data/my_storage/tmp/full-sync-ibd-profile-cycle61/build`, Clang 19.1.7, `RelWithDebInfo` (`-O2 -g`), wallet/GUI/tests/fuzz/bench/IPC/ZMQ disabled, production hardening flags enabled. The `bitcoind` and `bitcoin-cli` targets built successfully. `/tmp` was full, so all scratch state stayed under `/data`.
- Each sink started empty, connected only to the local source at `127.0.0.1:29614`, used `-par=1`, `-txindex=0`, `-coinstatsindex=0`, `-maxconnections=1`, and stopped only after RPC `getblockcount` reported 20,000. The final debug log for every run reported the exact source best hash, height 20,000, and `tx=20001`; each sink datadir was 27,199,430 bytes.

### P2P IBD measurements

`perf stat` measured the sink process with task-clock, cycles, instructions, cache misses, context switches, migrations, and page faults. Wall timing includes startup and the explicit RPC stop handshake. The first run's write-byte monitor was invalid because its selector also matched `cancelled_write_bytes`; the corrected runs use exact `/proc/io` field matching.

| Sink | DB cache | Wall seconds | Task-clock ms | Instructions | Cache misses | Context switches | Migrations | Page faults | Max RSS kB | Max write bytes |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `sink-a` | 64 MB | 129.980 | 132,641 | 964,677,407,336 | 2,509,906,488 | 151,056 | 7,506 | 7,274 | 66,840 | invalid monitor |
| `sink-b` | 64 MB | 133.811 | 136,236 | 964,731,161,659 | 2,646,004,212 | 150,737 | 7,241 | 7,100 | 66,580 | 15,732,736 |
| `sink-c16` | 16 MB | 134.441 | 136,751 | 964,738,121,053 | 2,670,432,715 | 152,166 | 7,389 | 7,807 | 68,588 | 15,749,120 |
| `sink-c256` | 256 MB | 129.870 | 132,277 | 964,774,781,831 | 2,417,041,334 | 150,991 | 7,238 | 7,575 | 66,392 | 15,732,736 |

The 64 MB repeats differ by 2.9% wall and 2.7% task-clock; the 16 MB run is 3.5% slower than the fastest run and has 2.2 MB more RSS; the 256 MB run is the fastest by 0.1% over the first 64 MB run but is within observed run-to-run noise. Instructions are effectively invariant while cache misses vary by about 10%, so this short one-host matrix does not prove a cache-size optimization.

Header and connect markers separate the workload further. For `sink-a`, header synchronization logged at `15:03:37Z` and the final tip at `15:05:34Z`; for `sink-b`, `15:06:15Z` and `15:08:14Z`; for `sink-c16`, `15:09:15Z` and `15:11:16Z`; for `sink-c256`, `15:11:49Z` and `15:13:45Z`. Header acquisition therefore completed in approximately 13--14 seconds after startup, while block download/validation/chainstate processing remained approximately 116--121 seconds. This points to the connect/validation path as the dominant phase on this local fixture, not network header transfer.

Every run used 20,000 coinbase-only blocks. This is suitable for chainstate, block I/O, and scheduling baselines, but it does not exercise a transaction-heavy signature/script workload; that remains an explicit limitation and prevents a claim about crypto validation scaling.

### Consistency-check attribution

The `perf record` run `sink-profile` used the same 64 MB IBD fixture and sampled 49 Hz with DWARF call stacks. It reached the exact tip in 128.487 seconds, produced a 54,683,552-byte profile, and lost zero samples. The self-overhead report attributes 55.27% of sampled cycles to `ChainstateManager::CheckBlockIndex`; the call graph shows `CChain::Contains` and `base_uint<256u>::CompareTo` as the main nested work. The raw profile and generated reports are under `logs/perf-sink-profile.data`, `logs/perf-report.txt`, and `logs/perf-report-self.txt`.

This is a deliberate regtest/debug configuration effect, not an accidental production path. `src/kernel/chainparams.cpp` sets `fDefaultConsistencyChecks = false` for mainnet at line 186 and `true` for regtest at line 616; `src/init.cpp` documents `-checkblockindex` as a debug-only consistency check and `src/validation.cpp` samples it according to the configured frequency. Independent current-source controls used the same source peer and stop condition:

| Check setting | Wall seconds | Task-clock ms | Instructions | Cache misses | Max RSS kB | Final tip |
|---|---:|---:|---:|---:|---:|---|
| default regtest (`1`) | 129.980--133.811 | 132,641--136,236 | ~964.7 billion | 2.51--2.65 billion | 66,580--66,840 | height 20,000, expected hash |
| `-checkblockindex=100` | 4.545 | 5,461 | 23.716 billion | 74.5 million | 66,476 | height 20,000, expected hash |
| `-checkblockindex=0` | 2.931 | 3,834 | 13.796 billion | 18.5 million | 66,280 | height 20,000, expected hash |

The frequency controls reproduce the documented behavior: the default full consistency walk dominates regtest IBD, while disabling it exposes a roughly three-second local chainstate/block path. The result is useful for interpreting benchmarks and for choosing debug settings, but changing the default would weaken a deliberate regtest invariant and is not justified by a production performance claim.

### Negative controls and limitations

- All sinks started empty, used one manually connected source peer, and reached the same deterministic tip. No error, corruption, or divergent chainstate result was observed.
- The source contains 20,000 coinbase-only blocks, so script verification, signature checks, transaction graph growth, mempool interaction, and transaction-heavy index behavior are not represented. A follow-up transaction-heavy source is required before conclusions about crypto or realistic mainnet validation throughput.
- Measurements are one host with powersave CPU behavior, one source peer, one block range, and short local storage paths. `perf stat` is robust for the relative controls, but not a cross-host benchmark. No `strace` or `iostat` is installed.
- No source defect or safe production optimization was proven. The profile identifies a debug-only regtest cost and confirms the cache-size hypothesis is inconclusive; no implementation commit is warranted.

## Verdict

- Dismissed for a confirmed production defect. The dominant sampled work is the intentional regtest consistency check; cache-size variation is within run-to-run noise, and no chainstate, IBD, persistence, or correctness regression was reproduced.

## Handoff

- Cycle complete with a journal-only handoff. Raw commands, console logs, `perf stat` counters, sampled call stacks, and scratch datadirs are under `/data/my_storage/tmp/full-sync-ibd-profile-cycle61/`. The next run must recheck the gate and select a distinct catalog goal; if this goal recurs, it must use transaction-heavy blocks and separate script/crypto validation from chainstate work rather than repeat the coinbase-only fixture.
