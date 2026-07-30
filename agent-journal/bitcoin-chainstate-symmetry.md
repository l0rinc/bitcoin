# Bitcoin chainstate, reorg, prune, and index crash-symmetry

## Cycle 162: directory-commit failure published chainstate metadata

Status: confirmed and fixed on the cycle branch.

Branch: `uber-cycle-162-bitcoin-chainstate-symmetry-20260730`

Start HEAD: `7bd126c42ffd0cea05e87efa12d6ea48e237743b`

Start origin/master: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`

Start merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`

Start divergence from origin/master: `1105 42`

### Scope and prior evidence

The selected campaign is goal 86, `bitcoin-chainstate-symmetry`. The trust boundary is the local filesystem and power-loss/restart boundary around block and undo files, block-index metadata, chainstate metadata, and `ChainStateFlushed` publication. The defect is not directly remotely triggerable; an I/O or filesystem failure is required, but the published locator can cause a restart to trust state that was not durably committed.

Earlier work was excluded from this cycle: Cycle 142 fixed a block-file open/flush failure being ignored before metadata publication (`d6e42bca9c`), Cycle 135 audited BlockFilterIndex physical-file and database cursor relationships without a finding, and Cycle 134 fixed RPC-cookie partial writes. The remaining queue explicitly included flat-file `FileCommit`/`DirectoryCommit` ordering and metadata publication after failed flush.

The current call chain was:

`Chainstate::FlushStateToDisk` -> `BlockManager::FlushChainstateBlockFile` -> `BlockManager::FlushBlockFile` -> `FlatFileSeq::Flush` -> `FileCommit` and `DirectoryCommit`.

`FlatFileSeq::Flush` ignored the `DirectoryCommit` result. `DirectoryCommit` was `void` and ignored both failure to open the directory and failure from `fsync`. This was especially significant because commit `457490403853321d308c6ca6aaa90d6f8f29b4cf` added the directory sync specifically to make a new block-file directory entry durable after file data was synced. LevelDB's analogous `SyncDirIfManifest` path propagates errors while accepting only `EINVAL` for filesystems that cannot sync directories.

### Falsifiable hypothesis

If the blocks-directory `fsync` returns `EIO` after the block file's `fdatasync` succeeds, the current implementation will return success from `FlushChainstateBlockFile`, write block-index and coin metadata, and enqueue `ChainStateFlushed`. That violates the flush/publication contract because the directory entry was not confirmed durable.

### Independent reproduction

The Linux-only fault harness was `agent-journal/cycle162_directory_fsync_fail.c`. It redirects the existing test's synthetic `/sys/kernel/uevent_seqnum` block-file symlink to a scratch regular file, injects one `EIO` only for an fd whose path is the test `blocks` directory, and logs each injected call. It was compiled outside the repository build tree:

`gcc -shared -fPIC -O2 -Wall -Wextra -o /data/my_storage/tmp/cycle162-chainstate-symmetry/directory_fsync_fail.so agent-journal/cycle162_directory_fsync_fail.c -ldl`

The pre-fix binary was `/data/my_storage/tmp/cycle89-build/bin/test_bitcoin`; it was built before this source edit and has SHA-256 `fc05b6e538769184b263de31f6eae991aecc2bdc70b3de9b997773df0acb3edc`. With a scratch `TMPDIR`, `FAIL_BLOCKS_DIRECTORY_FSYNC=1`, `FAKE_BLOCK_FILE=/data/my_storage/tmp/cycle162-chainstate-symmetry/fake-block.dat`, and that preload library, the existing test command was:

`TMPDIR=/data/my_storage/tmp/cycle162-chainstate-symmetry/unpatched-test-tmp FAIL_BLOCKS_DIRECTORY_FSYNC=1 FAKE_BLOCK_FILE=/data/my_storage/tmp/cycle162-chainstate-symmetry/fake-block.dat LD_PRELOAD=/data/my_storage/tmp/cycle162-chainstate-symmetry/directory_fsync_fail.so /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=chainstate_write_tests/flush_failure_stops_metadata_publication --random=162005 --catch_system_errors=no --log_level=test_suite -- -printtoconsole=1`

