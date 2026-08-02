# Campaign #61 — stateful-contract-fuzzing

Base: audit/resurrection @ f2bc9dc886 (rotation ledger commit for #16).
Branch: audit/stateful-contract-fuzz. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-27): script_sigcache verdict-transparency oracle — UPGRADED + mutation-verified

### Target selection
139 fuzz targets triaged by assert count
(`for f in src/test/fuzz/*.cpp; do grep -c "assert(" ...`): 14 targets with
zero asserts. `script_sigcache` chosen: consensus-adjacent cache whose single
fundamental contract — memoizing signature verdicts must never change them —
was unverified: both `VerifyECDSASignature`/`VerifySchnorrSignature` calls
ended in `(void)` (return ignored = campaign weak-shape #1). golomb_rice and
rolling_bloom_filter already have round-trip/state oracles (dismissed).

### Contract defined (from sigcache.cpp + interpreter.h)
- `CachingTransactionSignatureChecker::Verify*` = Get(entry) hit -> memoized
  true; miss -> base crypto verify; if valid && store -> Set(entry).
- Cache stores only valid verdicts. store=false never Sets (Get with erase).
- Therefore: store=true/false checkers, compute path, and cache-hit path
  must all agree on every (sig, pubkey, sighash).
- Verify* with a raw sighash is pure crypto — txTo/n_in/amount/txdata are
  unused, so the differential needs no script/txdata plumbing (this is also
  why the pre-existing nullptr-tx construction is safe).

### Upgrade (commit a4ff67417e)
1. Both branches (Schnorr/ECDSA): 4-way differential — store_checker
   (fresh cache), nostore_checker (fresh cache), store_checker again
   (cache-hit path when first verdict true), and the original fuzzed-store
   checker on the shared cache. Original byte-consumption order unchanged.
2. Guaranteed-valid-signature block (appended consumption, so old corpus
   seeds keep their meaning): ConsumePrivateKey + CKey::Sign +
   CKey::SignSchnorr; assert all six store/no-store/compute/cache-hit
   verifies return true. Without this, the cache-hit path essentially never
   sees a memoized entry (random sigs are never valid).

### Hard proof
- Clean run (ASan+UBSan `build_fuzz`): `FUZZ=script_sigcache
  ./build_fuzz/bin/fuzz -runs=200000 /tmp/sigcache_corpus` -> no crash,
  ~35-43 exec/s, corpus 71 seeds, cov 1115 -> 1119 (new block covered).
- Kill-proof (scratch, reverted after): deterministic key forced in the
  target + mutant in sigcache.cpp (`return true` -> `return false` on the
  Get-hit path). Empty seed aborts:
  `script_sigcache.cpp:117 ... Assertion
  'store_checker.VerifyECDSASignature(...)' failed` — exactly the cache-hit
  Assert (second store_checker.Verify). Revert -> green.
- Two independent verifiers for this consensus-adjacent oracle: (1) the
  4-way fuzzed differential, (2) the deterministic valid-signature block.

### Verdict
- Oracle gap CONFIRMED (return-ignored no-crash target). UPGRADED in
  a4ff67417e; mutation-verified. No production bug found (transparency
  holds on clean HEAD).

### Limitations / leads
- Throughput cost: ~2 signs + 7 verifies per run (~1ms). Consider gating
  the valid-sig block on a consumed bool if corpus growth stalls; left
  unconditional for deterministic cache-hit coverage.
- The 4-way differential cannot catch mutants in the SHARED base verify
  (both sides flip together) — by design; base crypto is covered by
  secp256k1/crypto targets.
- qa-assets corpus not present locally (not cloned; disk 99% full) —
  corpus transfer deferred; /tmp/sigcache_corpus is scratch.
- Other 0-assert targets triaged for future cycles: minisketch (round-trip
  likely covered elsewhere — check), blockfilter (build/match contract),
  p2p_handshake (state machine), txdownloadman (tracking invariants),
  validation_load_mempool (load/dump round-trip).

### Next queue for this campaign
- validation_load_mempool: dump->load round-trip / entry-count accounting
  oracle (persistence-adjacent).
- txdownloadman: add/GetTx announcer/tracking symmetry checks.
- blockfilter: GCSFilter MatchAny vs constructed element set (false-positive
  rate bounded by design; zero false negatives on members).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-28): all cycle-1 queue items already covered — DISMISSED + triage correction

