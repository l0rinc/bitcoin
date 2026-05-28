Performance
-----------

- After a node connects its configured assumevalid block (the default value is
  updated as part of each major release), Bitcoin Core now compacts the
  chainstate LevelDB database. This may temporarily increase disk activity, but
  can reduce chainstate disk usage after the initial sync completes.
