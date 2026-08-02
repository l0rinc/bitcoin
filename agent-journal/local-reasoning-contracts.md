# Campaign #57 — local-reasoning-contracts

Base: e264721cff (journal commit for #50 cycle-1 on
audit/introspector-blockers; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/local-contracts. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): m_all_zero flag contracts (bloom + estimator) — observable and guarded; broken-discipline mutation caught

### Draw
Random draw over the 31-goal pool (18 pending + 13 CYCLE-1; #50
excluded as just-cycled): raw=18274010591509698181, seed masked to 63
bits (9050638554654922373), index 11 -> #57.

### Subject
This session's own additions, the highest review-value contract:
CRollingBloomFilter::m_all_zero (#22 c2) and
TxConfirmStats::m_all_zero (#63 c1). Contract for each: flag is true
exactly while the underlying state is all-zero; set by
construction/reset, cleared by every state-dirtying operation
(bloom: insert; estimator: Record, removeTx-failAvg, Read).

### Local-reasoning audit
- Bloom: a reviewer can verify the contract LOCALLY from the
  observable semantics — after reset(), contains(x) is false for all
  x regardless of the fast path, because the flag only skips a fill
  that is the identity when the flag is honest. Copy/move preserve
  flag+data together (default special members).
- Estimator: the same observable discipline; dirtying points were
  enumerated exhaustively by the #63 verifier (including the
  failAvg-without-firstRecordedHeight and Read() cases the naive
  predicate would have missed).
- SanityCheck() intentionally doesn't assert the flag (it's an
  optimization, not an invariant the class depends on) — consistent
  with its documentation in the commits.

### Shadow proof (mutation control)
Broken discipline: delete `m_all_zero = false;` from
CRollingBloomFilter::insert (the flag then stays true through
inserts, so reset() skips the fill on dirty data):
- bloom_tests/rolling_bloom FAILS at :535
  (`check !rb1.contains(datum) has failed` after reset) — the
  contract break is caught by the pre-existing suite.
- Restored: suite green. Staged clean/mutation/repaired complete.

### Verdict
- CONFIRMED (contract quality): both flags' contracts are locally
  verifiable through observable behavior, and the existing tests
  guard the discipline (mutation caught). No defect; no new
  instrumentation needed.
- Campaign note: this is exactly the "make hidden assumptions
  explicit" result the campaign asks to KEEP when it proves useful —
  here the commit-time documentation plus the observable-semantics
  test coverage already carry it.

### Exact commands
- python3/sed mutation of src/common/bloom.cpp insert() + cp
  backup/restore; ninja -C build-before bin/test_bitcoin;
  test_bitcoin --run_test=bloom_tests[/rolling_bloom]

### Limitations / queue
- The estimator flag lacks a dedicated unit suite (its guard is the
  functional suite + the profile) — a feerate-estimator unit battery
  with the same mutation style is queued.
- CCoinsViewCache dirty/fresh flag relationships (the deeper
  cache/backend contract) remain the next candidate module.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): CCoinsViewCache dirty/fresh flagged-list invariants — 9.18M per-op checks green; DISMISSED

