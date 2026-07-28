# Campaign #2 — assertion-invariant-audit

Base: 12ccbdc860 (journal commit for #21 cycle-2 on
audit/rebuild-recovery-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/assertion-invariant. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): fork-added production Assumes — all construction-tautologies; cache-overflow fix verified correct

### Draw
Random draw over the 53-goal pool (33 pending + 20 CYCLE-1; #21
excluded as just-cycled): raw=9227947000855944028, seed masked to 63
bits (4574964001168220), index 0 -> #2.

### Method / audit set
Enumerated assertions added by fork commits (git log
--author=pap.lorinc, +-lines matching Assume/Assert/CHECK_NONFATAL,
test-dir lines excluded) and selected the two PRODUCTION sets for
deep reachability analysis (the rest are fuzz-target or unit-test
assertions, out of this campaign's release-path scope):
A. 0231239131 (interfaces mining lookup arity): Assume(results.size()
   == txids.size()) x2, wtxid x2 in src/node/interfaces.cpp
   getTransactionsByTxID/WitnessID.
B. 21a97c2721 (node: avoid cache allocation percentage overflow): 5
   Assumes at the end of CalculateCacheSizes (src/node/caches.cpp).

### Claim A: mining IPC positional contract — SAFE (fixed-by-construction)
Before the commit, both methods returned {} whenever NodeContext had
no mempool, violating the one-nullable-per-requested-id positional
contract (mining IPC clients align ids to results). The fix constructs
results(n) up front and fills per index; the Assumes restate the
arity. Falsification attempt: results is size-fixed at construction
(std::vector<CTransactionRef> results(txids.size())); no path resizes
it; the Assume is unfalsifiable (the good kind — it fires only if a
future edit breaks the arity). Reachability of the surrounding
behavior change: kernel/mining clients without a mempool now get N
nulls instead of 0 — contract-restoring, covered by the commit's
miner_tests.cpp additions (25 lines).

### Claim B: cache-budget Assumes — SAFE; the fixed overflow verified
The pre-commit code computed index budgets as total_cache * 10 / 100
and * 5 / 100 (multiply-first). With an extreme -dbcache the
intermediate multiply can wrap (total_cache * 10 overflows uint64
once total_cache > ~1.8e18 B; guarded upstream only by the
CalculateDbCacheBytes cap). The commit rewrites to division-first
budgets (tx_index_budget = total/10, secondary = total/20),
eliminating the wrap entirely, and adds 5 Assumes. Each audited:
 1. tx_index <= tx_index_budget — std::min construction. Tautology.
 2. txospender_index <= secondary_index_budget — same.
 3. filter_cache <= secondary_index_budget — (max/n)*n <= max <=
    budget (integer division only rounds down). Holds.
 4. total_cache == initial - filter - tx_index - txospender_index —
    exact arithmetic identity of the three subtractions; no underflow
    since the parts sum to <= 20% of initial. Holds.
 5. kernel_sizes.block_tree_db + coins_db + coins == total_cache —
    kernel::CacheSizes (src/kernel/caches.h:29-36) splits by tracked
    subtraction (block_tree_db=min(total/8,MAX); coins_db=
    min(rest/2,MAX); coins=rest) — lossless by construction. Holds.
Config-reachability: all five are unfalsifiable by any -dbcache /
index-flag combination; they are edit-guards, not input validation.

### Class check (campaign's key question)
No assertion in the audited set serves as untrusted-input validation:
A's inputs are IPC request vectors handled data-agnostically; B's
inputs are startup config reduced by std::min/divide-first before any
Assume. Fuzz-target asserts (fb6810ac8c chainstate_lifecycle,
0a2deeea1d chainstate delete) and unit-test asserts (ae2bf49412,
77c5a11526, a6e490ab5b, 4c106d58c9) are test-oracle scope, noted but
not in the release-path audit set.

### Verdict
DISMISSED: no invalid assumption, no missing validation, no misleading
contract in the fork's production Assume sets A/B. The one real defect
in the area (multiply-first overflow) was already fixed by the fork
with the correct division-first transformation; its Assumes are
unfalsifiable-by-construction edit-guards.

### Exact commands
- git log --format=%h --author='pap.lorinc' -- src/ (40 commits);
  per-commit -U0 +-line grep for Assume/Assert/CHECK_NONFATAL
- git show 0231239131 / 21a97c2721 (full diffs + commit rationales)
- src/kernel/caches.h:21-37 (kernel split losslessness)

### Limitations / queue for cycle 2
- Fuzz-only Assumes (G_ABORT_ON_FAILED_ASSUME blocks in txgraph,
  signet, net_processing) not audited this cycle — those are
  oracle-scope; a falsification pass over the txgraph saturation
  family (3ae78dbd25 lineage) is queued.
- Upstream-side assertions (assert/Assert in production pre-fork code)
  untouched — separate large cell, queued.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
