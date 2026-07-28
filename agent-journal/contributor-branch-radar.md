# Campaign #65 — contributor-branch-radar

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/contributor-branch-radar. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): l0rinc branch radar — encryption branch supersedes local #38 fix shape; 4 seeds recorded

### Draw
Random draw over the 51-goal eligible pool: raw=1291732092793296637,
index 34 -> #65. Contributor: l0rinc (the fork author; upstream
reviewer/PR author per reviews/ lineage).

### Inventory
`git ls-remote --heads l0rinc`: 856 branches = 225 named + 631
detachedN CI artifacts. Fetched 7 overlap-relevant branches shallowly
(--depth=30, explicit namespaced refspecs; no upstream refs altered).

### Radar entries
1. l0rinc/wallet-encryption-write-failures (tip 4b243f486e,
   2026-07-23, ACTIVE) — upstream-PR-shaped fix for the same wallet
   encryption write-failure class the fork covers locally
   (b8fcf9ed17/3be812ef57/501bd2e263/b16c831eae) and campaign #38 c1
   extended (9894fb8b6c in-memory rollback). The branch RESTRUCTURES:
   RunWithinTxn(GetDatabase(), ...) makes WriteMasterKey + all
   spk_man->Encrypt atomic, and mapMasterKeys registration happens
   only AFTER the durable commit — the mixed-state class our rollback
   patched cannot arise. CONFLICTING ASSUMPTION FLAGGED: 9894fb8b6c is
   correct for the current local shape but is the inferior fix
   (register-then-rollback vs order-after-commit); if the branch lands
   upstream, drop the local family instead of rebasing it. Its
   descriptor-key-erase fix is NOT APPLICABLE locally:
   EraseDescriptorKey does not exist in this tree (no callers).
2. l0rinc/txgraph-equal-feerate-prefix-overflow (2026-07-24) — the
   equal-feerate prefix tracking is already present locally
   (txgraph.cpp:558-559, 671, 1179-1185); the overflow guard belongs
   to the saturation class covered by 3ae78dbd25 (#25 c1 range).
   No action.
3. l0rinc/bloom-zero-elements (2026-07-27) — matches PR 35818,
   reviewed locally yesterday (reviews/2026-07-27-pr-35818-*:
   correct, nFPRate=0 leftover noted). No action.
4. l0rinc/wallet-redact-txids-infolog (2026-01-19, aging) — moves
   txid logging behind -debug=wallet. PRIVACY SEED for campaign #30's
   queue (txids at INFO are not secrets, but the privacy posture is
   consistent with that campaign's class).
5. l0rinc/verify-assumeutxo-hashes (2026-05-22) — consensus-guard
   seed; assumeutxo hash verification. Watch for upstream landing.
6. l0rinc/warn-dirty-coin-coint (2026-02-08) — coins invariant seed
   (61e8c5138d lineage territory).
7. l0rinc/wallet-bound-bdb-overflow-chains (2026-07-20) — likely the
   source PR of local 71cf0ba593 (BDB overflow chain bound);
   consistent, no action.

### Verdict
- Radar established; one actionable conflict-of-assumptions flagged
  (entry 1) and 4 seeds recorded for campaign queues. No unpublished
  work copied; all comparisons by content (recreated local history
  makes hash ancestry useless across the remote boundary too).

### Limitations
- 7 of 225 named branches inspected (overlap-ranked by name);
  optimization branches (IBD-optimizations, block-serialization,
  leveldb-to-rocksdb) unassessed — queued for a perf-oriented pass.
- knots remote not scanned this cycle.

### Exact commands
- `git ls-remote --heads l0rinc`
- `git fetch -q --depth=30 l0rinc refs/heads/l0rinc/<b>:refs/remotes/l0rinc/<b>`
- content checks: git show <branch>:<file> | grep (above)

### Next queue for this campaign
- Perf-branch pass: IBD-optimizations, block-serialization-
  optimizations, tune-leveldb-options, write-chainstate-every-hour —
  overlap with #20/#21/#23/#25 campaign measurements.
- knots remote scan (maintenance backports, adjacent to #66).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.