### Draw
Rebuilt-queue draw (seed_raw=15860256885964587176,
masked=6636884849109811368, n=2, idx=0) -> coinsview-flags -> #57
(second cycle; c1 queue cell "CCoinsViewCache dirty/fresh flag
relationships"). Branch: audit/local-reasoning-c2 from ed268ed3d4
(#24 c6 journal tip).

### Hypothesis
The fork's intrusive flagged-list rewrite of the coins cache
(m_prev/m_next + sentinel + m_dirty_count replacing upstream's
whole-map flag scan) could lose the DIRTY ==> linked invariant on
some operation path, silently dropping dirty entries at flush
(lost writes) or corrupting the cursor walk.

### Coverage audit (duplicate search, src/test/coins_tests.cpp,
src/coins.cpp SanityCheck:552-583)
Existing: CCoinsViewTest sim (Add/Spend/Uncache/intermediate
Flush+Sync/stack churn, SelfTest ~1/1000 iterations) + cursor
contract tests (:402, :441) + SanityCheck asserting the full
invariant set. Residual gap probed here: PER-OP checking (vs
1/1000 sampling) on the production cache + the two guard rails
(overwrite throw, FRESH-misapplication throw) + the single
production DANGER caller.

### Experiment (driver /tmp/flags_fuzz.cpp; production CCoinsViewCache
subclassed for internals access, scripted CCoinsView backend,
deterministic LCG)
200,000 ops over parent+child caches (Add/Spend/Uncache/GetCoin/
AccessCoin/Sync/Flush/child-Flush/child-Sync), invariants after
EVERY op: DIRTY-set == linked-list membership (with Next/Prev
cross-link integrity + cycle bound), m_dirty_count accurate,
valid-state table (spent => dirty&&!fresh; fresh => dirty),
cachedCoinsUsage recomputed exact.
- g++ -O2 -std=c++20 -I src -I build-before/src /tmp/flags_fuzz.cpp
  -L build-before/lib -lbitcoin_node -lbitcoin_common
  -lbitcoin_consensus -lbitcoin_util -lbitcoin_crypto
  -lbitcoin_clientversion -L build-before/src/secp256k1/lib -lsecp256k1

### Results
- phase1: 9,181,831 checks, ZERO violations. 1,573 overwrite-guard
  enforcements (AddCoin possible_overwrite=false on an unspent
  cached coin throws — coins.cpp:126), 827 FRESH-misapplication
  guard firings under deliberate parent/child interleaving
  (coins.cpp:330-334) — state stayed invariant-consistent across
  every mid-BatchWrite throw (child rebuilt after each).
- phase2 (positive control): stale-child FRESH add + concurrent
  parent add -> Flush throws 'FRESH flag misapplied to coin that
  exists in parent cache' exactly as designed.
- phase3: EmplaceCoinInternalDANGER duplicate outpoint keeps the
  FIRST coin (try_emplace no-replace), flags sane; the silent-drop
  is backstopped by the snapshot count check
  (validation.cpp:6135 'Bad snapshot coins count') — its only
  production caller (snapshot load, validation.cpp:6051) cannot
  accept a duplicated-outpoint snapshot.
- Second verifier: build-before/bin/test_bitcoin
  --run_test=coins_tests -> No errors detected.

### Verdict
DISMISSED: the flagged-list rewrite maintains all dirty/fresh
invariants under per-op adversarial sequencing; both guard rails
fire exactly where the contract says; the one DANGER caller is
backstopped. No new oracle needed (sim + cursor tests + this
evidence cover the space).

### Caller-contract note (reachability of the FRESH throw)
Production never interleaves parent/child mutation the way the
fuzz does: the per-block overlay (CoinsViewOverlay) is the sole
writer during ConnectTip, mempool views are serialized by cs_main,
and snapshot chainstates have their own CoinsTip — the
FRESH-misapplication throw is defensive-only (upstream-identical
design), NOT a reachable hazard.

### Limitations / queue
- CoinsViewOverlay prefetch lifecycle (StartFetching/StopFetching/
  ResetGuard thread interplay) is a distinct fork-new surface —
  queued as the next local-reasoning cell.
- The estimator unit battery from c1 remains queued.

## Rotation note
Two cycles; m_all_zero and the coins-flag module closed.

## Cycle 3 (2026-07-30): CoinsViewOverlay prefetch — parallel(8) vs serial(0) validation differential ALL EQUAL; DISMISSED

### Draw
Rebuilt-queue draw (seed_raw=13942745621896996524,
masked=4719373585042220716, n=3, idx=1) -> overlay-prefetch ->
#57 (third cycle; c2 queue cell "CoinsViewOverlay prefetch
lifecycle"). Branch: audit/local-reasoning-c3 from c8a15033f2
(#57 c2 journal tip).

### Hypothesis
The fork's parallel prevout prefetch (CoinsViewOverlay +
-prevoutfetchthreads, default 8) could diverge from serial
validation on blocks exercising its ordering contracts:
same-block dependency chains (earlier_txids filter), duplicate-
prevout invalid blocks (queue tail-match), and disconnects with
fetched prevouts (Reset/StopFetching path).

