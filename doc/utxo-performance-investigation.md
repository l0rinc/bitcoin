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

A secondary no-match `combo` scan, whose expansion uses multiple scripts,
checks the non-singleton path. Five singleton-enabled runs were followed by
three fresh first-byte-only controls; every response had SHA-256
`62539678afd6931c917d0135fedcc12d3f11e9e40958fe4983e461a5f0d0891a` and
reported 166,350,731 scanned UTXOs with no unspents:

| version | median wall | range | median daemon task time | median instructions | median branches |
| --- | ---: | ---: | ---: | ---: | ---: |
| first-byte-only base | 70.33 s | 70.31-70.81 s | 70.204 s | 675.427 B | 136.050 B |
| singleton-enabled path | 67.80 s | 67.68-68.56 s | 67.671 s | 646.419 B | 130.519 B |

This secondary measurement is favorable (3.60% wall and 4.30% instructions),
but the primary claim remains the direct one-script comparison. It establishes
that the predictable singleton selection does not impose an observed
multi-script regression on this machine. Raw evidence is under
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-single-needle-multi.{candidate1.cOHjy1,base1.8TLvtQ}/`.

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

### Lower LevelDB block restart interval ([PR #132](https://github.com/l0rinc/bitcoin/pull/132))

PR #132 sets `leveldb::Options::block_restart_interval` from the documented
default of 16 to 8. A table block stores a complete key at every restart point;
the proposed layout therefore halves the worst-case linear key reconstruction
after `Block::Iter::Seek`, while adding restart-array entries and more
unshared key bytes to every newly written SSTable. It is dynamically
compatible with existing tables, but cannot affect the already-written local
chainstate, so a warm RPC scan would be a false test. The direct UTXO profile
did show `Block::Iter`, key comparison, and iterator methods, which justified
a fresh, fixed-height rebuild test but not acceptance on profile evidence
alone.

The exact fresh OverlayFS `-reindex-chainstate` control already used for the
`CheckTransaction` experiment was repeated with only the one temporary option
line changed: source lowerdir `/mnt/my_storage/BitcoinData`, a distinct empty
upper/work directory for every run, page cache dropped before launch,
`-stopatheight=287000`, `-dbcache=450`, no networking or wallet, GCC Release,
and clean daemon shutdown/log assertions. The controls were the immediately
preceding current-tree runs; all candidates were rebuilt from the same source
plus the option. The smaller restart interval increased the retained upper
layer from 718 and 715 MiB in the controls to 740 and 735 MiB in the candidates
(about 3%), as its less-compressed keys predict.

| version/run | wall | user | system | peak RSS | major faults | input blocks | output blocks |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| default interval 16, control 1 | 574.61 s | 468.18 s | 41.73 s | 1,604,372 KiB | 890 | 30,990,304 | 9,456,240 |
| interval 8, candidate 1 | 571.59 s | 466.96 s | 42.00 s | 1,556,840 KiB | 890 | 30,989,504 | 9,914,000 |
| default interval 16, control 2 | 573.49 s | 467.92 s | 42.18 s | 1,634,192 KiB | 892 | 30,989,392 | 9,451,520 |
| interval 8, candidate 2 | 577.04 s | 467.27 s | 42.03 s | 1,556,800 KiB | 893 | 30,990,112 | 9,903,432 |

The two-run medians are 574.05 seconds with the default and 574.315 seconds
with interval 8: a 0.05% candidate regression. The first apparent 0.53%
speedup did not repeat; paired changes are -0.53% then +0.62%. Candidate user
CPU fell only 0.20% at a cost of slightly higher first-run output, while wall
time, input, faults, and the larger resulting chainstate showed no durable
gain.
Decision: reject #132. A persistent table-layout choice needs a clearly
reproducible end-to-end win, especially on the HDD workload it targets. The
temporary source edit was removed. Raw commands, logs, time reports, and upper
layers are retained under
`/mnt/my_storage/bitcoin-perf-scratch/restart-interval-8.run{1,2}.*`.

### Higher LevelDB Bloom-filter density ([PR #136](https://github.com/l0rinc/bitcoin/pull/136))

PR #136 raises the `NewBloomFilterPolicy` density from 10 to 16 bits per key.
This is a plausible negative-point-read optimization: LevelDB's documented
10-bit filter has about a 1% false-positive rate, while a denser filter can
avoid more unnecessary table-block probes. It does not change stored keys or
values. The policy name remains `leveldb.BuiltinBloomFilter2`, and
`BloomFilterPolicy::KeyMayMatch` reads the encoded probe count from each
filter, so existing 10-bit and newly written 16-bit filters coexist safely.
Only future table creation or compaction can benefit; ordered cursor scans such
as `gettxoutsetinfo`, `scantxoutset`, and `dumptxoutset` do not use a Bloom
filter to advance through every key.

Because reindex-chainstate does issue coins-DB point lookups while building
new tables, the exact fresh OverlayFS HDD rebuild protocol was used. Every run
used a distinct empty upper/work directory over `/mnt/my_storage/BitcoinData`,
dropped page cache, `-stopatheight=287000`, `-dbcache=450`, GCC Release, no
wallet/network activity, and clean height/version log checks. The two early
10-bit controls were followed by two 16-bit candidates. Their apparent result
looked promising and initially justified focused correctness checks:

```text
ninja -C build bitcoind test_bitcoin -j8
build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=test_suite
build/bin/test_bitcoin --run_test=coins_tests_dbbase --log_level=test_suite
build/bin/test_bitcoin --run_test=coins_tests/coins_db_leveldb_layout --log_level=test_suite
```

All tests passed, including DB-wrapper existing-data/reindex and coins cursor
coverage. However, the filter change is database-wide, and the 16-bit runs did
not reduce filesystem input or CPU time. A post-candidate 10-bit control was
therefore required before accepting the sub-percent result:

| policy/run order | wall | user | system | peak RSS | major faults | input blocks | output blocks | upper layer |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 10 bits, initial control 1 | 574.61 s | 468.18 s | 41.73 s | 1,604,372 KiB | 890 | 30,990,304 | 9,456,240 | 718 MiB |
| 10 bits, initial control 2 | 573.49 s | 467.92 s | 42.18 s | 1,634,192 KiB | 892 | 30,989,392 | 9,451,520 | 715 MiB |
| 16 bits, candidate 1 | 567.89 s | 468.34 s | 41.64 s | 1,549,872 KiB | 891 | 30,989,336 | 9,544,832 | 716 MiB |
| 16 bits, candidate 2 | 570.60 s | 469.28 s | 42.04 s | 1,610,808 KiB | 891 | 30,989,280 | 9,547,304 | 718 MiB |
| 10 bits, post-candidate control | 565.01 s | 467.71 s | 41.44 s | 1,610,224 KiB | 896 | 30,989,256 | 9,179,656 | 784 MiB |

The first two candidates had a superficially favorable 569.245-second median
versus 574.05 seconds for the initial controls (0.84%), but the later default
policy control beat both candidates by 2.88 and 5.59 seconds. Input blocks
were identical to rounding across all five runs, candidate user CPU was not
lower, and candidate output blocks were about 1% higher. The final control's
different retained upper-layer/output size further demonstrates that unrelated
LevelDB compaction and filesystem variation exceeds this option's observable
effect here. Decision: reject #136; retain the 10-bit default and do not claim
an HDD reindex, RPC, or snapshot benefit. The temporary option line was
removed. Raw commands, logs, reports, and OverlayFS layers are under
`/mnt/my_storage/bitcoin-perf-scratch/bloom-bits-{16.run1.*,16.run2.*,10.postcontrol.*}`.

### `ConnectBlock` next-transaction CPU prefetch ([PR #68](https://github.com/l0rinc/bitcoin/pull/68))

The broad async prevout-fetching part of #68 is obsolete: current master uses
`CoinsViewOverlay`, `StartFetching`, and configurable `-prevoutfetchthreads`
to collect and fetch block inputs in parallel before `ConnectBlock`. The
remaining simple, independently testable idea prefetches the next transaction
object plus its nonempty input and output buffers at the beginning of the
`ConnectBlock` transaction loop. A temporary compiler-gated
`__builtin_prefetch(ptr, read, no-temporal-locality)` helper and seven-line
call site were built; unsupported compilers would retain a no-op helper. The
candidate did not alter transaction order, validation, ownership, or memory
contents.

Five CPU-3-pinned GCC Release runs timed the existing representative
`ConnectBlockMixedEcdsaSchnorr` benchmark for at least five seconds each:

| version | median | range | median instructions/block | median branch misses/block |
| --- | ---: | ---: | ---: | ---: |
| base | 218.108 ms | 217.664-218.912 ms | 807.299 M | 342,196 |
| next-transaction prefetch | 218.758 ms | 217.941-218.917 ms | 808.047 M | 344,624 |

The annotation regressed the focused median 0.30%, increased instructions
0.09%, and increased branch misses 0.71%. It offers no evidence of an
end-to-end HDD improvement and would add a cross-compiler utility solely for a
regressing hot path. Decision: reject the standalone prefetch. The temporary
header and `ConnectBlock` calls were removed. Raw nanobench JSON is retained
under `/mnt/my_storage/bitcoin-perf-scratch/connectblock-prefetch/{baseline,candidate}/`.

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

### `dumptxoutset`: calculate the serialized UTXO hash while writing

`dumptxoutset` first calls `GetUTXOStats(..., HASH_SERIALIZED)` to obtain the
header count and commitment, then walks the complete coins cursor again to
write the snapshot. The snapshot writer already has every `COutPoint` and
`Coin`, so a tempting candidate changed both live and rolled-back preparation
to `CoinStatsHashType::NONE` and used the writer loop to call the same
`ApplyCoinHash(HashWriter&, ...)` before emitting each coin. It then assigned
the writer's final hash to the returned statistics. This is a distinct local
hypothesis, not an upstream patch: GitHub PR searches for
`PrepareUTXOSnapshot`, `WriteUTXOSnapshot`, and serialized-hash calculation
while writing found no equivalent already-pushed bitcoin/bitcoin change.

The candidate was semantically sound in the relevant tests:

```text
ninja -C build bitcoind test_bitcoin -j8
build/test/functional/test_runner.py rpc_dumptxoutset.py feature_utxo_set_hash.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-dumptxoutset-hash-stream --timeout-factor=2
```

Both functional tests passed. In particular, `rpc_dumptxoutset.py` checks the
snapshot bytes and the reported `txoutset_hash`. Each full manual run also
returned 166,350,731 coins, height 957779, base hash
`00000000000000000001b39b8e83075ff0f0f4eafab2eed20906496c13728881`, and
serialized hash
`ed1ed399ab6c15d571999b81cdabe07de50e7b1b58f5e07ab29c2770fbf1d2b3`.
The three 8.9 GiB output files were byte-identical (SHA-256
`f85de775f38c3aea3d07e7d210181b836e189f3d04a6af6e74a721946fd3d3d4`).

Nevertheless, it is a clear end-to-end regression. A normal current-tree
daemon was started with wallet and network activity disabled, all daemon
threads were pinned to CPU 3, and the RPC wrote its output to a separate
scratch directory on the same HDD. The second candidate was deliberately run
after the baseline, so the result cannot be explained by the first
candidate's colder ordering:

| version/run | wall | daemon task time | instructions | branches |
| --- | ---: | ---: | ---: | ---: |
| candidate 1 (before control) | 249.68 s | 241.945 s | 2.227 T | 363.148 B |
| base control | 233.56 s | 221.855 s | 2.050 T | 328.342 B |
| candidate 2 (after control) | 247.37 s | 232.473 s | 2.112 T | 340.667 B |

Relative to the interleaved control, the candidate regressed wall time 5.91%,
daemon task time 4.79%, instructions 3.01%, and branches 3.75%. Moving the
hash into the output loop avoids a cursor pass but combines independent
serialization/hashing work with the already write-heavy loop; that loses on
this HDD workload. Decision: reject and fully revert the temporary source
change. Raw commands, time reports, `perf stat` output, RPC results, and
byte-identical snapshots are retained in
`/mnt/my_storage/bitcoin-perf-scratch/dumptxoutset-hash-stream.{candidate1.VivC7o,base1.pmlQgU,candidate2.E9OThK}/`.

### `dumptxoutset`: move decoded coins into the output group

The snapshot writer decodes a `Coin` into a reusable loop local and then
copies it into `std::vector<std::pair<uint32_t, Coin>>` until the transaction
group is emitted. A one-line candidate changed that insertion to
`emplace_back(key.n, std::move(coin))`. The next cursor decode overwrites the
moved-from local, while moving a `Coin` can transfer a dynamically allocated
script rather than copying it. GitHub PR searches for `WriteUTXOSnapshot`,
`dumptxoutset`, `coins.emplace_back`, and `std::move` found no already-pushed
bitcoin/bitcoin proposal for this exact change; the related historical
snapshot-format PR #26045 uses the existing copying insertion.

The candidate built and passed the deterministic snapshot test:

```text
ninja -C build bitcoind test_bitcoin -j8
build/test/functional/test_runner.py rpc_dumptxoutset.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-dumptxoutset-move-coin --timeout-factor=2
```

Its full local height-957779 result returned the same 166,350,731 coins,
base/serialized hashes, and a byte-identical 8.9 GiB snapshot (SHA-256
`f85de775f38c3aea3d07e7d210181b836e189f3d04a6af6e74a721946fd3d3d4`) as
the preceding current-tree control. Both daemons were network/wallet disabled
and pinned to CPU 3:

| version | wall | daemon task time | instructions | branches |
| --- | ---: | ---: | ---: | ---: |
| base | 233.56 s | 221.855 s | 2.050 T | 328.342 B |
| move candidate | 233.62 s | 227.415 s | 2.138 T | 345.867 B |

Wall time is unchanged within 0.03%, but the candidate used 2.51% more daemon
CPU time, 4.31% more instructions, and 5.34% more branches. Any saving for
dynamically allocated scripts is dominated on this corpus by the changed
move/lifetime work; the aggregate CPU work is worse. Decision: reject and
revert the one-line change. Raw output is under
`/mnt/my_storage/bitcoin-perf-scratch/dumptxoutset-move-coin.candidate1.KS1d9a/`;
the paired base control is
`/mnt/my_storage/bitcoin-perf-scratch/dumptxoutset-hash-stream.base1.pmlQgU/`.

### `dumptxoutset` rollback callback batching

`CreateRolledBackUTXOSnapshot` copies every current UTXO into a temporary
database before disconnecting the requested blocks. Like the accepted
`ComputeUTXOStats` optimization, its loop called the RPC shutdown callback
through `std::function` for every coin. A temporary candidate kept the first
call immediate and checked every 8,192 processed coins instead. Searches for
`Copying UTXO set`/interruption, `CreateRolledBackUTXOSnapshot` performance,
`dumptxoutset`/`rpc_interruption_point`, and rollback `temp_cache.Flush` found
no matching already-pushed bitcoin/bitcoin change.

The superficially similar change does not transfer to this write-heavy loop.
One controlled base/candidate pair used the permitted height-957779 mainnet
chainstate, an in-memory temporary DB, rollback only to height 957778, CPU 3,
wallet/network disabled, and a FIFO consumed by `cat` so snapshot output did
not reach disk. Each trial was stopped immediately after the logged copy
completion boundary, before the subsequent hash and snapshot-write stages.
Both copied 166,350,731 UTXOs:

| version | copy start to completion | daemon task time through stop | instructions | branches |
| --- | ---: | ---: | ---: | ---: |
| per-coin base | 495 s | 527.469 s | 4.252 T | 812.882 B |
| 8,192-entry candidate | 497 s | 530.969 s | 4.321 T | 822.235 B |

The candidate regressed the directly logged copy phase by 0.40%, overall
daemon task time by 0.66%, instructions by 1.61%, and branches by 1.15%.
The base had 6,450 major faults while the later candidate had none, which
would favor the candidate rather than explain its regression. The change is
therefore rejected and fully reverted. This experiment does not claim a full
`dumptxoutset` timing because it intentionally terminated after the only code
region changed. Raw logs, FIFO harnesses, `time`, affinity, and `perf stat`
files are under
`/mnt/my_storage/bitcoin-perf-scratch/dumptxoutset-rollback-interrupt.{base1.xykOaf,candidate1.PtpTin}/`.

### Cursor key view instead of per-UTXO `COutPoint` copies

The only concrete `CCoinsViewCursor` implementation is the coins-DB cursor,
which already holds its decoded current `COutPoint` in `keyTmp`. The stats and
snapshot loops nevertheless ask it to copy that 36-byte outpoint into a local
on every UTXO. A temporary shared-interface candidate added
`GetKeyPtr() -> const COutPoint*`, retaining `GetKey(COutPoint&)` and the
existing null/failure behavior for malformed keys. It used the view in both
`ComputeUTXOStats` variants, snapshot writing, and rollback copying. The view
was documented as valid only until the next cursor operation. Searches for
`CCoinsViewCursor`, `CCoinsViewDBCursor`, `GetKeyPtr`, and `GetKeyView` found
no matching already-pushed bitcoin/bitcoin PR; PR #35191 is an unrelated
malformed-first-key fix already present in the current cursor behavior.

The temporary change built and passed the focused cursor and relevant RPC
tests:

```text
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=coins_tests_dbbase --log_level=message
build/test/functional/test_runner.py rpc_blockchain.py rpc_dumptxoutset.py --jobs=1 --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-cursor-keyptr --timeout-factor=2
```

All five `gettxoutsetinfo none` results on the local height-957779 chainstate
were byte-identical (SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`).
The first candidate run was cold (127.73 s and 8,196 major faults) and is not
used below. The four subsequent candidate runs and three fresh, reverted base
controls were all warm, network/wallet disabled, and pinned to CPU 3:

