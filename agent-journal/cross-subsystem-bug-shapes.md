# Journal: cross-subsystem bug shapes (goal 26)

Campaign: mine proven defect shapes from our own findings and map them to analogous sites.
Base: audit/resurrection @ de4c661c19. Rotation order: 26 → 5/52 → 62 → 85 → 20 → 56.

## Seed shapes (proven, from our own findings ledger)

| id | shape | proven instance | severity class |
|----|-------|-----------------|----------------|
| S1 | DB object replaced while long-lived iterator/cursor alive | coins DB resize vs RPC cursors (PR 35744) | crash/UAF |
| S2 | consensus rule relaxed under assumevalid/script-check-skip | sigops-under-assumevalid (reverted 2b6f78006f) | consensus |
| S3 | retry/reconnect without backoff after failure | torcontrol busy loop (sweep #2) | DoS (local) |
| S4 | bool-parse swallowing path-like value | -asmap digit-path ignored (sweep #5) | silent misconfig |
| S5 | O(n²) erase-in-loop under lock | gettxspendingprevout (sweep #3, fork PR 247) | DoS (auth-RPC) |
| S6 | zero-element/small-N division or log in formula | bloom zero-element (PR 35818); residual rate-domain | UB/crash |
| S7 | opaque object loaded without bound check (parser boundary bypass) | secp opaque sig (PR25), kernel get_ancestor (sweep #1) | abort/UB |
| S8 | signed overflow in RPC-controllable accumulation | mempool fee-delta saturate series (fork PRs 238/228/217/209/214) | UB |
| S9 | "first entry" handled differently from "rest" | first coin cursor key (PR 35654) | silent wrong data |

## Cycle 1 scope
Map S1 (DB/iterator lifetimes), S2 (assumevalid/script-skip gates), S5 (erase-in-loop)
across the tree. For each candidate site: hypothesis, trust boundary, caller trace,
verdict (confirmed/dismissed/inconclusive) + evidence.

## Verdicts
Cycle 1 complete (see sections below): S1 DISMISSED beyond origin class
(PR 35744), S2 DISMISSED on master, S5 DISMISSED beyond origin site
(fork branch l0rinc/rpc-gettxspendingprevout-linear). No new confirmed
findings; no fix commits required by this cycle.

## Next queue
- Cycle 2 (goal 5/52, boundary/integer): use S6/S7/S8/S9 as seeds; first
  candidates — S9 first-vs-rest divergence in other cursor/range consumers,
  S8 signed accumulation in other RPC loops, S6 rate-domain leftovers.
- Track open upstream PRs from these shapes: 35744 (coins cursor lock),
  gettxspendingprevout-linear PR, 35654, 35818 — via the watch crons.

## S2 verdict (cycle 1): DISMISSED on master

Enumerated every fScriptChecks gate in validation.cpp @ origin/master (e75b76b12c):
- Gated: checkqueue control (2528) + CheckInputScripts/queue (2585) — exactly script
  verification, which is precisely what assumevalid is documented to skip. OK.
- UNGATED (consensus enforced always): CheckTxInputs (HaveInputs/maturity/MoneyRange),
  BIP68 SequenceLocks (inside `if (!tx.IsCoinBase())`, NOT inside the fScriptChecks gate —
  verified at validation.cpp:2543), GetTransactionSigOpCost + MAX_BLOCK_SIGOPS_COST,
  fee accumulation. No consensus rule is relaxed by the gate.
- Other assumevalid-like skips checked: -assumeutxo (background chainstate validates
  fully by design), no skip-pow option, -prune validates fully.
Conclusion: the assumevalid contract holds on master; our reverted 2b6f78006f was the
only anomaly and it stays dead. Verdict: dismissed, no fix needed.

## S1 verdict (cycle 1): origin class covered by own PR 35744; all analogous sites DISMISSED

Mapped every DB-object-replacement vs live-iterator/cursor site:

1. Coins DB resize vs RPC cursors (gettxoutsetinfo / scantxoutset / dumptxoutset):
   the S1 origin class itself. Fix is our own open upstream PR 35744
   "coins: prevent DB resize from invalidating cursors" (state: open, unmerged on
   origin/master @ e75b76b12c); this branch already carries it (src/txdb.cpp:74-85
   ResizeCache takes m_db_mutex exclusively; Cursor() at txdb.cpp:252-254 returns a
   cursor holding a UniqueLock for its lifetime). Status: CONFIRMED-ALREADY-FIXED,
   no new work; track PR 35744 review in the watch crons.
2. BaseIndex restart (snapshot completion Stop/Init loop): the index DB object is a
   unique_ptr created once in the subclass ctor (txindex.cpp:69) and never replaced;
   only m_chainstate is republished — and that publication race is already fixed on
   this branch by f344e8102c "index: synchronize chainstate publication during
   restart" with regression tests index/txindex/txospenderindex_reinit_reader_race.
   Verdict: dismissed (fixed, tested).
3. Wallet Berkeley→SQLite migration (wallet.cpp:3890-3933): cursor and batch are
   explicitly reset (3909-3910) BEFORE m_database->Close() and replacement (3918,
   3932-3933). Iterator lifetime strictly bounded inside DB lifetime. Dismissed.
4. SQLite Close vs live cursor (sqlite.cpp:383-390): sqlite3_close with unfinalized
   statements returns SQLITE_BUSY and Close() throws — loud failure, no silent UAF;
   batches/cursors are short-lived locals under cs_wallet. Dismissed.
5. CCoinsViewDB::NeedsUpgrade raw NewIterator (txdb.cpp:40) bypasses m_db_mutex, but
   runs only at init (LoadChainstate) before any concurrent ResizeCache can exist.
   Dismissed (not concurrently reachable).
6. BlockTreeDB (LoadBlockIndexGuts iterators): blocktree DB is never replaced at
   runtime; no resize path. Dismissed.
7. Kernel C-API iterators (bitcoinkernel_wrapper.h): in-memory index iterators over
   caller-owned Ranges; coinstats cursor borrows a caller-owned CCoinsView. DB
   lifetime is the API consumer's contract. Dismissed (boundary documented).
8. Mempool dumps (mempool.dat/v2): flat serialized file written under mempool cs,
   no DB cursor. Dismissed (not the shape).
9. Flatfile/pruning vs ReadBlockData: pruning deletes blk/rev files; reads open by
   recorded position and fail gracefully (return false), no live handle into a
   replaced object. Dismissed.

No new S1 defect found on master beyond the already-patched origin class.

## S5 verdict (cycle 1): origin class covered by own fix branch; all analogous sites DISMISSED

Swept every `.erase(`/remove_if inside loops in rpc/, net_processing.cpp, net.cpp,
validation.cpp, txmempool.cpp, coins.cpp, txdb.cpp, node/, wallet/, index/,
kernel/, addrman.cpp, banman.cpp, blockstorage.cpp, and inline header containers.
Per-site container check:

1. Origin site — rpc/mempool.cpp:1015-1027 (gettxspendingprevout): `std::vector`
   erase-in-loop under `LOCK(mempool.cs)`, RPC-controllable length. CONFIRMED and
   already fixed on fork branch l0rinc/rpc-gettxspendingprevout-linear
   (b9d291db41 "rpc: avoid quadratic prevout resolution" + e0d7c81f10 test,
   std::erase_if single-pass compaction), upstream PR open. Not on this branch's
   base; no new work needed here.
2. net_processing.cpp:6206-6227 (tx relay trickle): erases from
   `std::set<Wtxid>` — O(log n) per erase, loop bounded by broadcast_max.
   Dismissed.
3. net_processing.cpp:2630 (m_getdata_requests deque): single range-erase of a
   consumed prefix per message, amortized O(k). Not a loop of middle erases.
   Dismissed.
4. net_processing.cpp:5597 / net.cpp:1980,2705,3787: erase-remove_if idiom —
   single O(n) pass, which is the fix pattern itself. Dismissed.
5. validation.cpp:3734 (highpow_outofchain_headers): std::multimap, O(log n).
   Dismissed. validation.cpp:1788 (results_final): std::map. Dismissed.
   setBlockIndexCandidates / m_blocks_unlinked / m_dirty_*: std::set. Dismissed.
6. txmempool.cpp (mapNextTx/mapTx/mapDeltas): boost multi_index/std::map —
   O(log n). Dismissed. coins.cpp (cacheCoins): hash map. Dismissed.
7. wallet/scriptpubkeyman.cpp:556-571, 689-760 (migration keyids/spks): both
   std::set, O(log n), and one-shot authenticated migration path. Dismissed.
8. addrman.cpp:950,971 (m_tried_collisions): std::set. Dismissed.
   banman.cpp:201 (m_banned): std::map. Dismissed.
9. rpc/mempool.cpp:825 (setDescendants), rpc/server.cpp:414,441 (argsIn),
   rpc/blockchain.cpp:1636 (setPrevs): std::set, O(log n). Dismissed.
10. node/miner.cpp:94 (tx.vout erase): single erase per coinbase construction,
    not in a loop. Dismissed.

Conclusion: no second instance of the quadratic erase-under-lock shape exists in
production code on master; the shape is fully accounted for by the origin site
and its open fix PR. Verdict: dismissed beyond origin.
