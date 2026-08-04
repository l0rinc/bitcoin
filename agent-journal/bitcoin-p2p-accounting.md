# Journal: Bitcoin P2P transport, permission, and peer-accounting audit (goal 89)

Campaign: severity-first rotation (cycle 3). Unauthenticated, remotely-reachable
defects rank first: any internet peer is the adversary.
Base: audit/resurrection @ 3d405e533f. Build: build-before (Release gcc).
Test harness: regtest functional tests + test_bitcoin; scratch datadirs only.

## Trust boundary
An unauthenticated inbound peer controls: message bytes, message order/timing,
message sizes (within transport limits), version handshake fields, protocol
version offered, service flags advertised, connection count (subject to
limits), and v1 vs v2 transport choice. It does NOT control: our config,
permissions (unless default-granted), other peers' state, or disk.

## Scope ledger (the queue — verdict per area)

| # | area | hypothesis seeds | verdict |
|---|------|------------------|---------|
| P1 | message processing per msg type | assert/Assume reachable from crafted msg; deser over-read; per-msg work amplification | DISMISSED |
| P2 | peer accounting / quota | m_addr_known/bloom filter memory per peer; send queue bounds; inventory tracking bounds | DISMISSED |
| P3 | in-flight block tracking | mapBlocksInFlight bounds; stalls; timeout accounting | DISMISSED |
| P4 | transport handshake (v1/v2) | pre-VERSION messages; duplicate VERSION; v2 key-exchange failure paths; message before handshake complete | DISMISSED |
| P5 | discouragement/ban state | misbehavior score accounting; discouragement vs permission bypass; stale state after reconnect | DISMISSED |
| P6 | disconnect/shutdown cleanup | peer state teardown races; m_peer_map vs CNode lifetime; orphan tracking on disconnect | DISMISSED |
| P7 | v1/v2 transport parity | checks applied on one transport but not the other; header size limits; garbage/buffer bounds | DISMISSED |

## Verdicts

### P1 (message processing — assert/Assume reachability): DISMISSED

Swept all 60 Assert/Assume sites in net_processing.cpp; examined the 8 clusters
with plausible peer influence. All unfalsifiable by an unauthenticated peer:

1. blockencodings.h:39-45 DifferenceFormatter::Unser — strictly-increasing
   indexes are guaranteed by construction (each index = prev + 1 + n, n >= 0,
   overflow throws). The Assume at net_processing.cpp:4381 (GETBLOCKTXN) is
   provably unreachable. Dismissed with proof.
2. net_processing.cpp:4705 (CMPCTBLOCK unsolicited) — Assume immediately
   preceded by the identical condition + return at 4701-4704. Dismissed.
3. net_processing.cpp:3548/4798 (LookupBlockIndex Assume in compact-block
   paths) — CMPCTBLOCK handler returns early when prev unknown (4627-4633);
   header is index-inserted via ProcessNewBlockHeaders (4647) before any
   Assume; block-index entries are never deleted at runtime. Dismissed.
4. Pre-VERSION filtering (3884-3888): any non-VERSION message before the
   handshake is dropped; duplicate VERSION dropped at 3648-3651; redundant
   VERACK dropped at 3891-3894. The VERACK-path Assumes (3921-3929 and
   similar at 3968-3969, 4043-4044) are set by our own VERSION handler.
   Handshake state machine tight. Dismissed.
5. VERSION deserialization (3662-3709): truncated streams set failbit →
   exception → standard deserialization-failure disconnect path
   (IsExpectedPeerMessageDeserializationFailure, 4063). Dismissed.
6. Guarded Assumes that return instead of abort (3246 package size,
   5584 addr bound, 754/757 feature id/data bounds): fail-safe direction.
   Dismissed.

Remaining P1 sub-area (work amplification per message type — INV/GETDATA/ADDR
rates) deferred to P2, where the accounting bounds live.

### P4 (transport handshake v1/v2): DISMISSED (merged into P1 items 4-5)

The handshake state machine (VERSION-first enforcement, duplicate suppression,
VERACK ordering, pre-version drop) is identical for both transports because
filtering happens above the transport layer. v2 key-exchange failure paths
remain to be verified in net.cpp (queued).

### P2 (peer accounting / quota): DISMISSED — all per-peer structures bounded

Hypothesis: unauthenticated peer could grow per-peer memory unboundedly via
repeated GETBLOCKS (m_blocks_for_inv_relay) or GETDATA (m_getdata_requests).
Both refuted by tracing the drain interleaving:

