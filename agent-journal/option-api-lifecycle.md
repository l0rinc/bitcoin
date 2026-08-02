# Campaign #43 — option-api-lifecycle

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/option-lifecycle. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): -prevoutfetchthreads lifecycle — all stages clean

### Draw
Random draw over the 35-goal eligible pool: raw=9057022781452753954,
index 14 -> #43. Target: the fork's home-grown option
(5bf1c32008) — least upstream-reviewed lifecycle surface.

### Lifecycle trace (each stage, exact evidence)
- registration: init.cpp:541 — help text states 0 disables, up to 16,
  default 8, negative rejected. Accurate vs implementation.
- parse/validation: chainstatemanager_args.cpp:63-68 — negative value
  returns util::Error (rejected at startup; the
  chainstatemanager_loadblockindex test asserts the -1 error and the
  clamp-to-MAX for 100). >MAX clamps silently to
  MAX_PREVOUTFETCH_THREADS=16 (validation.h:93) — documented-by-test.
- non-integer input: GetArg<int32_t> -> SettingTo ->
  LocaleIndependentAtoi (strencodings.h:106-127) — atoi emulation by
  design ("abc" -> 0, whitespace trimmed, leading + accepted). So
  garbage disables parallel fetching silently. Tree-wide convention
  for every integer option; this option follows it faithfully. Not a
  mismatch.
- storage/observation: kernel/chainstatemanager_opts.h:25,51 —
  DEFAULT_PREVOUTFETCH_THREADS{8}; single consumption point
  (validation.cpp:1969 Chainstate::InitCoinsCache).
- scheduling: CoinsViews::InitCache (validation.cpp:1884-1894) —
  creates a FRESH ThreadPool per call; >0 starts it with N workers +
  LogInfo; 0 skips Start (empty pool, no threads, no log).
  Double-start impossible (ThreadPool::Start throws on reuse at
  util/threadpool.h:110, but each InitCache makes a new pool).
- shutdown/restart: pool owned by CoinsViewOverlay; destructs with
  the chainstate (ThreadPool dtor joins workers). Option is
  command-line only — not persisted to settings.json (verified: only
  rw_settings via GUI/RPC persist; this is not one).
- non-primary modes: reindex/loadblock/prune share the same
  InitCoinsCache path (loadblockindex test covers the assumed-valid
  variant; snapshot chainstate uses its own CoinsViews at
  validation.cpp:5893).
- consensus relevance: I/O parallelism only; no behavior/validity
  effect.

### Verdict
- DISMISSED: no observable lifecycle mismatch. The option is
  convention-faithful at every stage (negative rejected, clamp
  documented-by-test, garbage->0 by the atoi convention, single
  consumption, clean start/stop, no persistence, no duplicate
  scheduling).
- Borderline noted, within convention: silent clamp (>16) and silent
  garbage-to-disable — both consistent with the help text and the
  tree-wide input handling; not defects.

### Exact commands
- reads: init.cpp:541, chainstatemanager_args.cpp:63-68,
  validation.cpp:1884-1894/1969/5893, kernel/chainstatemanager_opts.h,
  validation.h:93, util/threadpool.h:105-117, common/args.cpp:555-560,
  util/strencodings.h:106-127.

### Limitations
- Thread-count = 8 default on a 4-core host (oversubscription of the
  prevout pool) is a perf question, not lifecycle — out of scope here
  (#21/#23 profiling territory).
- Runtime RPC observation (getchainstates/getblockchaininfo exposure
  of the setting) — none exists; consistent with similar perf options.

### Next queue for this campaign
- -capturemessages lifecycle (#36 c1 touched its empty-payload write
  path): registration/parse/persistence + file lifecycle
  (rotation/truncation on restart).
- -v2transport restart persistence (settings.json round-trip).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): -capturemessages file lifecycle — append-across-restart verified; capture IO failure aborts node (upstream-identical)

