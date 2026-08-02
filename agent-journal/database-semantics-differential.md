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

## Cycle 4 (2026-07-29): durability differential — kill -9 mid-reindex, both engines recover to identical tip with zero corruption

### Draw
Re-rank draw over a rebuilt 8-cell queue:
raw=138518960771160384, index 0 -> #95 (fourth cycle; c3 queue
cell "durability/crash-consistency differential"). Branch:
audit/db-semantics-c4 from 050d86133f (#75 c4 bookkeeping).

### Method
Same 410-block tx-heavy chain, fresh copy per engine, truncated
debug.log, -reindex in foreground; wait for the first UpdateTip
past height 100, then kill -9; restart normally (no -reindex) and
wait for height=410. Compare restart tip hash + corruption lines.

### Result
- LevelDB: killed at height=227; restart -> tip 0f9188f13cb7b2c7
  (the full 410 tip), corruption-lines=0.
- RocksDB: killed at height=223; restart -> tip 0f9188f13cb7b2c7
  (identical hash), corruption-lines=0.
Both engines roll forward from the crash point to the identical
final state. Crash-consistency parity CONFIRMED (matches #93 c1's
LevelDB-only mid-flush result).

### Verdict
DISMISSED (clean): the RocksDB swap's durability behavior matches
LevelDB's on this crash class. The engine-swap assessment suite is
now complete: builds clean, reindexes correctly (identical tip),
CPU parity, wall advantage, crash-consistency parity. All findings
recorded for the fork author; no local defect.

### Exact commands
- /tmp/btc95_dur.sh (kill -9 protocol per engine)
- artifacts removed post-run (worktree, datadirs)

### Limitations / queue
- Kill timing is load-point-random (height 223-227); a
  write-flush-windowed kill (inside a batch commit) would need
  instrumented timing — noted as the harder cell.
- rocksdb-brute bulk-ops still unassessed (depends on the swap).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 5 (2026-07-31): write-flush-windowed kill — kill -9 inside every batch commit of a reindex; identical recovery at all 4 windows; DISMISSED

### Draw
RE-RANK draw 137 over the 5-cell queue: raw=16972436101960705472,
masked 7749064065105929664 -> idx 4 -> #95 write-flush-windowed kill
(c4's "harder cell"). Branch: audit/db-semantics-c5 from d44cad1b23.

### Cell and harness (preserved in /tmp/btc95c5/)
c4 killed load-point-random (height 223-227); c5 kills INSIDE the
batch commits themselves. Harness: /tmp/btc95c5_chain.py (260-block,
450-tx MiniWallet chain, --nocleanup), /tmp/btc95c5/run_trial.sh
(env-targeted kill + recovery check). Fault injection: a TEMPORARY
hook in CDBWrapper::WriteBatch (atomic counter; _Exit(42) at
BTC_KILL_AT_WRITE=N, right before pdb->Write), incrementally
rebuilt, REVERTED after the cell (tree clean; binary rebuilt).
Calibration: the full reindex+shutdown of this chain performs
exactly 5 WriteBatch calls: 1=chainstate(obf key), 2=index flush,
3=chainstate coins flush (end of reindex), 4=index flush, 5=
chainstate shutdown flush. gdb interposition was tried first and
abandoned: `silent` suppresses hit messages (blind counting), and
the chain fits dbcache even at -dbcache=4 (small UTXO set), so the
only real write windows are the final/shutdown flushes — a mid-run
flush-window cell needs a multi-GB UTXO chain (queued, disk-bound).
pkill note: never pkill -f 'build-before/bin/bitcoind' — the pattern
matches the invoking shell's own cmdline (self-kill); use -x.

### Result (control tip 09844db1e4ab... at height 260, clean reindex)
- kill at write 2 (index flush #1):  exit=42 mid-commit; restart ->
  height 260, tip 09844db1..., corruption lines 0.
- kill at write 3 (coins flush):     exit=42; identical recovery.
- kill at write 4 (index flush #2):  exit=42; identical recovery.
- kill at write 5 (shutdown flush):  exit=42; identical recovery.
All four windows roll forward to the byte-identical tip with zero
corruption/truncation/checksum log lines.

### Verdict
DISMISSED (clean): the HEAD_BLOCKS/WriteBatch crash protocol holds
when the kill lands inside any batch commit of the reindex lifecycle,
not just at random heights (c4). Combined with #93 c1 (LevelDB
mid-flush) and c4 (both engines, random timing), the durability
surface is closed at this scale.

### Exact commands
- /tmp/btc95c5_chain.py (prep), /tmp/btc95c5/run_trial.sh N for
  N in 2..5; hook patch: dbwrapper.cpp WriteBatch preamble (reverted;
  journal records the diff).

### Limitations / queue
- Mid-RUN flush window (kill while a dbcache-pressure flush commits
  at height ~200 of 260) needs a multi-GB UTXO chain — same disk
  constraint as c2's large-UTXO cell; queued together.
- RocksDB arm of the window kill not run (engine swap not built this
  cycle); c4's parity result covers the engine question at random
  timing.
- rocksdb-brute bulk-ops still unassessed (depends on the swap).

## Rotation note
Cycle 5 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 6 (2026-08-02, draw 173, raw=17651119340599297244, masked 8427747303744521436, idx 0/4 -> STALE (#55 exhausted; pool repair), redraw raw=2606098433264139421 (63-bit) idx 1/3): large-UTXO kill-during-pressure-flush — forced at SMALL scale via -dbcache=4; 3/3 trials zero-corruption; DISMISSED

### Hypothesis
H (c2/c5 queued): kill -9 while a dbcache-PRESSURE flush commits
(mid-run, not shutdown) corrupts or loses state. c5 noted it needs
a multi-GB UTXO chain. Reframed: -dbcache=4 (minimum) + many-
output txs force the same flush pressure at small scale; disk
constraint removed.

### Rig (/tmp/btc95c6/pressure_kill.py, preserved)
- Single node, -dbcache=4 -checkblockindex=1, MiniWallet RAW_P2PK.
- Pressure: 40 rounds of 8x send_self_transfer_multi(num_outputs=
  100, confirmed_only=True) + 1 block; then a spam thread keeps
  pressuring while the main thread sleeps a seeded-random 2-6s and
  SIGKILLs the daemon mid-connect (seed 0x95C6, recorded).
- Framework quirks recorded: sqlite wallet flush-warning lines
  match naive 'corrupt' greps (filtered by 'sqlite' exclusion);
  gettxoutsetinfo 'transactions' counts TXs not outputs (use
  'txouts'); immature-coinbase and dust-input failures need
  confirmed_only=True, not hand-picked max-value UTXOs.

### Results (3/3 trials)
- Trial 1: kill mid-pressure with 329 flush lines in the log;
  recovery h2=150 (== pre-kill quiescent height) -> forward-mined
  tip 160; corruption-lines=0 (keywords corrupt/checksum/truncat/
  database error/leveldb error, sqlite excluded).
- Trials 2, 3: identical RESULT lines (zero corruption, height
  preserved, forward mining clean).
- Node continues accepting and mining blocks post-recovery —
  chainstate and block files consistent.

### Verdict
DISMISSED: the dbcache-pressure flush window is as safe as the
c4 random-timing and c5 per-batch windows — the HEAD_BLOCKS/
WriteBatch crash protocol holds under sustained flush pressure;
the multi-GB chain was never necessary (-dbcache=4 forces the
same code path). Durability surface closed at this scale.

### Exact commands
- python3 /tmp/btc95c6/pressure_kill.py --tmpdir=... --configfile=
  build-before/test/config.ini (x3); RESULT lines above.

### Limitations / queue
- RocksDB arm of the pressure kill not run (c4 covers engine
  parity at random timing; the swap build is recreateable per c3).
- rocksdb-brute bulk-ops remains the last #95 cell (depends on
  the swap).

## Cycle 7 (2026-08-02, draw 175, raw=17804226048192678763, masked 8580854011337902955, idx 1/2): rocksdb-brute bulk-ops assessment — CDBWrapper::MultiRead on the unmerged branch has THREE independent defects, ASan-proven UAF + functional misalignment; CONFIRMED branch-local, ZERO HEAD exposure; campaign COMPLETE

### Scope note
External branch l0rinc/l0rinc/rocksdb-brute (tip d2cd534521, Oct
2024, bulked HaveInputs/AddCoins + GetCoins/MultiRead) — a seed,
not proof; the assessment below produced the runnable proof.
MultiRead exists ONLY on that branch (absent from rocksdb-
build-fix and leveldb-to-rocksdb); HEAD has NO MultiRead/GetCoins
anywhere (grep-verified 2026-08-02).

### Defects (verbatim code, branch:src/dbwrapper.h:241-267)
1. DANGLING SPANS: the loop builds DataStream ssKey per key, then
   keySpans.emplace_back(MakeByteSpan(ssKey)) — ssKey dies each
   iteration; MultiReadImpl receives spans into freed memory.
2. DOUBLE COUNT: keySpans(keys.size()) value-initializes N empty
   spans AND appends N real ones — MultiGet gets 2N slices, the
   first N empty.
3. RESULTS DOUBLING: results(strValues.size()) default-inserts
   N' values, then push_back appends N' more — 2N' results; the
   GetCoins assert(ret.size()==outpoints.size()) can never hold,
   and release builds index misaligned/default coins (deserial-
   ization of empty NotFound values throws into assert(false)//
   TODO in debug, results.clear() in release -> out-of-bounds
   indexing in AddCoins/HaveInputs).

### Runnable proof (/tmp/btc95c7/multiread_probe.cpp, preserved;
verbatim template extraction against HEAD headers, Span->std::span
mechanical substitution, Xor no-op omitted, mock MultiReadImpl)
- ASan (-fsanitize=address -O1): heap-use-after-free, first-invalid
  read in MultiReadImpl (probe:28) <- MultiRead (probe:49), freed
  by DataStream::~DataStream (streams.h:164, probe:42 loop-local),
  allocated by ssKey.reserve (probe:44). Full 3-frame lifecycle.
- Non-ASan: 6 slices for 3 keys (2N proof; first 3 EMPTY), and all
  3 real slices byte-IDENTICAL (c819aeaa0a000000 — freed-buffer
  reuse makes every key read as the same wrong key); MultiRead
  returned 12 results for 3 keys (4N; assert-break + misalignment
  proof).

### Reachability on the branch (severity context, NOT HEAD)
GetCoins is called by CCoinsViewCache::HaveInputs (every mempool
tx acceptance) and AddCoins(check_for_overwrite=true) (ConnectBlock
BIP30 path) — so the branch's bulk paths hit the defects on first
use: debug builds abort (assert), release builds query wrong/empty
keys and index out of bounds (UB). Consensus-adjacent ON THE
BRANCH; the branch was never merged and HEAD is structurally
unaffected (no MultiRead, per-coin FetchCoin semantics intact —
see c1/c2 contract rows).

### Verdict
CONFIRMED (branch-local defect, runnable evidence, two verifier
forms: sanitizer trace + functional miscount). NOT a HEAD/upstream
issue — no URGENT entry, no fix commit (superseded experiment;
repairing it is not a HEAD deliverable). The correct bulk-fetch
shape (persistent key storage, reserve-not-size, reserve-results)
recorded here for any future revival.

### Why the branch's own builds missed it
Release builds don't fire the size assert; the misindexed default
coins are spent-by-default, so HaveInputs conservatively returns
false (tx rejection, not acceptance) in the common path — the
silent-failure direction is availability, and only the BIP30
overwrite arm reads the wrong boolean in the dangerous direction.

### Campaign #95: COMPLETE
c1 leveldb exact sync; c2 dbwrapper contracts; c3 engine A/B;
c4 durability kill; c5 per-batch windows; c6 pressure-flush kill;
c7 rocksdb-brute bulk-ops autopsy.
