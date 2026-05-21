Updated behavior
----------------

- The chainstate database is now compacted once the first time the node leaves
  initial block download. This can take a long time and cause heavy disk I/O,
  but reduces chainstate disk usage after the initial sync.
