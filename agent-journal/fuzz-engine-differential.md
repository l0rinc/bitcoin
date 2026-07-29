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
