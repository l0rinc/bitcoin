# Campaign #94 — bindings-ffi-parity

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/bindings-ffi. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): C++ wrapper vs C header parity — one doc drift found + fixed (0a6c377ddb)

### Draw
Random draw over the 44-goal eligible pool: raw=1974142461351200227,
index 43 -> #94. Surface: the kernel C API's in-tree C++ wrapper
(queued by campaign #16 c2 as "wrapper vs C header contract drift").

### Parity matrix (wrapper site / C contract / verdict)
- check() null-return -> std::runtime_error: C functions document null
  on failure; the wrapper converts to exceptions uniformly. PARITY OK.
- Ownership: Handle copy-via-CopyFunc + DestroyFunc; unique_ptr Deleter
  guards null before destroy (wrapper never passes null to destroy
  functions; the C family is null-tolerant anyway post-55f1fa334f).
  PARITY OK.
- ScriptPubkeyApi::Verify: input_index passed unverified into the C
  assert — inherits the @pre documented at the C level (8b0e92b4a2);
  no wrapper-layer check, consistent with "wrapper inherits the C
  contract". precomputed_txdata nullptr path matches C's 'Nullable if
  taproot flag not set' exactly. PARITY OK.
- Output-on-failure: Verify returns bool + out-status — same shape as
  C's int + status. PARITY OK.
- View/Range lifetime: C documents on EVERY _at getter that the
  returned object "is not owned and depends on the lifetime of" the
  parent (bitcoinkernel.h:634-636 for get_output_at; same text on all
  six _at getters). The wrapper's View, Range/Iterator, and the
  GetOutput/GetInput/GetTransaction/GetCoin methods carried NOTHING —
  the wrapper-only reader sees TransactionOutputView with no hint it
  dies with the Transaction. DOC DRIFT (wrapper-side) — the classic
  FFI use-after-free shape. FIXED in 0a6c377ddb by stating the
  contract once at the two chokepoints (class View, class Range).

### Verdict
- CONFIRMED one documentation-side divergence (wrapper weakens a
  documented C lifetime contract by omission); FIXED. No core or
  wrapper CODE defect — the mirror design itself is faithful.
- Classification per campaign: documentation (wrapper), not core,
  not generated bindings.

### Verification
- Header-only change; test_kernel builds and passes
  (No errors detected).

### Limitations / leads
- Enum parity spot-checked structurally (wrapper aliases C enum
  underlying types); full value-by-value diff of all 11 enum pairs
  not run — queued (mechanical, could be a static_assert table).
- External bindings (Rust bitcoinkernel crate, etc.) not compared;
  the in-tree wrapper is the only maintained binding in-repo.
- Iterator validity under concurrent mutation: not a contract the C
  API offers either; single-threaded assumption documented nowhere —
  noted, not changed.

### Exact commands
- reads: bitcoinkernel_wrapper.h View/Handle/Range/Verify vs
  bitcoinkernel.h _at getter docs
- `cmake --build build-before -j4 --target test_kernel && ./build-before/bin/test_kernel`

### Next queue for this campaign
- Enum value table: static_assert parity of the 11 enum pairs
  (compile-time, catches future drift).
- Rust crate comparison if it lands in depends/CI scope.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): enum parity static_assert tables delivered + mutation-verified (073d543f26)

Base: f7b949ad29 (journal commit for #7 cycle-2 on
audit/resource-exhaustion-c2; ledger-lineage anchor audit/resurrection
@ 5d0155254c). Branch: audit/bindings-ffi-c2 (c1 journal carried).
Start state: clean (untracked scratch only).

### Draw
Random draw over the 51-goal pool (32 pending + 19 CYCLE-1; #7
excluded as just-cycled): raw=3778131996815310646, index 49 -> #94.
Queued item from c1: "Enum value table: static_assert parity of the
11 enum pairs".

### Inventory (11 mapping sites)
get_bclog_level (3), get_bclog_flag (11), cast_state (3),
cast_btck_warning (2), block-validation-result getter (9),
tx-validation-result getter (13), ValidationMode getter (direct
conditional, no enum), ChainType (btck→CChainParams switch, C-input
direction), wrapper enum aliases (self-consistent by construction),
ScriptVerifyStatus/Flags (not switched this cycle).

