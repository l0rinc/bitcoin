# Campaign #73 — network-state-machine

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/network-state. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): replayed/reordered BIP324 packets — oracle gap closed (754add19dd)

### Draw
Random draw over the 38-goal eligible pool: raw=56961841476504158,
index 30 -> #73.

### State model
Post-handshake BIP324 session: per-packet FSChaCha20Poly1305 counter
(AEAD nonce) + a byte-aligned length keystream. Invariant: ciphertext
delivered duplicated or out of order must (a) never be delivered to
net_processing as a valid message, and (b) kill the connection
(misaligned encrypted length -> packet too large, or tag mismatch ->
decryption failure, or small-garbage-length stall -> peer timeout).
Existing coverage: handshake-stage misbehavior matrix
(p2p_v2_misbehaving.py: early key response, excess garbage, wrong
terminator/garbage, no-AAD); prefix detection and downgrade
(p2p_v2_transport.py). NOTHING exercised post-handshake replay/reorder.

### Experiment (deterministic shim, no sleeps beyond framework)
New functional test p2p_v2_replay_reorder.py: NoDecoyState subclass
(the base handshake sends 0-10 RANDOM decoy packets — nondeterministic;
reimplemented complete_handshake without them) + framework v2 cipher
(FSChaCha20Poly1305 with evolving packet counter, verified in
bip324_cipher.py:_crypt).
- Replay: first pkt processed (baseline in node log), identical
  ciphertext again -> "V2 transport error: packet too large" ->
  disconnect; processed ping count stays at baseline.
- Reorder: later-counter packet first -> same class -> disconnect,
  zero processed pings.
- Both: getpeerinfo empty afterwards.
Ran twice for stability; mechanism lines captured from node log
(packet too large 11230082 / 7011299 bytes).

### Harness traps hit and resolved (recorded for future transport tests)
1. v2 packet contents need the type framing (shortid or 0x00+type),
   not the bare payload — first attempt fed raw nonce, giving
   "invalid message type (8 bytes contents)" (decryption WORKED; only
   framing was wrong).
2. build_message() seals with the advancing session cipher — craft
   post-handshake packets only through it.
3. The handshake's random 0-10 decoy packets make byte-level
   expectations nondeterministic — subclass them away.
4. send_version=False peers get "non-version message before version
   handshake" warnings and NO pong — any send_and_ping-style sync
   hangs; sync on node-log counts instead.
5. A misaligned encrypted length either errors (huge -> packet too
   large) or stalls (small -> waits) — bumpmocktime + peertimeout
   covers both without sleeps.
6. assert_debug_log(expected_msgs) requires ALL listed messages;
   alternative failure texts need manual log scans.

### Verdict
- Oracle gap CLOSED (754add19dd). No production defect: replay and
  reorder are never processed; the connection dies on all schedules.
  The BIP324 receive path behaves as designed.

### Limitations
- EOF-at-every-byte of the handshake and half-close not covered
  (queued below).
- v1 legacy transport garbage-scan behavior not re-audited (known
  bounded scanning; separate cell).
- The reorder case's tag-failure text (packet decryption failure vs
  packet too large) depends on the length misalignment; the oracle
  deliberately asserts non-processing + death, not a specific message.

### Exact commands
- `python3 test/functional/p2p_v2_replay_reorder.py --configfile=build-before/test/config.ini --tmpdir=/tmp/btc_r73[x]` (x2, both green)

### Next queue for this campaign
- Handshake EOF sweep: close at each byte offset of ellswift/garbage/
  terminator/version; assert cleanup, no half-open CNode, timeout.
- Half-close (shutdown write side, node keeps reading) on v1+v2.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): handshake EOF sweep — every offset closes clean, zero half-open peers

Base: 49023583e8 (journal commit for #75 cycle-2 on
audit/build-throughput-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/network-state-c2 (c1 journal carried).
Start state: clean (untracked scratch only).

### Draw
Random draw over the 25-goal pool (12 pending + 13 CYCLE-1; #75
excluded as just-cycled): raw=16562845737963747006, seed masked to 63
bits (7339473701108971198), index 23 -> #73. Queued cell from c1:
"Handshake EOF sweep: close at each byte offset; assert cleanup, no
half-open CNode, timeout".

### Artifact (/tmp/btc73_eof.py, deterministic)
Real-socket sweep against a regtest node (-debug=net): BIP324/v2
handshake truncated at N bytes of opaque garbage for N in
{0, 1, 32, 63, 64, 65, 100} (ellswift is 64 B/party), plus a v1
partial version frame (magic + type + 0x40 length + truncated body).
Each case: close immediately, measure getpeerinfo count 0.3-0.5s later.

### Results
- All 7 v2 offsets: peerinfo count = 0 immediately after close; the
  connection is reaped on EOF regardless of handshake phase.
- v1 partial version: peerinfo count = 0 after close.
- Node healthy throughout (getblockcount OK; log shows 8
  socket-closed/disconnecting events exactly matching the 8 cases).

### Verdict
- DISMISSED: EOF at any handshake offset is handled by connection
  teardown with no half-open CNode, no crash, no resource hold —
  the receive-path state machine cleans up on every schedule.

### Exact commands
- bitcoind -regtest -datadir=/tmp/btc73_n -port=29041
  -bind=127.0.0.1:29041 -rpcport=29042 -debug=net -daemon
- python3 /tmp/btc73_eof.py; debug.log disconnect count

### Limitations / queue
- Half-close (shutdown write side, node keeps reading) on v1+v2 —
  queued from c1, distinct from full close.
- Slow-drip ellswift (1 byte/s stalls between offset and completion)
  — timeout-path cell, queued.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-30): half-close (SHUT_WR) on v1+v2 — prompt disconnect, zero CPU spin, no half-open peers; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=11558546368712918904,
