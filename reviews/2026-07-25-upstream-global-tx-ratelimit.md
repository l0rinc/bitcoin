# Review: upstream merges 2026-07-25 — global tx rate-limiting series (#34628)

Range: 6b059d9dbd..b33a7fcd7b (13 commits, ajtowns). Verdict: 🟢 nothing critical.

## Anti-starvation analysis (the key question: can one spammy peer burn the global budget?)

No — the budget is spent in **global mining-score order**, not arrival order. `InitiateTxBroadcastToAll` appends every accepted tx to a shared backlog; each take (`InvToSendBucket::TakeForProcessing`) calls `mempool.ExtractBestByMiningScoreWithTopology(backlog, n_to_take)`, so the best feerate/topology txs relay first regardless of source peer. A spammer can only sort ahead by outbidding honest txs on feerate — starvation would require paying for it.

## Verified mechanics

- Dual token buckets (count: 14 tx/s, 420-tx cap; size: 20 kB/s, 12MB init/50MB cap) shared globally; outbound pair boosted 2.5×. Rate math in `util/tokenbucket.h` is straightforward (refill-on-increment, floor-clamped decrement).
- Stale hygiene: `ExtractBestByMiningScoreWithTopology` sorts, dedups, and **drops non-mempool entries** on every take, so evicted/confirmed txs don't linger; backlog capacity-300 is only a shrink target, insertion is bounded by the mempool itself.
- Lock order declared `m_inv_to_send_mutex ACQUIRED_BEFORE(m_mempool.cs)`; no reverse acquisition path (mempool code never takes the inv mutex).
- `count_floor` batching: work only happens once an average INV's worth of tokens accumulated — avoids re-sorting per-tx.

## Perf note (not a blocker)

`ExtractBestByMiningScoreWithTopology` sorts the whole backlog under `mempool.cs` per take — O(B log B) with B up to mempool size during a storm (100k+ entries = tens of ms of mempool.cs stall per take). Bounded by batching and the mempool's own size limit; watch if storms show msghand stalls.
