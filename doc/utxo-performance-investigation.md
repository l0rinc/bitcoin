# UTXO performance investigation log

This file records the performance-mining work on branch
`perf-utxo-rpc-hdd-investigation`. It is an experiment log, not a claim that
every result generalizes to every machine. Keep it updated in every subsequent
commit on this branch so accepted changes, rejected hypotheses, raw evidence,
and unresolved questions remain retriggerable.

Unless stated otherwise, the end-to-end measurements used a GCC Release build
on an Intel Core i7-7700 (8 logical CPUs), 62 GiB RAM, Linux, ext4, and
rotational storage. Daemons ran with networking and wallet activity disabled
against an explicit copy-on-write OverlayFS scratch datadir over the mainnet
data. The source datadir was not modified. Short benchmarks require at least
five runs; expensive full-chain runs are paired with a repeatable focused
benchmark when possible.

## Accepted changes

### `ceb0979096` coinstats: streamline un-hashed UTXO stats

`gettxoutsetinfo` with `hash_type=none` only needs counts, amounts, bogosize,
and transaction/output totals. The generic implementation nevertheless built
a `std::map` of every transaction's outputs for a hash that this mode never
computes. The change specializes the no-hash path and accumulates the same
statistics directly while advancing the ordered coins cursor.

Full mainnet chainstate, warm cache, five runs:

| version | median | range |
| --- | ---: | ---: |
| base | 67.155 s | 66.980-75.774 s |
| candidate | 57.571 s | 57.496-58.232 s |

The median improved by 14.3%. The functional test compares the no-hash output
to the default result after removing only fields that intentionally differ.
Raw results are
`/mnt/my_storage/bitcoin-perf-scratch/gettxoutsetinfo-none-{baseline,candidate}-warm.json`.

Validation:

```text
test/functional/rpc_blockchain.py --configfile=/mnt/my_storage/bitcoin/build/test/config.ini --tmpdir=/mnt/my_storage/bitcoin-perf-scratch/functional/rpc_blockchain --timeout-factor=2
ninja -C build bitcoind bitcoin-cli -j8
```

Reach: all live `gettxoutsetinfo hash_type=none` scans. Hash-producing modes,
indexes, and snapshot commitments are unchanged. The measured percentage is a
warm-scan CPU result; cold HDD latency can reduce its wall-time share.

### `20e73d97fb` rpc: speed up scantxoutset script lookups

`scantxoutset` tested every UTXO script against a descriptor-derived
`std::set`, making negative matches tree walks. The change uses the existing
salted script hasher in an `unordered_set` and rejects script sizes absent from
the needle set before hashing. Exact `CScript` equality remains the final
oracle.

Full chainstate at height 957779, 166,350,731 UTXOs, one combo descriptor with
range 1000, warm cache, five runs:

| version | median | standard deviation |
| --- | ---: | ---: |
| base | 73.662 s | 0.189 s |
| candidate | 69.595 s | 0.457 s |