| version | median wall | range | median daemon task time | median instructions | median branches |
| --- | ---: | ---: | ---: | ---: | ---: |
| base | 59.10 s | 59.01-59.76 s | 59.016 s | 549.626 B | 115.548 B |
| key-view candidate | 65.96 s | 65.88-65.97 s | 65.845 s | 635.646 B | 132.260 B |

The candidate regressed wall time 11.61%, daemon task time 11.57%,
instructions 15.65%, and branches 14.46%. Eliminating the visible outpoint
copy does not overcome the cost of this wider cursor-view shape in the actual
scan. Decision: reject and fully revert it rather than expose a new shared
cursor API. Raw candidate results are under
`/mnt/my_storage/bitcoin-perf-scratch/gettxoutsetinfo-keyptr.candidate1.1O8X4Q/`;
the fresh control is
`/mnt/my_storage/bitcoin-perf-scratch/gettxoutsetinfo-keyptr.base1.r3TjeJ/`.

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

A later current-tree trace confirms that the isolated allocation-saving line
does not have enough reach to justify reopening that coupled design. From the
last daemon section of the clean 287000-height, `-dbcache=450` OverlayFS
reindex used for the 4 KiB block-size control, `UpdateTip` cache telemetry
shows exactly five full cache resets after the initial genesis setup:

| reset height | cache before | cache after | entries before |
| ---: | ---: | ---: | ---: |
| 207934 | 434.9 MiB | 0.3 MiB | 3,155,908 |
| 231364 | 434.8 MiB | 0.8 MiB | 3,156,317 |
| 244099 | 434.9 MiB | 0.3 MiB | 3,186,835 |
| 269395 | 434.9 MiB | 0.5 MiB | 3,187,541 |
| 279817 | 435.0 MiB | 0.3 MiB | 3,186,915 |

Thus the intended saving is at most five pool/map destructions over a 561.307 s
run, not a per-block cost. Retaining a roughly 435 MiB allocation after each
flush would also keep the current capacity-based accounting at its trigger;
changing that requires the broader active-memory and cache-reservation
semantics that #59 bundles. The trace rules out a simple, independent
reindex-speed commit. It was obtained without a new source diff:

```sh
start=$(rg -n 'Bitcoin Core version' <debug.log> | tail -1 | cut -d: -f1)
sed -n "${start},\$p" <debug.log> | awk '<extract UpdateTip cache drops>'
```

Raw log: `/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/blocksize4-base-1/metrics/debug.log`.
The PR #59 part of the remaining-seed list is closed; a future cache-accounting
proposal must first establish a new invariant and benefit beyond these five
events.

### `CheckTransaction` duplicate/null fast path ([PR #180](https://github.com/l0rinc/bitcoin/pull/180))

The final direct-Core part of #180 special-cases one- and two-input
transactions, replacing the current `std::set<COutPoint>` duplicate check with
direct comparisons, and uses a sorted `std::vector<COutPoint>` plus adjacent
comparison for three or more inputs. It also finds null prevouts in the sorted
prefix. This is semantically plausible: `IsCoinBase()` is exactly the
one-input/null-prevout case, duplicate failure retains precedence over null
failure, and sorting groups every zero hash before a nonzero hash. A temporary
focused test covered the one-, two-, and 3+-input duplicate/null paths plus the
coinbase script-size error; it passed with the candidate. A temporary mutation
that skipped the sorted null check failed that test. The existing full
`transaction_tests` suite also passed.

Five CPU-3-pinned Release runs of the existing benchmarks gave substantial
micro improvements:

| benchmark | base median (range) | candidate median (range) | change |
| --- | ---: | ---: | ---: |
| `CheckBlockTest` | 951.706 us (949.010-955.274) | 735.652 us (733.909-738.256) | -22.7% |
| `DuplicateInputs` | 7.10255 ms (7.09917-7.16278) | 3.43159 ms (3.40617-3.46340) | -51.7% |

`CheckBlockTest` instructions fell from 7,629,967 to 6,265,108 per block;
the duplicate-input worst case fell from a median 20.55 million to 14.00
million instructions. Those results alone are insufficient for a consensus
performance commit, so the exact HDD reindex-chainstate control used a fresh
OverlayFS upperdir over `BitcoinData`, a dropped page cache before every run,
height 287000, `-dbcache=450`, and the same network-disabled command for
independently copied candidate and baseline binaries:

| version/run | wall | user | system | major faults | input blocks | output blocks |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| base 1 | 574.61 s | 468.18 s | 41.73 s | 890 | 30,990,304 | 9,456,240 |
| candidate 1 | 571.83 s | 464.00 s | 42.18 s | 894 | 30,989,416 | 9,424,936 |
| base 2 | 573.49 s | 467.92 s | 42.18 s | 892 | 30,989,392 | 9,451,520 |
| candidate 2 | 574.65 s | 463.05 s | 42.09 s | 893 | 30,990,800 | 9,431,888 |

The paired wall changes are -0.48% then +0.20%; medians are 574.05 seconds
base and 573.24 seconds candidate, only 0.14% apart and inside this workload's
HDD variation. User CPU time consistently fell 0.97% (median 468.05 to 463.53
seconds), with effectively identical I/O and faults, but that CPU movement did
not prove an end-to-end HDD speedup. Decision: do not commit a consensus-path
change for this goal. Raw benchmark JSON/text, copied binary hashes,
`/usr/bin/time -v` reports, and logs are under
`/mnt/my_storage/bitcoin-perf-scratch/checktransaction-pr180/`.

### Cached hash inside mutable `COutPoint` ([PR #162](https://github.com/l0rinc/bitcoin/pull/162))

Hypothesis: store the outpoint hash to avoid repeated SipHash work in coins
cache lookups. Rejected before benchmarking because `COutPoint::hash` and `n`
are public and mutable, with no universal invalidation point. A cached value can
survive mutation and can also be reused between deterministic and randomized
hasher instances. Equal keys could therefore produce unequal or stale hash
codes, violating the unordered-container contract. Decision: correctness
failure; do not retry without an immutable key representation or cache owned by
the container rather than `COutPoint`.

The later fork retry `cdc5144c17ccec928f860bc991e192e12c1e04aa` (`coins:
cache CCoinsMap outpoint hashes`) does not repair that contract. It adds a
mutable `size_t` to every `COutPoint` and fills it through `std::atomic_ref`
inside a new `CCoinsCacheHasher`. Normal copying of `COutPoint` still copies
the cache value without recording which hasher salt produced it, and normal
writes to the public `hash` or `n` fields do not invalidate it. The new hasher
tries to make production caches share one static random salt, but deterministic
`CCoinsViewCache` instances deliberately use a different salt; a copied key
can consequently carry a random-cache value into a deterministic map. It also
adds eight bytes to every `COutPoint`, including transaction inputs and values
that never enter `CCoinsMap`, counteracting the cache-node memory saving the
proposal is intended to obtain.

The atomic operation does not make the value semantics safe: the implicitly
generated copy/move operations access the same non-atomic member normally, so
the cache cannot be concurrently populated and copied without an additional
object-level synchronization contract. No such contract exists for this public
value type. The candidate was inspected with:

```sh
git show cdc5144c17 -- src/coins.cpp src/coins.h src/primitives/transaction.h
rg -n 'class COutPoint|COutPoint\\{|\\.prevout\\.(hash|n)' src test
```

Decision: reject without compiling or benchmarking. The affected path would be
hot (`SaltedOutpointHasher` -> `CCoinsMap` -> cache lookups), but an
unordered-container hash cache must be owned by an immutable key or the
container, not silently retained in a mutable, copyable public key.

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

All original `l0rinc/bitcoin` PR seeds named for this investigation have now
been either accepted as independently measured commits, found already upstream,
or rejected with recorded evidence. New work must begin from a current profile
of one of the target workloads rather than retuning a previously neutral cache,
allocation, mmap, or cursor idea.

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

### Skip `txsdata` allocation while assumevalid skips scripts ([PR #207](https://github.com/l0rinc/bitcoin/pull/207), [upstream PR #35663](https://github.com/bitcoin/bitcoin/pull/35663))

Both PRs were checked on 2026-07-19 and are closed with `merged_at: null`, so
this was not an already-pushed upstream fix. The candidate replaced the
unconditional
`std::vector<PrecomputedTransactionData> txsdata(block.vtx.size())` in
`Chainstate::ConnectBlock()` with an empty vector followed by
`if (fScriptChecks) txsdata.resize(block.vtx.size())`. This is locally safe:
the only indexed `txsdata[i]` uses are inside the same `fScriptChecks` branch,
and, when checks are enabled, the resize completes before any check queue can
retain pointers into the vector. When assumevalid disables scripts, the old
vector is otherwise unused.

The exact cold-HDD reindex-chainstate control used a fresh OverlayFS upperdir
over `/mnt/my_storage/BitcoinData`, `sync; echo 3 > /proc/sys/vm/drop_caches`,
`-stopatheight=287000 -dbcache=450 -blocksonly -networkactive=0 -listen=0
-dnsseed=0 -fixedseeds=0 -discover=0 -disablewallet -printtoconsole=0`, and
`perf stat` for task-clock, cycles, instructions, branches, and faults. Every
run logged `Disabling script verification at block #1`, reached height 287000,
and logged `Shutdown done`:

| version/run | daemon wall | user | system | task-clock | instructions | major faults | input / output KiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| candidate 1 (before base) | 588.145 s | 468.626 s | 61.251 s | 526.443 s | 2.634476 T | 886 | 31,021,824 / 9,456,360 |
| baseline | 582.938 s | 468.960 s | 62.305 s | 527.794 s | 2.636277 T | 888 | 31,022,832 / 9,450,832 |
| candidate 2 (after base) | 578.531 s | 466.913 s | 62.171 s | 525.621 s | 2.634045 T | 889 | 31,022,736 / 9,438,080 |

The candidate wall-time range straddles the baseline: -0.76% and +0.89%; its
two-run median is 583.338 seconds, 0.07% slower than baseline. It did lower
median task-clock by about 0.34% and median instructions by about 0.08%, but
the HDD reindex wall-time effect is not distinguishable from normal variation.
The peak RSS did not improve consistently (1,536,648 and 1,631,840 KiB for
candidates versus 1,537,056 KiB baseline), because the workload is dominated
by the chainstate cache and LevelDB activity rather than this short-lived
per-block vector.

Decision: reject the source change for this goal. It has a valid local
allocation-saving rationale and may be useful to revisit for deliberately
large assumed-valid blocks or a memory-constrained workload, but it does not
provide the required measurable HDD reindex speedup. The candidate diff was
fully reverted. Raw command harness and outputs are in
`/mnt/my_storage/bitcoin-perf-scratch/run_reindex_writebuf_trial.sh` and
`/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/txdata207-{candidate1,baseline1,candidate2}/metrics/`.
### Reject reindex-wide script-check bypass (`84d2706c57`)

An older local branch (`l0rinc/l0rinc/assumevalid-reindex`) proposes making
`fScriptChecks` false whenever the node is reindexing. It is not present in
the freshly fetched `origin/master`, but it has no associated pull request and
must not be treated as a performance candidate. The old patch predates the
current `script_check_reason` spelling, but its proposed condition is
unambiguous: it would make a normal `-reindex-chainstate` skip script checks
without requiring an externally verified assumed-valid ancestor.

Current `Chainstate::ConnectBlock()` uses `fScriptChecks` to gate both
contextual `GetTransactionSigOpCost()` and `CheckInputScripts()`. The latter
is the consensus script interpreter. `-assumevalid=0`, an unknown/non-ancestor
assumed-valid hash, insufficient chainwork, a recent header, and blocks above
the assumed-valid height deliberately leave `script_check_reason` non-null so
that those checks run. Replacing this decision with a reindex flag would accept
invalid scripts during a user-requested chainstate rebuild and cannot be
validated as a speed optimization.

Decision: reject without compiling or timing it. The exact current dataflow is
`script_check_reason` -> `fScriptChecks` (`src/validation.cpp`) ->
`GetTransactionSigOpCost` and `CheckInputScripts`; this is a correctness proof
that the candidate removes consensus checks rather than redundant work. No
source diff was retained. The only safe related family remains the committed
assumevalid-specific gates above, which retain the existing requirement that
the historical chain is externally verified.

### Reject standard-script legacy-sigops fast path (`d76d7531df`)

Local branch `l0rinc/l0rinc/short-circuit-known-script-types` contains the
2025 seed `d76d7531df` and it is absent from freshly fetched `origin/master`;
the commit has no associated GitHub pull request. The current tree still has
the relevant pre-contextual `CScript::GetSigOpCount()` call from
`GetLegacySigOpCount()` in `CheckBlock()`, so a small reimplementation was
evaluated rather than applying the stale patch verbatim.

The temporary switch recognized only exact fixed-size script forms whose
legacy sigop count follows directly from their opcode layout: empty, anchor,
P2WPKH, P2SH, P2PKH, P2WSH, P2TR, and compressed/uncompressed P2PK. All other
scripts fell through to the existing parser. Focused unit cases for each
template passed with:

```
ninja -C build test_bitcoin bench_bitcoin -j4
build/bin/test_bitcoin --run_test=sigopcount_tests --log_level=test_suite
```

Five CPU-3-pinned `CheckBlockTest` runs (`-min-time=5000`) did show the local
effect: baseline median 948,169 ns/block (947,086--950,508), candidate median
865,012 ns/block (864,892--867,605), a 8.77% reduction. Instructions fell
from 7,629,700 to 6,601,021 per block (13.48%) and branches from 1,211,223 to
1,023,329 (15.51%).

The decisive cold-HDD OverlayFS reindex-chainstate pair was nevertheless
neutral. It used the same height-287000, `-dbcache=450`, dropped-page-cache,
and network-disabled control as the assumevalid measurements; both logs
started at height 0, reached height 287000, and shut down cleanly:

| version | daemon wall | user | system | task-clock | instructions | branches | major faults | input / output KiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 502.176 s | 385.312 s | 61.862 s | 443.740 s | 2.133340 T | 214.085 B | 888 | 31,023,464 / 9,446,640 |
| candidate | 502.199 s | 382.835 s | 61.108 s | 440.550 s | 2.104613 T | 208.668 B | 885 | 31,023,048 / 9,422,480 |

The candidate is 0.0046% slower in wall time, well within noise, although it
uses 0.64% less user CPU, 0.72% less task-clock, 1.35% fewer instructions, and
2.53% fewer branches. The unchanged wall time with virtually identical I/O and
faults shows that this CPU-side win has insufficient reach in this HDD replay.
Decision: reject and fully revert the source and test diffs; do not make a
consensus-path commit based on a microbenchmark alone. Raw harness and outputs
are `/mnt/my_storage/bitcoin-perf-scratch/run_reindex_writebuf_trial.sh` and
`/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/sigopfast-{candidate1,baseline1}/metrics/`.

