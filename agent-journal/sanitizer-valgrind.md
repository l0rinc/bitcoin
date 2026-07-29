# Sanitizer and Valgrind True-Positive Sweep

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