The median improved by 5.5%. A five-second warmed profile recorded 45.6
billion instructions, 2.54 IPC, 22.6 million cache misses, and no page faults,
confirming a CPU-side lookup workload. Raw results are
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-{baseline,candidate}.json`.

Validation:

```text
ninja -C build bitcoind bitcoin-cli -j8
test/functional/rpc_scantxoutset.py --configfile=/mnt/my_storage/bitcoin/build/test/config.ini --tmpdir=/mnt/my_storage/bitcoin-perf-scratch/functional/rpc_scantxoutset --timeout-factor=2
```

Reach: every `scantxoutset` UTXO lookup. The size prefilter helps most when the
descriptor expansion produces relatively few script lengths; cold scans can
remain storage-bound.

### `736f48b4da` coinstats: stream ordered cursor entries while hashing

The coins DB cursor already returns a transaction's outputs consecutively in
output-index order. `ComputeUTXOStats` repeated that ordering by allocating,
copying, traversing, and erasing a `std::map` node per UTXO. The change makes
the ordering contract explicit, tests it across VarInt boundaries, and streams
cursor entries directly into statistics and the selected hash.

Full chainstate at height 957779, 166,350,731 UTXOs,
`hash_serialized_3`, warm cache, five runs:

| version | median | range |
| --- | ---: | ---: |
| `20e73d97fb` | 123.486 s | 123.274-123.572 s |
| candidate | 106.357 s | 106.238-106.398 s |

The median improved by 13.9%. Attached profiles recorded approximately 436.7
billion base cycles and 369.5 billion candidate cycles. Red-black-tree
insertion, allocation, erase, and map hashing disappeared from the candidate.
Both scans read zero storage bytes after warmup, and the complete RPC JSON was
byte-for-byte equal, including serialized hash
`ed1ed399ab6c15d571999b81cdabe07de50e7b1b58f5e07ab29c2770fbf1d2b3`.
Raw hyperfine files are
`/mnt/my_storage/bitcoin-perf-scratch/gettxoutsetinfo-hash-{baseline,candidate}.json`.

Validation:

```text
build/bin/test_bitcoin --run_test=coins_tests_dbbase
build/test/functional/test_runner.py feature_utxo_set_hash.py rpc_dumptxoutset.py --jobs=1
build/test/functional/test_runner.py rpc_blockchain.py --jobs=1
```

Reach: all `ComputeUTXOStats` modes and users, including
`gettxoutsetinfo`, snapshot/dump calculations, and serialized hashing. The
percentage is a warm CPU result; ordered cold scans may be more I/O-bound.

### `6410472950` coinstats: reserve MuHash serialization buffer

`ApplyCoinHash` and `RemoveCoinHash` created a fresh `DataStream` per coin and
grew it repeatedly while appending the outpoint, code, amount, and script. The
change reserves the exact serialized size before emitting the unchanged
commitment bytes. Insert and removal paths are both covered; `HashWriter` does
not own an intermediate vector and is intentionally untouched.

A temporary production-helper benchmark used a representative 22-byte script,
was pinned to CPU 3, and ran five times for at least five seconds:

| version | median | range | instructions/coin | branches/coin |
| --- | ---: | ---: | ---: | ---: |
| base | 4240.49 ns | 4237.44-4246.33 ns | 49,260.51 | 2,888.50 |
| candidate | 4161.10 ns | 4160.26-4177.14 ns | 48,670.51 | 2,761.50 |

The focused median improved by 1.87%; instructions fell 1.20% and branches
4.40%. Raw results are
`/mnt/my_storage/bitcoin-perf-scratch/muhashcoin-reserve-{baseline,candidate}-{1..5}.json`.

One full warm-cache MuHash scan of 166,350,731 UTXOs took 781.000 s on the
base and 758.816 s on the candidate, a 2.84% improvement. Raw results are
`/mnt/my_storage/bitcoin-perf-scratch/gettxoutsetinfo-muhash-{baseline,candidate}-full.json`.

Validation:

```text
build/bin/test_bitcoin --run_test=crypto_tests/muhash_tests
build/bin/test_bitcoin --run_test=coinstatsindex_tests
build/bin/test_bitcoin --run_test=coins_tests_dbbase
build/test/functional/test_runner.py feature_utxo_set_hash.py feature_coinstatsindex.py rpc_dumptxoutset.py rpc_blockchain.py --jobs=1
```

Reach: every coin inserted into or removed from a MuHash commitment, including
live MuHash RPC scans, coinstats-index maintenance and rollback, and snapshot
commitment generation/validation. It does not affect `hash_serialized_3` or
`hash_type=none`. Only one full scan was practical; the five-run focused test
supplies repeatability.

### LevelDB: recover readahead for contiguous mmap reads

Master commit `e3ec270a39` applies `MADV_RANDOM` to every successful read-only
table mmap. It fixed a resumed prune-assumevalid workload whose old-chainstate
point reads took 771 s without the hint and 177 s with it while avoiding
hundreds of MiB of swap. The user's full HDD reindex-chainstate benchmark at
height 957759 and `-dbcache=2000` exposed the opposing access pattern:

| commit | mean | range | user | system |
| --- | ---: | ---: | ---: | ---: |
| `cf0368fb76` before annotation | 27,110.965 s | 26,768.769-27,453.162 s | 40,276.824 s | 1,380.273 s |
| `e3ec270a39` annotated | 48,554.901 s | 47,760.373-49,349.430 s | 39,968.920 s | 1,912.711 s |

The global annotation made that reindex 1.79 times slower with similar user
CPU, implicating lost HDD readahead rather than additional validation work.
It could not simply be reverted without restoring the much larger random-read
regression.

Temporary offset instrumentation in `PosixMmapReadableFile::Read` established
that the same mmap abstraction serves two different workloads:

| workload | mapped files | reads | far | forward-near | exactly contiguous |
| --- | ---: | ---: | ---: | ---: | ---: |
| established SSTs, normal compaction | 547 | 528,063 | mixed | 89.825% | 89.445% |
| established SST point-read control, compaction suppressed | 480 | 55,869 | 74.937% | 3.257% | 0.465% |
| fresh reindex to height 287000 | 129 | 5,073,115 | 80.507% | 15.415% | 14.064% |

The first row was dominated by automatic compaction. Suppressing it only in a
temporary benchmark binary proved old-table point reads are genuinely random.
The fresh rebuild remained mostly random but contained millions of contiguous
reads from scans and compaction. The instrumentation and the temporary
compaction gate were fully removed; raw traces remain in
`/mnt/my_storage/bitcoin-perf-scratch/mmap-trace-established/`,
`mmap-trace-established-noauto/`, and
`reindex-writebuf/mmap-fresh-1/metrics/mmap-trace.tsv`.

The change retains `MADV_RANDOM`, atomically recognizes adjacent reads within
each mapped table, and issues a best-effort `MADV_WILLNEED` only when the reads
cross into a new 128 KiB window. This matches the test HDD's kernel readahead
unit, avoids changing persistent advice on a mapping shared by concurrent
readers, and bounds damage from coincidental adjacency. Platforms without
both advice constants compile the behavior out.

Current RocksDB was inspected only after the traces proved that Core cannot
see the per-SST offset locality. Its design supports the same conclusion:
[`advise_random_on_open`](https://github.com/facebook/rocksdb/blob/2809ce9d0f391c1c6499cb16e0740b37922e0af0/include/rocksdb/options.h)
remains enabled, while
[`BlockPrefetcher`](https://github.com/facebook/rocksdb/blob/2809ce9d0f391c1c6499cb16e0740b37922e0af0/table/block_based/block_prefetcher.cc)
starts iterator readahead only after sequential block reads and uses a separate
2 MiB compaction setting recommended for spinning disks. RocksDB's
[`FilePrefetchBuffer`](https://github.com/facebook/rocksdb/blob/2809ce9d0f391c1c6499cb16e0740b37922e0af0/file/file_prefetch_buffer.h)
explicitly does not support mmap readers, so importing that substantially
larger buffering subsystem was rejected. The bounded mmap hint is the
applicable subset, and 128 KiB remains conservative because this layer cannot
label a read as compaction.

Cold-HDD reindex to height 287000, `-dbcache=450`, fresh OverlayFS upperdir and
dropped page cache for every run:

| version/run | wall | user | system | peak RSS | major faults |
| --- | ---: | ---: | ---: | ---: | ---: |
| base 1 | 586.56 s | 468.87 s | 62.22 s | 1,538,628 KiB | 33,736 |
| candidate 1 | 578.11 s | 469.54 s | 61.37 s | 1,610,768 KiB | 1,006 |
| candidate 2 | 581.00 s | 468.48 s | 61.79 s | 1,605,760 KiB | 1,003 |
| base 2 | 590.85 s | 470.34 s | 62.31 s | 1,602,960 KiB | 33,732 |

The base median was 588.705 s and candidate median 579.555 s, a 1.55%
improvement; paired changes were -1.44% and -1.67%. Major faults fell by about
97% while total filesystem input remained unchanged, demonstrating readahead
fault clustering rather than skipped work. All runs reached height 287000 and
shut down cleanly. Raw results are under
`/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/madv-{baseline,candidate}-{1,2}/metrics/`.

An established-SST control processed the same 20 blocks from height 957759
with automatic compaction suppressed in both temporary binaries. This isolates
the random point-read case that motivated `MADV_RANDOM`:

| version | wall times | median | median major faults | median input blocks |
| --- | --- | ---: | ---: | ---: |
| base | 142.38, 143.65, 143.38, 143.31, 143.83 s | 143.38 s | 184,709 | 1,751,848 |
| candidate | 137.67, 139.10, 138.80, 140.36, 138.96 s | 138.96 s | 151,914 | 1,751,504 |

The candidate improved the five-run median by 3.08% and reduced major faults
17.75%, with unchanged I/O, RSS, and output. Thus it preserves the random hint
instead of trading the old regression for the reindex improvement. Raw results
are under
`/mnt/my_storage/bitcoin-perf-scratch/madv-point-control/{baseline,candidate}-{1..5}/metrics/`.

Validation:

```text
ninja -C build bitcoind test_bitcoin -j1
ctest --test-dir build --output-on-failure -R '^(dbwrapper_tests|coinsviewoverlay_tests|coinsviewoverlay_tests_noworkers)$'
cmake -S src/leveldb -B <scratch> -G Ninja -DCMAKE_BUILD_TYPE=Release -DLEVELDB_BUILD_TESTS=ON -DLEVELDB_BUILD_BENCHMARKS=OFF
env_posix_test  # with the test shell's fd limit set to 1024: 7/7 passed
env_test        # 7/7 passed
build/test/functional/feature_reindex.py --tmpdir=<nonexistent-scratch-path> --nocleanup
```

Reach: every 64-bit POSIX LevelDB mmap user can benefit when reads within an
SST become contiguous, including reindex-chainstate, compaction, ordered UTXO
scans (`gettxoutsetinfo`, `scantxoutset`, and `dumptxoutset`), and index builds.
Pure random reads retain the global random advice and normally only pay one
relaxed atomic exchange. The measured gains are specific to this rotational
device. A repeat of the user's height-957759 benchmark is still needed to
quantify full-chain reach; the shorter end-to-end result and five-run old-SST
control establish direction and protect the original workload.

## Rejected changes

### LevelDB block cache and write-buffer allocation ([PR #194](https://github.com/l0rinc/bitcoin/pull/194), [PR #203](https://github.com/l0rinc/bitcoin/pull/203))

Hypothesis: on 64-bit POSIX, uncompressed mmap-backed SST blocks are returned
with `cachable=false`, so half of the LevelDB cache allocation is normally
unused. Disable the block cache and give the two possible write buffers the
full budget. The temporary production diff used a zero-capacity block cache on
64-bit and doubled `write_buffer_size`; 32-bit behavior was unchanged.

The LevelDB contract supports the mechanism: mapped uncompressed reads point
directly into the mmap and explicitly avoid double caching. The production
chainstate had 480 SST files, below the approximately 990 table/mmap capacity.
Nevertheless, the end-to-end effect was neutral.

Local cold-HDD OverlayFS reindex to height 287000 with `-dbcache=450`:

| version/run | wall | user | system | peak RSS |
| --- | ---: | ---: | ---: | ---: |
| base 1 | 584.621 s | 470.88 s | 62.94 s | 1,610,948 KiB |
| candidate 1 | 582.248 s | 470.65 s | 62.00 s | 1,598,332 KiB |
| base 2 | 584.610 s | 468.26 s | 60.97 s | 1,635,248 KiB |

The user's stronger two-run full-chain HDD experiment used
`-reindex-chainstate -stopatheight=900000 -dbcache=4000`:

| commit/shape | mean | range |
| --- | ---: | ---: |
| `ef101b04a8` base | 21,829.557 s | 21,514.603-22,144.510 s |
| `78fa6e39e8` cache-allocation revert | 22,014.138 s | 21,958.993-22,069.283 s |
| `ba30dc7600` no block cache/default write buffer | 21,834.785 s | 21,825.790-21,843.780 s |
| `b4ba9ee596` cleanup endpoint | 21,905.920 s | 21,873.741-21,938.100 s |

The closest variant differs from base by only 0.024%; all ranges overlap the
base's variability. Decision: no commit. Cache-budget reshuffling should not be
retried without a profile showing a changed non-mmap or write-stall bottleneck.
Local raw logs remain under
`/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/`.

### `dumptxoutset` caller-owned stdio buffer

Hypothesis: the snapshot writer's small stdio writes cause avoidable syscall
overhead on HDD. A temporary 1 MiB caller-owned buffer reduced writes for a
9.50 GB full snapshot from 2,319,261 to 9,064. Passing a null buffer to
`setvbuf` did not reduce calls and regressed, so only the owned-buffer form was
evaluated further.

Five full HDD runs:

| version | total times | median total | median write phase |
| --- | --- | ---: | ---: |
| base | 221.27, 224.25, 229.79, 249.383, 252.361 s | 229.79 s | 118.96 s |
| candidate | 221.99, 225.71, 227.71, 233.29, 235.082 s | 227.71 s | 119.31 s |

Despite 256 times fewer writes, total time improved only 0.91% with heavily
overlapping ranges, while the directly affected write phase regressed 0.29%.
Decision: no commit; syscall-count movement did not produce an outside-noise
HDD speedup. Raw logs and the focused fixture remain under
`/mnt/my_storage/bitcoin-perf-scratch/dumptxoutset-buffer-*`.

### Cached hash inside mutable `COutPoint` ([PR #162](https://github.com/l0rinc/bitcoin/pull/162))

Hypothesis: store the outpoint hash to avoid repeated SipHash work in coins
cache lookups. Rejected before benchmarking because `COutPoint::hash` and `n`
are public and mutable, with no universal invalidation point. A cached value can
survive mutation and can also be reused between deterministic and randomized
hasher instances. Equal keys could therefore produce unequal or stale hash
codes, violating the unordered-container contract. Decision: correctness
failure; do not retry without an immutable key representation or cache owned by
the container rather than `COutPoint`.

### `SpendCoin` cached-lookup fast path ([PR #34](https://github.com/l0rinc/bitcoin/pull/34))

Hypothesis: validation normally accesses each input before spending it, so
`SpendCoin` can call `cacheCoins.find()` first and avoid the heavier
`try_emplace()` path inside `FetchCoin()` on hits. Misses retain the original
fallback but pay one extra lookup.

A temporary benchmark populated 10,000 deterministic cached coins during
untimed per-epoch setup and timed only the 10,000 `SpendCoin` hits. Seven runs:

| version | median | range | instructions/coin | cycles/coin | branches/coin |
| --- | ---: | ---: | ---: | ---: | ---: |
| base | 151.63 ns | 151.26-156.63 ns | 1,080.43 | 543.95 | 53.21 |
| candidate | 149.78 ns | 149.37-150.50 ns | 1,052.43 | 537.30 | 49.21 |

The focused hit operation improved 1.22% with the expected instruction and
branch reduction. The existing ConnectBlock benchmark was unsuitable as the
sole verifier: it produced only about three timed blocks per process and
shifted between roughly 74 and 107 ms as machine state changed.

The controlled cold-HDD OverlayFS reindex used height 287000,
`-dbcache=450`, identical separately built binaries, a fresh overlay, and a
dropped page cache for every run. All runs reached height 287000 and shut down
cleanly:

| version/run | wall | user | system | peak RSS | major faults |
| --- | ---: | ---: | ---: | ---: | ---: |
| base 1 | 578.62 s | 470.24 s | 62.47 s | 1,609,436 KiB | 33,736 |
| candidate 1 | 577.68 s | 469.00 s | 62.04 s | 1,635,756 KiB | 33,736 |
| candidate 2 | 581.55 s | 468.18 s | 62.34 s | 1,632,032 KiB | 33,734 |
| base 2 | 581.85 s | 470.95 s | 62.94 s | 1,611,084 KiB | 33,738 |

The two-run medians are 580.235 s base and 579.615 s candidate, only 0.107%
apart. Paired changes are -0.162% and -0.052%. Process-wide instructions moved
by less than 0.07%, and I/O/fault counts were effectively unchanged. Decision:
no commit; the real local micro improvement has negligible reach in HDD
reindex and does not justify making uncached calls more expensive. Raw focused
results are
`/mnt/my_storage/bitcoin-perf-scratch/spendcoin-fastpath/micro-{baseline,candidate}-{1..7}.txt`;
full metrics are under
`/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/spend-{baseline,candidate}-{1,2}/metrics/`.

## Open investigation

### Remaining PR seeds

The following seeds were screened but have not yet produced an accepted or
fully rejected independent change:

- [#48](https://github.com/l0rinc/bitcoin/pull/48)/[#68](https://github.com/l0rinc/bitcoin/pull/68): input/prevout prefetch variants. Current master already has eight
  prevout-fetch workers; only a measured change in ordering, batching, or HDD
  reads merits another commit.
- [#34](https://github.com/l0rinc/bitcoin/pull/34): repeated coins-cache lookup/hash and short-lived reallocation. The
  `SpendCoin` subcandidate is rejected above; other subpatterns remain open.
- [#77](https://github.com/l0rinc/bitcoin/pull/77): contiguous-deserialization zero-fill avoidance. It needs a production
  profile and a faithful snapshot/reindex confirmation before reconsideration.
- [#132](https://github.com/l0rinc/bitcoin/pull/132)/[#136](https://github.com/l0rinc/bitcoin/pull/136): Bloom-filter and restart-interval tuning. Point reads and ordered
  scans have opposing tradeoffs; test both before changing defaults.
- [#140](https://github.com/l0rinc/bitcoin/pull/140)/[#59](https://github.com/l0rinc/bitcoin/pull/59): UTXO allocation/layout/reservation variants. Confirm cache memory,
  allocator, and reindex effects independently.
- [#180](https://github.com/l0rinc/bitcoin/pull/180): input fetching, no-seek compaction, and flush timing. Split into minimal
  hypotheses; do not carry experimental logging or policy changes into a
  performance commit.
- [#195](https://github.com/l0rinc/bitcoin/pull/195)/[#152](https://github.com/l0rinc/bitcoin/pull/152): cursor/coinstats ideas. Their proven portions are represented by
  the accepted direct-streaming and hashing commits above; remaining layering
  changes need independent evidence.
- [#200](https://github.com/l0rinc/bitcoin/pull/200)/[#203](https://github.com/l0rinc/bitcoin/pull/203): mmap/open-file/cache-threshold variants. Existing and current
  cache/write-buffer experiments are neutral or harmful; reopen only if a new
  profile demonstrates a different bottleneck.
