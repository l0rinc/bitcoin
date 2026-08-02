# Sanitizer and Valgrind True-Positive Sweep

## Cycle 274 start

- Selected by the uber loop: exact `shuf -i 0-98 -n 1` -> `11` (`sanitizer-valgrind`).
- Branch: `uber-cycle-274-sanitizer-valgrind-20260802`.
- Cycle-start HEAD: `13938c14c47ca7ab00a3f552e5dcf8a2d7b179c6` (`uber-goal: record cycle 273 close`).
- Base: `origin/master` at `556988790a7f961693a8fd93f73725baea66476a`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Scope: run a fresh Clang 19 ASan+UBSan cell against current source and target the six recent persistence, wrapper, txgraph, wallet-database, and KDF changes; classify sanitizer reports, harness failures, dependency issues, and clean negative evidence separately.
- Exclusions: do not touch the protected `cycle217-sanitizer-wallet` binary or the other protected test processes; do not repeat Cycle 83's exact fuzz targets/seeds, standalone TSan corpus, bounded secp MSan smoke, or default-limit OOM probes. Valgrind remains a tool-availability check, not a source pass.
- Scratch root: `/data/my_storage/tmp`; all new build, cache, logs, and datadirs stay there.

### Cycle 274 hypotheses

1. ASan or UBSan finds a current lifetime, bounds, alignment, or invalid-state defect in one of the six recent changes that the prior UBSan-only wallet run did not cover.
2. A targeted sanitizer run exposes an error-path or recovery defect in the new durable UTXO snapshot publication or SQLite wallet transaction-abort path.
3. A sanitizer report is caused by the harness, dependency, build configuration, or protected environment rather than project code and can be independently classified.

### Cycle 274 planned evidence

- Pin `/usr/bin/clang-19`, CMake/Ninja configuration, compiler flags, source HEAD, and isolated `ASAN_OPTIONS`/`UBSAN_OPTIONS`.
- Run focused current-source tests for `minisketch_tests`, `txgraph_tests`, wallet scriptpubkeyman/database/crypto tests, and RPC snapshot coverage where the target is available; use scratch datadirs and deterministic seeds.
- For every diagnostic, retain the first invalid operation and raw output, then reproduce with a second independent sanitizer or minimized test. A clean run is evidence only for the exercised cell.
- Check `valgrind` availability exactly and record the blocker if absent.

## Cycle 83 start

- Selected by the uber loop: exact `shuf -i 0-98 -n 1` -> `11` (`sanitizer-valgrind`).
- Branch: `uber-cycle-83-sanitizer-valgrind-20260728`.
- Cycle-start HEAD: `acc8a0388d` (`journal: close resource exhaustion cycle 82`).
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Scope: run ASan, UBSan, TSan, MSan, LeakSanitizer, Valgrind/Memcheck, and sanitized fuzz or recovery workloads where the local toolchain supports them; minimize the first invalid operation and classify tool, harness, dependency, and source failures separately.
- Exclusions: do not repeat cycle 78's TokenPipe EPIPE status contract or its uninstrumented MSan daemon boundary; cycle 26's broad sanitizer/static matrix; cycle 73's LevelDB construction leak; cycle 76's compact-block read-failure path; cycle 81's PSBT parser boundary; or cycle 82's locator allocation boundary without a distinct caller or first-invalid operation.
- Gate: tracked source was clean after cycle 82; known untracked agent artifacts and `test/cache` are preserved and excluded. Use `/data/my_storage/tmp` for all scratch data because the root filesystem is full. No relevant process was running at initialization.

## Hypotheses

1. A current unit, functional, fuzz, or recovery path has a sanitizer true positive that existing default builds or corpus replay do not exercise.
2. A deterministic multi-threaded P2P, worker, callback, or shutdown schedule exposes a TSan race or lifetime error outside the closed cycle-35 scheduler/transport cells.
3. An instrumentable core parser, persistence reader, wallet operation, or crypto-adjacent helper has an MSan, Memcheck, UBSan, or LeakSanitizer finding hidden by build exclusions, short workloads, or broad suppressions.
4. A `no_sanitize`, suppression, disabled target, compiler skip, or recover-mode setting is broader than its historical justification and masks a reachable diagnostic.

