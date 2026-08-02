# Long-Running Rebuild, Recovery, and Compaction Profiling

## Cycle 206 - full reindex and interrupted index restart

- Date: 2026-07-31 UTC
- Goal index: 21
- Slug: `rebuild-recovery-profile`
- Selector command/result: `shuf -i 0-98 -n 1` -> `21`
- Branch: `uber-cycle-206-rebuild-recovery-profile-20260731`
- Gate timestamp: `2026-07-31T11:11:46Z`
- Start HEAD: `528ba5696a1a9786befb1f8ab779b8d50df86d80`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `1202 42` (`git rev-list --left-right --count HEAD...origin/master`)
- Pre-cycle state SHA256: `edf283a2fe2c5c09cf0c934f37d149c4016c6c5527d5274ee29723b53f5de46c`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`

### Scope and exclusions

This cycle profiles a distinct current-source cell: full `-reindex` scans of block files, compared with `-reindex-chainstate`, followed by an interrupted `txindex`/`coinstatsindex` rebuild restarted without an explicit reindex. The correctness contract is exact chain tip, chain height, and synced index height after each run; the resource contract records wall time, task-clock/instructions/cache counters when available, database directory size, process high-water RSS, and `/proc/<pid>/io` counters. All data, ports, wallets, and logs are scratch-only.

The prior Cycle 14 cells are excluded: its 20,000 coinbase-only chainstate cache-size comparison, tx/coinstats index cache comparison, chainstate crash/restart, and single-sample RSS monitor. This cycle does not claim broad storage or performance conclusions from one synthetic workload. The current `index/base.cpp` restart-readiness changes and the current full-reindex path are evidence seeds, not assumed defects.

### Hypotheses

1. Full block-file reindex and chainstate-only reindex have a measurable, reproducible work split that is not represented by the startup log or final state alone.
2. Killing an indexed rebuild after chainstate reaches an intermediate height and restarting without `-reindex` either converges to the exact source tip with both indexes synced or exposes a readiness, stale-index, or recovery defect.
3. The index rebuild's persisted size, I/O counters, and high-water memory remain bounded and logically equivalent after interruption and restart.

### Planned evidence

Use a current-source optimized daemon in `/data/my_storage/tmp/cycle206-bitcoind-build/`, a deterministic regtest source chain under `/data/my_storage/tmp/cycle206-rebuild-recovery/`, separate copied datadirs for each run, explicit ports, and fixed `dbcache`/parallelism. Capture command lines, source tip/hash, timing, `perf stat` output where permitted, `/proc` samples, `du -sb`, logs, `getblockchaininfo`, and `getindexinfo`. A confirmed defect requires a failing-before state or recovery invariant plus a minimized deterministic schedule and a focused regression; a profile-only difference must be repeated and causally attributable before any optimization is considered.

### Build and source fixture

The current start HEAD was built with Clang 19, Release, Ninja, wallet/tests/bench/fuzz/IPC/ZMQ/GUI disabled, and `bitcoind` plus `bitcoin-cli` enabled:

```text
TMPDIR=/data/my_storage/tmp/cycle206-build-tmp cmake -S . -B /data/my_storage/tmp/cycle206-bitcoind-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DBUILD_FOR_FUZZING=OFF -DBUILD_UTIL=ON -DENABLE_WALLET=OFF -DENABLE_IPC=OFF -DWITH_ZMQ=OFF -DWITH_GUI=OFF
TMPDIR=/data/my_storage/tmp/cycle206-build-tmp CCACHE_DIR=/data/my_storage/tmp/cycle206-ccache cmake --build /data/my_storage/tmp/cycle206-bitcoind-build --target bitcoind bitcoin-cli -j2
```

All 298 Ninja steps completed. Existing diagnostics were limited to the libstdc++ deprecation in `node/eviction.cpp` and existing unneeded-declaration/member warnings in `signet.cpp` and `txgraph.cpp`. `bitcoind` SHA256: `e644e56a7609f1a3c0f54d1332bd9291684109cc3ef423499ead16f91ba7ae19`. `bitcoin-cli` SHA256: `d8499ef3a8d59cf7e85641c47f7bcc93a4d0b3dd4f1dc4736edac633160963d1`.

The source node used an explicitly created scratch datadir, regtest ports 18614/18615, no listeners or peers, `-dbcache=32`, `-par=1`, and no indexes. After RPC `setmocktime 1700000000`, 300 batches of 100 `generatetoaddress` calls used the fixed functional-test unspendable address. The stopped fixture was:

```text
height=30000
best=5a0fba1ea376063a4d73e5059ff68ad53c9035e94b01f8b84d2b724e0df18478
source bytes=25734508
```

The first source launch omitted `mkdir -p "$SRC"`; the daemon correctly rejected the nonexistent explicitly supplied datadir. It was rerun with the directory created. This was a harness invocation error, not a product finding.

### Full reindex versus chainstate-only reindex

Each authoritative run copied the stopped source datadir and used a foreground current-source daemon under `perf stat` with `-dbcache=32`, `-par=1`, no indexes, explicit ports, and either `-reindex=1` or `-reindex-chainstate=1`.

| Run | perf elapsed | task-clock | instructions | cache misses | max RSS/HWM | bytes before restart |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `full-reindex-fixed` | 232.817 s | 233589.85 ms | 2,028,908,781,497 | 2,293,914,447 | 75024/75024 KiB | 32789830 |
| `chainstate-reindex-fixed` | 2.007 s | 2334.92 ms | 8,693,805,932 | 5,756,662 | 74268/74268 KiB | 35327550 |

Both runs emitted an exact height-30,000 `UpdateTip` with best hash `5a0fba1ea376063a4d73e5059ff68ad53c9035e94b01f8b84d2b724e0df18478`, followed by `initload thread exit`. A clean restart of each completed copy, without either reindex flag, independently returned `blocks=30000`, `headers=30000`, and the same best hash. The authoritative task-clock ratio is 100.04 and the instruction ratio is 233.37. This is a measured block-file versus chainstate work split, not a defect or an optimization target by itself.

The first full-reindex probe was stopped after the wrapper's broken `/proc` default expression produced invalid metric warnings; its console ultimately reached the exact tip, but its counters were discarded. The corrected full rerun completed normally. The corrected wrapper's process sampling was sparse because RPC calls blocked while initialization was in progress; RSS/HWM are observed high-water values, not a statistically complete memory trace. `perf` was available; `iostat` and `strace` were not.

### Interrupted indexed rebuild and restart

The first indexed control completed too quickly for a height-based RPC trigger because the copied block index already covered height 30000; it was stopped cleanly and excluded from the crash result. The distinct `indexed-interrupt-2` run started from the source with `-txindex=1 -coinstatsindex=1 -dbcache=16 -par=1`, waited for `coinstatsidx thread start`, waited exactly one second, captured `getindexinfo`, and sent `SIGKILL` to the foreground daemon. The trigger recorded:

```text
timestamp=2026-07-31T11:36:31Z
delay_after_coinstats_thread_start=1s
txindex: synced=true, best_block_height=30000
coinstatsindex: synced=false, best_block_height=0
status=137
```

No `Shutdown done` line was emitted. The crash monitor observed 8 samples, max RSS/HWM 75356 KiB, 4096 read bytes, and 2936832 write bytes. This was a controlled process interruption, not a filesystem power-loss simulation.

Restarting that exact partial datadir without `-reindex`, with both index flags and the same cache/parallelism, converged independently:

```text
status=0 ready=1 wall_ms=4630 samples=48 max_rss_kb=76456 max_hwm_kb=76456
read_bytes=16384 write_bytes=17051648 bytes=36687109
blocks=30000 headers=30000 best=5a0fba1ea376063a4d73e5059ff68ad53c9035e94b01f8b84d2b724e0df18478
```

The restart `perf stat` recorded 4368.70 ms task-clock, 17,571,196,170 instructions, and 11,139,047 cache misses. Its log reported `txindex is enabled at height 30000`, `coinstatsindex is enabled at height 30000`, `initload thread exit`, and `Shutdown done`, with no project error/corruption/assertion diagnostic. The exact ready JSON is in `logs/indexed-restart-2.ready`.

### Independent functional controls and history

The current-source functional controls both passed:

```text
TMPDIR=/data/my_storage/tmp/cycle206-functional-tmp python3 test/functional/feature_reindex.py --configfile=/data/my_storage/tmp/cycle206-bitcoind-build/test/config.ini --tmpdir=/data/my_storage/tmp/cycle206-functional-reindex --cachedir=/data/my_storage/tmp/cycle206-functional-cache --randomseed=206021 --loglevel=INFO
TMPDIR=/data/my_storage/tmp/cycle206-functional-tmp2 python3 test/functional/feature_coinstatsindex.py --configfile=/data/my_storage/tmp/cycle206-bitcoind-build/test/config.ini --tmpdir=/data/my_storage/tmp/cycle206-functional-coinstats --cachedir=/data/my_storage/tmp/cycle206-functional-cache2 --randomseed=206022 --loglevel=INFO
```

`feature_reindex.py` covered repeated full/chainstate reindex, out-of-order blocks, and its interrupted index restart control. `feature_coinstatsindex.py` covered indexed versus non-indexed UTXO-set results, restart, full reindex, chainstate reindex, reorg, deactivation, and unclean restart. Both reported `Tests successful` and cleaned their scratch directories.

Recent local history made the restart cell high value: `991997bb41` resets index readiness during restart and adds `baseindex_tests`; adjacent commits cover synchronized chainstate publication, flush failure, cursor decoding, and repeated DB cursor resize cycles. These were checked as history seeds and no mismatch appeared in the exercised paths. No online claim or single profile sample was treated as an oracle.

### Verdict and handoff

Hypothesis 1 is confirmed only as a resource characterization: full block-file reindexing is approximately 100x task-clock and 233x instructions versus chainstate-only reindex for this 30,000 coinbase-only fixture. No incorrect state, unexplained error, or actionable performance regression was demonstrated. Hypotheses 2 and 3 are dismissed for a product defect on the deterministic interrupted-index schedule: partial `coinstatsindex` state reopened, converged, and matched the exact chain tip with bounded observed counters.

No production source or test change is warranted. A future distinct cell for this goal should use chained signed transactions with repeated disk-versus-memory samples, or controlled short-write/ENOSPC faults; it must not recycle this coinbase-only full/chainstate/index-restart cell. Raw artifacts remain under `/data/my_storage/tmp/cycle206-rebuild-recovery/`. No cycle-206 process remains running.

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

## Cycle 258 - transaction-heavy rebuild, recovery, and compaction profile

- Date: 2026-08-02 UTC
- Goal index: 21
- Slug: `rebuild-recovery-profile`
- Selector command/result: `shuf -i 0-98 -n 1` -> `21`
- Branch: `uber-cycle-258-rebuild-recovery-profile-20260801`
- Start HEAD: `34847fe1393830f640e4ebc1b0947d1448bcb4ea`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `1305 42` (`git rev-list --left-right --count HEAD...origin/master`)

### Scope and prior-cell exclusion

The prior Goal 21 records were read before selecting work. Cycle 14 and Cycle 206 used large coinbase-only chains and interrupted index rebuilds; this cycle used a smaller transaction-heavy regtest fixture with 4,021 non-coinbase transactions and 12,402 transaction outputs, then added a maintained signed-transaction crash campaign. The cycle also separated ordinary post-reindex behavior from the hidden forced-compaction control. No default datadir, wallet, key, production database, or live network was used.

The contracts were: every profile run must reach the fixture's exact height 303 and best hash `108d3f53d8dcfa91908741c3d1203087c87fee32a064951376784ce4c7f32e8b`; indexed runs must report both indexes at height 303 before stopping; compaction must have explicit start/finish evidence; and crash recovery must reproduce the maintained test's exact UTXO-set equality and recovery assertions.

### Build and fixture

The current source was built in the existing wallet-disabled Clang 19 Release daemon build. The first build invocation hit the host-only `/root/.cache/ccache/tmp` space problem; relocating `CCACHE_DIR` to `/data/my_storage/tmp/cycle258-ccache` completed the 80-step `ninja` build for `bitcoind` and `bitcoin-cli`. The final binaries were:

```text
bitcoind    9f0affef7e7c34dffeef51888f04d389199344f15a36aec7c586f759185dcc03
bitcoin-cli 1ae16d5719da905561e63bc7ca7e92a17efcc56e61e538e109b0f482c98e554b
```

The fixture was generated by `/data/my_storage/tmp/cycle258-profile-fixture.py` with fixed `randomseed=258001`, a scratch datadir, no peers, and the current build. Its authoritative output was:

```text
FIXTURE blocks=303 best=108d3f53d8dcfa91908741c3d1203087c87fee32a064951376784ce4c7f32e8b txouts=12402 txoutset_bytes=712724
```

The source regtest directory was copied to independent run directories under `/data/my_storage/tmp/cycle258/runs/`. The profile wrapper was `/data/my_storage/tmp/cycle258-profile-runner.py`; it used foreground `bitcoind` under `perf stat`, fixed ports, `-par=1`, isolated logs, exact RPC readiness, and `/proc` high-water sampling. The direct runner saw zero physical `read_bytes`/`write_bytes` deltas because the workload was page-cache resident; those counters are not treated as disk-I/O evidence.

### Reindex and cache matrix

All rows below reached height 303 and the exact fixture hash. Task-clock is the `perf` value in milliseconds; wall time includes the controlled RPC stop and shutdown. HWM is the observed `VmHWM` in KiB.

| Run | Mode/cache | Wall ms | Task-clock ms | Instructions | Cache misses | HWM KiB |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `chain16a` | chainstate / 16 MiB | 754.4 | 485.20 | 3,234,788,722 | 2,053,560 | 13,612 |
| `chain16b` | chainstate / 16 MiB | 806.6 | 490.47 | 3,280,292,375 | 2,037,129 | 13,688 |
| `chain16c` | chainstate / 16 MiB | 807.3 | 488.26 | 3,287,342,598 | 2,189,284 | 13,660 |
| `chain16d` | chainstate / 16 MiB | 806.1 | 499.17 | 3,278,098,875 | 2,193,653 | 13,744 |
| `chain256` | chainstate / 256 MiB | 807.0 | 488.72 | 3,212,498,200 | 2,018,780 | 13,764 |
| `full16` | full `-reindex` / 16 MiB | 821.4 | 525.72 | 3,475,054,414 | 3,580,646 | 13,684 |
| `indexed16` | existing chainstate plus tx/coinstats indexes / 16 MiB | 813.2 | 299.79 | 2,381,942,044 | 1,756,549 | 13,608 |

The four 16 MiB chainstate runs have task-clock median 489.365 ms and range 485.20-499.17 ms. The 256 MiB result is inside that spread and is not a reproducible cache-size win. Full reindex is measurably higher in this fixture, as expected from the block-file scan, but no correctness or regression signal appeared. The indexed run's log independently states `txindex is enabled at height 303` and `coinstatsindex is enabled at height 303`; the runner waited for both `synced` flags before stopping. Every run's public readiness JSON had `blocks=303`, `headers=303`, and the same best hash.

### Flush and compaction behavior

The ordinary post-reindex workload was tested separately because chainstate reindex runs are in IBD and do not take the random periodic-compaction path. A 1,000-block post-reindex run at 16 MiB reached height 1,303 in 3,343.8 ms and a 5,000-block run at 1 MiB reached height 5,303 in 16,285.7 ms. Both had `compaction_starts=0`; their logs contained only the final shutdown flush. The 5,000-block run emitted two final `FORCE_FLUSH` records and committed 5,000 changed outputs. This is a workload characterization and a limitation on natural-compaction coverage, not a product failure.

The hidden debug control was then used explicitly on a fresh copy:

```text
python3 /data/my_storage/tmp/cycle258-profile-runner.py \
  --name=forcecompact --datadir=/data/my_storage/tmp/cycle258/runs/forcecompact \
  --rpcport=25819 --dbcache=16 --reindex=none --force-compact \
  --output-dir=/data/my_storage/tmp/cycle258/results
