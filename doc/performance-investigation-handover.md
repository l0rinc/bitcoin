# Performance investigation handover

This document hands over the `perf-utxo-rpc-hdd-investigation` branch. It is
intended to let a new agent reproduce, verify, and extend the work without
repeating already-settled experiments or treating one-host measurements as
universal results.

## Scope and current state

The investigation targeted the full-chainstate read paths behind:

- `gettxoutsetinfo`;
- `scantxoutset`;
- `dumptxoutset` and snapshot generation; and
- HDD `-reindex-chainstate`, with a secondary interest in ordinary validation
  and cache work that shares that path.

The detailed experiment log is
[`doc/utxo-performance-investigation.md`](utxo-performance-investigation.md).
It contains hypotheses, rejected candidates, exact commands, profile excerpts,
correctness checks, raw-artifact paths, and limitations. Treat it as the source
of record; this file is a compact map and restart checklist.

At handover the branch head is `4df9895f12` and its comparison base is
`origin/master` at `18c05d9301`. The working tree must remain clean apart from
the user-owned, untracked `qa-assets/` and `test/cache/` directories. Do not
stage, clean, alter, or benchmark against those paths as though they were
generated branch artifacts.

All investigation artifacts are outside the repository, below
`/mnt/my_storage/bitcoin-perf-scratch/`. They are evidence, not inputs to
commits. In particular, the full HDD reindex measurements live in
`reindex-writebuf/` and the post-fix RPC runs in `rpc-mmap/`.

## What is proven on this branch

The stack is deliberately grouped by the shared feature/path rather than by
when the idea was found. Each production commit is self-contained and has its
own detailed rationale and verifier in its commit message and the main log.
Document-only commits record a negative result or a measurement that prevents
future agents from retrying the same weak idea.

### 1. Sequential UTXO reads and UTXO RPCs

These changes improve full chainstate walks used by `gettxoutsetinfo`,
`scantxoutset`, `dumptxoutset`, snapshot-related scans, and in several cases
the scanning phase of coinstats-index work:

| area | commits on this branch | expected reach |
| --- | --- | --- |
| Direct cursor/value reads | `dac4d08672`, `b3af50801d`, `6bde7e1e77`, `8c07bbbb08` | Every chainstate value read through `CDBIterator`; obfuscation-specific portions apply only to the normal obfuscated chainstate database. |
| Forward LevelDB traversal | `fd825127fa`, `ae1fd27ff4`, `28a3be5948`, `c79f770867`, `c9954bce33`, `3b325460bd`, `7005a005c5`, `28e3e3036f` | Forward full-table scans, particularly the three UTXO RPCs. It is not a claim for reverse iterators or point lookups. |
| Cursor key handling | `03195e48ee`, `bc96882608`, `36803c075c`, `926b808a3c` | Key-consuming scans; no-match `scantxoutset` has a smaller reach where it can advance without decoding keys. |
| `scantxoutset` matching | `1a5bfbe875`, `a07f0eeff7`, `fd07d9125f`, `405ab76014` | `scantxoutset` only. Benefits depend on descriptor count and match rate; first-byte filtering is most useful for non-matching coins. |
| Coindata decode | `a68fcd4d77`, `bca3693c6b`, `99796439a3`, `71170ba6c6`, `fb0e4465e4`, `9cd937bc27` | Deserialization of compressed coin amounts and common special scripts, hence scans, validation/cache reads, and snapshot paths. It does not improve `gettxoutsetinfo hash_type=none` where only size information is needed. |
| Unhashed and MuHash coinstats | `ceb0979096`, `cb8a90829a`, `c806cd22bd`, `f38582a6fa`, `cd60d8694e`, `23940ec719`, `e8a3128581` | `gettxoutsetinfo`/coinstats-index variants selected by hash type. MuHash-specific changes do not speed `hash_type=none`; the un-hashed fast path does not speed hash-producing calls. |

The main log contains the individual microbenchmark evidence. The largest
whole-workload result, however, was not a micro-optimization:

| workload, cold rotating HDD | global `MADV_RANDOM` + bounded repair | default mmap policy | default-policy change |
| --- | ---: | ---: | ---: |
| `-reindex-chainstate -dbcache=2000 -stopatheight=957759` | 49,922.207 s | 27,226.893 s | **-45.46% wall time** |
| `gettxoutsetinfo` | median 230.790 s | median 195.405 s | **-15.33%** |
| deterministic no-match `scantxoutset` | 191.95 s | 164.00 s | **-14.56%** |
| `dumptxoutset ... latest` | 340.01 s | 302.12 s | **-11.14%** |

