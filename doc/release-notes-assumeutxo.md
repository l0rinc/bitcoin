UTXO snapshots
--------------

- Each network now supports only its newest compiled snapshot commitment. Older
  snapshot files and unfinished snapshot chainstates require a compatible older
  release or rebuilding through full IBD with `-reindex`.
- Full block validation checks the supported UTXO commitment by default when
  connecting its base block. The check flushes and scans the UTXO set, which can
  take several minutes. Successful checks log the hash and elapsed time. Use
  `-checkassumeutxo=0` or `-nocheckassumeutxo` to skip it. Mismatches are diagnostic
  and do not change block acceptance. Snapshot import hash checks remain
  mandatory. Background validation continues in this change.
- Snapshot imports reject empty transaction groups and mismatches between the
  announced coin count and the resulting unique UTXO count.
