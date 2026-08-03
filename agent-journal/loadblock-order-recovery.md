# Loadblock Ordering and Deferred-Parent Recovery

## Seeded from Cycle 321

- Origin goal: 43 (`option-api-lifecycle`), branch
  `uber-cycle-321-option-api-lifecycle-20260802`.
- Seed commit: `41614c589e` selected Goal 43; this follow-up goal was added
  after the child-first process reproduction described below.
- Scope: repeated and within-file `-loadblock` ordering, skipped-child
  retention, restart/retry behavior, CLI/config ordering, observability, and
  the smallest defensible deferred-replay design.

## Prior evidence

`src/init.cpp` preserves repeated `-loadblock` values and
`node::ImportBlocks()` calls `LoadExternalBlockFile(file)` once per path. The
no-map call intentionally has no unknown-parent map. `src/validation.h` says
cross-file out-of-order recovery is a reindex property and explicitly omits it
for `-loadblock`. The original 2012 implementation also processed repeated
values sequentially. The current functional test uses one linearized file.

In scratch directory `/data/my_storage/tmp/cycle321-loadblock-order-a`, a
regtest parent block at height 1 and child at height 2 were stored in separate
files. Release startup with child then parent completed at height 1; startup
with parent then child completed at height 2. The child-first result was
observable through `getblockcount` and `getblockhash`, but authoritative
ordering requirements are not yet established. A clean single-file fixture
containing child then parent also completed at height 1. After shutdown, a
restart with the child file alone recovered height 2 and hash
`6ec25883a09e8c56d3db5df77a3e30671b74c5e2f2f85a49e32adb1c53ad68ba`, proving
that the loss is transient but not automatically deferred.

## Required protocol

Rebase first and search this journal, Goal 43's journal, history, release notes,
bootstrap/linearization code, and existing loadblock tests. Use fresh scratch
datadirs and deterministic block fixtures. Run one-file child-first, split
child-first, parent-first, interleaved branches, duplicate files, missing
parents, malformed records, config-file ordering, and restart/retry controls.
For every result capture block count, headers, best hash, logs, exit status,
and durable state after restart.

Do not treat the existing child-first loss as a production defect without
contract evidence. If order independence is intended, add the smallest
regression and deferred-replay fix with independent before/after proof. If it
is not intended, identify the narrowest help/documentation/test contract that
prevents users from mistaking repeated paths for an order-independent import.
Preserve this seed and add a distinct follow-up only when new evidence changes
the risk map.

## Open queue

- Determine whether a skipped child is still accepted if retried after the
  parent is already in the block index.
- Compare one file containing child then parent with two files in that order.
- Establish whether command-line and config-file duplicate values retain a
  stable order and whether path errors are surfaced consistently.
- Search for historical users or tooling that feed raw block files rather than
  a linearized bootstrap stream.
