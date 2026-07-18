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
data. The source datadir was not modified. The user later explicitly permitted
read-only RPC profiling against `/mnt/my_storage/BitcoinData`; those entries
name that datadir and start a network-disabled, wallet-disabled daemon which is
cleanly stopped after each series. Short benchmarks require at least five runs;
expensive full-chain runs are paired with a repeatable focused benchmark when
possible.

Exact fixes that have already been pushed to
[`bitcoin/bitcoin`](https://github.com/bitcoin/bitcoin) are out of scope for
new commits on this branch. They may motivate a distinct local hypothesis, but
are screened before implementation so this branch does not duplicate an
upstream proposal.

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

### `dbwrapper`: decode unobfuscated iterator values directly

`CDBIterator::GetValue` used the reusable `DataStream` for every value: it
copied the LevelDB value, applied the database obfuscation transform in place,
then deserialized it. That work is required for normal chainstate, which has a
non-empty obfuscation key. It is needless for a database whose key is empty:
the iterator's stable value span can instead be deserialized directly through
`SpanReader`. The existing scratch-buffer path remains exactly in place for
obfuscated databases, including normal chainstate and every caller that
requires de-obfuscation.

This is related to [bitcoin/bitcoin#35247](https://github.com/bitcoin/bitcoin/pull/35247),
but it is not an already-pushed upstream fix: GitHub API check on 2026-07-19
reported that PR as `state=closed`, `closed_at=2026-05-08T22:18:26Z`, and
`merged_at=null`. The local implementation and current tests were revalidated
against this tree rather than cherry-picking it. This satisfies the branch rule
to ignore already-pushed bitcoin/bitcoin fixes.

The concrete important caller is rollback-mode `dumptxoutset`. It creates an
explicitly non-obfuscated temporary UTXO database, then scans it once to
compute stats and once to serialize the snapshot. A cold-cache full control
pair used the permitted `/mnt/my_storage/BitcoinData` at height 957779;
`rollback=957778`, `in_memory=true`, network and wallet disabled, CPU 3,
`sync; echo 3 > /proc/sys/vm/drop_caches` before each daemon, and a FIFO
consumed by `cat` so snapshot output did not reach disk. The daemon's
`perf stat` began immediately before the RPC and stopped after its response.
Both complete RPC responses agreed on `coins_written=166350403`, height
957778, base hash
`0000000000000000000087c7fed547001e850e5b350110fc2c82ca19cc4bdb6d`,
`nchaintx=1395785816`, and txoutset hash
`8b503452cc691d72837c5580f0c6140b833acb855a15c1ae576f61bfdf0c55ca`.

| phase or counter | reusable-stream base | direct-span candidate | change |
| --- | ---: | ---: | ---: |
| RPC wall time | 718.28 s | 704.01 s | -1.99% |
| daemon task time | 706.874 s | 692.783 s | -1.99% |
| daemon instructions | 5.866 T | 5.758 T | -1.83% |
| daemon branches | 1.074 T | 1.052 T | -2.11% |
| current-chainstate copy | 510 s | 515 s | +0.98% |
| temporary-DB statistics scan | 113 s | 94 s | -16.8% |
| temporary-DB snapshot scan | 95.17 s | 94.29 s | -0.92% |

The unchanged, intentionally obfuscated chainstate copy is the dominant
portion and varies by five seconds in this pair. The direct non-obfuscated
statistics scan nevertheless removes 19 seconds, and total daemon work falls
by 14.091 seconds with the expected instruction/branch direction. The snapshot
scan is near run-to-run noise, so the documented claim is the robust stats-pass
improvement and the measured 1.99% full-RPC result, not a broad LevelDB or HDD
claim. Raw candidate artifacts (including the temporary source diff) are
`/mnt/my_storage/bitcoin-perf-scratch/dumptxoutset-getvalue-noobf.candidate-full2.iBqI3n/`;
raw reverted-base artifacts are
`/mnt/my_storage/bitcoin-perf-scratch/dumptxoutset-getvalue-noobf.base-full1.M6iidb/`.
The candidate source-diff, result, debug-log, time, and perf SHA-256 values are
respectively
`6708dadd0b253dff0161e060266a3d716351c22db2a3d76e0fc92a5c87277614`,
`10e25384cfee02bd898042ff5d9bd5f29f00a35880c032829aa0f73a507943f3`,
`b82f4711514e4b8877eaa0dee303e8e3051b0e589f2abfcf8f4d64ad6fc408cd`,
`e161b82c3347f9daa0f623cae6f00b89461a8e6c7460c8b9782764d786fd9bdb`, and
`5e3e870fe6ccb44cbf18a3d9403b5f965976b3823ebed283c5a24e41eb83e38c`.

Correctness is covered by `dbwrapper_tests`: it runs both obfuscated and
non-obfuscated databases, attempts an oversized value decode, then decodes
the same iterator value successfully. That proves a failed direct
`SpanReader` decode leaves the iterator usable and preserves the existing
failure contract. Validation:

```text
ninja -C build bitcoind bitcoin-cli test_bitcoin -j4
build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=test_suite
build/test/functional/test_runner.py rpc_dumptxoutset.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-getvalue-unobfuscated --timeout-factor=2
```

Reach: only iterator reads from a deliberately unobfuscated `CDBWrapper`.
The measured path is the temporary in-memory DB used by `dumptxoutset
rollback`. The default block-tree DB and `BaseIndex` databases are also
unobfuscated and have iterator callers, so startup, reindex block-index work,
and index scans may benefit; this commit makes no unmeasured percentage claim
for them. `gettxoutsetinfo`, `scantxoutset`, regular `dumptxoutset`, and the
chainstate portion of reindex-chainstate keep their obfuscation key and
therefore their exact existing copy/de-obfuscate path. This is a direct Core
wrapper specialization, not a LevelDB change.

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

### `scantxoutset`: avoid unnecessary cached-key copies

`CCoinsViewDB::Cursor()` and the resulting cursor's `Next()` already
deserialize each coins DB key into its internal `keyTmp` cache before reporting
the cursor as valid.
`FindScriptPubKey` then copied that cached `COutPoint` into a local variable
for every UTXO, even though it only needs a key to update the advisory progress
field or to return an actual script match. The change reads the value first,
copies the cached key only at the existing 8192-entry interruption cadence to
update progress, and copies it for a matching result. A valid DB cursor
therefore preserves the previous key-validation invariant; the scan still
fails if a required cached key cannot be read.

Full local chainstate at height 957779, 166,350,731 UTXOs, one `combo`
descriptor with range 1000 and no matches, warm cache, five runs:

| version | median wall | range | median daemon task time | median instructions | median branches |
| --- | ---: | ---: | ---: | ---: | ---: |
| base | 70.35 s | 70.24-70.40 s | 70.286 s | 649.061 B | 130.590 B |
| candidate | 65.20 s | 65.14-65.42 s | 65.182 s | 587.216 B | 118.604 B |

The median wall time improved 7.32%, daemon task time 7.26%, instructions
9.53%, and branches 9.18%. Both versions had 549-550 minor faults and zero
major faults per run, so this is a warm CPU-side benefit. All ten response
JSON files have SHA-256
`62539678afd6931c917d0135fedcc12d3f11e9e40958fe4983e461a5f0d0891a`.
The 8192-entry progress cadence is about 3.2 ms on this workload, far below
human or RPC polling resolution, and matches the pre-existing interruption
cadence. Raw command outputs and `perf stat` files are under
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-key-copy/{baseline,candidate}/`.

Validation:

```text
build/test/functional/test_runner.py rpc_scantxoutset.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-scantxoutset-key-copy --timeout-factor=2
```

Reach: every `scantxoutset start` UTXO. The largest gain is for sparse or
negative descriptor matches, where nearly every cached key copy disappears.
Denser matches retain one key copy per returned UTXO; cold scans remain
storage-bound.

### `scantxoutset`: defer unneeded cursor outpoint materialization

The preceding `scantxoutset` improvement stopped copying the already cached
`COutPoint` into the RPC loop, but `CCoinsViewDBCursor::Next()` still decoded
and copied every next key into that cache. In a sparse or negative scan, the
next key is only needed at the existing 8192-entry progress cadence or for a
matching result. This change adds `NextNoKey()` with a safe default and uses it
only after a nonmatching scan entry. The DB cursor still reads and validates
the key tag, the complete 32-byte txid span, and the output-index VarInt; it
simply does not copy the txid into `keyTmp` until `GetKey()` is requested.

The existing eager path is retained for `Next()` and all full-key users, so
`gettxoutsetinfo`, `dumptxoutset`, snapshot construction, and cursor callers
that need every key do not take the lazy path. If a caller later requests a
key after `NextNoKey()`, the DB cursor decodes it then and caches it. The
focused DB cursor test advances with `NextNoKey()` and obtains the next key on
every iteration, covering both the deferred decode and output-index ordering.

Full local chainstate at height 957779, 166,350,731 UTXOs, a no-match
`combo(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)`
descriptor, warm cache, daemon pinned to CPU 3, five runs per version:

| version | median wall | range | median instructions | median branches |
| --- | ---: | ---: | ---: | ---: |
| base | 66.61 s | 66.57-66.69 s | 641.485 B | 129.069 B |
| candidate | 65.54 s | 65.43-65.58 s | 616.956 B | 124.440 B |

The median wall time improved 1.61%, instructions 3.82%, and branches 3.59%.
All ten RPC responses were byte-identical (SHA-256
`62539678afd6931c917d0135fedcc12d3f11e9e40958fe4983e461a5f0d0891a`),
reported the same 166,350,731 scanned UTXOs, and contained no matches. Raw
command output and `perf stat` files are under
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-lazy-key-{base,candidate}/`.

Validation:

```text
ninja -C build bitcoind test_bitcoin -j8
build/bin/test_bitcoin --run_test=coins_tests_dbbase --log_level=message
build/test/functional/test_runner.py rpc_scantxoutset.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-scantxoutset-lazy-key-final --timeout-factor=2
```

Reach: all `scantxoutset start` scans. The win approaches the measurement when
almost no entries match; matches and progress checkpoints already require the
key and continue through the eager path. `gettxoutsetinfo`, `dumptxoutset`,
snapshots, and other full-key cursor users keep their existing behavior. Cold
HDD scans remain I/O-bound, so the wall-time improvement can be smaller even
though the CPU work is removed.

### `scantxoutset`: reject impossible first script bytes before hashing

The existing `scantxoutset` fast path first rejects output scripts whose size
does not occur in the expanded descriptors, then hashes every remaining script
for the salted `unordered_set` lookup. This change also records the first byte
of each nonempty expanded descriptor script (and separately records whether an
empty script is a needle). After the size test, a script whose first byte is
absent cannot be an exact match and skips the hash/table lookup. Empty scripts
retain their old exact-match behavior, and `needles.contains(script)` remains
the final equality oracle. GitHub PR searches for `FindScriptPubKey`,
`scantxoutset`, `first byte`, and `scriptPubKey[0]` found no matching
already-pushed bitcoin/bitcoin change.

Full local chainstate at height 957779, 166,350,731 UTXOs, a raw 34-byte
P2TR script descriptor, 11 matching UTXOs, wallet/network disabled, daemon
pinned to CPU 3, and five warm runs per version:

| version | median wall | range | median daemon task time | median instructions | median branches |
| --- | ---: | ---: | ---: | ---: | ---: |
| base | 66.75 s | 66.62-67.33 s | 66.627 s | 654.000 B | 132.892 B |
| candidate | 64.47 s | 64.32-65.18 s | 64.355 s | 623.213 B | 127.158 B |

The median wall time improved 3.42%, daemon task time 3.41%, instructions
4.71%, and branches 4.31%. All ten RPC responses were byte-identical
(SHA-256 `58a893f83cf7619dca19bf7c8f2585cb2fce11f2aeedbe2db77c0385760d2b2a`),
reported the same 166,350,731 scanned UTXOs, and returned the same 11
unspents. Raw command output and `perf stat` files are under
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-first-byte.{candidate1.P2GVdm,base1.fTl41H}/`.

Validation:

```text
ninja -C build bitcoind -j4
build/test/functional/test_runner.py rpc_scantxoutset.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-scantxoutset-first-byte-final --timeout-factor=2
```

Reach: `scantxoutset start` scans whose requested scripts use few first bytes
relative to same-length UTXOs. The benefit is largest for a narrow template
such as P2TR; broad descriptor sets such as `combo` can pass more entries to
the existing lookup. `gettxoutsetinfo`, `dumptxoutset`, reindex, snapshots,
and all matching semantics are unchanged. Cold HDD scans can remain
storage-bound and see a smaller wall-time gain.

### `scantxoutset`: compare a single requested script directly

The first-byte prefilter still sends every feasible candidate to a salted
`unordered_set` lookup. When descriptor expansion produces exactly one
script, the set has one immutable element for the entire scan, so hashing and
bucket lookup are unnecessary: compare the candidate `CScript` directly to
that element. Multi-script scans retain the first-byte-filtered set lookup.
The exact `CScript` comparison is the same equality relation the set used.
GitHub PR searches for `FindScriptPubKey`, `needles.size()`, `needles.contains`,
and singleton `scantxoutset` performance found no matching already-pushed
bitcoin/bitcoin change.

The same full local height-957779 P2TR workload used above was run after the
first-byte commit: 166,350,731 UTXOs, 11 matches, wallet/network disabled,
daemon pinned to CPU 3. Five singleton-candidate runs were followed by three
fresh controls with only the first-byte prefilter restored:

| version | median wall | range | median daemon task time | median instructions | median branches |
| --- | ---: | ---: | ---: | ---: | ---: |
| first-byte-only base | 62.82 s | 62.70-63.39 s | 62.692 s | 594.322 B | 121.525 B |
| singleton candidate | 56.70 s | 56.48-57.39 s | 56.590 s | 525.473 B | 111.689 B |

The singleton path improved median wall time 9.74%, daemon task time 9.73%,
instructions 11.59%, and branches 8.09%. All eight RPC responses were
byte-identical (SHA-256
`58a893f83cf7619dca19bf7c8f2585cb2fce11f2aeedbe2db77c0385760d2b2a`),
with the same scan count and 11 unspents. Raw evidence is under
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-single-needle.{candidate1.RLp2o6,base2.yisobo}/`.

Validation:

```text
ninja -C build bitcoind -j4
build/test/functional/test_runner.py rpc_scantxoutset.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-scantxoutset-single-needle-final --timeout-factor=2
```

Reach: `scantxoutset start` calls whose descriptor expansion contains one
unique script, such as a single fixed address or `raw()` descriptor. Ranged or
multi-address descriptors retain the established set path, apart from one
well-predicted selection branch. The change does not affect
`gettxoutsetinfo`, `dumptxoutset`, reindex, snapshots, or matching semantics;
cold HDD scans can remain I/O-bound.

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

### Coins DB iterator value-copy variants

A high-rate `perf record` of a warm `gettxoutsetinfo none` against the user's
local chainstate at height 957779 attributed 5.64% of samples to
`std::vector<std::byte, zero_after_free_allocator>::_M_range_insert` beneath
`CDBIterator::GetValue`, and another 2.82% to `Obfuscation::operator()`. The
current code copies every LevelDB value into a reusable `DataStream`, XORs the
whole value in place, then deserializes the result. Five no-sampling baseline
passes took 54.480-54.696 seconds of daemon task time (median 54.582 seconds),
with 508.73-508.75 billion retired instructions and no major faults. Raw
profile and baseline files are in
`/mnt/my_storage/bitcoin-perf-scratch/current-utxo-profile/none-profile-99hz.*`
and `warm-baseline/`.

The first temporary variant copied values at or below the existing 1 KiB DB
preallocation size into a stack array, XORed that span, and used `SpanReader`;
larger generic values retained the old reusable buffer. It kept output exactly
the same for all six live scans. After excluding one run whose counters had
been attached to a stale shell process (the JSON output was valid, but the
counters were not), five clean runs took 54.446-54.638 seconds of daemon task
time (median 54.520 seconds) and 501.57-501.66 billion instructions. The
1.4% instruction decrease did not become a wall-clock win: branches fell from
about 107.79 to 106.05 billion but branch misses rose from 533-550 million to
614-622 million; median client wall time was 54.58 seconds. This is at most a
0.1% CPU-time movement and inside the end-to-end noise. The threshold branch
and 1 KiB stack use do not justify a commit. Raw valid runs are in
`warm-candidate-stack-v2/run-{2..6}.*`; the invalid first run is retained for
audit but excluded from all comparisons.

The second temporary variant changed `DataStream::write` to use
`std::vector::assign` when its backing vector was empty, hoping to remove the
profiled `insert` machinery while retaining its capacity. It was decisively
worse: five warm passes took 56.914-57.047 seconds of daemon task time (median
56.956 seconds), 4.35% above the baseline, and retired 550.52-550.68 billion
instructions, 8.24% above baseline. Client wall times were 56.99-57.07
seconds. This confirms that the current `insert` path is preferable for a
cleared reusable `DataStream`; neither temporary diff was committed. Raw
results are in `warm-stream-assign/`.

Both variants were fully reverted before continuing. The current profile still
establishes the copy/deobfuscate path as a legitimate CPU consumer, but a
future candidate must remove a memory pass without adding an unpredictable
per-value branch or a less efficient vector operation.

### Contiguous-stream zero-fill avoidance ([PR #77](https://github.com/l0rinc/bitcoin/pull/77))

PR #77 changes deserialization of contiguous `SpanReader`/`DataStream` byte
vectors and strings to avoid an initial zero fill. It is not a direct seed for
the named reindex or snapshot-file workload: block and snapshot file reads use
file-backed stream types (`CBufferedFile`/`AutoFile`), not this contiguous
stream path. The proposal's own full-reindex measurements also regressed: SSD
30382.664 to 30743.562 seconds (about 1% slower) and the i7 HDD run
48171.076 to 49655.945 seconds (about 3% slower). The direct live profile
above identified a different cursor-value copy path, and its two safe
specializations were neutral or worse. Decision: reject #77 for this goal;
do not repeat a multi-hour reindex unless a future profile points specifically
to contiguous vector/string deserialization.

### Larger coins-cache pool chunks ([PR #140](https://github.com/l0rinc/bitcoin/pull/140))

PR #140 changes the coins-cache pool chunk from 256 KiB to 1 MiB. A temporary
exact benchmark populated 100,000 deterministic `Coin` entries in a fresh
`CCoinsViewCache` and timed pool growth plus cache destruction; it was pinned
to CPU 3 and run five times. The baseline median was 372.684 ns/coin
(371.978-373.989), while the 1 MiB candidate median was 375.049 ns/coin
(374.486-375.700), 0.63% slower. Instructions were effectively unchanged
(1378.01 versus 1377.57 per coin) and faults were slightly higher (0.03504
versus 0.03598 per coin). The narrower allocator operation is therefore not a
speedup, and there is no justification to consume more cache memory in an HDD
reindex. No source diff was retained. Raw results and both benchmark binaries
are in `/mnt/my_storage/bitcoin-perf-scratch/coins-pool-chunks/`.

### Skip cache reallocation during IBD flush ([PR #59](https://github.com/l0rinc/bitcoin/pull/59))

The one-line-looking part of #59 suppresses cache reallocation after an IBD
flush, but it is not independent on current master. `DynamicMemoryUsage()`
counts allocated pool capacity, so an emptied cache whose pool is retained can
still appear over its target and immediately cause another flush. The PR's
claimed benefit relies on its earlier, broader `ActiveMemoryUsage` accounting
change, together with changes to flush behavior and cache reservation. Its
reported one-run roughly 4% improvement is not sufficient evidence for that
interdependent stack. Decision: do not cherry-pick or benchmark the isolated
line; revisit only if a current profile demonstrates repeated empty-cache
flushes and a minimal accounting invariant can be proven.

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
- [#132](https://github.com/l0rinc/bitcoin/pull/132)/[#136](https://github.com/l0rinc/bitcoin/pull/136): Bloom-filter and restart-interval tuning. Point reads and ordered
  scans have opposing tradeoffs; test both before changing defaults.
- [#140](https://github.com/l0rinc/bitcoin/pull/140)/[#59](https://github.com/l0rinc/bitcoin/pull/59): the pool-chunk and
  isolated no-reallocation variants are rejected above. Other allocation or
  accounting shapes still need a current profile and an independent invariant.
- [#180](https://github.com/l0rinc/bitcoin/pull/180): input fetching, no-seek compaction, and flush timing. Split into minimal
  hypotheses; do not carry experimental logging or policy changes into a
  performance commit.
- [#195](https://github.com/l0rinc/bitcoin/pull/195)/[#152](https://github.com/l0rinc/bitcoin/pull/152): cursor/coinstats ideas. Their proven portions are represented by
  the accepted direct-streaming and hashing commits above; remaining layering
  changes need independent evidence.
- [#200](https://github.com/l0rinc/bitcoin/pull/200)/[#203](https://github.com/l0rinc/bitcoin/pull/203): mmap/open-file/cache-threshold variants. Existing and current
  cache/write-buffer experiments are neutral or harmful; reopen only if a new
  profile demonstrates a different bottleneck.
### LevelDB: heapify forward merging iterators for full UTXO scans

The earlier reindex profile correctly deferred LevelDB work: its small,
throttled sample did not justify a storage change. A separate profile of the
actual `gettxoutsetinfo none` RPC over the local full chainstate did. That
scan had 166,350,731 UTXOs at height 957779, and its usable samples made
`leveldb::MergingIterator::FindSmallest()` (13.76%) and
`InternalKeyComparator::Compare` (11.84%) the two largest self-time signals.
The already-committed direct-Core cursor changes leave this iterator-selection
work intact.

This is therefore a narrowly justified LevelDB change, not a RocksDB port or
an options/cache experiment. The local LevelDB `MergingIterator` explicitly
scanned every child in `FindSmallest()` after every forward `Next()`, with a
comment that a heap might be useful for more children. Current RocksDB was
consulted only as an independent design hint: it maintains a min-heap for its
forward merging iterator. No RocksDB code is copied. The replacement keeps a
binary heap of valid child wrappers: seek builds it in O(n), and each forward
advance restores its root in O(log n), instead of finding the smallest child
in O(n). Reverse iteration deliberately retains the existing linear
`FindLargest()` path.

Exact ordering is preserved. On comparator equality the heap uses the child
wrapper's address as a stable tie-breaker. Those pointers all refer to the
same `children_` array, so this has the original forward scan's lowest-index
tie order; the unmodified reverse scan retains its original highest-index tie
order. The new `leveldb_tests` use deliberately equal keys with distinguishable
values, exercise `Seek()`, and change directions on a disjoint-key merge. A
temporary mutation reversing the heap tie-breaker rebuilt successfully but
made `merging_iterator_preserves_duplicate_tie_order` fail with the expected
reversed `a`, `c`, and `e` values. Restoring the tie-breaker passed:

```
cmake -B build
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=leveldb_tests --log_level=message
ctest --test-dir build --output-on-failure -R '^(leveldb_tests|dbwrapper_tests)$'
build/bin/test_bitcoin --run_test=coins_tests --log_level=message
```

For performance evidence, two otherwise identical network-disabled daemons
used `/mnt/my_storage/BitcoinData`, `-checkblocks=1`, `-persistmempool=0`, and
an isolated RPC cookie/debug-log directory. The first baseline full scan
(162.83 s) warmed the OS page cache and was excluded. The baseline's next five
and the candidate's next five `gettxoutsetinfo none` results all have SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`:

| version/runs | wall seconds | median wall | median task-clock | median instructions | median branches | median branch misses |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| baseline | 65.29, 65.19, 65.16, 65.27, 65.25 | 65.25 | 65.196 s | 631.535 B | 131.799 B | 539.718 M |
| heap candidate | 56.20, 55.61, 55.79, 55.64, 55.60 | 55.64 | 55.587 s | 508.649 B | 106.118 B | 520.077 M |

The candidate is 14.73% faster in median RPC wall time, with 14.74% less
task-clock, 19.46% fewer instructions, 19.49% fewer branches, and 3.64% fewer
branch misses. All measurements use `perf stat -p <daemon-pid>` over one RPC;
raw JSON, timings, counter CSV files, and SHA-256 files are retained under
`/mnt/my_storage/bitcoin-perf-scratch/leveldb-merger-heap-baseline3.sMT71a/`
and `/mnt/my_storage/bitcoin-perf-scratch/leveldb-merger-heap-candidate.SxJPUH/`.

Reach is broad for forward LevelDB merged iterators: full chainstate scans in
`gettxoutsetinfo`, `scantxoutset`, and `dumptxoutset`; other DB iterators with
overlapping mutable/L0 sources; and LevelDB compaction's merging iterator.
It does not change on-disk data, the comparator, value decoding, random
reads, cache sizing, write buffering, or reverse iteration. The benchmark is
one cache-warm full chainstate and one HDD host, not a cold-cache test and not
a reindex-chainstate result; do not extrapolate its 14.73% figure to those
workloads without rerunning them. `origin/master` was freshly fetched and its
`src/leveldb/table/merger.cc` still uses the linear `FindSmallest()` loop, so
this is not an already-pushed upstream fix.
### `dbwrapper`: decode obfuscated iterator values without a temporary copy

The earlier `7d1733df09` cursor change deliberately optimized only databases
with an empty obfuscation key: a stable LevelDB value span can then be passed
straight to `SpanReader`. The normal chainstate intentionally has a non-empty
key, so its `CDBIterator::GetValue()` still copied every value into
`m_scratch`, XORed the complete copy, then decoded it. That leaves the main
cursor path used by `gettxoutsetinfo`, `scantxoutset`, and ordinary
`dumptxoutset` outside the prior improvement.

The full-chainstate `scantxoutset` profile made this concrete. A
network-disabled daemon at height 957779 scanned all 166,350,731 UTXOs using
the no-match descriptor `raw(6a)#4mhr9ur5`; `CDBIterator::GetValue<Coin>` and
the vector range insertion in its temporary `DataStream` accounted for 7.82%
of the usable samples. This is a direct Core decoding change, so it avoids the
more invasive LevelDB/RocksDB work that the reindex profile did not justify.

`ObfuscatedSpanReader` borrows the iterator's value span, copies only each
requested deserialize field into its destination, XORs that destination at
the current byte offset, and advances the borrowed span. `ignore()` advances
the same offset without unnecessary decoding. Its bounds failures retain
`CDBIterator::GetValue()`'s existing catch-and-return-false contract. The
offset matters: serializing a vector reads its CompactSize and payload in
separate calls, while database obfuscation is defined over the whole original
value.

`dbwrapper_iterator` now stores such a vector in both an obfuscated and an
unobfuscated in-memory database, reads it after `Seek()` and after `Next()`,
and checks every byte. The normal validation passed:

```
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=message
build/bin/test_bitcoin --run_test=coins_tests --log_level=message
git diff --check
```

For a negative proof, a temporary mutation changed
`m_obfuscation(dst, m_key_offset)` to `m_obfuscation(dst)`. The focused test
then failed in both iterator positions with the expected corrupted payload
(`ad e7 84 b9 f1 c9 3e` rather than `00 01 02 03 04 05 06`); restoring the
offset passed the test again. This proves the new test checks the precise
cross-read invariant rather than merely exercising the code.

The performance comparison used the local HDD-backed `BitcoinData`, same
network/wallet-disabled daemon arguments as the profile, an isolated
cookie/log directory, `-checkblocks=1`, and a page-cache drop before each
startup. The baseline was this branch with only the uncommitted reader change
removed. All runs returned the identical 199-byte JSON response with SHA-256
`62539678afd6931c917d0135fedcc12d3f11e9e40958fe4983e461a5f0d0891a`:

| version/runs | RPC wall seconds | daemon task-clock | instructions | branches | major faults |
| --- | ---: | ---: | ---: | ---: | ---: |
| baseline | 152.97 | 63.836 s | 513.383 B | 107.171 B | 14,641 |
| reader candidate | 148.29, 149.91 | 59.555 s, 59.066 s | 473.305 B, 466.506 B | 97.452 B, 96.398 B | 14,737, 14,641 |

The two candidate scans are 3.06% and 2.00% faster in RPC wall time than the
matched baseline. Their task-clock falls 6.71% and 7.47%, retired
instructions 7.81% and 9.13%, and branches 9.07% and 10.05%; matching major
fault counts rule out a changed I/O path as the explanation. The sample is
small (one baseline, two candidates) and the wall-time effect is HDD-limited,
so the conservative claim is a reproducible CPU reduction with an initial
2--3% full-chainstate `scantxoutset` wall-time gain on this host, not a
general reindex result.

Reach is every obfuscated `CDBIterator` value decode. In particular it removes
the whole-value copy/deobfuscate buffer for normal chainstate scans in
`gettxoutsetinfo`, `scantxoutset`, and non-`in_memory` `dumptxoutset`; it can
also help other intentionally obfuscated Core databases and indexes that use
this iterator. It does not alter the XOR key or bytes on disk, serialization,
error handling, LevelDB iteration, the unobfuscated direct-reader path, or
reindex's write/validation work. It may reduce the chainstate iterator share
of `-reindex-chainstate`, but no reindex speedup is claimed until separately
measured. Raw baseline and candidate artifacts are retained at
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-obfuscated-reader-baseline.20260719-050739/`,
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-obfuscated-reader.vFF5Sg/`,
and
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-obfuscated-reader-candidate.S0rHPz/`.

Before considering this change, `origin/master` was freshly fetched at
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`. Its `CDBIterator::GetValue()`
still copies and deobfuscates its entire iterator value in a `DataStream`; its
history contains no `ObfuscatedSpanReader`. It is therefore not an already
pushed upstream fix.
### `coinstats`: batch bounded interruption checks

`ComputeUTXOStats` accepts a shutdown/interruption callback. Its RPC callback
only observes whether RPC is still running, but the old loop dispatched it
through `std::function` before every UTXO. The stats already maintain a
monotonic coin count, so check before the first entry and after every 8,192
successfully processed entries. This is the existing `scantxoutset`
interruption cadence: it preserves immediate initial shutdown detection and
bounds later detection to 8,192 UTXOs, while eliminating nearly all indirect
callback calls. The callback can still throw exactly as before. GitHub PR
searches for `ComputeUTXOStats`/interruption, `gettxoutsetinfo` interruption
performance, `SnapshotUTXOHashBreakpoint`, and an 8,192 UTXO-stats cadence
found no matching already-pushed bitcoin/bitcoin change.

Five warm, sequential full-chain `gettxoutsetinfo none` scans were run against
the explicitly permitted `/mnt/my_storage/BitcoinData` at height 957779,
with wallet/network activity disabled and the daemon pinned to CPU 3. The
candidate was measured first, then the exact source was restored, rebuilt, and
measured with five fresh controls:

| version | median wall | range | median daemon task time | median instructions | median branches |
| --- | ---: | ---: | ---: | ---: | ---: |
| per-UTXO base | 65.54 s | 65.47-66.24 s | 65.453 s | 636.253 B | 132.435 B |
| 8,192-entry candidate | 62.57 s | 62.45-63.14 s | 62.471 s | 600.341 B | 125.524 B |

The median wall time improved 4.53%, daemon task time 4.56%, instructions
5.64%, and branches 5.22%. Both series had zero major faults; their median
minor-fault counts were 21 and 20 respectively. Every one of the ten full RPC
responses had SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`,
including height, best block, counts, amount, and disk-size fields. Raw
commands, JSON, affinity, `time`, and `perf stat` output are under
`/mnt/my_storage/bitcoin-perf-scratch/gettxoutsetinfo-interrupt-batch.{candidate5.fUKNEq,base1.Ex0sdR}/`.

Validation:

```text
ninja -C build bitcoind bitcoin-cli -j4
build/test/functional/test_runner.py rpc_blockchain.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-coinstats-interrupt-batch --timeout-factor=2
```

Reach: all direct `ComputeUTXOStats` callers, including non-indexed
`gettxoutsetinfo` for every hash type, the statistics pass before a latest or
rolled-back `dumptxoutset`, and snapshot background validation. The measured
workload isolates `hash_type=none`, where callback dispatch is a larger share;
hashing and cold-HDD I/O reduce the percentage. Normal reindex-chainstate and
coinstats-index maintenance do not call this function. The only behavioral
tradeoff is the documented bounded shutdown/interrupt observation delay.

### `coinstats`: deserialize only amount and script size for un-hashed stats

The earlier no-hash specialization avoids building the transaction-to-output
map, but it still fully deserializes every `Coin` while walking the chainstate.
For `gettxoutsetinfo hash_type=none`, the remaining result needs a coin's
amount and the size of the script which normal coin deserialization would
produce; it never reads the script itself. A fresh full-chain profile after
the iterator-reader change still attributed 20.44% of the usable samples to
`CCoinsViewDBCursor::GetValue()` and its coin/script deserialization.

`CoinStatsValue` therefore reads the height/coinbase code, amount compression,
and a new `ScriptCompression::UnserSize()` helper. The helper consumes exactly
the same encoded bytes as `Unser()` but avoids constructing ordinary scripts:
P2PKH, P2SH, and compressed-P2PK encodings report their fixed reconstructed
sizes, ordinary scripts report their encoded size, and oversized scripts
report the one-byte `OP_RETURN` result. Compressed-pubkey encodings deliberately
still call `DecompressScript`, because an invalid curve point must retain the
normal empty-script result. Hash-producing statistics retain ordinary `Coin`
deserialization and `ApplyCoinHash`; their serialization commitment is not
weakened.

The focused unit test serializes coins containing P2PKH, P2SH, compressed and
uncompressed P2PK, ordinary, and oversized scripts. It compares the partial
value's amount and reported script size with an independently fully
deserialized `Coin`, and verifies that it consumes the whole record. A
temporary mutation changing the P2SH reconstructed size from 23 to 25 makes
the test fail, so the test checks the special-script rule rather than merely
executing the parser. The existing functional RPC test additionally compares
the live `hash_type=none` response with the full-hash response after removing
only intentionally different fields.

```text
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=coins_tests/coin_stats_value_matches_coin_deserialization --log_level=message
build/bin/test_bitcoin --run_test=coins_tests --log_level=message
build/test/functional/test_runner.py rpc_blockchain.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-partial-stats --timeout-factor=2
git diff --check
```

The performance series used the explicitly permitted local `BitcoinData` at
height 957779 with 166,350,731 UTXOs. A network-disabled, wallet-disabled
daemon with `-checkblocks=1` served the exact same `gettxoutsetinfo none` RPC;
each response had SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`.

| version/runs | RPC wall seconds | daemon task-clock | instructions | branches | major faults |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact baseline (3) | 51.95, 51.98, 51.93 | 51.908, 51.939, 51.881 s | 467.147, 467.144, 467.145 B | 96.278, 96.278, 96.278 B | 0, 0, 0 |
| partial-value candidate (3) | 47.91, 47.90, 48.04 | 47.869, 47.833, 47.982 s | 437.459, 437.313, 437.442 B | 89.115, 89.111, 89.112 B | 0, 0, 0 |

The matched medians improve RPC wall time from 51.95 to 47.91 seconds
(7.78%), daemon CPU from 51.908 to 47.869 seconds (7.78%), instructions by
6.35%, and branches by 7.44%. Zero major faults in every run makes this a
warm-cache CPU result; a cold HDD scan can realize a smaller wall-time share.
Raw baseline and candidate artifacts are retained in
`/mnt/my_storage/bitcoin-perf-scratch/gettxoutsetinfo-partial-stats-baseline.tyWJXO/`
and
`/mnt/my_storage/bitcoin-perf-scratch/gettxoutsetinfo-partial-stats-candidate.DMeAkx/`.

Reach is specifically non-indexed `gettxoutsetinfo hash_type=none` and any
future direct caller of `ComputeUTXOStats(NONE)`. It does not change
hash-producing `gettxoutsetinfo` modes, `dumptxoutset` (which requires the
serialized hash), `scantxoutset`, coinstats-index maintenance, snapshots, or
`-reindex-chainstate`; no speedup is claimed for those paths. Before this
change, `origin/master` was freshly fetched at
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; it contains neither
`CoinStatsValue` nor `UnserSize`, so this is not a duplicate of an already
pushed upstream fix.
### Skip contextual sigops counting while assumevalid skips scripts ([PR #170](https://github.com/l0rinc/bitcoin/pull/170))

Local PR #170 is open and its commit is not present in `bitcoin/bitcoin`, so it
passes the upstream-pushed exclusion. `CheckBlock()` continues to enforce the
unconditional legacy sigops bound. The change only skips contextual
P2SH/witness `GetTransactionSigOpCost()` work in `ConnectBlock()` when
`fScriptChecks` is false, alongside skipped script validation.

The cold-HDD OverlayFS reindex-chainstate control used height 287000,
`-dbcache=450`, a dropped page cache, and network-disabled settings. All runs
logged script-check disablement at block 1, reached the stop height, and shut
down cleanly:

| version/run | daemon wall | user | system | instructions | branches | input KiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| candidate 1 | 574.895 s | 457.603 s | 61.578 s | 2.496 T | 281.033 B | 31,024,016 |
| baseline | 576.680 s | 468.193 s | 62.119 s | 2.636 T | 296.868 B | 31,023,720 |
| candidate 2 | 569.623 s | 459.459 s | 61.543 s | 2.494 T | 280.772 B | 31,023,520 |

Candidate median wall time is 572.259 seconds, 0.77% faster than baseline;
median user CPU falls 2.06%, instructions about 5.3%, and branches about
5.3%. `feature_assumevalid.py` now funds P2SH outputs, accepts an over-limit
contextual-sigops block only beneath the assumed-valid block, then verifies
that an equivalent height-103 child is rejected as `bad-blk-sigops` when
script checks resume. Raw results are under
`/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/sigops170-{candidate1,baseline1,candidate2}/metrics/`.

### Reuse input heights for BIP68 sequence locks ([PR #138](https://github.com/l0rinc/bitcoin/pull/138))

Local PR #138 is open and its `f6c6d234e182446307395f92da87b27909377d89`
head is absent from freshly fetched `origin/master` at
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; it is therefore not an
already-pushed upstream fix. In `Chainstate::ConnectBlock()`, every
non-coinbase input was first consulted by `HaveInputs()`, again by
`Consensus::CheckTxInputs()`, then a third time solely to populate
`prevheights` for `SequenceLocks()`. The first availability pass must remain:
it gives missing/spent inputs precedence over later maturity and amount
failures. The second pass already has the required unspent `Coin`, however, so
the accepted change makes its height an optional output of `CheckTxInputs()`
and passes the pre-sized `prevheights` vector from `ConnectBlock()`.

This removes exactly the final cache lookup per non-coinbase input. It does not
change input validation, coin mutation, sequence-lock evaluation, or the
mempool path: callers that do not need heights retain the default null
argument. `HaveInputs()` establishes that all inputs are available before any
height is written, and `CheckTxInputs()` still reads the same `Coin` in the
same input order while checking maturity, amounts, and fees. The focused
`checktxinputs_prev_heights_test` uses two coins at heights 101 and 202 to
check the returned order and fee. A temporary `coin.nHeight + 1` mutation made
that test fail with `102 != 101` and `203 != 202`, then was reverted.

The same cold-HDD OverlayFS reindex-chainstate control used height 287000,
`-dbcache=450`, a dropped page cache, network-disabled arguments, and
`-disablewallet`. Each run started from a fresh upperdir over the same local
`BitcoinData` lowerdir. It verified `height=0`, assumed-valid script disablement
at block 1, height 287000, and clean shutdown. The two baseline results bracket
the candidate:

| version/run | daemon wall | user | system | task-clock | instructions | branches | branch misses | major faults | input / output KiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline 1 | 576.75 s | 457.02 s | 61.37 s | 514.848 s | 2.522384 T | 280.972 B | 4.305 B | 889 | 31,021,928 / 9,454,784 |
| candidate | 573.01 s | 455.12 s | 62.21 s | 513.740 s | 2.503694 T | 281.131 B | 4.302 B | 895 | 31,024,392 / 9,751,024 |
| baseline 2 | 574.96 s | 456.78 s | 61.83 s | 515.027 s | 2.522559 T | 281.004 B | 4.335 B | 889 | 31,023,664 / 9,449,344 |

Against the two-baseline midpoint, candidate wall time improves 0.49%,
task-clock 0.23%, user CPU 0.39%, and instructions 0.74%; branches are nearly
unchanged (+0.05%), while branch misses fall 0.41%. The five CPU-pinned
`bench_bitcoin -filter='ConnectBlock.*' -min-time=1000` controls agree on the
direction: median elapsed time improved 0.23% for all-ECDSA, 0.21% for
all-Schnorr, and 0.21% for mixed blocks; instruction movement was -0.45%,
+0.13%, and -0.30%, respectively. The macro reindex instruction result is the
acceptance evidence; the microbenchmark is only a low-cost regression screen.
Raw microbenchmark JSON/text are under
`/mnt/my_storage/bitcoin-perf-scratch/bip68-prevheights-connectblock/`; raw
reindex metrics and debug logs are under
`/mnt/my_storage/bitcoin-perf-scratch/bip68-prevheights-reindex/{baseline1,candidate,baseline2}/metrics/`.

### `coins`: avoid cache work for unspendable outputs ([fork PR #133](https://github.com/l0rinc/bitcoin/pull/133))

Fork PR #133 is open at `8c6ca2d76aaf49c1d6e5337d418a8fbfc4227cc1` and
is absent from freshly fetched `origin/master` at
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; it is not an already-pushed
upstream fix. `AddCoins()` previously built two `COutPoint`s and a `Coin` for
every output, while `CCoinsViewCache::AddCoin()` immediately returns for an
`IsUnspendable()` script. The change applies that existing condition before
the overwrite lookup and `Coin` construction, then reuses one outpoint for
spendable outputs.

The valid-chain contract is unchanged: `AddCoin()` already prevents an
unspendable output becoming a UTXO, `DisconnectBlock()` skips it as well, and
spendable outputs retain the same `HaveCoin()`/`AddCoin()` sequence. The
recovery caller with `check_for_overwrite=true` now omits a lookup only for an
output that valid block connection cannot have stored. This is a
performance-only rearrangement, so the before/after microbenchmark is the
primary verifier; normal `coins_tests` and `transaction_tests` cover cache
insertion, overwrites, spentness, `UpdateCoins`, and transaction validation.

Temporary CPU-3-pinned Release `bench_bitcoin` targets, five runs each with
`-min-time=1000`, gave:

| one-output case | base median ns/op | candidate median ns/op | base / candidate instructions | result |
| --- | ---: | ---: | ---: | --- |
| 42-byte `OP_RETURN` | 38.84 | 6.66 | 507 / 76 | 5.83x faster; -85.0% instructions |
| spendable `OP_TRUE` | 139.12 | 141.89 | 1,294 / 1,304 | +2.0%; +10 instructions |

The resulting instruction break-even is `10 / (431 + 10) = 2.27%`
unspendable outputs. A network-disabled daemon counted six disjoint 20-block
ranges from the permitted `BitcoinData`: 87,824 unspendable outputs among
571,251 total (15.3740%). The per-range shares at heights 240000, 400000,
550000, 700000, 850000, and 950000 were respectively 0.0000%, 0.4927%,
9.7539%, 2.3982%, 26.6171%, and 19.8392%. This mature-chain sample is well
above break-even. Raw benchmark text and counts are under
`/mnt/my_storage/bitcoin-perf-scratch/addcoins-unspendable/`.

Validation:

```text
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=coins_tests --log_level=message
build/bin/test_bitcoin --run_test=transaction_tests --log_level=message
git diff --check
```

Reach: all `AddCoins()` output additions in normal `ConnectBlock()` during
sync and `-reindex-chainstate`, `ReplayBlocks()` recovery, and the mempool
consistency checker. It does not directly affect `gettxoutsetinfo`,
`scantxoutset`, or `dumptxoutset`, whose scans read existing UTXOs. It removes
no disk I/O, so no broad HDD wall-time percentage is claimed; the gain is the
measured CPU work per unspendable output.
