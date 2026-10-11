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
  `-prune=0`, which disables the optimization. `-prune=1` permits the
  optimization despite otherwise requiring manual pruning.

  Blocks received before their parent is connected wait in memory within the
  normal block download window. This memory is separate from `-dbcache`.
  They are discarded and downloaded again with witnesses if they stop being
  eligible, for example when IBD ends or the best header changes.

  The download window defaults to 512 blocks and can be adjusted with
  `-blockdownloadwindow=<n>`. On systems with memory to spare, try 1024 or 2048
  for higher download throughput. Larger windows can increase memory pressure
  during `-pruneassumevalid`, so compare IBD time and memory use before keeping
  a higher setting.

  Ordinary validation, storage, and pruning apply to all other blocks, and
  whenever block filter or coin statistics indexes or a UTXO snapshot are used.
  Loading a Bitcoin Core wallet disables the optimization until restart so that
  ordinary pruning retains recent blocks needed for wallet crash recovery.
  This does not restore previously omitted history for wallet rescans.
  Existing blocks are retained. A notice reports transitions to ordinary pruning.
  If an omitted block fails to connect, the node stops.

  Completed chainstate flushes allow restart without omitted block files,
  including with the option disabled or assumevalid changed. These settings
  do not retrospectively validate omitted witnesses. A crash during block
  connection resumes from the last completed chainstate flush and redownloads
  unflushed progress. A crash during a multi-batch chainstate flush cannot be
  replayed without the omitted blocks, so startup asks for a full `-reindex`.
  A reorg requiring omitted block or undo data cannot be completed. Restoring
  and fully validating omitted history also requires redownload and reindex.

  Omitted blocks cannot be served or used for wallet rescans. Fetching one
  later restores its block data without reconstructing undo data, so startup
  verification at level 3 or 4 stops there. The node continues advertising
  `NODE_NETWORK_LIMITED` during IBD, when recent omitted blocks can be
  unavailable. Under default settings these blocks are normally older than
  the 288-block serving window when IBD ends. Custom settings can change this.
  `-blocknotify` and hash-only ZMQ notifications
  can name unavailable blocks. Raw-transaction ZMQ notifications for these
  blocks lack witnesses, and raw-block notifications require stored data.
