# Campaign #80 — fuzz-engine-differential

## Cycle 1 (2026-07-29): PSBT parser differential — C++ production vs independent Python reference; no divergence in the production direction

### Draw
Random draw over the 15-goal eligible pool (12 pending + 3 CYCLE-1,
#58 excluded as just-cycled): raw=7161189119308982694, index 9 ->
#80 (first cycle). Branch: audit/fuzz-engine-differential from
b012b6c3bd (#58 c2 bookkeeping; lineage anchor audit/resurrection @
5d0155254c). Start state: tracked-clean. Catalog note: #80's
campaign-focus block contains spec-mapping/conformance text — the
focus blocks in this region are offset from their title/slug pairs
(same artifact class as #49/#58); title+slug
fuzz-engine-differential is authoritative.

### Hypothesis
The C++ PSBT parser (decodepsbt RPC — the production parser the fuzz
engine exercises) and the test framework's independent Python PSBT
implementation (test_framework/psbt.py) can diverge on adversarial
inputs: either C++ over-accepts (production defect class) or the two
disagree on canonical form (round-trip inequality).

### Method
Independent generator (python random, fixed seed 0x80) mutating the
two RPC-verified valid seeds from #50 (/tmp/btc50_seed/{psbt_min,
psbt_1in}): 400 cases, byte mutations (flip/truncate/extend/insert/
delete) + map-level structural mutations (KV shuffle, key
duplication). Per case: C++ verdict via decodepsbt RPC, Python
verdict via PSBT.deserialize + serialize round-trip. Classes:
A = C++ accepts/Python rejects (critical direction),
B = double-accept but reserialize != input (non-canonical),
C = C++ rejects/Python accepts (expected only for Python-lax),
D = both reject, E = both accept + round-trip equal, R = Python
resource-limited (MemoryError under a 4 GiB RLIMIT_AS).

### Result (deterministic, reproducible)
TALLY: A=0 B=0 C=123 D=134 E=142 R=1.
- A=0: production never over-accepts vs the reference. Clean.
- E=142: every double-accepted document round-trips byte-identically
  (B=0 non-canonical double-accepts).
- C=123 sampled reasons — ALL Python-is-laxer, each with a named
  mechanism: "extra data after PSBT" (C++ full-consumption check;
  Python never checks EOF), "Separator is missing at the end of the
  global map" (C++ requires the 0x00 separator strictly),
  "non-canonical ReadCompactSize()" (C++ canonicality — the O1
  battery's domain; Python's deser_compact_size accepts
  non-canonical), "SpanReader end of data" (Python's deser_string
  under-reads short buffers without raising).
- R=1: one hostile-count case hit the 4 GiB per-process guard and
  was contained per-case (see incident note).

### Incident/harness notes (honest record)
1. First run OOM-killed (kernel log 2026-07-29 06:27, python3
   14.8 GB RSS): a mutated CompactSize input-count drove the
   framework's `[from_binary(PSBTMap, f) for _ in range(in_count)]`
   (psbt.py:132) to allocate unboundedly — the Python reference
   trusts untrusted counts; the C++ parser rejects the same bytes
   cheaply (fast bad_alloc). Fixed by RLIMIT_AS=4 GiB + per-case
   MemoryError classification (class R). No server restart; no other
   process harmed; /tmp/btc80 cleaned.
2. Second run was invalidated by MY harness bug: I passed hex to
   decodepsbt (expects base64) — E=0 exposed it immediately because
   ~1/7 of cases are zero-mutation and must double-accept. Added an
   explicit positive-control assertion (E>0) so the harness fails
   loud on this class. Fixed; v2 result above.

### Verdict
DISMISSED (no production divergence): 276/400 agreement (E+D), zero
critical-direction cases, all 123 C divergences are reference-lax
with named mechanisms. The Python framework's laxness (untrusted
counts, short reads, no EOF/canonicality checks) is test-helper
behavior, upstream-matching, and harmless in its role (input
construction, not oracle) — no local patch (would diverge from
upstream for zero production benefit; no security theater).

### Exact commands
- python3 /tmp/btc80_diff.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc80 (deterministic seed 0x80, 400
  cases, ~2.5 min; script preserved at /tmp/btc80_diff.py)
- dmesg -T | grep -i oom (incident evidence)

### Limitations / queue for cycle 2
- 400 cases from 2 seeds; a wider seed set (PSBTv2 documents,
  taproot fields) would stress version-2 paths the v0 seeds never
  reach.
- The differential used decodepsbt (accept/reject) as the C++
  verdict; the fuzz target's internal round-trip asserts (ser∘de=id)
  were not directly compared against Python's reserialization for
  v2 PSBTs.
