## Index

- The transaction index (`-txindex`) now stores less data on disk, more than
  halving the size of a fully rebuilt index. The index is backwards compatible,
  so existing users will not see the space saving unless the index is recreated.
  To do so, stop the node, delete the `<datadir>/indexes/txindex` directory, and
  restart; rebuilding can take up to a few hours depending on hardware. Once
  rebuilt, the index can no longer be read by previous releases, so downgrading
  will require rebuilding it again.
  Additionally, transactions that are only in blocks reorged out of the best
  chain are no longer kept in the index. To look up such a transaction, pass its
  (stale) block hash to the `getrawtransaction` RPC. (#35531)

- Pruned nodes can opt in to fetching previously processed active-chain blocks
  from outbound full-history peers for local RPC and wallet reads with
  `-blockfetchproxy`. The option requires prune mode, is disabled by default,
  and stores fetched blocks in the normal block files with a local-only marker.
  This lets repeated local reads and restarts reuse the block without fetching
  it again, while preventing this version from relaying or serving it to peers.
  The containing block file remains subject to normal pruning and may be deleted
  later. The serving peer can observe which block hashes are requested. Block
  undo data is not fetched, so `getblock` verbosity 3 remains unavailable for
  fetched blocks. Older versions do not understand the local-only marker and may
  serve retained blocks after a downgrade. Reindexing also rebuilds the block
  index without these markers and may make retained blocks servable.

  With this option, a fully synced transaction index can remain enabled when
  pruning. Existing indexes containing legacy full-txid entries must be
  recreated first. Enabling an unsynced transaction index after its required
  blocks have already been pruned still requires a full reindex.

  Wallet rescans can use the proxy to inspect pruned blocks. Compact block
  filters select blocks that match the wallet's known scripts, and a basic block
  filter index is required when the rescan needs fetched blocks. This can restore
  fully spent history for known descriptors, but it cannot discover transactions
  involving scripts or derivation ranges the wallet does not know.
