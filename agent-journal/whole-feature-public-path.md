# Campaign #109 — whole-feature-public-path

Base: a6aa1a4869 (journal commit for #45 cycle-2 on
audit/constant-time-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/whole-feature. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): compact-block relay end-to-end — every boundary guarded; wrong-vs-malicious classification correct

### Draw
Random draw over the 39-goal pool (22 pending + 17 CYCLE-1; #45
excluded as just-cycled): raw=2967907378595399043, index 24 -> #109.

### Feature map (public path, cross-file)
1. SEND (us->peer): NewPoWValidBlock -> CMPCTBLOCK per peer SENDCMPCT
   hb negotiation (net_processing.cpp:2530-2533; build from
   blockencodings.cpp BlockAndHeaderAndShortTxIDs).
2. RECEIVE CMPCTBLOCK (net_processing.cpp:4596): reconstruct via
   PartiallyDownloadedBlock::InitData (mempool + extra-txn cache);
   complete -> ProcessNewBlock; incomplete -> GETBLOCKTXN
   (4770/4780) with short-id request.
3. BLOCKTXN response (peer->us): FillBlock
   (blockencodings.cpp:238-307) -> ProcessNewBlock.
4. GETBLOCKTXN request (peer->us): we answer with BlockTransactions
   (2678; whole-block bounded response).

### Boundary audit (2-3 surfaces deep)
- Negotiation: SENDCMPCT hb honored before serving; fork guard
  684c9dae6e (blocksonly-ignore contracts) present.
- Reconstruction: short-id collisions handled as HONEST-possible —
  READ_STATUS_FAILED (witness/merkle commitment mismatch via
  IsBlockMutated, blockencodings.cpp:285-288) falls back to getdata
  full-block download with the failed partialBlock kept as a
  collision guard (net_processing.cpp:3555-3568). No ban — correct:
  commitment failure can never be the right block, but punishing
  would let an attacker get innocent peers banned by grinding
  collisions.
- Structural invalidity (wrong counts, malformed responses):
  READ_STATUS_INVALID -> Misbehaving (3551-3554, 4742-4745) —
  the malicious class, correctly separated from the collision class.
- Response amplification: a garbage BLOCKTXN forcing full-block
  fallback costs the attacker block bandwidth — no amplification,
  bounded by design.
- Fork contracts: 644d18a7f1 blocktxn response ref Assumes,
  non-null vtx invariants (blockencodings.cpp:241-242, 281-283,
  306-307), InitData capacity/duplicate checks, and the #65-radar
  merged extra-tx optimization (1a3cbf1bd2).

### Verdict
- DISMISSED: the compact-block public path is guarded at every
  boundary this cycle reached (negotiation, reconstruction,
  commitment, classification, amplification, fork contracts). No
  interaction-wrong pair of individually-valid components found.
- Deepest edge recorded for the map: the READ_STATUS_FAILED vs
  READ_STATUS_INVALID split is the whole wrong-vs-malicious
  contract — verified line-exact.

### Exact commands
- greps/seds: net_processing.cpp:2171-2678, 3545-3570, 4375-4780;
  blockencodings.{h,cpp}:141-310

### Limitations / queue
- Static map + boundary reads; no live two-node compact-block run
  this cycle (a v1/v2 + hb-mode matrix run is queued as c2 with the
  two-node harness recipe from #67).
- getblocktxn for INDEX-mode (low-bandwidth announcements) not
  separated — queued.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-31): live two-node compact-block matrix — v1/v2 x hb-synced/mempool-miss all match the static map; DISMISSED

### Draw
RE-RANK draw 133 over the 9-cell re-harvested queue: raw=
2340311064075852148 (already 63-bit) -> idx 1 -> #109 c2.

### Hypothesis (falsifiable)
Across {v1,v2 transport} x {hb mempool-synced, mempool-miss}, a
two-node regtest pair behaves exactly as the c1 static map claims:
hb peer receives unsolicited CMPCTBLOCK and reconstructs from mempool
with ZERO GETBLOCKTXN; a mempool-miss triggers exactly one
GETBLOCKTXN/BLOCKTXN pair; no full-block fallback on the new tip.

### Harness (preserved)
/tmp/btc109c2/compact_matrix.py — BitcoinTestFramework, 2 nodes,
-capturemessages on node1, per-cell wipe of regtest/message_capture,
MiniWallet for the mempool tx. Deterministic miss: tx created while
node1 is disconnected (mempool txs are not re-announced on
reconnect). Evidence = contrib/message-capture/message-capture-parser.py
counts over msgs_recv/msgs_sent.dat. Config:
build-before/test/config.ini. Run: python3 compact_matrix.py
--tmpdir=<fresh> --configfile=... (rc=0).
Setup lessons (fork-specific): connect_nodes(1,0) so the CAPTURED
node is outbound (hb sendcmpct is only requested on outbound conns);
blocksonly disables hb negotiation entirely (takes the full-block
headers path — cannot exercise GETBLOCKTXN); MiniWallet needs
w.generate(101, called_by_framework=True) for mature UTXOs; parser
emits ONE json array; captures live in regtest/message_capture/<peer>/.

### Results (message counts on node1, new-tip delivery only)
- v1-hb-synced:  cmpctblock=1, getblocktxn=0, blocktxn=0  PASS
- v2-hb-synced:  cmpctblock=1, getblocktxn=0, blocktxn=0  PASS
- v1-lb-miss:    cmpctblock(new tip)=1, getblocktxn=1, blocktxn=1  PASS
- v2-lb-miss:    cmpctblock(new tip)=1, getblocktxn=1, blocktxn=1  PASS
  (miss cells also show catch-up cmpctblock requests 95/38 from the
  reconnect — low-bandwidth getdata MSG_CMPCT_BLOCK for recent
  blocks, expected and separated from the new-tip delivery.)
