# Review: coins DB cursor-vs-resize shared_mutex series (2026-07-25)

Commits: 791ab17e8c (test characterization), f9b96fe758 (cursors hold shared lock; resize exclusive), 2f125947d4 (cursors allowed during compaction). Replaces PR 35744's atomic-count approach with the RAII std::shared_mutex design suggested in that review.

Verdict: 🟢 CORRECT. Deadlock and lifetime analysis all check out; two hardening nits.

## Why it's right

- **Lock ordering**: cursors hold shared lock for their whole lifetime; ResizeCache takes exclusive under cs_main. The only deadlock shape — cursor-holder acquiring cs_main while resize waits for shared locks to drain — is excluded by the documented invariant "while a cursor is alive, its thread must not acquire cs_main nor create a second cursor for the same DB" (coins.h). All current users verified to satisfy it (ComputeUTXOStats takes cs_main only around cursor creation; scantxoutset/dumptxoutset/snapshot paths release or destroy before re-acquiring).
- **Member destruction order**: `m_db_lock` declared before `pcursor` → iterator destroyed before lock release. Correct and commented.
- **Same-thread hazards handled**: recursive shared_lock on one thread is UB (reader-writer deadlock behind a waiting writer) — excluded by the "no second cursor on same thread" rule. The test's TryExclusiveLock helper deliberately probes from a separate thread for the same reason.
- **Commit 3 (compaction)**: CompactFullAsync drops the mutex entirely; instead ResizeCache waits for `m_compaction` (shared_future) before taking the exclusive lock. cs_main keeps the future object stable during the wait (CompactFullAsync also requires cs_main). LevelDB CompactRange is safe with live iterators, so scans and compaction now overlap legitimately; m_db can never be reset mid-compaction or mid-cursor.
- **EstimateSize/GetDBProperty race closed**: ComputeUTXOStats calls EstimateSize without cs_main at scan end — now safe because the still-alive cursor's shared lock prevents any concurrent m_db reset. This was exactly the exploitable window.
- **Characterization test (791ab17e8c)**: correctly documents old behavior (TryExclusiveLock==true, resize returns ready) and the test-access RetainOldDB maneuver (redirect path + retain old m_db) avoids LevelDB's live-iterator assertion while observing it. Post-fix expectations flip correctly (lock contended, resize times out at 100ms, completes after cursor.reset()).

## Nits (non-blocking)

1. 🟡 **Timing window in the test**: after `resize_started` (which only proves cs_main was acquired on the async thread), the 100ms `wait_for` could theoretically false-pass on a very loaded machine if ResizeCache hasn't reached the exclusive lock yet. No observable signal distinguishes "not yet at lock" from "blocked at lock" while the cursor lives, so a short comment acknowledging the bounded-timing design would help; alternatively structure the wait as bounded-retry.
2. 🟡 **Invariants documented but unenforced**: "no cs_main / no second cursor while alive" is comment-only. Cheap hardening: `AssertLockNotHeld(cs_main)` in ~CCoinsViewDBCursor (debug), optionally a thread-local cursor counter asserting in Cursor().
3. **Release-note worthy**: a full muhash gettxoutsetinfo / scantxoutset / dumptxoutset scan now blocks ResizeCache (IBD-exit rebalance, snapshot activation) for the whole scan while cs_main is held — the same availability tradeoff as 35744's approach; operators should know a long scan stalls validation briefly at IBD exit.

Co-author credits (sipa, andrewtoth) match the discussion thread. Ship it.
