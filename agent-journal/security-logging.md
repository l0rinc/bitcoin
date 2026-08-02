# Campaign #30 — security-logging

Base: audit/resurrection @ f460bc0218 (rotation ledger commit for #61).
Branch: audit/security-logging. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-27): RPC whitelist-rejection logs reflect raw client strings — CONFIRMED + FIXED

### Scope chosen
Value class: attacker-controlled strings into logs (the class with the
strongest exploitability). Surveyed surfaces first:
- net_processing/net.cpp: all peer-controlled strings (msg_type, subver,
  feature_id) already SanitizeString'd (10 sites, e.g.
  net_processing.cpp:3644, 5173). REJECT-message handling: removed upstream.
- HTTP server (httpserver.cpp): URI logged only on internal exception paths
  (lines 178/181) — attacker can't smuggle raw CR/LF through the HTTP
  request-line parser; control-char risk limited to escapes on an
  exception path. Parked as weaker.
- RPC (httprpc.cpp): method name reflected RAW at WARNING on the
  whitelist-rejection paths (lines 118, 147). CHOSEN.

### Hypothesis / trust boundary
`-rpcwhitelist=user:methods` exists to give LESS-TRUSTED consumers limited
RPC access. Such a consumer is authenticated but inside the threat model.
Its request "method" string is fully client-controlled; JSON escape
sequences (e.g. `\n`) are decoded by UniValue before the string reaches
LogWarning, so control characters land raw in debug.log.

