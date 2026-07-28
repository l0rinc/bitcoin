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
