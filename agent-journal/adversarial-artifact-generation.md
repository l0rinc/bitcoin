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

## Cycle 3 (2026-07-30): BIP324 v2 hostile-peer classes — all handled correctly

### Draw
Re-rank draw over the rebuilt 3-cell queue:
raw=11237199852652658799, masked 2013827815797882991, index 1
(of 3) -> #108 (third cycle; c2 queue cell "BIP324 v2 hostile-
peer variant"). Branch: audit/adversarial-artifact-c3 from
991b036dae (#69 c2 journal tip).

### Classes (driver /tmp/btc108_peer3.py on the framework's
EncryptedP2PState)
- V1 control: full v2 handshake + version exchange + short-id
  ping -> pong. OK.
- V2 random-ellswift: random 64B "key" (VALID ellswift by design —
  any 64 bytes decode) + >4 KiB junk with no garbage terminator ->
  node disconnects ("missing garbage terminator"). OK.
- V3 corrupted ciphertext: valid handshake, one flipped byte ->
  "packet decryption failure (20 bytes)", disconnect. OK.
- V4 handshake stall: 20 bytes of ellswift then silence -> 60 s V2
  handshake timeout. OK.

### Driver lessons (recorded)
- v2 message contents framing: SHORT form = [1-byte short id]
  (ping=18, pong=19); LONG form = [0x00][12-byte ascii type] — my
  first version omitted the 0x00 and the node logged "invalid
  message type" for each packet (exactly the right classifier
  behavior, observed in debug.log).
- EncryptedP2PState.complete_handshake takes a STREAM (BytesIO),
  not bytes.

### Verdict
DISMISSED (robustness confirmed): the v2 transport completes only
for correct handshakes, rejects terminator-less garbage streams,
fails decryption on tampered ciphertext, and enforces the 60 s
handshake timeout. V1 (c1-c2) and v2 hostile surfaces are both
closed.

### Exact commands
- bitcoind -regtest -datadir=/tmp/btc108_v2 -port=29003 -bind=
  127.0.0.1:29003 -rpcport=29004 -debug=net -daemon
- python3 /tmp/btc108_peer3.py (output above); debug.log grep
  "garbage terminator|decryption failure|handshake timeout"

### Limitations / queue
- Post-handshake v2 slowloris (inactivity timeout) untested —
  long-wall cell, low value.
- Malformed-ellswift EDGE classes (non-curve points after
  xswiftec inversion) — the framework has the test vectors
  (xswiftec_inv_test_vectors.csv) if a cycle lands here.

## Rotation note
Three cycles; v1+v2 hostile transports closed. Not exhausted
(xswiftec edge vectors).

## Cycle 4 (2026-07-30): xswiftec edge vectors vs production C — 256/256 match, both verifiers green

