# Persisted index key decode and false-negative audit

This seed was created from Cycle 325. `TxoSpenderIndex::FindSpender()` embedded `GetKey()` in its range-loop condition, so a truncated persisted key became a successful empty lookup. The fixed txospender path and regression are recorded in `exhaustive-algebraic.md`; this journal targets distinct key/value readers.

Initial queue:

1. Inventory serialized range readers in block-filter, coin-statistics, transaction, spender, RPC, and recovery indexes.
2. Separate clean exhaustion, wrong-prefix termination, malformed key/value, iterator status, and missing-record contracts.
3. Inject minimized malformed entries at adjacent boundaries, prove before/after behavior with mutation-sensitive tests, and preserve valid ordering and restart semantics.

Do not reopen the repaired txospender loop or closed Goal 18 GCS, compact-target, batch-parent, or iterator cells without a new key type, recovery mode, or recurrence.
