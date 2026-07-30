# Campaign #74 — memory-pressure-allocator

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/memory-pressure. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): mempool fill/drain — accounting vs RSS, in-process memory return

### Draw
Random draw over the 62-goal eligible pool: raw=16905212290020889489,
index 49 -> #74.

### Workload / environment
Isolated scratch regtest node (build-before Release, asserts on),
-persistmempool=0, MiniWallet self-transfer chains: 160 chains x 50 txs
= 8000 txs (chain length 50 stays under cluster-count limits; a 500-tx
chain was rejected with too-large-cluster (-26) — recorded as the limit
in action). 1-in-1-out txs, ~110 vbytes each. RSS via /proc/pid/status
VmRSS sampled at baseline, every 10 chains, peak, post-drain, post-restart.
Host: Cortex-A76, 15GB RAM, glibc allocator (no malloc_trim/mallopt
anywhere in production code — verified by grep; lockedpool arenas are
secure-memory only).

### Accounting formula (source)
getmempoolinfo.usage = CTxMemPool::DynamicMemoryUsage (txmempool.cpp:916):
mapTx estimate MallocUsage(sizeof(CTxMemPoolEntry) + 9 pointers) per entry
+ DynamicUsage(mapNextTx) + DynamicUsage(txns_randomized) +
m_txgraph->GetMainMemoryUsage() + cachedInnerUsage. The TxGraph
(cluster-linearization) share is included in the RPC number.

### OOM contract (static, out of scope for injection)
Only production bad_alloc site: src/support/allocators/secure.h:31.
No graceful-degradation contract exists in the mempool path — OOM
propagates and terminates by design, so allocation-failure injection is
not contract-permitted here and was not attempted.

### Results (8000 txs, /tmp/r74_probe_result.txt)
| stage | RSS (kB) | note |
|---|---|---|
| baseline | 58,724 | empty mempool |
| peak | 68,056 | 8000 entries, usage=8,487,696 B |
| in-process drain (mined 2 blocks) | 71,964 | usage=0; +3.9 MB mining work on top of retained fill |
| after restart | 58,104 | ~baseline |

- Accounting: 1061 B/entry accounted vs 1194 B/entry RSS delta =>
  RSS/usage ratio 1.13x. getmempoolinfo.usage is a tight estimator of
  actual RSS cost at this scale (includes the TxGraph share).
- Memory return: after in-process drain the process retains 13.2 MB
  over baseline (9.3 MB mempool fill + ~3.9 MB block-assembly/coins
  work). usage reports 0 — no leak; the retention is glibc arena
  behavior (no malloc_trim/mallopt anywhere, by design). Restart
  returns to baseline exactly.

### Verdict
- DISMISSED (no leak, no misaccounting). Accounting is honest (1.13x
  at 8k entries); retained memory post-drain is allocator behavior,
  reusable by the process, not a leak — the campaign's
  allocator-vs-leak distinction resolved by the restart control.

### Exact commands / artifacts
- probe: /tmp/r74_mempool_mem_probe.py (PYTHONPATH=test/functional,
  --configfile=build-before/test/config.ini --tmpdir=/tmp/btc_r74)
- results: /tmp/r74_probe_result.txt

### Limitations
- 8000-entry mempool (~2 MB serialized) — allocator behavior at the
  300 MB default cap not sampled (fill rate ~10 txs/s via RPC is the
  bottleneck; a C++-driver variant is queued).
- glibc-specific retention; no jemalloc/musl comparison.

### Next queue for this campaign
- C++-driver fill (mempool_stress bench + RSS sampler thread) for the
  100k-300k-entry regime; txgraph GetMainMemoryUsage share breakdown.
- RLIMIT_AS crash-behavior check on a scratch node (abort path
  cleanliness, not recovery).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): LockedPool exhaustion contract — oversize secure allocation fails gracefully (fault-injected, node alive)