### Draw
Harvested-queue draw (seed_raw=17579083735087921526,
masked=8355711698233145718, n=7, idx=3) -> xswiftec-edge-vectors cell
-> #108 (fourth cycle; c3 queue cell "Malformed-ellswift EDGE
classes"). Branch: audit/adversarial-artifact-c4 from 92beb0c745
(#42 c5 journal tip).

### Hypothesis
The in-tree C secp256k1 xswiftec_inv (the production BIP324 decode
primitive; secp256k1_ellswift_xswiftec_inv_var,
src/secp256k1/src/modules/ellswift/main_impl.h:168) could diverge
from the BIP324 edge-vector annotations
(test/functional/test_framework/crypto/xswiftec_inv_test_vectors.csv,
32 rows x 8 cases: ok / bad[valid_x(-x-u)] / bad[non_square(s)] /
bad[non_square(q)] / info[v=0]) on accept/reject classification or
output t. Test-gap context: the CSV is consumed in-tree ONLY by the
Python framework self-test (ellswift.py test_elligator_encode_
testvectors) and a lint rule — the C implementation never sees these
vectors in-tree (#81 c2 verified only CSV byte-identity vs upstream).

### Experiment 1 (C driver vs CSV)
Scratch harness /tmp/xsw_driver.c: unity build of secp256k1.c with
ENABLE_MODULE_ELLSWIFT=1 + precomputed_ecmult{,_gen}.c; parses the
CSV, runs secp256k1_ellswift_xswiftec_inv_var per (u,x,case),
compares NONE-vs-t and exact t bytes.
- gcc -O2 -I src/secp256k1 -I src/secp256k1/src /tmp/xsw_driver.c
  src/secp256k1/src/precomputed_ecmult.c
  src/secp256k1/src/precomputed_ecmult_gen.c -o /tmp/xsw_driver
- /tmp/xsw_driver test/functional/test_framework/crypto/xswiftec_inv_test_vectors.csv
  -> rows=32 cases=256 mismatches=0 (driver_exit=0)
(Driver v1 had a CSV parser double-increment bug — fixed before any
vector ran; noted so the green is not misread as a parser artifact:
the fixed parser verified field counts and 64-hex shapes strictly.)

### Experiment 2 (Python differential, second verifier)
- cd test/functional && python3 -m unittest
  test_framework.crypto.ellswift -v -> Ran 5 tests, OK
  (test_elligator_encode_testvectors runs the same CSV against the
  pure-Python xswiftec_inv, including xswiftec(u,t)==x re-encode).

### Verdict
DISMISSED (correctness confirmed): both independent implementations
agree with all 256 annotated edge cases — every rejection class and
every ok output matches exactly. A misclassification here would have
meant ellswift decode of non-canonical/non-curve inputs in the v2
handshake (wrong shared secret, availability-level); no such
divergence exists. Reachability note: any 64-byte peer ellswift is
decodable by design (c3 V2 class), so this edge surface IS the
reachable one — and it is clean.

### Exact commands
- as above; harness preserved at /tmp/xsw_driver.c (+/tmp/xsw_driver),
  CSV path in-tree, unittest module
  test.functional.test_framework.crypto.ellswift.

### Limitations / queue
- The C-side CSV check is NOT wired into any in-tree test — a
  persistent C++ or ctest harness for these vectors is the natural
  hardening follow-up (would convert this scratch verification into
  a regression gate). Queued as an oracle-delivery cell.
- Post-handshake v2 slowloris (inactivity timeout) still untested —
  long-wall cell, low value.
- secp256k1's own exhaustive ellswift suite not rerun here (upstream
  CI covers it; the CSV differential was the gap).

## Rotation note
Four cycles; v1+v2 transports and the ellswift edge-vector axis
closed. Remaining queue: the oracle-delivery hardening cell above.

## Cycle 5 (2026-07-30): xswiftec edge-vector C++ gate delivered (oracle O10), mutation-verified

### Draw
Rebuilt-queue draw (seed_raw=4478165777402910479, masked same,
n=4, idx=3) -> xswiftec-oracle-delivery -> #108 (fifth cycle; c4
queue cell "persistent C-side harness as regression gate"). Branch:
audit/adversarial-artifact-c5 from cc334e9f5d (#74 c4 journal tip).

### What was delivered
The c4 scratch verification converted into a persistent in-tree
gate over the PRODUCTION C decode path (c4 found the CSV was
consumed in-tree only by the Python framework self-test):
- src/test/data/xswiftec_inv_test_vectors.csv — byte-identical copy
  of the framework CSV (cmp-verified; itself upstream BIP324
  reference vectors, #81 c2 byte-drift-clean).
- src/test/CMakeLists.txt — added to target_raw_data_sources
  (embedded as test::data::xswiftec_inv_test_vectors via the
  GenerateHeaderFromRaw rule, same mechanism as asmap.raw).
- src/test/bip324_tests.cpp — new BOOST case
  xswiftec_inv_edge_vectors: parses all 32 rows; for every ok-case
  (non-empty t) builds the 64-byte encoding u||t, runs
  EllSwiftPubKey::Decode() (production secp256k1_ellswift_decode),
  requires IsValid + decoded X == annotated x. Exact-count guards
  (rows==32, encodings==98) catch silent CSV truncation.

### Evidence
- cmake --build build-before --target test_bitcoin -j4 (header
  regenerates via DEPENDS on the CSV).
- build-before/bin/test_bitcoin --run_test=bip324_tests
  -> No errors detected (all 98 decode/x checks pass).
- MUTATION KILLED: row1 case2 t-tail byte flipped (a287466b ->
  a2874600) -> x-comparison fails at bip324_tests.cpp:385; restore
  byte-identical (cmp vs framework copy) -> green.
- First draft asserted encodings>100; actual ok-case count is 98 —
  corrected to the exact count (tighter gate, recorded so the
  initial failure is not misread as a vector problem).

### Verdict
ORACLE DELIVERED (test infrastructure; mutation-verified). The
production C ellswift decode now has a persistent edge-vector
regression gate in the standard suite.

### Limitations / queue
- The bad-case rejection classes (xswiftec_inv returning None) are
  not expressible through the public decode path — they remain
  covered Python-side only; testing them C++-side would require
  exposing secp256k1 internals (rejected: subtree divergence).
- Remaining #108 queue: post-handshake v2 slowloris (long-wall,
  low value). #108 is otherwise complete.

## Rotation note
Five cycles; transports, edge-vector verification, and the
persistent gate are done. Marking #108 QUEUE-COMPLETE except the
low-value slowloris cell.