The pre-fix run injected `EIO`, then logged `ChainStateFlushed`, and failed all three expected assertions: `!flushed`, `state.IsError()`, and `!sub->m_did_flush`. This is an executable before/after proof, not a pattern-only finding.

### Fix

`DirectoryCommit` now returns `bool`, reports open, sync, and close errors, and treats only `EINVAL` from directory `fsync` as an unsupported-filesystem success. `FlatFileSeq::Flush` closes and returns failure when the directory commit fails. The existing chainstate flush path therefore stops before block-index and coin-state publication. `src/test/fs_tests.cpp` adds direct coverage for an unopenable directory path and the `[[nodiscard]]` declaration prevents future ignored results.

The patched target was built with:

`cmake --build /data/my_storage/tmp/cycle106-clang19-ubsan --target test_bitcoin -j2`

The patched binary `/data/my_storage/tmp/cycle106-clang19-ubsan/bin/test_bitcoin` has SHA-256 `5e47cde669521b2ea9d0943cf282dd2e717ff609226feb3e237e297b89875344`. The corresponding command was:

`TMPDIR=/data/my_storage/tmp/cycle162-chainstate-symmetry/patched-test-tmp FAIL_BLOCKS_DIRECTORY_FSYNC=1 FAKE_BLOCK_FILE=/data/my_storage/tmp/cycle162-chainstate-symmetry/fake-block.dat LD_PRELOAD=/data/my_storage/tmp/cycle162-chainstate-symmetry/directory_fsync_fail.so /data/my_storage/tmp/cycle106-clang19-ubsan/bin/test_bitcoin --run_test=chainstate_write_tests/flush_failure_stops_metadata_publication --random=162006 --catch_system_errors=no --log_level=test_suite -- -printtoconsole=1`

It exited zero, logged the injected `EIO`, `Failed to flush block file`, and `Failed to flush block file` state, and did not log `ChainStateFlushed`; the test ended with `*** No errors detected`.

### Validation

All commands used scratch `TMPDIR` directories and no default datadir, wallet, key, or production database.

- `cmake --build /data/my_storage/tmp/cycle106-clang19-ubsan --target test_bitcoin -j2`
- `test_bitcoin --run_test=fs_tests --random=162001 --catch_system_errors=no --log_level=test_suite -- -printtoconsole=0`: no errors
- `test_bitcoin --run_test=flatfile_tests --random=162002 --catch_system_errors=no --log_level=test_suite -- -printtoconsole=0`: no errors
- `test_bitcoin --run_test=chainstate_write_tests --random=162003 --catch_system_errors=no --log_level=test_suite -- -printtoconsole=0`: no errors
- `test_bitcoin --run_test=blockmanager_tests --random=162007 --catch_system_errors=no --log_level=test_suite -- -printtoconsole=0`: no errors
- `test_bitcoin --run_test=blockfilter_index_tests --random=162008 --catch_system_errors=no --log_level=test_suite -- -printtoconsole=0`: no errors
- `test_bitcoin --run_test=validation_flush_tests --random=162009 --catch_system_errors=no --log_level=test_suite -- -printtoconsole=0`: no errors
- `test_bitcoin --run_test=validation_chainstatemanager_tests --random=162010 --catch_system_errors=no --log_level=test_suite -- -printtoconsole=0`: no errors
- `git diff --check`: clean

The build emitted only the pre-existing Clang warning that object-size sanitization has no effect at `-O0`. The injected EIO test is Linux/ELF-specific; no actual power-loss or dm-flakey replay was attempted in this cycle, and the Windows no-op directory-commit path remains platform-specific.

### Commit and handoff

The source/test change must be committed as one self-contained finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, with this journal update. The next unchecked cells are prune unlink-directory durability after successful flat-file flush, failure ordering for other atomic rename users, and index publication/restart behavior under a database or directory-sync fault. Do not treat this cycle as repository completion.
