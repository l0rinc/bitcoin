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
