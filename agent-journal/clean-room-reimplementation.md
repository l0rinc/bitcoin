# Campaign #99 — clean-room-reimplementation

Base: 6949405f82 (journal commit for #73 cycle-2 on
audit/network-state-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/clean-room. Start state: clean (untracked
scratch only).

## Cycle 1 (2026-07-29): CompactSize clean-room differential — 804 cases, 0 mismatches

### Draw
Random draw over the 24-goal pool (11 pending + 13 CYCLE-1; #73
excluded as just-cycled): raw=1432777228634711222, index 14 -> #99.

### Feature and spec
CompactSize (serialize.h:303-366): the consensus wire-length
primitive. Spec written from the protocol rule: n < 253 -> single
byte; n <= 0xffff -> 0xfd + u16le; n <= 0xffffffff -> 0xfe + u32le;
else 0xff + u64le; any encoding below its form's floor is
non-canonical and must be rejected.

### Independent implementation (second agent role, other language)
/tmp/btc99_diff.py: a from-spec Python reference (encode/decode/
reject semantics) with no sight of the C++ source beyond the spec
rule, driving /tmp/btc99_driver (a thin C++ shell over production
WriteCompactSize/ReadCompactSize, compiled from serialize.h with the
node's own flags).

### Differential (804 cases, 0 mismatches)
- encode: n in 0..259 exhaustively + 0xFFFE/0xFFFF/0x10000/0x10001,
  0xFFFFFFFE..0x100000001, 2^63-1, 2^64-1 — byte-exact match on all.
- decode: all 253 non-canonical 253-form encodings (both reject),
  sampled 254/255-form non-canonicals (both reject), legal
  boundaries (both accept with identical values), malformed/truncated
  inputs (both reject; error text differs by design — Python uses a
  class label, C++ the exception message; the ACCEPT/REJECT decision
  is identical everywhere).

### Verdict
- DISMISSED (differential): the C++ implementation is behaviorally
  identical to an independent from-spec reference across the full
  domain — encode bytes, decode values, and reject decisions. No
  divergence to minimize; reference and vectors preserved
  (/tmp/btc99_driver.cpp, /tmp/btc99_diff.py).
- Cross-check with #48 c1: the exhaustive C++ battery and this
  independent differential agree (two independent verifier forms).

### Exact commands
- g++ -O2 -std=c++20 -I src -include streams.h -o /tmp/btc99_driver
  /tmp/btc99_driver.cpp build-before/lib/libbitcoin_util.a
  build-before/lib/libbitcoin_crypto.a
- python3 /tmp/btc99_diff.py

### Limitations / queue
- VarInt (the undo-format cousin) gets the same treatment — queued;
  its #35 battery is C++-side only.
- bech32 decode differential (python reference vs C++ decoder over
  the BIP173/350 tables) — queued; vector tables already proven (#81).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): bech32/bech32m decode differential — spec reference vs C++ decoder: 2433/2433 agree

### Draw
Re-rank draw over the remaining 3-cell queue:
raw=14006312474672917596, masked 4782940437818141788, index 1
(of 3) -> #89 (second cycle; c1 queue cell "bech32 decode
differential"). Branch: audit/clean-room-c2 from a1b39e2291
(#108 c2 journal tip).

### Experiment
Independent from-spec reference decoder (/tmp/btc89_diff.py:
polymod, hrp_expand, case rules, charset, 90-char limit, BECH32
vs BECH32M constants — written from the BIPs, not Core source)
vs a stdin driver over the fork's bech32::Decode
(/tmp/btc89_driver.cpp, direct-compile of src/bech32.cpp +
libs). Corpus (2433): BIP173 valid (6) + BIP350 valid (5) +
BIP173/350 invalid (29) + systematic single-char mutations of 2
valid addresses (~2380) + length/case boundary cases.

### Result
TALLY: agree_accept=11, agree_reject=2422, A(cpp-overaccept)=0,
B(mismatch)=0. Every case agrees: accept decisions, reject
decisions, and full (encoding, hrp, payload) tuples.
(Driver-lesson: the first build printed two nibbles per 5-bit
value — 4 apparent "mismatches" were a formatting artifact,
verified identical per-value after reformat; a reminder that
differential formatting IS part of the oracle.)

### Verdict
DISMISSED (differential clean): the bech32/bech32m decoder is
behaviorally identical to an independent from-spec reference
across the vector tables and the mutation sweep. Third
independent verifier form for this surface (#81 byte-exact
tables, #48 batteries, this).

### Exact commands
- g++ -O2 -std=c++20 -I src -o /tmp/btc89_driver
  /tmp/btc89_driver.cpp src/bech32.cpp
  build-before/lib/libbitcoin_common.a ..._util.a ..._crypto.a
  ..._clientversion.a
- python3 /tmp/btc89_diff.py (TALLY above)

### Limitations / queue
- VarInt differential (c1's queued cousin of the bech32 cell;
  undo-format) — next cell if redrawn.
- Segwit-address layer (convertbits + HRP=bc/tb + program
  length rules) is ABOVE bech32 proper — a separate surface if a
  cycle lands there.

## Rotation note
Two cycles; bech32 closed with a full-agree differential. Not
exhausted (VarInt cousin, segwit-address layer).

## Cycle 3 (2026-07-30): VarInt read/write differential — 5435/5435 agree; the differential caught MY reference's bug

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=8837950375080321296, index 0 (of 2) -> #89 (third cycle; c2
queue cell "VarInt differential"). Branch: audit/clean-room-c3
from 1c49d1e8ff (#108 c3 journal tip).

### Experiment
Independent reference (the (n>>7)-1 quirk encode from #35 c2,
plus a guarded decode for all four widths u8/16/32/64) vs the
fork's Read/WriteVarInt via a VectorWriter/SpanReader driver
(/tmp/btc89v_driver.cpp). Corpus: exact encodings for 0..2099 +
boundary values + 600 random-width values, decode of every
encoded value, and adversarial byte strings (truncated
continuation, overlong forms, width-overflow encodings).

### Result
TALLY: enc_ok=2713 dec_agree=2717 rej_agree=5 A=0 B=0 (5435
total). PROCESS LESSON: the first run showed 8 "mismatches" —
all on truncated-continuation inputs (single 0x80/0xff): my
reference accepted them as complete values; C++ correctly
rejects a continuation bit on the final byte (the encoding is
truncated). The reference was wrong, not the implementation —
the differential did its job by catching the REFERENCE's bug
(why two independent implementations matter; recorded per the
campaign's negative-result discipline).

### Verdict
DISMISSED (differential clean): Read/WriteVarInt matches the
corrected independent reference on every case — exact encodings,
all four widths, and all rejection shapes (overflow guards,
truncated continuation).

### Exact commands
- g++ -O2 -std=c++20 -I src -o /tmp/btc89v_driver
  /tmp/btc89v_driver.cpp build-before/lib/libbitcoin_common.a
  ..._util.a ..._crypto.a ..._clientversion.a
- python3 /tmp/btc89v_diff.py (TALLY above)

### Limitations / queue
- Signed-mode (NONNEGATIVE_SIGNED) differential — same machinery
  with the sign bit; queued nicety.
- The segwit-address layer from c2 remains the other open cell.

## Rotation note
Three cycles; bech32 and VarInt both closed with full-agree
differentials. Not exhausted (signed mode, segwit layer).

## Cycle 4 (2026-07-30): signed-mode (NONNEGATIVE_SIGNED) VarInt differential — byte-identical; another reference bug caught

### Draw
Re-rank draw over the rebuilt 4-cell queue:
raw=14085611517425201985, masked 4862239480570426177, index 1
(of 4) -> #89 (fourth cycle; c3 queue cell "signed-mode
differential"). Branch: audit/clean-room-c4 from 1ef2c65ac8
(#69 c3 journal tip).

### Mechanism finding (first)
NONNEGATIVE_SIGNED is a TYPE marker, not an encoding variant:
CheckVarIntMode's static_asserts only bind signedness to the mode
(DEFAULT=unsigned, NONNEGATIVE_SIGNED=signed); the encode/decode
bodies are mode-identical. The differential therefore reduces to:
signed-width encodings must be byte-identical to unsigned for
in-range values, and signed widths must guard their max on read.

### Experiment
Driver (/tmp/btc89s_driver.cpp) encoding with
NONNEGATIVE_SIGNED for int8/16/32/64 and DEFAULT for uint64,
decoding signed; reference in python. Corpus: 509 encode cases
(0..299, boundaries, random widths), 513 canonical decodes, and
overlong/truncated forms.

### Result
TALLY: ident=509 dec_ok=513 rej_ok=1 B=0 (1023 total). The one
first-run "mismatch" (overlong 8080808080000000) was MY
reference reading past the terminator: ReadVarInt correctly
returns at the FIRST non-continuation byte and leaves trailing
bytes unread; my loop kept consuming. Second cycle in a row the
differential caught a bug in the reference, not the
implementation — the discipline is doing its job.

### Verdict
DISMISSED (differential clean): signed-mode VarInt is
byte-identical where it must be, and the signed guards reject
correctly. The type-marker semantics is recorded (prevents
future misreading as a zigzag-style encoding).

### Exact commands
- g++ ... /tmp/btc89s_driver.cpp (libs as in c2)
- python3 inline differential (TALLY above)

### Limitations / queue
- The segwit-address layer (convertbits + HRP + program rules)
  remains the one open clean-room cell.

## Rotation note
Four cycles; bech32, VarInt, signed VarInt all closed. Not
exhausted (segwit layer).

## Cycle 5 (2026-07-30): segwit-address layer differential — 180/180 matrix cells agree

### Draw
Re-rank singleton (last queue cell): #89 (fifth cycle; c4 queue
cell "segwit-address layer"). Branch: audit/clean-room-c5 from
7301ef36a2 (#65 c11 journal tip).

### Experiment
Full (version x length) matrix: versions 0-17 x lengths
{1,2,19,20,31,32,33,39,40,41} (180 cells), each with a
deterministic program. Encode via key_io's EncodeDestination,
decode via DecodeDestination; compare against the reference rules
(v0: {20,32} bytes + BECH32; v1-16: 2..40 bytes + BECH32M; v17+:
reject). Driver /tmp/btc89a_driver.cpp (link closure:
key_io+bech32+script+block+transaction+merkle+uint256+pubkey+
hash + common/util/crypto/clientversion/secp256k1/univalue libs +
SelectParams(MAIN)).

### Result
TALLY: roundtrip=130 enc_rej_ok=50 dec_rej=0 B=0 (180 total).
All 130 valid cells round-trip byte-exactly (version + program);
all 50 invalid cells encode-reject (empty string; v0 wrong
lengths 1,2,19,31,33,39,40,41 + v1-16 lengths 1,41 + all v17).
BIP173's canonical example address also round-trips.

### Verdict
DISMISSED (differential clean): the segwit-address layer matches
the reference rules at every matrix cell. Clean-room cells now
closed: bech32 (c2), VarInt (c3), signed VarInt (c4),
segwit-address (c5).

### Exact commands
- g++ -O2 -std=c++20 -I src -I src/secp256k1/include -o
  /tmp/btc89a_driver /tmp/btc89a_driver.cpp src/key_io.cpp
  src/bech32.cpp src/script/script.cpp src/primitives/block.cpp
  src/primitives/transaction.cpp src/consensus/merkle.cpp
  src/uint256.cpp src/pubkey.cpp src/hash.cpp <libs>
- python3 matrix differential (TALLY above)

### Limitations / queue
- ConvertBits edge forms (padding variants) are covered by the
  decode path per cell; explicit pad-garbage strings would be the
  paranoid extension — low value after c2+c5.

## Rotation note
Five cycles; the clean-room campaign's cells are closed. Not
exhausted (new reference-able surfaces only).
