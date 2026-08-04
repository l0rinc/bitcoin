# Campaign #107 — conformance-test-transplant

Base: 92458c9398 (journal commit for #59 cycle-2 on
audit/supply-chain-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/conformance-transplant. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): RFC 4231 case 5 transplanted for HMAC-SHA-256 and HMAC-SHA-512 (0d36c6cd80)

### Draw
Random draw over the 27-goal pool (14 pending + 13 CYCLE-1; #59
excluded as just-cycled): raw=6086368032283125981, index 18 -> #107.

### Gap analysis
src/test/crypto_tests.cpp: both hmac suites cover RFC 4231 cases
"1, 2, 3, 4, 6 and 7" — case 5 ("Test With Truncation") missing.
Reason it was skipped: the RFC's expected value is truncated to 128
bits, and Bitcoin's CHMAC_SHA256/512 always produce the full 32/64-
byte output. The transplantable relationship: the RFC truncated value
IS the full output's first 16 bytes.

### Transplant (provenance: RFC 4231 section 4, case 5)
Independent recompute with python's hmac (authoritative
implementation) and byte-compare against the RFC text:
- SHA-256 full: a3b6167473100ee06e0c796c2955552bfa6f7c0a6a8aef8b93f
  860aab0cd20c5 (first 16 B = RFC truncated vector).
- SHA-512 full: 415fad6271580a531d4179bc891d87a650188707922a4fbb366
  63a1eb16da008711c5b50ddd0fc235084eb9d3364a1454fb2ef67cd1d29fe67
  73068ea266e96b (first 16 B = RFC truncated vector).
Added as case 5 in both suites with the truncation relationship
documented in the comment; suite headers updated "1, 2, 3, 4, 6 and 7"
-> "1, 2, 3, 4, 5, 6 and 7". crypto_tests: No errors detected.

### Verdict
- CONFIRMED (conformance gap closed): all 7 RFC 4231 cases now
  covered for both hashes; the implementation matches the independent
  reference byte-for-byte on the previously missing case.
- Mismatch class: none — intentional API difference (untruncated
  output), handled by verifying the full output whose prefix is the
  RFC value.

### Exact commands
- python3 hmac recompute (above); ninja -C build-before
  bin/test_bitcoin && test_bitcoin --run_test=crypto_tests

### Limitations / queue
- Wycheproof AES-CBC vectors for ctaes (bit-sliced backend) — the
  larger transplant candidate; queued for c2.
- BIP340 schnorr vectors: subtree suite already covers them (#69's
  int128/int64 suites both green) — no gap.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): Wycheproof AES-CBC-PKCS5 transplant — 24/24 valid + 48/48 invalid rejected, openssl cross-verified; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=7779331150053916422, masked
