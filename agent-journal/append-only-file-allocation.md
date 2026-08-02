# Append-only preallocation and physical/logical file-size audit

## Cycle 302: Windows physical/logical file-size guard

### Selection and fresh gate

- Exact selector: `shuf -i 0-101 -n 1` -> `100` (`append-only-file-allocation`).
  The dedicated branch is
  `uber-cycle-302-append-only-file-allocation-20260802`.
- Cycle-start HEAD was `61d868e7f6c5e26a1151d91ca8ec7c300735bc96`;
  `origin/master` was `556988790a7f961693a8fd93f73725baea66476a`; merge-base
  equaled `origin/master`; start divergence was `0 1398`; and the tracked
  worktree passed the fresh post-Cycle-301 gate. The seven protected test
  processes remained alive and were not touched.
- The Linux/POSIX fallback fixed in Cycle 300 was excluded. This cycle
  selected the current `WIN32` implementation and its separate physical-file
  boundary.

### Hypothesis and independent reproduction

The current Windows branch computed `offset + length`, called
`SetFilePointerEx`, and unconditionally called `SetEndOfFile`. If a flat file
already has physical bytes beyond the logical append position, the latter
operation truncates those bytes. A failed `SetFilePointerEx` could also leave
the handle at an unintended position before `SetEndOfFile` ran.

This is supported by the current source and history: `fdec41914f` records the
same physical-size guard as an unmerged public fix, while the current branch
still contains the pre-guard sequence. The independent model
`agent-journal/cycle302_windows_allocate_model.cpp` implements the documented
Windows handle semantics without Bitcoin code. Compiled and run with:

```text
clang++-19 -std=c++17 -Wall -Wextra -Werror agent-journal/cycle302_windows_allocate_model.cpp -o /data/my_storage/tmp/cycle302-windows-allocate-model
/data/my_storage/tmp/cycle302-windows-allocate-model
```

It returned:

```text
legacy_size=200 fixed_size=256 failed_seek_size=256
```

The legacy sequence therefore violates the physical/logical preservation
property for a 256-byte file and a requested logical end of 200; the guarded
sequence preserves the file and does not truncate on a failed seek.

### Fix and regression

The Windows branch now returns when `GetFileSizeEx` fails or the requested end
is not beyond the current physical size. It checks `SetFilePointerEx` before
calling `SetEndOfFile`. The new
`flatfile_allocate_preserves_data_beyond_logical_end` test writes a 256-byte
file, asks `FlatFileSeq::Allocate()` to extend from logical offset 100 to 200,
and requires the size and every byte to remain unchanged. The existing
offset-zero preservation test remains in the same suite.

### Validation

The rebuilt target passed:

```text
cmake --build /data/my_storage/tmp/cycle246-wallet --target test_bitcoin -j2
```

With a pre-created scratch temporary directory, the focused test passed 10/10
assertions and the complete `flatfile_tests` suite passed 6/6 cases and 48/48
assertions:

```text
mkdir -p /data/my_storage/tmp/cycle302-flatfile
TMPDIR=/data/my_storage/tmp/cycle302-flatfile /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=flatfile_tests/flatfile_allocate_preserves_data_beyond_logical_end --random=30206 --log_level=all --report_level=short --color_output=false
TMPDIR=/data/my_storage/tmp/cycle302-flatfile /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=flatfile_tests --random=30207 --log_level=test_suite --report_level=short --color_output=false
```

An earlier attempt without a pre-created `TMPDIR` used the nearly-full `/tmp`
filesystem and failed in test setup/disk-space preparation; it was not treated
as product evidence and was rerun with the recorded scratch directory. Linux
compiled and exercised the already-fixed POSIX branch; the Windows branch was
not executable on this host, so the model and cross-platform regression are
the independent Windows evidence. `git diff --check` passed.

### Verdict and learned queue

**Confirmed and fixed.** The current Windows implementation could truncate
valid existing flat-file bytes when physical size exceeded logical position,
and it could call `SetEndOfFile` after a failed seek. The smallest fix is in
`src/util/fs_helpers.cpp`, with the production-path regression in
`src/test/flatfile_tests.cpp`. No claim is made about macOS or arithmetic
overflow.

The next learned cell is the macOS `F_PREALLOCATE` plus `ftruncate` path:
determine whether it has the same physical/logical shrink behavior, whether a
failed fallback allocation is ignored, and how its `off_t` conversion behaves.
Add a separate goal and journal seed before the next draw; do not repeat the
Windows or Linux cells without a changed platform or failure boundary.

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
