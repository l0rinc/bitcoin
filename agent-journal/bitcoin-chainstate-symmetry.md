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

### C2 (flush/restart durability): one real defect, already fixed by own open PR 35714; rest DISMISSED

Flush ordering trace (FlushStateToDisk, validation.cpp:2807-2851):
blk/rev flush → WriteBlockIndexDB → prune unlink → coins flush.

1. Coins flush two-phase commit (txdb.cpp:123-190): first batch erases
   DB_BEST_BLOCK and writes DB_HEAD_BLOCKS=[new_tip, old_tip] transition
   marker; partial coin batches follow; final batch erases HEAD_BLOCKS and
   writes BEST_BLOCK. Every batch is one atomic LevelDB WriteBatch. Crash
   mid-flush → DB has HEAD_BLOCKS but no BEST_BLOCK → restart replays from
   old_tip with a loud -reindex-chainstate error on inconsistency
   (130-141). Correct under any-instruction crash. Crash-injection oracle
   exists: simulate_crash_ratio (171-177) exercised by feature_dbcrash.py.
   DISMISSED.
2. Crash windows in the ordering: block files or block index ahead of the
   coins flush are harmless — unconnected candidate blocks on disk are
   re-requested/reconnected on restart; chainstate never references
   unflushed block data because coins flush is LAST. DISMISSED.
3. CONFIRMED-ALREADY-FIXED (own PR): block-file flush failure was warn-only
   (validation.cpp:2819-2823 TODO) — FlushStateToDisk continued to
   WriteBlockIndexDB and the coins flush and advanced m_last_flushed_block
   after a failed block-file flush, potentially marking undurable state as
   flushed. Fixed by own open upstream PR 35714 "validation: stop writes
   after flush failure" (head e1a337ee96, + characterization test
   0f04fbee2f): return the error before the metadata/coins writes. Open,
   unmerged at check time; not new work — tracked by the watch crons.

### C3 (reorg accounting): DISMISSED — complete-membership invariants asserted over the whole block index

1. setBlockIndexCandidates has BIDIRECTIONAL membership invariants in
   CheckBlockIndex (validation.cpp:5508-5563): every block that has more work
   than the tip, all parents processed, data not missing, and not invalid
   MUST be a candidate (5554); every block sorting worse than the tip or with
   an unprocessed ancestor MUST NOT be (5562). A stale entry left by an
   unequal-height reorg, or an entry wrongly dropped, both trip asserts.
2. m_best_header bound asserted globally (5506): no valid block may have
   more work than m_best_header.
3. All invalidation/reorg paths erase candidates explicitly: InvalidBlockFound
   (2030), InvalidChainFound/FindMostWorkChain (3229-3247), InvalidateBlock
   (3650/3734 highpow cache), ResetBlockFailureFlags (3826/3963), and
   PruneBlockIndexCandidates. m_blocks_unlinked tracks data-missing former
   candidates (5561-5562).
4. Oracles: CheckBlockIndex runs on every startup (LoadBlockIndex) and
   periodically under test debug builds; block_index_tree fuzz target;
   functional reorg coverage (feature_block, p2p_invalid_block,
   feature_assumeutxo snapshot candidates, invalidate/reconsider cycles).
5. Impact framing: candidates only steer download/activation choice, never
   validity — even a hypothetical stale entry could stall sync, not corrupt
   state; and it would assert on restart anyway.

## Next queue
(start C1 with a unit-level connect→disconnect→reconnect differential on
regtest chains, then C2 via FlushStateToDisk ordering trace + dbcrash replay)