same, n=2, idx=0) -> conformance-aes-cbc -> #107 (second cycle; c1
queue cell "Wycheproof AES-CBC vectors for ctaes"). Branch:
audit/conformance-transplant-c2 from 1e21c1bfa9 (#68 c3 tip).

### Hypothesis
The bit-sliced ctaes AES backend under the in-tree AES-256-CBC
wrapper (wallet key encryption path) could diverge from the
Wycheproof AES-CBC-PKCS5 vectors — block math error, CBC chaining
bug, or padding-validation acceptance.

### Vectors and semantics
C2SP/wycheproof testvectors_v1/aes_cbc_pkcs5_test.json (fetched
2026-07-30): 216 tests total; 72 with 32-byte keys (24 valid +
48 invalid, flags BadPadding/NoPadding/Pseudorandom). Wrapper
semantics from aes.cpp: Encrypt pads PKCS#7; Decrypt constant-time
padding check returns 0 on failure (:111-127) — "rejection" means
return 0 (not -1). In-tree coverage before this cycle: 9 NIST-ish
TestAES256CBC cases, no Wycheproof.

### Differential
Driver /tmp/wp_aes_driver.cpp vs build-before libs
(-lbitcoin_crypto -lbitcoin_util -lsecp256k1):
- valid: Encrypt == ct byte-exact, Decrypt == pt byte-exact:
  24/24.
- invalid (padding-oracle class): Decrypt returns 0 (rejected):
  48/48.
- Second verifier: openssl 3.0.13 enc -aes-256-cbc over the same
  24 valid vectors: 24/24 byte-identical.

### Boundary note (not a defect)
Encrypt of an EMPTY message returns 0 (no padding block emitted) —
upstream-identical early-return (aes.cpp:96-97 'if (!data || !size
|| !out)'). The Wycheproof empty-message vector instead asserts
enc_ret==0 + decrypt-to-empty (verified); openssl emits the
padding block for the same input (24th vector matched there).
Recorded as an API-shape boundary; the wallet never encrypts
empty data.

### Verdict
DISMISSED (differential) / CONFIRMED (conformance): the ctaes
backend matches Wycheproof AES-CBC-PKCS5 on every 32-byte-key
vector, invalids properly rejected, cross-verified by an
independent implementation.

### Exact commands
- curl testvectors_v1/aes_cbc_pkcs5_test.json; python3 filter
  (64-hex keys) > /tmp/wp_input.txt (72 lines)
- g++ -O2 -std=c++20 -I src /tmp/wp_aes_driver.cpp
  -L build-before/lib -lbitcoin_crypto -lbitcoin_util
  -L build-before/src/secp256k1/lib -lsecp256k1
- /tmp/wp_aes_driver < /tmp/wp_input.txt -> 24+48, 0 mismatches
- openssl enc -aes-256-cbc -K <key> -iv <iv> -nosalt per vector

### Limitations / queue
- 16/24-byte keys not run (the in-tree wrapper is AES256-only;
  the shared block primitive is covered by the 32-byte run +
  ctaes's own suite).
- ct64 alignment checks left to the harness (all Wycheproof ct
  are block-aligned by construction).

## Rotation note
Two cycles; HMAC and AES-CBC transplants both clean.

## Cycle 3 (2026-08-02, draw 178, raw=11916844877793972431, masked 2693472840939196623, idx 55/56): Wycheproof chacha20_poly1305 transplant — 316/316 (256 valid byte-exact + round-trip, 60 invalid tags rejected), second verifier (python cryptography/OpenSSL) 0 mismatches; CONFORMANCE CONFIRMED, differential DISMISSED

### Cell selection
In-tree crypto_tests has only ~4 RFC 8439 ChaCha20Poly1305
cases (crypto_tests.cpp:1220-1264); the Wycheproof suite (edge
AAD/msg sizes, invalid tags, boundary lengths) is the coverage
gap. Primitive is BIP324 transport crypto — in scope.

### Transplant
- Vectors: wycheproof main testvectors_v1/
  chacha20_poly1305_test.json, filtered to 32-byte key + 96-bit
  nonce (the in-tree AEAD's contract) -> 316 cases.
- Driver (/tmp/btc107c3/wp_chacha_driver.cpp, preserved):
  AEADChaCha20Poly1305 Encrypt byte-exactness vs ct||tag AND
  Decrypt round-trip; invalid-tag cases must fail Decrypt.
  Nonce96 is pair<uint32_t,uint64_t> LE (chacha20.h:47-55) —
  recorded; linked against build-before libbitcoin_crypto.
- Result: n=316 valid_ok=256 invalid_rejected=60 fail=0.
- Second verifier (independent implementation): python
  cryptography ChaCha20Poly1305 (OpenSSL) re-encrypts all 316:
  valid-mismatch=0. Chain Wycheproof == in-tree == OpenSSL.

### Verdict
CONFORMANCE CONFIRMED / defect hypothesis DISMISSED: the in-tree
AEAD matches the external suite on every applicable vector,
invalid tags are always rejected (no tag-truncation or
comparison-lax case accepted).

### Exact commands
- curl wp.json; python filter (316); g++ -O2 line above;
  ./wp_driver < wp_input.txt (RESULT above); python crosscheck
  (0 mismatches).

### Limitations / queue
- XChaCha20-Poly1305 not applicable (in-tree AEAD is 96-bit only,
  matching BIP324).
- FSChaCha20Poly1305 (the rekeying wrapper) covered by its own
  in-tree tests; not part of this external suite.

## Cycle 350 (2026-08-04, r185) — BIP352 vectors transplanted-and-executed

Draw r185 (raw=5661128680711004139 -> #107). Reopen PASS: the
secp256k1 subtree update (c26d4e2d6f) added the OFFICIAL BIP352
conformance vectors (bip352_send_and_receive_test_vectors.json, 28
groups) — present in-tree but never executed by Core builds (Core
does not build secp256k1's own test suite).

Transplant experiment: standalone subtree build (cmake
-DCMAKE_BUILD_TYPE=Release, tests ON, exhaustive/ctime/bench/examples
OFF, /tmp/sp-build scratch) — /tmp/sp-build/bin/tests full suite
GREEN (rc=0, 127.8s), with MAKE_TEST_MODULE(silentpayments)
registered (tests.c:8076) and the module's tests_impl.h (which drives
the BIP352 vector JSON) included. The official vectors pass against
the exact module code compiled into Core's libsecp256k1.

Verdict: conformance CONFIRMED for the compiled-in module (execution
evidence, not just presence). Feeds watch cell
W-silentpayments-first-caller (cycle 347): the module's compiled
state is vector-clean at this revision. Scratch build deleted
(disk). No Core defect; no Core change made.
