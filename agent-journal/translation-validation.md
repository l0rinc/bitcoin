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

## Cycle 2 (2026-08-02, draw 174, raw=3650315433786879391 (63-bit), idx 1/3): optimization-level output differential — g++ {-O0,-O2,-O3} x clang++-18 {-O0,-O2} byte-identical on the consensus arithmetic boundary corpus; DISMISSED; campaign COMPLETE

### Hypothesis
H (c1 queue): optimizer level changes the RESULT of consensus
header-only arithmetic (UB exploitation, contract violation,
__int128 lowering divergence) — falsifiable by running an
identical boundary corpus through the same TU compiled at
multiple opt levels and compilers, byte-diffing outputs.

### Harness (/tmp/btc78c2/optdiff.cpp, preserved)
Single-TU: util/feefrac.h (Mul/MulFallback __int128 vs pair,
Div/DivFallback round_down, Add) + consensus/amount.h
(MoneyRange, CAmount add) over the #100-c4 boundary corpus
(23 a-values x 12 b-values incl. INT32_MIN/MAX, INT64_MAX/4,
21e14, 1<<40 edges) = 742 output lines. No linker deps beyond
the headers (arith_uint256 arm dropped: mul/GetHex are NOT
header-only — recorded). g++ -O0 needs an assertion_fail stub
because Assume is a REAL call without optimization — exactly
c1's confirmed contract (erasure only under optimization); the
stub aborts and never fires on these valid inputs.

### Result
g++ 13.3 {-O0,-O2,-O3} + clang++ 18.1 {-O0,-O2}: all five
binaries' outputs md5-identical
(382248379b24e73a5a37fd3878e873e4, 742 lines each). Zero
opt-level or cross-compiler divergence.

### Verdict
DISMISSED: the fork's feefrac/Amount arithmetic is optimization-
invariant and compiler-invariant at the boundary corpus,
consistent with #100 c4's backend-exactness and UBSan-clean
results. The c1 transformation contract is now backed at both
ends: erased in optimized builds (c1 disassembly), real-but-
correct at -O0 (this cycle).

### Exact commands
- g++/clang++-18 -O{0,2,3} -std=c++20 -I src optdiff.cpp; md5sum
  of the five output files above.

### Limitations / queue
- Alive2/IR-level validation remains host-unavailable (c1 note).
- No #78 cells remain queued.

### Campaign #78: COMPLETE
c1 Assume-erasure binary contract (CONFIRMED); c2 opt-level
arithmetic differential (DISMISSED).