### Coverage audit (duplicate search)
Existing: coins_view_overlay + coins_view_stacked fuzz targets
(REAL ThreadPool + StartFetching + ASan/UBSan build),
feature_block.py with -prevoutfetchthreads=8 (the invalid-block
acceptance suite), option-api-lifecycle c1 (option lifecycle
clean), differential-metamorphic queue note ("disconnect domain
with fetched prevouts not differentially tested" — the cell this
cycle closes). PeekCoin proven non-mutating (coins.cpp:30-49);
StopFetching idempotent (coins.h:734-751); ResetGuard [[nodiscard]]
+ caller holds it (validation.cpp:3105-3106); production default 8
(kernel/chainstatemanager_opts.h:25), framework default 1.

### Experiment (driver /tmp/ovpf_diff.py; two TestNodes, PortSeed 319)
Node A -prevoutfetchthreads=8, Node B =0; identical blocks to both
(built on A, mirrored as hex via submitblock): 101 maturity, 6
chain-heavy blocks (25-deep dependency chains + 10 fan-outs each),
one malleated duplicate-prevout block, then A-side invalidate(3)+
rebranch(4) forcing a reorg of the chain-heavy blocks on BOTH
(disconnect path with fetched prevouts).
Checkpoints: tip + gettxoutsetinfo("muhash") equality.

### Results
- ALL 9 checkpoints EQUAL (tips + muhashes): maturity, 6 chain-
  heavy blocks, post-invalid, post-reorg.
- Invalid block: both reject identically
  ('bad-txns-inputs-duplicate').
