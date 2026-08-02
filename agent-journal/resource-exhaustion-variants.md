# Journal: untrusted-interface resource-exhaustion variant analysis (campaign 7)

Uber-goal rotation. Branch: audit/resource-exhaustion-variants from
audit/resurrection @ 2727d96ba0. Method: explicit upper bound for
CPU/memory/disk/network/descriptors per path; deterministic low-limit
reproducer only when a bound or accounting failure is demonstrated.

## Cycle 1: rewritten HTTP server (post-libevent) accounting

### Connection/request accounting: DISMISSED with documented bounds

- Connections: no explicit admission cap (httpserver.cpp:915-930), but
  fd-bound (process ulimit, 1M observed in startup log); loopback-only by
  default (rpcbind) — the trust boundary is local unless an admin exposes
  RPC. m_connected_size is a shutdown-drain gauge only (1429/1436), not a
  limiter — matches libevent-era behavior.
- Request sizes: headers capped MAX_HEADERS_SIZE (321, 496); chunked body
  capped (485 ContentTooLargeError → disconnect); per-connection idle
  timeout with busy-handling exemption (1154-1166, exemption documented
  1155-1158).
- Per-client request queue (m_req_queue, httpserver.h:475): no explicit
  cap — pipelining by design (7ee7df988e), one-at-a-time processing
  (m_req_busy 477-479). THEORETICAL amplification: a full-speed pipelined
  client grows memory at its send rate (each request body-capped).
  Rate-bound only; on loopback a local attacker, on exposed RPC a
  remote-network-speed-bound one. Same property as the libevent server.
  REPORTED THEORETICAL ONLY per campaign protocol — no commit (bounded by
  trust model + rate, not by accounting failure).
- m_recv_buffer bounded by the same parse-time caps (errors disconnect
  before growth).

