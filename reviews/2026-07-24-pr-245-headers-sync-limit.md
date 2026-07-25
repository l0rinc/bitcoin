# Review: fork PR 245 — net: limit low-work headers syncs in IBD (2026-07-24)

Verdict: 🟢 correct; ship with one test addition (since added: 0f8c2ba1c6 "test: cover headers presync limit").

## Correctness (full lifecycle verified)
- Cap exact in practice: check + state creation + stats insertion all run on the single msghand thread (ProcessHeadersMessage) — no race window; m_headers_presync_mutex only serves getpeerinfo readers.
- Lifecycle can't leak: stats inserted on first continuation (net_processing.cpp:2766), erased on every exit — sync failure/reset (:2754, :3023) and disconnect (:1756 FinalizeNode). No permanent cap-shrink.
- Outbounds exempt from rejection but counted → real bound is 8 inbound + 8 outbound presyncs (~16 × ~100KB+ ≈ 2MB worst during IBD); if all 8 outbounds sync first, inbound cap is 0 — intended "keep room for outbounds" semantics; test validates (8 inbound + 1 outbound accepted, 2 more rejected).
- No lock-order issue: IsInitialBlockDownload() takes and releases cs_main before WITH_LOCK(m_headers_presync_mutex) — never held simultaneously at this site.
- Soft failure: dropping headers with return true (no ban/disconnect/state change) — peer retries post-IBD. Correct DoS-response shape; blocks state creation only, never kills in-flight syncs (killing would be amplifiable churn).

## Coverage gaps (suggested)
1. 🟡 Slot-release test: new inbound peer can presync after an existing presync completes/fails (guards the stats-leak regression class). ✅ added via 0f8c2ba1c6 (title).
2. 🟡 Post-IBD test: cap must be inert after IBD exit.
3. Optional: exact boundary (stats==7 accepts, ==8 rejects).

## Simplifications
None — 9 lines. Doc nits: worth a comment that stats include outbound peers (effective inbound cap 0 when all outbounds sync); MAX_CONCURRENT_HEADERS_SYNCS coupling to MAX_OUTBOUND_FULL_RELAY_CONNECTIONS is loose-but-documented.