### Reject `CheckTxInputs` fetch reuse (`9fb074106f`)

Local seed `9fb074106f` replaces `CheckTxInputs()`'s up-front
`inputs.HaveInputs(tx)` pass with a spent-coin test inside the later value and
maturity loop. It is absent from freshly fetched `origin/master`. The fork
commit query maps it to a closed, unmerged PR, so it is not excluded merely as
an already-pushed upstream fix; it is rejected on semantics instead.

The change is not observationally equivalent. Today `HaveInputs()` checks
*every* prevout first, before any coinbase-maturity or amount rule. A
transaction whose first input is an existing immature coinbase and whose later
input is missing therefore returns `TX_MISSING_INPUTS`. The candidate examines
the first coin first and returns `TX_PREMATURE_SPEND`, never reaching the
missing input. This follows directly from `CCoinsViewCache::HaveInputs()` and
the proposed loop; no benchmark can make the changed result safe.

The distinction has live policy reach: `AcceptToMemoryPool` passes the result
through `node::ToTransactionError()`, where `TX_MISSING_INPUTS` becomes
`TransactionError::MISSING_INPUTS`; the orphanage and tx download manager
explicitly use `TX_MISSING_INPUTS` to retain/process possible orphan
transactions. Reclassifying the described transaction changes peer and RPC
behavior. Decision: reject without applying, building, or timing the patch.
No source diff was retained. A safe optimization would have to preserve the
all-input availability pass or preserve its exact error precedence, which this
seed does not.

### Reject bespoke jumboblock SipHash for the coins cache (`ac805c2917`)

Local commit `ac805c2917` (currently in unmerged fork PR #180) is absent from
freshly fetched `origin/master`. It changes `SaltedOutpointHasher`, the hash
function of `CCoinsMap`, from the current standard SipHash-2-4 specialization
to a new `PresaltedSipHasher13Jumbo`: a non-standard SipHash-1-3 variant that
injects the four uint256 limbs together and omits the usual fixed-shape padding
step.

This is not a transparent implementation optimization. `CCoinsMap` backs
`FetchCoin`, `AccessCoin`, `HaveCoin`, `SpendCoin`, and cache flushes; its
`COutPoint` keys originate in transactions and blocks processed from peers and
on disk. The current keyed SipHash-2-4 construction is the map's collision
resistance boundary. The candidate deliberately changes its round count and
compression construction, and its single fixed-output vector only establishes
implementation determinism, not keyed collision or hash-flood resistance.

Decision: reject without compiling or benchmarking it. A throughput gain does
not justify weakening or replacing the established keyed-hash construction in
an attacker-facing cache absent a cryptographic/security analysis and a
project-wide acceptance of the new primitive. The exact dataflow is
`SaltedOutpointHasher` -> `CCoinsMap` -> coin-cache lookups and updates. No
source diff was retained; this goal is limited to behavior-preserving direct
Core speedups.

### Current accepted-stack reindex profile: defer LevelDB/RocksDB work

One fresh cold-cache profile was collected on the current accepted stack,
including the assumevalid contextual-sigops change, to decide whether the
remaining work should move into LevelDB. The workload was a
network-disabled `-reindex-chainstate` over a fresh OverlayFS view of the
local `BitcoinData` copy to height 287000 with `-dbcache=450`. It used
`perf record -F 99 --call-graph dwarf` and `/usr/bin/time -v`; the page cache
was dropped before startup. The daemon completed normally in 509.25 s
(384.09 s user, 50.85 s system), with 1,600,184 KiB peak RSS, 1,010 major
faults, and 31,045,120 / 9,463,968 KiB input / output.

The kernel throttled sampling from the requested 99 Hz to 1 Hz. The 463-cycle
sample is consequently useful only for selecting investigations, not for
claiming precise percentages or measuring a change. Its strongest identifiable
hot paths were:

| coarse self-sample signal | path | interpretation |
| --- | --- | --- |
| 12.55% | `std::_Hashtable<COutPoint,...>::_M_find_before_node`, including 6.91% below `CCoinsViewCache::BatchWrite` | coins-cache lookup/flush work; the known duplicate `BatchWrite` lookup removal is already in `origin/master` and must not be reintroduced. |
| 8.94% | SHA256 `Lloop1` while `BlockManager::ReadBlock` constructs transactions | candidate direct-Core investigation: determine whether hash construction during block deserialization is required by the cached-hash contract. |
| 6.79% | `PresaltedSipHasher::operator(uint256, uint32)` | normal keyed coins-cache hashing; the proposed bespoke SipHash replacement is rejected above for security reasons. |
| 4.26% | LevelDB memtable skip-list `FindGreaterOrEqual` | real but below the cache and transaction-construction signals. |
| 0.56% | LevelDB bloom-filter lookup | too small to justify a LevelDB/RocksDB port or invasive storage change. |

This profile therefore does **not** justify touching LevelDB or consulting
RocksDB implementation details yet. The next candidate is the direct Core
transaction-hash construction path, and any change must first prove that it
preserves transaction immutability, hash caching, deserialization, and all
callers. Raw profiler and daemon data are retained under
`/mnt/my_storage/bitcoin-perf-scratch/reindex-profile-current/metrics/`
(`perf.data`, `time.txt`, and `debug.log`).

### Reject lazy `CTransaction` hashes ([fork PR #21](https://github.com/l0rinc/bitcoin/pull/21))

Fork commit `feb458e6bf` is still open and unmerged, and is absent from freshly
fetched `origin/master`, so it passes the already-pushed exclusion. It replaces
the two eager, inline `CTransaction` hash values with heap-allocated mutable
pointers guarded by `std::once_flag`; it also adds custom copy/move/destructor
operations. `GetHash()` then computes both txid and wtxid on first access.
The patch itself has no test change.

This cannot remove the reindex work identified by the profile. A normal
connection calls `Chainstate::ConnectBlock(..., fJustCheck=false)`, which calls
`CheckBlock(..., fCheckMerkleRoot=true)`. `CheckMerkleRoot()` immediately
calls `BlockMerkleRoot()`, and that calls `GetHash()` for every transaction.
For blocks with a witness commitment, `CheckWitnessMalleation()` likewise calls
`BlockWitnessMerkleRoot()`, which calls `GetWitnessHash()` for every
non-coinbase transaction. The same cached hashes are subsequently used for
coin creation and validation diagnostics. Thus the 8.94% coarse
`CTransaction::ComputeHash()` sample from the reindex profile is required
consensus work, not lazily avoidable construction work.

The BIP30-specific assumevalid candidate is deliberately not in this stack.
Its checks are meaningful only in the early historical height range, where the
blocks are nearly empty; the user rejected that narrow reach despite its
short-range benchmark. The retained contextual-sigops change has broader
assumevalid reach and is documented and tested independently.

The candidate would only move this hashing from construction to the immediate
merkle check in the target workload, while adding pointer allocation,
deallocation, `call_once` synchronization, and nontrivial copy/move behavior
to a widely used primitive. Other common deserialization paths (mempool and
P2P transaction processing) request txid/wtxid immediately as well. No
separate workload that retains deserialized transactions without their hashes
has been demonstrated. Decision: reject without applying, building, or timing
the patch; its expected reindex-chainstate reach is zero.

### Exclude already-pushed `ReadBlock` header-hash reuse

The similarly named fork candidate `8117c16fd` avoids recomputing a block
header hash inside `BlockManager::ReadBlock()` by reusing it for proof of work
and the optional index-integrity comparison. It must be ignored under the
upstream-pushed rule: the equivalent commit `09ee8b7f278627b917f0784adf23cbc76cae5fa0`
(`node: avoid recomputing block hash in ReadBlock`) is reachable from freshly
fetched `origin/master`, and the current source already has its
`expected_hash` overload and one computed `block_hash`. No duplicate change,
benchmark, or further investigation is warranted.
### Heap merging iterator reach: neutral for reindex-chainstate

The heap change above was then checked against the same cold-HDD OverlayFS
`-reindex-chainstate` control used for prior direct-Core candidates. A detached
worktree at its parent `b19dcade20` supplied the baseline; `de88ee88c0`
supplied the heap version. Each run used the local `BitcoinData` as a
read-only OverlayFS lower layer, a separate scratch upper/work/merged layer,
dropped page cache before startup, `-stopatheight=287000`, `-dbcache=450`, and
network-disabled/wallet-disabled flags. Each newly written log reached height
0, height 287000, and `Shutdown done` after the run; the overlay mount was
then removed. Thus the local source data was not modified.

| version | daemon wall | user | system | task-clock | instructions | branches | major faults | input / output KiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| parent baseline | 499.902 s | 386.764 s | 62.384 s | 445.745 s | 2.131096 T | 213.476 B | 889 | 31,023,056 / 9,178,000 |
| heap candidate | 500.661 s | 386.634 s | 62.482 s | 445.575 s | 2.134999 T | 214.144 B | 886 | 31,022,712 / 9,454,672 |

The candidate is 0.15% slower in one pair, with 0.18% more retired
instructions and 0.31% more branches. These differences are too small and in
the wrong direction to claim any reindex-chainstate benefit. This is expected
reach separation, not a reason to revert the full-UTXO-scan optimization:
reindex's earlier profile was dominated by coins-cache and transaction work,
whereas the `gettxoutsetinfo none` profile directly exposed forward LevelDB
merge selection. The accepted change may still serve compaction and other
forward merged iterators, but it is not a demonstrated HDD reindex speedup.
Raw commands and metrics are retained in
`/mnt/my_storage/bitcoin-perf-scratch/run_reindex_writebuf_trial.sh` and
`/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/mergerheap{base,}-1/`.

### Reject `scantxoutset` selective coin deserialization

The scan loop already rejects a decoded script by size and first byte before
performing the descriptor-set lookup. A temporary direct-Core candidate moved
that rejection into coin deserialization: it consumed the height and compressed
amount, inspected the compressed script's possible output size/first byte, and
only deserialized a full `Coin` when a requested script could match. P2PKH,
P2SH, compressed P2PK, ordinary, oversized, and empty scripts have an exact
shape at that point. Compressed encodings for uncompressed pubkeys need a
conservative exception: an invalid curve point becomes an empty script in
normal `DecompressScript`, so the candidate retained those entries whenever an
empty script was requested.

The temporary unit test compared the partial shape with full coin
deserialization for every common compression form and crafted an invalid
compressed uncompressed-pubkey record. The latter fully deserialized to an
empty script while the prefilter correctly recorded both its usual 67-byte
shape and its possible-empty state. This established that the proposed filter
would not silently omit that malformed on-disk case. The temporary code and
test were fully reverted after measurement.

The controlled full-chain comparison used separate current-tree and candidate
Release binaries, the explicitly permitted local `BitcoinData` at height
957779, 166,350,731 UTXOs, one CPU-3-pinned network/wallet-disabled daemon per
variant, `-checkblocks=1`, and one unmeasured warm scan before two measured
no-match `raw(6a)#4mhr9ur5` scans. All four 199-byte RPC replies had SHA-256
`62539678afd6931c917d0135fedcc12d3f11e9e40958fe4983e461a5f0d0891a`.

| version/runs | RPC wall seconds | daemon task-clock | instructions | branches | branch misses | major faults |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| current baseline | 50.48, 50.42 | 50.351, 50.307 s | 455.580, 455.585 B | 94.610, 94.609 B | 502.023, 502.005 M | 0, 0 |
| selective-parser candidate | 50.41, 50.33 | 50.280, 50.218 s | 456.014, 456.012 B | 94.489, 94.489 B | 552.647, 545.435 M | 0, 0 |

The candidate's two-run wall median is only 0.16% lower and its task-clock is
also only 0.16% lower, while instructions rise 0.09% and branch misses rise
about 9%. That is not a credible speedup, and the extra cursor overload,
compressed-script shape parser, and malformed-pubkey state would make the scan
path more complex. Reject the idea rather than expanding the sample.

Raw replies, wall times, perf CSVs, daemon logs, and isolated cookies are in
`/mnt/my_storage/bitcoin-perf-scratch/scantxoutset-prefilter-runs/{baseline.fBuhev,candidate.h2wUih}/`.
Freshly fetched `origin/master` at
`18c05d93016b28a9afd4c716dfe00b6e0accb30b` contains neither `CoinScanInfo`
nor `UnserInfo`; this was an independently tested but rejected candidate, not
a duplicate of an already-pushed fix.

### Obfuscated iterator values: avoid word setup for one-byte reads

A warm full-chain `gettxoutsetinfo none` profile on the current accepted stack
identified `ReadVarInt<ObfuscatedSpanReader>` as a plausible direct-Core lead.
The profile had only 48 usable samples, so its apparent 12.96% combined sample
share is routing evidence only, not a percentage claim. Inspection established
that each byte read presently copies one byte, zero-initializes a `uint64_t`,
XORs the word through `Obfuscation::operator()`, then copies one byte back.
Coin stats deserialization performs many such reads for compact-size and
VARINT fields.

The retained change adds `Obfuscation::ObfuscateByte()` and uses it only for a
one-byte `ObfuscatedSpanReader::read()`. It XORs the byte with the matching
byte in the original key representation, advances `m_key_offset` once, and
advances the source span once. Multi-byte reads retain the established,
optimized word/chunk implementation. The implementation is endian-safe: the
key was originally copied into `m_rotations[0]`, and `HexKey()` already exposes
that same object representation as the key bytes.

A temporary two-one-byte-VARINT benchmark (five runs per version) measured
6.47--6.49 ns and 94 instructions at base versus 6.16--6.30 ns and 90
instructions with the shortcut (medians 6.49 -> 6.18 ns, 4.8% faster). That
alone was insufficient to commit, so it was removed after routing the
end-to-end check below.

The macro comparison used the explicitly permitted local
`/mnt/my_storage/BitcoinData` chainstate at height 957779, one normal
network-disabled/wallet-disabled daemon per build, and `gettxoutsetinfo none`.
Each candidate daemon first completed an unmeasured warm scan. Measured scans
attached `perf stat` to the daemon for `task-clock`, cycles, instructions, and
branch counters; every response had SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`.

| version/runs | RPC wall seconds | daemon task-clock | instructions | branches | branch misses |
| --- | ---: | ---: | ---: | ---: | ---: |
| base | 47.80, 47.96 | 47.749, 47.908 s | 437.937, 438.004 B | 89.376, 89.377 B | 500.469, 508.194 M |
| one-byte reader | 47.24, 47.36 | 47.185, 47.290 s | 435.653, 435.667 B | 87.957, 87.960 B | 484.138, 488.327 M |

The two base instruction counts differ by 0.015%, and the two candidate
counts by 0.003%; the candidate consistently retires 0.53% fewer instructions.
Its two-run wall median is 1.2% lower, but the small sample should not be read
as a precise wall-time claim. The defensible result is a reproducible CPU
reduction in a full 166,350,731-UTXO scan, with matching observable output.

`dbwrapper_tests/obfuscated_span_reader` covers nonzero key bytes, a one-byte
read, `ignore()` across two key offsets, a multi-byte read, and a wrapped
single-byte key offset. It passes with the shortcut. As a temporary mutation,
forcing the shortcut to always use offset zero made a standalone program using
the same production headers fail at the byte after `ignore()` (exit 2); the
unmutated program exits 0. This proves the added oracle catches the critical
offset accounting error rather than merely executing the path.

Expected reach is CPU time after an obfuscated LevelDB value is available, not
disk I/O. It directly covers chainstate iterator users including
`gettxoutsetinfo`, `scantxoutset`, and `dumptxoutset`, and any other
`CDBIterator::GetValue()` use that deserializes byte-at-a-time fields. It does
not accelerate plain LevelDB data, LevelDB merge selection, block reads, or
HDD seeks. Raw microbenchmark outputs, RPC replies, perf CSVs, and timing
files are retained in `/mnt/my_storage/bitcoin-perf-scratch/obfuscated-reader-byte/`.

### UTXO statistics: borrow the cached cursor key

`CCoinsViewDBCursor` already caches the current `COutPoint` in `keyTmp` until
the cursor advances. Its old `GetKey(COutPoint&)` virtual method copied that
36-byte object for every UTXO. Both `ComputeUTXOStats()` loops immediately
used the copy and advanced the cursor: the no-hash loop only needs the txid to
detect transaction boundaries, and the hashed loops pass the key straight to
the serialization/hash helper. The copy is therefore unnecessary in all UTXO
statistics modes.

The cursor now exposes a borrowed `const COutPoint*`, documented as valid only
until cursor movement. The existing `GetKey(COutPoint&)` interface remains as
a nonvirtual copying wrapper for callers that need an owned key. The two
statistics loops borrow the cached key, use it before `Next()`, and preserve
their per-transaction `prevkey` copy (which is needed after cursor movement).
No key serialization, ordering, value deserialization, or error behavior
changes.

The full-chain measurement used the local `BitcoinData` chainstate at height
957779, one normal network-disabled/wallet-disabled daemon per build, an
unmeasured warm `gettxoutsetinfo none`, and two subsequent RPCs with daemon
`perf stat` counters. Every measured JSON reply had SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`.

| version/runs | RPC wall seconds | daemon task-clock | instructions | branches | branch misses |
| --- | ---: | ---: | ---: | ---: | ---: |
| copying key | 47.37, 47.50 | 47.320, 47.452 s | 435.176, 435.308 B | 88.045, 88.048 B | 496.993, 502.878 M |
| borrowed key | 46.89, 46.86 | 46.811, 46.791 s | 428.139, 428.140 B | 86.867, 86.867 B | 506.050, 506.841 M |

The candidate instructions vary by only 0.00024% between its two scans and
are 1.63% below the base median; task-clock and wall medians improve by 1.23%
and 1.18%, respectively. The small two-run wall sample remains secondary to
the stable instruction result. Raw replies, timings, perf CSVs, logs, and
cookies are retained in
`/mnt/my_storage/bitcoin-perf-scratch/cursor-key-reference/`.

`coins_tests_dbbase` now uses both cursor forms over output indices 0, 1, 127,
128, 255, 256, and `uint32_t` max, verifies that both forms agree, and advances
with `NextNoKey()`. This covers byte/VARINT key decoding, borrowed-key
lifetime through use, the retained copying compatibility path, ordering, and
the transition to a lazily decoded next key. The configured functional suite
`build/test/functional/test_runner.py rpc_blockchain.py --jobs=1` passed under
both v1 and v2 transport.

Expected reach is all `ComputeUTXOStats()` calls. `gettxoutsetinfo none` is the
demonstrated high-reach case; its `hash_serialized` and `muhash` variants share
the borrowed key but spend more time hashing, so their percentage gain should
be smaller. `dumptxoutset` uses UTXO statistics during snapshot preparation
and can benefit in that phase, but its copy/write work limits end-to-end reach.
`scantxoutset` and snapshot-writing loops retain the copying wrapper because
they need an owned key; reindex-chainstate does not use this cursor path.

### Reject fused no-hash cursor stats accessor

After borrowing cached cursor keys, the no-hash UTXO statistics loop still
made two virtual calls per UTXO: `GetKey()` and `GetValue(CoinStatsValue&)`.
A temporary `GetCoinStatsValue()` virtual combined them in
`CCoinsViewDBCursor` by retrieving the borrowed key and decoding the value in
one derived-method call. The existing methods remained available, so the
candidate was behavior-preserving in source review and built successfully.

It is nevertheless a clear regression in the only workload it targets. The
comparison used the same full local chainstate, normal network-disabled and
wallet-disabled daemons, one warm `gettxoutsetinfo none`, and daemon-attached
`perf stat` counters; all replies had SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`.

| version/runs | RPC wall seconds | daemon task-clock | instructions | branches | branch misses |
| --- | ---: | ---: | ---: | ---: | ---: |
| borrowed-key base | 47.07, 47.09 | 47.043, 47.042 s | 433.931, 434.061 B | 88.128, 88.129 B | 515.845, 515.736 M |
| fused virtual candidate | 50.18, 50.17 | 50.130, 50.117 s | 435.971, 436.032 B | 88.275, 88.274 B | 731.854, 733.090 M |

The candidate takes 6.5% more task-clock and 6.6% more RPC wall time, retires
0.46% more instructions, and has about 42% more branch misses. Both runs are
internally stable, so this is not measurement noise. The added virtual ABI/API
surface also weakens the case for retaining a negative result. Decision:
fully revert the source and test prototype; do not trade a theoretically saved
dispatch for demonstrably worse code generation. Raw artifacts are retained in
`/mnt/my_storage/bitcoin-perf-scratch/cursor-fused-stats/`.

### Reject fork PR #201 fixed-extent serialization as a UTXO/RPC seed

The remaining potentially related fork seed was draft
`l0rinc/bitcoin#201` (head `d2c3e708ed81b4e56aa4446c2f3a5e642a111bc1`,
"Serialization specializations"). Its five commits benchmark and optimize
`SizeComputer` and fixed-extent `write()`/`read()` stream overloads. The
reported target is block serialization and block-size computation; it is an
alternative to upstream PR #31868, not an established UTXO database or
chainstate-iterator optimization.

