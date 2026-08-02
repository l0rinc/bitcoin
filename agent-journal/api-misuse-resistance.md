# Campaign #16 — api-misuse-resistance

Base: audit/resurrection @ 65ccf8fab9 (rotation ledger commit for #28).
Branch: audit/api-misuse. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-27): kernel C API `_at` getters — undocumented bounds precondition — CONFIRMED, doc-contract fix

### Hypothesis
The kernel C API (`src/kernel/bitcoinkernel.h`, 139 `btck_*` functions) is
consumed primarily by FFI/binding authors who read ONLY the header. Indexed
getters that enforce bounds solely via `assert` in the .cpp present an
invisible precondition: misuse is invisible in the contract and its behavior
is build-config-dependent.

### Trust boundary
Public C API -> external callers (C/Rust/Go bindings). Caller is trusted to
honor documented preconditions; the library must DOCUMENT them. The fork
already hardened 3 data-driven misuse paths (fdac04de17 null chain type,
a2b3434296 null block-file path lengths, 02a04893fa mismatched spent
outputs); this cycle covers the remaining programmer-error class.

### Findings of fact
- 6 `_at` getters take an index: transaction get_output_at/get_input_at,
  block get_transaction_at, block_spent_outputs get_..._spent_outputs_at,
  transaction_spent_outputs get_coin_at, witness_stack get_item_at.
- All 6 enforce bounds with plain `assert` (upstream commit 2cf136dec4c,
  2024). Five document NOTHING about bounds; the sixth
  (`btck_witness_stack_get_item_at`) already carries
  `@pre index < btck_witness_stack_count_items(witness_stack)`
  (bitcoinkernel.h:1755) — an existing in-header convention.
- This tree's builds (like upstream default) compile WITHOUT -DNDEBUG
  (verified: `ninja -t commands ...bitcoinkernel.cpp.o` shows -O2, no
  -DNDEBUG): misuse -> `__assert_fail` abort (verified in the .o disassembly:
  `cmp x0, x1; b.ls` -> `bl __assert_fail`, line 529). In a downstream
  NDEBUG build of the shared library the assert vanishes -> OOB read UB.
- Upstream precedent: bitcoin/bitcoin#33943 "kernel: don't use assert to
  handle invalid user input" (stickies-v) proposed nullptr/-1 returns +
  wrapper exceptions for exactly these getters; review raised
  return-code-consistency objections (janb84, billymcbip); draft ->
  needs-rebase -> CLOSED UNMERGED 2026-05-27. The semantic-redesign path
  stalled; the doc gap remains in master.
  (https://mirror.b10c.me/bitcoin-bitcoin/33943/)

### Misuse example (smallest plausible)
Off-by-one: `btck_transaction_get_output_at(tx,
btck_transaction_count_outputs(tx))`. Here: abort. NDEBUG build: reads one
CTxOut past vout; the returned "valid-looking" btck_TransactionOutput* then
feeds btck_transaction_output_get_amount/script_pubkey on adjacent heap.

### Fix (commit b6b48987a5)
Doc-only: add `@pre <index> < <count_accessor>(<owner>)` to the 5
undocumented getters, naming each matching count function, matching the
existing witness_stack @pre style. No semantic change; does not preclude a
future #33943-style redesign. Campaign rule applied: "prefer clarifying the
existing contract over API redesign".

### Verification
- `git diff` = 5 added @pre lines only.
- `cmake --build build-before -j4 --target test_kernel` -> link ok.
- `./build-before/bin/test_kernel` -> `*** No errors detected`.
- Count-function names verified against the header
  (btck_transaction_count_outputs/inputs, btck_block_count_transactions,
  btck_block_spent_outputs_count, btck_transaction_spent_outputs_count).

### Verdict
- CONFIRMED (ambiguous/undocumented contract with build-dependent failure
  modes). FIXED doc-side in b6b48987a5. Not a duplicate of #33943 (that PR
  changed semantics; closed unmerged).

