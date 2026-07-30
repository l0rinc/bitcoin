# Campaign #108 — adversarial-artifact-generation

Base: 8afe43e9a3 (journal commit for #95 cycle-2 on
audit/db-semantics-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/adversarial-artifacts. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): hostile V1-transport peer artifact — 4/4 classes classified correctly, node survives

### Draw
Random draw over the 34-goal pool (21 pending + 13 CYCLE-1; #95
excluded as just-cycled): raw=11798951043027699187, seed masked to 63
bits (2575579006172923379), index 23 -> #108.

### Artifact (/tmp/btc108_peer.py, deterministic, preserved)
A hostile protocol peer at the real socket boundary (deeper than the
fuzz harnesses, which bypass wire framing): python socket driver
against a regtest bitcoind (-debug=net), exercising the production
V1TransportDeserializer with four classes:
A. garbage magic bytes
B. well-formed version then junk-checksum verack
C. well-formed version/verack then 20,000,000-byte headers announcement
D. well-formed version/verack/ping (valid control)

### Results (client view + node debug.log)
- A: node sends its version (187 B) then drops on unreadable stream;
  log "socket closed, disconnecting peer=0".
- B: log "Header error: Wrong checksum (verack, 4 bytes), expected
  40ed057c was 00000000" -> disconnect. Correct: framing errors are
  framing errors, not content misbehavior.
- C: log "Header error: Size too large (headers, 20000000 bytes)" ->
  "receiving message bytes failed, disconnecting". The max-message
  guard fires before any 20 MB read.
- D: pong received (328 B total incl. version+verack); clean close.
Node alive and responsive after all four (getblockcount OK).

### Verdict
- DISMISSED (production robustness confirmed): the V1 transport
  classifies all four hostile classes correctly with no crash, hang,
  or resource hold; each failure mode disconnects at the right layer.
- Artifact value beyond the verdict: the peer driver is reusable for
  future boundary classes (split-packet, slowloris, mid-message
  truncation) — preserved at /tmp/btc108_peer.py with the port map.

### Exact commands
- bitcoind -regtest -datadir=/tmp/btc108_n -port=29001
  -bind=127.0.0.1:29001 -rpcport=29002 -debug=net -daemon
- python3 /tmp/btc108_peer.py; debug.log grep
  'disconnect|mismatch|checksum|magic|error'

### Limitations / queue
- Split-packet and slowloris classes (byte-at-a-time and delayed
  sends) — queued; the artifact takes new chunk schedules directly.
- BIP324 v2 hostile-peer variant (encrypted framing boundary) —
  queued; needs the v2 handshake in the driver.
- Fuzzed-but-plausible content classes (valid frame, hostile payload)
  are the fuzz harnesses' job (#9, #71) — boundary kept distinct.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): split-packet + slowloris classes — all handled correctly (byte-at-a-time completes; 60s timeout enforced)

### Draw
Re-rank draw over the remaining 4-cell queue:
raw=14986909651204603959, masked 5763537614349828151, index 3
(of 4) -> #108 (second cycle; c1 queue cell "split-packet and
slowloris"). Branch: audit/adversarial-artifact-c2 from b3a67a05ff
(#52 c2 journal tip).

### Classes (driver /tmp/btc108_peer2.py, extends c1's artifact)
- E1 byte-at-a-time: full version+verack+ping handshake sent as
  200 one-byte chunks (4 ms apart) -> handshake COMPLETES, pong
  received.
- E2 weird chunks: 73 random-size chunks (1/2/3/7/11 B) ->
  COMPLETES, pong received.
- E3 half-version slowloris: 20 bytes of the version frame, then
  stall -> node disconnects at exactly 60 s (peer_connect_timeout,
  net.h:87).
- E4 no-bytes slowloris: connect and send nothing -> node
  disconnects at 60 s with "socket no message in first 60
  seconds... disconnecting peer" in debug.log.

### Verdict
DISMISSED (robustness confirmed): the V1 transport assembles
arbitrarily fragmented frames correctly at any chunk granularity,
and the 60-second handshake timeout closes both stall shapes at
the expected constant. Matches c1's hostile-frame classes; the
split/stall surface is closed for V1.

### Exact commands
- bitcoind -regtest -datadir=/tmp/btc108_n -port=29001
  -bind=127.0.0.1:29001 -rpcport=29002 -debug=net -daemon
- python3 /tmp/btc108_peer2.py (output above); debug.log grep

### Limitations / queue
- BIP324 v2 hostile-peer variant (encrypted framing boundary)
  remains queued — needs the v2 handshake in the driver.
- Post-handshake stall (after version completes; the 30-min
  inactivity class) untested — long-wall cell, low value.

## Rotation note
Two cycles; V1 split/stall closed. Not exhausted (v2 variant).
