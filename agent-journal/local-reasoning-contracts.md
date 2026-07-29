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