MATRIX VERDICT: all 4 cells match the static map (rc=0, asserts in
harness).

### Verdict
DISMISSED (confirms c1 at the protocol level): negotiation,
reconstruction, and the GETBLOCKTXN fallback behave exactly as
mapped, identically on v1 and v2 transports. No interaction-wrong
pair. The wrong-vs-malicious classification (c1) is now backed by
live traffic, not just code reads.

### Limitations / queue
- Short-id collision class (READ_STATUS_FAILED -> full-block
  fallback) not exercised live — needs a ground collision or
  extra-txn poisoning; queued as the only remaining live cell.
- getblocktxn SERVING path (us answering a peer's GETBLOCKTXN from
  our block index) observed only as the responder side of the miss
  cells; an INDEX-mode (low-bandwidth announce) serve matrix is a
  nicety on top.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

---

## Cycle 3 (2026-08-01, draw 167, raw=1164948412836237405, masked same, idx 3/6)
### Cell: short-id collision / READ_STATUS_FAILED -> full-block fallback (last live cell)

### Hypotheses
H1 (collision attack): a remote peer can force a 48-bit short-id
collision against a victim's mempool tx to steer compact-block
reconstruction. Falsifiable by grinder calibration + bound.
H2 (fallback machinery): the READ_STATUS_FAILED -> full-block
fallback path that a collision (or any commitment-breaking
reconstruction) funnels into actually detects and recovers on the
public P2P path. Falsifiable by a tampered-prefilled drive.

### Evidence — H1: DISMISSED (infeasible, not merely unobserved)
- Selector keys are per (header, nonce): siphash key derived from
  SHA256(header || nonce); shortids are 48 bits of SipHash-2-4 over
  the wtxid. Matching ONE specific shortid is preimage work ~2^48,
  NOT the 2^24 birthday figure (birthday only applies to colliding
  two attacker-chosen ids within one block, which still requires
  both to land in the victim's mempool/reconstruction window).
- Grinder (C++ at /tmp/btc109c3/grind.cpp, linked against
  src/crypto/siphash.cpp + build-before libs) validated end-to-end:
  HIT value=50000 with shortid 0x8c4ad8751aa7 matching an
  independent Python SipHash-2-4 implementation; captured real
  cmpctblock shortid 0x46bdb4c6abf9 == computed shortid_of(
  header, nonce, wtxid(T)) — cross-implementation agreement.
- 2^26 grind against the real target shortid: MISS, matching
  prediction (hit probability ~2^-22). Verdict: forced collision is
  ~2^48 work per target per block (GPU-weeks for a determined
  attacker); out of practical remote reach, and even success only
  reaches the H2 gate, not acceptance.

### Evidence — H2: PROVEN LIVE (tampered-prefilled drive)
Rig (/tmp/btc109c3/tamper_drive.py, regtest, 2 nodes):
- n1/n2 synced to 101; MiniWallet self-transfer T in BOTH mempools;
  nodes disconnected; n1 mines B = [coinbase, T].
- Python peer to n2: sendcmpct(announce, v2), headers(B). n2 issues
  getdata MSG_CMPCT_BLOCK (type 4) — solicited compact fetch,
  mapBlocksInFlight entry from our peer (passes the
  "unsolicited compact block" gate at net_processing.cpp:4702).
- We answer with a cmpctblock carrying the CORRECT header, nonce,
  and the CORRECT shortid of T (recomputed with the validated
  Python siphash under selector sha256(header||nonce)), but a
  prefilled coinbase with vout[0].nValue + 1.
- n2 log (debug=cmpctblock, exact lines, run2/node1/regtest/debug.log):
    InitData   PartiallyDownloadedBlock for block 18cf1bd2...
    FillBlock  (reconstruction completed structurally)
    validation.cpp:4199 [IsBlockMutated] Block mutated:
                bad-txnmrklroot, hashMerkleRoot mismatch
  -> READ_STATUS_FAILED -> n2 sends getdata MSG_BLOCK|MSG_WITNESS_FLAG
     (type 1073741826) for B -> we serve the real block -> n2 accepts:
     getbestblockhash() == B. RESULT line: "tampered cmpctblock drove
     mutation-detect + full-block fallback; tip==B".
- Framework deltas needed in this fork (recorded for reuse): no
  P2PInterface.send_message (use send_without_ping); node rejects
  cmpctblock without prior sendcmpct and without an in-flight
  request (net_processing.cpp:4612, 4702); node requests new-tip
  blocks from a v2 peer as MSG_CMPCT_BLOCK type 4.

### Verdict
DISMISSED. The collision attack is ~2^48 per target and terminates
at the IsBlockMutated gate, which is PROVEN to fire on the public
path (bad-txnmrklroot) and to recover via full-block fallback with
no misbehavior score consequence beyond a wasted round-trip. The
worst realistic outcome of a (astronomically expensive) collision
is one redundant block download — bandwidth noise, not consensus or
availability impact.

### Campaign #109: COMPLETE
c1 static map + wrong-vs-malicious partition; c2 live v1/v2 x
hb/lb matrix; c3 collision-class bound + live fallback proof.
All cells closed with executable evidence.

### Limitations
- Grinder miss is a bound, not an impossibility proof (2^48 work is
  brute-force reachable for nation-state GPU budgets); but impact
  after success is capped at fallback noise, so severity stays null.
- Tamper drive used a +1sat coinbase (merkle mismatch). The
  witness-commitment arm of IsBlockMutated shares the same gate and
  was covered by the c1 code read; not re-driven live.
