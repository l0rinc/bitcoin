New settings
------------

- A new experimental `-pruneassumevalid` option reduces bandwidth and block/undo
  file writes while bootstrapping a pruned node.
  Automatic pruning normally deletes most historical block data before initial block download (IBD) finishes, and assumevalid already skips script checks for part of this history.
  Only while the existing assumevalid conditions apply during pruned IBD, blocks can be downloaded without witnesses, connected to the UTXO set without writing block or undo data, and discarded.
  The existing assumevalid ancestry, minimum-chainwork, and two-week burial checks apply.
  This extends assumevalid trust to witness commitments and availability, in addition to scripts.
  The option is disabled by default.
  If `-prune` is omitted, enabling the option selects automatic pruning with
  a 550 MiB target. Explicit pruning settings are retained, including
  `-prune=0`, which disables the optimization.

  A fixed ten-block limit covers cached blocks and outstanding stripped
  requests, reserving one slot for the next missing parent. Reaching the limit
  pauses stripped requests rather than storing eligible blocks. The ordinary
  1024-block download window and per-peer in-flight limit are unchanged.
  This bounds block count, not allocated memory or total process memory.
  The initial limit is not configurable.

  Ordinary validation, storage, and pruning apply when the preconditions fail.
  Stored non-genesis block history, block filter or coin statistics indexes, and
  UTXO snapshots disable the optimization. Once ordinary block storage starts,
  the optimization stays disabled for the run. Existing blocks are retained.
  Already accepted cached blocks keep their admission-time script decision,
  including after a best-header change. `-prune=1` permits the optimization
  despite otherwise requiring manual pruning.
  If an accepted stripped block fails to connect, the node stops.

  Completed chainstate flushes allow restart without omitted block files,
  including with the option disabled or assumevalid changed. These settings
  do not retrospectively validate omitted witnesses. A crash during block
  connection resumes from the last completed chainstate flush and redownloads
  unflushed progress. An interrupted multi-batch flush is detected at startup
  and requires a full `-reindex` when using `-pruneassumevalid`.
  A reorg requiring omitted block or undo data cannot be completed. Restoring
  and fully validating omitted history also requires redownload and reindex.

  Omitted blocks cannot be served or used for wallet rescans. Fetching one
  later restores its block data without reconstructing undo data, so startup
  verification at level 3 or 4 stops there. The node continues advertising
  `NODE_NETWORK_LIMITED` during IBD, when recent omitted blocks can be
  unavailable. Under default settings these blocks are normally older than
  the 288-block serving window when IBD ends. Custom settings can change this.
  Wallet transactions learned from stripped blocks can lack witnesses and
  report their txid as their wtxid. `-blocknotify` and hash-only ZMQ notifications
  can name unavailable blocks. Raw-transaction ZMQ notifications for these
  blocks lack witnesses, and raw-block notifications require stored data.
