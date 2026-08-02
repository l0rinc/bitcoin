# Campaign #48 — property-oracle-expansion

Base: f17086ff0e (journal commit for URGENT.md #10-c2 item on
audit/fuzz-gaps-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/property-oracle. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): CompactSize exhaustive boundary + non-canonical battery — mutation-verified oracle delivered

### Draw
Random draw over the 60-goal pool (41 pending + 19 CYCLE-1; #10
excluded as just-cycled): raw=11003589857354665880, seed masked to 63
bits (1780217820499890072), index 12 -> #48.

### Prior-art check (avoid re-running sibling campaigns)
#18 exhaustive-algebraic (QUEUE-COMPLETE): CompressScript +
Coin-composition round-trips with mutation-verified tests. #28 c2:
merkleblock mutation battery. Neither covers CompactSize canonicality.
Existing coverage: serialize_tests.cpp compactsize (powers-of-2
round-trips) + noncanonical (8 sampled strings). Gap: non-exhaustive
non-canonical coverage and no boundary/length-class battery.

### Cell: CompactSize (serialize.h:303-366) — consensus canonicality oracle
Hypothesis: sampled non-canonical tests can miss boundary encodings;
an exhaustive 253-form battery plus boundary length classes would
expose any canonicality divergence. Trust boundary: CompactSize is
THE vector-length primitive of the consensus wire format; accepting
non-canonical encodings is a known consensus-divergence class
(differential fuzzing vs other implementations targets it).

### Test delivered (src/test/serialize_tests.cpp,
compactsize_exhaustive_boundaries)
1. Boundary round-trips with encoded-length class: n in 0..260 (1B/3B
   split), 0xFFFE..0x10001 (3B/5B), 0xFFFFFFFE..0x100000001 (5B/9B),
   2^63-1, 2^64-1 (9B); encode->decode == n, exact byte count.
2. EXHAUSTIVE non-canonical 253-form: ALL 253 encodings of values
   < 253 in 3-byte form must throw "non-canonical ReadCompactSize()".
3. Sampled non-canonical 254/255-forms (values below the wider form's
   floor must throw).
4. MAX_SIZE (0x02000000) range_check both sides: parses at MAX_SIZE,
   throws "size too large" at MAX_SIZE+1.

### Staged controls (clean / mutation / repaired, per protocol)
- clean: serialize_tests suite GREEN.
- mutation: serialize.h:345 `if (nSizeRet < 253)` -> `< 252` (weakens
  the 253-arm canonicality check; backup/restore via cp):
  battery FAILS as required (the exhaustive 252-in-253-form case kills
  the mutant). Full suite failure observed.
- repaired: serialize.h restored; full suite GREEN again.
- Bring-up note: the battery's first run failed on my own test design
  (default range_check rejects reads above MAX_SIZE — intended
  behavior, not a production defect); fixed by reading with
  range_check=false and adding the dedicated MAX_SIZE arm.

### Verdict
- DISMISSED (production): CompactSize canonicality is exactly enforced
  on clean HEAD across the exhaustive boundary set; no divergence.
- CONFIRMED (oracle): the mutation-verified battery is delivered and
  has teeth (1/1 injected boundary mutants killed).

### Exact commands
- ninja -C build-before bin/test_bitcoin &&
  build-before/bin/test_bitcoin --run_test=serialize_tests[...]
- mutation: cp src/serialize.h /tmp/serialize_h.bak; sed -i
  's/if (nSizeRet < 253)/if (nSizeRet < 252)/' src/serialize.h;
  rebuild + run; cp back; rebuild + run

### Limitations / queue
- 254/255-form non-canonical sets are sampled (2^16 and 2^32 cases
  are too large for exhaustive); the 253-form is exhaustive (253/253).
- Wider differential vs an independent implementation (functional
  framework's deserializer) queued for cycle 2.
- Other serialization primitives (VarInt, limited-string, big-
  message lengths) unclaimed.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-29): CompactSize 254-class exhaustive battery — 130k+ cases, guard-weakening mutant killed 61440x

