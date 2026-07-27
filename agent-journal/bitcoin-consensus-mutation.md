# Journal: Bitcoin consensus mutation-score and kill-test audit (goal 85)

Campaign: severity-first rotation (cycle 2). Consensus divergence is the most severe
bug class in this codebase, so this campaign runs before wallet/P2P/secp cycles.
Base: audit/resurrection @ ac730b2dd0. Build: build-before (Release, gcc, ~10 min
incremental). Machine: Cortex-A76 — narrow test runs only, no full suites.

## Method per mutant
1. Hypothesis + reachability trace (which validation path, which BIP/rule).
2. Apply the minimal mutant (invert/shift/skip) in a scratch working tree state.
3. Incremental rebuild; run the narrowest covering test(s).
4. Verdict: KILLED-BY <test> / SURVIVED / EQUIVALENT, with key output.
5. Surviving reachable non-equivalent mutants = oracle gap → smallest
   distinguishing test, one independent commit. Mutants are NEVER committed.

## Ranked site list (by divergence impact)

### Tier 1 — inflation / spend-validity (direct consensus divergence)
- A. tx_check.cpp:41-45 duplicate-input rejection (CVE-2018-17144 class).
- B. tx_check.cpp:24-34 output value range + running-total MoneyRange
  (CVE-2010-5139 class, overflow of nValueOut).
- C. tx_verify.cpp:196-199 nValueIn < value_out (fee/spend balance).
- D. tx_verify.cpp:179-182 coinbase maturity off-by-one (< vs <=).
- E. tx_verify.cpp:185-188 input value MoneyRange.

### Tier 2 — timing/locktime (cross-implementation divergence risk)
- F. tx_verify.cpp:21 IsFinalTx locktime vs height/time comparison.
- G. tx_verify.cpp:88/90 BIP68 relative-lock -1 semantics (nLockTime "last
  invalid" conversion).
- H. tx_verify.cpp:101 EvaluateSequenceLocks >= vs > boundary.
- I. tx_verify.cpp:51 fEnforceBIP68 version/flag gate.

### Tier 3 — block-level structural
- J. tx_check.cpp:14-17 vin/vout empty rejection.
- K. tx_check.cpp:19-21 oversize (weight) rejection boundary.
- L. tx_check.cpp:49-50 coinbase scriptSig size [2,100] bounds.
- M. validation.cpp ContextualCheckBlock: BIP34 height, witness commitment,
  MAX_BLOCK_WEIGHT/sigops accumulation, MTP.
- N. validation.cpp GetBlockSubsidy halving schedule (site TBD).

## Mutant ledger

- M1 (site A, tx_check.cpp:43): duplicate-input rejection disabled
  (`&& false`). Hypothesis: directly unit-covered. Result: KILLED-BY
  test_bitcoin transaction_tests/tx_invalid — "Tx unexpectedly passed" on the
  BADTX duplicate-prevout vector (tx_invalid.json). Reachability: consensus,
  CVE-2018-17144 class. Oracle adequate.
- M2 (site B, tx_check.cpp:32): running-total MoneyRange overflow check
  disabled. Result: KILLED-BY transaction_tests/tx_invalid — 2-output vector
  with sum > MAX_MONEY unexpectedly passed. CVE-2010-5139 class. Oracle
  adequate.
- M3 (site C, tx_verify.cpp:196): value-in >= value-out balance check disabled
  (`if (false)`). Result: KILLED-BY
  transaction_tests/checktxinputs_invalid_transactions_test — expected
  "bad-txns-in-belowout", got "bad-txns-fee-outofrange". IMPORTANT NUANCE:
  the mutant is semantically EQUIVALENT at consensus level — the subsequent
  txfee MoneyRange check (tx_verify.cpp:203) rejects every tx the removed
  check would have caught (negative fee is out of range; no int64 overflow
  possible since both operands <= MAX_MONEY). The kill came from the
  reject-reason oracle, not from a validity difference. The two checks are
  defense-in-depth redundant; only removing BOTH would allow inflation.
  Verdict: killed + equivalent. Oracle adequate (and precise to reason codes).
- M4 (site D, tx_verify.cpp:179): coinbase maturity boundary `<` → `<=`
  (requires depth 101 instead of 100 — network-splitting direction).
  Unit tests (transaction_tests) do NOT kill it. KILLED-BY functional
  mempool_spend_coinbase.py: mutant rejects a legitimate spend at depth 100
  ("tried to spend coinbase at depth 100"). Runtime ~1s. Boundary oracle
  adequate at functional level; unit-level gap noted (acceptable — the
  functional test is the boundary oracle).
- M5 (site H, tx_verify.cpp:101): EvaluateSequenceLocks `>=` → `>` (accepts
  one-block-early relative-lock txs). KILLED-BY functional
  feature_bip68_sequence.py: "No exception raised" where boundary rejection
  expected. Runtime ~1s. BIP68 boundary oracle adequate.
- M6 (site M, validation.cpp:4055): witness-merkle commitment match disabled
  (`false && memcmp(...)`) — accepts blocks with a wrong witness commitment.
  KILLED-BY functional p2p_segwit.py (AssertionError, bad-commitment block
  accepted where rejection expected). Runtime ~12s. Segwit commitment oracle
  adequate.

## Cycle 2 verdict

6 mutants applied across 3 consensus check sites (tx_check.cpp, tx_verify.cpp,
validation.cpp). ALL KILLED: 2 by unit vectors (tx_invalid.json),
1 by unit reason-code oracle (semantically equivalent mutant — see M3),
3 by targeted functional tests (mempool_spend_coinbase, feature_bip68_sequence,
p2p_segwit). No surviving reachable non-equivalent mutant → no oracle-gap fix
commit required this cycle. All mutants reverted; tree clean; baseline
transaction_tests green after revert.

Notable observations (not defects):
- M3: CheckTxInputs in-belowout and fee-outofrange are mutually redundant
  defense-in-depth; the unit oracle pins exact reason strings, so any
  reordering/removal is detected.
- M4/M5: consensus boundaries rely on functional tests, not unit tests —
  expected, but worth knowing for future refactors that only run unit tests.

## Next queue
(after cycle: rotation → goal 88 wallet key-loss → goal 89 P2P DoS;
residual mutants worth future cycles:
- site E: input-value MoneyRange removal (needs utxo fixture; check
  checktxinputs_invalid_transactions_test coverage directly)
- site F: IsFinalTx locktime threshold boundary (LOCKTIME_THRESHOLD mixing
  height/time; feature_cltv.py is the likely oracle)
- site G: BIP68 -1 semantics removal in CalculateSequenceLocks (subtler than
  M5; same functional oracle, different line)
- site I: fEnforceBIP68 version<2 gate flip (accept v1 txs with relative
  locks — check feature_bip68_sequence version coverage)
- site J/K/L: vin/vout-empty, oversize, coinbase-length bounds
  (feature_block.py / mining_template_verification.py oracles)
- site N: GetBlockSubsidy halving off-by-one (unit: validation tests? subsidy
  is exercised implicitly everywhere — find the explicit oracle or note gap)
- script interpreter flag mutants (SCRIPT_VERIFY_DERSIG/NULLDUMMY/CLEANSTACK
  skips) — p2p_segwit.py + feature_taproot.py oracles)
