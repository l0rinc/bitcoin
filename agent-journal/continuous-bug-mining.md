# Journal: continuous evidence-first bug mining (campaign 0)

Uber-goal rotation. Branch: audit/continuous-bug-mining from
audit/resurrection @ 93e89be11d. This campaign is the rotation's
collector: leads from all other journals land here.

## Cycle 1: TransactionCanBeBumped fuzz target — DELIVERED (da8b249776)

Task carried from #56 (PR 33916 revival). Settled design per the review:
real CTxMemPool (no mock), direct AddToWallet state construction,
independent oracle.

Target (src/wallet/test/fuzz/spend.cpp): builds a wallet bump candidate
from a confirmed funding tx, fuzzes the PreconditionChecks state
dimensions (membership, confirmed/mempool, replaced_by_txid, wallet
descendants, foreign inputs) and asserts the result equals an
independently recomputed expectation. Verified: build_fuzz clean;
-runs=5000 zero failures (268s aarch64).

## Leads map (from accumulated journals)
- P2.1 drain invariant (goal 89): CONFIRMED-STRONGER (campaign 62 R1) — closed.
- 33916 bump fuzz gap (56): DELIVERED this cycle — closed.
- PR 35740 http linger-close (56): open revival candidate, low priority.
- feerate.h:128 CFeeRate deserialize invariant (98): fragility note — no
  production raw deserializer exists; watch for new READWRITE users.
- estimaterawfee NaN-passing comparison (98): unreachable, boundary-proven — closed.
- Interrupted-migration load-time detection (88 W5): future improvement candidate.

## Next queue
(rotate per uber-ledger — next: re-rank from accumulated journals)
## Cycle 2 (2026-07-28): TODO/FIXME evidence-source rotation — 56 production items swept, 0 defects, 7 verified risk-map cells

Base: 13ed36b0f4 (journal commit for #1 cycle-1 on
audit/comment-contract; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/bug-mining-c2 (c1 journal carried in
e7a201205a). Start state: clean (untracked scratch only).

### Draw
Random draw over the 29-goal eligible pool (30-pool minus #1, just
cycled): raw=15840963109778752071, seed masked to 63 bits
(6617591072923976263), index 0 -> #0.

### Evidence-source rotation rationale
c1 used a fuzz-target delivery. #0's charter lists TODOs as a distinct
evidence source; no rotation cycle had swept them. Method: full
production-tree TODO/FIXME/XXX/HACK enumeration (56 items after
excluding subtrees/tests/qt/ipc), shortlist by trust-boundary stakes,
verify each shortlisted claim against current code. Leads-map items
(PR 35740 linger-close, 88-W5 migration detection) reviewed and left
parked: both are improvement candidates without a falsifiable defect
hypothesis at current evidence.

### Verified risk-map cells (all current, none a defect)
1. coins.h:813 (AddCoins overwrite assumption): callers validation.cpp
   :2049 (check=false, ConnectBlock per-tx) and :4973 (check=true,
   VerifyDB rescan path). The check=false overwrite-only-for-coinbase
   assumption rests on BIP30 duplicate-txid prevention; the TODO wants
   a pre-BIP34 boolean to narrow it further. Defense-in-depth
   wishlist, upstream-known. No unknown overwrite path found.
2. validation.cpp:2119 (CuckooCache external cs_main requirement):
   machine-enforced by the AssertLockHeld at the same line; cache
   accesses at :2120/:2168 are inside the same locked region; setup at
   :2075 is init-time. Design wishlist, not a race.
3. txdownloadman_impl.cpp:308 (orphan-resolution DoS limits):
   partially addressed — non-permissioned peers are capped by the
   MAX_PEER_TX_ANNOUNCEMENTS count check directly below (resolution
   parents included in the count); per-orphanage-usage metering is the
   remaining wishlist. No bypass found.
4. secure.h:52 (RPC params not mlock()ed): still true — a
   walletpassphrase RPC leaves the passphrase in non-mlocked UniValue
   memory until request teardown even though the handler copies into
   SecureString. Upstream-documented privacy limitation; recorded as a
   secret-bearing-path risk-map entry (not a new finding).
5. validation.cpp:2821 (FlushChainstateBlockFile error swallowed to
   LogWarning): reachable only on I/O fault at block-file rotation
   (CheckDiskSpace ran just above); worst case is unreadable block
   data on restart -> reindex, not silent acceptance. Upstream-known
   murk; bounded.
6. net_processing.cpp:4794 (optimistic compact-block reconstruction
   failure ignored): the ignore is on an optimization fallback path;
   the block proceeds via the normal in-flight path. Logging wishlist.
7. coins.cpp:298/305 (BatchWrite non-dirty skip / fresh-spent erase):
   defensive-code annotations; the fork's own Assumes already guard
   the same loop (MoneyRange / snapshot invariants). No invariant
   violation reachable.

### Cross-link
validation.cpp:6280 "XXX ... slow and will hold cs_main for
potentially minutes" is the documented form of the CheckBlockIndex
cost profiled in #21 c1 / #22 c1-c2 (85-92% of regtest rebuild wall).
The code documents it; the profiles quantify it.

### Classification of the remainder (49 items)
Upstream-documented design wishlists (txreconciliation unused fields,
CNodeState member moves, sync.h recursive-lock note, BIP30 cutoff
1,983,702 awaiting a command-block mechanism, index v31 cleanup
(future), descriptor keypath-spend fee-estimate FIXMEs
(under-estimation for script-path wallets, upstream-known), wallet
resend/rescan/versioning notes, httpserver error formatting, fs
preallocation byte-write nit, CJDNS consideration, mining.expires
recheck, interfaces/chain.h layering, rpc mocktime sync, util
subprocess cleanups, sign.cpp conflicting-info note, blockencodings
predictive fill, validation structural merges). No stale claims (all
still describe current code), no hidden defect.

### Verdict
DISMISSED (defect hypotheses): 0 of 56 production TODO/FIXME/XXX/HACK
items is a live defect. Risk map expanded with 7 verified
trust-boundary cells (4 deep-verified + 3 tail-checked). No commit
manufactured; journal-only snapshot per policy.

### Exact commands
- grep -rn 'TODO|FIXME|XXX|HACK' src/ --include='*.cpp' --include='*.h'
  (exclusions: test/ bench/ secp256k1 leveldb crc32c minisketch
  univalue libmultiprocess qt/ ipc/) -> 56 items
- sed context reads: coins.h:795-830, validation.cpp:2110-2130,
  2812-2830, txdownloadman_impl.cpp:300-320, secure.h:45-58,
  coins.cpp:285-312, net_processing.cpp:4785-4800
- caller checks: grep 'AddCoins(' src/validation.cpp;
  grep 'm_script_execution_cache.' src/validation.cpp

### Limitations / next leads
- Comment-adjacent evidence sources now used: TODOs (#0 c2),
  must/never/always claims (#1 c1). Remaining fresh sources per
  charter: whole-history regression re-check, coverage gaps, external
  advisories vs current tree.
- PR 35740 (http linger-close) and 88-W5 (interrupted-migration
  load-time detection) stay parked — improvement candidates, not
  falsifiable defects.
