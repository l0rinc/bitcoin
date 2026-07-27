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
(empty — filled per area)

## Next queue
(start C1 with a unit-level connect→disconnect→reconnect differential on
regtest chains, then C2 via FlushStateToDisk ordering trace + dbcrash replay)
