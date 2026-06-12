Performance
-----------

Bitcoin Core now occasionally compacts the chainstate database in the background after post-IBD periodic flushes.
This reduces disk usage left by obsolete UTXO entries without blocking validation.

Nodes with chainstate databases created by pre-29 versions may also schedule one background compaction at startup to migrate the old small-table LevelDB layout.
This can cause elevated disk activity, and shutdown may wait for the compaction thread.
Interrupting the process remains crash-safe, and later startup can compact any remaining data.
