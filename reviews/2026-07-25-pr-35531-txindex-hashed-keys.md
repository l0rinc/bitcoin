# Review: PR 35531 — txindex: hash keys and pack block positions (andrewtoth), re-verified 2026-07-25

Verdict: 🟢 no merge-blocker. Full detail in chat; key points below.

## Design
12-byte DB key (tag + 5-byte salted SipHash prefix + packed BlockTxPosition[3-byte BE block_seq + 3-byte BE tx_offset]) replaces the 32-byte txid key; position moves from value into key (empty values). ~66GB → ~26GB.

## Verified
- Collision handling: FindTx seeks the prefix, collects all sharing positions, walks newest-block-first, resolves block_seq→block_hash via bidirectional seq/hash maps, requires BLOCK_HAVE_DATA, verifies candidate txid. Cost per collision = one extra read+deserialize+hash. ~164 natural collision pairs expected at 600M txs/2^40; grinding 2^40 infeasible.
- Stale blocks: newest-first + active-chain preference, newest-stale fallback, pruned skipped.
- 3-byte limits (16.7M) safe vs max block ~4MB; offset accounting matches on-disk layout.
- Legacy rows readable; downgrade re-syncs via new locator; new DBs skip legacy lookups + bloom (coherent: hashed lookups are iterator Seeks).
- Per-candidate LOCK(cs_main) fine (tiny collision counts).

## Fork relationship
l0rinc/txindex_optimization has the IDENTICAL key format — 35531 is the upstreamed version of the same design (d08d95cef9 ≡ be3bf13c7a). Cherry-pick anything ahead in the fork branch; otherwise retire it.

## Coverage gap (carried from sweep)
Upgrade→downgrade roundtrip functional test would pin the migration both ways.
