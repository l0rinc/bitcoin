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
