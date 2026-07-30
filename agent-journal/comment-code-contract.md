# Campaign #1 — comment-code-contract

Base: c17c4e7574 (journal commit for #22 cycle-2 on
audit/full-sync-ibd-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/comment-contract. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): validation.cpp strong-claim audit — 1 imprecise bound fixed, 3 deep claims verified true

### Draw
Random draw over the 30-goal eligible pool (31-pool minus #22, now
CYCLE-2): raw=11412206341905458499, seed masked to 63 bits
(2188834305050682691), index 1 -> #1.

### Scope / method
Cell: src/validation.cpp comments using must/never/always (66 raw hits,
~40 after filtering Assert/Log/error-message noise). Deep verification
of 4 high-stakes semantic claims + 3 spot checks. For each: identify
governed code and callers, compare comment claim vs implementation,
decide code-wrong or comment-wrong.

### Claim A — validation.cpp:255-256 (LockPoints) — COMMENT WRONG, fixed
Claim: "tip->GetAncestor(max_input_height) should never return a
nullptr because max_input_height is always less than the tip height."
Verification (CalculateLockPointsAtTip, validation.cpp:219-261):
prev_heights from CalculatePrevHeights assigns confirmed inputs their
coin height and mempool inputs next_tip height (tip+1); the maximum
excludes only the tip+1 entries. A mempool transaction spending an
output of the TIP block (ordinary CPFP case: parent mined in tip,
child in mempool with a BIP68 sequence) yields
max_input_height == tip->nHeight, i.e. EQUAL, not "always less than".
GetAncestor (chain.cpp:100) returns nullptr only for height > nHeight
or < 0, so the conclusion (never nullptr) is still correct — but via a
different bound (<=) than the comment states (<). Decision: comment
wrong (bound direction), code correct. Fix: comment corrected to state
the <= bound with its two-part rationale (confirmed inputs at/below
tip; mempool inputs at tip+1 excluded). No behavior change; TU rebuild
clean (ninja bitcoin_node.dir/validation.cpp.o). No duplicate of the
claim elsewhere in src/ (grep 'always less than the tip height').
Trust boundary: none (comment accuracy); master-relative severity:
documentation only, but the wrong bound invites a reader to "tighten"
the code incorrectly. Why existing tests missed it: tests exercise the
conclusion (Assert never fires), not the stated reason; the equal-height
case occurs constantly in production (any 1-conf parent spend).

### Claim B — validation.cpp:2200-2207 (DisconnectTip AddCoin) — TRUE
Claim: "If the coin already exists as an unspent coin in the cache,
then the possible_overwrite parameter to AddCoin must be set to true...
When fClean is false, an unspent coin already existed and it is an
overwrite." Verification: fClean is set false iff view.HaveCoin(out)
(line 2188) or the undo-metadata fallback path; AddCoin is called with
possible_overwrite = !fClean (line 2208), matching the claim exactly.
The AccessByTxid fallback cannot perturb the out's HaveCoin state
(same-outpoint case implies HaveCoin was already true; different-n
case touches a different outpoint). Comment and code agree.

### Claim C — validation.cpp:4668-4670 (TestBlockValidity lock) — TRUE
Claim: "Lock must be held throughout this function for two reasons:
1. tip stability 2. CheckBlock fChecked race (see ProcessNewBlock)."
Verification: AssertLockHeld(chainstate.m_chainman.GetMutex()) at
entry; both reasons match the code (hashPrevBlock-vs-tip check then
ConnectBlock onto the same tip; ProcessNewBlock's critical section at
4597 includes CheckBlock for fChecked). All caller paths hold the lock:
miner.cpp CreateNewBlock takes LOCK(::cs_main) at :142 (same mutex —
ChainstateManager::GetMutex() is ::cs_main), rpc/mining.cpp:409 and
:771 LOCK before :423/:781, node/interfaces.cpp:1022 LOCK(chainman()
.GetMutex()) before :1023. Claim accurate and machine-enforced.

### Claim D — validation.cpp:5371 (CheckBlockIndex nullptr parent) — TRUE
Claim: "Only genesis, which must be part of the best header chain, can
have a nullptr parent." Verification: LoadBlockIndexDB
(node/blockstorage.cpp:147) links every loaded index to
insertBlockIndex(hashPrev); blocks load sorted by height so parents
link first; pprev==nullptr is therefore possible only for genesis (or
a placeholder from a corrupt DB missing a parent — which is exactly
what this assert is auditing). Genesis is in m_best_header's chain by
construction (best header descends from genesis). Comment, assert, and
load logic agree; the surrounding forward-size assert cross-checks the
partition.