### Draw
Random draw over the 20-goal eligible pool (14 pending + 6 CYCLE-1,
#1 excluded as just-cycled): raw=3880495123191271155, index 15 ->
#43 (second cycle; c1 queue cell "-capturemessages lifecycle").
Branch: audit/option-lifecycle-c2 from 669bdf57fb (#1 c2 bookkeeping;
lineage anchor audit/resurrection @ 5d0155254c). Journal file pulled
forward from 331048ba1e (c1, only on audit/option-lifecycle).

### Option plumbing (verified by read)
- Registration: init.cpp:687, DEBUG_ONLY (hidden) flag; consumed at
  init.cpp:2141 -> CConnman::Options::m_capture_messages
  (net.h:1125/1163). Command-line only; not persisted to
  settings.json (same convention as c1's finding).
- Write path: CaptureMessageToFile (net.cpp:4296-4330) — per-message
  fopen("ab")/fclose into <datadir_net>/message_capture/<addr_port>/
  msgs_{recv,sent}.dat; record = 8-byte time + 12-byte type + 4-byte
  length + payload. Call sites: incoming net_processing.cpp:5272
  (BEFORE the try around ProcessMessage), outgoing net.cpp:4183.
- Upstream parity: raw fetch of bitcoin/bitcoin master today shows
  the identical call-site placement (master net_processing.cpp:5435,
  CaptureMessage before the try) and the identical AutoFile
  null-handle throw (master streams.cpp:97).

### Phase A — restart file lifecycle: APPEND, prefix intact (PROVEN)
Hypothesis: same-peer capture file persists across restart (fopen
"ab") rather than being truncated/rotated. Experiment (functional
framework, regtest, -capturemessages, script /tmp/btc43_lifecycle.py):
outbound p2p connection (fixed p2p_port -> same peer addr), record
msgs_sent.dat, restart_node (args preserved), reconnect same addr.
Result: 488 -> 976 bytes, first 488 bytes byte-identical
("PHASE-A-OK append-across-restart: 488 -> 976 bytes, old prefix
intact"). No rotation, no truncation; the appended stream stays
structurally parseable (same record framing; existing
p2p_message_capture.py mini_parser validates framing).

### Phase B — capture IO failure: node ABORTS (PROVEN)
Hypothesis: a failed capture write throws out of the message-handler
thread and kills the node. Mechanism by read: fopen failure -> null
FILE* -> AutoFile::write throws ios_base::failure("file handle is
nullptr") (streams.cpp:103); call site at net_processing.cpp:5272 is
outside any try; ProcessMessages -> ThreadMessageHandler (no catch)
-> util::TraceThread logs + RETHROWS (util/thread.cpp:15-28) ->
std::terminate. Experiment: as root, chmod is bypassed
(CAP_DAC_OVERRIDE), so the peer's msgs_recv.dat was replaced by a
DIRECTORY (fopen("ab") on a dir fails EISDIR even for root); one
further inbound ping then sufficed. Result: node died rc=-6
(SIGABRT); debug.log tail:
  EXCEPTION: NSt8ios_base7failureB5cxx11E
  AutoFile::write: file handle is nullptr: iostream error
  bitcoin in msghand
("PHASE-B node died, returncode=-6"; "PHASE-B-OK ..."). Outgoing
site (net.cpp:4183) is identically unprotected (by inspection).
Timeline note: two framework setup pings at :09.293 captured fine
pre-replacement; the first post-replacement inbound message threw at
:09.345 — consistent with capture-precedes-ProcessMessage ordering.

### Verdict
- CONFIRMED behavior, NOT a local defect: (1) append-across-restart
  is the actual lifecycle (undocumented but parser-compatible);
  (2) any capture IO failure (disk full, EACCES, EISDIR, EMFILE)
  aborts the node via an uncaught exception in the msghand thread.
  Master-relative severity: NONE — code paths are byte-identical to
  upstream master today (call sites + AutoFile throw); trigger
  requires a DEBUG_ONLY option plus a local filesystem fault, and
  fail-loud is a defensible design for a capture tool (silent capture
  gaps would be worse). No code change made; behavior change without
  an independently supported contract would be unendorsed churn.
  Journal-only cycle; reproducer preserved (/tmp/btc43_lifecycle.py,
  2-phase, green end-to-end; runs in ~20s).
- Why existing tests miss it: p2p_message_capture.py covers only the
  happy path (handshake capture + framing), no restart and no fault
  injection.

### Exact commands
- python3 /tmp/btc43_lifecycle.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc43 (scratch, deleted post-run)
- reads: init.cpp:687/2141, net.h:1125/1163, net.cpp:4182-4184/
  4296-4336, net_processing.cpp:5271-5276, streams.cpp:101-109,
  util/thread.cpp:15-28
- upstream parity: curl raw.githubusercontent.com/bitcoin/bitcoin/
  master/src/{net_processing.cpp,streams.cpp}

### Limitations / queue for cycle 3
- Only the EISDIR/fopen-null path was executed; create_directories
  failure (fs::filesystem_error) propagates identically by
  inspection, not by experiment.
- -v2transport settings.json round-trip cell still open.
- If ever upstreamed: a one-line try/catch + LogWarning around both
  CaptureMessage call sites is the obvious candidate patch; NOT
  applied locally (upstream-design question).

## Cycle 3 (2026-07-29): -v2transport settings.json lifecycle — honored, persisted, CLI-overridable (all three proven)

### Draw
Re-rank draw over the rebuilt 8-cell queue:
raw=6417013600593057308, index 4 -> #43 (third cycle; c1 queue cell
"-v2transport restart persistence (settings.json round-trip)").
Branch: audit/option-lifecycle-c3 from 9e1ed9a2b5 (#106 c3
bookkeeping).

### Mechanism (read)
common/init.cpp:98-114 reads settings.json at startup
(ReadSettingsFile) then normalizes it back (WriteSettingsFile);
args precedence: command line > settings.json > defaults
(common/args.cpp). DEFAULT_V2_TRANSPORT=true (net.h:101).

### Experiment (functional, /tmp/btc43_v2settings.py)
Observable: the NODE_P2P_V2 service bit (1<<11) in the node's own
version message (unambiguous — the framework's v2 attempt falls
back to v1 silently, so a handshake-success probe can't
distinguish; recorded as a probe-design lesson).
- settings.json {"v2transport": false}: node advertises
  NODE_P2P_V2=false (v1 works); startup write-back PRESERVES the
  manual entry (not clobbered).
- Restart: still false (persistence round-trip).
- -v2transport=1 on the command line over the same settings.json:
  advertises true (CLI precedence).
"Tests successful" end to end.

### Verdict
DISMISSED: the -v2transport lifecycle is convention-faithful at
every stage (settings honored, normalized-but-preserved, restart-
stable, CLI-overridable). No defect.

### Exact commands
- python3 /tmp/btc43_v2settings.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc43v
- reads: common/init.cpp:98-114, net.h:101, init.cpp:598/1015,
  test_node.py:713-733

### Limitations / queue
- -nosettings behavior (settings ignored entirely) not exercised
  (trivial by code read).
- GUI rw_settings write paths are qt-only (out of scope).
- Campaign cells remaining: none queued beyond this; lifecycle
  surface of the tracked options is now covered (-prevoutfetchthreads
  c1, -capturemessages c2, -v2transport c3).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-08-02, draw 203, raw=5711495130052899885 (63-bit), idx 15/30): deprecated -limitancestorcount/-limitdescendantcount lifecycle — documentation ACCURATE (wallet-only reporting, cluster-based acceptance); DISMISSED

### Hypothesis
The deprecated options' help text ('replaced by cluster limits...
and only used by wallet for coin selection', init.cpp:677/683)
might contradict behavior if the values still constrained mempool
acceptance.

### Trace (full lifecycle)
- Read: node/mempool_args.cpp:39/41 — still applied into
  MemPoolLimits.ancestor_count/descendant_count.
- Consumers (grep census): exactly ONE — node/interfaces.cpp:730
  getPackageLimits, which reports the values to the WALLET
  (coin selection's package-limit awareness).
- Acceptance path: CheckPolicyLimits is cluster-based
  (interfaces.cpp:736-741, error 'too many unconfirmed
  transactions in cluster'); txmempool's CalculateAncestorData
  (:1057-1068) computes stats for reporting, not gating.
- So: deprecated-for-acceptance (replaced by -limitclustercount/
  -limitclustersize), still honored for wallet coin selection —
  exactly what the help text claims.

### Verdict
DISMISSED: the deprecation lifecycle is coherent and the
documentation accurate at every stage. No defect.

### Exact commands
- sed mempool_args.cpp:25-45; grep ancestor_count census;
  sed interfaces.cpp:720-741, txmempool.cpp:1057-1068.

### Limitations / queue
- Behavioral probe (setting =5 and watching coin selection) not
  run — wallet-side effect, and the plumbing is single-consumer
  (census is the evidence).
- Campaign cells: -prevoutfetchthreads c1, -capturemessages c2,
  -v2transport c3, deprecated ancestors c4 — surface covered.