## Verification protocol

- Inventory sanitizer flags, suppressions, excluded targets, CI jobs, and historical rationale before interpreting output. Pin compiler/build-tree/tool versions and keep each sanitizer family in a separate clean build.
- Run the smallest reproducible unit, functional, fuzz, or recovery input first. Record raw command, seed/fixture, first invalid operation, stack, allocation/lifetime state, suppression path, and exit status. Never hide a failure with a timeout, catch, narrower input, or new suppression.
- For each report, reproduce under a second independent form when practical: another sanitizer, Valgrind/Memcheck, a minimized unit fixture, a fuzz replay, a temporary guard mutation, or a source/dataflow proof. Classify dependency/tool limitations explicitly.
- Fix only a confirmed local root cause. Keep one self-contained source/test change per finding, preserve the minimized regression, run narrow then broad validation, and update this journal before the source commit.

## Initial evidence queue

- Compare the current sanitizer configuration and suppressions with `doc/fuzzing.md`, CMake target registration, CI configuration, and recent sanitizer-related history.
- Rebuild or reuse isolated Clang 19 ASan/UBSan, TSan, and available MSan/LSan targets under `/data/my_storage/tmp`; run high-risk parser, persistence, wallet, P2P, and recovery suites with deterministic scratch directories.
- Check whether Valgrind/Memcheck is installed and usable on a small production path; if unavailable, retain the exact tool/version blocker rather than treating absence as a pass.
- Rank findings by trust boundary and first-invalid operation, then continue to the next distinct unchecked cell after each verdict.

## Cycle 83 inventory and evidence

### Configuration and suppression inventory

- `/usr/bin/clang-19` is Clang 19.1.7. `valgrind` is not installed. CMake reported `Could NOT find Valgrind` for the Bitcoin Core ASan/TSan fuzz builds and the standalone libsecp256k1 MSan build, so no Memcheck execution was possible.
- `build_fuzz_asan_clang19` was rebuilt with `SANITIZERS=fuzzer,address,undefined`; the build completed 19/19 actions. `build_fuzz_tsan_clang19` was rebuilt with `SANITIZERS=thread`; it completed 399/399 actions and is a standalone corpus runner. `build_fuzz_tsan_libfuzzer_clang19` was rebuilt with `SANITIZERS=fuzzer,thread`; it completed 401/401 actions and provides libFuzzer mutation runs.
- `build_unit_clang19` has no sanitizer, `build_unit_tsan_clang19` has `SANITIZERS=thread`, and the available fuzz ASan/UBSan build has wallet support disabled. No local `build_fuzz_msan` or full Bitcoin Core MSan build exists; the external dependency and libc shadow requirements make a complete Core MSan run an environment task rather than an untested source claim.
- `test/sanitizer_suppressions/tsan` has 34 lines covering documented test deadlocks, GUI/libzmq behavior, and issue-specific epoll/iostream cases. `valgrind.supp` has 54 lines of dependency/shutdown suppressions. `ubsan` has 71 specific dependency or intentional arithmetic entries, and `lsan` has 4 Qt dependency entries. No broad new suppression was added.
- Current `no_sanitize` sites are limited to the documented x86 SHA256 inline assembly address-sanitizer workaround, the old-Clang minisketch CLMUL memory-sanitizer workaround, and nanobench measurement arithmetic. No current production parser, wallet, network, or crypto path was found to have a broad sanitizer exclusion.
- `qa-assets` is not present locally. All scratch data and logs below are under `/data/my_storage/tmp/sanitizer-cycle83`; the root filesystem was full throughout the cycle.

### ASan, UBSan, and LSan fuzz evidence

The first default-limit probes used the current `build_fuzz_asan_clang19/bin/fuzz` binary with `ASAN_OPTIONS='detect_leaks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1'` and `UBSAN_OPTIONS='print_stacktrace=1:halt_on_error=1:report_error_type=1'`.

