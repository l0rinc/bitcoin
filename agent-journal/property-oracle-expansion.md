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