- Other parser pairs with independent Python references in the
  framework: CTransaction/CTxIn (messages.py vs core), merkleblock
  (already covered #6 c2), BIP152 HeaderAndShortIDs — candidates
  for the next cell.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-29): raw-transaction parser differential — A=0/300; Python-lax divergences only

### Draw
Re-rank draw over the rebuilt 2-cell queue:
raw=4732580478433972031, index 1 -> #80 (second cycle; c1 queue
cell "CTransaction/CTxIn messages.py vs core"). Branch:
audit/fuzz-engine-differential-c2 from e91bd7bdfa (#40 c2
bookkeeping).

### Method (c1 harness shape)
Independent generator (python random, fixed seed 0x80) mutating 12
real MiniWallet transactions (6 signed, both witness and stripped
serializations): 300 cases across bit flips, truncations,
extensions, insertions, deletions. C++ verdict via
decoderawtransaction RPC (strict DecodeHexTx, full-consumption);
Python verdict via CTransaction.deserialize + reserialize round-
trip. Same classes as c1 (A/B/C/D/E/R), 4 GiB RLIMIT_AS, E>0
positive control.

### Result
TALLY: A=0 B=0 C=240 D=1 E=59 R=0.
- A=0: production never over-accepts vs the reference. Clean.
- E=59, B=0: every double-accepted transaction reserializes
  byte-identically in both implementations.
- C=240: C++ rejects/Python accepts — the Python-lax class with
  named mechanisms (no EOF/trailing check, BytesIO short reads
  without error, no witness-marker strictness, no canonical
  CompactSize enforcement). Expected and classified, as in c1.
- D=1: both reject. R=0: no hostile-count blowups (guard held).

### Harness note
MiniWallet funding: exactly-101 blocks matures ONE coinbase; the
seed loop needs 120 (the StopIteration failure shape from the first
run; fixed).

### Verdict
DISMISSED: no parser divergence in the production direction for
raw transactions either; the strictness asymmetry (C++ strict,
Python lax) is uniform and upstream-matching.

### Exact commands
- python3 /tmp/btc80_txdiff.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc80t (deterministic, ~1 min)

### Limitations / queue
- decoderawtransaction is decode-only (no CheckTransaction); the
  consensus-level acceptance differential (CheckBlock/AcceptToMemPool
  on mutated txs) is a different, heavier cell — queued.
- CTxIn/CTxOut standalone twins folded into this run (transaction
  deserialization covers them).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 3 (2026-07-29): consensus-level acceptance differential — A=0/300; no structural over-acceptance