### Key structural facts (decide what each table must pin)
- All mappings are NAME-based switches (#92 c1): value-independent,
  so the pairing list is the drift surface, not the numbers.
- Five families are ALSO numerically identical today:
  SynchronizationState 0-2, Warning 0-1, LogLevel 0-2 (util/log.h:52
  Trace=0), BlockValidationResult 0-8, TxValidationResult 0-12.
- Two families deliberately differ: btck_ChainType (MAINNET=0,
  TESTNET=1, TESTNET_4=2, SIGNET=3, REGTEST=4) vs util/chaintype.h
  ChainType (MAIN=0, TESTNET=1, SIGNET=2, REGTEST=3, TESTNET4=4) —
  different ORDER by design; btck_LogCategory (index) vs
  BCLog::LogFlags (bitmask) — different kind.

### Deliverable (073d543f26, 101 insertions, 9 deletions)
- constexpr on all four cast functions; block/tx result switches
  extracted to constexpr helpers called by the getters (no mapping
  duplication — the assert table and runtime share one switch).
- Pairing static_asserts on every mapped case (36 asserts).
- Numeric-identity static_asserts for the five identical families
  (int-to-int), with a comment recording the name-mapping contract of
  the two non-identity families.

### Mutation controls
1. btck_Warning #define renumber: does NOT fire — correct, both sides
   reference the same #define name (documents what the table does and
   does not guard; recorded so nobody expects otherwise).
2. kernel::Warning enumerator reorder: FIRES at
   bitcoinkernel.cpp:268 (static assertion failed) as required.
3. Restored tree: builds; test_kernel passes (No errors detected).

### Verdict
- CONFIRMED (guard delivered): the pairing and the numeric identity
  of all mapped enum families are now compile-time pinned with a
  proven tripwire; no drift found today (all tables hold on clean
  HEAD).
- DISMISSED (production defect): none — mappings were correct.

### Exact commands
- greps/seds: bitcoinkernel.h enum surface, notifications_interface.h,
  kernel/warning.h, util/chaintype.h, util/log.h,
  consensus/validation.h
- ninja -C build-before bin/test_kernel && test_kernel
- mutation 1: sed btck_Warning #define 1->7 (no fire, documented)
- mutation 2: python3 reorder of kernel::Warning (fire at :268)

### Limitations / queue
- btck_ScriptVerifyStatus/Flags and btck_TxValidationErrorDetails
  enums have no switch mappings to pin (direct/conditional or
  reserved) — noted, no table.
- The numeric-identity asserts deliberately document that identity is
  today's state, not a promised contract; future intentional
  divergence must edit both the switch and the tripwire (by design).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-08-02, draw 193, raw=14784019335709398726, masked 5560647298854622918, idx 38/40): C++ wrapper ownership + copy-family null contract — templates sound; copy family is DOCUMENTED nonnull (ARG_NONNULL(1), live SIGSEGV + compiler warning); DISMISSED

### Hypothesis
The wrapper layer (bitcoinkernel_wrapper.h Handle/UniqueHandle)
or the 16 btck_*_copy functions might carry the same null-deref
shape as the destroy family fixed in #16 c4.

### Evidence
- Wrapper templates (bitcoinkernel_wrapper.h:343-404): deep-copy
  ctor via CopyFunc; copy-and-swap assign; move ctor/assign null
  the source; ~Handle DestroyFunc(m_ptr) on possibly-null (moved-
  from) — safe per the destroy family's null convention (proven
  by #16 c4's destroy_null test); UniqueHandle's Deleter null-
  checks (:388-394); check() throws on null construction
  (:183-190). Sound.
- Copy family: all 16 btck_*_copy route through Handle::copy
  (bitcoinkernel.cpp:125-129) -> get(ptr) = *reinterpret_cast
  (null-deref on null). BUT the header declares every copy
  BITCOINKERNEL_ARG_NONNULL(1) (e.g. :1077-1078) — null is a
  DOCUMENTED contract violation. Live probe (/tmp/btc94c3/
  copynull.c): gcc warns 'argument 1 null where non-null
  expected' at COMPILE time, and btck_script_pubkey_copy(NULL)
  dies SIGSEGV (exit 139) at runtime — annotated-misuse outcome,
  not a latent defect. Destroys carry NO such annotation
  (free()-style convention — the #16 c4 asymmetry rationale).
- Upstream @556988790a has the identical Handle::copy code.

### Verdict
DISMISSED: the copy family's null behavior is the documented
nonnull contract with both compile-time and runtime teeth; the
wrapper ownership is leak/double-free-clean. No fix warranted
(changing annotated-nonnull behavior would ALTER the upstream
API contract — out of minimal-diff scope; the destroy family's
unannotated asymmetry was the genuine gap, already fixed).

### Exact commands
- sed/grep line refs above; gcc + ./copynull (warning + exit 139);
  git show origin/master copy code.

### Limitations / queue
- Copy-of-moved-from C++ wrapper (CopyFunc(nullptr) through
  wrapper copy-ctor) is unspecified-behavior misuse; check()
  would throw only if CopyFunc returned null gracefully — noted
  as wrapper-internal, unreachable from conforming C++.
- Enum/struct parity cells closed in c2; no #94 cells queued.