### Spot checks (mechanical, all TRUE)
- :4114 "First transaction must be coinbase, the rest must not be" —
  CheckBlock enforces verbatim.
- :1510 "Transactions must meet two minimum feerates: the mempool
  minimum fee and min relay fee" — matches CheckFeeRate call sites.
- :5067 "At and above m_params.SegwitHeight, segwit consensus rules
  must be validated" — matches deployment gate in ConnectBlock.

### Exact commands
- grep -nE '\b(must|never|always)\b' src/validation.cpp (filter pass)
- sed context reads at :219-261, :2188-2210, :4660-4690, :5360-5380
- GetAncestor semantics: src/chain.cpp:100-104
- caller lock sweep: grep LOCK/CreateNewBlock/TestBlockValidity over
  src/node/miner.cpp, src/rpc/mining.cpp, src/node/interfaces.cpp
- load invariant: node/blockstorage.cpp:147,251-271
- duplicate search: grep -rn 'always less than the tip height' src/
- build check: ninja -C build-before
  src/CMakeFiles/bitcoin_node.dir/validation.cpp.o

### Verdict
- CONFIRMED (minor): 1 stale/imprecise comment claim — fixed in the
  comment-fix commit on this branch.
- DISMISSED: 3 deep + 3 spot claims are accurate; no code defect.

### Limitations / queue for cycle 2
- validation.cpp still has ~30 unverified must/never/always comments
  (lower-stakes); net_processing.cpp (46 hits) and txmempool.cpp
  untouched — queued.
- The fork's own hardening commits add claim-bearing comments (e.g.
  txdownload "is only supposed to clear ... must not perturb") — those
  are backed by Assumes/fuzz oracles (checked in #22 c2 context);
  a dedicated pass over fork-added comments queued.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-29): net_processing.cpp strong-claim audit — 9 claims verified, 0 wrong

### Draw
Random draw over the 20-goal eligible pool (14 pending + 6 CYCLE-1,
#101 excluded as just-cycled): raw=817997966924375334, index 14 ->
#1 (second cycle; c1 queue cell "net_processing.cpp (46 hits)"
chosen; 52 raw must/never/always hits in this tree, ~40 after
log/error filtering). Branch: audit/comment-contract-c2 from
2d46265680 (#101 c1 bookkeeping; lineage anchor audit/resurrection @
5d0155254c). Start state: tracked-clean.

### Deep claim 1 — :4414-4416 (getblocktxn prune read) — TRUE
Claim: within-MAX_BLOCKTXN_DEPTH block "cannot get pruned after we
release cs_main above, so this read should never fail" (assert(ret)).
Verification: MAX_BLOCKTXN_DEPTH=10 (net_processing.cpp:142);
pruning only deletes blocks deeper than MIN_BLOCKS_TO_KEEP=288
(validation.h:76) below tip at flush time. For the requested block
(height >= tip0-10) to become prunable, >=278 blocks must connect
AND a flush must complete between the cs_main release (:4409) and
ReadBlock (:4413) — physically impossible in that window (block
connection is orders of magnitude slower). The only remaining
ReadBlock failure mode is disk corruption, outside the comment's
claim. Claim accurate.

### Deep claim 2 — :4977-4979 (GETADDR SetupAddressRelay) — TRUE
Claim: "Since this must be an inbound connection, SetupAddressRelay
will never fail." Assume(SetupAddressRelay(...)). Verification:
SetupAddressRelay (:5765-5795) fails only for IsBlockOnlyConn or
IsFeelerConn; the :4972 early return drops !IsInboundConn(), and
IsInboundConn() is true ONLY for ConnectionType::INBOUND
(net.h:852-854) — BLOCK_RELAY, FEELER, MANUAL, ADDR_FETCH, and the
fork's PRIVATE_BROADCAST (net.h:865, outbound class) are all
excluded. Both failure branches unreachable past the guard. TRUE.

### Deep claim 3 — :5697 (block-relay-only tx rejection) — TRUE
Claim: "block-relay-only peers may never send txs to us."
Verification: RejectIncomingTxs (:5695-5703) returns true for
IsBlockOnlyConn; enforcement at every tx ingress: VERSION
reconciliation offer disconnects (:4099-4104), wtxidrelay/INV paths
ignore via reject_tx_invs (:4176), TX message processing returns
early (:4516). "may never send" = never accepted; comment describes
our acceptance policy accurately. TRUE.

