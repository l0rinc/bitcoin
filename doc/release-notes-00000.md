# RPC

* The `gettxoutproof` RPC now bounds UTXO lookup work across all requested
  txids when no block hash or synced transaction index is available. For
  requests with multiple txids, this fallback may fail to locate the block even
  when one tx has an unspent output. Specify the block hash or enable `-txindex`
  when reliable lookup is required.
