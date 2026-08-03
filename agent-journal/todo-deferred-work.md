# Cycle 314: TODO, FIXME, stub, and deferred-work challenge audit

## Scope and protocol

- Branch: `uber-cycle-314-todo-deferred-work-20260802`
- Selection: catalog goal 96, `todo-deferred-work`
- Selection commit: `bad67469285bd69415fd9baf68a60902a397b19b`
- Base/state-close: `52f7c73943d6b0b957db00f33adf2718ddb012ab`
- Catalog SHA at selection: `7012173cab79a6d83c1b465e41c6015bbe42c408dbee283b7191b8771404b2c5`
- Scope: project-owned TODO/FIXME/XXX markers, disabled tests, stubs, expected failures, and deferred compatibility or error-path work. Every marker must be tied to current code, history, a concrete risk, or an exact blocker.

The inventory excludes vendored LevelDB, bundled libmultiprocess, dependency build trees, generated locale/XPM content, and the already-covered broad dead/stale-code cells from Goal 29. Previously reviewed markers are linked rather than re-opened unless new evidence changes their reachability or contract.

## Initial queue

1. `src/script/descriptor.cpp` Taproot `TRDescriptor` and `RawTRDescriptor` satisfaction-size assumptions.
2. `src/rpc/node.cpp` mocktime synchronization TODO and its lock/lifecycle contract.
3. `src/prevector.h` `new_handler`/reallocation FIXME and allocation-failure behavior.
4. `src/test/coins_tests.cpp` untested failure cases and rollback oracle quality.
5. `src/test/txvalidationcache_tests.cpp` remaining script-flag coverage.
6. `src/httpserver.cpp` deferred error-formatting contract.

Initial inventory: `/data/my_storage/tmp/cycle314-todo-inventory.txt`. Excluded from this queue because Goal 29 already established their current status: compact-block reconstruction failure handling, orphan-resolution policy, transaction-reconciliation scaffolding, minisketch skips, CoinStats compatibility cleanup, and the former `ScriptIsChange` comment.

## Current hypothesis

The Taproot descriptors return a key-path-only maximum satisfaction weight and one witness element even when a script path is present. `src/wallet/spend.cpp` consumes those values for maximum signed input sizing, so a descriptor with a script-path spend may understate required transaction weight and fee. Confirm the descriptor grammar, the actual signing/satisfaction path, the wallet call graph, and the failure or compatibility contract before changing code.

## Evidence ledger

| ID | Marker | Status | Evidence / next action |
| --- | --- | --- | --- |
| C314-1 | Taproot satisfaction-size FIXME | confirmed and fixed | `TRDescriptor` reported keypath-only values even when `tr()` contained a script path. `MaxInputWeight()` consumes the descriptor byte bound and element count directly. A `tr(NUMS_H,pk(NUMS_H))` leaf has a 66-byte maximum satisfaction, 34-byte script, 33-byte depth-zero control block, and three witness elements. The old estimate was 66 bytes/one element; the corrected script-path bound is 135 bytes/three elements, making the full witness 136 bytes and the input estimate 75 vbytes instead of 58. `RawTRDescriptor` remains keypath-only because its unknown tree cannot provide a safe script-path bound. |

## Confirmed finding: Taproot script-path fee underestimation

The TODO was a live correctness defect, not stale documentation. `src/script/sign.cpp` first attempts keypath signing and then appends the leaf script and smallest control block to every successful script-path satisfaction. `TaprootBuilder::GetSpendData()` defines the control block as 33 bytes plus 32 bytes per Merkle depth. `src/wallet/spend.cpp` adds `MaxSatisfactionWeight()` to the input weight and separately serializes `MaxSatisfactionElems()` as the witness stack count.

The descriptor bound now takes the maximum of the keypath bound and every known leaf's serialized satisfaction plus compact-size-prefixed leaf script and control block. It returns unknown if a child bound is unknown. The element bound adds the leaf script and control block to the child's stack. This matches the signing path and does not double-count the outer witness count. The independent `test/functional/wallet_taproot.py` fee-rate workaround, present since the original Taproot wallet tests, supplied historical evidence that script-path fee estimation was known to be incomplete; its stale explanation was removed after the fix.

The descriptor regression expands the tree and checks the 34-byte script and 33-byte depth-zero control block before asserting 135 bytes for both signature policies and three elements. The rebuilt scratch binary passed `descriptor_tests/taproot_script_path_satisfaction_size` and all 14 `descriptor_tests` cases. The exact commands were:

    /data/my_storage/tmp/cycle314-test_bitcoin --run_test=descriptor_tests/taproot_script_path_satisfaction_size --log_level=test_suite
    /data/my_storage/tmp/cycle314-test_bitcoin --run_test=descriptor_tests --log_level=test_suite

Both ended with `*** No errors detected`.

## Handoff

Finding commit is pending after the final diff check. Next queue: verify nested Miniscript and `multi_a` bounds, then inspect `src/rpc/node.cpp` mocktime lifecycle and `src/prevector.h` allocation-failure FIXME. Add a dedicated reusable goal for differential Taproot satisfaction and wallet fee-bound testing before the next random selection.