1. LOAD-BEARING INVARIANT (net.cpp:3232-3242 ThreadMessageHandler): for each
   node, SendMessages(node) runs immediately after EVERY single-message
   ProcessMessages(node). A peer's queued messages are processed one at a
   time with a full drain pass between them. Any code that accumulates
   per-peer state "until SendMessages" therefore has a transient bound of
   one message's worth. If this interleaving ever changes (batch processing
   before SendMessages), the GETBLOCKS accumulation below becomes a ~266x
   memory-amplification DoS (83k 60-byte GETBLOCKS in the 5MB recv window ×
   500 × 32B hashes ≈ 1.3GB transient per peer). Worth a comment or test?
   Noted as fragility, not a current defect.
2. m_blocks_for_inv_relay (net_processing.cpp:4363): ≤500 hashes per
   GETBLOCKS message, unconditionally clear()ed at 6147 in the SendMessages
   pass that follows each message. Transient bound ~500 entries. Bounded.
3. m_getdata_requests (4302): appended ≤50000 invs per GETDATA (MAX_INV_SZ
   punished at 4261-4264), drained inline by ProcessGetData (4303) and at
   the top of each ProcessMessages (5230-5233); new messages are not polled
   while the deque is non-empty (5243-5248). Transient bound 50000 invs.
   Send-side throttled by fPauseSend (2618). Bounded.
4. m_addr_known: CRollingBloomFilter, fixed allocation by construction.
5. m_addrs_to_send: MAX_ADDR_TO_SEND cap enforced at push site (guarded
   Assume at 5584). Bounded.
6. vExtraTxnForCompact: ring buffer capped at m_opts.max_extra_txs
   (default 100), 1934-1940. Bounded.
7. Receive side: m_msg_process_queue_size pauses recv at m_recv_flood_size
   (5MB, net.cpp:4130/4142). Send side: fPauseSend thresholds. Bounded.

Verdict: no unbounded per-peer memory found on the unauthenticated surface.

### P3 (in-flight block tracking): DISMISSED — bounded at every insertion path

1. Per-peer in-flight cap (MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16) is enforced at
   all three BlockRequested call paths: headers direct-fetch (2938 gate),
   compact-block reconstruction (4727 gate), and FindNextBlocksToDownload
   budget (6337-6347, budget = 16 - in-flight). A peer cannot exceed 16
   in-flight blocks (~64MB worst case, but only for blocks we requested).
2. Per-block compact in-flight cap (MAX_CMPCTBLOCKS_INFLIGHT_PER_BLOCK)
   enforced at 4727; Assumes at 1243/1278 are downstream of that gate.
3. In-flight entries are only created for blocks WE requested (or compact
   blocks from announced high-bandwidth peers, gated as above); an
   unauthenticated peer cannot insert entries by itself.
4. Stall accounting: m_stalling_since + BLOCK_STALLING_TIMEOUT marks the
   staller and redownloads elsewhere (nodeStaller return path); sustained
   stalls hit BLOCK_DOWNLOAD_TIMEOUT → disconnect. Worst case an attacker
   wastes one download slot, not memory.
5. Headers-presync (headerssync.cpp): m_header_commitments hard-capped by
   m_max_commitments (213, peer flagged malicious above); m_redownloaded_headers
   capped by redownload_buffer_size (311-320). Area is heavily fuzzed
   (src/test/fuzz/headerssync.cpp). Bounded.
6. Disconnect cleanup of in-flight state (FinalizeNode → RemoveBlockRequest)
   verified present at 1253-1267; deeper disconnect-lifecycle checks are P6.

### P5 (discouragement/ban state): DISMISSED — permission gates correct, state fresh per connection

1. Misbehavior model: any Misbehaving() sets m_should_discourage (1948);
   MaybeDiscourageAndDisconnect (5177-5214) disconnects and discourages the
   address. Score-based accumulation is gone — one strike, by design.
2. Permission bypass checks (5188-5207): NoBan (config-granted only),
   IsManualConn, IsLocal — none are settable by an unauthenticated remote
   peer. Verified the flags come from connection setup, not message content.
3. Reconnect from a discouraged address is rejected at connection handling
   (5854: IsDiscouraged/IsBanned → immediate disconnect). Discouragement is
   address-scoped in banman and persisted to banlist.dat.
4. m_should_discourage lives on the Peer object, destroyed with the
   connection — no stale ban state carries to a new NodeId.