Base: 447cbeee19 (journal commit for #53 cycle-1 on
audit/timing-channel; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/memory-pressure-c2 (c1 journal carried).
Start state: clean (untracked scratch only).

### Draw
Random draw over the 42-goal pool (25 pending + 17 CYCLE-1; #53
excluded as just-cycled): raw=13070059795069912386, seed masked to 63
bits (3846687758215136578), index 16 -> #74.
STATE NOTE: #74 c1 (2ef390de05, mempool accounting + glibc retention)
lived on audit/memory-pressure without a ledger row; recorded
retroactively in this cycle's update. Draw honored as cycle 2.

### Cell: secure allocator pressure boundaries
LockedPool (support/lockedpool.{h,cpp}): 256 KB arenas, grown on
demand via AllocateLocked (mlock-backed up to the OS limit, unlocked
fallback + LockingFailed callback); single allocations > ARENA_SIZE
(256 KB) return nullptr; secure_allocator (secure.h:27-34) converts
nullptr to std::bad_alloc. Arena count has no explicit cap — growth
is usage-driven and reclaimed on deallocate.

### Fault injection (isolated scratch wallet)
1. regtest node, createwallet + encryptwallet (short pass).
2. RPC walletpassphrase with a 300,000-char passphrase
   (strWalletPass assignment exceeds ARENA_SIZE).
Observed: HTTP 500 {"code":-1,"message":"std::bad_alloc"} — the
RPC dispatcher catches the exception and returns a normal error;
the node stays up (getblockcount after: OK). getmemoryinfo after:
one 256 KB arena, used=400, free=261744, chunks 2/2 — no growth,
no leak from the failed path.

### Verdict
- DISMISSED: the oversize secure-allocation path is graceful
  (bad_alloc -> RPC error), the arena model has no hidden cap or
  wedge, and the failed allocation leaves no residue.
- Note recorded: the error surfaces as a generic std::bad_alloc
  (RPC code -1) rather than a classified "passphrase too long" —
  cosmetic, documented, not a defect worth diverging for.
- Adjacent contract re-confirmed: the passphrase sits in
  request.params[i] unmlocked (secure.h:52 TODO, #0 c2 risk-map
  cell) — known, documented, unchanged.

### Exact commands
- greps/seds: lockedpool.cpp:285-361, secure.h:27-42,
  wallet/rpc/encrypt.cpp:50-70
- node: bitcoind -regtest -datadir=/tmp/btc74_w (createwallet,
  encryptwallet); python3 raw HTTP POST walletpassphrase
  ['A'*300000, 60]; getblockcount; getmemoryinfo

### Limitations / queue
- Sustained arena growth under adversarial long-lived secure usage
  (many distinct keys) not stress-tested — the wallet-scale bound
  (thousands of keys x tens of bytes) makes it theoretical; queued
  behind a real workload.
- dbcache-vs-RSS accounting during tx-heavy sync (rss ~57 MB vs
  450 MB default cache budget) unmeasured this cycle — queued.
- Locked arena mlock-failure path (AllocateLocked without lock +
  callback) not exercised (needs an RLIMIT_MEMLOCK=0 container) —
  queued.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-30): sustained arena growth — high-water bounded, zero runaway; churn and size-alternation recycle fully

### Draw
Re-rank draw over the remaining 4-cell queue:
raw=2928821531056304015, index 3 (of 4) -> #74 (third cycle; c2
queue cell "sustained arena growth under adversarial long-lived
secure usage"). Branch: audit/memory-pressure-c3 from a0a11e0214
(#10 c4 journal tip).

### Experiment (driver /tmp/btc74_arena.cpp, LockedPoolManager
stats API)
- A: 1M rounds of alloc(32)+free with a 1-block live set ->
  total=262144 (ONE chunk; no runaway).
- B: grow live set to 200k distinct 24B blocks ("many keys") ->
  used=6.4MB (block rounds to 32), total=6,553,600 = 25 x 256KB
  chunks, proportional to the live set. Free all -> used=0,
  total UNCHANGED (lifetime retention, by design).
- C: 1M churn rounds again -> total UNCHANGED at 6,553,600
  (freed blocks fully recycle; zero further growth).
- D: 200k alternating alloc(8)/alloc(96) pairs -> total UNCHANGED
  (no fragmentation-driven growth).

### Verdict
DISMISSED: the locked arena is high-water-bounded under
adversarial long-lived secure usage — growth tracks the live-set
peak exactly, churn never regrows it, size alternation causes no
fragmentation growth. Retention until process end is the
documented LockedPool design (stats confirm it plateaus at peak).

### Exact commands
- g++ -O2 -std=c++20 -I src -o /tmp/btc74_arena
  /tmp/btc74_arena.cpp src/support/lockedpool.cpp
  src/support/cleanse.cpp
- /tmp/btc74_arena (stats table above)

### Limitations / queue
- dbcache-vs-RSS accounting during tx-heavy sync (c2 queue)
  unmeasured — queued.
- mlock-failure path (RLIMIT_MEMLOCK=0 container) unexercised —
  queued.

## Rotation note
Three cycles; the growth cell is closed with a measured plateau.
Not exhausted (RSS accounting, mlock-failure).