The exact patch was fetched without modifying this worktree and inspected with
`git show d2c3e708 -- src/serialize.h src/streams.h`. The final change adds a
fixed-extent `SizeComputer::write()` fast path and special-cases fixed spans
in generic stream wrappers. Current-tree call-site search,
`rg -n -C 3 "GetSerializeSize\\(" src/validation.cpp src/node/blockstorage.cpp
src/rpc/blockchain.cpp src/coins.cpp src/txdb.cpp`, found no call in the
`ComputeUTXOStats()` or `scantxoutset` cursors. Thus it cannot accelerate
`gettxoutsetinfo`, `scantxoutset`, or the snapshot iteration/writing portions
of `dumptxoutset`.

For reindex-chainstate, the relevant current calls are the per-block consensus
size check in `CheckBlock()` and normal block-storage size accounting. The
latter is on block/undo writing paths rather than a chainstate-only replay;
neither establishes the many-UTXO or HDD I/O reach required by this
investigation. Replacing the current generic serialization interfaces with a
large, draft specialization series would therefore have high implementation
risk and no demonstrated target workload. Do not prototype or commit it for
this goal. It remains a separate IBD/block-serialization lead if a future
profile identifies size computation as material.

### Reject skipped unused coin-height VARINT decoding in un-hashed stats

`CoinStatsValue::Unserialize()` reads the height/coinbase `uint32_t` first in
every serialized coin, but un-hashed UTXO statistics use only the amount and
the decompressed script size. An assembly probe confirmed that this path still
called `ReadVarInt<uint32_t>` even though its final value was not retained. The
candidate introduced a temporary `SkipVarInt()` with the same overflow errors
and byte-consumption order as `ReadVarInt()`: it read the first byte, returned
immediately for a terminating byte, and tracked the accumulated integer only
after a continuation bit required further overflow checks.

A temporary unit test compared `SkipVarInt<uint32_t>` with `ReadVarInt` for
the one-, two-, three-, and five-byte boundaries (0, 127, 128, 16511, 16512,
and `uint32_t` max). It also compared success/failure and remaining input for
a truncated continuation and two overflowing sequences. The focused
`serialize_tests/skip_varint_matches_read_varint` and existing
`coins_tests/coin_stats_value_matches_coin_deserialization` passed before the
temporary source and test were reverted. This proves the proposed parser model
was behavior-equivalent for the checked encodings, but does not prove a speedup.

