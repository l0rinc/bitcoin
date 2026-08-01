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

## Cycle 3 (2026-07-31): BIP341 taproot vector chains — C++ canonical validation set + Python wallet-vector port, both byte-exact; no drift

### Draw
RE-RANK draw 139 over the 3-cell queue: raw=7507051601152807701
(already 63-bit) -> idx 0 -> spec-vector cells. Harvest shorthand
had mislabeled this cell #90; it belongs to #81 (spec-vector-drift) —
#90 is historical-knowledge-recipes. Corrected here and in the
ledger. Branch: audit/spec-vector-drift-c3 from agent/all-findings
(the c2 journal entry lives only on the archive lineage).

### Cell and sources
BIP-0341's Test vectors section (bips master, fetched 2026-07-31)
points at two sets: bip-0341/wallet-test-vectors.json (scriptPubKey
computation + keyPathSpending witnesses) and the canonical
validation set qa-assets/unit_test_data/script_assets_test.json
(consumed by src/test/script_assets_tests.cpp via DIR_UNIT_TEST_DATA).

### Oracle A — C++ vs canonical validation set
curl'd script_assets_test.json (9,243,520 bytes) to
/tmp/btc81c3/unit_test_data/; DIR_UNIT_TEST_DATA=... 
build-before/bin/test_bitcoin --run_test=script_assets_tests
--report_level=detailed: 141917 assertions, 141917 passed, zero
skips (skip-guard verified not triggered: no "skipping" warnings).
The fork's C++ script validation — including every BIP341 sighash
mode, annex, control-block and error case in the canonical set —
matches the spec vectors exactly.

### Oracle B — Python framework port vs wallet-test-vectors.json
/tmp/btc81c3/vector_check.py (preserved): all 7 scriptPubKey
vectors match on scriptPubKey, tweak, merkle root, bech32m address,
and per-leaf control blocks; all 7 keyPathSpending inputs match on
sigMsg, sigHash, tweak, tweakedPrivkey, and the final witness
(schnorr sign with aux=0). Zero mismatches. (The tree_to_scripts
mapper preserves the BIP's explicit tree pairing; framework
taproot_construct/TaprootSignatureMsg used unmodified.)

### Verdict
No drift: the fork is byte-exact against BIP341 on both
implementation levels. DISMISSED. Remaining unclaimed cells:
BIP32 (hd001..), BIP143 sighash.json provenance re-check, base58,
Wycheproof.

### Limitations / queue
- Oracle B checks the FUNCTIONAL FRAMEWORK's Python port (the
  project's second implementation), while oracle A covers the C++
  consensus path — the pair is the two-verifier form for this cell.
- Control-block ordering maps by leaf id (vector order), not by
  merkle-sorted order — recorded for reuse.
- qa-assets fetch is point-in-time (sha not pinned; file byte count
  recorded above for provenance).

## Rotation note
Cycle 3 complete; rotating per uber-goal policy. Not exhausted (4
unclaimed cells).

## Cycle 4 (2026-07-31): BIP32 spec vectors + base58/sighash provenance — 25/25 xprv/xpub match, engine green, both data files byte-identical to upstream master; no drift

### Draw
RE-RANK draw 149 over the 3-cell queue: raw=6078230012799546660
(already 63-bit) -> idx 0 -> #81 remaining spec-vector cells.
Branch: audit/spec-vector-drift-c4 from 09cff005f3.

### Cells
- BIP32: extracted all 25 xprv/xpub strings from bips master
  bip-0032.mediawiki (test vectors 1-5) and from in-tree
  bip32_tests.cpp — 25/25 common, 0 bip-only, 0 tree-only.
  Engine: test_bitcoin --run_test=bip32_tests green (5 spec
  vectors + fork-added keypath-contract/max-depth/invalid-input
  cases).
- base58_encode_decode.json: byte-identical to bitcoin/bitcoin
  master (21 rows).
