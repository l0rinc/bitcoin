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