### Draw
Re-rank draw over the rebuilt 6-cell queue:
raw=4934760897020020062, index 0 -> #48 (second cycle; c1 queue
"254/255-form sampling widened; independent-deserializer
differential"). Branch: audit/property-oracle-c2 from 80cad73b35
(#35 c2 bookkeeping).

### Change (test-only)
serialize_tests.cpp: compactsize_254form_exhaustive —
- EXHAUSTIVE canonical 0xFD-form: all 65283 values [253, 0xFFFF]
  encode at 3 bytes and round-trip exactly.
- EXHAUSTIVE non-canonical 0xFE-form: all 65536 values [0, 0xFFFF]
  presented in 0xFE form must throw (upgrades c1's 8-sample arm to
  the full domain).
- 0xFE-form boundary acceptance: [0x10000, 0x100FF] at 5 bytes
  (256 cases; first draft wrongly used the 0xFF-class boundary —
  caught by the 9!=5 failure, fixed and recorded).

### Verification
- serialize_tests green (all cases incl. c1's battery).
- Mutation: weaken the 254-guard (0x10000 -> 0x1000) ->
  61440 failures ("exception expected but not raised" for every
  wrongly-accepted non-canonical in [0x1000, 0xFFFF]); restored,
  re-ran green. Note: BOOST_CHECK_EXCEPTION failures print
  "exception ... expected but not raised", not "has failed" — a
  grep-pattern trap recorded for future greps.
- Independent-deserializer differential cell: already delivered by
  #99 c1 (804-case clean-room differential, 0 mismatches) —
  cross-referenced, not repeated.

### Verdict
CONFIRMED oracle extension: the 254 class now has exhaustive
accept-and-reject coverage with a guard-weakening mutant killed
61440-fold. No production defect.

### Exact commands
- cmake --build build-before -j4 --target test_bitcoin
- test_bitcoin --run_test=serialize_tests[/
  compactsize_254form_exhaustive]
- mutation: serialize.h:351 guard weaken -> 61440 failures ->
  revert (backup /tmp/serialize_h.bak)

### Limitations / queue
- 0xFF-class (9-byte) acceptance is boundary-sampled (c1) not
  exhaustive (2^32+ values — infeasible).
- GetSizeOfCompactSize exhaustiveness across classes: implicitly
  covered by the round-trips (WriteCompactSize length asserts).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-29): WriteVarInt per-line sweep — coverage already complete; M1 mutation killed by the existing suite

### Draw
Re-rank draw over the 3-cell queue after a degenerate radar cell
(draw 72 raw=16888814550837975994 masked idx 2 -> #65 c8
DISCARDED: re-scan minutes after c6/c7's quiet sweeps has zero
information content; the draw is recorded and the cell deferred
to the next radar interval):
draw 73, raw=14363064615506178869, masked 5139692578651403061,
index 1 (of 2) -> #35 (second cycle; URGENT oracle-item queue
"WriteVarInt per-line sweep"). Branch: audit/property-oracle-c2
from f316e0e016 (#65 c7 journal tip).

### Sweep (serialize.h:429-446 WriteVarInt + :415-422 GetVarIntSize)
Branches and their killers in the EXISTING suite
(test/serialize_tests.cpp):
1. custom decrement `n = (n >> 7) - 1` (the non-LEB128 quirk):
   exact-encoding checks :141-156 (0x80->"8000", 0xffffffff,
   i64max, u64max all decrement-sensitive) + the 0..99999
   round-trip loop.
2. continuation bit `len ? 0x80 : 0x00`: every multi-byte exact
   check.
3. boundary `n <= 0x7F`: the "7f"/"8000" pair.
4. buffer bound `tmp[CeilDiv(bits,7)]`: u64max exact check at
   :156 (10-byte "80fefefefefefefefe7f" — the 10-byte arm is
   ALREADY covered; my initial gap hypothesis was wrong, line
   :156 exists).
5. GetVarIntSize consistency: size == ss.size() asserts in the
   loop (:115-118).

### Mutation proof (failing-before / passing-after)
M1: drop the decrement from BOTH write-side sites (serialize.h
:420 GetVarIntSize and :438 WriteVarInt — a self-consistent
mutant that only the canonical encodings + read-side catch).
Result: serialize_tests fails comprehensively — exact checks at
:154-156 (8fffffff7f != 8efefefe7f etc.) AND 99,985 round-trip
failures. Restored (backup /tmp/serialize_h.bak): suite green,
git diff empty.

### Verdict
DISMISSED (coverage complete): every WriteVarInt line is pinned
by exact encodings or round-trips; the write side needs no new
oracle. The c1 read-side battery plus this write-side sweep
closes the VarInt family.

### Exact commands
- sed mutation (backup first) + cmake --build build-before
  --target test_bitcoin + --run_test=serialize_tests (mutant
  output above; restored output "No errors detected")
- python3 reference encode/decode of the quirk (verified
  u64max -> 80fefefefefefefefe7f etc. before checking coverage)

### Limitations / queue
- M4 (buffer shrink) needs ASan to be meaningful; the integer
  fuzz target covers VarInt continuously under ASan.
- CTxUndo consumer-side fuzzed VARINT fields (the other c1 next)
  remains queued.

## Rotation note
Two cycles; VarInt write side closed. Not exhausted (CTxUndo
consumer side).

## Cycle 3 (2026-07-29): CTxUndo consumer-side fuzzed VARINT fields — all paths safe; fork amount-hardening EXCEEDS upstream

### Draw
Re-rank draw over the remaining 4-cell queue:
raw=4741416227469711203, index 3 (of 4) -> #35 (third cycle; c1
queue cell "CTxUndo consumer-side fuzzed VARINT fields"). Branch:
audit/property-oracle-c3 from bfae1cf0c9 (#9 c6 journal tip).

### Surface (undo.h TxInUndoFormatter + compressor.h)
The undo deserialization consumes fuzzed VARINT at: nCode,
dummy-version, amount (AmountCompression::Unser), script size
(ScriptCompression::Unser). Existing fuzz coverage:
txundo_deserialize, blockundo_deserialize,
txoutcompressor_deserialize (the latter with a fork-added
MoneyRange assert).

### Paths (all safe; arms run no-crash on build_fuzz)
- oversized script (nSize > MAX_SCRIPT_SIZE+6): short stream ->
  SpanReader::ignore throws "end of data" -> expected-reject;
  full stream -> script replaced with OP_RETURN (upstream-tolerated
  degrade, identical code in origin/master). Arms:
  /tmp/undo_armA (10 B, reject) and /tmp/undo_armB (10007 B,
  accept) both exit 0 on txundo_deserialize.
- special scripts (nSize 0-5): fixed-size reads only.
- VARINT overflow itself: c1's battery (both guards).
- amount: FORK ADDED a check upstream lacks — upstream master
  (7dea464d6b, fetched today) has AmountCompression::Unser =
  `val = DecompressAmount(v)` with NO bound; the fork throws on
  amount > MAX_MONEY and Assumes it on Ser. Self-consistent with
  the fork's MoneyRange assert in the fuzz target (throw makes the
  assert unreachable on bad input).