5. KNOWN DESIGN TRADEOFF (not a defect): inbound onion peers get a fresh
   random address per connection, so they are disconnected but not
   discouraged (5200-5207 comment) — discouragement would be meaningless.
   A malicious Tor peer can reconnect-and-misbehave in a loop, but each
   iteration costs only a transient connection slot. Documented in code.

### P6 (disconnect/shutdown cleanup): DISMISSED — teardown complete with global consistency asserts

FinalizeNode (net_processing.cpp ~1700-1783) removes every per-peer state:
mapBlocksInFlight entries erased (with m_peers_downloading_from accounting),
m_txdownloadman.DisconnectedPeer, m_txreconciliation ForgetPeer,
m_headers_presync_stats erase (under its mutex), private-broadcast slot
returned to the pool, and all download counters decremented. When the last
peer is removed, hard asserts verify global consistency: mapBlocksInFlight
empty, all counters zero, txdownloadman empty (1757-1765). Peer object is
removed from m_peer_map via RemovePeer. Shutdown: flagInterruptMsgProc is
checked per message and per loop iteration; connman joins the message
handler before tearing down. No stale-state or use-after-teardown path found.

### P7 (v1/v2 transport parity): DISMISSED — checks are transport-agnostic; v2 state machine fuzzed

1. All message-level checks (pre-VERSION drop, per-message size caps,
   inventory limits, misbehavior) live in net_processing above the transport
   layer — verified in P1/P4 that no check branches on transport type.
2. V2Transport (net.cpp): key-exchange and packet state machine is
   deterministic over input bytes; all malformed inputs (wrong keys, garbage
   terminator mismatch, AAD/decrypt failure, oversized packets) return
   errors → disconnect. The Assumes in the transport guard internal
   invariants (buffer caps by construction per state, state enums), not
   peer-controlled values.
3. Fuzz coverage matches the exact parity surface: dedicated targets
   p2p_transport_bidirectional_v2 and p2p_transport_bidirectional_v1v2
   (src/test/fuzz/p2p_transport_serialization.cpp:417-434) fuzz v2↔v2 and
   v1↔v2 interop with randomized garbage and keys.

## Cycle verdict

All 7 ledger areas verdict-locked: P1, P2, P3, P4, P5, P6, P7 all DISMISSED
with file:line evidence. No confirmed defect on the unauthenticated P2P
surface in this audit pass; no fix commits required. Two fragilities noted
for the record (not defects): the ThreadMessageHandler drain interleaving is
load-bearing for m_blocks_for_inv_relay bounds (P2.1), and inbound-onion
misbehavior is disconnect-only by design (P5.5).

## Next queue
Goal 89 audit pass complete (all 7 areas DISMISSED with evidence). Residual
for future cycles:
- Dynamic validation of the P2.1 load-bearing invariant: a functional test
  that floods GETBLOCKS and asserts m_blocks_for_inv_relay stays bounded
  would convert the fragility note into a regression oracle (needs a
  debug-only accessor; judged not worth the surface change for a non-defect).
- Orphanage and txdownloadman adversarial sequences (ProcessOrphanTx paths)
  belong to the mempool-accounting campaign (goal 87) — cross-reference.
- Rotation: next campaign per severity-first order — goal 88 (wallet
  key-loss) was passed over for this goal; return to it or to goal 86
  (chainstate crash-symmetry).

## Branch assessment (2026-08-04, author flood+1): l0rinc/bip35-relay-capacity c83a7c79f9 — NOT-APPLICABLE to this lineage (rebase-watch)

### Claim
Inbound fRelay=false peer escapes inbound tx-relay capacity
accounting via BIP35 mempool requests; author's fix reclassifies
(m_relays_txs=true) + MaybeDisconnectForTxRelayCapacity before
queuing the response.

### Lineage check (failing the adoption at the mechanism step)
- Our net_processing has the OLD handler: no reclassification,
  CONFIRMED (net_processing.cpp:5044-5070).
- BUT our lineage LACKS the entire inbound-relay capacity
  machinery the fix hooks: zero MaybeDisconnectForTxRelayCapacity,
  zero -inboundrelaypercent, no eviction-candidate logic, and
  m_relays_txs is WRITE-ONLY here (set at 3797/5119/5167, no
  consumers). There is no capacity counter for a BIP35 peer to
  evade — the defect is specific to upstream's newer feature
  (post-base 18c05d9301).
