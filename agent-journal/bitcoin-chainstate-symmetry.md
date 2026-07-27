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

### C4 (prune vs read races): DISMISSED — deletion under cs_main, reads graceful or buffer-protected

### C5 (block/undo file lifecycle): DISMISSED — allocation/undo/xor all idempotent or loud under crash

Resumed from goal-86 pause (C1-C4 locked earlier).
1. Position allocation vs crash: FindNextBlockPos allocations live in
   m_blockfile_info memory until WriteBlockIndexDB persists them. Crash
   loses unflushed allocations → stale tail bytes at file end are
   OVERWRITTEN when the same positions are re-allocated from DB-loaded
   sizes on restart. Consistent by construction.
2. Undo lifecycle (WriteBlockUndo, blockstorage.cpp:1022-1089): undo bytes
   written and file fclose-checked BEFORE nUndoPos/HAVE_UNDO is set in
   m_dirty_blockindex (1082-1085); HAVE_UNDO persists only with the block
   index DB. Crash between → index lacks HAVE_UNDO → undo rewritten at the
   same re-allocated position (idempotent). Crash after index persisted
   but before coins flush → C2's two-phase replay reconnects and reuses
   existing undo (GetUndoPos non-null, skip at 1029). Consistent.
3. blk/rev pairing: undo goes to the rev file matching block.nFile by
   design.
4. xor.dat (InitBlocksdirXorKey, 1240-1277): random key only on first
   run; existing file takes priority; exclusive "wbx" create; fclose
   checked. Torn xor.dat on restart → deserialization throws → LOUD
   startup failure, never silent wrong-key reads. Disable-after-random-key
   rejected (1268-1275).
5. Known area issues (PR 33324 resumable reobfuscation,
   DirectoryCommit+blocksxor=0) already fixed/verified per standing
   cron state.

### C6 (snapshot/background chainstate transitions): DISMISSED — atomic renames, loud recovery, startup-only by design

ValidatedSnapshotCleanup (validation.cpp:6520-6585) sequence:
ResetChainstates (DBs closed) → rename(main → main_todelete) →
rename(snapshot → main) → best-effort delete of _todelete.
1. Each rename is atomic (POSIX same-fs): the validated snapshot contents
   are never in a half-moved state. Crash before/between/after the two
   renames leaves BOTH directories intact — worst case is names in an
   intermediate arrangement, never torn data.
2. The cleanup is deliberately confined to startup (node/chainstate.cpp:
   200-203 comment: "too risky to do in the middle of normal runtime").
   The validated-snapshot marker is persisted in the snapshot chainstate's
   own DBs (m_assumeutxo == VALIDATED), so the completion state survives
   the crash and MaybeValidateSnapshot re-detects it on restart
   (node/chainstate.cpp:204-235), rerunning the cleanup.
3. Ugliest case — crash between rename #1 and #2: main dir missing,
   snapshot intact. On restart a fresh empty main chainstate may be
   created, then the rerun's rename #1 hits the pre-existing _todelete →
   ENOTEMPTY → rename_failed_abort → FatalError with an explicit message
   (6550-6553). LOUD halt, manual cleanup (remove the stale dir, restart),
   zero corruption, zero data loss (validated snapshot intact).
4. No fsync of the directory entries between renames: a lost rename #2
   reverts to the pre-rename state, which is exactly the handled case (3).
5. CAVEAT (not a defect): there is no "cleanup in progress" marker file;
   the crash-between-renames recovery needs one manual step. Upstream's
   startup-only design accepts this; documented here for future cycles.

### C7 (BaseIndex durability/rewind): DISMISSED — locator guard, idempotent rewind, assert-backed replay

1. Locator durability guard: Commit skips when the index best block is
   ahead of the chainstate's last flushed block (base.h:96-99 +
   base.cpp:344-349 "the committed index state must never be ahead of the
   flushed chainstate"); ChainStateFlushed commits only for
   chainstate-durable blocks (ancestor check, base.cpp:443). Crash →
   locator ≤ flushed tip → consistent replay.
2. Commit failure is safe by design (base.cpp:451-454): a missed commit
   only reprocesses from an older locator.
3. Crash MID-REWIND (the critical case): CustomRemove's only durable write
   is the height→hash index copy batch (coinstatsindex.cpp:218-227) —
   idempotent (same key, same values; the height entry still exists on
   replay). The muhash/counter rollback in RevertBlock (326-403) is
   MEMORY-ONLY until the guarded CustomCommit — so a crash leaves no
   durable index-state change to double-apply.
4. Replay verification: RevertBlock recomputes the rolled-back muhash and
   ASSERTs it equals the stored parent muhash (386) — any replay
   inconsistency is a LOUD failure, never silent. The hash-index fallback
   (341-345) exists precisely so repeated rewinds find the parent after
   height-entry overwrite.
5. Restart stress coverage exists: index/txindex/txospenderindex
   _reinit_reader_race tests (f344e8102c series).

## Goal-86 cycle complete

All 7 ledger areas locked: C1, C3, C4, C5, C7 DISMISSED; C2 one real defect
already fixed by own open PR 35714; C6 DISMISSED with startup-recovery
caveat. No new defects. Rotation: uber-ledger marks #86 DONE, next #88.

1. Deletion protocol: FindFilesToPrune (blockstorage.cpp:363-442) runs under
   cs_main, calls PruneOneBlockFile (clears BLOCK_HAVE_DATA/positions for
   contained blocks) in the SAME critical section in which
   FlushStateToDisk later calls UnlinkPrunedFiles — no cs_main-holding
   reader can observe HAVE_DATA on a file that is already unlinked.
2. Height guard: only files entirely within [min_block_to_prune,
   last_block_can_prune] are pruned; last_block_can_prune is bounded by
   MIN_BLOCKS_TO_KEEP (288) below the tip and by every registered prune lock
   (m_prune_locks, e.g. dumptxoutset-rollback, rpc/blockchain.cpp:3048).
3. Non-cs_main read paths fail gracefully: ProcessGetData block serving
   (net_processing.cpp:2484-2490) logs "Block was pruned before it could be
   read" / disk error and disconnects — no assert, no corruption.
4. The two assert-on-read-failure paths are protected by the 288-block
   buffer, not by luck: GETBLOCKTXN (4413-4416) only reads blocks within
   MAX_BLOCKTXN_DEPTH(10) of the tip (gated at 4406-4408); CMPCTBLOCK
   announcement (6082-6083) reads our own tip block. Both are unreachable
   by pruning. Asserts unfalsifiable.
5. POSIX semantics help the remaining window: a reader that already opened
   the file before unlink keeps a valid fd; only pre-open unlink is
   possible, which is the graceful path in (3).

## Next queue
(start C1 with a unit-level connect→disconnect→reconnect differential on
regtest chains, then C2 via FlushStateToDisk ordering trace + dbcrash replay)