Commit `219c09d86b` removes the global `MADV_RANDOM` advice introduced by the
branch's historical seed and restores the `origin/master` mmap-reader policy.
It therefore has no net source diff relative to current master, but is
essential context: adding global random advice to the LevelDB mmap reader is a
known severe HDD sequential-read regression. The full reindex also showed
7,944,698 to 285,760 major faults and 2,347.936 to 1,670.962 system-CPU
seconds. The two `gettxoutsetinfo` samples were run in reversed order; the
other two RPCs have one pair each. Do not claim a cross-device or SSD result
from these numbers.

The corresponding raw artifacts are:

```text
/mnt/my_storage/bitcoin-perf-scratch/reindex-writebuf/
  master-full-1/metrics/
  current-full-1/metrics/       # global random + repair
  default-full-1/metrics/       # default mmap candidate
/mnt/my_storage/bitcoin-perf-scratch/rpc-mmap/
  fixed-*/metrics/
  random-*/metrics/
```

Correctness proofs for the direct RPC pair include byte-identical JSON for
`gettxoutsetinfo` and `scantxoutset`, plus equal base/hash/coin count and an
identical 9,499,680,598-byte snapshot hash for `dumptxoutset`; see the main
log for SHA256 values and commands.

### 2. Reindex and ordinary validation/cache work

The following are narrow validation/cache optimizations with expected reach
beyond the RPCs:

| commits | change | expected reach |
| --- | --- | --- |
| `1569d9fe02` | avoid cache bookkeeping for provably unspendable outputs | Block connection and reindex only for outputs that are not inserted into the UTXO set. |
| `7b2ae494f9` | reuse input heights in sequence-lock calculation | Transaction validation where the contextual sequence-lock path is active. |
| `2b6f78006f` | skip contextual sigop counting under proven assumevalid conditions | Assumevalid validation/reindex range only; it intentionally does not change consensus outcomes. |

`3f82342732`, `cc52508eb8`, and `58bd8e5e6d` record important full-workload
controls: the pre-fix current stack regressed badly against master, and the
periodic-write/2-GiB-dbcache seed was not active. Do not revive cache,
`block_cache`, or `write_buffer_size` tuning without a new workload showing
that it applies. The user's independent HDD data at height 900000 also found
the proposed LevelDB block-cache/write-buffer settings neutral within noise.

### 3. Explicitly closed paths

Do not spend a first pass reimplementing these. Their measurements, temporary
diffs, or contract objections are recorded in the main log and matching
document commits:

- BIP32 avoidance was dropped: its first-240k-block reach is immaterial for
  the target workload.
- LevelDB block cache, `write_buffer_size`, periodic writes at a 2-GiB
  dbcache, denser Bloom filters, lower restart intervals, 32-KiB tuning, and
  RocksDB-inspired changes did not provide a demonstrated target benefit.
- Cache hash-node caching; forced inlining of the obfuscation/VarInt/cursor
  helpers; append-based block-key reconstruction; and several iterator
  dispatch rewrites were neutral or slower.
- `dumptxoutset` stdio buffering, moving coins into output groups, streamed
  hashing, and rollback interruption batching were neutral or regressions.
- A singleton `scantxoutset` script-size shortcut was neutral. Selective coin
  deserialization is not valid for the scan's required semantics.
- The current post-fix `gettxoutsetinfo` profile mostly attributes time to
  `ComputeUTXOStats` and existing cursor/LevelDB operations already optimized
  by the stack. It did not justify another isolated source change.

## Safe continuation protocol

1. Start from this branch with a new branch on top. Record `git status`, HEAD,
   and the master base. Preserve `qa-assets/` and `test/cache/`.
2. Reuse only explicit scratch datadirs or a documented OverlayFS copy. The
   local `/mnt/my_storage/BitcoinData` is allowed as a read-only lower/input,
   never as a test target to mutate or clean.
3. Build Release binaries with the same compiler and generator for every
   comparison. For any cache-sensitive HDD measurement, use a fresh upper,
   stop the daemon, `sync`, and drop page cache before each timed run. Record
   storage, filesystem, CPU, RAM, dbcache, height, command, commit hash, and
   run order.