- `FUZZ=process_messages`, seed `8301`, 64 requested runs, and the default libFuzzer RSS limit of 2048 MiB exited 71 after 5 executions with a libFuzzer OOM at about 2227 MiB. The stack was fixture setup in `ValidationCache`/`ChainstateManager`; there was no ASan or UBSan report and no target-level invalid operation.
- `FUZZ=validation_block_reorg`, seed `8303`, with the same default limit had the same setup-time OOM classification after 5 executions. This was a harness RSS-limit failure, not a source finding.
- `FUZZ=dbwrapper_threaded`, seed `8302`, 64 runs completed with 8 new corpus units and no diagnostic.
- Replaying the two setup-heavy targets with `-rss_limit_mb=4096` removed the harness limit. The authoritative reruns are logged in `asan-process_messages-rerun.log` and `asan-validation-block-reorg-rerun.log`: `process_messages`, seed `8311`, 16 executions, coverage 36257, peak RSS 509 MiB; `validation_block_reorg`, seed `8312`, 16 executions, coverage 39275, peak RSS 599 MiB. Both added one corpus unit and exited 0 without ASan, UBSan, or LSan output.
- `dbwrapper_threaded`, seed `8313`, 16 executions, coverage 12216, peak RSS 260 MiB, exited 0 without a diagnostic. The log is `asan-dbwrapper-rerun.log`.

### TSan corpus and mutation evidence

- The standalone TSan corpus runner used `TSAN_OPTIONS='suppressions=/data/my_storage/bitcoin/test/sanitizer_suppressions/tsan:halt_on_error=1:second_deadlock_stack=1:history_size=7'`. Current corpora completed without a TSan report: `process_messages` 9 files, `dbwrapper_threaded` 7 files, `p2p_private_broadcast` 9 files, and `validation_block_reorg` 11 files. Logs are `tsan-process_messages.log`, `tsan-dbwrapper.log`, `tsan-p2p-private-broadcast.log`, and `tsan-validation-block-reorg.log`.
- The fresh `fuzzer,thread` build ran fixed-seed libFuzzer slices with `-rss_limit_mb=8192`: `process_messages` seed `8331`, 10 executions, 318 MiB peak RSS; `dbwrapper_threaded` seed `8332`, 8 executions, 128 MiB; `p2p_private_broadcast` seed `8333`, 10 executions, 311 MiB; and `validation_block_reorg` seed `8334`, 12 executions, 312 MiB. All exited 0 with no TSan report. Corresponding logs are under `/data/my_storage/tmp/sanitizer-cycle83/logs/tsan-libfuzzer-*.log`.
- A suppression-independent replay of `process_messages`, seed `8341`, 10 executions, peak RSS 318 MiB, also exited 0. The log is `tsan-nosupp-process_messages.log`; therefore this corpus did not demonstrate a diagnostic hidden by the repository suppression file.

### MSan libsecp256k1 evidence

- A separate Clang 19 build of `src/secp256k1` used `-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -O1 -g`, with normal tests enabled, exhaustive and ctime tests disabled, and no Valgrind. It built the shared library, `tests`, and `noverify_tests` successfully.
- The full `tests` binary started with seed `c6b09764d9f4133d495d368b63d9f33d`, but remained CPU-bound for 14:28 without a diagnostic and was interrupted with exit 130. This is inconclusive, not a pass or finding; the raw log is `msan-secp-tests.log`.
- The bounded command `MSAN_OPTIONS='halt_on_error=1:exit_code=86:report_umrs=1:print_stats=1' /data/my_storage/tmp/sanitizer-cycle83/msan-secp/bin/noverify_tests --iterations=1 --seed=83c3c0de --target=general --target=integer --target=hash --target=scalar --target=field --target=group --target=ecmult --target=ec --target=ecdh --target=ecdsa --target=extrakeys --target=schnorrsig --target=musig --target=ellswift --target=silentpayments --target=utils --log=1` completed all 16 modules in 176.119 seconds with exit 0 and no MSan report. The longest cases were `ecmult_multi_tests` at 102.248 seconds and Silent Payments vectors at 35.351 seconds. The complete output is `msan-secp-smoke.log`.

