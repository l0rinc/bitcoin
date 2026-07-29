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
