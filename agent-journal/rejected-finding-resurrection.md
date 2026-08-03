# Journal: rejected-finding resurrection and assumption attack (campaign 62)

Uber-goal rotation. Branch: audit/rejected-finding-resurrection from
audit/resurrection @ f112f38d99. Method: collect previously dismissed
candidates from ALL accumulated journals, treat each dismissal as a
falsifiable claim, attack ONE assumption at a time.

## Candidate ledger

| # | candidate (origin) | dismissal assumption attacked | verdict |
|---|--------------------|------------------------------|---------|
| R1 | m_blocks_for_inv_relay unbounded growth (goal 89, P2.1 fragility) | "SendMessages may skip the drain under fPauseSend or peer-state conditions" | attack FAILED (dismissal stands) |
| R2 | estimaterawfee NaN threshold (campaign 98 fragility) | boundary-proven unreachable via any input path | attack skipped (already rigorously proven) |

## R1 attack log

Assumption attacked: the P2.1 dismissal said ThreadMessageHandler
interleaves one ProcessMessages + one SendMessages per node per iteration,
and SendMessages unconditionally clears m_blocks_for_inv_relay — but is
the drain skipped when the send buffer is full (fPauseSend) or the peer is
in some state?

Attack trace (net_processing.cpp SendMessages):
- Only gates before the inventory drain: version-handshake complete
  (5905) and !fDisconnect after ping (5932). NO fPauseSend guard, NO
  early return between 5938 and 6148 in any peer state.
- The drain itself (6134-6148): vInv built from m_blocks_for_inv_relay
  every call, capped at MAX_INV_SZ per message, and
  m_blocks_for_inv_relay.clear() at 6147 runs UNCONDITIONALLY.
  fPauseSend affects PushMessage (bounded send buffer), not the queue
  clear.
- ThreadMessageHandler (net.cpp:3232-3242) calls ProcessMessages then
  SendMessages for EVERY node EVERY iteration, unconditionally; pre-VERSION
  messages are dropped, so GETBLOCKS can't arrive before the drain exists.

Verdict: ATTACK FAILED. The invariant is stronger than the original note —
the drain is unconditional per iteration for every connected peer.
P2.1's dismissal stands: no fPauseSend/state path lets GETBLOCKS
accumulate the queue. Confirmed again, no fix needed.

## Next queue
(R3: re-audit "suppressed-as-intentional" ubsan entries from campaign 98
with fresh eyes? No — mechanism-proven. R4: attack the "no production raw
CFeeRate deserialization" claim from G-slice-1 by searching NEW
serialization users since (none added — verify quickly next cycle if a new
READWRITE appears). Otherwise rotate per ledger: #56 stale PR resurrection.)

## Cycle (2026-08-03, cycle-324 r54, raw=4384128710462411582 -> idx 62): R4 queue cell CLOSED — "no production raw CFeeRate deserialization" survives

### Attack
grep for new CFeeRate serialization users (READWRITE/*fee* across
src/, tests excluded): ONLY src/policy/feerate.h:128
SERIALIZE_METHODS(CFeeRate, obj) { READWRITE(obj.m_feerate.fee,
obj.m_feerate.size) } — the sat/vB wrapper pair, not a raw CFeeRate.
git log origin/master --since=2026-07-25 on transaction.h/core_io.h:
empty (no new serialization sites upstream either).
### Verdict
Claim SURVIVES (no new users). R4 closed; R3 remains
mechanism-proven; queue rotates per ledger.