The decisive paired macro benchmark used the explicitly permitted local
`BitcoinData` chainstate at height 957779, normal network-disabled and
wallet-disabled daemons, one warm `gettxoutsetinfo none` per binary, then two
daemon-attached `perf stat` scans per binary. The base and candidate differed
only in whether `CoinStatsValue` used the original `ReadVarInt` result or the
temporary skip helper. Every JSON result had SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`.

| version/runs | RPC wall seconds | daemon task-clock | instructions | branches | branch misses |
| --- | ---: | ---: | ---: | ---: | ---: |
| original `ReadVarInt` | 46.56, 46.45 | 46.495, 46.390 s | 428.134, 428.134 B | 86.866, 86.866 B | 510.101, 504.386 M |
| temporary `SkipVarInt` | 47.46, 47.45 | 47.391, 47.408 s | 437.475, 437.493 B | 88.546, 88.549 B | 507.028, 504.431 M |

The candidate samples vary only 0.004% in instructions and the base samples
only 0.0002%. The candidate retires 2.18% more instructions, takes 2.06% more
task-clock, and has about 1.94% more branches. The intended shortcut therefore
made code generation materially worse despite avoiding an unused final value.
Fully revert the helper and test; do not replace a compact, well-optimized
general parser with a specialized duplicate without a demonstrably better
whole-program result. Raw replies, time files, perf CSVs, logs, and cookies
are retained in `/mnt/my_storage/bitcoin-perf-scratch/skip-varint-base.jK4NTU/`
and `/mnt/my_storage/bitcoin-perf-scratch/skip-varint-candidate.ozYUNM/`.

### Reject historical buffered `ReadBlockFromDisk` seed as superseded

The fork branch `l0rinc/buffered-ReadBlockFromDisk` contains historical commit
`41a8220f1d7286e6f9a32ff14ea9275ddd505d5a`,
`blockstorage: Use BufferedFile in ReadBlockFromDisk instead of AutoFile`.
Its premise was sound for its old implementation: deserialize a block through
a small `BufferedFile` ring buffer, rather than issuing one `fread()`-backed
`AutoFile::read()` for every serialized field. The commit reports a 9% saving
inside block deserialization and about 1.2% on an SSD-like reindex benchmark,
so it initially appeared to be an appropriate direct HDD replay seed.

It is not an applicable change in the live tree. Both the investigation base
and `origin/master` have no diff in `src/node/blockstorage.cpp`; current
`BlockManager::ReadBlock()` first calls `ReadRawBlock(pos)` to read the complete
serialized block into one byte vector, then deserializes with `SpanReader`.
This already eliminates the old per-field file-read pattern without retaining
a fixed-size ring buffer or its rewind/error-boundary complexity. Current
`LoadExternalBlockFile()` separately retains `BufferedFile` because that path
must scan for markers and rewind around malformed external block-file input;
it is not the regular indexed-block replay path.

Inspection commands were:

```sh
git show 41a8220f1d -- src/node/blockstorage.cpp
git diff --stat origin/master -- src/node/blockstorage.cpp
rg -n "ReadRawBlock\\(|BlockManager::ReadBlock\\(" src/node/blockstorage.cpp src/node/blockstorage.h
```

Do not resurrect the old buffered wrapper, and do not spend a destructive HDD
reindex run comparing it with the already-more-direct raw-block path. A future
block-reader candidate must beat the existing single bulk read and preserve
its partial-read and error-reporting contracts.

### Reject sorted `CCoinsViewDB::BatchWrite` batches

Fork branch `l0rinc/sorted-BatchWrite` contains `eec8edea2142d72e2bceb55831a468940ec84a88`,
`optimization: sort BatchWrite batches in descending order`, followed by
commits titled `Collect unspent hashes from cursor` and `Sort unspent dirty
outpoints by hash`. It accumulates dirty entries in a `std::vector`, sorts them
by outpoint hash, then serializes and writes that vector in place of the
existing cursor-order batches. This looks HDD-relevant at first glance because
chainstate flushes occur during reindex-chainstate.

The mechanism does not establish an HDD I/O win. `CDBWrapper::WriteBatch()`
passes the ordered `leveldb::WriteBatch` to LevelDB, whose
`WriteBatchInternal::InsertInto()` inserts each record into the comparator-
ordered memtable before tables are emitted. Reordering the application batch
therefore cannot make the resulting memtable/SST key order more sequential;
it instead adds an `O(n log n)` sort and copies/moves every dirty `Coin`.

More importantly, the fork substitutes the existing encoded-byte boundary
`batch.ApproximateSize() > m_options.batch_write_bytes` with
`batch_write_bytes / sizeof(std::pair<COutPoint, Coin>)`. `Coin` has dynamic
script storage, so that is neither an encoded-size bound nor a stable memory
bound. It changes when crash-recovery head markers are committed, can create a
zero-entry threshold for small debug `-dbbatchsize` values, and adds a large
second representation of a dirty flush. A broad recovery-path change requires
clear macro evidence, not an unprofiled locality intuition.

Reviewed with:

```sh
git show eec8edea21 -- src/txdb.cpp
sed -n '125,230p' src/txdb.cpp
sed -n '105,150p' src/leveldb/db/write_batch.cc
sed -n '1215,1245p' src/leveldb/db/db_impl.cc
```

Reject without a prototype or reindex benchmark. Any future ordered-write
experiment must retain byte-accurate batch boundaries and demonstrate reduced
LevelDB write/compaction I/O, not merely reordered calls.

### Reject custom fixed-buffer COutPoint DB-key serialization as too small

The non-upstream fork branch `l0rinc/custom-coinentry-serialization` ends at
`b1afdcec0c7db01b1f9c8dacf5f5829abd60d750`, which replaces generic
`CoinEntry` serialization in `GetCoin`, `HaveCoin`, and `BatchWrite` with a
hand-written `WriteCOutPoint()` into a fixed byte buffer and adds raw
span-based database APIs. It reaches chainstate point lookups and flushes, so
it was screened after the larger LevelDB ideas were ruled out.

The current generic path already reserves `DBWRAPPER_PREALLOC_KEY_SIZE` (64)
bytes for read keys and reuses a 64-byte `CDBBatch::m_key_scratch` for every
batch entry. A coin key is one database-prefix byte, 32 hash bytes, and at
most five VARINT bytes: 38 bytes. Thus the candidate does not remove an
allocation or change a LevelDB read/write; it replaces a short, capacity-reused
generic serializer with custom encoding plus new raw-key API surface. The
batch value serializer, LevelDB copying, WAL, memtable insertions, and table
I/O all remain unchanged.

The current scratch lifetime is explicit: `ScopedDataStreamUsage` requires
the scratch stream to be empty at entry and clears it without releasing its
capacity after each key. This rules out the historical rationale of repeated
short-key allocations. A tiny instruction-count micro-win would not justify
duplicating the database key contract in `txdb` or claiming an HDD benefit.

Reviewed with:

```sh
git show b1afdcec0c -- src/txdb.cpp src/txdb.h src/dbwrapper.h src/dbwrapper.cpp
rg -n "DBWRAPPER_PREALLOC_KEY_SIZE|ScopedDataStreamUsage" src/dbwrapper.h src/dbwrapper.cpp src/streams.h
sed -n '145,215p' src/dbwrapper.cpp
```

Reject without code changes. Reconsider only if a profile on a real
chainstate-flush workload attributes a material fraction of time to
`CoinEntry` serialization after the existing reusable scratch buffers.

### Reject seek-compaction work as already present and upstream

The remaining historical LevelDB lead was the fork series headed by
`e73f0cd4b5cdaf213be1b696cfedcf99a76f0cc3`, `leveldb: disable seek
compactions`, and its later equivalent `e483b98eb015b048549c11f1443356634c9ba0e0`,
`Disable seek compaction`. Its diagnosis is relevant to a rotational
chainstate: LevelDB normally decrements a file's `allowed_seeks` budget for
point-read and iterator samples. Once exhausted, `Version::UpdateStats()` can
schedule a read-triggered compaction. Hash-random UTXO keys make that heuristic
capable of causing large, write-amplifying rewrites that do not reflect size
pressure.

It is not a candidate for this branch. The current live implementation already
defines `Version::UpdateStats(const GetStats&)` as an intentional no-op in
`src/leveldb/db/version_set.cc`: it returns `false` with the explanation that
the random-key heuristic can cause severe write amplification. The adjacent
`RecordReadSample()` code remains only to parse a sample and invoke that no-op;
size-triggered and manual compactions remain separate paths. Both the current
tree and freshly fetched `origin/master` have this exact implementation, so a
new commit would duplicate an already-pushed fix contrary to this
investigation's scope.

The behavior is protected rather than merely commented: `db_test.cc` creates
an L0/L2 layout, performs one thousand misses, and asserts that the file layout
is unchanged before separately proving that a manual compaction still moves
the L0 file. `autocompact_test.cc` repeatedly scans a range and asserts it does
not shrink. Re-enabling or retuning the `allowed_seeks` counter without a
full write-amplification/recovery study would reopen the known HDD risk, not
provide a simple scan or reindex speedup.

Reviewed with:

```sh
git show e73f0cd4b5 -- src/leveldb/db/version_set.cc src/leveldb/db/autocompact_test.cc
git show e483b98eb0 -- src/leveldb/db/version_set.cc src/leveldb/db/autocompact_test.cc src/leveldb/db/db_test.cc
rg -n -C 5 "RecordReadSample|UpdateStats\\(|allowed_seeks|seek compaction" src/leveldb/db
git diff --stat origin/master -- src/leveldb/db/version_set.cc src/leveldb/db/autocompact_test.cc
```

The final comparison produced no diff. Decision: do not benchmark, modify, or
port this already-applied compaction policy. Any later LevelDB proposal must
first identify a different, currently enabled source of I/O or CPU cost.

### Reject fork PR #34 cache-propagation and short-lived-cache seeds as upstream

Fork PR [#34](https://github.com/l0rinc/bitcoin/pull/34), fetched as
`l0rinc/pr-34` at `195fc2fe102197eae27c33b5e3969a6b15d1253a`, contains two
otherwise relevant ideas. The first commit, `b23c6a1fc8f9a44561add79427d2f3fff4c3718f`,
replaces a `find()` then `try_emplace()` pair in
`CCoinsViewCache::BatchWrite()` with one leading `try_emplace()` and branches
on `inserted`. That removes a redundant SipHash/bucket traversal when a child
cache propagates an entry that is absent from its parent. The second commit
adds a `Flush(reallocate_cache)` parameter so local one-block caches do not
destroy and reconstruct their map immediately before destruction.

Neither is a remaining change. The live `BatchWrite()` has the exact leading
`auto [itUs, inserted]{cacheCoins.try_emplace(it->first)}` shape, and
`origin/master` records it as `c8f5e446dc coins: reduce lookups in dbcache
layer propagation`. The live `Flush(bool reallocate_cache = true)` retains the
reallocation policy for reusable caches and all local validation/RPC teardown
callers explicitly pass `false`; `origin/master` records the current naming as
`3e0fd0e4dd refactor: rename will_reuse_cache to reallocate_cache`.

The latter has potentially broad reindex and rollback reach only because it
prevents repeated destruction/recreation of a map that will never be used
again. It is already correctly applied to `ConnectTip`, `DisconnectTip`,
snapshot activation, and the relevant `dumptxoutset` rollback cache. The
former has cache-layer propagation reach, not UTXO RPC scan reach. Reapplying
either would either be a no-op or duplicate already-pushed Core work, which is
explicitly out of scope.

Reviewed with:

```sh
git fetch -q l0rinc 'refs/pull/34/head:refs/remotes/l0rinc/pr-34'
git log --reverse --format='%H %s' $(git merge-base l0rinc/pr-34 origin/master)..l0rinc/pr-34
git show b23c6a1fc -- src/coins.cpp src/coins.h src/validation.cpp
rg -n -C 4 'try_emplace\(it->first\)|Flush\(/\*reallocate_cache=\*/false\)' src/coins.cpp src/coins.h src/validation.cpp src/rpc/blockchain.cpp
git log origin/master --oneline -G 'try_emplace\(it->first\)|Flush\(/\*reallocate_cache=\*/false\)' -- src/coins.cpp src/coins.h src/validation.cpp
```

Decision: remove PR #34 from the open seed list and do not benchmark a
duplicate. A new coins-cache candidate must differ materially from both
upstream forms and prove a measurable end-to-end effect without weakening the
cache's persistence or memory-reclamation contract.

### Reject fork PR #48 32 KiB LevelDB data blocks on cold HDD reindex

Fork PR [#48](https://github.com/l0rinc/bitcoin/pull/48), fetched as
`l0rinc/pr-48` at `a07162850e8dcbf49429704b697a39c9f7ed4607`, is mostly a
historical experiment series. Its final five commits change only
`CDBWrapper::GetOptions()` from LevelDB's default 4 KiB data block target to
1, 2, 8, 16, and finally 32 KiB. Its input-prefetch commits predate the
current `CoinsViewOverlay` work and are not part of this test. Larger data
blocks could reduce index/filter and block-restart overhead while building a
fresh chainstate, but they also increase the read unit for point lookups and
apply to every Core LevelDB database, not only chainstate.

The setting affects only newly written SSTables: `TableBuilder::Add()` flushes
a data block at `options.block_size`, while existing `/mnt/my_storage/BitcoinData`
tables retain their historical layout. A fresh OverlayFS
`-reindex-chainstate` is therefore the correct test, whereas an RPC scan over
the existing database would be a no-op. A temporary one-line
`options.block_size = 32 * 1024` prototype was built, then completely removed.

Both cold runs used the same permitted local data as a read-only OverlayFS
lowerdir, independent upper/work directories, dropped page cache, GCC Release
binary, `-stopatheight=287000`, `-dbcache=450`, wallet/network disabled, and
the harness checks for height 0, height 287000, and clean shutdown. The
candidate run did complete successfully, but it has no reliable speedup:

| metric | 4 KiB default | 32 KiB prototype | change |
| --- | ---: | ---: | ---: |
| wall | 561.307 s | 558.972 s | -0.42% |
| task-clock | 512.186 s | 521.629 s | +1.84% |
| user CPU | 454.20 s | 464.25 s | +2.21% |
| system CPU | 61.48 s | 60.96 s | -0.85% |
| instructions | 2.497 T | 2.494 T | -0.11% |
| filesystem output | 9,440,840 KiB | 9,655,352 KiB | +2.27% |
| OverlayFS upperdir | 743,959,541 B | 727,348,603 B | -2.23% |
| major faults | 892 | 894 | +0.22% |

The sub-half-percent wall difference is far below the noise bar for a costly
full reindex pair, while both task-clock and user CPU are materially worse and
filesystem output increases. The reduced final upperdir is a layout outcome,
not a justification for a slower build path; its relation to full-height disk
usage is unproven. Repeating four more pairs would not be a responsible use of
the HDD solely to rescue a wrong-direction first result. The temporary source
line was removed, leaving the upstream 4 KiB default intact.

Commands and raw artifacts:

```sh
git fetch -q l0rinc 'refs/pull/48/head:refs/remotes/l0rinc/pr-48'
git show 46ad2d3d2e -- src/dbwrapper.cpp
ninja -C build bitcoind -j4
/mnt/my_storage/bitcoin-perf-scratch/run_reindex_writebuf_trial.sh blocksize4-base 1 build/bin/bitcoind
/mnt/my_storage/bitcoin-perf-scratch/run_reindex_writebuf_trial.sh blocksize32-candidate 1 build/bin/bitcoind
```

Raw `time`, `perf stat`, logs, and upperdir-size files are under
`/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/{blocksize4-base-1,blocksize32-candidate-1}/metrics/`.
Decision: remove PR #48 from the open seed list and retain the default. Reopen
only with a profile that identifies LevelDB data-block overhead as material and
with a change scoped to the database/workload it demonstrably helps.

### Close fork PR #152/#195 cursor and coinstats seeds

Fork PR [#152](https://github.com/l0rinc/bitcoin/pull/152), fetched as
`l0rinc/pr-152` at `3ac40d0989e97e5c7d865f451d9a2561a7771dff`, contains the
three direct scan ideas that seeded this branch: specialized un-hashed UTXO
statistics, hashed `scantxoutset` script lookup with a script-size prefilter,
and reduced scan-progress/key-copy work. All performance-relevant parts are
already covered, and then extended, by the accepted commits in this stack:
`ceb0979096`, `c806cd22bd`, `f38582a6fa`, `1a5bfbe875`, `a07f0eeff7`,
`36803c075c`, `fd07d9125f`, and `405ab76014`.

In particular, the live `FindScriptPubKey()` retains PR #152's progress update
only every 8192 coins and fetches a key only for that update or a matching
coin. It further avoids eager next-key decoding through `NextNoKey()`,
prefilters by first script byte, and directly compares singleton needles. The
live un-hashed coinstats path also goes beyond the seed by streaming ordered
cursor entries, borrowing the cached key, avoiding script construction, and
batching interruption checks. Reapplying the old PR would regress those
improvements or conflict with their tests.

Fork PR [#195](https://github.com/l0rinc/bitcoin/pull/195), fetched as
`l0rinc/pr-195` at `34620b0490a1a6be681afbbdc89ba037d6143739`, is not an
additional hot-loop optimization. It narrows `Cursor()` from the abstract
`CCoinsView` interface to `CCoinsViewDB`, passes that concrete type through
the coinstats helpers, and removes now-unreachable null checks. A cursor is
created once per RPC, so this is API/layering cleanup; it neither removes a
per-UTXO virtual call nor changes LevelDB iteration. It would be inappropriate
to claim it as a measurable UTXO-RPC or HDD-reindex speedup without a separate
maintainability goal.

Reviewed with:

```sh
git fetch -q l0rinc 'refs/pull/152/head:refs/remotes/l0rinc/pr-152' 'refs/pull/195/head:refs/remotes/l0rinc/pr-195'
git log --reverse --format='%H %s' $(git merge-base l0rinc/pr-152 origin/master)..l0rinc/pr-152
git show 06fad3b1c4 967d2b0226 3ac40d0989 -- src/kernel/coinstats.cpp src/rpc/blockchain.cpp
git show f821dbe736 6ad167db5a 5df2bbf543 -- src/coins.h src/txdb.h src/kernel/coinstats.cpp src/rpc/blockchain.cpp
sed -n '2240,2273p' src/rpc/blockchain.cpp
```

Decision: remove both PRs from the open performance-seed list. The accepted
scan stack is the stronger implementation. Any future cursor-interface cleanup
belongs to a separate refactor/audit and must not be bundled with a performance
claim.

### Close fork PR #200/#203 mmap and cache-allocation seeds

Fork PR [#200](https://github.com/l0rinc/bitcoin/pull/200) is exactly
`e3ec270a39094e56cfaa581c00487d5a9b5a6966`, the upstream
`MADV_RANDOM` mmap-table-read annotation. It is not a new candidate: the
user's full height-957759 HDD benchmark established that its global advisory
caused the 1.79x reindex regression recorded above. The accepted
`df4669a112` follow-up retains the upstream random-read protection and adds a
strictly bounded sequential-read `MADV_WILLNEED` recovery. That controlled
change, not a duplicate of #200, improved the cold height-287000 reindex
median 1.55% and the established-SST random-read control 3.08%.

Fork PR [#203](https://github.com/l0rinc/bitcoin/pull/203), fetched as
`l0rinc/pr-203` at `b4ba9ee5969ac188012a051b3e8b195d0e7b09c3`, removes the
LevelDB block cache and leaves its default write-buffer size after a larger
cache-allocation/refactor series. The precise behavior was tested directly on
the requested HDD at height 900000 and `-dbcache=4000`: the relevant
`78fa6e39` control, `ba30dc7600` cache/write-buffer candidate, and following
cleanup were all within ordinary variation (means 22014.138 s, 21834.785 s,
and 21905.920 s for only two runs). The repository's independent cold
height-287000 pair likewise found a 0.107% median difference. The premise
does not yield a reproducible reindex gain.

The two PRs therefore cover already-applied mmap behavior plus a measured
neutral cache knob. Neither has an untried small descendant suitable for this
goal. Reviewed with:

```sh
git fetch -q l0rinc 'refs/pull/200/head:refs/remotes/l0rinc/pr-200' 'refs/pull/203/head:refs/remotes/l0rinc/pr-203'
git show e3ec270a39 -- src/leveldb/util/env_posix.cc
git log --reverse --format='%H %s' $(git merge-base l0rinc/pr-203 origin/master)..l0rinc/pr-203
```

Decision: remove both from the open list. Do not retune mmap advice, block
cache, or write buffer size without a new profile showing a different access
pattern and enough repeated end-to-end HDD evidence to overcome the existing
negative controls.

### Close fork PR #180: no independent flush or hash-cache speed seed remains

Fork PR [#180](https://github.com/l0rinc/bitcoin/pull/180), fetched as
`l0rinc/pr-180` at `a93d6789b6198243cf16f0d26b11c2e263a3437b`, was a
collection of unrelated experiments rather than a remaining flush proposal.
Its parallel `CoinsViewOverlay` input-fetch series is already merged in
`origin/master` as `ab2a379237 coins: fetch inputs in parallel` and related
commits, so is explicitly out of scope. Its `f5a15c37` LevelDB statistics
addition is diagnostic logging, not an optimization. Its `e483b98eb` seek
compaction policy is already live and was closed above. Its custom
SipHash-1-3 outpoint hasher is rejected above on the cache's collision-resistance
boundary, and its `CheckTransaction` duplicate/null change is rejected above
for insufficient HDD reach.

The only superficially simple remaining item, `32eba3b3d48`, removes
`noexcept` from `SaltedOutpointHasher::operator()`. That is not a universally
safe speed annotation. Current source documents the deliberate trade-off:
libstdc++ treats the noexcept fast hash as non-cached, recomputes it during
rehash, costs about 1.6% in that narrow operation, but saves one `size_t` per
node and about 9% cache memory, permitting a larger `-dbcache`. Reversing this
would make a platform-library implementation policy part of Core's cache
budget, complicate the pool allocator's portable node-size assumption, and
trade memory for an unmeasured microbenchmark. The live cache-reallocation
trace above reinforces why preserved cache capacity is a first-order resource
contract during IBD.

There is no PR #180 flush patch to test after separating these components;
the earlier flush discussion belonged to PR #59 and is now closed with a
current trace. Reviewed with:

```sh
git fetch -q l0rinc 'refs/pull/180/head:refs/remotes/l0rinc/pr-180'
git log --reverse --format='%H %s' $(git merge-base l0rinc/pr-180 origin/master)..l0rinc/pr-180
git log origin/master --oneline -- src/coins.h
sed -n '56,95p' src/util/hasher.h
```

Decision: remove #180 from the open seed list. Do not turn the intentional
`noexcept` memory trade-off into a claimed HDD optimization without a
cross-library cache-capacity analysis and a reproducible end-to-end win.

### Reject local total-amount accumulator split for un-hashed UTXO stats

Disassembly of the `CoinStatsHashType::NONE` specialization showed that the
common, non-overflowing `stats.total_amount` update reloads and stores the
`std::optional<CAmount>` member in the large stack-resident `CCoinsStats`
object for every UTXO. This is real per-coin work, but its overflow behavior is
part of the public stats contract: `total_amount` must become `nullopt` after
the first signed `CAmount` overflow and remain so for the rest of the scan.

A temporary, source-local prototype therefore used a `CAmount` accumulator and
a `bool total_amount_overflow` only in the un-hashed loop. Each coin still used
`CheckedAdd`; a failed addition set the boolean, later additions were skipped,
and the loop copied the value back into `stats.total_amount` or reset the
optional after traversal. This generated a register-held accumulator on the
common path while preserving the observable overflow transition. It was not
committed.

The first paired warm-cache full-chain trial looked marginally favorable:

| binary | five `gettxoutsetinfo none` runs (s) | median (s) |
| --- | --- | ---: |
| baseline before | 47.00, 46.97, 47.02, 47.09, 47.10 | 47.02 |
| temporary accumulator | 46.67, 46.72, 46.73, 46.75, 46.77 | 46.73 |

That apparent 0.62% change is not sufficient evidence. A fresh baseline daemon
started immediately after the candidate, pinned to the same CPU and with the
same network-disabled/wallet-disabled options, produced 46.59, 46.53, 46.58,
and 46.62 seconds (46.585 s median). It is already faster than the candidate
by about 0.31%. A fifth reverse-control request was interrupted during harness
shutdown and wrote an empty response, so it is explicitly excluded rather than
used as a favorable or unfavorable data point. All five baseline-before,
candidate, and the four valid reverse-control JSON replies have SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`.

The initial baseline process required a cold 2:28.62 scan after startup;
candidate and reverse-control warmups were 47.50 and 47.20 seconds. Measured
iterations themselves were tightly clustered, but the between-daemon drift is
larger than the proposed gain. The temporary binaries, replies, timers,
debug logs, and command artifacts are retained under
`/mnt/my_storage/bitcoin-perf-scratch/total-accumulator-baseline.X4Bhfo`,
`.../total-accumulator-candidate3.0800Z5`, and
`.../total-accumulator-baseline2.mQ0Lt9`.

Reviewed with:

```sh
ninja -C build bitcoind -j4
# one network-disabled, wallet-disabled daemon per binary, taskset -c 0
build/bin/bitcoin-cli -rpcclienttimeout=0 -datadir=/mnt/my_storage/BitcoinData \
  -rpccookiefile=<trial>/.cookie gettxoutsetinfo none
sha256sum <baseline>/run-*.json <candidate>/run-*.json <reverse>/run-{1,2,3,4}.json
```

Decision: reject the split accumulator. The optional field is a plausible
micro-optimization target, but preserving its overflow contract buys no
reproducible end-to-end `gettxoutsetinfo none` gain here. Do not add a second
amount state machine or broaden the change to hashed stats/dumptxoutset unless
a profile and an interleaved, repeated benchmark establish a materially larger
win.

### `coinstats`: inline the fixed bogo-size calculation

`GetBogoSize(uint64_t)` is a fixed `50 + script_pub_key_size` calculation. It
was nevertheless an out-of-line, stack-protected function and appeared once
per UTXO in both `ComputeUTXOStats` loops. The full un-hashed stats disassembly
showed a call to it after every decoded `CoinStatsValue`, including the
function's stack-canary load/check and return. The same helper was also used
for every added and spent output while maintaining the coinstats index.

The size-taking overload is now `constexpr` in `kernel/coinstats.h`; direct
call sites pass `scriptPubKey.size()`, so the compiler emits the exact `+50`
calculation at each use. This removes the now-unneeded CScript wrapper and its
out-of-line definition. The metric is unchanged: it remains txid 32, output
index 4, height/coinbase 4, amount 8, script-length 2, and the exact script
byte count. The arithmetic has the same `uint64_t` promotion and wrap behavior
as the former implementation.

The measurement used the permitted local `BitcoinData` chainstate at height
957779 (166,350,731 UTXOs), release binaries, CPU 3 pinning, network and
wallet disabled, a fresh daemon for each series, one unmeasured warm scan, and
daemon-attached `perf stat` for every `gettxoutsetinfo none` request. Every
valid response had SHA-256
`4b96d583ae030f7391f6b960ee9c738360da5ea27532da61c7057cc206dccfa0`.

| binary / valid runs | wall median | task-clock median | instructions median | branches median |
| --- | ---: | ---: | ---: | ---: |
| out-of-line base (5) | 47.13 s | 47.006 s | 433.890 B | 87.782 B |
| inline candidate, first series (5) | 47.16 s | 47.044 s | 432.398 B | 87.647 B |
| inline candidate, reverse control (3) | 46.79 s | 46.710 s | 431.817 B | 87.400 B |

Daemon-to-daemon wall-clock and frequency drift is larger than this small
change, so the stable evidence is retired work rather than a precise wall-time
claim. The first matched five-run pair consistently removes 1.492 B
instructions per scan (0.344%) and 135 M branches (0.154%), with each series'
instruction range below 0.03%. The reverse candidate has the same direction
and a lower task-clock, confirming that the reduced call sequence is not a
response or counter artifact. Candidate and baseline binaries, JSON replies,
timers, perf CSVs, debug logs, and cookies are retained under
`/mnt/my_storage/bitcoin-perf-scratch/bogosize-inline-{candidate.dTLKg1,baseline.nDjNp7,candidate2.TEwAqn}`.

Validated with:

```sh
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=coins_tests/coin_stats_value_matches_coin_deserialization --log_level=message
build/bin/test_bitcoin --run_test=coinstatsindex_tests --log_level=message
build/test/functional/test_runner.py rpc_blockchain.py feature_coinstatsindex.py --jobs=1 \
  --tmpdirprefix=/mnt/my_storage/bitcoin-perf-scratch/functional-bogosize-inline
git diff --check
```

Expected reach is every direct `ComputeUTXOStats` scan, including all
non-indexed `gettxoutsetinfo` hash types and the preparation phase of
`dumptxoutset`, plus coinstats-index block connect/disconnect maintenance. It
does not change `scantxoutset`, chainstate reindex, snapshot output writes, or
LevelDB I/O; cold HDD wall gains can be proportionally smaller.

### Reject fused dirty/fresh coins-cache flag setting

The unmerged `l0rinc/coins-cache-invariants` history contains
`000efb0bdf928195ddc168be80741c55708b047a` (`coins: pass freshness through
SetDirty() in production`). It is not reachable from freshly fetched
`origin/master`, so it was a legitimate non-upstream seed. The patch observes
that normal production insertion already knows whether a cache entry is fresh:
`AddCoin()` calls `SetDirty()` and then, for the common new-output case,
`SetFresh()`. The inserted-parent branch in `CCoinsViewCache::BatchWrite()`
has the same two-call sequence.

A temporary direct-Core prototype changed `SetDirty()` to accept `fresh` and
called `AddFlags(DIRTY | (fresh ? FRESH : 0), ...)` once at those two sites.
For an entry with no flags, the original first call links it and sets `DIRTY`;
the second sees the existing link and ORs `FRESH`. The prototype performs the
same link and the same combined bit update once. It deliberately retained
`SetFresh()` for test-only artificial states, and did not adopt the later,
broader fork refactor that derives dirtiness from linked-list membership.

The compiler effect is real but small: on the GCC Release binaries,
`CCoinsViewCache::AddCoin()` shrank from 967 to 919 bytes (48 bytes, 4.96%).
Its fresh-output path no longer performs the second `AddFlags()` test, linked
list check, and flag store. The changed parent-flush function instead grew
from 2783 to 2822 bytes, so this does not establish a uniformly reduced code
path. Existing cache coverage passed under the temporary source change:

```sh
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=coins_tests_base --log_level=message
build/bin/test_bitcoin --run_test=coins_tests_dbbase --log_level=message
build/bin/test_bitcoin --run_test=coinscachepair_tests --log_level=message
```

The macro check used two separately built binaries and the same cold OverlayFS
reindex harness as the other HDD controls: the supplied `BitcoinData` was the
read-only lower layer; a new upper/work/merged directory was used for every
run; page cache was dropped; the target was height 287000 with `-dbcache=450`,
wallet and networking disabled. Both logs reached heights 0 and 287000 and
`Shutdown done`; their output block hash, transaction count, and final cache
statistics match. Raw data is retained under
`/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/{freshflags-base-1,freshflags-candidate-1}/metrics/`.

| version | perf wall | task-clock | instructions | branches | major faults | output KiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original two calls | 574.925 s | 512.057 s | 2.496001 T | 279.701 B | 896 | 9,411,928 |
| fused temporary prototype | 571.533 s | 510.934 s | 2.496458 T | 279.540 B | 894 | 9,432,744 |

The nominal 0.59% wall and 0.22% task-clock decrease is contradicted by
457.1 M additional retired instructions (0.018%) and 0.59 more user CPU
seconds. It is also accompanied by 1.75% more filesystem output and a 1.46%
larger OverlayFS upper directory (739,811,571 versus 729,154,392 bytes).
One cold pair cannot attribute that I/O/frequency variation to a few removed
flag instructions, and its instruction direction is wrong. The branch count
does fall by 161.5 M (0.058%), as expected, but that is not enough to prove a
meaningful end-to-end reindex improvement.

Decision: reject and fully revert the prototype. The exact state transition is
valid and may be useful as a cleanup seed, but it has no reproducible CPU or
HDD benefit at this workload. Do not add a cache-persistence change for this
tiny code-size reduction without interleaved controls that show a clear
instruction decrease and an end-to-end win outside storage noise.

### Close upstream UTXO-RPC and reindex flush seeds

Two fork commits initially looked like direct improvements for the named
workloads, but both are already in `origin/master` and therefore must not be
duplicated on this branch.

* `226b6c7d6e69c1f032079c8bc5af5710d8e54efb` (`validation: do not wipe utxo
  cache for stats/scans/snapshots`) is merged upstream as
  `4e4fa0199e` / `c6ca2b85a3`. It split forced persistence into non-wiping
  `FORCE_SYNC` and wiping `FORCE_FLUSH`, and changes `gettxoutsetinfo`,
  `scantxoutset`, and `dumptxoutset` snapshot preparation to call
  `ForceFlushStateToDisk(/*wipe_cache=*/false)`. That is exactly the desired
  behavior: dirty cache entries are safely written while useful clean entries
  remain available. No branch commit is needed.
* `7fc8d7f9c1f0fe3795b397327e38465ee6f76b83` (`validation: periodically
  flush dbcache during reindex-chainstate`) is merged upstream as
  `84820561dc`. It moves the existing periodic flush inside the
  `ActivateBestChain()` outer loop, so a long reindex persists progress rather
  than accumulating the entire activation. It is an availability/recovery
  property, not an unmeasured replacement for the current cache or LevelDB
  work.

The live tree contains both exact mechanisms. Verified with:

```sh
git log origin/master --oneline -- src/validation.cpp src/rpc/blockchain.cpp
git show c6ca2b85a3 -- src/validation.cpp src/validation.h src/rpc/blockchain.cpp
git show 84820561dc -- src/validation.cpp
rg -n 'ForceFlushStateToDisk\\(/\\*wipe_cache=\\*/false\\)|Write changes periodically to disk' \
  src/rpc/blockchain.cpp src/validation.cpp
```

Decision: close both historical seeds. Their reach is already present in the
base code; applying either again would be duplicate history, not a measurable
new speedup.

### Reject non-portable cache-map and tombstone-list rewrites

The high reindex samples in `CCoinsMap` make an alternative map attractive,
but the remaining fork variants are not simple, behavior-preserving Core
changes.

`1630e368d190a75870399cc38af3c6023faaf2d9` replaces the standard map with
`boost::unordered_node_map`. It requires Boost 1.82, while the live CMake
configuration requires Boost 1.74. The change also needs a new
implementation-specific dynamic-memory formula and a new pool-node slack
assumption. That makes cache capacity and flush thresholds depend on a new
third-party container layout across all supported platforms. No profile or
end-to-end result shows that the dependency-floor and cache-accounting change
would improve this HDD workload. It is not an eligible minimal optimization.

The two-commit `l0rinc/l0rinc/singly-linked-coins-cache` branch has a different
tradeoff. `7ccc01bde4` retains spent FRESH entries as tombstones, adds a
`m_spent_fresh_count`, modifies `AddCoin`, `SpendCoin`, `BatchWrite`, `Sync`,
`Reset`, uncaching, and cache-state accounting, then scans the flagged list to
compact tombstones at a new memory-pressure boundary. Only after that larger
state-machine change does `1645927008` remove one linked-list pointer per
entry. The supposed pointer saving is therefore coupled to delayed deletion,
additional branches/counting on normal spends, a new compaction traversal, and
changed cache persistence invariants. The branch itself ends at that prototype;
it does not contain an HDD benchmark or a full recovery/fuzz validation stack.

Reviewed with:

```sh
git show 1630e368d1 -- cmake/module/AddBoostIfNeeded.cmake src/coins.h src/memusage.h
sed -n '20,42p' cmake/module/AddBoostIfNeeded.cmake
git log --reverse --oneline \
  $(git merge-base l0rinc/l0rinc/singly-linked-coins-cache origin/master)..l0rinc/l0rinc/singly-linked-coins-cache
git show 7ccc01bde4 1645927008 -- src/coins.cpp src/coins.h src/validation.cpp src/test
```

Decision: reject both without a source prototype. The Boost change is a
dependency and portability project, while the singly linked variant changes
the coins-cache state machine and can retain more entries during the exact
memory-constrained HDD replay it aims to speed up. A future map investigation
must first demonstrate a materially larger map-lookup share with a
container-independent capacity/recovery proof; neither variant justifies
touching LevelDB or RocksDB.

### Reject per-entry dirty-count updates during wiping cache flushes

`CoinsViewCacheCursor::NextAndMaybeErase()` decrements `m_dirty_count` for
every flagged entry even when `m_will_erase` is true. In that mode the caller
will immediately clear the whole map, and `CCoinsViewDB::BatchWrite()` already
reads the initial dirty count before iteration for logging and disk-space
checks. A temporary Core-only prototype therefore kept the count unchanged
while a wiping cursor iterated, set it to zero only after the final entry, and
returned early before the per-entry `IsDirty()`/`TrySub()` work. The
non-wiping `Sync()` path retained the exact original decrement, spent-entry
erase, and flag-cleanup behavior.

The prototype preserves the current wipe behavior: a completed normal
`BatchWrite()` reaches the final linked-list entry, then `Flush()` clears the
map and observes a zero count. If a backend stops iteration early, the count
remains nonzero and the existing post-`BatchWrite()` assertion fails instead
of silently accepting an incomplete flush. Artificial FRESH-only test entries
remain harmless in wiping cursors because the map is discarded, as before.

The focused cache suites passed with separately saved original and candidate
release binaries:

```sh
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=coins_tests_base --log_level=message
build/bin/test_bitcoin --run_test=coins_tests_dbbase --log_level=message
build/bin/test_bitcoin --run_test=coinscachepair_tests --log_level=message
git diff --check
```

The candidate binary is
`/mnt/my_storage/bitcoin-perf-scratch/cursor-dirty-count/bitcoind-candidate`
and the original is `.../bitcoind-baseline`. `nm -S -C` shows that the
inlined wiping path shrank `CCoinsViewDB::BatchWrite()` from 9438 to 9351
bytes (87 bytes, 0.92%), but grew `CCoinsViewCache::BatchWrite()` from 2783 to
2831 bytes (48 bytes, 1.72%). That mixed code-size result does not establish a
uniform instruction reduction.

More importantly, this has too little end-to-end reach for a further HDD run.
The controlled height-287000, `-dbcache=450` log has only five normal
full-cache drops after genesis, each with 3,155,908 to 3,187,541 entries: at
most 15,910,519 per-entry updates. The same run retires 2.496 trillion
instructions. Even an unrealistically generous ten saved instructions per
entry would bound the effect below 0.007% of that execution, while the prior
cold controls vary by several tenths of a percent. The full local-chain
telemetry has similarly only six large drops at 2 GiB cache capacity, not a
per-block flush.

The flush counts were extracted from the saved `UpdateTip` telemetry with:

```sh
rg 'UpdateTip: new best=.*cache=' \
  /mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/freshflags-base-1/metrics/debug.log
nm -S -C /mnt/my_storage/bitcoin-perf-scratch/cursor-dirty-count/bitcoind-{baseline,candidate}
```

Decision: fully revert the prototype without an HDD timing claim. It is a
valid local simplification, but cannot produce a measurable reindex speedup at
the observed flush frequency; the expanded parent-cache code also makes the
direction less clear. Do not replace an exact dirty-entry accounting invariant
with a special wipe-mode path unless a workload has substantially more flush
events and a profiling result establishes that this cursor bookkeeping is
material.

### Reject buffered `WriteVarInt()` fast paths after a matched HDD reindex

The local, non-upstream historical commit
`2d4518f1ab78aef48d8fe92e5aaabf31273dd646` (`serialize: specialize VarInt
hot paths`) suggested a direct write-side seed for `CCoinsViewDB::BatchWrite`.
The current `WriteVarInt()` emits every encoded byte through
`ser_writedata8()`. A temporary, fully reverted prototype retained the exact
encoding but wrote the common two- and three-byte encodings through one stack
buffer and one `Stream::write()` call; longer encodings were similarly
assembled into a bounded stack buffer. It retained the one-byte fast path.

The isolated result looked promising. A scratch `DataStream` benchmark created
10,000,000 streams, reserved 256 bytes each, and wrote 100 VARINTs per stream
using the mix observed by the historical experiment: 54 one-byte values (42),
14 two-byte values (300), 28 three-byte values (70,000), and four four-byte
values (3,000,000). Seven CPU-pinned release samples in seconds were:

| version | samples | median |
| --- | --- | ---: |
| current byte-at-a-time code | 3.57092, 3.57093, 3.57218, 3.50071, 3.57323, 3.56848, 3.56997 | 3.57092 |
| temporary buffered writes | 3.02166, 3.02815, 3.02594, 3.02269, 3.02877, 3.02851, 3.02746 | 3.02594 |

That is a 15.26% microbenchmark improvement. The temporary source change
also preserved serialization tests and made the inlined
`CCoinsViewDB::BatchWrite()` symbol smaller (9,438 to 9,051 bytes):

```sh
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=serialize_tests --log_level=message
git diff --check
nm -S -C /mnt/my_storage/bitcoin-perf-scratch/varint-fastpath/bitcoind-{baseline,candidate}
```

