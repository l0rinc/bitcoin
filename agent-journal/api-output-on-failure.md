# Campaign #46 — api-output-on-failure

## Cycle 1 (2026-07-29): kernel C API output-on-failure audit — 5 surfaces defined, 1 robustness note

### Draw
Random draw over the 4-goal eligible pool (3 pending + 1 CYCLE-1,
#25 excluded as just-cycled): raw=7914243779083168148, index 0 ->
#46 (first cycle). Branch: audit/api-output-on-failure from
e4dbd9c271 (#25 c2 bookkeeping; lineage anchor audit/resurrection @
5d0155254c). Start state: tracked-clean. Catalog note: #46's
campaign-focus block holds build-parity text — same offset artifact
class; title+slug authoritative.

### Cell selection
The kernel C API (bitcoinkernel.h/cpp) is where output-on-failure
bugs bite hardest: C callers see no exceptions, only out-params and
return codes. Audited the functions that write through pointers or
return fresh objects on error paths.

### Surfaces audited (all DEFINED)
1. btck_script_pubkey_verify (bitcoinkernel.cpp:700-735): status
   set on every reachable path (ERROR_INVALID_FLAGS_COMBINATION,
   ERROR_SPENT_OUTPUTS_REQUIRED, OK before VerifyScript). Contract
   ("will be set to an error code if the operation fails, or OK
   otherwise") honored.
2. btck_block_read (:1303-1311): failure returns nullptr; no
   out-param written.
3. btck_chainstate_manager_process_block (:1441-1452): _new_block
   out-param always written — verified the uninitialized-read
   hypothesis DOWN at the callee: ProcessNewBlock sets
   `if (new_block) *new_block = false;` as its first statement
   (validation.cpp:4595), before every failure path.
4. btck_chainstate_manager_process_block_header (:1454-1469):
   nullptr on caught exception.
5. btck_script_pubkey_to_bytes (:620-624): single-shot callback
   over in-memory data; no partial-output state in the API itself.

### Robustness note (not a defect)
btck_script_pubkey_verify line 717: `assert(input_index <
tx.vin.size())` — a plain assert on a public C API parameter. With
asserts on (this tree's builds) it aborts; in an NDEBUG downstream
consumer an out-of-range index is UB. No status-code path exists
for it, and the header documents no precondition. Upstream-matching
(kernel API is explicitly WIP per CMakeLists comment); no in-tree
caller can reach it. Recorded for the upstream-watch file, not
inflated.

### Verdict
DISMISSED: no output-on-failure defect on the audited surfaces;
the one plain-assert precondition is upstream WIP-matching and
unreachable in-tree.

### Exact commands
- grep BITCOINKERNEL_API bitcoinkernel.h (139 decls; non-destroy
  subset triaged)
- reads: bitcoinkernel.cpp:620-755/1303-1342/1441-1469,
  validation.cpp:4589-4629 (ProcessNewBlock new_block init)

### Limitations / queue for cycle 2
- Remaining getter/copy functions follow the same two shapes
  (ref-return or fresh-object); spot-checked, not enumerated.
- RPC output-on-failure is structurally exception-transactional
  (no partial UniValue); noted, not re-proven per endpoint.
- btck_chainstate_manager_import_blocks and worker-thread paths
  (callback reentrancy on failure) unexamined — next cell if a
  cycle lands here.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-29): import_blocks output-on-failure + callback reentrancy constraint — clean; one undocumented constraint verified

### Draw
Re-rank draw (last of the rebuilt 5-cell queue; singleton):
#46 (second cycle; c1 queue cell "btck import_blocks and worker-
thread callback reentrancy"). Branch: audit/api-output-c2 from
7787a0ba76 (#42 c3 bookkeeping).

### btck_chainstate_manager_import_blocks (bitcoinkernel.cpp:1207-1228)
- Whole body under try/catch: -1 on exception, 0 on success; no
  out-params written on failure. Clean.
- Null path entries skipped silently (documented behavior);
  lens-array-nullable contract honored (:1214-1216, the fork's own
  doc-comment matches the header).
- uint32_t loop var vs size_t len: cosmetic only (a >4B-entry
  paths array is not a realistic input).

### Callback reentrancy (the worker-thread cell)
- The validation-interface callbacks (BlockConnected etc.) execute
  INLINE on the validation thread with cs_main held: validation.cpp
  :3468-3471 (inside ActivateBestChain's cs_main scope) ->
  kernel::Notifications -> KernelNotifications::BlockConnected
  (bitcoinkernel.cpp:408-415) calls the consumer's callback with
  no thread handoff.
- Consequence: a consumer callback that calls back into
  btck_chainstate_manager_process_block / import_blocks from the
  same thread deadlocks on non-recursive cs_main.
- The constraint is REAL and UNDOCUMENTED both in this tree's
  header and upstream master's (byte-compared: identical callback
  type docs, no lock-state/reentrancy statement). The standard
  validation-callback pattern (no reentry while cs_main is held)
  applies but is unstated for C API consumers.

### Verdict
DISMISSED: import_blocks is output-on-failure clean; the callback
reentrancy constraint is an upstream-identical documentation gap
(WIP API), not a local defect. Journal-only; the gap is recorded
for the upstream-watch file (the kernel docs' callback section
should state the cs_main-held/no-reentry rule when the API
stabilizes).

### Exact commands
- reads: bitcoinkernel.cpp:405-425/1207-1228,
  validation.cpp:3455-3478, bitcoinkernel.h:360-450;
  upstream master bitcoinkernel.h (raw fetch, same sections)

### Limitations / queue
- NotifyProgress/NotifyBlockTip lock states not individually
  traced (same class).
- KernelValidationInterface callbacks (pow_valid etc.) same inline
  execution pattern (verified by the same wrapper code).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
