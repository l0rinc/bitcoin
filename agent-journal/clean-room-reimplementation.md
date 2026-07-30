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