```

With `-forcecompactdb=1`, the log recorded start/finish pairs for both `regtest/blocks/index` and `regtest/chainstate` (`database_compaction_starts=2`, `database_compaction_finishes=2`). It reached the exact fixture tip with wall 604.4 ms, task-clock 173.2 ms, 1,555,639,971 instructions, 1,677,170 cache misses, and 13,760 KiB HWM. Reopening the same compacted datadir without the force flag reached the same tip with wall 601.6 ms, task-clock 150.37 ms, and no further compaction. This independently checks persistence after compaction; it does not claim the random production compaction trigger was forced.

### Maintained crash and recovery campaign

The repository's `test/functional/feature_dbcrash.py` was run against the same current daemon build with fixed `randomseed=258002`, `portseed=25820`, `--nocleanup`, and scratch paths. Its existing design uses four disconnected nodes: one reference node, three nodes with `-dbcrashratio=8/16/24` and `-dbcache=4/8/16`. It prepares 5,000 UTXOs, creates 2,500 signed self-transfers per iteration, mines and sometimes reorgs, submits the blocks to each crashing node, restarts after failures, and compares `hash_serialized_3`.

The full 40-iteration run completed successfully. Its final log records:

```text
Prepped 5000 utxo entries
Restarted nodes: [10, 6, 4]; crashes on restart: 12
Tests successful
```

Thus every injected node restarted repeatedly, all three configurations experienced crashes during restart/recovery, and the final UTXO hashes matched the reference node. The retained scratch tree was 368,222,990 bytes, primarily debug logging; no node or profile process remained running. This is strong evidence for the tested partial-batch/recovery schedule, not a proof of all filesystem, power-loss, or arbitrary corruption schedules.

### Verdict and residual queue

The cache-size hypothesis is **dismissed for a defect or actionable optimization**: four repeated 16 MiB runs and one 256 MiB control agree within run noise and reach identical state. The full/index/compaction hypotheses are **dismissed for a defect**: all exact state and index contracts held, forced compaction completed on both databases, and its output reopened cleanly. The crash hypothesis is **dismissed for a defect** on the maintained 40-iteration schedule: 12 recovery-time crashes were exercised and final serialized UTXO state matched.

No production source or permanent test change is warranted. This cycle has no source finding and will close with a journal-only commit. The next Goal 21 cell, if selected again, must use a longer chained-output profile with repeated disk-versus-memory separation or controlled ENOSPC/short-write injection; it must not recycle the 303-block matrix or the same `feature_dbcrash.py` seed without a new source, filesystem, or toolchain condition. Raw JSON, perf records, logs, fixture generator, runner, and retained test data are under `/data/my_storage/tmp/cycle258/`.
