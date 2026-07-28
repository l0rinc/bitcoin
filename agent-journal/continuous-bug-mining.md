# Journal: continuous evidence-first bug mining (campaign 0)

Uber-goal rotation. Branch: audit/continuous-bug-mining from
audit/resurrection @ 93e89be11d. This campaign is the rotation's
collector: leads from all other journals land here.

## Cycle 1: TransactionCanBeBumped fuzz target — DELIVERED (da8b249776)

Task carried from #56 (PR 33916 revival). Settled design per the review:
real CTxMemPool (no mock), direct AddToWallet state construction,
independent oracle.

Target (src/wallet/test/fuzz/spend.cpp): builds a wallet bump candidate
from a confirmed funding tx, fuzzes the PreconditionChecks state
dimensions (membership, confirmed/mempool, replaced_by_txid, wallet
descendants, foreign inputs) and asserts the result equals an
independently recomputed expectation. Verified: build_fuzz clean;
-runs=5000 zero failures (268s aarch64).

## Leads map (from accumulated journals)
- P2.1 drain invariant (goal 89): CONFIRMED-STRONGER (campaign 62 R1) — closed.
- 33916 bump fuzz gap (56): DELIVERED this cycle — closed.
- PR 35740 http linger-close (56): open revival candidate, low priority.
- feerate.h:128 CFeeRate deserialize invariant (98): fragility note — no
  production raw deserializer exists; watch for new READWRITE users.
- estimaterawfee NaN-passing comparison (98): unreachable, boundary-proven — closed.
- Interrupted-migration load-time detection (88 W5): future improvement candidate.

## Next queue
(rotate per uber-ledger — next: re-rank from accumulated journals)