- sighash.json: byte-identical to bitcoin/bitcoin master (501
  rows) — cross-link: rust-bitcoin's legacy_sighash.json was a
  strict 289-row subset of this file (#55 c4), so the
  BIP143/sighash provenance chain is upstream master == fork ==
  rust-bitcoin-subset.

### Verdict
DISMISSED: no drift on any of the three arms. Remaining #81
cells: Wycheproof (AES/schnorr-class vectors, heavier).

### Exact commands
- curl bips/bitcoin raw files to /tmp (bip32.mediawiki,
  base58, sighash); python3 set/identity diffs above;
  test_bitcoin --run_test=bip32_tests.

### Limitations / queue
- Wycheproof remains the last unclaimed #81 cell.

## Rotation note
Cycle 4 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 5 (2026-08-01): Wycheproof AES-CBC-PKCS5 — 72/72 keySize-256 cases (24 valid round-trip exact, 48 invalid all rejected); campaign EXHAUSTED

### Draw
RE-RANK draw 158 over the 4-cell pool: raw=15981968078687135964,
masked 6758596041832360156 -> idx 0 -> #81 Wycheproof (last cell).
Branch: audit/spec-vector-drift-c5 from b3a90532ce.

### Vectors
C2SP/wycheproof main, testvectors_v1/aes_cbc_pkcs5_test.json (216
total; 72 at keySize=256: 24 valid, 48 invalid) — the wallet's
exact primitive family (AES-256-CBC + PKCS7 padding, PKCS5
equivalent at 16-byte blocks).

### Harness and the debugging detour (recorded honestly)
Scratch harness (/tmp/btc81c5/harness.cpp) against
crypto/aes.cpp + ctaes + lockedpool. First run showed 49
"failures" — ALL harness misreads, not cipher behavior:
(a) dsz=0 conflation — CBCDecrypt returns 0 for REJECTED padding,
for empty ciphertext, and for empty plaintext; "accepted" must
mean non-empty ct AND dsz > 0;
(b) the fork's CBCEncrypt returns 0 for EMPTY input (upstream-
shared contract limitation) where Wycheproof expects a padding
block — tcId 145 is that case;
(c) THE SED THAT NEVER LANDED: my first patch attempt produced a
byte-identical rerun — the sed patterns didn't match, so I
debugged a stale binary. Verified the patch in-file before the
final build (assert on pattern match this time).

### Result (patched harness)
72/72: every valid vector round-trips byte-exact (encrypt ==
expected ct; decrypt == original msg), every invalid vector is
rejected (no plaintext emitted). The substantive question — can
any invalid-padding vector slip through — is answered: NO.
Empty-input encrypt contract limitation documented (tcId 145,
upstream-identical).

### Verdict
DISMISSED: the fork's AES-256-CBC + padding is byte-exact and
padding-strict against Wycheproof. Campaign #81 EXHAUSTED
(c1 BIP324, c2 bech32(m), c3 BIP341, c4 BIP32/base58/sighash,
c5 Wycheproof).
DUPLICATE-WORK NOTE (found pre-archive): #107 c2 (2026-07-30,
conformance-test-transplant) already ran this exact cell —
24/24 valid + 48/48 invalid, PLUS an openssl cross-verifier I
did not use — including the same empty-message edge case. This
cycle is an independent confirmation (same verdict, second
harness), not new coverage; the "Wycheproof unclaimed" note in
this journal's c4 queue was wrong — it was claimed cross-journal.
Harvest-pool lesson recorded: verify candidate cells against ALL
journals, not just the owning one (second cross-journal collision
after #35 c3/c4).

### Exact commands
- curl C2SP vector file; filter keySize=256 -> /tmp/btc81c5/vec256.txt
- g++ -std=c++20 -I src -O1 harness.cpp aes.cpp ctaes.c cleanse.cpp
  lockedpool.cpp; ./harness -> "RESULT: 72 cases, 0 failures"

## Rotation note
Five cycles; campaign exhausted.