masked=2335174331858143096, n=3, idx=1) -> network-half-close ->
#73 (third cycle; c1/c2 queue cell "half-close, distinct from
full close"). Branch: audit/network-state-c3 from af02ccd167
(#49 c9 journal tip).

### Hypothesis
A peer that half-closes (shutdown(SHUT_WR) after a completed
handshake, keeping its read side open) could leave the node with
(a) a half-open peer held indefinitely, or (b) a busy loop on the
repeatedly-readable EOF socket (recv()==0 treated as no-data
instead of close) — the classic read-0 event-loop DoS class,
distinct from c2's full-close EOF sweep.

### Experiment (driver /tmp/hc_probe.py; regtest node
-debug=net, isolated datadir, framework msg_version +
EncryptedP2PState)
- V1: full version/verack, then SHUT_WR. Node: peer drop visible
  at the first poll (drop_at=0.0 s), node's own FIN at 1.0 s,
  debug.log 'socket closed, disconnecting peer=0', peers_end=0.
- V2 (BIP324): complete ellswift handshake + encrypted
  version/verack, then SHUT_WR. Identical: drop at 0.0 s, FIN at
  1.0 s, 'socket closed, disconnecting peer=1', peers_end=0.
- Busy-loop check: node utime+stime delta over 4 s post-half-close
  = 0 jiffies (0.0% of one core) on both.
- Harness lessons (recorded): regtest magic is fa bf b5 da (my
  mainnet f9beb4d9 frame got 'V1 peer with wrong MessageStart');
  ser_string subver is compactsize-prefixed (framework msg_version
  used after first malformed attempt); pkill -f matches the
  launching shell's own cmdline (self-kill, v2).

### Verdict
DISMISSED: both transports treat the read-side EOF as an orderly
close and disconnect promptly (sub-second), leave no half-open
peer, and spin zero CPU — the read-0 busy-loop class is absent on
v1 and v2.

### Exact commands
- bitcoind -regtest -datadir=/tmp/hc_node -port=29003
  -bind=127.0.0.1:29003 -daemon -debug=net
- python3 /tmp/hc_probe.py (output above); debug.log disconnect
  lines recorded.

### Limitations / queue
- The probe measures the node's reaction, not the peer's view of
  node-initiated half-close (node SHUT_WR toward us) — same code
  path from the other side, unmeasured; queued nicety.
- Slow-drip ellswift (1 byte/s stall) remains queued (timeout
  cell).

## Rotation note
Three cycles; handshake EOF, replay/reorder, and half-close all
closed. Slow-drip remains.

## Cycle 4 (2026-07-31): slow-drip ellswift — handshake timeout reaps mid-drip at ~64s, real peer unaffected; DISMISSED

### Draw
RE-RANK draw 145 over the 7-cell queue: raw=15010669081987093008,
masked 5787297045132317200 -> idx 3 -> #73 slow-drip ellswift
(c1/c2 timeout cell). Branch: audit/network-state-c4 from
83d10f09c9.

### Mechanism (net.cpp InactivityCheck, :2048-2102)
Inactivity checks start at connect + m_peer_connect_timeout (60s,
-peertimeout). A drip keeps last_recv fresh, so the 20-minute
send/recv timeouts never fire — but the final gate is
!fSuccessfullyConnected -> unconditional disconnect ("V2 handshake
timeout" for DETECTING/v2, "version handshake timeout" for v1).
Handshake progress does NOT extend the 60s budget: at 1-2 B/s the
64-byte ellswift cannot complete in time, by design.

### Experiment (real-socket probe, isolated regtest pair)
- SLOW-drip (2 s/byte, would need 128s): disconnected at 64s after
  32 bytes; debug.log line "V2 handshake timeout, disconnecting
  peer=1"; getpeerinfo = 0 after.
- CONTROL (second regtest bitcoind, full valid BIP324 handshake):
  still connected at 65s — the 60s gate is handshake-gated, not a
  blanket connection-age limit.
- Harness /tmp/btc73c4_drip.py (preserved). Trap recorded: the
  inbound node reads before sending (DETECTING transport) — a
  probe that recv()s first just times out; drip from t=0.

### Verdict
DISMISSED: the slow-drip class is bounded by the 60s handshake
budget with an explicit, correctly-labeled disconnect; no
half-open state, no resource hold, real peers unaffected.

### Limitations / queue
- FAST-drip garbage (0.4 s/B) completes the ellswift but not a
  valid version exchange — same 60s class, not separately run.
- Node-initiated half-close (SHUT_WR toward us, c3's nicety note)
  remains the only open #73 cell.

## Rotation note
Cycle 4 complete; rotating per uber-goal policy. Not exhausted.
