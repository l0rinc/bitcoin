# Campaign #54 — raii-resource-leaks

## Cycle 1 (2026-07-29): core resource-acquisition sweep — RAII pervasive; two manual paths analyzed, no reachable leak

### Draw
Random draw over the 2-goal eligible pool (1 pending + 1 CYCLE-1,
#102 excluded as just-cycled): raw=9209411641343266232, index 0 ->
#54 (first cycle). Branch: audit/raii-resource-leaks from
374f28664a (#102 c1 bookkeeping; lineage anchor
audit/resurrection @ 5d0155254c). Start state: tracked-clean.
Catalog note: #54's campaign-focus block holds alt-implementation
mining text — same offset artifact class; title+slug authoritative.

### Hypothesis
An exception or early return in a core write/read path leaks a
resource (file handle, allocation) — the residual class after
RAII-ification.

### Sweeps
1. fopen sweep (node, validation, index, dbwrapper, util): every
   core call site wraps immediately in AutoFile (RAII) —
   utxo_snapshot.cpp:32-33/67-68, blockstorage.cpp:1250/1254/1354.
   The two unwrapped sites are util/readwritefile.cpp:15-34/36-49
   (manual fclose on every path) and util/fs.cpp itself.
2. raw new/delete sweep (validation, txmempool, net_processing,
   coins, txdb, blockstorage, dbwrapper): no bare `= new` outside
   dbwrapper.cpp:74/:146 (analyzed below); no manual deletes in
   core objects (matches are comments).
3. Peer/node lifetime: refcount caution at FinalizeNode already
   verified (#1 c2 spot check).

### Manual paths analyzed (both upstream-matching, theoretical-only)
- util/readwritefile.cpp: ReadBinaryFile/WriteBinaryFile close on
  every path; the only leak shape is a bad_alloc from
  std::string::append (ReadBinaryFile:30) skipping fclose — memory-
  pressure-only, upstream code (Zcash lineage), no production
  scenario (bounded maxsize callers: settings/banlist).
- dbwrapper.cpp:74: new char[30000] freed at :106-107 on the only
  exit path; leak needs bad_alloc from LogDebug — same class.
  dbwrapper.cpp:146: options.info_log ownership follows LevelDB's
  documented transfer — upstream-identical pattern.

### Verdict
DISMISSED: no reachable resource leak in core paths; RAII coverage
is pervasive; the two manual paths are upstream-matching and
exception-theoretical only.

### Exact commands
- greps: fopen( in node/validation/index/dbwrapper/util; `= new `/
  delete in 7 core TUs
- reads: readwritefile.cpp (full), utxo_snapshot.cpp:28-78,
  dbwrapper.cpp:60-155, blockstorage.cpp:1250/1354

### Limitations / queue for cycle 2
- LevelDB-internal ownership (info_log on failed DB::Open) is
  subtree behavior; upstream domain.
- GUI/qt object trees out of scope (deprioritized).
- IPC/capnp resource lifecycles unexamined (deprioritized unless
  core-reachable).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
