Updated settings
----------------

- Nodes using automatic pruning can now download eligible assumevalid blocks
  without witnesses and connect them without writing block or undo data during
  initial block download (IBD).
  The existing assumevalid ancestry, minimum-chainwork, and two-week burial checks apply.
  This extends assumevalid trust to witness commitments, witness availability, and total block weight, in addition to scripts.
  Manual pruning (`-prune=1`) retains downloaded blocks until they are pruned
  through RPC. With `-assumevalid=0`, automatic pruning downloads, verifies, and
  writes blocks before pruning them.

  During and after bootstrap, the available history can be shorter than the configured
  pruning target. Choose an earlier assumevalid block before syncing to retain
  more history. As blocks beyond the assumevalid range arrive, the
  ordinary retained window fills. Changing the anchor does not recover
  previously omitted history.

  Blocks received before their parent is connected can wait in memory.
  The debug option `-blockdownloadmemory=<n>` sets their serialized-byte budget,
  which is 64 MiB by default and separate from `-dbcache`. The budget also
  applies to nearby full blocks queued during ordinary initial block download.
  Requests for pre-SegWit blocks still set the witness flag so peers can serve
  them directly from disk while local block and undo writes are omitted.
  Queued stripped blocks are discarded and downloaded again with witnesses if
  they stop being eligible, for example when IBD ends or the best header changes.

  The debug option `-blockdownloadwindow=<n>` adjusts the download window
  (default: 1024 blocks).

  Ordinary validation, storage, and pruning apply to all other blocks, and
  whenever block filter or coin statistics indexes or a UTXO snapshot are used.
  Loading a Bitcoin Core wallet disables the optimization until restart so that
  ordinary pruning retains recent blocks needed for wallet crash recovery.
  This does not restore previously omitted history for wallet rescans.
  A notice reports transitions to ordinary pruning.
  If an omitted block fails to connect, the node stops.

  Completed chainstate flushes allow restart without omitted block files,
  including after switching to manual pruning or changing `-assumevalid`.
  These changes do not retrospectively validate omitted witnesses. A crash during block
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
  blocks can lack witnesses, and raw-block notifications require stored data.