- The author's test file is upstream-current content (+117 lines
  vs our suite); our lineage doesn't carry
  p2p_connection_limits.py at all (untracked here — removed after
  assessment).

### Verdict
DISMISSED for this tree (mechanism not realizable); severity
Medium-low even upstream (DEFAULT_PEERBLOOMFILTERS=false gates
reachability; existing OutboundTargetReached gate bounds
amplification). REBASE-WATCH: when the fork rebases past the
upstream capacity-eviction feature, this branch becomes the fix
(upstream master 1ed14c6122 still carries the TODO, author's
test flips it; adopt then if upstream hasn't merged it).

## Cycle 332 (2026-08-04) — upstream #35832 assess-and-adopt (getblocktxn/bloom hardening)

Upstream merged #35832 (975a314667): two new net_processing checks,
both now in-tree via the 17c5e33e9c rebase.

1. 28641fd195 — GETBLOCKTXN with empty req.indexes -> disconnect
   (previously: passed deserialization, vacuously passed the
   strictly-increasing loop, then did a pointless block disk read).
   Sender-side safety PROVEN: our sender never emits empty indexes —
   net_processing.cpp:5014-5015 sets fProcessBLOCKTXN=true and skips
   the send when nothing is missing; GETBLOCKTXN is only pushed in
   non-empty branches. Disconnect cannot punish an honest peer.
2. 9871fb726c — MSG_FILTERED_BLOCK inv from a peer when we don't
   advertise NODE_BLOOM -> disconnect BEFORE the block lookup
   (previously: ignored only after reading the block from disk).
   BIP37 filtered blocks require NODE_BLOOM; the request is
   protocol-invalid by construction. No interplay with our lineage:
   the bip35/tx-relay-capacity machinery remains absent here
   (cycle-327 NOT-APPLICABLE stands), and no fork hunks touch either
   function.

Verdict: DISMISSED as a defect source (correct, well-scoped DoS
hardening; disk-read avoidance confirmed by code motion only, not
profiled). Adopted by rebase. p2p_getdata.py (upstream's new
split-subcase form) + p2p_compactblocks.py run as the executable
confirmation — result recorded in the ledger entry for cycle 332.

### Delay-queue subsystem scout (delta-A, commits df31ee57aa/026f70e05f/e1b7490fbc/46c8c471dc/74a47a5207/4842903ac1/6307bd034b)

Upstream replaced per-peer tx-inv rate limiting with a GLOBAL dual
token-bucket delay queue (util::TokenBucket<NodeClock>: count 14/s
cap 420, size 12MB/600s cap 50MB; new util/tokenbucket.h). Review
notes:

- TokenBucket arithmetic SOUND by inspection: refill clamped to cap,
  backward-time safe (no refill, baseline update only), deficit
  spending deliberate (decrement may take balance negative; senders
  gate on value()).
- Backlog = std::vector<Wtxid> per direction (inbound/outbound);
  InitiateTxBroadcastToAll pushes unconditionally; entries are 32-byte
  wtxids only. No explicit size cap — bounded indirectly: every
  broadcast tx passed ATMP (mempool caps distinct live txs); stale
  wtxids of evicted txs drain at bucket rate via
  ExtractBestByMiningScoreWithTopology skipping missing entries.
  No unbounded-state defect identified; DROPPED as a cell.
- Capacity hygiene: shrink-to-300-cap only when backlog empties
  (line ~2436) — under sustained flood capacity stays, which is then
  justified. Our retained-capacity shrink_to_fit (cycle-263 adoption)
  survives in the rewritten per-peer drain lambda (line ~6472).
- Accepted tradeoff (not a bug): one flooder can consume the global
  bucket and delay honest relay; upstream chose this over per-peer
  storage/compute costs. Watch cell: if a public report shows global
  starvation in the wild, re-open with a starvation-differential
  experiment (flood one inbound, measure honest outbound delay).

### bip35 sequencing note (delta-A 46c8c471dc)

Upstream now bumps m_last_inv_sequence to m_mempool.GetSequence()
explicitly when answering BIP35 mempool requests, instead of relying
on the incidental bump from the normal INV path. This guarantees a
follow-up GETDATA for any just-announced tx is honored (announced set
and the sequence watermark are consistent by construction). In-tree
via the rebase. The cycle-327 NOT-APPLICABLE verdict for the author's
inbound-capacity machinery is unaffected (different mechanism; still
absent upstream).
