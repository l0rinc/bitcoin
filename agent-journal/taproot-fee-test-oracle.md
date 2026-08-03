# Taproot fee-estimation test-oracle and workaround audit

## Seed from Cycle 323

Cycle 323 found that `2ac99aac74` repaired Taproot script-path satisfaction
size bounds and removed the explanatory comments in
`test/functional/wallet_taproot.py`, but left the two `fee_rate=200` overrides
that those comments described as a workaround. Blame traced both values to
the old script-path estimation defect. Removing the overrides let the full
current Taproot functional test exercise automatic estimation successfully.

## Continuing protocol

Start from a fresh gate and a dedicated branch. Search this journal, Goal 114,
the descriptor and wallet fee journals, history, release branches, and review
precedent before selecting a new test surface. Inventory every explicit fee
rate, safety margin, expected vsize, skipped script path, and default-estimator
override. Distinguish deliberate compatibility tests for old releases from
current tests that accidentally retain a workaround.

For each candidate, construct a deterministic descriptor and transaction that
forces the intended key-path or script-path satisfaction. Compare automatic
and explicit-rate fee/vsize results with an independent serialized-witness
oracle. A successful mempool acceptance is not enough: require an assertion
that catches underestimation. Use NUMS internal keys, nested leaves, `multi_a`,
unknown trees, maximum-signature policy, and external or unavailable-key PSBT
paths where their contracts apply. Preserve minimized fixtures and exact RPC
traces. Change tests only when the old assertion or override is demonstrably
stale, and keep the source fix separate from any oracle repair.
