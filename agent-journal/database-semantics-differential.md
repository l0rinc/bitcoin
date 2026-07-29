# Campaign #95 — database-semantics-differential

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/database-semantics. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): LevelDB lineage — exact sync, 3 documented differences, wrapper assumptions consistent

### Draw
Random draw over the 46-goal eligible pool: raw=1294509615595961491,
index 45 -> #95. Cell: LevelDB subtree vs upstream lineage (wrapper
assumptions vs backend behavior).

### Lineage (three tiers, commit-pinned)
- google/leveldb upstream: master HEAD 7ee830d02b (2026-03-11; commits
  list fetched via GitHub API). All semantic changes through HEAD are
  in the lineage (e.g. 578eeb702e Hash pointer-arithmetic fix,
  302786e211/4ee78d7ea9 C++23 fixes).
- bitcoin-core/leveldb-subtree: this tree (5fe0615f7a, 2026-05-28,
  fanquake) is squashed-synced to a7f9bdc611 (2026-05-28), the subtree
  repo's master at that point. Since then the subtree repo gained only
  CI commits (8e89537cef, 47abe23ff2 — no semantic drift).
- Local: no local patches on top of the sync (git log -- src/leveldb
  shows only the sync).

### Implementation differences vs google/leveldb (all INTENTIONAL, verified in-tree)
1. mmap read-only limit 1000 (google: 4096 since 2023): reverted by
   bitcoin-core/leveldb-subtree#52. Verified:
   env_posix.cc:45-46 kDefaultMmapLimit = 1000.
   WRAPPER DEPENDENCY FOUND: dbwrapper.cpp:114-135 SetMaxOpenFiles
   budgets file descriptors around exactly this behavior ("up to that
   amount LevelDB will use an mmap implementation"). Against google
   upstream's 4096 the wrapper's fd budgeting would be off by ~3x.
   The revert keeps the wrapper's documented assumptions true.
2. Seek compaction disabled (bitcoin-core/leveldb-subtree#61): the
   UpdateStats seek-decrement/trigger was removed in the sync
   (version_set.cc diff verified); the seek_compaction branch remains
   but is never armed. Effect: no read-triggered compactions — I/O
   determinism for IBD/reindex (the dominant local workload,
   cf. campaign #21's reindex profiles).
3. [[fallthrough]] modernization (subtree#60): trivial, C++17 cleanup.

### Verdict
- DISMISSED (no drift, no wrapper-assumption bug): the subtree is an
  exact sync of the bitcoin subtree repo at 2026-05-28; every
  divergence from google upstream is an intentional, in-tree-verified
  bitcoin patch; the one wrapper dependency found (mmap fd budgeting)
  is CONSISTENT with the kept behavior.
- Notable for future engine-swap discussions: dbwrapper.cpp:114-135
  implicitly pins the 1000-mmap behavior; any LevelDB upgrade that
  re-raises it needs SetMaxOpenFiles re-derivation.

### Exact commands
- GitHub API: repos/google/leveldb/commits (HEAD + hash.cc/env_posix.cc paths)
- raw.githubusercontent.com diffs: hash.cc at 578eeb702e and 7ee830d02b
  vs src/leveldb/util/hash.cc (showed the subtree#60 patch)
- repos/bitcoin-core/leveldb-subtree/commits (post-sync drift check)
- git show 5fe0615f7a -- src/leveldb/db/version_set.cc (seek disable)

### Limitations
- No engine-alternative trace differential (RocksDB/Pebble) run —
  lorinc/leveldb-to-rocksdb branch exists on the contributor radar
  (#65 queue) as the seed for that heavier cell.
- WAL/MANIFEST corruption fixtures not exercised this cycle.
- SQLite-vs-BDB wallet differential: legacy BDB is on its removal path
  upstream; skipped as low-value.

### Next queue for this campaign
- RocksDB trace differential (via lorinc/leveldb-to-rocksdb):
  comparator ordering, snapshot/iterator semantics, WAL recovery —
  build engine-neutral op traces per campaign protocol.
- dbwrapper batch-atomicity and iterator-invalidation assumption sweep
  (documented LevelDB contract vs wrapper usage).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): dbwrapper contract sweep — batch atomicity via WriteBatch + HEAD_BLOCKS protocol; iterators snapshot-stable in scan-only contexts

Base: b28128fc6b (journal commit for #37 cycle-2 on
audit/build-dead-zones-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/db-semantics-c2 (c1 journal carried).
Start state: clean (untracked scratch only).

### Draw
Random draw over the 35-goal pool (22 pending + 13 CYCLE-1; #37
excluded as just-cycled): raw=11891347519295756407, seed masked to 63
bits (2667975482440980599), index 34 -> #95. Queued cell from c1:
"dbwrapper batch-atomicity and iterator-invalidation assumption
sweep".

### Axis 1: batch atomicity
CDBWrapper::WriteBatch (dbwrapper.cpp:300-308) submits a leveldb
WriteBatch — all-or-nothing by engine guarantee; HandleError turns
engine failures into dbwrapper_error. The coins BatchWrite
(txdb.cpp) splits large flushes into MULTIPLE batches at
batch_write_bytes — cross-batch durability is handled one level up
by the two-phase protocol (DB_HEAD_BLOCKS=[new,old] during the
transition, DB_BEST_BLOCK=new on completion; a crash mid-way is
detected at load and demands reindex). The fork's
5ad0317853 "check db best block state contracts" hardens exactly
this protocol. Contract chain verified end-to-end.

### Axis 2: iterator invalidation/freshness
CDBIterator wraps leveldb::Iterator directly (dbwrapper.cpp:380-386)
— LevelDB's implicit consistent-view-at-creation semantics; later
writes are invisible to it. Callers: txdb NeedsUpgrade (one-shot),
CCoinsViewDB::Cursor (stats/scans, lock-lifetime-guarded by #7 c2's
m_db_mutex), blockstorage blockindex load, and the three index
backends — ALL scan/load contexts; the hot validation path reads via
the coins cache, never raw iterators. Point-in-time consistency is
sufficient everywhere the wrapper is used.

### Verdict
- DISMISSED: both wrapper contracts hold as used; no assumption is
  violated by any production caller. The remaining freshness question
  (scan staleness) is a semantic non-issue in every current context
  (stats/loads tolerate point-in-time views by design).

### Exact commands
- greps/seds: dbwrapper.h:86-155, dbwrapper.cpp:156-167,300-308,
  380-392; txdb.cpp:40,254; NewIterator caller sweep

### Limitations / queue
- RocksDB trace differential (lorinc/leveldb-to-rocksdb) remains the
  heavy queued cell (needs the engine on-host; the branch is
  unassessed per #65 c2).
- WAL/MANIFEST corruption fixtures unexercised (c1 note) — queued
  behind a fixture harness.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-29): LevelDB vs RocksDB reindex A/B on a tx-heavy chain — CPU parity (validation-bound), RocksDB wall -43%

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=1659704294019222637, index 2 -> #95 (third cycle; c2/#65-c3
queued "large DB-engine experiment; #95 differential method").
Branch: audit/db-semantics-c3 from b681a8dc0a (#23 c4 bookkeeping).

### Method (the #95 differential, finally executed)
- Worktree at l0rinc/rocksdb-build-fix (67b37ca71b, base 2026-06-03):
  cmake Release, wallet/tests/fuzz/gui OFF; bitcoind built clean
  (601 edges) — the branch COMPILES AND LINKS today.
- Workload: identical 410-block / ~20k-tx OP_TRUE regtest chain
  (built once by HEAD; reindex rebuilds index+chainstate from
  engine-independent blk*.dat). Foreground /usr/bin/time, wipe-
  aware gating (fresh debug.log per run).
- LevelDB (build-before): user 2.71s, wall 2.26s.
- RocksDB (branch): user 2.77s, wall 1.28s.

### Result
User-CPU parity within noise (+2%): the reindex is validation-
bound (#21 c3: ~86% EC math even for OP_TRUE), so the engines do
not separate on CPU at this scale. RocksDB's wall time is -43%
(1.28s vs 2.26s) — consistent with write-path parallelism
(RocksDB's concurrent memtable/compaction vs LevelDB's serialized
writer); single-run, small-scale, read as directional only.
UTXO-scan differential (gettxoutsetinfo) is meaningless on this
chain: MiniWallet self-transfers keep the set at ~560 entries.

### Harness incidents (recorded)
1. -daemon under /usr/bin/time measures the forking parent
   (0.11s "result"); fixed to foreground + CLI stop.
2. Stale-log gate matched the copied chain log; fixed by
   truncating debug.log per run.
3. rocksdb worktree has no bitcoin-cli (bitcoind-only target) —
   main-tree cli is protocol-compatible; noted.

### Verdict
Findings of fact, journal-only: the swap branch builds and runs a
full reindex correctly today (functional parity, chainstate
byte-level operation confirmed by identical UpdateTip hashes);
the engine differential is CPU-neutral on validation-bound work
and wall-positive for RocksDB on this workload. No local defect;
no adoption decision (fork author's branch).

### Exact commands / artifacts
- git worktree add /tmp/btc95_wt (removed after); cmake --build
  --target bitcoind (601 edges)
- /tmp/btc95_ab.sh (v4, foreground + wipe-aware gate)
- workloads: /tmp/btc25_mw chain (removed)

### Limitations / queue
- DB-dominated separation (mainnet-scale chainstate, or a
  large-UTXO workload > dbcache) untested — needs a much bigger
  experiment (days-scale disk budget this host lacks).
- Durability/crash-consistency differential (kill -9 mid-write,
  recovery comparison) — the natural c4 cell.
- rocksdb-brute (bulk coin ops) unassessed (depends on the swap).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
