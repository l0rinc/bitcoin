# Disk I/O, persistence growth, and write-amplification audit

## Cycle 17

- Date: 2026-07-27 UTC
- Goal index: 24
- Slug: `disk-io-amplification`
- Selector command/result: `shuf -i 0-98 -n 1` -> `24`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at cycle start: `1926a4dbf612f3ce2fd43b61c0691360930a952f`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Scratch root: `/data/my_storage/tmp/disk-io-amplification-cycle17/`

### Scope and prior evidence

The campaign audited durable writers and storage-heavy workflows: chainstate and block/index flushes, LevelDB batches, txindex, txospenderindex, blockfilterindex, and the persistence contracts around restart and recovery. Peer-address persistence was excluded from the main hypothesis because cycle 16 already fixed the scheduler/shutdown `peers.dat` race.

Cycle 14 had already profiled a fixed 20,000-block chainstate and index rebuild at 16 MiB and 256 MiB caches, including an interrupted reindex and automatic restart. Both cache settings converged to the exact chain tip, and no optimization or persistence defect was proven. That journal identified the missing stronger experiment: blocks with many chained signed inputs and outputs, with separate disk and memory index runs.

The current source audit found the following contracts:

- `BaseIndex::Commit()` puts subclass state and the best-block locator into one `CDBBatch`, and skips a commit when the index is ahead of the last flushed chainstate. Its ordinary `WriteBatch()` is intentionally asynchronous because an index may safely recover by replaying missed work.
- `TxIndex::CustomAppend()` batches all transaction positions from one block into one LevelDB write.
- `TxoSpenderIndex::CustomAppend()` and `CustomRemove()` batch all spender keys from one block. Its bloom filter is disabled because lookups are prefix/range scans, and values are zero-byte markers by design.
- `BlockFilterIndex::CustomCommit()` commits the flat filter file before adding the durable filter position to the LevelDB batch. This ordering leaves extra file bytes harmless after a crash rather than publishing a database pointer to uncommitted filter data.
- `Chainstate::FlushStateToDisk()` flushes block and undo data, writes the block index, then flushes or syncs the coins cache and publishes `ChainStateFlushed`. No write-order or accounting violation was found in the inspected path.

### Independent benchmark seed and provenance