- 'prefetch queue was not fully consumed' warnings on A: 0 —
  the earlier_txids filter + in-order tail consumption held across
  every valid chain-heavy block (the filter's stress case).
- Prevout-related errors: 0; both nodes stopped clean.
- Harness iterations recorded: v1 gettxoutsetinfo needs hash_type
  ("muhash") in this fork; v2/v3 framework CTransaction computes
  txid/wtxid as properties (no rehash()); v4 CBlock field is
  hashMerkleRoot (CBlockHeader).

### Verdict
DISMISSED: parallel prefetch is behaviorally identical to serial
validation across dependency chains, invalid duplicate-prevout
blocks, and reorgs; the lifecycle contracts (PeekCoin purity,
StopFetching idempotency, guard scoping, AllInputsConsumed) hold
end-to-end. With the existing fuzz targets + feature_block(8) +
option-lifecycle c1, the overlay-prefetch cell is CLOSED.

### Exact commands
- python3 /tmp/ovpf_diff.py (output above); code refs: coins.h
  :583-820, coins.cpp:584-616, validation.cpp:1895, :3105-3106.

### Limitations / queue
- TSan-class race detection is not in the toolbox (ASan/UBSan
  only); the fuzz targets + PeekCoin non-mutation proof are the
  mitigation. If a TSan build ever lands, coins_view_overlay is
  the first target.
- Remaining #57 queue: estimator unit battery (from c1).

## Rotation note
Three cycles; m_all_zero, coins-flags, and overlay-prefetch all
closed. Estimator battery remains.

## Cycle 4 (2026-07-30): m_all_zero battery delivered (oracle O11) — M2/M3 gaps found by sweep, killed by the new tests

### Draw
Rebuilt-queue draw (seed_raw=1896312681528823955, masked same,
n=2, idx=1) -> estimator-battery -> #57 (fourth cycle; c1 queue
cell "feerate-estimator unit battery with the same mutation
style"). Branch: audit/local-reasoning-c4 from d264be344d
(#57 c3 journal tip).

### Method: mutation-first (c1 pattern) before writing any test
Baseline policyestimator_tests green, then sweep over the four
m_all_zero sites in block_policy_estimator.cpp:
- M1 Record set-omission (:272): KILLED by the pre-existing suite
  (3 failures) — Record path already guarded.
- M2 removeTx failAvg set-omission (:593): SURVIVED — a failAvg-only
  dirtying (tx evicted unconfirmed after >= scale blocks, zero
  Record calls) would never decay (frozen pessimism until the
  first Record). Real behavioral gap.
- M3 Read set-omission (:549): SURVIVED — a restored nonzero state
  (exactly the 675011ba86 failAvg-without-records case) would
  never decay on the fresh instance. Real behavioral gap.
- M4 skip-site deletion (:285): SURVIVED as designed — perf-only,
  bit-identical behavior.

### Battery delivered (src/test/policyestimator_tests.cpp,
feestats_dirty_decay_contracts, public-API only)
- Drives CBlockPolicyEstimator directly (processTransaction /
  processBlock / removeTx are public): add tx at height 1, age 4
  blocks, evict -> failAvg-only dirty state (asserts every other
  decayed average is EXACTLY zero and failAvg is bumped — the
  state the flag must not mistake for clean).
- Snapshot oracle: estimator.Write -> parse back (mirroring
  TxConfirmStats::Write field order) -> assert every avg equals
  snapshot * decay^3 after 3 empty blocks (bit-exact).
- Contract 2: Write -> Read into a fresh estimator -> same 3
  blocks on both -> bit-identical results required.
- Mutation verification: M2 re-applied -> 12 failures; M3
  re-applied -> 6 failures; restore -> suite green.

### Verdict
ORACLE DELIVERED + two real test gaps closed (M2/M3 set-omissions
were behaviorally unguarded). No production defect: with the flag
present, all four sites set/decay correctly — the battery pins the
contract so a future edit cannot silently drop a set.

### Exact commands
- mutation sweep: python3 inline edits (recorded above),
  cmake --build build-before --target test_bitcoin -j4,
  build-before/bin/test_bitcoin --run_test=policyestimator_tests
- battery: --run_test=policyestimator_tests/feestats_dirty_decay_contracts

### Limitations / queue
- The battery does not observe the skip-site (M4) — bit-identical
  by design; a perf regression there would need the profile
  harness (c1's IBD profile), not a unit test.
- Contract 2 relies on the estimator's own Write/Read round-trip;
  a Write-side bug would mask — but Write format is separately
  covered by reject_corrupt_fee_estimate_file_vectors.

## Rotation note
Four cycles; m_all_zero contracts now have a dedicated
mutation-verified battery. #57 cells all closed.

## Cycle 5 (2026-08-02, draw 214, raw=8524466033060714935 (63-bit), idx 11/17): successor-cell survey — FRESH/spent and m_dirty_count disciplines already battery-covered in-tree; campaign hypothesis space EXHAUSTED

### Survey (the two natural successors to the closed m_all_zero
cells)
1. FRESH/spent flush discipline (UTXO-resurrection class):
   AddCoin's spent-DIRTY no-refresh rule (coins.cpp:130-152) and
   the parent-propagation rule (:317-322) are battery-tested at
   the State-matrix level in coins_tests.cpp (:848-875,
   CLEAN/DIRTY/FRESH/DIRTY_FRESH x SPENT variants). COVERED.
2. m_dirty_count (fork-added counter): Assume pre/post snapshots
   at every mutation site (coins.cpp:34-107) + dedicated test
   ccoins_cursor_dirty_count_contracts (coins_tests.cpp:475) +
   GetDirtyCount assertion (:466). COVERED.

### Verdict
EXHAUSTED: every flag/dirty-state contract domain reachable
from the campaign's discipline has an in-tree battery; the two
successor candidates are already covered (verified, not
manufactured). Reopen condition: a NEW flag/counter domain
lands (e.g., the author's txgraph-retained-capacity branch's
usage accounting — watch #65's 🟡).

### Exact commands
- grep/sed line refs above (coins.cpp:34-152, 317-322;
  coins_tests.cpp:150/466/475/848-875).

### Limitations
- Coverage is by test-presence census + mutation-verify history
  (c4 battery), not fresh re-runs (the cells' own journals
  carry the green evidence).
