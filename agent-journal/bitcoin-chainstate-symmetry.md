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
## Cycle 240: chainstate, reorg, prune, and index crash symmetry

Status: no confirmed finding; journal-only close.

Exact selector: `shuf -i 0-98 -n 1` -> `86` (`bitcoin-chainstate-symmetry`); no reroll.

Branch: `uber-cycle-240-bitcoin-chainstate-symmetry-20260731`.

Cycle-start HEAD: `cc943e5285b20a6aac5521f4163faee3a1f0e435`.

Cycle-start `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.

Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.

The fresh gate found the same 42/1263 divergence from `origin/master`, unchanged catalog, prompt, TSV, and protocol hashes, and the known unrelated untracked artifacts. Protected long-running tests were left untouched. `/` remained full while `/data` had approximately 22 GB available, so all new data used `/data/my_storage/tmp` and no default datadir, wallet, key, or production database was used.

### Scope and expected invariant

The trust boundary is local block/undo storage, the block-index database, chainstate publication, indexes, pruning, restart, and simulated crash or I/O failure. The expected invariant is that a published chainstate or block-index transition has a recoverable durable predecessor and that connect/disconnect, prune/restart, index publication, and replay produce the same authoritative state. A failure before the durable metadata transition must not publish the transition; a failure after metadata publication must be safe to repeat during startup.

The existing queue came from Cycle 162: prune unlink-directory durability after a successful flat-file flush, failure ordering for other atomic rename users, and index publication/restart under database or directory-sync faults. The current code was also checked for the tempting alternative hypothesis that `WriteBlockIndexDB()` loses dirty state if its synchronous database write fails.

### Candidate ledger

1. **Block-index database error loses dirty state on a retry.** `WriteBlockIndexDB()` drains dirty sets before `WriteBatchSync`, but the LevelDB wrapper throws on a failed synchronous write and `FlushStateToDisk` treats the exception as a fatal shutdown path. No live retry path was found that could observe the drained sets as a successful flush. This is dismissed for the current contract; it remains a useful fault-injection target if the shutdown contract changes.

2. **Prune unlink failure leaves an unrecoverable state.** Pruning writes the block-index metadata and pruned-file state before `UnlinkPrunedFiles`. Unlink failures leave files present, while startup `ScanAndUnlinkAlreadyPrunedFiles()` finds zero-sized pruned file records and retries cleanup. A crash before unlink is therefore explicitly covered by the startup scan, and the ordering avoids publishing a deleted file before its metadata transition. The existing code ignores unlink errors, but no restart-visible correctness defect was reproduced; this candidate is dismissed, with the limitation that a real power-loss replay was not run.

3. **Rollover flush failure is silently accepted and later publication becomes asymmetric.** `FindNextBlockPos()` intentionally does not propagate the flush result for an already-written previous block/undo file. The current kernel notification contract allows this caller policy, and the node notification implementation immediately requests shutdown on `flushError`. Historical commits `f0207e0030` and `064859bbad6` document that this behavior is deliberate: the prior block need not block the operation, while a reindex undo inconsistency and untrimmed files are left for the shutdown/recovery path. The existing directory-fsync preload hit this path deterministically, reported the injected `EIO`, and exited cleanly without a restart-visible inconsistency. This candidate is dismissed, not silently accepted as proof of safety.

4. **Index publication gets ahead of flushed chainstate during prune/reorg/restart.** The current `BaseIndex` and chainstate code preserve the index-behind-flushed-chainstate rule, and the existing functional index/prune workflow exercises disable/restart, pruning, reindex, and reorg-lock stages. No new publication ordering defect was found.

5. **Combined reorg, block-index, prune, and recovery transitions violate symmetry.** The production `validation_block_reorg` fuzz target completed 1,000 deterministic runs without a crash or assertion. The initial default 2 GiB ASan RSS limit produced an OOM artifact at input `16 0a ff ff` with a 2,225 MB baseline peak; this was classified as a sanitizer configuration artifact. The rerun used `-rss_limit_mb=8000`, completed with peak RSS 2,224 MB, coverage 42,595, and no crash.

6. **Crash during chainstate writes leaves divergent UTXO state.** `feature_dbcrash.py` completed all 40 built-in iterations. It generated 2,500 transactions per iteration, restarted nodes after 22 injected crashes, verified UTXO hashes on every node, and exited zero with `Tests successful`. This is strong bounded evidence against the current crash-recovery hypothesis, not an unbounded power-loss proof.

### Independent reproduction and validation

The focused baseline suites used the existing binary `/data/my_storage/tmp/cycle214-build/bin/test_bitcoin`:

`/data/my_storage/tmp/cycle214-build/bin/test_bitcoin --run_test=blockmanager_tests --random=240001 --catch_system_errors=no --log_level=test_suite`

This passed 12 cases and 127 assertions. The chainstate write suite passed 3 cases and 84 assertions; its expected directory-fsync error log was present.

The existing Linux preload harness was compiled with:

`gcc -shared -fPIC -O2 -o /data/my_storage/tmp/cycle240-rollover-preload/libcycle162_directory_fsync_fail.so agent-journal/cycle162_directory_fsync_fail.c -ldl`

The deterministic fault run was:

`FAIL_BLOCKS_DIRECTORY_FSYNC=1 LD_PRELOAD=/data/my_storage/tmp/cycle240-rollover-preload/libcycle162_directory_fsync_fail.so TMPDIR=/data/my_storage/tmp/cycle240-rollover-preload /data/my_storage/tmp/cycle214-build/bin/test_bitcoin --run_test=blockmanager_tests/blockmanager_scan_unlink_already_pruned_files --log_level=test_suite --random=240004`

It exited zero, injected `EIO` into the blocks-directory fsync, and logged the fatal internal flush error. This exercised the rollover path but did not produce a contradictory restart state.

The block-index fuzzer was run for 2,000 executions with `-rss_limit_mb=8000`, seed 240003, and no artifact. The validation/reorg fuzzer was run for 1,000 executions with seed 240005, `-rss_limit_mb=8000`, and no artifact; its final line was `Done 1000 runs in 168 second(s)` with peak RSS 2,224 MB.

The prune/index functional run used the intact scratch cache `/data/my_storage/tmp/cycle89-build/test/cache` because the repository's current `test/cache/node0` is incomplete and was not modified:

`TMPDIR=/data/my_storage/tmp/cycle240-functional-tmp python3 test/functional/feature_index_prune.py --configfile=/data/my_storage/tmp/cycle214-build/test/config.ini --cachedir=/data/my_storage/tmp/cycle89-build/test/cache --tmpdir=/data/my_storage/tmp/cycle240-feature-index-prune2 --randomseed=240006 --loglevel=INFO`

It exited zero with `Tests successful` after index disable/restart, pruning, reindex, and reorg-lock checks.

The crash campaign used the same intact scratch cache:

`TMPDIR=/data/my_storage/tmp/cycle240-functional-tmp python3 test/functional/feature_dbcrash.py --configfile=/data/my_storage/tmp/cycle214-build/test/config.ini --cachedir=/data/my_storage/tmp/cycle89-build/test/cache --tmpdir=/data/my_storage/tmp/cycle240-feature-dbcrash --randomseed=240007 --loglevel=INFO`

It exited zero. Its final evidence was `Restarted nodes: [14, 4, 2]; crashes on restart: 22`, followed by UTXO-hash verification and `Tests successful`.

### Verdict and limitations

No source change is justified by this cycle. The database-write retry candidate is blocked by the fatal-error contract, the unlink candidate is covered by metadata-before-unlink ordering plus the startup scan, the rollover candidate is deliberate and fault-injected without a restart mismatch, and the independent fuzz/functional campaigns found no new state divergence. This does not prove arbitrary power-loss schedules, filesystem behavior, or all index/database failure combinations; those remain unchecked evidence cells rather than claims of exhaustion.

The first `feature_index_prune.py` attempt was rejected as an environment fixture failure because the default repository cache had chain height zero instead of the expected prepared chain. It was rerun with the intact scratch cache and passed. The first block-index fuzz run was rejected as an ASan RSS-limit artifact and rerun with an explicitly recorded higher limit.

### Handoff

Keep the next queue distinct: perform a targeted database fault schedule around index commit and restart; exercise a crash between prune metadata commit and unlink with a durable scratch filesystem; then test block/undo file recovery under a controlled short-write or ENOSPC schedule. Reuse the existing directory-fsync harness only for a new state transition, not to repeat the dismissed rollover cell. Do not treat this cycle as repository completion.
