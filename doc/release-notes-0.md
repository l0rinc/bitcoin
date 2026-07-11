New settings
------------

- A new experimental `-pruneassumevalid` option allows pruned nodes to reduce
  bandwidth and block/undo file writes during IBD before the configured
  assumevalid block. When enabled with `-prune` and a non-zero assumevalid hash,
  the assumevalid block and its historical ancestors are requested without
  witness data, connected without writing block or undo data to disk, and then
  dropped from memory. This mode is disabled by default and is incompatible with
  assumeutxo snapshots. It skips witness download, witness validation, witness
  commitment checks, and witness availability checks for the assumevalid block
  and its historical ancestors, and reduces restart and reorg resilience while
  syncing that region. It is also incompatible with `-blockfilterindex` and
  `-coinstatsindex`, which require block and undo data. If a crash interrupts a
  chainstate flush while this mode is active, a full `-reindex` may be required
  to redownload blocks that were intentionally not stored.