### Deep claim 4 — :3913-3924 (tx-inventory-empty at VERSION) — TRUE
Claim: m_tx_inventory_to_send must be empty and m_next_inv_send_time
0s at VERSION completion, else handshake-time txs would be
advertised immediately, leaking arrival time to a spy. Verification:
message ordering rejects any non-VERSION message while
pfrom.nVersion==0 (:3884-3888), so no INV/TX processing can precede
VERSION; m_tx_inventory_to_send is filled only by PushTxInventory
(INV/TX processing or relay from other peers under the same
g_msgproc_mutex that serializes ProcessMessage) and reconciliation
(Erlay) requires sendtxrcncl negotiation, gated between VERSION and
VERACK (:4085). m_next_inv_send_time is first set in SendMessages
after fSuccessfullyConnected (post-VERACK). Both conjuncts hold by
construction + Assume. TRUE.

### Deep claim 5 — :4378-4381 (getblocktxn differential encoding) — TRUE
Claim: "indexes must be strictly increasing; DifferenceFormatter
should guarantee this property during deserialization."
Verification: DifferenceFormatter::Unser (blockencodings.h:39-45)
accumulates m_shift and emits v=m_shift then increments, so
successive outputs strictly increase by construction; the overflow
guard throws on wrap. The same Assume loop also lives inside
BlockTransactionsRequest::SERIALIZE_METHODS
(blockencodings.h:57-59), so the invariant is enforced twice
(serializer + net_processing loop). Redundant but consistent;
"merely improvable" duplication, not a defect. TRUE.

### Spot checks (all TRUE)
- :368/:383 "must correlate" (m_addr_known <-> m_addr_relay_enabled):
  single writer for both (:5785 exchange, :5789 init under the same
  condition); false-path Assumes at :5771-5772 machine-enforce.
- :1717 FinalizeNode refcount caution matches shared_ptr semantics
  (RemovePeer returns PeerRef; deferred destruction possible).
- :1311 -blocksonly never requests HB compact mode: sole HB-marking
  function returns early on ignore_incoming_txs (:1314).
- :2514-2517 merkleblock "MUST always provide at least what the
  remote peer needs": vMatchedTxn loop sends every match (:2518-2519).

### Exact commands
- grep -nE '\b(must|never|always)\b' src/net_processing.cpp (52 hits)
- sed/Read at :1305-1320, :1710-1728, :2495-2550, :360-392,
  :3870-3894, :3900-3930, :4095-4114, :4370-4434, :4955-4994,
  :5690-5724, :5765-5795
- cross-refs: net.h:852-870 (IsInboundConn/PRIVATE_BROADCAST class),
  validation.h:76 (MIN_BLOCKS_TO_KEEP), blockencodings.h:26-60
  (DifferenceFormatter + serializer-side Assume),
  writer grep for m_addr_relay_enabled/m_addr_known

### Verdict
DISMISSED (no defect): all 9 audited strong claims in
net_processing.cpp are accurate; 3 of them carry machine-enforced
Assumes matching the prose. No code or comment change warranted;
journal-only cycle.

### Limitations / queue for cycle 3
- ~30 lower-stakes net_processing hits not deep-verified
  (ping/pong heuristics, eviction prose, send-side scheduling).
- txmempool.cpp cell from c1 still untouched.
- validation.cpp ~30 c1 leftovers still open.
- Fork-added comment pass (PRIVATE_BROADCAST prose is new here;
  worth a dedicated sweep under the c1 queue item).

## Cycle 3 (2026-07-29): txmempool.cpp + txgraph.cpp strong-claim audit — 8/8 verified TRUE

