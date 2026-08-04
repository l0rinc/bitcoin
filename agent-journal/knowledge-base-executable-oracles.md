
## Cycle 337 (2026-08-04) — r155 #114: two new oracle inputs assessed

Reopen inputs since r150: (a) cycle-333 SipHash-1-3-UJ doc-only
reference (preserved: agent-journal/artifacts/siphash13uj-reference/,
replay green); (b) cycle-336 delay-queue extraction contract.

(b) assessed for oracle conversion: NOT needed — upstream's fuzz
tx_pool target already asserts the contract executably
(src/test/fuzz/tx_pool.cpp:608-630): drain accounting
(remaining <= before - expected), expected-count-or-empty, all
remaining wtxids in-mempool (dead entries dropped), dedup, and
composes with TrimToSize eviction + Expire immediately after —
the exact flood+eviction composition from cycle 336's concern.
Ran clean in the cycle-332/333 smokes (2000 runs).

Verdict: knowledge captured, no new oracle required; #114 returns
to its prior state (reopen-FAIL absent NEW inputs). NP 3/20.
