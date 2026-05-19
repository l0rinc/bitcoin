# Seek Compaction Patch Notes

## External patch check

- Fetched `https://github.com/bitcoin-core/leveldb-subtree` into `/data/my_storage/leveldb-subtree`.
- Added Mojang as a remote and fetched `https://github.com/Mojang/leveldb`.
- Mojang's custom changes are on `mojang/main`; its default `master` tracks the upstream mirror.
- Cherry-picking Mojang commit `c069804` (`disable_seek_autocompaction`) onto `origin/bitcoin-fork` applied cleanly.
- Cherry-picking Mojang commit `58cdc9e` (L0 slowdown/stop thresholds) onto `origin/bitcoin-fork` applied cleanly.
- The latest Mojang `mojang/main` still has `disable_seek_autocompaction` in `Options`, gates `Version::UpdateStats`, gates new-file `allowed_seeks` initialization, and has L0 slowdown/stop thresholds at `16`/`64`.

## Core-local patch

The current Core-local patch is intentionally smaller than Mojang's configurable option:

- `Version::UpdateStats`
  - This is the scheduling point used by `DBImpl::Get()` and iterator read sampling to turn read seek counters into `file_to_compact_`.
  - It now returns `false` without decrementing `allowed_seeks`.
  - This globally disables seek-triggered compactions for Core's vendored LevelDB.
  - Size compactions still use `compaction_score_`; manual compactions still use `CompactRange`.

- No `leveldb::Options` knob
  - The latest issue discussion favored disabling seek compaction globally rather than adding chainstate-only Core plumbing.
  - Avoiding an option keeps the diff local to LevelDB behavior and avoids `DBOptions`/`CDBWrapper` pass-through code.
  - This is broader behaviorally than the earlier chainstate-only draft because it also affects txindex, coinstatsindex, block filter indexes, and block tree LevelDB.

- `DBTest.GetEncountersEmptyLevelDoesNotSeekCompact`
  - Reuses the upstream `GetEncountersEmptyLevel` layout.
  - Repeated reads used to schedule a seek compaction of an L0 file.
  - The updated test proves the same reads leave the level layout unchanged.

- `AutoCompactTest`
  - This upstream test specifically expected read-triggered auto-compaction.
  - With seek compaction disabled globally, it now verifies repeated reads do not shrink the touched range.

## Deliberately not carried into Core

- Mojang's C++ option and C API setter are unnecessary for the global Core-local change.
- Mojang's new-file `allowed_seeks` initialization guard is unnecessary because the counter is no longer consumed by `Version::UpdateStats`.
- Mojang's L0 threshold patch is separate. It may affect L0 pressure, but it does not disable seek-triggered compaction.

## Validation

- `git diff --check`
- `cmake --build build-35298-seekfix --target test_bitcoin bitcoind -j8`
- `build-35298-seekfix/bin/test_bitcoin --run_test=dbwrapper_tests,validation_flush_tests,coins_tests`
- Manually compiled and ran LevelDB `db_test` against Core's `libleveldb.a`: `==== PASSED 56 tests`
- Manually compiled and ran LevelDB `autocompact_test` against Core's `libleveldb.a` with `TEST_TMPDIR=/data/my_storage/tmp/leveldb-tests`: `==== PASSED 2 tests`
