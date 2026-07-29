# Knowledge recipes (campaign #90)

Synthesized from locally verified findings, 2026-07-27..28 (cycles across
30 campaigns; every recipe links its primary evidence). Keyed by
subsystem/trigger. Stale/version-limited rules are marked.

## wallet/rescan
R1. When touching ScanForWalletTransactions exit paths: test the
    ScanResult status contract, not just the happy path. The
    mid-scan-reorg branch (block goes inactive between the nextBlock
    check and the block's own iteration) is only reachable with a mock
    chain; read-failure is reachable by corrupting an obfuscated blk
    file (locate blocks via XOR-key periodicity; genesis prefix gives
    the key). Log claims must match status — "completed" on FAILURE
    was a real defect (fe6c1c4339). (#30 c4, #10 c1)
R2. Smart-time monotonicity on rescan comes from GetBlockTimeMax, not
    block nTime; a mutation to blocktime breaks it. Test with
    setmocktime chains whose block times decrease at one step
    (28abead3a4). (#41 c1)
R3. Functional framework: addr() descriptors need
    disable_private_keys=True; importdescriptors returns per-item
    success objects — always inspect them; assert_debug_log only scans
    bytes appended INSIDE the context. (#30 c1/c4, #41 c1)

## wallet/encryption
R4. Audit commit-point order: any in-memory registration that precedes
    its durable write needs a rollback on the failure path — or, the
    superior shape, reorder the registration after the durable commit
    (l0rinc/wallet-encryption-write-failures RunWithinTxn). ChangeWallet
    Passphrase already rolls back; EncryptWallet missed it
    (9894fb8b6c). Fault-inject via g_mockable_fail_writes. (#38 c1, #65 c1)

## mempool/txgraph
R5. Any ungated per-item graph walk (GetAncestors/GetDescendants/
    IsConnected per tx or chunk) in production costs double-digit
    percent on reorg/add-heavy paths. Gate with
    `if constexpr (G_ABORT_ON_FAILED_ASSUME)` (the HasDescendants
    idiom); measure ComplexMemPool / MemPoolAddTransactions A/B,
    interleaved, before/after (83f9989a68, -32%). Prime suspects must
    be rejected by measurement — the 81-line commit was clean; the
    small one regressed (#23 c1, #25 c1).
R6. mapTx entries are TxGraph::Ref; ~Ref deregisters from BOTH main
    and staging graphs. Node-handle splices (ChangeSet::Apply) are the
    load-bearing detail keeping graph pointers valid. (#57 c1)

## kernel/C-API
R7. btck_*_destroy must be NULL-tolerant (free()-style). Check the
    whole family for the odd one out before relying on it
    (55f1fa334f). Document assert-only preconditions with @pre naming
    the matching count function (b6b48987a5, 8b0e92b4a2). (#16 c1-c3)

## rpc/logging
R8. Attacker-controlled strings into logs: sanitize at the source;
    BCLog::LogEscapeMessage passes '\n' through by design, so the
    chokepoint for prefixed multi-line values is input validation
    (4c3829c9aa, 19c7dc6233). Inbound-peer log amplification is
    demoted by design (inbound=DEBUG, outbound=INFO) — check design
    before claiming flood risk (#30 c1-c3).

## measurement
R9. Whole-process perf of nanobench binaries is setup-contaminated
    (bench tx-creation appeared as "sha512 hotspot"). Attribute setup
    vs timed region before claiming a hotspot (#23 c1). Bisect with
    interleaved A/B medians and require adjacent-commit confirmation
    before naming first-bad (#25 c1).
R10. RSS-vs-accounting: getmempoolinfo.usage tracks RSS within ~1.13x
    at 8k entries; no malloc_trim anywhere — post-drain retention is
    glibc arenas, verify "leak" suspicions with a restart control
    (2ef390de05). (#74 c1)

## fuzz-harness
R11. Consume control flags BEFORE data loops (short inputs starve
    trailing flags: 2000/2000 all-SUCCESS proof); SetMockTime before
    wallet construction (check_globals watchdog); synthetic CBlocks
    need nonzero nBits (IsNull = failed read); reserve() clears
    pre-existing abort requests; m_keypool_size=1 for ~10x wallet
    iteration speedup (df87300ffc). (#10 c1)

## test-oracles
R12. Malformed-tree/parser negatives: a packed byte is 8 flag bits —
     bit-budget fixtures need 8-tx trees; verify WHICH branch a
     fixture trips with branch-level coverage, not just test pass
     (32d5d1dcc4). Exact-duplicate claims require checking interleaved
     case-specific checks (burn check inside a decode loop) before
     extracting a helper (4f97fbfe1e). (#34 c1, #58 c1)

## process (fork-specific, version-limited)
R13. DONE in a ledger must name where the fix lives: all 18 confirmed
     campaign fixes were unreachable from the ledger-tip lineage
     (#66 c1). Verify fix content on the tip, not just merge-base.
R14. origin/master here is a depth-1 shallow fetch: origin/master..HEAD
     counts the whole tree. Fork-authored code = author+path filter
     (#49 c1).

## Validation of recipes (dry reviews, this cycle)
- R5 applied to addrman/blockstorage/validation/txmempool scan for
  ungated per-item verification loops: recovered the known unfixed
  instance (txmempool.cpp:132 UpdateTransactionsFromBlock — consistent
  with R13, fix on audit/perf-flamegraph), no false positives
  elsewhere (the two other Assume-gated sites already follow R5).
  RECIPE VALIDATED.
- R1's "completion claims on failure paths" applied to index/snapshot
  load logs: snapshot "loaded N coins" (validation.cpp:6110) is
  post-validation (leftover-coins error returned before) — clean; the
  log-then-FlushSnapshotToDisk ordering was flagged for a second look
  (flush failure handling after the success claim). RECIPE USEFUL,
  no new defect.

## Cycle-2 additions (2026-07-29, from #9/#21/#24/#35/#94 cycles)

R15. sancov "UNCOVERED_FUNC hits: 0" on a small static function with
few call sites is usually an INLINING artifact: the out-of-line
symbol is never called while inlined copies run at the call sites.
Never classify a branch uncovered from UNCOVERED_FUNC alone — confirm
with per-line UNCOVERED_PC on the function's source lines. Single-file
-runs=1 -print_coverage can omit the function entirely; grep fallback
logic is not evidence. (#9 c2)

R16. Plain -reindex on a synced datadir is block-INDEX-only (cheap,
no revalidation); the validation cell needs -reindex-chainstate.
Completion gating must never poll state that is true PRE-wipe
(getblockchaininfo=tip/False matches before the chainstate wipe
starts — naive polls stop the node instantly and measure startup).
Gate on debug.log markers ('Wiping LevelDB' -> final
'UpdateTip.*height=N version'), and distrust gates against a copied
datadir's stale log. /usr/bin/time user-CPU is the gate-independent
metric. (#21 c2)

R17. Direct MiniWallet/TestNode use in this fork's framework needs:
pathlib datadir, Binaries(paths=SimpleNamespace(bitcoind, bitcoin_cli)),
PortSeed.n set BEFORE TestNode, explicit -regtest in extra_args,
-rpcport matching rpc_port(0) for that seed, stdout/stderr subdirs
pre-made, called_by_framework=True on generate() (fork's mining-RPC
guard), and get_utxos(..., mark_as_spent=False) for probes. Wallet-RPC
chain building is 0.1-26 tx/s with silent failure modes (fresh
regtest REQUIRES -fallbackfee); MiniWallet does ~80 tx/s. (#21 c2,
#24 c1)

R18. /proc/PID/io read_bytes/write_bytes MISS buffered writes entirely
(page cache) — use rchar/wchar for logical I/O. Sample the pid from
the node's own -pid file; pgrep-by-cmdline races daemon startup.
(#24 c1)

R19. Mutation testing order that prevents useless batteries: inject
mutants FIRST and record kill/survive, write the oracle only for the
survivors (a battery for already-killed mutants proves nothing). One
survivor class at a time; staged clean/mutation/repaired runs.
ccache makes repeat-content mutation builds ~12s after the first
compile — don't fear header-only mutants. (#35 c1, #48 c1)

R20. Rotation pools must be rebuilt MECHANICALLY from the ledger's
per-cycle table rows, never from prose handoff lists — the prose
drifts (we hit stale CYCLE-2+ entries and missing DONE rows twice).
Reconcile on every pool build: pending + exactly-CYCLE-1, minus
EXHAUSTED/QUEUE-COMPLETE/deferred, minus just-cycled. (#36, #64
pool repairs)

R21. Fuzzing P2P message handlers from an empty corpus: random
12-byte message-type strings are ~2^-80 to guess. A padded NetMsgType
dictionary (-dict, 28 tokens from src/protocol.h) lifts edge coverage
~42% at equal runs; -focus_function="(anonymous
namespace)::Name(sig)" steers further once the fully-qualified name
is given. (#9 c2, #79 c1)

R22. Wallet flags: CWallet::SetWalletFlag persists to the database
IMMEDIATELY (WriteWalletFlag). Any harness that seeds a FLAGS record
and then "sets up" the wallet clobbers its own seed before the loader
runs — the resulting oracle failure looks like a production bug and
isn't one. Read expectations back from the DB after ALL setup, before
asserting. Generalized: distinguish seed state from setup-derived
state in every round-trip oracle. (#10 c2)

## Rotation note
Cycle 2 additions complete; the index stays additive (never prune
validated recipes; mark superseded ones explicitly).