The macro result rejects it. Separately built baseline and candidate binaries
were run on the supplied local `BitcoinData` HDD chainstate with the Linux
page cache dropped before each run, `-dbcache=4000`, no wallet or networking,
and the same process metrics. The initial baseline command named
`-stopatheight=700000`, but it was cleanly stopped through `bitcoin-cli stop`
at height 382619 immediately after the first 29,153,566-entry full cache
flush; the candidate used `-stopatheight=382619` and reached exactly the same
block hash, transaction count (91,594,661), first flush size, and final flush
size (14,219,291 entries). Both exited successfully. This makes the measured
range, rather than the abandoned height-700000 suffix, the comparison unit.

```sh
sync
echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null
perf stat -e task-clock,cycles,instructions,major-faults,minor-faults \
  /usr/bin/time -v <baseline-or-candidate-bitcoind> \
    -datadir=/mnt/my_storage/BitcoinData -stopatheight=382619 \
    -dbcache=4000 -reindex-chainstate -blocksonly -disablewallet \
    -connect=0 -listen=0 -dnsseed=0 -printtoconsole=0
```

| version | wall | task-clock | user | system | instructions | major faults | filesystem output |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| current byte-at-a-time code | 1444.120 s | 1253.123 s | 1153.726 s | 106.147 s | 5.941447 T | 897 | 22,191,248 KiB |
| temporary buffered writes | 1450.904 s | 1254.519 s | 1155.379 s | 105.941 s | 5.985218 T | 887 | 23,891,760 KiB |

The candidate is 6.783 seconds (0.47%) slower, retires 43.738 billion more
instructions (0.74%), and writes 7.66% more filesystem data. The matched
first flush took 295 seconds in the baseline (11:00:08--11:05:03 UTC) and 294
seconds in the candidate (11:24:58--11:29:52 UTC), while the final flush plus
shutdown took 22 versus 33 seconds. Neither local timing difference overturns
the whole-workload regression, and a single pair is already sufficient to
reject a change whose sole justification is speed.

Raw benchmark source, binaries, commands, logs, `perf stat`, and `/usr/bin/time`
outputs are retained under
`/mnt/my_storage/bitcoin-perf-scratch/varint-fastpath/`. The temporary source
diff was reverted before this commit; the tracked tree retains no serialization
change.

Decision: do not commit the generic buffered-VarInt change. Its small,
allocation-free microbenchmark win does not transfer to the write-heavy
chainstate workload, and it would broaden a serialization primitive used well
beyond the named UTXO paths. Revisit only with a profile showing VARINT byte
writes as a material end-to-end bottleneck and interleaved controls that show
a reproducible whole-workload win.

### Reject txid-only cursor progression for un-hashed UTXO statistics

The un-hashed `ComputeUTXOStats()` loop, used by `gettxoutsetinfo none`, needs
the current database key's txid to count transaction boundaries, but not its
output index. `CCoinsViewDBCursor::Next()` eagerly deserializes both members
of the `COutPoint`. A temporary direct-Core candidate added
`GetKeyTxid()`/`NextTxid()` cursor hooks. The coins-DB override cached the
32-byte txid rather than materializing the `uint32_t` output index; it still
parsed and range-validated the output-index VARINT into a local, so malformed
keys retained the existing failure behavior. Existing cursors retained a
correct full-key default.

A normal daemon could not provide a stable RPC benchmark after the earlier
height-382619 reindex: RPC remained in warm-up while background activation
rapidly advanced the chainstate. It was sent a graceful `SIGTERM` instead of
allowing uncontrolled progress and shut down cleanly at height 385302. That
attempt is not performance evidence; its dedicated log is
`/mnt/my_storage/bitcoin-perf-scratch/key-hash-stats-baseline/bitcoind.log`.

The focused measurement instead used a scratch-only, separately linked
program opening the closed `BitcoinData/chainstate` through `CCoinsViewDB`.
It performs the same key/value, txid-boundary, amount, bogo-size, and cursor
advance operations as the `NONE` stats loop, but deliberately excludes RPC,
`cs_main`, block-index lookup, and final disk-size accounting. Both versions
returned exactly the same aggregate tuple for 32,659,375 UTXOs at the
post-shutdown chainstate: 7,659,292 transaction ids, total amount
1,488,241,113,523,993 satoshis, and bogo size 2,480,426,961. The database was
warm, each run was pinned to CPU 3, and all had zero major faults.

Five newly collected alternating baseline/candidate pairs give:

| version | wall samples | median wall | median task-clock | median instructions | median branches |
| --- | --- | ---: | ---: | ---: | ---: |
| full `COutPoint` cursor key | 7.15577, 7.17739, 7.19080, 7.14002, 7.19949 s | 7.17739 s | 7.230831 s | 68.151431 B | 13.783771 B |
| temporary txid-only key | 7.11395, 7.20070, 7.21824, 7.10533, 7.20625 s | 7.20070 s | 7.254107 s | 67.886764 B | 13.815732 B |

The candidate consistently removes 264.667 million instructions per scan
(0.388%), but increases median branches by 31.961 million (0.232%) and has a
0.322% task-clock regression. Pairwise wall changes alternate in direction
(-0.58%, +0.32%, +0.38%, -0.48%, +0.09%); their variation is larger than the
possible benefit. The candidate adds two virtual cursor extension points, a
second 32-byte cache, and extra cache-state transitions to obtain this
non-transferable instruction saving.

The temporary source and test change built, and the cursor ordering test
requested txids before full keys so it exercised both the txid-only cache and
later full-key materialization across indices 0, 1, 127, 128, 255, 256, and
`uint32_t` max:

```sh
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=coins_tests_dbbase/coins_db_cursor_order --log_level=message
build/bin/test_bitcoin --run_test=coins_tests/coin_stats_value_matches_coin_deserialization --log_level=message
git diff --check
```

Scratch source, the baseline/candidate binaries, and the direct closed-DB
reader are under
`/mnt/my_storage/bitcoin-perf-scratch/key-hash-stats-baseline/`. The
temporary tracked source change was fully reverted before this commit.

Decision: reject. This affects only `CoinStatsHashType::NONE` and therefore
only the no-hash `gettxoutsetinfo` path among the target workloads; hashed
statistics, `scantxoutset`, `dumptxoutset`, and reindex-chainstate retain the
full-key path. A stable instruction decrease alone is not enough to widen the
cursor interface when the actual timed loop has no reproducible speedup.

### Screened historical cached-SipHash seed: already present upstream

Historical local commit `a8df73ee020c` (`optimization: refactor: Introduce
Uint256ExtraSipHasher to cache SipHash constant state`) measured 2.3--6.4%
throughput improvements in `SaltedOutpointHasher` microbenchmarks. Its idea
was narrow and sound: construct the keyed SipHash initial state once, then
copy that state for each `uint256` plus output-index hash instead of repeating
the four key-dependent xor operations for every map lookup. That is directly
relevant to the `CCoinsViewCache::cacheCoins` map, which is active in
validation and reindex-chainstate.

It is not an available follow-up. Current master already contains the same
mechanism under the clearer `PresaltedSipHasher` name. Commit `ec11b9fede`
introduced it and `9ca52a4cbe` migrated `SipHashUint256` users. The live
implementation stores immutable `SipHashState` in `PresaltedSipHasher`, and
its `(uint256, uint32_t)` overload starts each hash from that saved state.
`SaltedOutpointHasher` holds a `const PresaltedSipHasher`, so both normal and
deterministic coins-cache maps use the cached state rather than reconstructing
it for every `COutPoint` hash.

This was verified without relying on name similarity:

```sh
git log origin/master --format='%h %s' -S'PresaltedSipHasher' \
  -- src/crypto/siphash.h src/crypto/siphash.cpp src/util/hasher.h src/util/hasher.cpp
git blame -L 46,70 -- src/crypto/siphash.h
rg -n -C 3 'PresaltedSipHasher|SaltedOutpointHasher' \
  src/crypto/siphash.h src/crypto/siphash.cpp src/util/hasher.h \
  src/util/hasher.cpp src/coins.h src/coins.cpp
```

The commands identify the two upstream commits, the cached-state declaration,
the specialized 32-byte-plus-index implementation, and its construction in
the active coins-cache hasher. The reindex profile's 6.79% coarse sample share
in `PresaltedSipHasher::operator(uint256, uint32)` is therefore not evidence
that this historical fix is missing. The separate non-standard SipHash-1-3
proposal remains rejected on collision-resistance grounds; replacing the
already-cached standard SipHash-2-4 path would be a materially different,
security-sensitive change.

Decision: no code change. This historical speedup is already on master, so it
is excluded under the rule to ignore fixes pushed to `bitcoin/bitcoin`.

### Reject libstdc++ `CCoinsMap` hash-node caching

The current reindex profile still has a coarse `std::_Hashtable<COutPoint,...>`
signal, so the existing `SaltedOutpointHasher::operator() noexcept` policy was
rechecked rather than assuming that less hash recomputation must be better.
On libstdc++, a fast, non-throwing hasher selects an unordered-map node without
a cached hash code. Removing only `noexcept` makes libstdc++ retain the hash
in every node, avoiding some stored-key SipHash calls when probing or
rehashing. It would affect the active `CCoinsViewCache::cacheCoins` map and
thus validation and reindex-chainstate, but also every other
`SaltedOutpointHasher` map in the mempool, orphanage, policy, and wallet.
It does not accelerate the cursor-based `gettxoutsetinfo`, `scantxoutset`, or
`dumptxoutset` scan loops.

This is an intentional current tradeoff, not an omission. The live hasher
comment records that the no-cache choice saves `sizeof(size_t)` per node so a
fixed `-dbcache` can retain more coins. A scratch compile against the actual
GCC 14.2 libstdc++ verified the mechanism rather than relying on that comment:
`std::__cache_default<COutPoint, SaltedOutpointHasher>::value` is `0` with the
live `noexcept` declaration and becomes `1` after removing it. The matching
internal hash-node sizes are 128 and 136 bytes respectively. The 6.25% node
increase is in the adverse direction for an HDD reindex bounded by a configured
coins-cache budget; it leads to fewer cached coins and potentially more flush
work. The production `PoolAllocator` deliberately has enough headroom for
either node shape, so this is a performance/capacity tradeoff rather than an
allocator correctness problem.

A temporary one-line candidate removed `noexcept`; it built successfully.
Five CPU-3-pinned GCC Release `CCoinsCaching` runs, each with
`-min-time=5000`, measured the existing cache access path:

| version | all ns/op samples | five-run median | result |
| --- | --- | ---: | --- |
| current uncached nodes | 573.24, 572.63, 664.27, 651.11, 572.21 | 573.24 | baseline |
| temporary cached nodes | 977.14, 1015.66, 587.19, 586.87, 585.21 | 587.19 | 2.43% slower |

The benchmark has machine-frequency outliers in both variants, but even its
five-run medians are wrong-direction. Its three close baseline samples have a
572.63 ns median, versus 586.87 ns for the three close candidate samples
(2.49% slower). That direct cache-path regression, plus the per-node memory
increase and the already documented fixed-cache advantage, is enough to reject
the idea before spending another multi-hour HDD reindex pair on it.

The temporary source edit was reverted and the baseline build and relevant
tests passed:

```sh
ninja -C build bench_bitcoin test_bitcoin -j4
build/bin/test_bitcoin --run_test=coins_tests,coinscachepair_tests --log_level=message
git diff --check
```

The trait probe and its output are retained under
`/mnt/my_storage/bitcoin-perf-scratch/coins-hash-cache/`; the compiled probe
reported `0 128 136` before the temporary edit and `1 128 136` with it.

Decision: retain `noexcept`. Cached hashes cannot be reused safely through the
public mutable `COutPoint` itself (covered by the earlier rejection), and the
container-owned alternative is both locally slower and reduces the capacity
that makes the cache effective on the target HDD workload.

### LevelDB: avoid `std::string::assign()` in forward DB iteration

A full Callgrind run of the closed local chainstate was used because the
machine's global perf sample cap is one sample per second. The scratch reader
executes the same per-coin key/value, txid-boundary, amount, bogo-size, and
forward-cursor operations as the un-hashed `ComputeUTXOStats()` loop, while
deliberately excluding RPC dispatch, `cs_main`, block-index lookup, and final
disk-size accounting. It processed 32,659,375 UTXOs and returned 7,659,292
transaction ids, 1,488,241,113,523,993 satoshis, and bogo size 2,480,426,961.

The instrumented run retired 67.36 billion instructions. Cursor advancement
accounts for 73.07% inclusive work: LevelDB `DBIter::Next()` saves the current
user key in `saved_key_` before advancing, so it can suppress old versions or
deletions of that key. The resulting `std::string::assign()` call is executed
once for every forward entry. `MergingIterator`, key comparison, and value
deserialization remain the larger surrounding costs, but the repeated
same-sized string assignment is a narrow, behavior-preserving direct target.

`SaveKey()` only receives a slice owned by its underlying iterator, never a
slice into its destination string. Replacing `assign(k.data(), k.size())` with
`clear(); append(k.data(), k.size())` therefore preserves the copied bytes and
replacement semantics for normal values, deletions, forward/reverse direction
changes, and empty keys. `clear()` preserves the reusable allocation; `append`
uses the known-disjoint source without `assign()`'s replacement/overlap path.
A scratch microbenchmark that clears the same retained 37-byte string 32,659,375
times measured medians of 0.118592 seconds for `assign()` and 0.109490 seconds
for `append()` (7.68% faster), but the real scan below is the acceptance proof.

Six alternating, warm, CPU-3-pinned Release scans of the closed chainstate all
returned the exact aggregate tuple above:

| version | wall samples | median wall | change |
| --- | --- | ---: | ---: |
| baseline `assign()` | 7.12722, 7.10781, 7.17174, 7.13544, 7.14286, 7.13335 s | 7.134395 s | baseline |
| `clear()` + `append()` | 7.03359, 7.03578, 7.03851, 7.05839, 7.05229, 7.03142 s | 7.037145 s | 1.36% faster |

Three additional alternating `perf stat` pairs confirm the causal CPU change:

| metric | baseline median | candidate median | change |
| --- | ---: | ---: | ---: |
| task-clock | 7.18570 s | 7.09297 s | -1.29% |
| instructions | 68.144589 B | 67.242723 B | -1.32% |
| branches | 13.782547 B | 13.551479 B | -1.68% |
| cache misses | 38.871008 M | 38.928748 M | +0.15% |

The changed routine is a generic LevelDB forward-iteration primitive. Its
demonstrated reach is full forward scans through chainstate, including
`gettxoutsetinfo none`, `scantxoutset`, and the chainstate-copy phase of
`dumptxoutset`; it may help other Core LevelDB scans. It is not claimed as an
HDD reindex-chainstate speedup: that workload is dominated by validation,
coins-cache, and point-lookup/write paths rather than a full forward DB scan.

Validation used the existing LevelDB iterator and coin-cursor tests:

```sh
ninja -C build libleveldb.a bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=leveldb_tests --log_level=message
build/bin/test_bitcoin --run_test=coins_tests_dbbase/coins_db_cursor_order --log_level=message
build/bin/test_bitcoin --run_test=coins_tests/coin_stats_value_matches_coin_deserialization --log_level=message
git diff --check
```

The Callgrind profile, reader sources/binaries, string microbenchmark, and
raw commands remain under
`/mnt/my_storage/bitcoin-perf-scratch/{key-hash-stats-baseline,dbiter-savekey}/`.
RocksDB was consulted only after this profile established a LevelDB iterator
bottleneck; its current merging iterator also uses a min-heap with a
replace-top operation, supporting the already accepted heap structure but not
justifying a RocksDB port or option change for this smaller string-copy fix.

### Reject an `ObfuscatedSpanReader` byte-read serialization specialization

The same forward cursor profile has a visible `ReadVarInt`/coin-value decoding
component. The current `ObfuscatedSpanReader::read()` already has a special
case for a one-byte destination: it directly obfuscates the byte and advances
the span, avoiding the word-at-a-time XOR setup used for longer reads. The
generic `ser_readdata8()` helper still makes a one-byte `Span` and dispatches
through `read()`, so a temporary candidate added a `read_byte()` member and an
ADL `dbwrapper_private::ser_readdata8(ObfuscatedSpanReader&)` overload. It
would have removed only that short-span construction and dispatch for the
VARINT byte reads in obfuscated chainstate values.