### Upstream note (seed, not a local defect)
Upstream silently accepts invalid amounts from a CORRUPTED LOCAL
DB (DecompressAmount can overflow and exceed MAX_MONEY; no check,
no fuzz assert at origin/master). Trust-boundary choice (local
disk is trusted); the fork's throw+assert exceeds it and could be
offered upstream. DecompressAmount's own overflow-wrap means the
fork's post-computation check could in principle pass a wrapped
small value — a local-corruption corner, noted for completeness;
no reachable-by-design path (only hostile/corrupt datadir).

### Verdict
DISMISSED (consumer side safe; fork strictly better than
upstream). The c1/c2/c3 VarInt + undo family is closed: read-side
guards (c1), write-side encodings (c2), undo consumer (c3).

### Exact commands
- layout from undo.h TxInUndoFormatter; arm blobs per
  [CompactSize(1) varint(4) varint(0) varint(1) varint(10007)]
- FUZZ=txundo_deserialize build_fuzz/bin/fuzz -runs=1
  /tmp/undo_armA /tmp/undo_armB (both exit 0)
- git show origin/master:src/compressor.h (AmountCompression,
  no check); git show origin/master:src/test/fuzz/deserialize.cpp
  (no MoneyRange assert); git diff origin/master..HEAD --
  src/compressor.h (fork hardening; constexpr diff is
  upstream-ahead cosmetic)

### Limitations / queue
- The overflow-wrap corner (DecompressAmount internal overflow
  wrapping below MAX_MONEY) is unpatched in both trees; a
  DecompressAmount-range-INSIDE check would close it — candidate
  micro-fix if a cycle lands here.
- PSBT_OUT_TAP_* family belongs to #80 c10.

