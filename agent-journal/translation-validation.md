# Campaign #78 — translation-validation

Base: 3364770337 (journal commit for #92 cycle-1 on
audit/abi-alignment; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/translation-validation. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): Assume-erasure contract validated at the binary — fork hardening is exactly zero-cost in release

### Draw
Random draw over the 54-goal pool (35 pending + 19 CYCLE-1; #92
excluded as just-cycled): raw=820245210827069523, index 21 -> #78.

### Cell: compile-time transformation of the fork's Assume pattern
Contract (src/util/check.h): upstream bare Assume(x) EVALUATES x in
release but never traps (inline_assertion_check<IS_ASSERT=false> only
calls assertion_fail when std::is_constant_evaluated() ||
G_ABORT_ON_FAILED_ASSUME). The fork's hardening instead wraps whole
verification blocks in `if constexpr (G_ABORT_ON_FAILED_ASSUME)` —
stronger erasure: the check is not even evaluated in release.
Configuration: build-before has BUILD_FOR_FUZZING=OFF and
ABORT_ON_FAILED_ASSUME undefined -> G_ABORT_ON_FAILED_ASSUME=false.

### Binary evidence (build-before/bin/bitcoind, gcc -O2 Release,
assertions ON per the fork's #error "Cannot compile without
assertions!")
- nm: exactly 1 assertion_fail reference in the whole binary
  (the Assert machinery itself — present as designed).
- PeerManagerImpl::ActiveTipChange (0xe82e0, 101 instructions): calls
  only pthread_mutex_lock, TxDownloadManager::ActiveTipChange,
  unique_lock::unlock, stack_chk_fail, __throw_system_error,
  _Unwind_Resume. No snapshot loops, no Assume machinery, no
  assertion_fail path.
- TxDownloadManager::ActiveTipChange (0x183630, 93 instructions):
  calls only stack_chk_fail, operator new (lazy filter alloc),
  CRollingBloomFilter ctor, operator delete. The fork's 6-Assume
  state-snapshot block (peer-info/txrequest/orphanage counts) is
  fully erased — no all_of scans, no Assume calls.

### Verdict
- CONFIRMED (transformation contract holds): the fork's
  if-constexpr Assume pattern compiles to zero release code — the
  hardening adds no production overhead, and no miscompilation or
  leaked evaluation was observed.
- DISMISSED (defect hypothesis): no optimization-dependent behavioral
  divergence in the audited pattern.

### Exact commands
- grep -B3 -A15 'define Assume|G_ABORT_ON_FAILED_ASSUME' src/util/check.h
- nm -C build-before/bin/bitcoind | grep -c assertion_fail
- objdump -d --start-address/--stop-address on the two
  ActiveTipChange symbols; CMakeCache BUILD_FOR_FUZZING=OFF

### Limitations / queue
- Alive2/IR-level validation unavailable on this host (no alive2) —
  disassembly inspection substituted; noted.
- Optimization-level output differential (-O0 vs -O2 on arithmetic
  vectors) not run this cycle; queued (needs a cheap single-TU
  harness, not full-tree rebuilds).
- The estimator m_all_zero and bloom m_all_zero fast paths are plain
  bools — transformation-trivial; verified behaviorally in their own
  campaigns (#63, #22), not re-validated here.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
