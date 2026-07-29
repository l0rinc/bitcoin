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
