# Filesystem, power-loss, and crash-consistency injection

## Cycle 113: durable-boundary fault injection

### Gate and scope

- Selected by exact `shuf -i 0-98 -n 1` -> `72` (`filesystem-crash-consistency`) after Cycle 112 closed goal 63.
- Branch: `uber-cycle-113-filesystem-crash-consistency-20260729`.
- Cycle-start HEAD: `82dfe93f418a43051269bcbee791fdf965dc2817`.
- Base: `origin/master` at `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` is `40 1015`.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- The fresh gate passed fetch, tracked/index cleanliness, `git diff --check`, catalog hashes, and process checks. PID `777094` (`wallet_tests`, parent `725042`) was preserved.

### Contract and exclusions

This cycle tests whether a process kill, failed flush, short write, rename interruption, or restart leaves an authoritative file/DB state that is valid, recoverable, and idempotent. The experiments use only scratch directories, generated regtest data, and temporary preload hooks. They do not use the default datadir, wallets, keys, or production databases.

Goal-86 chainstate/reorg/prune/index symmetry and earlier database-engine/rebuild profiling are excluded unless a new filesystem fault schedule changes their evidence. The initial distinct cells are:

1. Read-write settings and other JSON-style files: direct stream writes, close failures, temporary-file rename, and directory durability.
2. Flat block/undo files: partial writes, `FileCommit`/`DirectoryCommit` failures, flush publication, and restart behavior.
3. Atomic rename users: whether a successful file rename is enough for the caller's stated recovery contract when the parent directory is not committed.

### Initial evidence and queue

- `common::WriteSettings()` writes to the caller-provided path and checks stream write/close failures. `ArgsManager::WriteSettingsFile()` normally supplies `settings.json.tmp`, then renames it over `settings.json`; it does not call `DirectoryCommit()`.
- `SerializeFileDB()` uses a randomized temporary file, calls `AutoFile::Commit()`, closes it, then renames it into place. `RenameOver()` is a thin `fs::rename()` wrapper and does not commit the parent directory.
- `FlatFileSeq::Flush()` commits the file, calls `DirectoryCommit(m_dir)` without checking its result, then closes the file. `FlushStateToDisk()` logs and ignores a failed block/undo flush before writing block-index metadata and chainstate state; the source has a TODO asking whether this error should be propagated.
- Historical commit `0654511e1b` fixed settings write/close failure before rename, so this cycle must not repeat that finding. The remaining hypotheses require a concrete crash or fault-schedule oracle, not a generic statement that fsync is useful.

Next queue: build the smallest deterministic fault hook; test settings temp-file failure and rename/dir-sync behavior; then test block/undo flush failure and whether metadata publication outruns durable file state. Record exact syscall/order traces, restart results, rejected harness failures, and the highest-value remaining cell after each experiment.

### Confirmed finding: banlist direct-write crash loss

The first production path selected was `CBanDB::Write()`. Before the fix it passed the final `banlist.json` path directly to `common::WriteSettings()`. A temporary `LD_PRELOAD` hook at `agent-journal/banlist_partial_write.c` was compiled with `gcc -shared -fPIC -O2 ... -ldl`; it returned `EIO` for banlist file writes while leaving all other files alone.

Independent pre-fix reproduction used the older `/data/my_storage/tmp/cycle86-parent-build/bin/bitcoind` (`v31.99.0-f0da26cfc8a4`) in `/data/my_storage/tmp/cycle113-banlist-pre-old`. After creating a valid ban containing `127.0.0.1/32`, the daemon was restarted with `FAIL_BANLIST_WRITE_ALWAYS=1`, and `setban 198.51.100.12 add 3600` returned `CLI_STATUS=0`. The existing `/regtest/banlist.json` was then `0` bytes and did not contain the requested address. After an immediate `SIGKILL` of scratch daemon PID `853930`, restart and `listbanned` returned `[]`. This is a concrete local power-loss/write-error state loss: the RPC reports success, the authoritative file is truncated, and restart discards the previously durable banlist.

The initial single-failure hook was also run against the pre-fix daemon. A graceful stop allowed `BanMan::~BanMan()` to retry its dirty in-memory map, so that schedule was rejected as a crash oracle. The final evidence uses repeated failure plus `SIGKILL`, isolating the write failure from normal shutdown retry behavior.

### Fix and verification

`CBanDB::Write()` now writes `banlist.json.tmp` through `common::WriteSettings()` and renames it over `banlist.json` only after the temporary write succeeds. A failed write leaves the previous valid file intact; a crash can leave a stale temporary file, which `CBanDB::Read()` ignores. The new `banman_tests/write_uses_temp_file` regression pre-creates stale temporary content, performs a second ban write, verifies the temporary path is consumed, and reloads both entries.

The fixed tree was built with:

    mkdir -p /data/my_storage/tmp/cycle113-ccache
    CCACHE_DIR=/data/my_storage/tmp/cycle113-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin bitcoind -j4

Focused validation passed:

    TMPDIR=/data/my_storage/tmp/cycle113-test-tmp /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=banman_tests,argsman_tests,fs_tests --log_level=message
    Running 26 test cases... *** No errors detected

The same fixed binary was run in `/data/my_storage/tmp/cycle113-banlist-post-kill`. With the repeated `EIO` hook, the failed RPC returned `CLI_STATUS=0`, the prior `banlist.json` remained `384` bytes with the original ban, and the temporary file was `0` bytes. After `SIGKILL` of scratch PID `853758`, restart and `listbanned` returned the original `127.0.0.1/32` ban. The full current-tree unit suite then ran 1,211 cases and exited `0` with `*** No errors detected`; its expected informational fault-injection logs and skipped script-assets warning were not failures.

The source/test change is one self-contained finding. Final review checked `git diff --check`, the existing settings write-failure fix at `0654511e1b`, `ArgsManager`'s analogous temp/rename path, stale-temp cleanup, and preserved unrelated dirty state. The remaining limitation is that a failed `setban` write can lose the newly requested in-memory ban after a crash, while the prior durable banlist is preserved; changing the RPC's best-effort success contract is a separate API question. This finding is a local persistence-integrity issue, not a consensus or remote P2P issue. Status transitions: `scouted -> independently reproduced -> fixed -> crash-replayed -> reviewed -> reported`.