Open Bitcoin Core PR [#35827](https://github.com/bitcoin/bitcoin/pull/35827), head `4f79b7275ac78cf88341a83167067c4343270a8d`, supplied an independent benchmark implementation. Its selected diff against `origin/master` adds only `src/bench/index_sync_util.{h,cpp}`, realistic txindex/txospenderindex benchmarks, a transaction-heavy blockfilter benchmark, and documentation for measuring I/O and memory. The PR describes the existing coinbase-only blockfilter benchmark as unable to expose input/output scaling and adds disk-backed versus in-memory variants. The PR was open during this cycle, received a Concept ACK from `l0rinc`, and its author suggested splitting the documentation and benchmark changes. It was used as a seed, not treated as a correctness oracle, and no unpublished branch code was copied into this worktree.

The detached worktree at `/data/my_storage/tmp/disk-io-amplification-cycle17/pr/` was configured and built independently:

```text
cmake -S /data/my_storage/tmp/disk-io-amplification-cycle17/pr \
  -B /data/my_storage/tmp/disk-io-amplification-cycle17/build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang-19 \
  -DCMAKE_CXX_COMPILER=clang++-19 -DBUILD_TESTS=OFF -DBUILD_BENCH=ON \
  -DBUILD_FUZZ_BINARY=OFF -DBUILD_FOR_FUZZING=OFF -DBUILD_UTIL=OFF \
  -DENABLE_WALLET=OFF -DENABLE_IPC=OFF -DWITH_ZMQ=OFF -DWITH_GUI=OFF
cmake --build /data/my_storage/tmp/disk-io-amplification-cycle17/build \
  --target bench_bitcoin -j2
```

The build completed at 100%. The host filesystem was ext4 on `/data`, not tmpfs. CPU frequency scaling was enabled and the governor was `powersave`; all benchmark results retain nanobench instability warnings.

### Reproducible workload and results

The benchmark fixture builds 50 blocks with 50 chained, validly signed transactions per block, then syncs each index five or more times internally. Each command used a new scratch `-testdatadir` and `-min-time=1000`:

| Benchmark | Disk ns/op | Memory ns/op | Disk/memory | Disk footprint |
|---|---:|---:|---:|---:|
| `TxIndexSync` | 47,265,145.00 | 6,612,169.14 | 7.15x | 118,474 bytes |
| `TxoSpenderIndexSync` | 58,699,625.00 | 7,980,682.15 | 7.36x | 101,728 bytes |
| `BlockFilterIndexSyncRealistic` | 53,039,709.80 | 13,536,632.12 | 3.92x | 1,075,989 bytes |

The disk `TxIndexSync` repeat was 47,118,169.20 ns/op, within 0.4% of the first run. The blockfilter footprint includes the intentional 1 MiB `fltr00000.dat` allocation chunk; this is bounded file preallocation, not unexplained growth. Memory-only runs did not create LevelDB index directories.

The benchmark's internal assertions passed for every sync and lookup run. Warm lookup checks also passed:

```text
TxIndexLookup:          13,532.76 ns/lookup, 0.3% error
TxoSpenderIndexLookup:  14,142.92 ns/lookup, 0.8% error
```

For a sequential `perf stat` run of `TxIndexSync` with `-min-time=100`, the syscall counters were:

| Variant | `write` | `pwrite64` | `fsync` | `fdatasync` |
|---|---:|---:|---:|---:|
| Disk-backed | 9,789 | 0 | 0 | 236 |
| Memory-only | 408 | 0 | 0 | 0 |

These counters include the test fixture's block and chainstate setup as well as index work, so they are attribution evidence rather than an exact per-index syscall count. The large delta, nearly identical instruction counts, and stable disk/memory timing ratio establish that the benchmark is exercising the intended persistence path.

Representative commands:

```text
build/bin/bench_bitcoin -filter='TxIndexSyncDisk' -min-time=1000 -testdatadir=...
build/bin/bench_bitcoin -filter='TxIndexSyncMem' -min-time=1000 -testdatadir=...
perf stat -e syscalls:sys_enter_write,syscalls:sys_enter_pwrite64,syscalls:sys_enter_fsync,syscalls:sys_enter_fdatasync \
  build/bin/bench_bitcoin -filter='TxIndexSyncDisk' -min-time=100 -testdatadir=...
du -sb .../datadir/regtest/indexes/txindex
```

### Hypotheses and verdicts

1. **Per-block index writes omit batching or create avoidable write amplification.** Dismissed. Each index batches all records for a block; the measured disk cost is consistent with the intended LevelDB path, and no duplicated record or unbounded write pattern was observed.
2. **Index persistence can publish state out of order or lose required recovery data.** Dismissed for the tested paths. The source contract keeps subclass state and the locator in one batch and blocks commits ahead of the flushed chainstate. The blockfilter file/database ordering is conservative. This cycle did not reproduce a crash or restart failure.
3. **The blockfilter index has unexplained persistence growth.** Dismissed. The approximately 1 MiB footprint is the documented fixed filter-file chunk, while the LevelDB log was 19,155 bytes for the fixture.
4. **The repository lacks an input/output-realistic index I/O benchmark.** Confirmed as a testing/observability gap in the current tree: its existing blockfilter sync benchmark is coinbase-only and there are no txindex or txospenderindex benchmarks. The gap is already addressed by open PR #35827, so this cycle does not duplicate that active contribution or create a speculative local patch.

### Limitations and handoff

No `strace` or `iostat` is installed. `perf` tracepoints were available, but the syscall counts include setup work. Measurements are one-host, short, and affected by powersave/turbo behavior; they are not a performance claim for all hardware. The external PR was built separately from the current source branch, and its benchmark code was not merged or committed locally. A broader crash-consistency schedule, larger transaction graph, and repeated benchmark matrix remain useful if new evidence points there.

**Cycle verdict: dismissed for a confirmed production defect; confirmed benchmark coverage gap already represented by PR #35827.** No production source change is warranted. This cycle leaves a journal-only handoff snapshot. The next run must re-check the worktree and processes, draw a distinct catalog goal with `shuf -i 0-98 -n 1`, and record the new risk-map cell before investigating it.
