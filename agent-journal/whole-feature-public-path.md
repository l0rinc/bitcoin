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
