# Journal: error-path partial-state mutation audit (campaign 27)

Uber-goal rotation. Branch: audit/error-path-state from audit/resurrection
@ 0db546a022. Method: enumerate failure edges in mutating functions;
check unchanged/rolled-back/invalidated contracts with evidence.

## Cycle 1 verdicts

### BaseIndex::ProcessBlock / CustomAppend failure: DISMISSED — memory mutation dies loud, DB untouched

CoinStatsIndex::CustomAppend increments m_total_subsidy (coinstatsindex.cpp:111)
BEFORE the prev-hash check that can return false (116-120) — a genuine
partial in-memory mutation. But the only caller (ProcessBlock,
base.cpp:397-402) converts any false into FatalErrorf → process abort.
The mutated memory dies with the process; the DB was never written
(CustomAppend is memory-only accumulation; the guarded CustomCommit is
the only DB writer and isn't reached). Restart replays from the last
guarded commit. TxIndex: per-block single atomic CDBBatch, best-block
advanced strictly after success, replay idempotent via the Exists guard.
No durable partial state possible.

### CCoinsViewCache::SpendCoin failure: DISMISSED — mechanically asserted no-op

coins.cpp SpendCoin: the assume_failed_spend_noop lambda captures cache
size/usage/dirty count and Assume-verifies ZERO mutation on both failure
returns (cache miss, already-spent). The contract is enforced in-code —
fires in fuzz/debug builds, not just documented.

## Campaign 27 cycle complete

The two failure-contract shapes audited are exemplary: fail-loud abort
with memory-only mutation (indexes), and mechanically asserted
no-mutation (coins cache). No partial-state defects. Rotation:
uber-ledger marks #27 DONE, next #7.