### Triage correction (methodology)
The cycle-1 "0 asserts" table used `grep -c "assert("`, which misses
`Assert(` (the dominant spelling in newer targets). The truthful no-crash
set (`grep -cE "\bAssert\(|\bassert\(" <= 1` over src/test/fuzz/*.cpp) is
just TWO targets: process_messages.cpp and strprintf.cpp — both no-crash
BY DESIGN (P2P driver; formatting covered by unit tests). The fuzz suite
is much better fortified than the cycle-1 triage implied.

### Queue item verdicts (read the actual targets)
- validation_load_mempool: ALREADY a full state-machine checker —
  AssertMempoolPersistContracts (pool.check + unbroadcast consistency),
  SnapshotEntries/SnapshotPrioritisations equality across v2/v1/reload
  round-trips, import-options matrix (use_current_time/fee_delta/
  unbroadcast), atomic-failure file preservation
  (AssertFailedDumpPreservesFile). Lines 39-209, 253-365. DISMISSED.
- blockfilter: ALREADY a contract checker — AssertGCSFilterMatchesElements
  (GetN, MatchAny == !empty, per-element Match, zero false negatives on
  members), constructed/checked/unchecked encode equality, ExpectedBasic
  FilterElements cross-check vs block+undo, MatchAny==any_match,
  serialize round-trip byte identity. Lines 26-186. DISMISSED.
- txdownloadman (+ _impl): ALREADY exhaustive — per-operation
  before/after snapshots (txrequest counts, orphan usage/announcements,
  peer_info fields), CheckOrphanagePublicView recomputes all orphanage
  accounting from scratch, CheckInvariants after EVERY op, disconnect ->
  CheckIsEmpty for all peers and globals. 703 lines. DISMISSED.

### Verdict
- No oracle gaps in the cycle-1 queue; journal-only cycle.
- The one real upgrade delivered by this campaign remains a4ff67417e
  (script_sigcache), whose gap was found by reading the target, not by
  the flawed count.

### Next queue for this campaign
- Only honest no-crash targets left: process_messages (per-message
  invariants are largely asserted inside net_processing itself + its
  own tracking asserts; upgrading requires deep P2P state modeling —
  high effort, queue behind other campaigns) and strprintf (unit-test
  covered; skip).
- New direction next cycle: pick a MEDIUM-strength target and deepen it
  (e.g. coins_view/coinscache_sim: check whether exact serialized bytes
  or just simulated cache equality is asserted; tx_pool entry-accounting
  recomputation).

## Rotation note
Two bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-28): medium-band targets all already strong — DISMISSED x3 + triage guidance updated

### Assessments (read the targets, counted both Assert( and assert()
- coinscache_sim (1026 lines, 97 asserts): full CCoinsViewCache-stack
  simulation with model comparison, SanityCheck, dirty-count/memory
  recomputation, overlay semantics. Deep. DISMISSED.
- coins_view (707 lines, 47 asserts): strong. DISMISSED.
- script_flags (94 lines, 5 asserts): already a semantic property
  checker, not no-crash: result/error contract (35), error vs
  no-error-call agreement (37-38), and the flag-monotonicity property in
  both invariant directions (80-89: removing flags from a passing
  verdict keeps it passing; adding flags to a failing verdict keeps it
  failing) — THE correct monotonicity oracle for script flags.
  IsValidFlagCombination gates invalid combos on both passes.
  Complete for its scope. DISMISSED.

### Methodology note (updates cycle-2 triage guidance)
Assert-count triage is exhausted: every target in the 0-10 band that is
not no-crash-by-design (process_messages, strprintf) is already a
property/state-machine checker. Future cycles of this campaign should
select targets by SUBSYSTEM + history (known-weak areas, recent bug
fixes, alternative-node divergences) instead of by oracle-strength
counting; remaining genuinely-weak spots are more likely in NEW targets
(review incoming PRs) than in the existing suite. script_sigcache
(a4ff67417e) was found exactly that way (reading, not counting).

### Next queue for this campaign
- Watch-list incoming fuzz-target PRs (pr-watch cron) for new targets
  shipping without contract oracles — review at arrival.
- coins composition: CTxOutUndo/undo-data round-trip shapes (also #18's
  queue) — one target per future cycle, only after reading confirms a gap.

## Rotation note
Three bounded cycles complete; rotating per uber-goal policy. Not exhausted.
