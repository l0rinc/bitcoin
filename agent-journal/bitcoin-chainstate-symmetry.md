# Journal: chainstate, reorg, prune, and index crash-symmetry audit (goal 86)

Campaign: severity-first rotation (cycle 4). Persisted-state corruption and
unrecoverable node states rank first.
Base: audit/resurrection @ e8f234d6dd. Build: build-before (Release gcc).
Tools: regtest + scratch datadirs; -dbcrashratio + feature_dbcrash.py for
LevelDB crash injection; unit fixtures for connect/disconnect symmetry.

## Trust boundary
Crash (SIGKILL/power-cut equivalent) at ANY instruction; OS write reordering
within documented fsync/fdatasync semantics. NOT in scope: silent disk
bit-flips, adversarial filesystem, cosmic rays (hardware campaign).

## Scope ledger (the queue — verdict per area)

| # | area | hypothesis seeds | verdict |
|---|------|------------------|---------|
| C1 | connect/disconnect symmetry | DisconnectBlock not exact inverse of ConnectBlock (coins cache, undo data, flags); replay divergence | open |
| C2 | flush/restart durability | locator/best-block advancement past durable chainstate; block files written but chainstate not flushed; HEAD_BLOCKS/BEST_BLOCK ordering | open |
| C3 | reorg accounting | unequal-height reorg leaves stale setBlockIndexCandidates / dirty flags / m_best_header | open |
| C4 | prune vs read races | block/undo file deleted between HAVE_DATA check and open; ReadBlockData on pruned file | open |
| C5 | block/undo file lifecycle | file position allocation vs crash (pos allocated, block not written; rev without blk); xor/obfuscation state | open |
| C6 | snapshot/background chainstate | ValidatedSnapshotCleanup file moves vs crash mid-move; ResetChainstates reinit; assumeutxo reload edge states | open |
| C7 | BaseIndex durability/rewind | index locator ahead of chainstate flush (the base.cpp:96-99 guard); Rewind vs crash mid-reorg; Commit ordering | open |

## Verdicts

### C1 (connect/disconnect symmetry): DISMISSED — inverse by construction, oracles at 4 levels

1. BY CONSTRUCTION: DisconnectBlock (validation.cpp:2215-2284) spends every
   output the block created and restores every spent input from undo data,
   verifying EXACT equality per output (value, script, height, coinbase flag,
   2249-2259); any mismatch flips fClean — a non-inverse disconnect is loud,
   not silent. BIP30 duplicate-coinbase exceptions are hash-pinned (2237-2238).
2. CACHE-LEVEL randomized inverse simulation (coins_tests.cpp:620-698):
   random tx connect/ApplyTxInUndo-undo with full-cache equality verification
   every ~1000 iterations against a reference map, across cache-stack flushes
   and uncaches.
3. BLOCK-INDEX metadata invariants asserted by CheckBlockIndex
   (validation.cpp:5421-5506): VALID_TRANSACTIONS ⟺ nTx>0 (pruning-
   independent), HAVE_UNDO ⟹ HAVE_DATA, failed-flag propagation. nChainTx
   persisting after disconnect is by design (LoadBlockIndex invariant 5475).
4. FUNCTIONAL round-trip oracle: feature_coinstatsindex.py performs
   invalidateblock/reconsiderblock cycles and compares the incrementally
   maintained muhash of the UTXO set (lines 266-291) — a disconnect that
   failed to restore the exact UTXO set would corrupt the muhash and fail
   the comparison.

No asymmetry candidate survives; the connect/disconnect pair is one of the
most heavily oracled paths in the tree.

## Next queue
(start C1 with a unit-level connect→disconnect→reconnect differential on
regtest chains, then C2 via FlushStateToDisk ordering trace + dbcrash replay)