### Non-evidence command classifications

- The first two standalone TSan attempts passed libFuzzer flags to the non-libFuzzer `SANITIZERS=thread` executable; its custom main correctly treated `-runs=16` as an input and aborted before target execution. The corrected corpus-only invocations are the authoritative standalone TSan results above.
- One ASan DB wrapper command referenced a nonexistent scratch directory and exited before target execution. It was rerun against the existing corpus as seed `8313`; that rerun is the authoritative result.
- A zero-byte libFuzzer OOM artifact was moved from the repository root to the cycle scratch directory. It is retained as evidence and was not staged.

## Cycle 83 verdict

- No current Bitcoin Core or libsecp256k1 source defect was confirmed. ASan/UBSan/LSan, standalone TSan, TSan+libFuzzer, suppression-free TSan, and bounded libsecp256k1 MSan evidence produced no source report. The only negative default-limit results were fixture RSS-limit failures, and the full libsecp MSan run was explicitly inconclusive because it was interrupted.
- No source patch or regression test is warranted from this cycle. The next run should revisit the unchecked sanitizer cells with a complete MSan-capable Core dependency environment, installed Valgrind, qa-assets corpora, or a distinct recovery/functional workload; do not repeat these exact seeds and target/build combinations without a new trust boundary or first-invalid-operation hypothesis.
- Relevant command/log artifacts: `/data/my_storage/tmp/sanitizer-cycle83/logs/`; build trees: `build_fuzz_asan_clang19`, `build_fuzz_tsan_clang19`, `build_fuzz_tsan_libfuzzer_clang19`, and `/data/my_storage/tmp/sanitizer-cycle83/msan-secp`.

## Cycle 274 evidence

### Build and tool gate