## Rotation note
Three cycles; the VarInt/undo family is closed. Not exhausted
(the overflow-wrap corner).

## Cycle 4 (2026-07-29): DecompressAmount overflow corner — VACUOUS (bijection + range check is complete)

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=18024658476397289347, masked 8801286439542513539, index 1
(of 2) -> #35 (fourth cycle; c3 limitation "overflow-wrap
corner"). Branch: audit/property-oracle-c4 from 805bd1c3a0
(#60 c8 journal tip).

### Hypothesis (from c3)
DecompressAmount's internal uint64 overflow (n *= 10^e wrapping
at ~1.8e19) might produce a SMALL result that slips under the
fork's amount > MAX_MONEY check while being mathematically
invalid — an accepted-corrupt amount.

### Analysis + constructive search (bounded proof)
- compress(MAX_MONEY) = 21,000,000 exactly; decompress is a
  bijection by decomposition uniqueness ((n,d,e) forced per x:
  e = (x-1)%10, d = ((x-1)/10)%9 + 1 in [1,9]). Every x reads to
  a UNIQUE deterministic amount; every amount has exactly its
  canonical encoding.
- compress is NOT monotonic (compress(21e14-1) ~ 1.9e13 >>
  compress(21e14) = 2.1e7), so x > 21M is NOT invalid — the
  candidate search confirmed it: x=21000001 -> decompress
  2,333,334 sats, which IS the canonical valid encoding of that
  amount (verified by recompression).
- The wrap loop (n *= 10^e mod 2^64) can alias distinct huge
  encodings to the same amount, but (a) writes only ever encode
  valid amounts, and (b) the only meaningful validity question
  for a read is RANGE, which the fork's post-computation check
  answers exactly. There is no "intended" out-of-range value for
  a read to be mistaken about.

### Verdict
DISMISSED (corner vacuous): the fork's range check is complete
for amount validity; the c3 limitation note is CORRECTED (the
mechanism was real but the exploit class doesn't exist — wrap
only affects huge encodings, and range-checked output is always a
well-defined in-range amount). No fix needed; the amount
compression family is fully analyzed.

### Exact commands
- python3 reference compress/decompress (bijection + range
  search, output above); src/compressor.cpp:176-198 read

### Limitations / queue
- None open for the VarInt/amount/undo family (c1-c4 closed).

## Rotation note
Four cycles; the family is closed with the corner proven vacuous.

## Cycle 5 (2026-08-02, draw 211, raw=15898866038248710399, masked 6675494001393934591, idx 11/20): FeeFrac ByRatio ordering-laws oracle — 55,201 checks (trichotomy, antisymmetry, operator-agreement, reference match, transitivity), ZERO mismatches; DISMISSED

### Cell
The amount/VarInt family is closed (c1-c4); fresh algebraic
surface: the ByRatio<FeeFrac> ordering (feefrac.h:242-289) —
cross-product comparison via T::Mul (__int128/fallback), never
law-verified.

### Oracle (/tmp/btc48c5/order_oracle.cpp, preserved)
- Corpus: 149 fractions (18 fee edges x 8 sizes incl.
  INT32_MIN/MAX, INT64_MAX/4, 21e14, 1<<40, sign edges) +
  equal-ratio pairs (1/2,2/4,3/6,-1/2,-2/4).
- Laws checked:
  1. trichotomy + antisymmetry on all 149² pairs (a<=>b ==
     -(b<=>a));
  2. agreement with an INDEPENDENT exact reference
     (boost::multiprecision::cpp_int cross products) on all pairs;
  3. specialized operators <,>,<=,>= all agree with <=> on all
     pairs (the documented-efficiency set at :269-289);
  4. transitivity on sampled triples (stride 7/5/3).
- TALLY pairs+triples=55,201 mismatches=0.

### Verdict
DISMISSED (laws hold): the ordering is a proper total preorder
with exact cross-product semantics at every adversarial edge,
on both the int128 path and (by the #100-c4 backend-equality
result) the fallback. No defect.

### Exact commands
- g++ -O2 line above; ./order_oracle (TALLY above).

### Limitations / queue
- ByRatioNegSize (:314-326) same shape, not separately swept
  (same Mul core; noted).
- Corpus is edge-dense, not exhaustive over the 64-bit fee
  domain (infeasible; the reference agreement at edges is the
  evidence form).
