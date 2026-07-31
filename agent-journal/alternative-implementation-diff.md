# Campaign #55 — alternative-implementation-diff

Base: e1d258e71d (journal commit for #38 cycle-2 on
audit/failure-cleanup-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/alt-impl-diff. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): noble-secp256k1 ECDSA differential — 2019/2019 RFC6979 vectors match (+5/5 no-entropy entries)

### Draw
Random draw over the 22-goal pool (16 pending + 6 CYCLE-1; #38
excluded as just-cycled): raw=7089479505672159737, index 9 -> #55.

### Sibling and shared vectors
Sibling: paulmillr/noble-secp256k1 (TypeScript secp256k1 reference).
Shared vectors: test/vectors/secp256k1/ecdsa.json @ main (fetched
2026-07-29): 2019 valid + 5 extraEntropy-empty entries, format
{m: 32-byte msgHash, d: privkey, signature: compact r||s}.

### Differential
Driver (C, secp256k1_ecdsa_sign with nonce_function_rfc6979, NULL
ndata, against build-before's libsecp256k1.a): for each vector, sign
m with d and compare the compact signature byte-for-byte.
- valid: 2019/2019 MATCH.
- extraEntropy entries (all 5 have empty extraEntropy fields): 5/5
  MATCH with the same default path.

### Process notes (honest, for the replay trail)
1. First vector file (rfc6979.json) has a different schema — its
   k0/k1/k15 are aux-entropy variants consumed by their DRBG test,
   not plain signatures; two independent implementations (in-tree
   secp AND a from-RFC python ECDSA) disagreed with it until the
   consuming test file (test/secp256k1.test.ts:17,135-144) showed
   the actual vector path (secp256k1/ecdsa.json with prehash:false).
2. My first differential run hashed the message a second time
   (double-hash) — 10/10 systematic mismatch; semantics fixed by
   reading the sibling's test consumer (m used directly as msgHash).
3. An independent RFC6979+ECDSA reimplementation in python agreed
   with the in-tree secp's behavior before the convention fix —
   ruling out an in-tree implementation divergence; the mismatch was
   harness-side throughout.

### Verdict
- DISMISSED (differential): the in-tree libsecp256k1's RFC6979 ECDSA
  is byte-identical to the sibling implementation on 2019+5 shared
  vectors. No divergence, no local or sibling bug; adapter lessons
  recorded above.
- CONFIRMED (conformance): cross-implementation byte-exact signing.

### Exact commands
- curl noble-secp256k1 test/vectors/secp256k1/ecdsa.json
- gcc driver vs build-before/src/secp256k1/lib/libsecp256k1.a;
  python rfc6979 reimplementation (recorded in conversation)

### Limitations / queue
- extraEntropy vectors with real entropy (ndata path) — the 5 fetched
  were all entropy-empty; ndata-covered vectors queued.
- schnorr/BIP340 sibling vectors (noble's BIP340 set) vs in-tree
  schnorrsig — queued.
- btcd/rust-bitcoin tx-serialization vector differentials — heavier;
  queued.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): extraEntropy/ndata differential — 25/25 vectors + 200/200 randomized, three implementations byte-identical; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=12034554563773083305,
masked=2811182526918307497, n=4, idx=1) -> alt-impl-extrantropy ->
#55 (second cycle; c1 queue cell "ndata-covered vectors"). Branch:
audit/alt-impl-diff-c2 from 2b9cb9b685 (#1 c4 tip).

### Hypothesis
The in-tree secp256k1 RFC6979-with-extra-data nonce path
(secp256k1_nonce_function_rfc6979 + 32-byte ndata) could diverge
from the sibling implementation (noble-secp256k1 extraEntropy) —
concat-order or DRBG-instantiation variant — or be wrong outright.
c1 verified only the no-entropy baseline (the 5 entries' signature
fields).

### Semantics established (from the sibling's consumer, c1 lesson 1)
noble's test/vectors/secp256k1/ecdsa.json 'extraEntropy' section
(5 entries) stores, per entry, the expected signatures for 5 FIXED
32-byte entropy constants (test/secp256k1.test.ts:181-198):
0x00..00, 0x00..01, rand 6e72.., n-1, 0xff..ff. Convention probe:
a python RFC6979+ECDSA implementing the secp256k1 concat order
(seckey32 || msg32 || extra32) reproduced all 5 noble expected
values for entry 0 -> conventions agree bit-exactly.

### Differential (three implementations)
- in-tree C driver vs build-before/src/secp256k1/lib/libsecp256k1.a
  (secp256k1_ecdsa_sign + nonce_function_rfc6979 + ndata):
  25/25 byte-identical to noble's expected values.
- independent python reference (RFC6979 DRBG + EC math, this file's
  recorded commands): 25/25 vs noble.
- in-tree secp vs python over 200 randomized (d, m, ent) cases
  (seed 0xEE55): 200/200 byte-identical.

### Boundary notes (not defects)
- noble accepts ARBITRARY-length extraEntropy (1-byte and 48-byte
  cases in its test); secp256k1's ndata is a fixed 32 bytes by API
  design — the length-variant space is outside the in-tree
  contract, recorded as an API-shape boundary.
- Reachability: Core callers never pass extra entropy (CKey::Sign
  uses the bare RFC6979 path) — the ndata path is library-surface
  only in-tree; verified here for backend correctness.

### Verdict
DISMISSED (differential) / CONFIRMED (conformance): the
RFC6979+ndata nonce path is byte-identical across in-tree
secp256k1, noble-secp256k1, and an independent python reference.
The c1 entropy-coverage gap is closed.

### Exact commands
- curl noble ecdsa.json + secp256k1.test.ts (consumer semantics)
- gcc -O2 -I src/secp256k1/include /tmp/ee_diff.c
  build-before/src/secp256k1/lib/libsecp256k1.a -o /tmp/ee_diff;
  /tmp/ee_diff < /tmp/ee_input.txt -> 25/25
- /tmp/ee_sign + python reference (both recorded above) -> 200/200

### Limitations / queue
- Only the 5 noble entries x 5 constants as fixed vectors; the
  200 randomized cases widen coverage beyond them.
- Remaining queued: schnorr/BIP340 sibling vectors; btcd/
  rust-bitcoin tx-serialization differentials.

## Rotation note
Two cycles; ECDSA baseline + extraEntropy both differential-clean.

## Cycle 3 (2026-07-31): BIP340 schnorr sibling vectors — official CSV vs Python port, in-tree subset, and noble's copy; byte-exact everywhere; DISMISSED

### Draw
RE-RANK draw 140 over the 2-cell queue: raw=15293687238298433213,
masked 6070315201443657405 -> idx 1 -> #55 schnorr/BIP340 sibling
vectors (c1/c2 queue). Branch: audit/alt-impl-diff-c3 from
0a4178cb88.

### Sources (fetched 2026-07-31)
- Official: bips master bip-0340/test-vectors.csv (19 rows:
  indices 0-14 original + 15-18 variable-message-length added
  2022-12).
- Sibling: noble-secp256k1 main test/vectors/secp256k1/schnorr.csv
  (15 rows) — byte-IDENTICAL to official rows 0-14; noble has not
  picked up the 2022-12 additions (same vintage as this tree).

### Differentials
- Python framework (test_framework/key.py schnorr): 19/19 verify
  with expected TRUE/FALSE, 8/8 sign with exact sig (rows 0-3 plus
  15-18 — the variable-length messages 0/1/17/100 bytes pass the
  port). Script /tmp/btc55c2_check.py (preserved; arg order note:
  verify_schnorr(key, sig, msg)).
- In-tree C++ (key_tests bip340_test_vectors): suite green;
  provenance diff — all 14 distinct 64-byte sigs in the file are a
  byte-exact SUBSET of the official CSV, zero drift; the omitted
  official indices 15-18 are the 2022-12 variable-message rows,
  which the in-tree harness cannot express (uint256 msg slot) —
  upstream selection, not drift; same vintage as noble's copy.
- Sibling agreement: noble schnorr.csv == official rows 0-14
  byte-identical (diff shows only the missing 15-18).

### Verdict
DISMISSED: no drift on any level — C++ subset byte-exact, Python
superset 19/19+8/8, sibling identical modulo the 2022 additions.
Remaining queued: btcd/rust-bitcoin tx-serialization differentials
(heavier).

### Limitations / queue
- btcd/rust-bitcoin tx-serialization differentials remain the last
  queued cell of this campaign.
- Variable-length-message BIP340 cases are Python-covered only in
  this tree (harness shape); if upstream ever extends key_tests to
  byte-vector messages, rows 15-18 drop in directly.

## Rotation note
Three cycles; ECDSA baseline, extraEntropy, BIP340 all
differential-clean. One cell (tx-serialization) remains.

## Cycle 4 (2026-07-31): rust-bitcoin tx-serialization fixtures — 4/4 agree (sighash subset, BIP341 identical, huge-witness decode, block round-trip); DISMISSED

### Draw
RE-RANK draw 143 over the 9-cell queue: raw=606380819244863949
(already 63-bit) -> idx 0 -> #55 tx-serialization differential
(the campaign's last queued cell). Branch: audit/alt-impl-diff-c4
from 0742687c2e. CONSTRAINT: no Go/Rust toolchains on this host —
the differential uses rust-bitcoin's PUBLISHED machine-readable
fixtures (bitcoin/tests/data, fetched 2026-07-31) rather than
running their code; btcd arm not runnable here.

### Cells (all from rust-bitcoin master tests/data)
- A legacy_sighash.json (290 rows incl. header): STRICT byte-level
  subset of the fork's in-tree src/test/data/sighash.json (501
  rows) — 289/289 shared rows identical, 0 drift; Core carries 212
  additional vectors. Fork C++ sighash_tests: green.
- B bip341_tests.json: BYTE-IDENTICAL to the official BIP-0341
  wallet-test-vectors.json (bips master, c139's fetch) — the
  agreement chain rust-bitcoin == BIP == fork(C++ 141917/141917) ==
  fork(Python port) closes.
- C huge_witness.hex (1,000,285 B): fork C++ decoderawtransaction
  accepts: txid 73be398c4bdc..., size 500142, vsize 125109, 1-in/
  1-out — the huge-witness stress class parses identically.
- D testnet_block_...4497b.raw (4,319 B): parses to exactly the
  filename's block hash (sha256d of the 80-byte header), recomputed
  merkle root matches the header, re-serialization BYTE-EXACT.

### Verdict
DISMISSED: no tx/block serialization drift against rust-bitcoin's
fixture set on any of the four classes. The campaign's queue is
now EMPTY (c1 ECDSA, c2 extraEntropy, c3 BIP340, c4 tx-serialization
— all differential-clean).

### Exact commands
- curl tests/data fixtures to /tmp/btc55c4/ (preserved);
  provenance: sha/path recorded above, sizes in cell text.
- python3 subset/identity diffs (this journal); decoderawtransaction
  via HTTP POST (argv too long for bitcoin-cli at 1 MB hex);
  framework CBlock parse + sha256d header hash.

### Limitations / queue
- btcd arm not runnable (no Go toolchain); the same vector classes
  are covered by the rust-bitcoin + noble lineage — noted, not a gap
  in the fixture evidence itself.
- C uses the C++ decoder's acceptance, not a byte-level
  re-serialization proof (decoderawtransaction doesn't re-emit);
  D covers re-serialization at block level.
- Campaign #55 EXHAUSTED.

## Rotation note
Four cycles; all differential-clean. Campaign exhausted.
