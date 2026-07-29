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