### Draw
Re-rank draw over the 4 remaining CYCLE-2+ open cells:
raw=9064950354337441572, index 0 -> #1 (third cycle; c1 queue cell
"txmempool.cpp untouched" + "fork-added comments pass" ->
txgraph.cpp). Branch: audit/comment-contract-c3 from ff98babc95
(#65 c3 bookkeeping).

### txmempool.cpp (5 raw hits; 4 substantive)
- :537-539 feerate-diagram "should never get behind" -> assert
  (diagram_iter->size >= check_total_adjusted_weight) — matches,
  machine-enforced. TRUE.
- :586-590 topo/score iteration "All parents must have been checked
  before their children" -> assert(mempoolDuplicate.HaveCoin) at
  :590 enforces. TRUE.
- :621 "CheckTxInputs() should always pass" (dummy_state in
  CTxMemPool::check) -> assert at :624; basis: mempoolDuplicate is
  built from all confirmed+mempool coins in the same function.
  TRUE.
- :893-895 CCoinsViewMempool::GetCoin "always return the mempool
  entry; guaranteed to never conflict with the underlying cache" ->
  a txid in both means same transaction (unconfirmed mempool entry
  vs just-connected block's version — identical outputs);
  m_temp_added handles the disconnect-pool residue; mempool entries
  are full (no pruned). TRUE.

### txgraph.cpp (38 raw hits; 4 deep-verified)
- :3060-3061 "If this Cluster has an acceptable quality level, its
  chunks must be connected" -> assert IsConnected inside
  `if (level == 0 && IsAcceptable())` (:3029) — the comment's guard
  matches the code's guard EXACTLY; disconnected (NEEDS_SPLIT)
  clusters are never acceptable, and GetChunking's collapse
  (3ae78dbd25) guarantees chunk-connectedness for connected
  clusters. TRUE.
- :2997-3004 mapping/linearization cardinality + hole-free-unless-
  splitting asserts — consistent with the split machinery. TRUE.
- :314 "always kept topological" -> sanity asserts at :3020-3024
  (m_done.IsSupersetOf(Ancestors)) machine-enforce. TRUE.
- :1495 "The transaction must appear in the chunk" ->
  Assume(chunk[cluster_idx]) in AppendTrimData (the #25 c2
  consumer). TRUE.

### Verdict
DISMISSED: 8/8 strong claims accurate; the fork's txgraph claims
are exceptionally well machine-guarded (sanity-check function
mirrors the prose with asserts). No code or comment change.

### Limitations / queue
- txgraph.cpp remaining ~30 hits are lower-stakes (locator/lifecycle
  prose); the sanity function itself covers most.
- wallet/GUI comment cells remain deprioritized.
- Campaign cells complete across validation.cpp (c1),
  net_processing.cpp (c2), txmempool.cpp+txgraph.cpp (c3) — the
  strong-claim surface of the core TUs is now covered.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Core
strong-claim surface covered; not marking exhausted (lower-stakes
hits + wallet cells remain).

## Cycle 4 (2026-07-30): fork-added PRIVATE_BROADCAST comment pass — all five strong claims verified TRUE

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=5820747569139027543, index 1 (of 2) -> #45 (fourth cycle;
c3 queue cell "fork-added comment pass"). Branch:
audit/comment-contract-c4 from 3b58782cf2 (#89 c2 journal tip).

### Claims verified (comment -> code evidence)
1. net.h:1231-1233 "Opening private broadcast connections will be
   paused if this is equal to 0" (semaphore) -> net.cpp:3315
   CountingSemaphoreGrant ctor calls blocking Acquire()
   (semaphore_grant.h:69-76); the thread pauses exactly at 0 until
   a release. TRUE.
2. net.h:1220-1228 m_outbound_tor_ok_at_least_once docstring (set
   only after a real outbound Tor connection incl. P2P exchange)
   -> net.cpp:4173-4177 sets it only when !inbound && IsTor &&
   sending VERACK (both directions happened); read at
   ProxyForIPv4or6 (:3206-3212) for the stated purpose. TRUE.
3. net_processing.cpp:5911 "The logic below does not apply to
   private broadcast peers, so skip it" -> early return before
   MaybeSendPing/addr logic; private peers get only the lifetime
   check; the comment honestly scopes itself as an optimization
   with PushMessage-side enforcement. TRUE.
4. net_processing.cpp:1769-1771 "don't call Connected() for
   private broadcast (could leak information in addrman)" -> the
   condition explicitly excludes IsPrivateBroadcastConn(). TRUE.
5. net.cpp:4154-4161 outbound allowlist (VERSION, VERACK, INV,
   TX, PING only) -> minimal and privacy-coherent; the inbound
   GETDATA path (net_processing.cpp:4273-4298, #49 c3) only
   serves the announced tx and disconnects on anything else.
   TRUE.

### Verdict
DISMISSED: the fork-added PRIVATE_BROADCAST prose is accurate and
honest about its own scope (including the optimization framing at
:5912-5913 and the addrman leak rationale at :1770). No comment
or code change needed.

### Exact commands
- net.h:1217-1290 (PrivateBroadcast class docs);
  net.cpp:3315/3708/4154-4177/3206-3212; semaphore_grant.h:13-90;
  net_processing.cpp:1768-1772/5905-5925/4273-4298

### Limitations / queue
- validation.cpp ~30 lower-stakes c1 leftovers remain open
  (bounded by lower value).
- txgraph.cpp ~30 lower-stakes hits (c3 queue) remain; the sanity
  function covers most.
- wallet/GUI comment cells remain deprioritized per scope.

## Rotation note
Four cycles; the fork-added comment surface is verified. Core
strong-claim surface covered; not marking exhausted (leftovers).
