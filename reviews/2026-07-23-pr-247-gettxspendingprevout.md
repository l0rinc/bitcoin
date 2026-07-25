# Review: fork PR 247 — rpc: avoid quadratic gettxspendingprevout (2026-07-23)

Verdict: 🟢 CORRECT — merge it. Fixes sweep finding #3 (d23641564).

## What it fixes
- `std::erase_if` with a predicate replaces the erase-in-loop: mempool pass is one O(n) compaction under `mempool.cs` instead of O(n^2) memmove. The DoS scenario (~8.5MB authenticated request stalling validation for minutes) collapses to one `GetConflictTx` per entry (inherent minimum).
- Results stored by `result_index` → response follows request order (secondary nit from the sweep fixed too).

## Correctness audit
- No unfilled result slots leak: `mempool_only` resolves everything in the pass; otherwise the index loop fills the remainder; the throw path fires before assembly exactly when unresolved entries exist and the index is unavailable/unsynced — identical to old early-return/throw structure.
- `block_hash` lifetime safe (used immediately in-loop); duplicate outpoints get independent slots; empty request → empty result; `EXCLUSIVE_LOCKS_REQUIRED(mempool.cs)` annotation correct.
- Edge paths preserved: unsynced-index error, null-spender "unspent" indicator objects, VARR move assembly via push_backV + make_move_iterator.

## Nits (non-blocking)
- The 8-line functional test covers mixed ordering; nothing pins the quadratic regression itself (note why timing tests are flaky in CI).
- `results(output_params.size())` preallocates N default UniValues on the invariant "every slot filled on all non-throwing paths" — worth a comment.
- 4 commits for 51-line net change could be 2, but readable as-is.

Branch carries unrelated upstream script_tests.json vector commits (#35664 merge) — no conflicts. Ready to upstream.
