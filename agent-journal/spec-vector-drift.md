# Campaign #81 — spec-vector-drift

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/spec-vector-drift. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): BIP324 + RFC8439 vector chain — byte-exact, no drift

### Draw
Random draw over the 53-goal eligible pool: raw=5809351032215595712,
index 46 -> #81. Spec chosen: BIP324 (v2 transport) — recent, Final,
and session-adjacent (#53 c1 measured its AEAD timing).

### Version pinning / provenance
Reference: bitcoin/bips master, bip-0324/ directory, fetched
2026-07-28 via raw.githubusercontent.com:
- packet_encoding_test_vectors.csv (8 lines: header + 7 rows)
- xswiftec_inv_test_vectors.csv (33 lines)
- ellswift_decode_test_vectors.csv (77 lines)

### Comparison (mechanical, not eyeball)
1. xswiftec_inv_test_vectors.csv vs
   test/functional/test_framework/crypto/xswiftec_inv_test_vectors.csv:
   diff -> IDENTICAL.
2. ellswift_decode_test_vectors.csv vs
   test/functional/test_framework/crypto/ellswift_decode_test_vectors.csv:
   diff -> IDENTICAL.
3. packet_encoding_test_vectors.csv vs the 7 TestBIP324PacketVector
   calls in src/test/bip324_tests.cpp: regenerated the expected C++
   argument lists from the CSV with the conversion documented in the
   test file itself (lines 171-195) and compared all 7 x 14 fields:
   0 mismatches. (The regex also captured the function definition and
   the conversion-comment as "calls" — a 2-offset extraction artifact,
   corrected by alignment; logged as method note.)
4. RFC 8439 (the AEAD BIP324 builds on): section 2.3.2, 2.4.2, A.1
   series, and the A.5 AEAD vector all present in
   src/test/crypto_tests.cpp (:794, :801, :813-827, :1220).

### Execution evidence
- build-before/bin/test_bitcoin --run_test=bip324_tests ->
  No errors detected.

### Verdict
- DISMISSED (no drift): in-tree vectors are byte-exact against the
  current authoritative source at all four points, and the suite
  executing them is green. No obsolete-draft copies, no missing rows,
  no content edits.

### Why this was worth checking anyway
BIP324 reached Final recently and draft-era vector files could have
lingered; the in-tree CSVs are verbatim copies of the BIP repo files
(a copy is only as fresh as its last sync — today they coincide).

### Limitations
- One spec family covered (BIP324 + RFC8439). Other mapped families
  for future cycles: bip341_wallet_vectors.json (taproot wallet),
  blockfilters.json (BIP158), sighash.json (BIP143), script_tests.json
  (consensus script), bip32_tests (BIP32), asmap.raw.
- Upstream "master" is a moving target; the comparison pins
  today's fetch, not a tagged release (BIPs have no per-vector
  versioning).

### Exact commands / artifacts
- `curl -sf https://raw.githubusercontent.com/bitcoin/bips/master/bip-0324/<file> -o /tmp/r81_*.csv`
- `diff /tmp/r81_xswift.csv test/functional/test_framework/crypto/xswiftec_inv_test_vectors.csv`
- `diff /tmp/r81_elldec.csv test/functional/test_framework/crypto/ellswift_decode_test_vectors.csv`
- python3 field-level comparison (journal history; 0 mismatches)
- `build-before/bin/test_bitcoin --run_test=bip324_tests`

### Next queue for this campaign
- bip341_wallet_vectors.json vs the BIP341 reference (wallet descriptors).
- sighash.json + script_tests.json provenance (are they regenerated
  from the current consensus code or stale imports?).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.
