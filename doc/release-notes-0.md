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
  and its historical ancestors, and reduces restart and reorg resilience during
  initial sync.

  While this mode is syncing, automatic pruning no longer forces an immediate
  chainstate write (except for the first prune event, which anchors the
  chainstate on disk); the chainstate is written on the regular periodic
  schedule or under cache pressure instead. After an unclean shutdown the node
  may therefore restart at an older height and automatically redownload the
  missing recent blocks; a crash during the chainstate write itself may require
  a restart with `-reindex`. Clean shutdowns are unaffected, and the standard
  behavior of writing the chainstate on every prune event resumes once initial
  sync completes.
