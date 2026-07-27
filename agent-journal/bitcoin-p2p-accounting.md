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
| P1 | message processing per msg type | assert/Assume reachable from crafted msg; deser over-read; per-msg work amplification | open |
| P2 | peer accounting / quota | m_addr_known/bloom filter memory per peer; send queue bounds; inventory tracking bounds | open |
| P3 | in-flight block tracking | mapBlocksInFlight bounds; stalls; timeout accounting | open |
| P4 | transport handshake (v1/v2) | pre-VERSION messages; duplicate VERSION; v2 key-exchange failure paths; message before handshake complete | open |
| P5 | discouragement/ban state | misbehavior score accounting; discouragement vs permission bypass; stale state after reconnect | open |
| P6 | disconnect/shutdown cleanup | peer state teardown races; m_peer_map vs CNode lifetime; orphan tracking on disconnect | open |
| P7 | v1/v2 transport parity | checks applied on one transport but not the other; header size limits; garbage/buffer bounds | open |

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

## Next queue
(start P1 with highest-amplification message handlers: INV/GETDATA/ADDR/TX,
then P2 accounting bounds; then P4 handshake state machine)
