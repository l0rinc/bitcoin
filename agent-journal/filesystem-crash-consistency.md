# Filesystem, power-loss, and crash-consistency injection

## Cycle 134: RPC cookie partial-write recovery

### Selection and fresh gate

- Selected by exact `shuf -i 0-98 -n 1` -> `72` (`filesystem-crash-consistency`) after the Cycle 133 gate; no reroll was needed.
- Branch: `uber-cycle-134-filesystem-crash-consistency-20260730`.
- Cycle-start HEAD: `55585fa9a2b54ffc61c06439a16f475488926ae1`; `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `40 1054`.
- The gate passed fetch, tracked/index cleanliness, `git diff --check`, all four catalog/protocol hashes, and process checks. PID `777094` (`wallet_tests`, parent `725042`) was preserved. Known unrelated untracked artifacts remain outside the cycle.
- Catalog hashes: reusable goals `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, random prompt `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, goals TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and uber protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

### Distinct scope and hypothesis

Cycle 113 already fixed direct-write banlist truncation and excluded the settings write/close check fixed by `0654511e1b`. This cycle selected the remaining temporary-file boundary for RPC authentication cookies. `GenerateAuthCookie()` writes credentials to `.cookie.tmp`, then renames that file into `.cookie`, but previously ignored both the stream write state and the state after `close()`. The falsifiable hypothesis was that an `EIO`/short-write schedule could make the daemon report successful cookie generation while publishing an empty or partial credential, replacing a previously valid cookie and leaving restart/authentication inconsistent.

### Independent pre-fix reproduction

A scratch `LD_PRELOAD` library at `/data/my_storage/tmp/cycle134_cookie_partial_write.c` was compiled with `gcc -shared -fPIC -O2 ... -ldl`. It returns `EIO` for writes, `writev`, `pwrite`, `fwrite`, and `fflush` on descriptors whose path contains `.cookie.tmp`, and leaves all other paths unchanged. The older pre-fix daemon `/data/my_storage/tmp/cycle124-wallet-tool/bin/bitcoind` was `v31.99.0-a6bc9afb5417`.

In scratch datadir `/data/my_storage/tmp/cycle134-cookie-pre-2`, an existing 19-byte `/regtest/.cookie` containing `__cookie__:previous` was present before startup. With `FAIL_COOKIE_WRITE=1` and the hook, the old daemon logged both `Generated RPC authentication cookie` and `Using random cookie authentication`, reached the HTTP startup path, and was then killed with `SIGKILL`. Afterward `.cookie` was `0` bytes and `.cookie.tmp` no longer existed. This is an independent durable-state replacement proof: a write failure was reported as successful initialization and destroyed the prior usable credential.

### Fix and regression evidence

`GenerateAuthCookie()` now checks `file.fail()` immediately after writing and again after `file.close()`. Either failure returns `AuthCookieResult::Error`, so `InitRPCAuthentication()` aborts before `RenameOver()` can publish the failed temporary file. `rpc_tests/rpc_cookie_write_failure_preserves_existing_file` creates an existing cookie and a `.cookie.tmp` symlink to Linux `/dev/full`; it verifies the result is `Error` and the prior cookie remains byte-for-byte unchanged.

The fixed tree was built with:

    CCACHE_DIR=/data/my_storage/tmp/cycle134-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin bitcoind -j4

The focused regression passed after creating its scratch `TMPDIR`:

    mkdir -p /data/my_storage/tmp/cycle134-test-tmp
    TMPDIR=/data/my_storage/tmp/cycle134-test-tmp /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=rpc_tests/rpc_cookie_write_failure_preserves_existing_file --log_level=test_suite
    *** No errors detected

The full `rpc_tests` suite passed 23 cases; `rpc_tests,settings_tests,fs_tests,netbase_tests` passed 58 cases; and the complete current-tree unit binary ran 1,217 cases and exited 0 with `*** No errors detected`. The first focused invocation was rejected as an environment setup failure because its requested `TMPDIR` did not exist; the rerun used the explicit `mkdir -p` above.

The fixed daemon `/data/my_storage/tmp/cycle89-build/bin/bitcoind` was run in `/data/my_storage/tmp/cycle134-cookie-post-1` with the same hook and a prior 19-byte cookie. It exited status 1, logged `Unable to close RPC authentication cookie file` followed by `Unable to start HTTP server`, preserved the old cookie at 19 bytes, and left a zero-byte `.cookie.tmp`. Starting normally from that same directory then produced a fresh 75-byte cookie with the `__cookie__:` prefix, proving the failed temporary state is recoverable on restart.

### Verdict and handoff

Confirmed local persistence/authentication-integrity finding. The old code treated a failed temporary credential write as successful and renamed it over the durable cookie; the fix fails closed before publication and preserves restart recovery. Source, regression test, and this journal are one self-contained finding commit. The remaining goal-72 queue is flat block/undo `FileCommit` and `DirectoryCommit` fault ordering, chainstate/index metadata publication after failed flush, and other atomic-rename users; do not repeat the banlist or settings cells. Scratch artifacts are under `/data/my_storage/tmp/cycle134-cookie-*`. Status transitions: `scouted -> independently reproduced -> fixed -> fault-replayed -> restart-verified -> reviewed`.

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
