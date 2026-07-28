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

## Cycle 2 (2026-07-28): BIP173/350 bech32(m) vector chain — no drift, three-layer mapping complete

Base: cf949ccfea (journal commit for #47 cycle-2 on
audit/build-ci-parity-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/spec-drift-c2 (c1 journal carried in the
carry commit). Start state: clean (untracked scratch only).

### Draw
Random draw over the 62-goal repaired pool (40 pending + 22 CYCLE-1;
#47 excluded as just-cycled): raw=13968128799102658417, seed masked to
63 bits (4744756762247882609), index 33 -> #81.
STATE REPAIR: #81 cycle-1 (0f6c2640b7, BIP324/RFC8439 byte-exact)
lived on audit/spec-vector-drift but had NO row in the linear ledger;
recorded retroactively in this cycle's ledger update. The pool treated
#81 as pending; the draw is honored as cycle 2.

### Version pinning / provenance
bitcoin/bips master fetched 2026-07-28 via raw.githubusercontent.com:
bip-0173.mediawiki (415 lines), bip-0350.mediawiki (334 lines).
Vectors extracted from <tt> markup; test-side strings extracted from
src/test/bech32_tests.cpp literals.

### Comparison (mechanical)
1. Valid bech32 (7: A12UEL5L, a12uel5l, an83...bio1tt5tgs,
   abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw, 11qq...c8247j,
   split1...2y9e3w, ?1ezyfcl): exact match in
   bech32_testvectors_valid, incl. encode-decode re-derivation and
   LocateErrors checks in the test body.
2. Valid bech32m (7): exact match in bech32m_testvectors_valid.
3. Invalid lists: ALL BIP173 invalid strings present in
   bech32_testvectors_invalid — the control-character cases appear
   escaped (" 1nwldj5", "\x7f""1axkwrx", "\x80""1eym55h",
   "de1lg7wt\xff", " 1xj0phk", "\x7f""1g6xzxy", "\x80""1vctc34");
   a naive regex extraction misses these (extraction artifact, now
   documented). BIP350 invalid segwit-address vectors live in the
   correct deeper layers: key_io_invalid.json (bc1gmk9yu etc.) and
   test/functional/rpc_validateaddress.py (bc1pw5dgrnzv,
   bc1rw5uspcuh, ...).
4. Test-side extras (not in BIPs): A12uEL5L/a12UEL5L (mixed-case
   invalid — BIP rule, locally added cases), two mutated checksummed
   variants (abcdef1qpzrz9x8..., abcdef1l7aum6echk45nj2...), and two
   test1zg69... strings (LocateErrors/bech32m-invalid additions).
   All are deliberate ADDITIONS consistent with BIP rules, not drift.
5. Segwit ADDRESS vectors (BC1QW508..., tb1...) are not in
   bech32_tests.cpp by design; verified present in
   rpc_validateaddress.py.

### Verdict
DISMISSED: no spec/vector drift for BIP173/350 across the three test
layers (unit bech32, key_io JSON, functional validateaddress).
Provenance intact; additions are rule-consistent and localized.

### Exact commands
- curl raw.githubusercontent.com/bitcoin/bips/master/bip-01{73,50}.mediawiki
- python3: <tt> extraction + literal extraction + set diffs
  (initial regex under-extracted table-fragment noise; corrected to
  <tt> markup + escape-aware membership checks)
- grep: control-char escapes in bech32_tests.cpp; segwit vectors in
  key_io_invalid.json / rpc_validateaddress.py

### Limitations / queue
- segwit_addr.py (functional framework) is a code PORT of the BIP
  reference implementation, not vectors; port-drift not diffed this
  cycle — queued.
- BIP32 (hd001..hd00N), BIP143/341 sighash, base58, and Wycheproof
  cells unclaimed.
- bkb-mcp not available; primary links recorded inline above.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
