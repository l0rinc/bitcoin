# Bitcoin Core #35298 LevelDB Compaction Investigation

Date: 2026-05-18 UTC
Updated: 2026-05-19 UTC

## Summary

I reproduced the high chainstate write amplification on the local datadir
`/mnt/my_storage/BitcoinData` with `-txindex=0`, after the node was fully synced.
The reproduced wave was not an IBD/reindex/startup catch-up wave and was not
caused by txindex. It was a delayed LevelDB compaction wave that started without
a new chainstate flush and rewrote most of the chainstate.

The strongest root-cause explanation is LevelDB seek compaction. Bitcoin Core's
chainstate uses random keys, many LevelDB levels, and a small LevelDB write buffer
for the coins DB. Normal reads can charge LevelDB's `allowed_seeks` counters and
schedule read-triggered compactions. Those compactions are not useful for this
workload and can rewrite many GiB.

A local candidate fix is implemented in this worktree: disable LevelDB
seek-triggered compactions globally in the vendored LevelDB by making
`Version::UpdateStats()` a no-op. This avoids new Core option plumbing and
applies the same behavior to chainstate, indexes, and the block tree DB. Size
compactions and manual compactions remain enabled.

## Sources Checked

- Current issue: https://github.com/bitcoin/bitcoin/issues/35298
- PR #30611, periodic chainstate writes: https://github.com/bitcoin/bitcoin/pull/30611
- PR #30039, 32 MiB LevelDB files: https://github.com/bitcoin/bitcoin/pull/30039
- Prior Core issue: https://github.com/bitcoin/bitcoin/issues/29662
- LevelDB prior art: https://github.com/google/leveldb/issues/229
- LevelDB prior art: https://github.com/google/leveldb/issues/857
- Geth disabled seek compaction: https://github.com/ethereum/go-ethereum/pull/20130
- goleveldb per-read option discussion: https://github.com/syndtr/goleveldb/pull/297
- C++ LevelDB option precedent: https://github.com/Mojang/leveldb/pull/4

## Local Run

Run directory:

```sh
issue-35298/runs/20260518T182043Z
```

Command:

```sh
/data/my_storage/bitcoin/build-35298/bin/bitcoind -datadir=/mnt/my_storage/BitcoinData -daemonwait -pid=/data/my_storage/bitcoin/issue-35298/runs/20260518T182043Z/bitcoind.pid -server=1 -nosettings -txindex=0 -dbcache=800 -maxmempool=100 -debug=leveldb -debug=bench -debug=coindb -debuglogfile=/data/my_storage/bitcoin/issue-35298/runs/20260518T182043Z/debug.log -printtoconsole=0
```

Run metadata:

- Commit: `ccbd00ab87c09779e9edc589af6df5c967687a2f`
- Version: `v31.99.0-ccbd00ab87c0`
- Datadir: `/mnt/my_storage/BitcoinData`
- Filesystem target: `/data`, source `/dev/nvme0n1p4`, ext4, `rw,relatime`
- Config used: `server=1`, `-nosettings`, `txindex=0`, `dbcache=800`,
  `maxmempool=100`, `debug=leveldb`, `debug=bench`, `debug=coindb`
- The node reached fully synced state at `2026-05-18T19:14:04Z`
  with `blocks=headers=949977`, `initialblockdownload=false`.
- The node was stopped cleanly after reproduction to avoid continued SSD writes.

## Steady-State Evidence

The startup/catch-up period was excluded. Clean steady-state began after the
large catch-up/cache-pressure wave ended around `2026-05-18T19:19:39Z`.

Parsed waves from `debug.log` after that point:

| Wave | Start | End | Trigger context | Levels | Generated GiB | Notes |
| --- | --- | --- | --- | --- | ---: | --- |
| 1 | 20:09:14 | 20:09:19 | periodic flush | 1->2 | 0.050 | Small immediate compaction |
| 2 | 20:35:36 | 20:37:01 | delayed, no flush | 2->3 | 0.050 | Small delayed compaction |
| 3 | 21:00:29 | 21:03:35 | periodic flush plus delayed part | 2->3 | 0.054 | Small |
| 4 | 21:25:41 | 21:40:34 | delayed, no flush | 3->4 | 8.445 | Large reproduced wave |

The last periodic chainstate flush before the large wave was at:

```text
2026-05-18T21:00:29Z [coindb] Writing chainstate to disk: flush mode=PERIODIC, prune=0, large=0, critical=0, periodic=1
2026-05-18T21:00:29Z [bench] BatchWrite: write coins cache to disk (44802 out of 405035 cached coins) started
2026-05-18T21:00:30Z [bench] BatchWrite: write coins cache to disk (44802 out of 405035 cached coins) completed (168.76ms)
```

The large wave started later, without a new chainstate flush:

```text
2026-05-18T21:25:41Z [leveldb] Compacting 1@3 + 10@4 files
...
2026-05-18T21:40:34Z [leveldb] compacted to: files[ 0 0 0 1 278 71 0 ]
```

Process I/O for the large wave window:

```text
window: 2026-05-18T21:25:33Z to 2026-05-18T21:41:43Z
read_bytes:  5.314758 GiB, 19.725 GiB/hour
write_bytes: 8.452435 GiB, 31.370 GiB/hour
```

Chainstate file age after reproduction:

```text
all:   351 files, 10.635 GiB
<=1h:  277 files, 8.448 GiB, 79.43%
>2h:    73 files, 2.187 GiB, 20.56%
```

This matches the issue reports that most chainstate SST files become very recent.

## Interpretation

This was not just the expected 50-70 minute write. The periodic write created
small immediate/delayed compactions. The large wave occurred around 25 minutes
later and involved repeated `1@3 + 10/11@4` compactions, consistent with
read-triggered seek compactions.

This was not a txindex-only effect. The local command explicitly used
`-txindex=0`, and issue comments now include no-txindex reports as well. txindex
can still amplify the symptom by taking a cache share and by adding its own DB
activity, but it is not required.

This was not just the 2 MiB to 32 MiB migration. The chainstate was mostly
32 MiB-ish files and the wave rewrote modern large SSTs. `-forcecompactdb=1`
also did not fix the issue for the reporter: after a one-shot compact all small
files disappeared, but about one hour later the node rewrote about 10 GiB again,
with a post-compaction size distribution still dominated by large files
(`648` files around 33 MiB, plus a small tail of partial files).

The issue also has reporter evidence that the behavior does not simply settle
after the post-v29 file-size migration: one report measured about 13.6-13.8
GiB/hour for around 12 days on v30.2/v31.0 nodes, and later no-txindex reports
showed most chainstate bytes touched within a few hours. An overnight no-txindex
report on unmodified v31.0 measured about 197 GiB of generated LevelDB output
since the marker and still had 99.89% of chainstate bytes touched within two
hours.

The write buffer point matters, but it is not the whole bug. For the chainstate
DB, Core caps `coins_db` LevelDB cache at 8 MiB, and `CDBWrapper` sets
`write_buffer_size = nCacheSize / 4`, so the chainstate write buffer is about
2 MiB. During IBD or reindex-chainstate, a full cache flush will still exceed
that and create/compact SSTs regardless of a moderate write-buffer increase.
In steady-state, even a small sync with more than about 2 MiB of dirty entries
can create a new SST. That can increase compaction opportunities, but it does
not explain the delayed whole-DB rewrite by itself. The reproduced large wave
maps to LevelDB's seek compaction path.

## Relevant Code

- `src/dbwrapper.cpp`: `write_buffer_size = nCacheSize / 4`,
  `max_file_size = max(default, 32_MiB)`.
- `src/kernel/caches.h`: chainstate LevelDB cache is capped by
  `MAX_COINS_DB_CACHE{8_MiB}`.
- `src/leveldb/db/db_impl.cc`: `DBImpl::Get()` calls
  `current->UpdateStats(stats)`, and `DBImpl::RecordReadSample()` can schedule
  compaction after iterator read samples.
- `src/leveldb/db/version_set.cc`: `Version::UpdateStats()` decrements
  `allowed_seeks`; `PickCompaction()` selects `file_to_compact_` when no size
  compaction is pending.

## Version Timeline

- LevelDB seek compaction entered Bitcoin Core with the LevelDB import,
  commit `5e650d6d2d`, first in `v0.8.0`.
- PR #30039 / commit `b73d331937` raised LevelDB max file size to 32 MiB,
  first in `v29.0`.
- PR #30611 / merge `5b8046a6e8` added the 50-70 minute chainstate write
  cadence, first in `v30.0`.

## Candidate Fix

Current local diff:

```text
src/leveldb/db/autocompact_test.cc | updated expectation
src/leveldb/db/db_test.cc          | updated seek-compaction regression test
src/leveldb/db/version_set.cc      | disable seek-triggered compaction
```

Behavior:

- `Version::UpdateStats()` always returns `false`, so reads do not decrement
  `allowed_seeks` and do not set `file_to_compact_`.
- This disables LevelDB seek compactions for all Core LevelDB users without
  adding a new Core-facing option.
- Size compactions still use `compaction_score_`.
- Manual compactions still use `CompactRange`.

This is narrower in code than a chainstate-only option because it avoids
plumbing through `DBOptions`/`CDBWrapper`. The behavioral change is broader: it
also applies to txindex, coinstatsindex, block filter indexes, and block tree
LevelDB. The current issue discussion argues that this is acceptable because the
seek-compaction cost model was tuned for disks with expensive random seeks, and
such storage is not a realistic target for modern bitcoind workloads.

The latest issue measurements support this direction: a patch that disables
seek compaction measured around `2.19 GiB` read and `511 MiB` written, while
raising the coins DB write buffer measured around `34.74 GiB` read and
`31.97 GiB` written, and master measured around `216.66 GiB` read and
`155.92 GiB` written over the same style of run. The higher write buffer delays
the problem but does not remove the seek-compaction path.

## Tests Run

Build:

```sh
TMPDIR=/data/my_storage/tmp CCACHE_DIR=/data/my_storage/ccache CCACHE_TEMPDIR=/data/my_storage/ccache/tmp cmake --build build-35298-seekfix --target bitcoind bitcoin-cli test_bitcoin -j8
```

Unit tests:

```sh
build-35298-seekfix/bin/test_bitcoin --run_test=dbwrapper_tests
build-35298-seekfix/bin/test_bitcoin --run_test=validation_flush_tests
build-35298-seekfix/bin/test_bitcoin --run_test=coins_tests
```

Standalone LevelDB tests:

```sh
TEST_TMPDIR=/data/my_storage/tmp/leveldb-tests /data/my_storage/tmp/leveldb-manual-tests/db_test
TEST_TMPDIR=/data/my_storage/tmp/leveldb-tests /data/my_storage/tmp/leveldb-manual-tests/autocompact_test
```

All passed. `db_test` now passes 56 tests because the earlier additional
chainstate-option test was removed and the existing empty-level seek-compaction
test was converted to the global no-seek-compaction expectation.

## Mitigations and Tradeoffs

Likely helpful:

- Disable LevelDB seek compaction. Size and manual compactions remain enabled.
- Increase `-dbcache` if memory is available. This reduces chainstate DB misses
  but does not increase chainstate LevelDB block cache past the 8 MiB cap.
- Disable unneeded indexes. This helps cache budget and avoids index DB work,
  but it is not sufficient because the local no-txindex run reproduced the bug.
- Reduce mempool workload, for example smaller `-maxmempool` or `-blocksonly`,
  if acceptable.

Less useful as a root fix:

- `-forcecompactdb=1`: useful for one-shot cleanup, but reporter data and the
  local reproduction indicate recurring seek compactions can return. The
  reporter's post-`forcecompactdb` run rewrote about 10 GiB again after one hour
  despite the chainstate being mostly 33 MiB files.
- Increasing the periodic write interval: reduces flush cadence but does not
  address read-triggered seek compactions.
- Increasing LevelDB write buffer size: may reduce L0 churn in steady-state,
  IBD, or reindex-chainstate, but does not address the seek compaction path that
  rewrote most of chainstate here.

Expected side effects of disabling seek compaction:

- Read amplification may increase in Core LevelDB workloads where LevelDB's seek
  heuristic is actually useful.
- Old overwritten/deleted entries may persist until size or manual compaction.
- Size compactions still keep the DB bounded and balanced.
- Manual/full compaction still works.

## Remaining Risks

- I did not run the node for 1-2 days after reproduction because the bug was
  reproduced, issue comments already include multi-day persistence evidence,
  and continuing the local unpatched run would intentionally write many more
  GiB to the SSD.
- I did not run a patched mainnet node for a full periodic cycle. The patch
  directly disables the scheduling path identified by the reproduction and by
  upstream prior art, and targeted unit tests pass, but long-run A/B data would
  still be useful before opening a PR.
