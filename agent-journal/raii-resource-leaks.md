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

## Cycle 2 (2026-07-30): CONFIRMED+FIXED — options-owned allocations leak on every failed CDBWrapper construction (73a6798206)

### Draw
Re-rank draw over the remaining 3-cell queue:
raw=18214628344123528261, masked 8991256307268752453, index 2
(of 3) -> #13 (second cycle; c1 queue cell "LevelDB-internal
ownership (info_log on failed DB::Open)"). Branch:
audit/raii-resource-leaks-c2 from 96075f9172 (#74 c3 journal tip).

### Hypothesis (c1 cell, now confirmed)
LevelDB does not take ownership of options.info_log /
filter_policy / block_cache; the caller must delete them. On a
failed DB::Open the CDBWrapper constructor throws, ~CDBWrapper
never runs (partially-constructed), and LevelDBContext had no
destructor — every failed construction leaks the option-owned
allocations.

### Mechanism (code chain)
dbwrapper.cpp:228 GetOptions (info_log new :146, block_cache
NewLRUCache :142, filter_policy :144, penv maybe :235) ->
:253 DB::Open fails (or :245 TryCreateDirectories throws) ->
:254 HandleError throws dbwrapper_error -> constructor unwinds:
m_db_context (unique_ptr) destroys the LevelDBContext STRUCT but
no member pointers are freed. Upstream master: same missing
destructor (origin/master:197).

### Failing-before (ASan+LSan probe /tmp/lk_probe.cpp)
19 failed opens (alternating file-as-path and garbage-CURRENT):
79,800 bytes leaked in 361 allocations; direct-leak stacks for
all three classes under GetOptions <- CDBWrapper::CDBWrapper.
SUMMARY line captured.

### Fix (73a6798206, smallest RAII change)
LevelDBContext destructor: delete pdb first, then
options.filter_policy, options.info_log, options.block_cache,
penv. Safe on both paths (Close() nulls them on success;
deleting nullptr is safe).

### Passing-after
- LSan: 20 failed opens, silent (no leak report).
- Regression: dbwrapper_tests "No errors detected".

### Verdict
CONFIRMED+FIXED: a real per-failure RAII leak (small but exact:
~3.7KB+internal per failed open, unbounded under repeated
failure), with deterministic sanitizer evidence and a green
regression suite. Upstream-matching gap; offerable upstream.

### Exact commands
- g++ -g -fsanitize=address ... /tmp/lk_probe.cpp src/dbwrapper.cpp
  build-before/src/libleveldb.a ... (link list in probe dir)
- ASAN_OPTIONS=detect_leaks=1 /tmp/lk_probe (before/after)
- build-before/bin/test_bitcoin --run_test=dbwrapper_tests

### Limitations / queue
- Repeated-failure frequency in production is low (startup-time
  only, one wrapper per coins db per open) — severity small; the
  value is the exact RAII defect + upstreamable one-liner-class
  fix, not the bytes.
- GUI/qt and IPC/capnp lifecycles remain deprioritized.

## Rotation note
Two cycles; the LevelDB ownership cell is closed with a fix. Not
exhausted (nothing queued).

## Cycle 3 (2026-08-02, draw 212, raw=6892152841030680972 (63-bit), idx 12/19): subprocess.h resource hygiene — Linux arms RAII-clean; Windows szCmdline leaked per Popen (vendored cpp-subprocess defect, present in the library itself); CONFIRMED-LATENT external, no local fix

### Sweep (util/subprocess.h, 1,486 lines, vendored MIT
cpp-subprocess by arun11299; Core consumers: external_signer,
util/exec, common/run_command)
- LINUX arm (host-relevant): pipe_cloexec fds closed on fork
  failure before OSError (:1184-1186); child closes parent fds +
  err_rd (:1195-1198); parent closes err_wr + child fds,
  fclose(err_fp) (:1212); fdopen-failure arm closes err_rd
  before throw. Popen streams fclose-deleter wrapped (:854-856).
  RAII-CLEAN on every error path.
- WINDOWS arm: execute_process allocates szCmdline via raw
  `new wchar_t[command_line.size() + 1]` (:1104) and NEVER
  deletes it — zero delete[] in the entire file (grep-verified).
  Leaks (len+1)*2 bytes per Popen on BOTH the success and the
  CreateProcess-failure paths. cleanup_future_ is safe by
  std::future(async) blocking-destructor semantics (the `this`
  capture cannot outlive the Popen).

### Provenance/scope
- Upstream Bitcoin master (origin/master @556988790a): same
  code, 0 delete[].
- Upstream LIBRARY (arun11299/cpp-subprocess master, fetched
  today): same, 0 delete[] — the defect lives in the vendored
  library itself; the correct venue is a library-upstream fix,
  not Core-side divergence (vendored minimal-diff rule).
- Severity: Windows-only, ~100-500 B per subprocess invocation
  (external-signer/exec call sites are operator-frequency, not
  per-block). CONFIRMED-LATENT external — no URGENT entry.

### Verdict
CONFIRMED-LATENT (vendored, Windows-only, per-call small leak);
Linux arms proven clean. No local fix (vendored divergence
rule); report-ready one-liner (delete[] after CreateProcessW /
unique_ptr<wchar_t[]>) recorded for a library-upstream report.

### Exact commands
- sed/grep line refs above; git show origin/master version
  (0 delete[]); curl library master (0 delete[]).

### Limitations / queue
- Windows behavior is static-only (no Windows build here).
- GUI/qt and IPC/capnp lifecycles remain deprioritized (c2).
