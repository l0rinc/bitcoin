# Exhaustive and Algebraic-Invariant Audit

## Cycle 181 start: persistence and iterator identity matrix

- Fresh gate: `git fetch origin master` succeeded. Branch:
  `uber-cycle-181-exhaustive-algebraic-20260731`. Start HEAD:
  `41bc1fee6cc9e1dc7b08421bb1d05f9b468a2d29`; origin/master:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence:
  `origin/master...HEAD = 42 1150`.
- Exact selector: `shuf -i 0-98 -n 1` -> `18`,
  `exhaustive-algebraic`. The prior GCS `MatchAny` and compact-target
  canonicalization cells are closed; this cycle opens a distinct persistence
  and iterator identity cell. Catalog, prompt, corrected TSV, protocol, and
  state hashes are recorded by the fresh gate. The TSV has a header plus 99
  records with IDs 0 through 98. Known untracked artifacts remain preserved;
  PIDs `777094` and `956381` remain unrelated long-running tests and must not
  be stopped. Root storage is critically constrained, so all scratch data
  stays under `/data/my_storage/tmp`.

### Scope and initial queue

Audit one production persistence relation at a time, stating its exact
domain, authoritative representation, failure behavior, and independent
oracle before changing code. Prioritize LevelDB wrapper and index relations
not already covered by the BaseIndex readiness/restart audit:

1. `CDBWrapper`/`CDBBatch` write-read-delete and iterator ordering across
   snapshots, overwrites, and restart, with a model map as the oracle.
2. `CDBIterator` seek/next/prev/validity behavior at empty, singleton,
   duplicate-overwrite, prefix, and end boundaries, including failed reads.
3. A database-backed index's durable best-block/cache reconstruction relation
   after clean close and injected write/flush failure, only if the first two
   cells do not expose a stronger defect.

Search history, callers, existing tests, and prior journals before each
candidate. A source finding requires a deterministic failing-before test or
independent model counterexample, a minimal repair, and a mutation that the
oracle kills. Do not confuse documented LevelDB semantics with a Bitcoin
wrapper contract, and do not commit a disposable matrix without a confirmed
production defect.

## Cycle 50: GCS `MatchAny` identity and checked reconstruction

- Gate: `HEAD=dd77f06eaf4b2020d9c4cd1692b2d9be0f084999`; `origin/master=7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD=2 867`.
- Selector: `shuf -i 0-98 -n 1` -> `18`, `exhaustive-algebraic`.
- Scope decision: prior block-filter work already covered construction counts, constructed/encoded GCS round trips, basic-filter element membership, and a 100-element `MatchAny` relation. This cycle closed a distinct bounded identity cell by exhaustively varying small filter and query subsets and checking the checked decoder as a second implementation path. No production behavior was changed.

### Contract and domain

For a fixed GCS filter `F` and query set `Q`, the optimized multi-query operation must satisfy:

`F.MatchAny(Q) == OR(q in Q) F.Match(q)`.

The empty query set must return false, and a singleton query must agree exactly with `Match`. This is an identity about the filter's probabilistic predicate; it does not assert that a false positive is impossible. A filter reconstructed from its encoded bytes with checked decoding must preserve `GetN()`, the encoded bytes, and the same aggregate query result.

The disposable test used fixed SipHash keys, `P=3`, `M=17`, and the four one-byte elements `{0}`, `{1}`, `{2}`, `{3}`. It enumerated all 16 included-element subsets and all 16 query subsets, for 256 filter/query pairs. Every nonempty query also exercised the singleton identity. Each constructed filter was reconstructed through `GCSFilter(params, encoded, false)` and checked for count, byte-for-byte encoding, and aggregate-query parity.

### Evidence

- Disposable matrix build: `env TMPDIR=/data/my_storage/tmp/cycle50-gcs-exhaustive cmake --build build_unit_clang19 --target test_bitcoin -j2`.
- Unmutated matrix run: `build_unit_clang19/bin/test_bitcoin --run_test=blockfilter_tests --log_level=test_suite --report_level=short --catch_system_errors=no --color_output=false`; 9 selected test cases passed, 1,555/1,555 assertions passed. Raw log: `/data/my_storage/tmp/cycle50-gcs-exhaustive-baseline.log`.
- Mutation: temporarily replaced `GCSFilter::MatchAny` in `src/blockfilter.cpp` with `return false;`, rebuilt `test_bitcoin`, and ran only `blockfilter_tests/gcsfilter_matchany_exhaustive_small_domain`. It exited 201. The first failure was `src/test/blockfilter_tests.cpp:122`, `filter.MatchAny({value}) == filter.Match(value)` with `[false != true]`; aggregate and checked-decoder assertions failed afterward. The mutation run recorded 450/1,056 assertions passed and 606 failed. Raw log: `/data/my_storage/tmp/cycle50-gcs-exhaustive-mutation.log`.
- Restoration: removed the disposable test and restored the production implementation, rebuilt the target, and reran `blockfilter_tests`; 8 selected test cases passed, 499/499 assertions passed. Raw log: `/data/my_storage/tmp/cycle50-gcs-exhaustive-restored.log`.
- `git diff --check` passed, and `src/blockfilter.cpp` plus `src/test/blockfilter_tests.cpp` returned clean. The temporary test was intentionally not retained because the existing deterministic and fuzz oracles already cover the durable production contract; the exhaustive result is preserved here as evidence.

### Verdict and limits

No source defect was confirmed. The `MatchAny` OR identity and checked reconstruction relation survived the bounded exhaustive matrix, and the oracle detected a deliberately incorrect implementation. The remaining uncertainty is outside this cell: the matrix used only four one-byte elements and one non-production parameter pair, did not enumerate malformed encoded filters, and did not compare an independent backend. Existing block-filter tests and fuzz coverage remain the evidence for malformed encodings and production BASIC parameters.

