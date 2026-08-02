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

## Cycle 2 (2026-08-01): per-seed runtime profile — FLAT (max/median 1.05x); no oversized-seed dominance; ~99.5% of per-invocation cost is process startup; hypothesis REFUTED

### Draw
RE-RANK draw 157 over the 5-cell pool: raw=6943923678126847234
(already 63-bit) -> idx 4 -> per-seed profiling (c1 queue
"oversized seeds dominating exec time"). Branch:
audit/fuzz-corpus-c2 from 2d5dace4db. (Pool shorthand said #9;
this campaign is #79 — #9 is hit-frequency-coverage.)

### Measurement (preserved corpus /tmp/btc9_corpus, 442 seeds)
Per-invocation timing (-runs=1 per seed, /tmp/btc9c2_profile.py +
.json): total 345.6s, mean 781.8ms, median 780.9ms, max 821.4ms —
max/median = 1.05x. The slowest 10 seeds are SMALL (11-138 B).

### Control (why flat): startup floor vs in-process cost
- 1-byte seed, -runs=1: 783 ms wall — the per-invocation cost is
  ~99.5% fixed startup (binary init + harness setup), not seed
  execution.
- Whole corpus in ONE process (-runs=0): 497 runs in 1 s (2.4 s
  wall) -> ~4 ms/seed marginal in-process cost.
So seed cost is flat across size; the c1 concern is refuted for
this corpus, and normal campaign mode amortizes the startup
entirely.

### Verdict
DISMISSED (hypothesis refuted): no oversized/pathological seed in
the preserved corpus; runtime-per-seed is uniform. METHOD LESSON:
per-invocation profiling measures STARTUP, not seed cost — any
future seed-cost claim must use in-process batch runs (-runs=0)
or subtract the startup floor (measured with a 1-byte seed).

### Exact commands
- /tmp/btc9c2_profile.py (per-seed loop, 30s per-seed timeout);
  FUZZ=process_messages build_fuzz/bin/fuzz -runs=1 /tmp/btc9c2_tiny;
  FUZZ=process_messages build_fuzz/bin/fuzz -runs=0 /tmp/btc9_corpus.

### Limitations / queue
- Profile is process_messages-specific; other targets may have
  heavier per-seed curves (same method applies).
- Remaining #79 cells: qa-assets selective import (GB-scale),
  crash-artifact policy (already standing).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-08-01, draw 172, raw=184429387142081413 (63-bit), idx 1/6 -> STALE (#108 complete; pool repair), redraw raw=14500292587252405485, masked 5276920550397629677, idx 2/5): qa-assets selective import — pinned-commit sparse import of 3 in-scope corpora validated clean end-to-end; DISMISSED

### Hypothesis
H: importing the upstream qa-assets corpora for in-scope targets
(process_messages, transaction, script) surfaces seeds the fork's
fuzz build rejects/crashes on (corpus-vs-build skew, e.g. from the
fork's Assume-hardened validation). Falsifiable by a full single
pass per seed under the fork's own fuzz binary.

### Import (disk-bounded, provenance exact)
- Sparse blob-filtered clone: git clone --depth 1 --filter=
  blob:none --sparse https://github.com/bitcoin-core/qa-assets
  /tmp/qa-assets -> HEAD 918cdd3 == the CI pin from #59 c3
  (918cdd36fec3...): the pin is current upstream HEAD.
- Upstream renamed the corpus dir: fuzz_seed_corpus/ ->
  fuzz_corpora/ (sparse patterns adjusted; recorded for #59 c3's
  CI script which references the path — verified ci/test/
  03_test_script.sh's clone is path-agnostic, no drift).
- Imported: process_messages 3,783 seeds (56M), transaction 1,527
  (89M), script 2,536 (24M) — 7,846 seeds, 218M total, disk 4.0G
  free kept.

### Validation (fork binary, single pass per seed)
build_fuzz/bin/fuzz -runs=0 <corpus> per target:
- transaction: 2,530 runs, cov 5136 ft 27865, 325s, DONE clean.
- script: 2,537 runs, cov 5287 ft 14350, 36s, DONE clean.
- process_messages: 4,958 runs, cov 18187 ft 58436, 60s, DONE
  clean.
Zero crashes, zero timeout/slow-unit/leak artifacts: find
-newer <import marker> over crash-*/timeout-*/slow-unit-*/
leak-*/oom-* returns 0; the two untracked crash-* files share
one identical mtime (the cycle-167 stash/pop instant) and are
unrelated pre-existing state, untouched.

### Verdict
DISMISSED: no corpus-vs-build skew; the pinned upstream corpus is
fully executable by the fork's fuzz build, and the import path
(sparse, pinned, selective) is proven cheap enough (218M vs
GB-scale full clone) to repeat per-target on demand.

### Exact commands
- clone/sparse lines above; per-target FUZZ=<t> build_fuzz/bin/
  fuzz -runs=0 lines above; du/ls counts above.

### Limitations / queue
- Single pass only (no mutation campaign this cycle); coverage
  deltas vs a local corpus not computed (no local baseline corpus
  for these targets in-tree).
- /tmp/qa-assets kept (218M); delete on disk squeeze, re-sparse
  per the lines above.
- crash-artifact policy cell remains standing (already policy).