- The exact source under test was HEAD `13938c14c47ca7ab00a3f552e5dcf8a2d7b179c6` (`uber-goal: record cycle 273 close`) on branch `uber-cycle-274-sanitizer-valgrind-20260802`. The tracked tree had only this journal edit; all pre-existing untracked agent artifacts were preserved.
- A new isolated build at `/data/my_storage/tmp/cycle274-asan-wallet` used `/usr/bin/clang-19` 19.1.7, CMake 3.25.1, Ninja 1.11.1, `RelWithDebInfo`, `-DSANITIZERS=address,undefined`, `-DENABLE_WALLET=ON`, `-DENABLE_IPC=OFF`, `-DBUILD_TESTS=ON`, and ccache under `/data/my_storage/tmp/cycle274-asan-ccache`. The build completed all `502/502` actions for `test_bitcoin`; `bitcoind` then linked in `4/4` actions.
- CMake's generated command inventory contains `-fsanitize=address,undefined`. Runs used strict unsuppressed `ASAN_OPTIONS='abort_on_error=1:halt_on_error=1:detect_leaks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:strict_string_checks=1:detect_odr_violation=2:print_scariness=1'`, `UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:report_error_type=1'`, and `LSAN_OPTIONS='report_objects=1:print_suppressions=1'`, with each log path isolated under `/data/my_storage/tmp`.
- Valgrind is unavailable: CMake reported `Could NOT find Valgrind`, and `command -v valgrind` returned no executable. No Memcheck claim is made. Existing suppression files remain unchanged: `test/sanitizer_suppressions/valgrind.supp` (54 lines), `ubsan` (71), `lsan` (4), and `tsan` (34). Current production `no_sanitize` sites remain the documented SHA256 assembly address-sanitizer workaround, nanobench measurement arithmetic, and the minisketch CLMUL memory-sanitizer workaround; no recent target was broadly excluded.

### Focused ASan, UBSan, and LSan evidence

- `test_bitcoin --run_test=minisketch_tests/minisketch_invalid_copy_assignment --random=274001` exited 0: 1 case, 3 assertions. This exercised the invalid-source assignment and destination clearing added by `301207370c`.
- `test_bitcoin --run_test=txgraph_tests/txgraph_memory_usage_allows_retained_empty_graph --random=274002` exited 0: 1 case, 2 assertions. This exercised retained allocation accounting after the last transaction removal from `3474cbeaf5`.
- `test_bitcoin --run_test=wallet_crypto_tests/passphrase_rounds_limit --random=274003` exited 0: 1 case, 3 assertions. This exercised zero, oversized, and sane KDF iteration counts from `879b3b7b17`.
- `test_bitcoin --run_test=scriptpubkeyman_tests/encrypt_descriptor_abort_preserves_state --random=274004` exited 0: 1 case, 7 assertions. This exercised the normal SQLite transaction-abort callback path from `9f15a43bd1`.
- The affected suite set `minisketch_tests,txgraph_tests,scriptpubkeyman_tests,wallet_crypto_tests,walletdb_tests` with seed `274005` exited 0: 58 cases, 11,519 assertions. The lifecycle/RPC unit set `wallet_tests,wallet_transaction_tests,walletdb_tests,rpc_tests` with seed `274007` exited 0: 52 cases, 591 assertions. These runs covered nearby error, cleanup, persistence, and restart-supporting branches rather than only the four new tests.
- None of the focused or broad logs contained `AddressSanitizer`, `UndefinedBehaviorSanitizer`, `runtime error:`, `LeakSanitizer`, `ERROR:`, or `SUMMARY:`. No ASan, UBSan, or LSan report files were created by the configured `log_path` values. All unit commands used scratch `TMPDIR` and separate `HOME` directories; no default wallet or datadir was used.

### RPC and persistence functional evidence

- `bitcoind` from the same sanitized build ran `test/functional/rpc_dumptxoutset.py` with Python random seed `274006`, `--tmpdir=/data/my_storage/tmp/cycle274-dumptxoutset-tmp`, and the scratch config `/data/my_storage/tmp/cycle274-functional-config.ini`. The test exited 0 and covered the latest snapshot, rollback snapshot, in-memory rollback, fork handling, existing/invalid paths, and unknown snapshot type. The retained datadir contains `txoutset.dat`, `txoutset_fork.dat`, and `txoutset_fork_mem.dat`; the node stopped cleanly.
- The functional log and retained node stdout/stderr contained no ASan, UBSan, LSan, or runtime diagnostic. The raw artifacts are `/data/my_storage/tmp/cycle274-dumptxoutset-asan.log`, `/data/my_storage/tmp/cycle274-dumptxoutset-tmp`, and `/data/my_storage/tmp/cycle274-asan-build.log`. Artifact hashes at capture were: `test_bitcoin` `fade5a4273d6812b67be8e03d4f81c82947bc097779de0a449e3a112fdad29a6`, `bitcoind` `18a675fef253f402147ef8886e706d649d3804f692477bd3af94eab50e868fe8`, and the functional log `6290d6bd2c215a04c5b9df14070aa7e5650e99f606446d3fa7d822db09761986`.

### Cycle 274 verdict and handoff

- No current Bitcoin Core or libsecp256k1 source defect was confirmed. The fresh ASan+UBSan+LSan unit cell and the ASan-instrumented `dumptxoutset` process path completed without a source diagnostic. The clean result is limited to the exercised current-source paths; it does not replace the missing Valgrind run or a Core-wide MSan run.
- No source patch or regression test is warranted from this cycle. The only journal change is this evidence and handoff snapshot. Do not repeat Cycle 83's exact fuzz targets/seeds, standalone TSan corpora, bounded secp MSan smoke, or default-limit OOM probes without a new trust boundary or first-invalid-operation hypothesis.
- Next queue, ranked: obtain an installed Valgrind/Memcheck environment and run a small current wallet/persistence path; construct a complete MSan-capable Core dependency build; then exercise a current-source TSan or sanitizer fuzz target using new wallet/database or crash-recovery schedules. Preserve the exact build and functional artifacts above for independent review.