The candidate kept the existing bounds/error contract: it first checked that
the input span was non-empty, then used `ObfuscateByte(m_data.front(),
m_key_offset++)`, advanced exactly one byte, and otherwise threw the same
`std::ios_base::failure` text as `read()`. A static Release reader over the
closed local chainstate built successfully and produced the identical full
scan aggregate on every run:

```
32659375 7659292 1488241113523993 2480426961
```

Five warm alternating CPU-3-pinned pairs compared the already accepted
`DBIter::SaveKey()` `clear()+append()` version with this byte-read candidate:

| version | wall samples | median wall | change |
| --- | --- | ---: | ---: |
| existing one-byte `read()` path | 7.04221, 7.18058, 7.02681, 7.00651, 7.04197 s | 7.04197 s | baseline |
| temporary ADL byte specialization | 7.06690, 7.03072, 7.05166, 7.04814, 7.03110 s | 7.04814 s | 0.09% slower |

The single large baseline result does not change the conclusion: the
five-sample median is already wrong-direction, and the candidate's closer
four samples do not establish a meaningful win. Extending the public reader
surface and adding a serialization ADL overload would make the low-level
contract harder to follow, while the live reader already contains the
important one-byte XOR fast path. The temporary edit was fully reverted.

The baseline rebuild and focused tests passed after that restoration:

```sh
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=message
build/bin/test_bitcoin --run_test=coins_tests/coin_stats_value_matches_coin_deserialization --log_level=message
git diff --check
```

The reader, static binaries, and raw timing commands are retained under
`/mnt/my_storage/bitcoin-perf-scratch/dbiter-savekey/` as
`cursor_stats_bench_candidate` and `cursor_stats_bench_byte_candidate`.

Decision: retain the existing `read()` one-byte fast path and do not add the
specialization. This candidate can reach every obfuscated chainstate cursor
value, including the UTXO RPC scans, but it has no demonstrated benefit and
is not an HDD reindex-chainstate optimization.

### Reject bytewise `DBIter` comparator dispatch specialization

After the forward-scan profile and the `SaveKey()` allocation improvement, the
next small LevelDB target was `DBIter::FindNextUserEntry()`. On every normal
forward user-key transition it calls the configured `Comparator` virtually to
decide whether the candidate is hidden by the preceding key. Core's
`CDBWrapper::GetOptions()` starts from `leveldb::Options`, whose default is
the built-in `BytewiseComparator`, and does not replace the comparator.

A temporary generic LevelDB candidate recorded at `DBIter` construction
whether `cmp == BytewiseComparator()`. Its comparison helper called inline
`Slice::compare()` on that path and retained the original virtual
`Comparator::Compare()` call for every non-bytewise database. It changed no
on-disk key ordering and did not assume that third-party LevelDB users have
the Core default comparator. A temporary unit test constructed internal keys
in reverse-comparator order and confirmed that the fallback yields `b:b,a:a`.
Forcing the candidate to treat that comparator as bytewise made the test fail
after `b:b`, proving that the test catches the comparator-contract violation.

The candidate built and every direct reader run returned the unchanged closed
chainstate aggregate:

```
32659375 7659292 1488241113523993 2480426961
```

Ten warm alternating, CPU-3-pinned scans compared the committed
`clear()+append()` `SaveKey()` baseline with the dispatch candidate:

| version | wall samples | median wall |
| --- | --- | ---: |
| current virtual comparator | 7.02371, 7.00939, 7.00936, 7.04597, 7.05488, 7.06018, 7.04080, 7.03671, 7.05490, 7.05314 s | 7.043385 s |
| temporary bytewise dispatch | 7.02355, 7.03485, 6.99840, 7.00999, 7.06842, 6.99501, 7.03057, 7.10994, 7.03569, 7.02169 s | 7.027060 s |

The unpaired medians imply 0.23% faster elapsed time, but the more relevant
paired-median saving is only 0.010595 seconds (0.15%), while individual pairs
range from a 1.04% regression to a 0.93% improvement. That is not enough
elapsed-time evidence for a cross-project LevelDB control-flow change.

Three alternating `perf stat` pairs did establish the intended CPU mechanism:

| metric | baseline median | candidate median | change |
| --- | ---: | ---: | ---: |
| instructions | 67.245310 B | 66.584504 B | -0.98% |
| branches | 13.551919 B | 13.452173 B | -0.74% |
| task-clock | 7.11135 s | 7.11212 s | +0.01% |

The candidate's instruction reduction is stable, but the second task-clock
sample was a 7.46327-second frequency/scheduling outlier. The median
task-clock does not improve, so neither the instruction counter nor the
unpaired wall median is sufficient to claim a user-visible speedup.

The temporary source, CMake include path, and generic-comparator test were
fully reverted. The restored build and focused baseline tests passed:

```sh
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=leveldb_tests --log_level=message
build/bin/test_bitcoin --run_test=coins_tests_dbbase/coins_db_cursor_order --log_level=message
build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=message
git diff --check
```

The candidate reader and the direct-scan source are retained under
`/mnt/my_storage/bitcoin-perf-scratch/dbiter-savekey/` as
`cursor_stats_bench_bytewise_candidate` and `cursor_stats_bench.cpp`.

Decision: do not retain the specialization. Its potential reach is generic
bytewise LevelDB forward and reverse iterators, including the UTXO RPC scans,
but it has no reproducible elapsed-time benefit on the measured chainstate and
does not justify the added state, conditional comparison path, private-header
test plumbing, or broader LevelDB maintenance surface. It is not an HDD
reindex-chainstate optimization.

### LevelDB: directly compare bytewise internal user keys

The symbolized follow-up Callgrind profile used the same closed local
chainstate reader as the `DBIter::SaveKey()` investigation. It processed
32,659,375 UTXOs and retained 66.46 billion instructions. The source-attributed
LevelDB trace showed 53,348,030 calls to
`InternalKeyComparator::Compare()` from the forward merge heap's
`MinHeapLess()` path, with 4.27 billion inclusive instructions below the
internal-key comparator. The current comparator always dispatches virtually
through its configured user comparator, even when it is LevelDB's built-in
bytewise comparator.

Core's `CDBWrapper::GetOptions()` starts from `leveldb::Options`, whose
default comparator is `BytewiseComparator()`, and does not replace it. The
candidate records that pointer identity once in `InternalKeyComparator` and
uses the equivalent inline `Slice::compare()` for the bytewise case. All
other comparators retain the original virtual `Comparator::Compare()` call.
The internal-key ordering remains: user keys first, then the original
decreasing sequence/type comparison for equal user keys. No database bytes,
comparator name, filter keys, or external API changes.

A focused test constructs two equal-sequence internal keys ordered by a custom
reverse comparator and verifies that `b` sorts before `a`. A temporary mutation
that forced every comparator into the bytewise path failed this oracle with
`1 >= 0`, proving that the test protects the fallback rather than merely
executing the normal Core case.

Six warm alternating CPU-3-pinned scans of the closed chainstate reader all
returned the exact same aggregate tuple:

```
32659375 7659292 1488241113523993 2480426961
```

| version | wall samples | median wall | change |
| --- | --- | ---: | ---: |
| current virtual user comparison | 7.04774, 7.02387, 7.02696, 7.02305, 7.05191, 7.04165 s | 7.034305 s | baseline |
| cached bytewise comparison | 7.04598, 7.00489, 7.00296, 6.99977, 7.00487, 7.05978 s | 7.004880 s | 0.42% faster |

The paired-median improvement is 0.02113 seconds (0.30%); five of six pairs
favor the candidate. Three additional alternating `perf stat` pairs provide
the causal CPU evidence:

| metric | baseline median | candidate median | change |
| --- | ---: | ---: | ---: |
| task-clock | 7.08671 s | 7.05557 s | -0.44% |
| instructions | 67.244941 B | 66.377648 B | -1.29% |
| branches | 13.551859 B | 13.392437 B | -1.18% |
| branch misses | 47.754826 M | 48.682570 M | +1.94% |
| cache misses | 38.928653 M | 38.991140 M | +0.16% |

The instruction and branch reductions are consistent across all three counter
pairs; small miss increases do not overcome the reproducible task-clock and
wall-time improvement. The earlier bytewise `DBIter` dispatch experiment was
rejected because it had no task-clock or paired elapsed-time win. This hotter
internal-key comparison has materially more calls in the merged forward scan
and clears that acceptance bar.

Validation used both the generic-comparator regression test and ordinary
Core cursor/database tests:

```sh
git fetch -q origin master
git log origin/master --format='%h %s' -S'user_comparator_is_bytewise_' -- src/leveldb/db/dbformat.h src/leveldb/db/dbformat.cc
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=leveldb_tests --log_level=message
build/bin/test_bitcoin --run_test=coins_tests_dbbase/coins_db_cursor_order --log_level=message
build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=message
git diff --check
```

The symbolized profile, its separately linked reader, and raw benchmark
commands/binaries are retained under
`/mnt/my_storage/bitcoin-perf-scratch/dbiter-savekey/` as
`callgrind-leveldb-debug.out`, `cursor_stats_bench_leveldb_debug`, and
`cursor_stats_bench_internal_cmp_candidate`. The temporary RelWithDebInfo
build directory is `build-profile/` and is not part of the change.

Reach is all LevelDB paths using an `InternalKeyComparator` with the built-in
bytewise comparator, especially forward merged scans such as
`gettxoutsetinfo`, `scantxoutset`, and the chainstate-copy/statistics phases of
`dumptxoutset`; compaction and point-lookup paths can also execute this
comparator. A non-bytewise comparator keeps its old path. This is a warm,
CPU-bound chainstate measurement and does not claim a full HDD
`-reindex-chainstate` wall-time speedup.

### Rejected: avoid uncompressed-P2PK reconstruction in `CoinStatsValue`

The symbolized closed-chainstate profile also shows legacy uncompressed-P2PK
script handling in the no-hash statistics reader. `ScriptCompression::UnserSize`
must preserve the normal decompressor's special behavior: an invalid encoded
curve point produces an empty script, while a valid one produces a 67-byte
uncompressed-P2PK script. The current implementation reads the 32-byte x
coordinate and calls `DecompressScript`, which validates, expands, and sizes
the key/script.

A temporary direct-Core prototype retained the read bytes and point validation
but replaced expansion with `CPubKey::IsFullyValid()` and the known 67-byte
size. It avoids public-key serialization and copying the expanded key/script,
without changing the valid or invalid result. The existing
`coin_stats_value_matches_coin_deserialization` test passed for compressed and
uncompressed P2PK as well as the other script encodings. The prototype built
with:

```sh
ninja -C build test_bitcoin bitcoind -j4
build/bin/test_bitcoin --run_test=coins_tests/coin_stats_value_matches_coin_deserialization --log_level=message
```

However, the candidate did not clear the performance acceptance bar. Four
warm CPU-3-pinned closed-chainstate reader runs returned the same complete
aggregate for both binaries:

```
32659375 7659292 1488241113523993 2480426961
```

| version | wall samples | median wall | change |
| --- | --- | ---: | ---: |
| reconstruct valid uncompressed P2PK script | 7.164563, 7.181662, 7.153630, 7.141339 s | 7.159096 s | baseline |
| validate and report only its size | 7.146717, 7.119680, 7.165411, 7.169703 s | 7.156064 s | 0.04% faster |

The 0.00303-second median difference is substantially below the 17--23 ms
standard deviation. An earlier ad-hoc pair varied in both directions, which
is why it was not treated as evidence. The raw Hyperfine data is retained in
`/mnt/my_storage/bitcoin-perf-scratch/stats-pubkey-validity/direct-reader.json`;
the temporary linked candidate reader is
`/mnt/my_storage/bitcoin-perf-scratch/dbiter-savekey/cursor_stats_bench_stats_pubkey_validity_candidate`.

Decision: restore `DecompressScript`. The potential reach is only
`gettxoutsetinfo hash_type=none` (via `CoinStatsValue`) on chainstates that
contain these historical uncompressed-P2PK outputs; it does not affect hashed
statistics, `scantxoutset`, `dumptxoutset`, or reindex-chainstate. The tiny
unreproducible difference does not justify another correctness-sensitive
partial-deserialization implementation.

### LevelDB: cache the next data-block restart offset

The symbolized closed-chainstate scan profile used for the forward-iterator
work processed 32,659,375 UTXOs and attributed 4.92 billion inclusive
instructions to `Block::Iter::Next()` and 4.43 billion to
`Block::Iter::ParseNextKey()`. Every parsed data-block entry checked whether it
had crossed the next restart point by decoding that fixed-width restart-array
word afresh, even though the next offset remains unchanged for all entries in
the same restart interval.

`Block::Iter` now caches that next offset. `SeekToRestartPoint()` initializes
it from the following restart (or the existing `restarts_` sentinel), and the
existing `while` loop advances it only after the current entry crosses it. The
condition and loop are deliberately retained: malformed/non-monotonic restart
arrays continue to take the same multi-step recovery path. The cache is only
an equivalent representation of the value formerly returned by
`GetRestartPoint(restart_index_ + 1)`; key reconstruction, block bytes,
comparator behavior, filtering, and iterator ordering do not change.

The new `block_iterator_crosses_restart_points_in_both_directions` test builds
an eight-entry block with a two-entry restart interval, checks complete forward
and reverse traversal, then seeks across restart points and switches direction.
It exercises the cached state through `SeekToFirst`, `SeekToLast`, `Seek`,
`Next`, and `Prev` rather than relying on a synthetic field assertion.

Two independently run warm CPU-3-pinned Hyperfine groups of the closed
chainstate reader returned the same aggregate in every execution:

```
32659375 7659292 1488241113523993 2480426961
```

Across all eight samples, the existing reader median is 7.184500 s and the
cached-restart reader median is 7.141964 s, a 0.59% improvement. The two raw
groups were:

| group | baseline samples | candidate samples |
| --- | --- | --- |
| 3 runs | 7.184214, 7.303063, 7.228086 s | 7.161673, 7.236245, 7.104488 s |
| 5 runs | 7.184786, 7.208772, 7.174608, 7.152168, 7.108873 s | 7.136076, 7.120083, 7.147851, 7.206967, 7.134511 s |

The five-run group alone has 7.174608 s versus 7.136076 s medians (0.54%).
Three subsequent alternating `perf stat` pairs provide the causal result:

| metric | baseline median | candidate median | change |
| --- | ---: | ---: | ---: |
| task-clock | 7.05065 s | 7.01138 s | -0.56% |
| instructions | 66.377233 B | 66.054427 B | -0.49% |
| branches | 13.392379 B | 13.326661 B | -0.49% |
| branch misses | 48.735 M | 45.426 M | -6.79% |
| cache misses | 38.879 M | 38.996 M | +0.30% |

The instruction/branch reduction matches the eliminated fixed-width decode;
the candidate wins all three paired task-clock measurements and has a lower
median. The raw Hyperfine JSON and per-pair perf CSV files are retained
under `/mnt/my_storage/bitcoin-perf-scratch/block-restart-cache/`; the linked
candidate reader is
`/mnt/my_storage/bitcoin-perf-scratch/dbiter-savekey/cursor_stats_bench_block_restart_candidate`.

Validation:

```sh
git fetch -q origin master
git log origin/master --format='%h %s' -S'next_restart_' -- src/leveldb/table/block.cc
ninja -C build bitcoind test_bitcoin -j4
build/bin/test_bitcoin --run_test=leveldb_tests --log_level=message
build/bin/test_bitcoin --run_test=coins_tests_dbbase/coins_db_cursor_order --log_level=message
build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=message
git diff --check
```

The fresh `origin/master` at
`18c05d93016b28a9afd4c716dfe00b6e0accb30b` has no equivalent cache; the
only historical `GetRestartPoint(restart_index_ + 1)` search result is the
original LevelDB import. This is a profile-backed LevelDB change, not an
options change or a RocksDB port. It reaches forward/reverse data-block
iteration in the target UTXO scans and other LevelDB table reads, including
some compaction paths. The evidence is a warm, CPU-bound closed-chainstate
scan; no full HDD `-reindex-chainstate` wall-time improvement is claimed.
