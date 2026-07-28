# Journal: hit-frequency and suspicious-branch coverage audit (campaign 9)

Uber-goal rotation. Branch: audit/hit-frequency-coverage from
audit/resurrection @ 50302ae602. Tool: libFuzzer sancov via
`-print_coverage=1` (the deprecated -dump_coverage produces nothing on
current libFuzzer; print_coverage gives UNCOVERED_FUNC/UNCOVERED_PC with
edge counts). Host runs ~10 exec/s for coins_view.

## Cycle 1: coins.cpp / txdb.cpp via coins_view target (2000 runs)

### Coverage result: every substantive production function COVERED

No non-destructor function in coins.cpp is entirely uncovered. Uncovered
PCs classify into 4 benign buckets:

1. ASSERT-ONLY lines: uncache base_cache-present assertions
   (coins.cpp:484-488, G_ABORT_ON_FAILED_ASSUME-gated), uncache TrySub
   bookkeeping (473).
2. TERNARY ARM: PeekCoin on a spent cache entry (coins.cpp:36 true
   branch → nullopt) — mechanically asserted at 37; harness only peeks
   unspent/missing. Coverage nicety, not a defect.
3. Reset() internals (543-544): memory-resource/map reconstruction —
   the harness doesn't drive Reset in these runs.
4. Sibling-target entries + dtors (noise floor: each target's entry is
   uncovered when running another; resize-cursor target has its own
   290-edge entry, not run here).

### Verdict: DISMISSED — no suspicious uncovered logic branch

The fuzz harness hits all substantive logic; the rare branches are the
defensive ones, exactly where they belong. No missing-scenario branch,
no dead code found in the audited subsystem.

## Next queue
(same method on net_processing — larger, ~10 exec/s here so needs
longer/parallel runs or targeted entry points; then rotate per ledger)