### Mechanism (confirmed by reading + repro)
httprpc.cpp `ExecuteHTTPRPC`: on 403 (method not in the user's whitelist)
`LogWarning("RPC User %s not allowed to call method %s", ..., jreq.strMethod)`
— singleton path (line 118) and batch path (line 147). No sanitization.
authUser is operator-controlled (must match a configured rpcuser/rpcauth
entry to authenticate) — left as-is deliberately.

### Reproduction (failing-before / passing-after)
Extended test/functional/rpc_whitelist.py with test_log_injection: user1
(restricted) calls method `getblock\nFORGED LOG LINE` (JSON-escaped
newline), gets 403, and the log is asserted to contain the sanitized
rendering and NOT the raw newline.
- BEFORE fix: `AssertionError: Unexpected message "\nFORGED LOG LINE"
  found in log`; debug.log line 519/520 shows the forged standalone line.
  (Also visible: the separate `[rpc] ThreadRPCServer method=...` line at
  rpc/request.cpp:248 already renders the method sanitized — the
  whitelist-rejection path was the inconsistent one.)
- AFTER fix: `Tests successful`; rpc_users.py also green.

### Framework trap found (worth remembering)
`assert_debug_log` only scans bytes appended INSIDE its context manager
(test_node.py:580: prev_size captured at entry). Calling it AFTER the
action makes both expected/unexpected checks vacuous — my first version
"passed" pre-fix for exactly this reason. The RPC call must be inside the
`with` block.

### Fix (commit 19c7dc6233)
SanitizeString (SAFE_CHARS_DEFAULT, strips control chars, keeps spaces and
`.,;-_/:?@()`) on the method name at both 403 call sites. Whitelist
decision still uses the exact string; only log rendering changes. Matches
the net_processing convention. Correlation preserved: sanitized method
name still identifies what was attempted (same rendering as the
rpc/request.cpp execution log).

### Verdict
- CONFIRMED log-injection (severity: low-moderate; requires valid
  restricted RPC credentials). FIXED with regression test.
- Two verifier forms: functional failing-before/passing-after +
  adjacent-path consistency check (rpc/request.cpp already sanitized).

### Exact commands
- `cmake --build build-before -j4 --target bitcoind`
- `python3 test/functional/rpc_whitelist.py --configfile=build-before/test/config.ini --tmpdir=/tmp/btc_func_tmp [--nocleanup]`
- `python3 test/functional/rpc_users.py --configfile=...` -> Tests successful

### Limitations / leads
- Not audited this cycle: wallet label/descriptor strings in wallet logs,
  torcontrol/PCP replies, `-debug` category rates (log-flood amplification
  via unauthenticated P2P at non-default categories), misleading-state
  claims. Queued below.
- httpserver URI-on-exception path (178/181): consider SanitizeString with
  SAFE_CHARS_URI as belt-and-braces — left unfixed (no proven raw CR/LF
  route through the request parser; exception trigger unclear).

### Next queue for this campaign
- Amplification: enumerate WARNING/INFO logs reachable by unauthenticated
  peers per connection (bounded by discouragement?) — volume proof.
- Misleading claims: spot-check "success" logs on partial-failure paths
  (e.g. importmulti/importdescriptors partial results).

## Cycle 2 (2026-07-28): wallet names inject raw newlines into INFO logs — CONFIRMED + FIXED at validation

### Hypothesis / trust boundary
Wallet name is RPC-user-controlled (createwallet/restorewallet; delegable
via -rpcwhitelist to a less-trusted consumer) and reaches INFO-level logs
raw: WalletLogPrintf prefixes every line with the name (wallet.h:949) and
sites like wallet.cpp:249 pass it as a format argument.

### Key mechanism discovered
The logging layer DOES escape control characters —
BCLog::LogEscapeMessage (logging.cpp:337-348, applied to every message at
line 428) — but deliberately passes '\n' through (line 341), because
intentional multi-line log messages exist. Therefore newline injection is
only preventable at the SOURCE (user-controlled strings), exactly the
net_processing convention and the cycle-1 RPC fix.

### Reproduction (failing-before, scratch regtest)
- `createwallet 'inj\nFORGED INFO LINE'` -> success (control chars are
  legal in Linux filenames; GetWalletPath blocks only path tricks).
- debug.log lines 119+: standalone `FORGED INFO LINE] Setting spkMan to
  active: id = ...` — forged INFO log lines.

### Fix (commit 4c3829c9aa)
Validation at the two creation entry points (CreateWallet, RestoreWallet):
reject names containing chars < 0x20 or 0x7f via a 3-line HasControlChars
helper, existing FAILED_NEW_UNNAMED status (RPC_INVALID_PARAMETER, -8,
matching the empty-name convention). Load paths for pre-existing wallets
unchanged (documented). Chosen over per-site sanitizing: one chokepoint
prefix + N argument sites can't all be intercepted, and newline directory
names are a footgun regardless of logging.

### Verification (passing-after)
- wallet_createwallet.py + new rejection case -> Tests successful.
- restorewallet 'evil\nFORGED' -> -8 same message; goodrestore -> success.
- Pre-existing normal wallets unaffected (create/load/restore exercised).

### Verdict
- CONFIRMED log injection (severity: low; requires wallet-creation-capable
  RPC credentials). FIXED at validation with functional regression test.
- Two verifier forms: pre-fix injection repro + post-fix rejection tests
  on both RPC paths.

### Leads
- Address labels: flow into RPC responses/errors (JSON-escaped, safe) but
  also into some WalletLogPrintf paths? Spot-checked: no label logging at
  INFO found this cycle (labels mostly silent). Low priority now.
- Descriptor strings: wallet.cpp:3832 logs descriptor ToString() at INFO —
  parser-normalized output (no raw user input, no secrets — ToString()
  renders xpubs only). DISMISSED.

### Next queue for this campaign
- Misleading claims: "success" logs on partial-failure paths.

## Cycle 3 (2026-07-28): unauthenticated-peer log amplification — DISMISSED by design (volume proof)

### Question
Can an unauthenticated peer force INFO/WARNING log output (disk-flood
amplification) per connection or per reconnect cycle?

### Design facts (code)
- Misbehavior logging is DEBUG-only: PeerManagerImpl::Misbehaving uses
  LogDebug(BCLog::NET, ...) (net_processing.cpp:1949) — invisible at
  default logging.
- Inbound new-peer messages are deliberately demoted: "Log successful
  connections unconditionally for outbound, but not for inbound as those
  can be triggered by an attacker at high rate"
  (net_processing.cpp:3907-3912) — inbound: LogDebug; outbound: LogInfo.
- Remaining INFO/WARNING P2P lines are outbound-specific (insufficient
  work, stale tip), self-connect, or block-stall disconnects — not
  reachable by an inbound garbage peer.
- Addr-message rate limiting is token-bucket on processing
  (net_processing.cpp:5826-5837), and it fails silently at default level.

### Volume proof (scratch functional probe, /tmp/amp_probe.py, removed after)
Battery per round: version handshake + 40 getaddr + 1 invalid sendcmpct
(announce=2, triggers misbehavior + disconnect); 5 reconnect rounds.
- Production default (-debug=0): bytes_added=0, info_lines_added=0,
  warn_lines_added=0. ZERO — the attack is invisible.
- Framework debug mode (-debug, all categories): bytes_added=73,065
  (~14.6 KB/round: handshake traces, message-processing lines, 5
  Misbehaving lines). Framework nodes run with -debug by default
  (test_node.py:144) — noted so future probes don't mistake debug-mode
  output for production behavior.

### Verdict
- DISMISSED: no default-level amplification; the design is deliberately
  amplification-proof (inbound demotion + DEBUG misbehavior). Enabling
  -debug=net accepts ~15 KB per hostile connection cycle with unbounded
  reconnects — an explicit operator choice, and debug logging is
  documented as verbose. No rate limiting on DEBUG P2P logs exists
  (LogRateLimiter is not used in net_processing/net) — recorded as a
  contextual note, not a defect.

### Exact commands
- `python3 /tmp/amp_probe.py --configfile=build-before/test/config.ini
  --tmpdir=/tmp/btc_amp [--nocleanup]` (framework debug)
- `PROD_LOG=1 python3 /tmp/amp_probe.py ... --tmpdir=/tmp/btc_amp_prod`
  (-debug=0)

### Next queue for this campaign
- Misleading claims: "success" logs on partial-failure paths
  (importdescriptors partial results).

## Rotation note
Three bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 4 (2026-07-28): "Rescan completed" logged on FAILURE path — CONFIRMED + FIXED (fe6c1c4339)

### Draw
Random draw over the 70-goal eligible pool (pending + CYCLE-1, DONE and
prereq-missing 72/77 excluded, #21 excluded as just-cycled):
seed=2440153803534145668 (od -N8 /dev/urandom), index 18 -> #30. This is
the queued cycle-4 hypothesis from cycle 3's next queue.

### Hypothesis / trust boundary
Misleading-claims class: a log message misdescribes actual state on a
partial-failure path. The queued importdescriptors angle dissolved on
inspection (wallet RPC import paths log nothing at all; per-item
success/error lives in the RPC response), so the survey moved to where
failure paths DO log: CWallet::ScanForWalletTransactions exit logging.

### Mechanism (code proof)
wallet.cpp tail of ScanForWalletTransactions: exit branches covered
fAbortRescan and shutdownRequested, everything else printed
"Rescan completed in Xms" — including result.status == FAILURE
(block data unreadable: wallet.cpp:2021-2025; mid-scan reorg:
2002-2007). The wallet.h:649 doc even says last_failed_block "will be
set if status is FAILURE" — the log claimed completion anyway.

### Reachability / callers
- rescanblockchain (rpc/transactions.cpp:913): hasBlocks pre-check
  (node/interfaces.cpp:650) only tests BLOCK_HAVE_DATA index flags, so
  on-disk corruption/loss past the startup VerifyDB window
  (DEFAULT_CHECKBLOCKS=6 recent blocks) is only discovered mid-scan.
  RPC still errors ("Rescan failed. Potentially corrupted data files.")
  — the log alone lies.
- AttachChain startup rescan (wallet.cpp:3363) and RescanFromTime
  (importdescriptors) share the same exit logging.
Severity: low. No acceptance/funds impact; operator-facing misdiagnosis
on a recovery path where the log is the primary diagnostic.

### Reproduction (failing-before, deterministic fault injection)
Scratch probe /tmp/r30_rescan_probe.py: 250-block regtest chain, corrupt
the merkle-root region of block 100 inside the XOR-obfuscated
blk00000.dat (obfuscation discovered live — blk files are XORed with the
8-byte chainstate key, block located via key periodicity; genesis-prefix
key derivation confirmed). Index flags untouched -> hasBlocks passes ->
scan fails at block 100 -> RPC -1 "Rescan failed" AND debug.log contains
"Rescan completed in". Result: misleading_completed_logged=True.
The corruption survives restart because VerifyDB only re-reads the last
6 blocks (checkblocks default).

### Fix + verification (passing-after)
Commit fe6c1c4339: on ScanResult::FAILURE log
"Rescan failed: unable to scan block <hash>"; "Rescan completed" only
otherwise. Committed regression test
test/functional/wallet_rescan_failure_log.py (registered in
test_runner.py) asserts both directions:
- success rescan: "Rescan completed" present, "Rescan failed" absent;
- corrupted rescan: RPC error + "Rescan failed" present,
  "Rescan completed in" absent in the appended log window.
Run order (clean/mutation/repaired controls, per protocol):
1. test vs UNFIXED build-before binary -> FAILED with
   'Unexpected message "Rescan completed in" found in log' (failing-before);
2. rebuild (cmake --build build-before --target bitcoind) ->
   Tests successful (passing-after);
3. regression battery: wallet_assumeutxo, wallet_rescan_unconfirmed,
   wallet_createwallet functional + test_bitcoin wallet_tests all green.
   (wallet_import_rescan.py no longer exists in this tree — noted, not
   a failure.)

### Framework traps (worth remembering)
- TestNode has start(), the framework has start_node(i);
  node.start_node() silently dispatches to RPC via __getattr__ and dies
  with "no RPC connection".
- skip_if_no_wallet() sets uses_wallet=True; without it the node gets
  -disablewallet and wallet RPCs return -32601.
- With uses_wallet, import_deterministic_coinbase_privkeys creates
  "default_wallet"; a second createwallet makes wallet RPCs ambiguous
  (-19). Set self.wallet_names = ["w"] instead of manual createwallet.

### Why existing tests missed it
No test exercised the FAILURE exit of ScanForWalletTransactions; the
only log assertion was the success path (DebugLogHelper
"Rescan completed", wallet_tests.cpp:878) — still green after the fix.

### Limitations
- The mid-scan-reorg FAILURE branch shares the exit log and is fixed by
  the same condition but not separately exercised (racy to drive
  deterministically).
- Fault injection models corruption/data loss, not a live prune race
  (hasBlocks-check-to-scan window) — same code path, unproven timing.

### Next queue for this campaign
- AttachChain/init-time rescan failure: operator sees init error
  "Failed to rescan the wallet during initialization" — check surrounding
  log context for misleading "completed" siblings on other recovery
  paths (index rebuilds, assumeutxo load).
- httpserver URI-on-exception belt-and-braces (parked since cycle 1).

## Rotation note
Four bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 5 (2026-08-02, draw 207, raw=4599885279918200254 (63-bit), idx 8/26): httpserver URI-on-exception — safe by design (no Core endpoint accepts secrets via URI; body/auth never logged); DISMISSED

### Question (parked since c1)
On an RPC/REST handler exception, httpserver.cpp:178/181 logs
req->GetURI() at WARNING level — could the URI carry secrets
into the log?

### Evidence
- RPC: all calls POST to target '/' with JSON in the BODY; the
  URI is constant and secret-free. Auth is the Authorization
  header — the failure path (httprpc.cpp:216-217) logs ONLY
  peerAddr, never the header/credentials.
- REST (rest.cpp): URI paths carry block/tx HASHES and heights
  only — public chain data, no secret class.
- The parse-error arms (:1097-1118) log origin/id/e.what() —
  e.what() is the parser's own message, not client data.
- The exception reply body (:183-186) echoes e.what() to the
  AUTHENTICATED caller only (same trust domain).

### Verdict
DISMISSED: the URI-on-exception log is safe by construction —
no Core endpoint accepts secrets via URI, and the body/auth
channels are never logged. Belt-and-braces suppression would
defend nothing; parked cell closed.

### Exact commands
- sed httpserver.cpp:170-195, 1094-1118; httprpc.cpp:213-223.

### Limitations / queue
- Third-party handlers registered via RegisterHTTPHandler
  (none in-tree) would inherit the same log line — contract
  note, not a current leak.
- AttachChain/init-time rescan context remains queued (c4 note).