### Draw
Re-rank draw over the rebuilt 2-cell queue:
raw=5930901419059390541, index 1 -> #80 (third cycle; c2 queue
cell "consensus-level acceptance differential (CheckBlock/mempool
on mutated txs)"). Branch: audit/fuzz-engine-c3 from 0c94597ec9
(#44 c2 bookkeeping).

### Method
Independent Python oracle re-implementing CheckTransaction's
structural rules (tx_check.cpp: vin/vout empty, weight cap, amount
range/overflow, dup inputs, null prevouts for non-coinbase), vs C++
testmempoolaccept (parse + consensus + policy), over 300 mutated
real transactions (RLIMIT guard, E>0 positive control).

### Result
TALLY: A(cpp-ok/py-reject)=0, E(both accept)=53,
C(cpp-reject/py-accept)=148, D(both reject)=99.
- A=0: no transaction C++ accepted that the independent structural
  oracle rejects — consensus structural acceptance never
  over-accepts.
- C=148: expected classes by construction: (a) C++ parse-strict
  failures the Python parser tolerates (c2's Python-lax family);
  (b) mempool POLICY rejections outside CheckTransaction's scope
  (standardness/fees/mempool state), which the structural oracle
  deliberately doesn't model.
- D=99 agreement; E=53 agreement.

### Harness lesson (recorded)
First run reported A=53 with py-reason="parse" for ALL cases and
E=0 — the try block swallowed an AttributeError from MY oracle
(COutPoint has .hash/.n in this framework, not .txid). Every
"reference rejects" was my bug. The E>0 positive control caught it
instantly (zero double-accepts is impossible with valid seeds) —
the pattern keeps proving itself.

### Verdict
DISMISSED: no consensus-level over-acceptance on the structural
rule set; C++ parse+policy is strictly more rejecting than the
structural oracle, exactly as designed.

### Exact commands
- python3 /tmp/btc80_cons.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc80c

### Limitations / queue
- Script-level acceptance (CheckTxInputs/script verification)
  beyond the structural set is out of this oracle's scope (would
  need the framework's script engine or a signed-corpus
  differential — the #55 ECDSA differential covered signature
  semantics separately).
- Block-level (CheckBlock) differential folded into the tx-level
  run by construction (same CheckTransaction core).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-07-29): PSBTv2 differential — v2 paths clean too (A=0/400 mixed corpus)

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=1166893013096824274, index 4 -> #80 (fourth cycle; c1 queue
cell "PSBTv2 seeds — version-2 paths the v0 seeds never reach").
Branch: audit/fuzz-engine-c4 from 0676a2142d (#91 c4 bookkeeping).

### Seed construction (RPC-verified)
Hand-built 87-byte PSBTv2 (BIP370 globals: TX_VERSION=2,
INPUT_COUNT=1, OUTPUT_COUNT=1, VERSION=2; 1 input with zero
prevout + 1 OP_TRUE output). decodepsbt accepts it (inputs 1,
outputs 1). Seed preserved at /tmp/psbt_v2_seed.

### Differential (c1 harness, mixed corpus: 300 v2 + 100 v0 cases)
TALLY: A=0 B=0 C=124 D=168 E=107 R=1.
- A=0: production never over-accepts, v2 paths included.
- E=107: double-accepted with round-trip equality (v2 seed among
  them — positive control).
- C=124: Python-lax classes (same as c1: no EOF check, short
  reads, canonicality). B=0: canonical agreement on double-accepts.
- R=1: one hostile-count case contained by the 4 GiB guard.

### Verdict
DISMISSED: the PSBTv2 parser surface is differential-clean like
v0; no divergence in either direction beyond the reference's known
laxness.

### Exact commands
- seed constructor (python struct + ser_compact_size; in journal
  history); python3 /tmp/btc80_diff.py (patched corpus mix)
  --configfile=build-before/test/config.ini --tmpdir=/tmp/btc80

### Limitations / queue
- v2-specific FIELDS (taproot keypaths, tx modifiable flags) not
  deeply seeded (the seed is minimal); a richer v2 corpus is the
  next depth step if a cycle lands here.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 5 (2026-07-29): rich PSBTv2 corpus — taproot keypath differential clean (A=0); the seed taught me BIP371's value layout twice

### Draw
Re-rank draw over the rebuilt 2-cell queue:
raw=6241307819045081386, index 0 -> #80 (fifth cycle; c4 queue
cell "richer v2 corpus — taproot keypaths, tx modifiable flags").
Branch: audit/fuzz-engine-c5 from 139d4571e5 (#65 c4 bookkeeping).

### Seed construction saga (recorded honestly — 6 iterations)
The 299-byte rich v2 seed (TX_VERSION=2, counts, TX_MODIFIABLE=3;
input with prevout/sequence/TAP_INTERNAL_KEY/TAP_BIP32_DERIVATION;
output with amount/P2TR-script/TAP_INTERNAL_KEY/TAP_BIP32) failed
decodepsbt 5 times before acceptance:
1. TAP_INTERNAL_KEY written under TAP_KEY_SIG's id (0x13 vs 0x17).
2. TAP_BIP32 under MUSIG2's id (0x1a vs 0x16).
3. Pubkey in the keypath VALUE (BIP371 puts the x-only pubkey in
   the KEY, 33 bytes total incl. type).
4. Compact 33-byte pubkey instead of x-only 32-byte (key must be
   33 incl. type byte).
5. Extra inner CompactSize before the leaf-hashes set: the parser
   (psbt.h:833-851) reads the outer value length, then the set
   count; my inner 0x0d was read as count=13 hashes -> 416 bytes ->
   end of data. Correct value: [count][hashes][fingerprint][path],
   no inner length. The Python framework parser accepted the
   malformed doc (its keypath handling is lax) — the C++ parser's
   exact expectation was the teacher (matches the campaign's
   Python-lax theme).

### Verification
- decodepsbt ACCEPTED with taproot_bip32_derivs +
  taproot_internal_key on both input and output.
- Differential (c1 harness, 400 mixed cases: 50% rich-v2, 25%
  minimal-v2, 25% v0): TALLY A=0 B=0 C=147 D=134 E=115 R=4.
  Production never over-accepts; the four hostile-count cases
  (keypath hash sets / path vectors — the new allocation class the
  richer fields add) contained by the 4 GiB guard.
- Seed preserved: /tmp/psbt_v2_rich_seed.

### Verdict
DISMISSED (clean): the rich v2 surface is differential-clean; the
keypath allocation classes are guard-contained.

### Exact commands
- python3 seed constructor (6 iterations recorded above)
- python3 /tmp/btc80_diff.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc80

### Limitations / queue
- MuSig2 fields (0x1a-0x1c) unseeded — next depth step if a cycle
  lands here.
- TAP_LEAF_SCRIPT/TAP_MERKLE_ROOT paths unseeded.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 6 (2026-07-29): MuSig2 PSBT field seeding — differential clean; format-from-source worked first try

### Draw
Re-rank draw over the rebuilt 4-cell queue:
raw=7792775892002329801, index 1 -> #80 (sixth cycle; c5 queue
cell "MuSig2 fields unseeded"). Branch: audit/fuzz-engine-c6 from
6a9c6b44af (#10 c3 bookkeeping).

### Seed (229 bytes, first-try correct)
v2 document with PSBT_IN_MUSIG2_PARTICIPANT_PUBKEYS (0x1a): key =
type + 33-byte compressed aggregate pubkey; value = two
concatenated 33-byte participant pubkeys. Format read from
psbt.h:219-274 first (after the c5 keypath saga): no iteration
needed. decodepsbt accepts (musig2_participant_pubkeys listed).

### Differential (corpus: 25% each of musig/rich-v2/minimal-v2/v0)
TALLY: A=0 B=0 C=159 D=138 E=100 R=3. Production never
over-accepts; MuSig2 paths included; three hostile-count cases
contained by the guard.

### Verdict
DISMISSED (clean): the MuSig2 participant-pubkey surface is
differential-clean. Seed preserved: /tmp/psbt_v2_musig_seed.

### Limitations / queue
- MUSIG2_PUB_NONCE (0x1b) / PARTIAL_SIG (0x1c) with the 67/99-byte
  key shape (part+agg [+leaf_hash]) unseeded — next depth step.
- TAP_LEAF_SCRIPT / TAP_MERKLE_ROOT remain unseeded.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 7 (2026-07-29): MuSig2 nonce + partial-sig PSBT fields seeded (67/99-byte keys) — differential clean

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=12027658190395197936, masked 2804286153540422128, index 0
(of 2) -> #80 (seventh cycle; c6 queue cell "MUSIG2_PUB_NONCE /
PARTIAL_SIG unseeded"). Branch: audit/fuzz-engine-c7 from
ed6ad1e971 (#50 c8 journal tip).

### Seed (632 B, format verified against psbt.h first)
/tmp/psbt_v2_musig2_seed (sha256
e8e81bce4d297130ae80d80f19bce9ea25c4892e7ef3a375c6478b6b9da6ea60):
the c6 musig doc plus, on input 0:
- 0x1b MUSIG2_PUB_NONCE, key = type+agg+part (67 B), value 66 B
  (two concatenated compressed pubkeys as the pubnonce)
- 0x1b with the +leaf_hash key variant (99 B) — covers the
  second allowed key length
- 0x1c MUSIG2_PARTIAL_SIG, key = type+agg+part (67 B), value 32 B
Key layout and the 67-or-99 length gate verified against
psbt.h:870-901 before construction (agg BEFORE part; value must be
exactly 66 / 32 bytes — no iteration needed, first-try accepted).

### Verifications
- decodepsbt: accepted; input lists musig2_pubnonces=2,
  musig2_partial_sigs=1, musig2_participant_pubkeys=1.
- Differential (400 cases; new seed 1/3 of the corpus mix with
  c6-musig/rich-v2/minimal-v2/v0): TALLY A=0 B=0 C=167 D=132
  E=101 R=0. Production never over-accepts; 101 round-trip-equal
  accepts (positive control); no resource-guard events (R=0).

### Verdict
DISMISSED (clean): the MuSig2 nonce/partial-sig parse surface is
differential-clean, including the leaf-hash key-length variant.
The full MuSig2 PSBT field family (0x1a/0x1b/0x1c) is now seeded
and clean.

### Exact commands
- python3 seed extension (framework PSBT parse + append + reser)
- python3 /tmp/btc80_dec.py --configfile=... (DECODE-OK)
- python3 /tmp/btc80_diff_c7.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc80_c7 (TALLY above)

### Limitations / queue
- TAP_LEAF_SCRIPT / TAP_MERKLE_ROOT fields remain unseeded (the
  taproot parse surface counterpart) — next cell if redrawn.
- The differential covers parse/round-trip only; nonce/sig VALUE
  validation (musig2 session logic) is the wallet's, not the
  parser's.

## Rotation note
Seven cycles; MuSig2 field family closed. Not exhausted (taproot
parse fields).
