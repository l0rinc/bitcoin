# Fuzz Introspector Blocker and Complexity Audit

## Cycle 34 Selection and Gate

- Selected index: `50`
- Selected slug: `fuzz-introspector`
- Selected title: `Fuzz Introspector blocker and complexity audit`
- Selector: `shuf -i 0-98 -n 1`
- Selection timestamp: `2026-07-28T06:20:19Z`
- Catalog: `agent-goals/REUSABLE_AGENT_GOALS.md`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Gate HEAD: `f3964c7cea38f2f2a6183395f49a6d7d2825d1a8`
- Gate origin/master: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Gate merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Gate divergence: `origin/master...HEAD = 2 839`
- Gate state: tracked and staged files clean; agent/catalog artifacts and `test/cache/` remain untracked and were preserved; no relevant process was running.

## Scope and Hypothesis

Use static reachability, dynamic coverage, complexity, and blocker evidence to find a high-risk fuzz target whose official coverage run does not exercise its meaningful state machine. The primary candidate was `process_messages`, a P2P lifecycle target with a 30-iteration message loop and a current QA-assets directory containing zero files.

The hypothesis was that `CoverageFuzz.cmake` would invoke the generic runner with its default `-runs=1` empty-corpus behavior. Since `FuzzedDataProvider::ConsumeBool()` returns false on an empty buffer, the target would skip the random message loop and report only initialization/guided-slice coverage. The existing `test_runner.py --empty_min_time` mode and CI invocation suggested a small, intended configuration fix rather than a harness rewrite.

## Static Reachability and Complexity

- The combined fuzz binary listed 229 registered targets. `process_messages` is registered at `src/test/fuzz/process_messages.cpp:107` and initialized by `initialize_process_messages()`.
- The target's static path resets Connman/Chainman, replaces AddrMan and PeerManager, creates one to three peers, and dispatches through `ReceiveMsgFrom()`, `ProcessMessagesOnce()`, and `PeerManager::SendMessages()`. Its meaningful random state-machine edge is `LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 30)` at `src/test/fuzz/process_messages.cpp:155`.
- The empty input therefore reaches setup and later deterministic relay assertions but cannot enter the random message loop: there is no byte available for the first `ConsumeBool()`.
- The Clang 19 object inventory ranked this target as the largest of the inspected candidate bodies: `process_messages_fuzz_target` is `0x69b6` bytes in `process_messages.cpp.o`, compared with `validation_block_reorg_fuzz_target` at `0x818` and `coins_view_db_resize_cursor_fuzz_target` at `0x12f1`. The source sizes are 478, 755, and 718 lines respectively. `validation_block_reorg` has 2,257 QA inputs; `coins_view_db_resize_cursor` has no dedicated QA directory but its empty-input path still constructs one to 16 coins and one to four resizes, so it is a weaker corpus-blocker candidate.
- No Fuzz Introspector executable, Python module, or stored report was present. The object symbols, source call chain, target registry, corpus inventory, and libFuzzer counters are the manual static/dynamic substitute; the missing tool is recorded as a limitation, not treated as proof of unreachable production code.

## Reproduction and History

The current QA snapshot had `process_messages` at zero files while `process_message` had 3,696 files. Running the existing build's equivalent coverage command without empty-corpus fuzzing:

```text
build_fuzz_libfuzzer_clang19/test/fuzz/test_runner.py --loglevel DEBUG --par 1 /data/my_storage/tmp/qa-assets/fuzz_corpora process_messages
```

selected `-runs=1` with the empty directory and produced `#2 DONE`, `cov: 11470`, and two executions. The same target with the runner's existing empty-corpus mode:

```text
build_fuzz_libfuzzer_clang19/test/fuzz/test_runner.py --loglevel DEBUG --par 1 --empty_min_time=2 /data/my_storage/tmp/qa-assets/fuzz_corpora process_messages
```

selected `-max_total_time=2`, produced 161 executions, and reached `cov: 11482`. This is a bounded smoke comparison, not a claim that the 12-counter increase represents complete P2P coverage. The runner source explicitly switches from `-runs=1` to `-max_total_time` for empty directories at `test/fuzz/test_runner.py:338-345`.

The behavior is consistent with history. Commit `0000f552937ee787d25c8fd0af3278ea94889216` added the empty-corpus mode so CI could run targets without inputs, and the current CI script passes `--empty_min_time=60`. `cmake/script/CoverageFuzz.cmake` was added later and retained corpus replay plus `JOBS` support, but never passed that option. Thus the local coverage-report path and CI fuzz path disagree on the treatment of an empty high-risk corpus.

## Fix

`cmake/script/CoverageFuzz.cmake` now appends `--empty_min_time` with a default of 60 seconds to the runner command. The value is overrideable through `FUZZ_EMPTY_MIN_TIME`, preserving a bounded default while allowing resource-constrained or extended coverage jobs to choose their own interval. Existing nonempty corpora remain on the one-pass replay path; only missing/empty target directories use the time budget.

## Verification

- `cmake -S . -B build_fuzz_libfuzzer_clang19 -G Ninja`: passed and regenerated `build_fuzz_libfuzzer_clang19/CoverageFuzz.cmake` with `--empty_min_time=${FUZZ_EMPTY_MIN_TIME}` and the default 60 assignment.
- `python3 -m py_compile test/fuzz/test_runner.py`: passed.
- `git diff --check`: passed.
- The targeted runner smoke tests above passed with exit code 0; the empty-corpus run remained at `cov 11470`, while the bounded run reached `cov 11482`.
- A full `CoverageFuzz.cmake` execution was not run: it would launch all 229 targets and requires the local lcov/genhtml coverage toolchain. The source change is command construction only; no production or fuzz C++ code changed.

## Verdict

**Confirmed tooling/coverage blocker; fixed.** A high-risk stateful target with no QA inputs was reduced to an empty-input initialization pass by the coverage script even though the shared runner and CI already had a bounded empty-corpus mode. The patch restores dynamic fuzzing for that class of target without changing production behavior or claiming a complete coverage proof.

## Handoff

The source fix and this journal must be committed together as one self-contained change authored as `Lőrinc <pap.lorinc@gmail.com>`. Future cycles should search for other differences between CI fuzz invocation, local coverage scripts, corpus generation, and Fuzz Introspector/static reachability reports. The next run must perform a fresh gate and draw a distinct eligible goal; do not re-open this `process_messages` cell unless the empty-corpus policy or coverage script changes again.
