# Campaign #35 — mutation-testing

Base: 6ba8b6eea8 (journal commit for #63 cycle-1 on
audit/loupe-pipeline; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/mutation-testing. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): ReadVarInt overflow-rejection — 2 survivors found by sweep, oracle delivered and mutation-verified

### Draw
Random draw over the 56-goal pool (37 pending + 19 CYCLE-1; #63
excluded as just-cycled): raw=10700746438949304036, seed masked to 63
bits (1477374402094528228), index 4 -> #35.

### Cell selection (prior-art check)
#28 c2 amount battery (5/5 killed), merkleblock battery (2 survivors
fixed); #19 benchmark mutation sweep (honest); #48 c1 CompactSize
battery (1/1 killed). Distinct surface: VARINT (serialize.h:428-466,
the undo-data encoding family, consensus-relevant in the CTxUndo
format) — untouched by those batteries.

### Mutation sweep (mutants first, per campaign doctrine)
serialize.h is header-only — mutations rebuild the test TU + relink
(documented shortcut: the suite exercises the test TU's own template
instantiations; ccache makes repeat-content builds ~12s).
- M1 (write loop `n = (n >> 7) - 1` -> `n = (n >> 7)`): KILLED by the
  existing varints round-trip (decoded 256 vs expected 128). Write
  path strong.
- M2 (delete pre-shift guard `if (n > max>>7) throw`): SURVIVED —
  full serialize suite green with the guard gone.
- M3 (delete post-shift guard `if (n == max && continuation) throw`):
  SURVIVED — same.
Score: 1 killed / 2 survived — both overflow-rejection paths were
test-blind.

### Oracle delivered (serialize_tests.cpp, varints_overflow_rejection)
Rejection cases at BOTH guards for uint8/16/32/64, each paired with a
positive control at the type's legal maximum encoding (exact value +
full stream consumption), plus one NONNEGATIVE_SIGNED case. Every
case asserts the exact "size too large" message class.

### Mutation-verification of the oracle (staged clean/mutation/repaired)
- clean: suite green.
- M2 reapplied: suite RED — pre-shift cases report "exception
  expected but not raised" (2 cases).
- M3 reapplied: suite RED — post-shift cases now mis-throw
  ("end of data" where "size too large" was deleted), caught by the
  exact-message predicates.
- repaired: suite green. 2/2 surviving mutants now die.
- Bring-up note: my first uint16 legal-max control was mis-derived
  (0xFF vs 0xFE); caught by the control itself failing on clean code
  (oracle self-checking).

### Verdict
- CONFIRMED (test-oracle gap, class of #28 c2's survivors): both
  ReadVarInt overflow guards were untested; an accidental deletion or
  weakening would have shipped silently (undo-data parsing is
  consensus-adjacent: CTxUndo deserialization uses VARINT).
- FIXED (oracle): mutation-verified battery committed; production
  code untouched (both guards verified correct on clean HEAD).

### Exact commands
- python3 replace-based mutant application + cp backup/restore of
  src/serialize.h; ninja -C build-before bin/test_bitcoin;
  test_bitcoin --run_test=serialize_tests[/varints_overflow_rejection]

### Limitations / queue
- Mutation coverage of WriteVarInt's other boundary (SizeComputer
  overload at serialize.h:426 — M1 accidentally hit both overloads,
  both killed by the same test) is single-mutant deep; a systematic
  per-line sweep queued.
- NONNEGATIVE_SIGNED negative-value rejection not in the battery
  (formatter-level, separate contract) — queued.
- Undo-format end-to-end (CTxUndo with fuzzed VARINT fields) is the
  natural consumer-side extension — queued.

## Rotation note
Cycle 1 complete with a delivered oracle; rotating per uber-goal
policy. Not exhausted.

## Cycle 2 (2026-07-29): WriteVarInt per-line sweep — 3 more mutants, all killed; CTxUndo fuzz coverage confirmed

### Draw
Re-rank draw over the rebuilt 7-cell queue:
raw=4506871108920428011, index 1 -> #35 (second cycle; c1 queue
cells "WriteVarInt per-line sweep" + "CTxUndo consumer-side fuzzed
VARINT fields"). Branch: audit/mutation-testing-c2 from 2689e08db3
(#34 c3 bookkeeping).

### WriteVarInt per-line sweep (serialize.h:429-444)
c1 killed the loop-step mutant (n = (n>>7)-1). This cycle, three
more line-level mutants, each built and run against
serialize_tests + coins_tests + validation_chainstatemanager_tests:
- M2: continuation bit always set (tmp[len] = (n&0x7F)|0x80) ->
  34 failures. KILLED (every multi-byte read continues past the
  payload).
- M3: GetSizeOfVarInt break shift (n<=0x7F -> n<0x7F, :415) ->
  137 failures. KILLED (size/content mismatch across all
  multi-byte encodings). (sed's first-match hit the size computer
  rather than the writer; recorded as M3 and covered separately.)
- M4: WriteVarInt break shift (:436) -> 137 failures. KILLED
  (same class; identical failure count to M3, consistent).
All mutants reverted after measurement; restore verified green.

### CTxUndo consumer-side VARINT fields — already covered
Dedicated fuzz targets exist: txundo_deserialize and
blockundo_deserialize (src/test/fuzz/deserialize.cpp:256-262),
plus blockfilter.cpp:51 consuming CBlockUndo::vtxundo. No gap;
O2's ReadVarInt battery covers the reader guards themselves.

### Verdict
DISMISSED: the WriteVarInt family is fully mutation-covered
(per-line: loop-step c1, continuation bit M2, both break
conditions M3/M4 — 0 survivors). The CTxUndo VARINT consumer is
fuzz-covered by existing targets. No oracle gap; no code change.

### Exact commands
- per-mutant: edit serialize.h, cmake --build build-before -j4
  --target test_bitcoin, test_bitcoin --run_test='serialize_tests,
  coins_tests,validation_chainstatemanager_tests'
- backup/restore: cp /tmp/serialize_h.bak (diff-verified clean)

### Limitations / queue
- The do/while write-loop and len++ lines are mutation-equivalent
  to the killed break/step lines (any change there desyncs the
  encoding identically) — noted, not separately run.
- CTxUndo SEMANTIC apply-vs-reject differential (c1 queue mention)
  remains open (the #51 oracles cover undo correctness, not
  hostile-field semantics).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
