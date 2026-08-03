# URGENT — live findings (max 10, severity-ordered)

Updated after every cycle and immediately after any verdict/severity
change. Legend: 🚨 Critical | 🔴 High | 🟠 Medium/correctness-data-loss
risk | 🟡 promising-unverified | ⚪ blocked/inconclusive | ✅ fixed +
independently verified.

## ✅ KDF iteration-count overflow — wallet unlock hang (fixed, audit/kdf-rounds-overflow)
- Mechanism: nDeriveIterations (unsigned, unbounded on wallet-file
  deserialize) narrows into the KDF's signed `int count`; rounds >
  INT_MAX -> negative count passes the `!count` guard -> ~2^31 SHA-512
  rounds per unlock (~2.4h) + signed-overflow UB. Crafted wallet file
  -> wallet-scope DoS. Identical to upstream PR bitcoin#35859.
- Evidence: mechanism probe (guard_accepts=1 while narrowed count =
  -2147483648); passing-after wallet_crypto_tests/passphrase_rounds_limit
  (0/INT_MAX+1/UINT_MAX rejected, DEFAULT accepted).
- Fix: reject rounds > INT_MAX + harden KDF guard !count -> count < 1.
- Next: offer upstream (already covered by #35859 — track its merge).

## ✅ CI external-input pinning completed (PR 35754, 4 arms)
- Mechanism: test pip deps unpinned, actions by floating tags, OCI
  images by tag, lint tool binaries unverified — all mutable between
  runs without a repository change (the F9/F18 family's open surface).
- Evidence: gap greps at HEAD; adoption verified in-tree:
  --require-hashes + 190-hash lockfiles (pycapnp/pyzmq), actions@
  full commits, images @sha256 digests, shellcheck/mlc sha256sum.
- Branch/commits: audit/adopt-ci-pinning (5671b32614, 9bf68d70bc,
  0f0eb35c8b, 49cc4e8cab); archive ed457edca9; index F23.
- Next: lockfile-refresh cadence rides the #42 watch.

## ✅ Empty-headers sync-slot stall — IBD liveness (adopted 5-commit stack)
- Mechanism: a sync peer answering valid empty headers keeps its
  initial-sync slot (empty input fails the sync-continuation check,
  timestamp not cleared, slot release fires only when a replacement
  exists) — IBD stalls on a slot the peer never fills.
- Evidence: FAILING-BEFORE — the PR's own test fails at unfixed HEAD
  ('Test empty headers response during initial sync'); PASSING-AFTER —
  full p2p_initial_headers_sync.py green on the archive tip.
- Branch/commits: audit/adopt-empty-headers (297c0f7ca7, 7de4fba5b8,
  79aca0b97f, 92a98ffb30, 6d10e8e193); archive 93d076cd38 + stack-
  completion repair 212672d6c9; journal #65 c17; index F22.
- Next: track PR 35839 upstream (open 2026-07-29).

## ✅ RPC method-name log injection + wallet-name control chars (adopted 3 commits)
- Mechanism: whitelist-rejection warnings logged method names UNSANITIZED
  (httprpc.cpp:118/:147) — newline injection forges node-looking lines;
  wallet names accepted control chars (paths/UIs/logs).
- Evidence: FAILING-BEFORE — characterize test fails expecting the
  injected forged-consensus-error payload ('getblock\nERROR: ConnectTip:
  ConnectBlock 0000...deadbeef failed'); PASSING-AFTER — rpc_whitelist.py
  green, createwallet bad\nname -> error -8, goodname created.
- Branch/commits: audit/adopt-sanitize-logs (9d5fb22f1d + 6ed8e2af39 +
  ed4eb51e9f); archive 0c7d19aec8; journal #30 c6; index F20.
- Next: track PR 35833 upstream.

## 🟠➜✅ FlushStateToDisk writes after block-flush failure (adopted f90291ffb9)
- Mechanism: a failed FlushChainstateBlockFile only logged (TODO at
  validation.cpp:2821-2822) — block-index + coins writes PROCEEDED and
  m_last_flushed_block advanced, recording a block as flushed while its
  block/undo data may not be durable (restart/wallet then trust it).
- Trigger: block-file IO failure during a flush (disk full/error/perms —
  #93-c2 unwritable-blocksdir family).
- Evidence: FAILING-BEFORE — PR's boundary test fails 2 assertions with
  the pre-fix validation.cpp (marker advanced); PASSING-AFTER — test
  green (exit EXIT_FAILURE, marker unchanged). Author PR 35714 (open).
- Branch/commit: audit/adopt-flush-failure @ f90291ffb9; archive this cycle.
- Next: track PR 35714 upstream; adopt 0f04fbee2f characterization test
  if the author reworks it.

## ✅ txdb cursor valid over malformed first key (F25, adopted 8481b1f27f)
- Mechanism: CCoinsViewDB::Cursor() cached the default DB_COIN tag
  when first-key decode failed -> Valid()/GetKey() reported a usable
  cursor over an undecodable key; Next() already invalidated it.
- Trigger: corrupt or foreign-written chainstate keyspace (readable
  value under a malformed coin key); not network reachable.
- Evidence: failing-before (Valid/GetKey true over one-byte 'C'
  key) / passing-after (focused + coins_tests green).
- Branch/commit: audit/adopt-txdb-cursor-firstkey 8481b1f27f;
  archive 35548eb2ba; upstream PR 35654 open (covered-ahead).
- Next: track 35654 merge.

## ✅ xor.dat short-write leaves unbootable datadir (F26, adopted 2110abf119)
- Mechanism: InitBlocksdirXorKey left a truncated xor.dat when the
  key write/close failed; next startup read it as authoritative ->
  AutoFile::read: end of file -> unbootable until manual deletion.
- Trigger: local IO fault (disk full/quota/transient) on first key
  creation; loud failure, availability only. F19 write-failure family.
- Evidence: deterministic one-shot LD_PRELOAD fwrite interposer —
  pre-fix 1-byte xor.dat + restart EOF; post-fix file removed,
  restart regenerates key, getblockcount=0, clean stop.
- Branch/commit: audit/adopt-xor-key-shortwrite 2110abf119;
  archive ebd42ea45e; upstream master vulnerable.
- Next: none locally; offerable upstream with the injection harness.

## 🔴 UTXO-scan/resize race — upstream master (fixed in-tree e049f064e1)
- Mechanism: gettxoutsetinfo/scantxoutset create a LevelDB cursor
  under cs_main then scan unlocked; assumeutxo ResizeCache resets
  m_db mid-scan → iterator use-after-free → LevelDB abort
  (VersionSet dtor assert).
- Public trigger: authorized/local RPC during snapshot activation or
  cache rebalance — availability kill, not consensus.
- Evidence: upstream master raw fetch today has no cursor lock; the
  fork's clean-master reproducer aborted (commit message e049f064e1);
  tree holds a UniqueLock for cursor lifetime — verified in txdb.cpp
  :231-262.
- Branch/commits: fix in lineage (e049f064e1 + unit test + resize
  fuzz target); journal resource-exhaustion-variants.md c2;
  archived on agent/all-findings (hash-present e049f064e1).
- Next: watch upstream 35744 (open, head 38b84769608a; DrahtBot
  flagged a TSan pthread_cond_destroy race in its shared-lock
  rework 2026-07-28 — that shape cannot exist in-tree: zero
  condvar/shared_mutex in txdb, resize-cursor test green at HEAD
  per #42 c1). Nothing to do locally.

## ✅ txgraph retained-capacity accounting (adopted + verified locally 28ba79168b)
- Mechanism: Compact() pops m_entries without releasing vector
  capacity; GetMainMemoryUsage charged live count only -> retained
  memory outside -maxmempool accounting.
- Evidence: fix 475ab49da6 (DynamicUsage(m_entries) charge) adopted
  onto the lineage; flipped characterization test GREEN (churned
  usage now GT fresh); full txgraph_tests green; #22-c4 churn
  profile rerun unchanged within noise (accounting truth at large
  scales; RSS residual is allocator slack).
- Branch/commit: audit/adopt-retained-capacity @ 28ba79168b;
  author's upstream vehicle l0rinc/txgraph-retained-entry-usage.
- Next: none local; watch the author's PR for upstream.

## ✅ dbwrapper failed-construction leak (fixed 73a6798206, #13 c2)
- Mechanism: CDBWrapper ctor can throw (LevelDB open failure) after
  options members block_cache/filter_policy/info_log were allocated;
  LevelDBContext had no destructor, leaking them per failed open.
- Evidence: failing-before LSan probe — 79,800 B / 361 allocs over
  19 failed opens (stacks NewLRUCache dbwrapper.cpp:142, filter_policy
  :144, info_log :146); passing-after 20 failed opens LSan-silent;
  dbwrapper_tests green. Upstream master has the same missing
  destructor (offerable).
- Branch/commit: audit/raii-resource-leaks-c2 @ 4d8ae03172 (fix
  73a6798206); journal raii-resource-leaks.md c2. Archived on
  agent/all-findings @ d87da3929e (fix 461c21cbfa).
- Next: offer upstream (small RAII fix, mirrors existing option
  ownership comments); re-verified NOT duplicated upstream @
  9611a35603 (#42 c5). Audit other throwing ctors holding raw
  members.

---
Recently removed from this list (dismissed/closed): Fee-estimator
zero-state per-block waste (fixed 675011ba86, verified, no local
follow-ups); LockPoints
bound comment (fixed b1c267c9f1, no follow-ups); l0rinc CheckBlock
dup-input equivalence (PROVEN, #40 c1 — resolved, no local action);
clang-18
differential (green, #36 c2); net_processing sancov 0/23 alarm
(inlining artifact, #9 c2); TODO sweep (0/56 defects, #0 c2);
BIP173/350 vectors (no drift, #81 c2); install manifest parity
(exact, #47 c2); BIP324/RFC8439 vectors (byte-exact, #81 c1);
v28.2/v0.20.1 ↔ HEAD P2P differential (no divergence, #67 c1);
write-amplification ~2x bounded by design (#24 c1).
