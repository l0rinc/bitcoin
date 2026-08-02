# macOS Preallocation and Physical/Logical File-Size Semantics Audit

## Seed from Cycle 302

Cycle 302 confirmed and fixed the current Windows `AllocateFileRange()`
physical/logical truncation bug. The same invariant remains open in the
`__APPLE__` branch of `src/util/fs_helpers.cpp`:

- `F_PREALLOCATE` is attempted with `F_PEOFPOSMODE` and `fst_offset = 0`;
- a failed contiguous allocation retries `F_ALLOCATEALL`, but the second
  `fcntl` result is ignored;
- `ftruncate(static_cast<off_t>(offset) + length)` is called unconditionally;
- the `ftruncate` result is ignored, and an existing physical file larger than
  the requested logical end may be shrunk.

Build or run on macOS filesystems, including an exFAT case where the project
already has special handling, and use a 256-byte existing file with logical
allocation ending at 200 as the first regression. Inject `F_PREALLOCATE` and
`ftruncate` failures, test contiguous-allocation fallback, inspect `off_t`
width and `offset + length` overflow, and replay scratch reindex/recovery with
existing `blk*.dat` and `rev*.dat` files. A faithful Darwin syscall model is
useful when hardware is unavailable, but label it separately from executed
evidence. Preserve bytes and metadata across successful advisory allocation,
failure, crash, and restart. Do not repeat the Linux or Windows cells without
new macOS semantics or a changed fault boundary.
