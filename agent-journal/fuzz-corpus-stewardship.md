# Campaign #79 — fuzz-corpus-stewardship

Base: 10d1e80030 (journal commit for #65 cycle-2 on
audit/contributor-radar-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/fuzz-corpus. Start state: clean (untracked
scratch only).

## Cycle 1 (2026-07-28): cross-seed transfer audit — process_messages corpus lifts 3 sibling targets 39-55%; merge-minimize -36% size at zero loss

### Draw
Random draw over the 51-goal pool (32 pending + 19 CYCLE-1; #65
excluded as just-cycled): raw=4007249998758158245, index 28 -> #79.
NOTE: this cycle straddles the objective/catalog update (2026-07-28):
campaign-goals-99.md was replaced by campaign-goals.md (110 goals,
numbering stable); the ledger migrated to uber-goal-state.md; URGENT.md
and agent/all-findings adopted. Draw and pool predate the swap but the
goal number/slug is unchanged.

### Setup
Seed corpus: /tmp/btc9_corpus — built in #9 c2 on HEAD cd20010714:
442 files, 1.8 MB, 5382 edges on process_messages (empty start +
20k runs + 30k runs with a 28-token padded NetMsgType dictionary +
30k focus-function runs). Fuzz binary: build_fuzz clean rebuild at
10d1e80030-era source (post-probe-revert).

### Cell 1: cross-seed transfer (fixed 3000-run budget, empty vs seeded)
| target | empty cov | seeded cov | delta |
|---|---|---|---|
| process_message (singular) | 3625 | 5047 | +39% |
| p2p_handshake | 2339 | 3488 | +49% |
| cmpctblock | 3891 | 6017 | +55% |
The process_messages corpus transfers strongly to all three
structurally related P2P targets (same message-type tokens and
payload shapes drive their parsers deeper than unguided mutation
finds in 3000 runs). Verdict: CONFIRMED positive transfer; corpus
sharing across the P2P message family is the correct stewardship
policy for these targets. (First run of this experiment was killed
by a session restart after 3/6 cells; the partial results matched
the rerun.)

### Cell 2: merge-minimize (exact same binary)
`FUZZ=process_messages build_fuzz/bin/fuzz -merge=1 /tmp/btc9_min
/tmp/btc9_corpus`: 442 -> 281 files (1.8 -> 1.2 MB, -36%) retaining
ALL 5382 coverage edges and 12687 features. No feature loss; the
removed 161 files were true duplicates.

### Verdict
- CONFIRMED: the #9-c2 corpus is a transferable asset (3/3 siblings
  +39-55% at fixed budget) and minimizes losslessly to 64% of size.
- Stewardship actions recorded (not committed — corpora are scratch
  artifacts, not tree content): keep /tmp/btc9_min as the canonical
  process_messages-family seed; the 28-token NetMsgType dictionary
  (/tmp/btc9.dict) is the reusable companion.

### Exact commands
- FUZZ=<t> build_fuzz/bin/fuzz -runs=3000 -print_coverage=1 <dir>
  for t in process_message/p2p_handshake/cmpctblock x {empty,seeded}
- FUZZ=process_messages build_fuzz/bin/fuzz -merge=1 /tmp/btc9_min
  /tmp/btc9_corpus

### Limitations / queue
- qa-assets public corpus import not done (GB-scale clone; the local
  corpus is HEAD-specific and fresher for this tree) — queued with a
  per-target selective fetch method.
- Old crashers/regression inputs: none stored in-tree or in journals
  to re-run (the rotation's findings were not crash-preserved) —
  policy note: future crashes get minimized + stored with provenance.
- Flakiness/runtime-per-seed profiling (oversized seeds dominating
  exec time) not measured — queued.
- /tmp/btc9_corpus + /tmp/btc9_min + /tmp/btc9.dict kept as scratch
  seeds; sibling-target dirs /tmp/btc79_* removed after measurement.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
