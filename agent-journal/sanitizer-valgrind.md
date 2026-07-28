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