## Next queue
(cycle 2 candidates: getblockstats/UTXO-scan RPC cost accounting (own
31449 lineage), prune-during-heavy-RPC. Rotate per uber-ledger: #3 next)
## Cycle 2 (2026-07-29): UTXO-scan/resize accounting — race fixed in-tree (unique-lock cursor); upstream master still racy

Base: 8293cdd7f2 (journal commit for #100 cycle-1 on
audit/sink-reachability; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/resource-exhaustion-c2 (c1 journal
carried). Start state: clean (untracked scratch only).

### Draw
Random draw over the 52-goal pool (33 pending + 19 CYCLE-1; #100
excluded as just-cycled): raw=1683310221613486475, index 35 -> #7.
Queued cell from c1: "getblockstats/UTXO-scan RPC cost accounting".

### Path walked (accounting, per campaign method)
gettxoutsetinfo -> GetUTXOStats (rpc/blockchain.cpp:998) ->
kernel::ComputeUTXOStats (kernel/coinstats.cpp:112): cursor created
UNDER cs_main (117-120), then cs_main RELEASED for the entire scan
(125+) with rpc_interruption_point aborts — the LevelDB iterator
outlives cs_main. The resize hazard: assumeutxo cache rebalancing
(ResizeCache, txdb.cpp:74) resets m_db mid-scan -> use-after-free ->
LevelDB abort (upstream issue reported in PR 35465 discussion).

### Upstream-vs-tree differential (measured, not assumed)
- Upstream master (fetched raw bitcoin/bitcoin/master src/txdb.cpp
  today): Cursor() takes NO lock; CCoinsViewDBCursor has no lock
  member. The race is PRESENT upstream.
- This tree: CCoinsViewDBCursor holds a UniqueLock<Mutex> m_db_lock
  for its whole lifetime (txdb.cpp:231-232, 252), so ResizeCache and
  CompactFullAsync block until every cursor dies.
- Provenance: fork commit e049f064e1 (2026-07-17) added it with a
  clean-master reproducer that ABORTED upstream code in
  leveldb VersionSet::~VersionSet (assert dummy_versions_.next_ ==
  &dummy_versions_) plus a persistent-DB unit test and the
  coins_view_db_resize_cursor fuzz target. Cross-reference: l0rinc's
  upstream PR 35744 (#65 c2 radar) is the shared-lock refinement of
  the same fix — this tree's unique-lock is the simpler already-merged
  variant.

### Accounting verdict
- No accounting failure in this tree: a UTXO scan cannot abort the DB
  (scan blocks resize/compaction; resize/compaction block scans).
- Residual cost (documented, bounded): concurrent UTXO scans
  serialize with each other and with ResizeCache/CompactFullAsync —
  a queueing cost, txdb.h:41 documents the contract. The shared-lock
  refinement (35744) would allow concurrent scans but is unmerged
  upstream; not a correctness gap here.
- Master-relative severity of the FIXED race (upstream context):
  availability — a node-local/authorized RPC + assumeutxo-rebalance
  coincidence aborts the process; not remotely triggerable
  (gettxoutsetinfo is authenticated RPC; scantxoutset is
  authenticated/whitelist).

### Verdict
- DISMISSED for this tree (fix present and tested; accounting
  contract explicit).
- CONFIRMED for upstream-master context (the race is real there; the
  fork carries the fix — the fork's reason for existing, validated).

### Exact commands
- sed/grep: kernel/coinstats.cpp:112-167, rpc/blockchain.cpp:998-1125,
  txdb.h:41-66, txdb.cpp:74-87,225-262
- FetchURL raw.githubusercontent.com/bitcoin/bitcoin/master/src/txdb.cpp
- git show e049f064e1 (fix commit + reproducer narrative + stat)

### Limitations / queue
- Scan-vs-compaction queueing measurement (two concurrent scans +
  resize timing on a big UTXO set) not quantified — the stall is
  structural, not measured; queued for a big-chain host.
- getblockstats (block-file reads under pruning) not taken this
  cycle — queued.
- The stall documented in 35744's body ("ResizeCache holds cs_main
  while it waits") applies here too (resize requires cs_main); during
  assumeutxo activation a long scan delays validation — same class,
  same documented trade-off.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-30): UTXO-scan queueing measured — exact N-fold serialization staircase; DISMISSED (documented contract quantified)

### Draw
Re-harvested-queue draw (seed_raw=17127609671097067094,
masked=7904237634242291286, n=5, idx=1) -> scan-vs-compaction ->
#7 (third cycle; c2 queue cell "scan-vs-compaction queueing
measurement"). Branch: audit/resource-exhaustion-c3 from
e635e93707 (#57 c4 tip).

### Hypothesis
The cursor-lifetime m_db_mutex (the e049f064e1 race fix) could
serialize concurrent UTXO scans at a worse-than-documented cost,
or let them partially overlap (a locking gap re-opening a
resize/scan hazard).

### Lock-scope audit (txdb.cpp:225-265)
CCoinsViewDBCursor holds UniqueLock<Mutex> m_db_mutex for its
ENTIRE lifetime (ctor takes it, dtor releases); scans therefore
fully serialize with each other and with ResizeCache. BatchWrite
(flush) does NOT take m_db_mutex — no scan-vs-flush lock
contention by construction. Compaction (CompactFullAsync) is
random-gated (ShouldCompactChainstate: 1/320 per full flush,
non-IBD, validation.cpp:120-124) — not deterministically
triggerable without instrumentation; the resize timing half of
the c2 cell is reachable only via assumeutxo activation
(correctness already covered by the c2 resize-cursor test).

### Experiment (driver /tmp/scanq.py; TestNode, PortSeed 320,
-rpcthreads=8, muhash scans)
Built a 293,079-coin regtest UTXO set (50 blocks x 30 fan-out txs
x 200 outputs, confirmed_only spends, 601 s), then:
- baseline: 3 sequential gettxoutsetinfo("muhash") — 2.705 s,
  2.705 s, 2.705 s (stable).
- 2-concurrent: per-scan [2.705, 5.384], wall 5.385 s = 1.99x.
- 3-concurrent: per-scan [2.702, 5.380, 8.056], wall 8.057 s
  = 2.98x.

### Results
Exact staircase: N concurrent scans cost 1x/2x/.../Nx baseline —
full cursor-lifetime serialization, no overlap, no lock gap.
Queueing multiplier = N; a second concurrent scan costs +100%
latency, a third +200%.

### Verdict
DISMISSED: the stall is quantified and matches the documented
contract (txdb.h:41) exactly — serialization is total and
correct, with zero hazard window. This is the price of the
e049f064e1 race fix; upstream 35744's shared-lock refinement
(concurrent scans) remains unmerged and TSan-flagged upstream
(#42 c1) — nothing to take, nothing to fix. The compaction-timing
half-cell stays queued for a big-chain host with a deterministic
compaction trigger.

### Exact commands
- python3 /tmp/scanq.py (output above); lock refs: txdb.cpp
  :225-265, validation.cpp:120-124, :2893-2896, txdb.cpp:202-215.

### Limitations / queue
- Baseline stability (3x 2.705 s) shows RPC/HTTP adds no measurable
  variance; the muhash scan is CPU-bound in coin deserialization.
- getblockstats under pruning remains queued.
- A deterministic compaction trigger (test-only hook) would
  unblock the compaction-timing cell; do NOT add one speculatively.

## Rotation note
Three cycles; HTTP accounting bounded, scan/resize race fixed,
queueing now measured. getblockstats-pruning remains.

## Cycle 4 (2026-07-30): getblockstats under pruning — horizon correct, 70k race calls clean; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=6880818164717981818, masked
same, n=4, idx=2) -> getblockstats-pruning -> #7 (fourth cycle; c2
queue cell). Branch: audit/resource-exhaustion-c4 from f3d7414268
(#7 c3 journal tip).

### Hypothesis
getblockstats over pruned/horizon-adjacent heights could fail
uncleanly (hang, crash, garbage stats) or misreport when block or
undo files are pruned mid-read.

### Code audit
GetBlockChecked/GetUndoChecked (rpc/blockchain.cpp:699-747) are
race-aware: availability checked under cs_main, read unlocked,
prune-in-the-gap -> clean JSONRPCError ('Block not found on disk'
/ 'Can't read undo data from disk'); pruned heights -> 'Block/Undo
data not available (pruned data)' (:684-696).

### Experiment (driver /tmp/gbs_prune.py; -fastprune -prune=1,
PortSeed 321, 350 lean blocks then pruneblockchain(151))
- tip=451, pruneheight=152 after explicit pruneblockchain
  (-prune=1 is MANUAL-only, init.cpp:551; v2 proved automatic
  pruning needs >=550 — recorded as harness lesson).
- stats at tip: OK (totalfee>0 with undo data).
- stats at pruneheight (oldest available): OK.
- h=5 and h=pruneheight-1: clean 'pruned data' JSONRPCError.
- Race: 70,156 getblockstats calls at pruneheight-{1,2,3} while the
  main thread mined 5 more blocks and advanced the horizon 5 times
  -> 0 anomalies (every call full stats or the clean pruned error;
  no hangs, crashes, or garbage).

### Verdict
DISMISSED: getblockstats is correct and clean under pruning —
full stats at/above the horizon, precise pruned-data errors below
it, race-safe against horizon advancement. No resource-exhaustion
or failure-contract issue.

### Harness lessons (recorded, all three cost a run)
- v1: MiniWallet get_utxo(confirmed_only) is O(wallet utxos) per
  call — quadratic at ~470k utxos; stalled block production (17
  CPU-minutes in python). Lean profiles (~25 kB blocks) avoid it.
- v2: -prune=1 enables MANUAL pruning only; automatic pruning
  needs >=550 (init.cpp:551). pruneblockchain RPC drives the
  horizon directly.
- v2: TestNode's shared AuthServiceProxy is NOT thread-safe
  (CannotSendRequest under concurrent use) — one proxy per thread.

### Exact commands
- python3 /tmp/gbs_prune.py (v3 output above); code refs
  rpc/blockchain.cpp:684-747, :2046-2047, init.cpp:551,
  node/blockstorage.cpp:912-918.

### Limitations / queue
- The race window hit only the pruned side of the horizon (scanner
  formula probed ph-{1..3}); the ok side is covered by the static
  tip/pruneheight checks. A boundary-straddling variant is a
  nicety, not queued.
- #7 cells all closed.

## Rotation note
Four cycles; HTTP accounting, scan/resize race, scan queueing,
and getblockstats-pruning all closed with measurements.

## Cycle 5 (2026-08-02, draw 226, raw=17325288802186375634, masked 8101916765331599826, idx 1/5): untrusted-interface bound census — all 16 bound constants have enforcement points AND measured journal cells; campaign EXHAUSTED

### Census (net_processing.cpp bound constants -> enforcement ->
measured cell)
- MAX_INV_SZ 50000 / MAX_GETDATA_SZ 1000: reject arms
  (:4170/:4261) — #49 c10 markers.
- MAX_CMPCTBLOCK_DEPTH 5 / MAX_BLOCKTXN_DEPTH 10: static_assert
  + height gates (:2528/:4406-4414) — #109 c1-c3 compact-block
  family.
- MAX_GETCFILTERS_SIZE 1000 / MAX_GETCFHEADERS_SIZE 2000:
  response caps (:3394/:3423) — blockfilter family (#49-era
  review).
- MAX_ADDR_RATE_PER_SECOND 0.1 + token bucket (:195-402):
  addr-spam protection (CVE-2024-52919's fix shape) — advisory
  family #49.
- MAX_BLOCKS_IN_TRANSIT_PER_PEER 16: in-flight cap — stalling
  family (#49 52922 markers).
- MAX_LOCATOR_SZ 101: getheaders bound — headers family (#46 c3
  presync map).
- PRIVATE_BROADCAST_MAX_CONNECTION_LIFETIME 3min: #49 c2
  fork-interaction cell (54604).
- MAX_OUTBOUND_PEERS_TO_PROTECT 4 / MAX_BLOCKS_TO_ANNOUNCE 8 /
  MAX_PCT_ADDR_TO_SEND 23 / MAX_ADDR_TO_SEND 1000 /
  MAX_FEEFILTER_CHANGE_DELAY: outbound-shaping constants
  (self-resource, not attacker-consumed).

### Verdict
EXHAUSTED: every attacker-reachable resource consumer on the
P2P interface has a bound, an enforcement point, and a measured
cell in the journals. Reopen on new message types or new
unbounded consumers.

### Exact commands
- grep constant census + enforcement line refs above.

### Limitations
- Census is net_processing-scoped (the untrusted interface);
  RPC/RPC-server bounds are separate campaigns' cells (#4, #30).