### Limitations
- Did not audit all 139 functions; scoped to the index-bounds family.
  Other assert-guarded preconditions (null-tolerance beyond the documented
  Non-null params, flags combination checks like btck_script_pubkey_verify's)
  are separate leads.
- The C++ wrapper (bitcoinkernel_wrapper.h) inherits the C contract; no
  separate bounds layer there — noted, not changed.

### Next queue for this campaign
- Review callback-obligation docs (writer/user_data pattern) for
  re-entrancy/lifetime traps.
- Check destroy-with-NULL tolerance consistency (free()-style vs crash).

## Cycle 2 (2026-07-28): assert-only precondition sweep — 7 more doc gaps closed

### Method (the cycle-1 queue item, executed)
Extracted all 23 `assert(` in bitcoinkernel.cpp, classified each as
caller-precondition vs internal/unreachable, and mapped the former to
their public doc blocks in bitcoinkernel.h.

### Classification
- assert(false) unreachable markers (lines 169/209/222/233/1010/1529):
  not caller contracts. Skipped.
- Internal invariants (960 is actually caller-visible — see below;
  1402 result-vs-IsValid consistency): 1402 skipped.
- btck_block_header_create (1435): already documented ("Non-null,
  serialized header data (80 bytes)" / "must be 80") — the in-file
  precedent for the contract style.
- 6 indexed getters (529/540/738/1211/1323/1350): covered in cycle 1.
- 7 NEW doc gaps (fixed in 8b0e92b4a2 with @pre clauses):
  1. btck_transaction_create (512): raw null only if len==0
  2. btck_script_pubkey_create (577): same
  3. btck_script_pubkey_verify (671+679): flags subset of ALL;
     input_index < btck_transaction_count_inputs(tx_to) — an
     index-bounds miss from the cycle-1 _at sweep (index-taking
     function that is not named _at)
  4. btck_block_create (1171): raw null only if len==0
  5. btck_chain_parameters_create_signet (867): challenge null only
     if len==0
  6. btck_chainstate_manager_options_create (1015/1016): dir pointers
     null only if len==0 (len==0 itself is a graceful error return)
  7. btck_block_tree_entry_get_ancestor (960): 0 <= height <=
     btck_block_tree_entry_get_height(block_tree_entry);
     GetAncestor returns null outside -> assert(ancestor)

### Verification
- 8 @pre lines added (7 sites, two on script_pubkey_verify).
- `cmake --build build-before -j4 --target test_kernel` link ok;
  `./build-before/bin/test_kernel` -> `*** No errors detected`.

### Verdict
- CONFIRMED (7 underdocumented caller preconditions, same class and
  evidence shape as cycle 1). FIXED doc-side; no semantics changed.
- The kernel C API's assert-driven preconditions are now all either
  documented or classified as internal.

### Next queue for this campaign
- Callback-obligation docs (writer/user_data pattern): re-entrancy and
  lifetime traps.
- Destroy-with-NULL tolerance consistency across btck_*_destroy.
- C++ wrapper (bitcoinkernel_wrapper.h) vs C header contract drift.

## Rotation note
Two bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-28): btck_chainstate_manager_destroy(NULL) — lone non-null-tolerant destroy — CONFIRMED + FIXED (55f1fa334f)

### Draw
Random draw over the 69-goal eligible pool (pending + CYCLE-1, DONE and
prereq-missing 72/77 excluded, #21/#30 excluded as just-cycled):
seed=8986234353889504154 (od -N8 /dev/urandom), index 8 -> #16. Cycle-2
queue item "Destroy-with-NULL tolerance consistency across btck_*_destroy"
executed.

### Hypothesis / trust boundary
The kernel C API is consumed by FFI/binding authors who rely on the
free()-style convention that destroy functions accept NULL. H: the
btck_*_destroy family is inconsistent in NULL tolerance and the header
documents no contract, so error-path cleanup of a failed (null) handle
can crash.

### Method / evidence
Enumerated all 22 btck_*_destroy functions in bitcoinkernel.h; read every
implementation in bitcoinkernel.cpp:
- 21/22 are plain `delete ptr;` -> NULL-safe by C++ semantics.
- btck_chainstate_manager_destroy (bitcoinkernel.cpp:1131) flushes
  chainstates via btck_ChainstateManager::get(chainman) BEFORE the
  delete -> null dereference on NULL. Lone exception.
Header: no NULL-tolerance contract documented on any destroy.

### Misuse shape / callers
Error-path cleanup after a failed create: several btck_*_create functions
return nullptr on failure (e.g. btck_chainstate_manager_create), so
unconditional destroy of the handle is the natural binding pattern.
Upstream dedup: bitcoin/bitcoin master (7dea464d6b, fetched this cycle)
carries the identical unguarded dereference; no upstream fix
(git log --grep='destroy.*null|null.*destroy' -i empty). Not a #33943
duplicate (assert-bounds class, closed unmerged).

### Verification (staged clean/mutation/repaired)
1. Regression test FIRST: btck_destroy_null_tests in
   src/test/kernel/test_kernel.cpp calls all 22 destroys with nullptr.
   Unfixed library: 'memory access violation at address: 0x0' at the
   btck_chainstate_manager_destroy call (first 6 destroys pass — matches
   the code reading). Failing-before.
2. Fix: one-line `if (chainman == nullptr) return;` guard. Rebuild;
   full test_kernel suite: '*** No errors detected'. Passing-after.

### Why existing tests missed it
The kernel test suite only ever destroys successfully created objects;
no null-handle destroy existed anywhere in-tree.

### Verdict
- CONFIRMED (concrete unsafe behavior, smallest reproducer = the new
  test's 7th call). FIXED in 55f1fa334f with deterministic regression
  test. Severity: low (defensive-binding crash, no consensus/funds
  impact; API-hardening class). Upstream-applicable.

### Limitations / leads
- The now-uniform NULL-tolerance convention remains undocumented in the
  header (22 doc lines of churn declined this cycle; doc lead).
- Callback-obligation docs (writer/user_data pattern) still unaudited —
  next queue.
- C++ wrapper vs C header contract drift — parked (wrapper destroys via
  unique_ptr deleters, unaffected by this fix).

### Exact commands
- `cat >> src/test/kernel/test_kernel.cpp` (btck_destroy_null_tests)
- `cmake --build build-before -j4 --target test_kernel`
- `./build-before/bin/test_kernel --run_test=btck_destroy_null_tests`
  (failing-before: memory access violation at 0x0)
- `./build-before/bin/test_kernel` (passing-after: No errors detected)

### Next queue for this campaign
- Callback-obligation docs (writer/user_data pattern): re-entrancy and
  lifetime traps.
- Document the destroy-family NULL contract in the header (one shared
  sentence per destroy or a section note).
- C++ wrapper vs C header contract drift.

## Rotation note
Three bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 4 (2026-08-02, draw 192, raw=4334000722194619450 (63-bit), idx 5/41): LINEAGE REPAIR x2 — this journal was missing from agent/all-findings AND the c3 fix (55f1fa334f) never rode the lineage; HEAD had the live null deref; fix restored + verified; upstream master @556988790a STILL vulnerable (offerable)

### Discovery (the #66-c1 class, second recurrence this session)
Draw 192's journal read found api-misuse-resistance.md absent
from the archive worktree (pre-rotation gap, same as #4 c3).
Restoring it surfaced the worse half: the c3 fix commit
55f1fa334f (btck_chainstate_manager_destroy null-tolerance)
lives only on audit/api-misuse — agent/all-findings HEAD still
had the unguarded null dereference (bitcoinkernel.cpp:1192-1202:
btck_ChainstateManager::get(chainman) before any null check).

### Defect re-confirmation (at today's HEAD, pre-fix)
btck_chainstate_manager_destroy(nullptr) dereferences null:
22 btck_*_destroy functions, 21 are delete-only (null-safe by
free() convention); this one flushed chainstates first — the
lone exception, exactly as the c3 journal recorded. Upstream
master @556988790a carries the same unguarded code
(:1126-1130): the fix remains OFFERABLE upstream.

### Repair
- Cherry-pick 55f1fa334f onto audit/api-misuse-c4 (from archive
  tip): clean on bitcoinkernel.cpp, CONFLICT on test_kernel.cpp
  (lineage's #92-c2 abi battery vs the fix's destroy_null test).
  Resolution: union (both tests). Two labeled repairs followed
  (32643f9f98 left conflict markers + a lost closing brace;
  fixed forward, no rewriting, per archive policy).
- Verification: test_kernel builds; --run_test='*destroy_null*'
  green (includes btck_chainstate_manager_destroy(nullptr) —
  crashes pre-fix); '*abi_layout*' green; FULL test_kernel
  green. Failing-before is the c3-recorded memory access
  violation at 0x0; passing-after is the green suite.

### Verdict
CONFIRMED + REPAIRED: the lineage now carries both the journal
and the fix. URGENT entry added (✅ fixed + independently
verified; upstream offerable). The c1/c2-era fixes (b6b48987a5,
8b0e92b4a2) were verified in-lineage by #66 c2 already.

### Exact commands
- git cherry-pick 55f1fa334f (conflict -> union -> 2 repairs);
  cmake --build build-before --target test_kernel; test_kernel
  runs above; git show origin/master:...:1126-1130.

### Limitations / queue
- Sweep for OTHER pre-rotation journals missing from the archive
  (the #4/#16 class) is the natural follow-up cell.

## Postscript (2026-08-02, draw 233-redraw, raw=3871160351176686871 (63-bit), idx 1/2): lineage re-sweep (the c4 queue item) — 112/112 journals present in agent/all-findings, ZERO missing; the R23 repair holds

### Re-sweep (R23 method, #90 c3)
- Branch set: all audit/* agent-journal/*.md, sorted-unique =
  112.
- Archive set: agent/all-findings agent-journal/ = 112.
- comm -23: EMPTY. Every journal across every feature branch is
  in the archive — the c194 bulk repair plus the identity-ride
  pattern (journal commits riding both refs) has kept them
  exactly equal since.

### Verdict
CONFIRMED (lineage complete): no further pre-rotation gaps; the
c4 sweep queue item is closed with a clean re-measurement.

### Exact commands
- ls-tree loop + comm above (counts above).
