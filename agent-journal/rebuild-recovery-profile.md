# Long-Running Rebuild, Recovery, and Compaction Profiling

## Cycle 14

- Date: 2026-07-28 UTC
- Goal index: 21
- Slug: `rebuild-recovery-profile`
- Selector command/result: `shuf -i 0-98 -n 1` -> `21`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start and end: `bfa5a84f0ac8a18ce73bc24c0c280040c67e2a34`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Scratch root: `/data/my_storage/tmp/rebuild-recovery-profile-cycle14/`

### Scope and hypotheses

The selected campaign called for a reproducible rebuild, reindex, recovery, snapshot-load, or compaction workload with CPU, memory, disk, progress, and correctness evidence. The source workload is a fixed-time regtest chain with 20,000 coinbase-only blocks, 20,001 transactions, and 20,000 spendable outputs. All runs used copies of a stopped source datadir and explicit non-default ports. No default datadir, wallet, key, or production database was used.

The cycle tested three falsifiable hypotheses:

1. Rebuilding chainstate with a 16 MB database cache has materially different resource behavior from a 256 MB cache on the same blocks.
2. Rebuilding chainstate together with `txindex` and `coinstatsindex` exposes a cache-dependent persistence or correctness problem.
3. Killing reindexing at a deterministic intermediate height leaves a state that cannot be reopened and converged to the source tip without an explicit reindex.

The primary correctness invariant was:

```text
getblockchaininfo.blocks == 20000
getblockchaininfo.headers == 20000
getblockchaininfo.bestblockhash == 05d0f52afcec58b38babfe37d622c1ad418726f26657bd2a9b5562407e5f17d0
```

For indexed runs, `getindexinfo.txindex` and `getindexinfo.coinstatsindex` also had `synced: true` and `best_block_height: 20000`. The reindex completion marker was `initload thread exit`. The fixed mock timestamp made the generated block hashes reproducible, but it also caused `initialblockdownload: true` and a low `verificationprogress` on the host clock; those fields were not used as completion criteria.

### Build and environment

The profile binary was configured from the current tree with the production-like optimized configuration below:

```text
cmake -S . -B /data/my_storage/tmp/rebuild-recovery-profile-cycle14/build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 \
  -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF \
  -DBUILD_FOR_FUZZING=OFF -DBUILD_UTIL=OFF -DENABLE_WALLET=OFF \
  -DENABLE_IPC=OFF -DWITH_ZMQ=OFF -DWITH_GUI=OFF
cmake --build /data/my_storage/tmp/rebuild-recovery-profile-cycle14/build --target bitcoind -j2
```

The build completed at 100% and produced `build/bin/bitcoind`. Existing warnings were an old libstdc++ deprecation in `node/eviction.cpp`, an unneeded internal function in `signet.cpp`, and unneeded members in `txgraph.cpp`; no warning was caused by this cycle. The machine permitted `perf stat`. `/usr/bin/time`, `iostat`, and `strace` were unavailable, so `perf` counters, shell nanosecond timestamps, `du -sb`, and one `/proc` monitor were used instead. The indexed monitor captured `VmHWM` and `/proc/<pid>/io` counters, but the short workload yielded one process sample; this is an observed high-water mark, not a statistically complete RSS trace.

### Source dataset

The source node used:

```text
bitcoind -regtest -datadir=/data/my_storage/tmp/rebuild-recovery-profile-cycle14/source \
  -port=18514 -rpcport=18515 -rpcbind=127.0.0.1 -rpcallowip=127.0.0.1 \
  -listen=0 -dnsseed=0 -natpmp=0 -discover=0 -connect=0 \
  -fallbackfee=0.00001000 -dbcache=16 -par=1 -txindex=0 -coinstatsindex=0 -daemonwait
```

RPC `setmocktime(1700000000)` was followed by 200 deterministic `generatetoaddress` calls of 100 blocks each, using the fixed regtest address from the functional test framework. The final RPC result was:

```text
blocks=20000
bestblockhash=05d0f52afcec58b38babfe37d622c1ad418726f26657bd2a9b5562407e5f17d0
chain=regtest
verificationprogress=1
initialblockdownload=false
```

The stopped source datadir was 28,952,713 bytes. Copies were made as `low16-a`, `low16-b`, `high256`, `indexes16`, `indexes256`, `crash`, `crash-auto`, and `rss-indexes16`. Each copy started from the same stopped source state.

### Chainstate reindex comparison

Each run used `-reindex-chainstate`, `-par=1`, `-txindex=0`, and `-coinstatsindex=0`. The foreground daemon was wrapped by `perf stat` with task-clock, cycles, instructions, cache misses, branches, branch misses, context switches, CPU migrations, and page faults. RPC stopped the daemon after the expected tip was observed; the console log confirmed `initload thread exit`.

| Run | Cache | Perf task-clock | Instructions | Cache misses | Page faults | Wall wrapper | Final bytes |
|---|---:|---:|---:|---:|---:|---:|---:|
| `low16-a` | 16 MB | 1,550.41 ms | 6,186,134,913 | 5,487,122 | 5,221 | 18.186 s | 35,339,925 |
| `low16-b` | 16 MB | 1,552.81 ms | 6,214,693,406 | 4,682,537 | 5,226 | 13.321 s | 35,339,924 |
| `high256` | 256 MB | 1,663.75 ms | 6,301,897,860 | 4,637,249 | 5,446 | 10.723 s | 35,339,929 |

