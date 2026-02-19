Updated `dbcache` default settings
----------------------------------

- When `-dbcache` is not set explicitly, Bitcoin Core now chooses a RAM-aware default that can be as high as `3000` MiB.
  Explicit `-dbcache` values continue to override the automatic default. (#34641)