Next queue: select another distinct identity cell, prioritizing a production state transition or serialization/recovery relation not already closed by the prior property campaigns. Do not reopen this `MatchAny` cell without a new implementation path, parameter domain, or regression signal.

## Cycle 64: compact target canonicalization at exponent boundaries

- Gate: `HEAD=fda626b39ebae557a7b628f300b764aa58f84249`; `origin/master=7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD=2 899`.
- Selector: `shuf -i 0-98 -n 1` -> `18`, `exhaustive-algebraic`.
- Scope decision: the previous GCS `MatchAny` identity is closed. This cycle selects the independent compact-target encoding relation in `arith_uint256`, covering canonical mantissa quantization, sign-bit carry, exponent transitions, overflow flags, and conversion boundaries used by proof-of-work validation.

### Contract and domain

`SetCompact` decodes the 32-bit signed-magnitude compact representation into a non-negative 256-bit value plus negative/overflow metadata. For a non-negative value `x` representable by a compact mantissa, `SetCompact(x.GetCompact()) == x`; for arbitrary encodings, `GetCompact(SetCompact(c))` must be the canonical encoding of the decoded value and must not retain a sign bit for zero. The relation is intentionally many-to-one for discarded low mantissa bytes, so tests must compare decoded values rather than raw inputs except for canonical encodings.

The boundary matrix will enumerate compact exponents around 0, 1, 2, 3, 4, 31, 32, 33, 34, and 35; mantissas around zero, the sign bit, `0x007fffff`, and the carry threshold `0x00800000`; all 256 possible low-byte truncations at selected exponents; and values at the 256-bit top boundary. Separate rows will check negative zero, negative nonzero, and overflow encodings. A second matrix will construct values from exact powers of two and mantissa limits, then round-trip them through `GetCompact` and `SetCompact`.

### Evidence plan

- Inspect the implementation, existing hand-written compact tests, proof-of-work tests, and the integer/pow fuzz contracts before changing code.
- Add a disposable focused test or standalone harness only after the domain table is fixed. Require value, canonical encoding, sign, overflow, and `ArithToUint256`/`UintToArith256` checks.
- Run the focused unit suite and relevant proof-of-work tests in `build_unit_clang19`, with a stable `TMPDIR`; preserve raw logs under `/data/my_storage/tmp/`.
- Temporarily mutate one canonicalization branch and one oracle assertion to prove the matrix detects both a production result error and a weak test. Restore all disposable source before committing.

### Evidence and verdict

- A disposable `boost::multiprecision::cpp_int` oracle checked 902 compact encodings: exponents 0 through 40, 11 mantissas around zero, truncation, sign, and overflow thresholds, and both sign-bit states. It independently decoded the value modulo 256 bits, checked the documented sign/overflow flags, and checked canonical reparse. A second value matrix covered exponents 3 through 32, five high-mantissa states around `0x00800000`, and four low tails, for 600 additional generated values. The matrix passed 7,515 assertions.
- The matrix also documented a deliberate invalid-input edge: an encoding with an exponent beyond the 256-bit storage width can report `negative=true` from its nonzero compact mantissa while its decoded 256-bit value is zero. Canonical `GetCompact` correctly drops that sign because compact negative zero is not representable; the test expected this only for zero decoded values.
- The restored focused matrix passed 1 case and 7,515 assertions. The complete `arith_uint256_tests` suite passed 14 cases and 18,255 assertions. The proof-of-work suite passed 17 cases and 1,085 assertions. Raw logs are `/data/my_storage/tmp/exhaustive-algebraic-cycle64-boundary3.log`, `/data/my_storage/tmp/exhaustive-algebraic-cycle64-arith.log`, and `/data/my_storage/tmp/exhaustive-algebraic-cycle64-pow2.log`.
- A temporary sign-carry mutation changed the `0x00800000` check in `GetCompact`; the focused matrix terminated at the existing mantissa assertion in `src/arith_uint256.cpp:212`. A temporary decoder-shift mutation changed the small-exponent shift; the focused matrix terminated with status 132. Both mutations were restored. Raw controls are `/data/my_storage/tmp/exhaustive-algebraic-cycle64-mutation.log` and `/data/my_storage/tmp/exhaustive-algebraic-cycle64-mutation2b.log`.
- The Clang 19 ASan/UBSan libFuzzer `integer` target completed 1,000 fixed-seed runs with no diagnostic, and `pow` completed 250 fixed-seed runs with no diagnostic. Logs are `/data/my_storage/tmp/exhaustive-algebraic-cycle64-fuzz-integer.log` and `/data/my_storage/tmp/exhaustive-algebraic-cycle64-fuzz-pow.log`.
- The first proof-of-work invocation used a nonexistent `TMPDIR` and failed in the test fixture before exercising the target; creating the directory and rerunning passed. No source or durable test change was justified. `git diff --check` passed and no relevant process remains running.

### Limits and handoff

No source defect was confirmed. The compact decoder agrees with the independent bounded arithmetic oracle, canonical re-encoding is stable, and the proof-of-work consumer preserves its existing contracts. The matrix did not exhaust all 2^23 mantissas or every arbitrary 256-bit value, and libFuzzer smoke runs started from empty corpora; existing unit/fuzz coverage remains the evidence outside those domains.

Next queue: draw a fresh goal after rechecking the gate. Do not reopen this compact cell without a new consensus vector, cross-implementation divergence, compiler/architecture result, or production regression.
