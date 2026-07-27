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
(empty — filled per area)

## Next queue
(start P1 with highest-amplification message handlers: INV/GETDATA/ADDR/TX,
then P2 accounting bounds; then P4 handshake state machine)
