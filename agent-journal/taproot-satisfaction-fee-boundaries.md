# Learned Goal 114: Taproot satisfaction and wallet fee-bound differential

## Seed

- Learned in cycle 314 from the live `TRDescriptor` satisfaction-size FIXME.
- Source cycle: `uber-cycle-314-todo-deferred-work-20260802`.
- Initial selection: goal 96, `todo-deferred-work`.
- Seed finding: known `tr()` script paths were estimated as keypath-only by `MaxSatisfactionWeight()` and `MaxSatisfactionElems()`, understating wallet input weight and fee.

## Initial evidence

`src/script/sign.cpp` attempts keypath signing and then appends the selected leaf script and control block to a script-path witness. `TaprootBuilder::GetSpendData()` defines a control block as 33 bytes plus 32 bytes per Merkle depth. `src/wallet/spend.cpp` consumes the descriptor byte bound and separately adds the witness stack-element count.

For `tr(NUMS_H,pk(NUMS_H))`, the leaf satisfaction is 66 serialized bytes, the leaf script is 34 bytes, and the depth-zero control block is 33 bytes. The corrected script-path bound is 135 bytes excluding the outer witness count, with three witness elements and a 136-byte serialized witness. The old keypath-only estimate produced 58 vbytes for the input; the corrected bound produces 75 vbytes.

The existing `test/functional/wallet_taproot.py` contains a historical fee-rate workaround for script-path fee estimation. The cycle-314 fix removes only its stale explanation and adds a descriptor regression that expands the tree and checks the script/control-block sizes before asserting the bound.

## Continuing queue

1. Compare `tr()` descriptors containing `pk`, `multi_a`, nested Miniscript, and mixed-depth leaves against independently serialized signed witnesses.
2. Check `use_max_sig=true/false`, compact-size thresholds, maximum script sizes, and control-block depth boundaries without integer narrowing or overflow.
3. Trace multipath descriptor expansion, duplicate leaves, unavailable keys, external signers, PSBT fee calculation, and wallet input/vsize callers.
4. Determine whether `rawtr()`'s keypath-only fallback is an intentional unknown-tree contract or an unsafe underestimation at any caller.
5. Search history, release branches, and functional workarounds for incomplete follow-ups; replay every confirmed boundary on current HEAD.

## Required evidence

Use an independent formula oracle and an independent signing or wallet/PSBT oracle. Preserve minimized descriptor, witness, and transaction fixtures. Any fix must prove the estimate is never below the serialized witness/input size, keep unknown cases unknown, and pass focused plus relevant descriptor, miniscript, wallet, and functional tests.
