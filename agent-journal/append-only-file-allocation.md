# Append-only preallocation and physical/logical file-size audit

## Seed from Cycle 300

This goal was added after the Goal 65 contributor-branch radar found and fixed
the Linux/POSIX fallback cell in PR #35524's evidence neighborhood. The exact
current-tree finding is closed in commit `272de16fea`: before the fix,
`AllocateFileRange()` sought to the logical offset and wrote the full requested
length when `posix_fallocate()` failed. A forced `EOPNOTSUPP` run against the
real `FlatFileSeq::Allocate()` path overwrote 100 bytes of a 128-byte existing
sentinel file. The repair seeks the physical end and appends only a missing
suffix. Do not repeat that exact Linux fallback cell without new callers or
changed platform behavior.

## Next evidence cells

- Compare the Windows implementation's file-size guard and `SetEndOfFile`
  behavior against existing files and failed `SetFilePointerEx` calls.
- Check the macOS `F_PREALLOCATE` plus `ftruncate` path for accidental shrinking
  when physical size exceeds the requested logical end.
- Bound `offset + length` and `off_t` conversions on 32-bit and unusual
  filesystems; distinguish impossible production chunk sizes from public-helper
  misuse.
- Inject short writes, `fwrite` errors, close failures, and crashes around
  block-file allocation and reindex. Verify which bytes and metadata callers
  treat as durable after an advisory helper returns.
- Replay a scratch reindex/recovery sequence with existing `blk*.dat` and
  `rev*.dat` files, then compare block hashes, file positions, and final
  chainstate. Keep the fault schedule and image for any new failure.

Each new cell must establish the physical/logical contract from callers and
history, use an independent failing-before/passing-after or platform trace,
and preserve the smallest regression. A platform-only or remote-only issue is
an evidence entry unless the current repository has an applicable fix.