The wrapper wall time includes daemon shutdown and is not a clean startup-only metric. The repeated low-cache CPU measurements were close, and the high-cache run was not a reproducible improvement: it used more task-clock, cycles, instructions, and page faults while reducing cache misses slightly. The final chainstate contents and public tip were equivalent. No cache-size optimization is justified.

### Index rebuild comparison

The same source was rebuilt with `-txindex=1` and `-coinstatsindex=1`, and the probe waited for both indexes to report height 20,000.

| Run | Cache | Perf task-clock | Instructions | Cache misses | Page faults | Wall wrapper | Final bytes |
|---|---:|---:|---:|---:|---:|---:|---:|
| `indexes16` | 16 MB | 4,357.13 ms | 16,427,857,351 | 9,019,017 | 5,314 | 16.360 s | 41,230,811 |
| `indexes256` | 256 MB | 4,302.92 ms | 16,417,823,296 | 9,212,980 | 5,559 | 17.366 s | 41,419,778 |

Both index databases reported `synced: true` at height 20,000. The approximately 1.2% task-clock difference is below what this one-run-per-cache workload can establish, and the on-disk LevelDB layouts differed without changing the logical result. The indexed run added roughly 12.3 MB over the source. The related current PR #35827 (`bench: add index benchmarks`) identifies the same measurement gap and intentionally uses chained valid transactions and disk-versus-memory benchmarks; this cycle's coinbase-only chain is therefore useful for chainstate and baseline index startup, but not for scaling conclusions about input/output-heavy index work. Its stated priorities are I/O-versus-CPU attribution, on-disk size, and memory; likely review concerns are fixture realism and whether benchmark counters are reported consistently. It was treated as a seed, not an oracle.

The adjacent current PR seeds were also recorded: #35731 hardens the flush-error notification invariant, #35744 protects LevelDB cursors from cache resize, and #35753 handles a null mempool during chainstate deletion. None supplied a reproduced failure in this workload. Their relevant review themes are explicit error-path invariants, lifetime/locking scope, and preserving kernel-versus-node behavior.

### Crash and restart recovery

For `crash`, a foreground `-reindex-chainstate` process was killed with `SIGKILL` after the fresh console log reached height 10,000. The kill trigger was recorded in `logs/crash-kill.trigger`; no `Shutdown done` line was emitted. Restarting the same copy with `-reindex-chainstate` wiped and rebuilt the chainstate, then reached the expected tip.

The stronger `crash-auto` trial restarted the killed copy without `-reindex-chainstate`. Startup reopened the existing LevelDB, loaded the partial state at height 9,999, replayed heights 10,000 and 10,001, and reached height 20,000 with the exact source hash. The relevant log sequence was:

```text
Opening LevelDB in .../crash-auto/regtest/chainstate
UpdateTip: new best=... height=9999
UpdateTip: new best=... height=10000
UpdateTip: new best=... height=10001
UpdateTip: new best=05d0f52afcec58b38babfe37d622c1ad418726f26657bd2a9b5562407e5f17d0 height=20000
initload thread exit
```

The automatic recovery probe returned `blocks=20000`, `headers=20000`, and the expected best hash. This is positive evidence for the tested crash schedule, not a proof of all filesystem or power-loss schedules. The `crash-auto` final chainstate was 38,890,638 bytes and no corruption/error line was reported.

### RSS and filesystem counters

The `rss-indexes16` run repeated the indexed 16 MB case with a polling monitor over `/proc/<pid>/status` and `/proc/<pid>/io`. The monitor observed:

```text
samples=1
max_rss_kb=72040
max_hwm_kb=72040
max_read_bytes=0
max_write_bytes=22044672
```

The low sample count is a limitation caused by the approximately four-second workload completing between monitor iterations. The value is retained as a sanity check, not used to claim a peak-memory regression. All raw console, perf, wall, state, index, trigger, process, and source-generation files remain under the cycle scratch root.

### Verdict

All three hypotheses were **dismissed for a confirmed product defect** on the tested workload. The profile established reproducible correctness and resource baselines, but no cache-size win, persistence-accounting mismatch, crash-recovery failure, or actionable performance regression was demonstrated. No production source change is warranted and no optimization commit was forced.

Changed files in this handoff:

- this journal
- `agent-journal/uber-goal-state.md`

The only planned commit is a journal-only handoff snapshot. The next cycle must select a distinct catalog goal. A stronger follow-up for this goal would use the index benchmark approach from #35827 with many chained signed inputs/outputs, separate memory/disk runs, repeated samples, and a longer profile window before considering any code change.

### Artifact index

- Build: `/data/my_storage/tmp/rebuild-recovery-profile-cycle14/build/`
- Source chain: `/data/my_storage/tmp/rebuild-recovery-profile-cycle14/source/`
- Reindex copies: `low16-a/`, `low16-b/`, `high256/`
- Indexed copies: `indexes16/`, `indexes256/`, `rss-indexes16/`
- Crash copies: `crash/`, `crash-auto/`
- Raw logs and counters: `/data/my_storage/tmp/rebuild-recovery-profile-cycle14/logs/`