4. First establish a baseline and a profile. Only then make one narrow
   hypothesis. A full UTXO scan is not evidence for a point lookup, and a
   warm microbenchmark is not evidence for a cold HDD reindex.
5. Require an observable output equivalence plus the narrowest relevant unit,
   functional, or database test. For performance commits collect at least five
   short runs where practical; for multi-hour reindex runs alternate at least
   three cold runs per side or clearly label weaker evidence.
6. Put each independent finding in one self-sufficient commit authored as
   `Lőrinc <pap.lorinc@gmail.com>`. Update both this handover and
   `doc/utxo-performance-investigation.md` with raw paths, commands, results,
   reach, and limitations. Do not commit profiler output, chainstate data,
   binaries, or scratch scripts.

Useful existing validation examples are:

```bash
ninja -C build bitcoind -j1
ctest --test-dir build --output-on-failure -R '^(dbwrapper_tests|coinsviewoverlay_tests|coinsviewoverlay_tests_noworkers)$'
build/test/functional/test_runner.py feature_utxo_set_hash.py rpc_scantxoutset.py rpc_dumptxoutset.py --jobs=1
```

Choose the test set from the changed path rather than mechanically using these
three commands. The main log lists the exact commands used by every accepted
change.

## Ranked cheap next investigations

These are **workloads to profile first**, not proposed patches. There is no
remaining evidence-backed one-line source fix. A follow-up should stop after a
profile disproves the premise instead of turning these into speculative churn.

1. **Finish the confidence interval for the mmap result.** This is the
   highest-value remaining task, not a new optimization. Alternate at least
   two more cold full reindexes at the fixed height and run another cold pair
   for `scantxoutset` and `dumptxoutset`. Attribute time with `perf stat`,
   major faults, and block-device I/O. This establishes the scope of the
   already-proven regression on HDD before any LevelDB work is considered.

2. **Separate cold scan I/O from CPU in the three UTXO RPCs.** Run
   `gettxoutsetinfo hash_type=none`, `hash_serialized_3`, and `muhash` under
   the same database and capture a call graph plus I/O counters. The existing
   profile says the post-fix generic scan is cursor-dominated; a hash-specific
   profile is required before touching MuHash or serialization. For
   `dumptxoutset`, measure the scan separately from the 9.5-GB snapshot write
   before considering output code.

3. **Use representative `scantxoutset` descriptor shapes.** Benchmark one
   no-match raw script, one matching script, and multi-descriptor requests of
   controlled sizes. Capture match count, script mix, and allocation samples.
   The existing first-byte and lookup changes have different reach in each
   shape; do not infer multi-descriptor wins from the no-match control.

4. **Profile high-rate point RPCs before broadening the code search.** The
   cheapest useful next surface is a deterministic local RPC harness for
   `gettxout`, `getblockheader`, `getblock`, and (with a controlled txindex
   fixture) `getrawtransaction`. Measure cold and warm separately, including
   JSON construction and database lookups. These calls are commonly polled,
   but no current profile identifies a safe shared bottleneck, so an agent must
   not pre-commit serialization, caching, or lock changes.

5. **Profile normal block connection rather than inferring it from reindex.**
   Use a local replay or `bench_bitcoin`/existing validation benchmark with
   representative scripts and a fixed cache. Compare the script-check,
   cache, and LevelDB proportions. This has potentially broad IBD/block-relay
   reach, but consensus and DoS contracts make unprofiled shortcuts
   unacceptable.

6. **Only revisit LevelDB after (1) or (2) identifies a non-mmap bottleneck.**
   The one justified LevelDB policy issue is already fixed by using default
   sequential-friendly mmap behavior. RocksDB is a source of design ideas,
   not a drop-in answer; do not port cache, compaction, or prefetch machinery
   without a measured LevelDB bottleneck and an isolated correctness proof.

## Stop conditions and review bar

Report and do not commit when a candidate is within noise, moves time into a
different bottleneck, changes the workload, weakens validation, adds unbounded
memory/I/O, or lacks output equivalence. In particular, do not trade
consensus/validation, persistence, privacy, DoS resistance, or maintainability
for a microbenchmark result.

A good next handover may contain no source commits: a reproducible profile and
a well-documented rejected hypothesis are more valuable than another broad
performance patch.
