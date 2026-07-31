# Fuzz Introspector Blocker and Complexity Audit

## Cycle 195 start: current target reachability and blocker matrix

### Selection and fresh gate

- The exact selector was `shuf -i 0-98 -n 1 -> 50`; this is goal `fuzz-introspector`, titled `Fuzz Introspector blocker and complexity audit`. It is not the exact current Cycle 194 cell, so no reroll was required. Selection timestamp: `2026-07-31T07:08:44Z`.
- Dedicated branch: `uber-cycle-195-fuzz-introspector-20260731`. Start HEAD: `8fbc056d289d0d6c23ce4e2e656db545255f2698` (`uber-goal: record cycle 194 state`). `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `git rev-list --left-right --count origin/master...HEAD` reported `42 1180` (local divergence convention: `1180 42`). `git fetch origin master` completed successfully.
- The tracked worktree was clean, `git diff --check` passed, and protected PIDs `777094`, `956381`, and `1138182` remained alive. Root storage was `41M` free and `/data` had `46G` free; new artifacts must remain under `/data/my_storage/tmp`.
- Authoritative hashes at the gate were reusable catalog `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, random prompt `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, goals TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, and pre-cycle uber state `93e92d1453ddac54271777ba19d9a0d4daab172085f3a8d40468f3dd26d5ecde`.

### Scope and exclusions

Cycle 34 already fixed the exact `process_messages` empty-corpus `CoverageFuzz.cmake` policy gap by passing `--empty_min_time`; that cell is closed and will not be reopened. Cycle 188 fixed the separate `validation_load_mempool` realism gap with a deterministic valid transaction fixture, and Cycles 147/171/175 closed private-broadcast, process-message reset, and transport-engine cells. The current campaign excludes those exact target/corpus/harness hypotheses. It also excludes broad claims based only on the absent Fuzz Introspector executable: tool availability is a limitation, not a finding.

The new falsifiable question is: after the prior corpus and runner fixes, does the current target inventory still contain a high-risk production-backed fuzzer whose static reachability/complexity profile materially exceeds what its corpus or deterministic replay can exercise, and can a real blocker be removed without weakening the target oracle? The evidence map must distinguish a true target/harness blocker from a legitimate precondition, a stale binary, an intentionally empty corpus, or a production code path already covered by another target.

### Planned evidence and cycle queue

1. Rebuild or verify a current-head fuzz dispatcher under `/data`, enumerate all registered targets, source files, object/function sizes, target-specific corpora, and corpus byte/file distributions. Rank target risk by production trust boundary, statefulness, complexity, call depth, and corpus absence/weakness.
2. For the top distinct candidates, compare static source/control-flow reachability with fixed empty, structured, and available QA/cross-target corpus replays. Preserve libFuzzer counters, LLVM coverage where available, target logs, peak RSS, and first blocker conditions. Do not treat `cov` or function size as comparable across targets without recording instrumentation and build settings.
3. Inspect the highest-value blocker in source/history and formulate the expected invariant before changing code. Use a minimal scratch harness, a temporary mutation, a targeted corpus, or a focused profile to prove that the proposed unblocker reaches meaningful production behavior and that its oracle detects a relevant fault.

Every candidate needs a verdict of confirmed, dismissed, or inconclusive. A harness change requires before/after reachability or coverage evidence plus an independent sanitized replay and mutation/oracle sensitivity where practical. A production change requires a failing-before regression and a narrow permanent test. Keep this cycle separate from the already-fixed empty-corpus policy and `validation_load_mempool` fixture changes.

## Cycle 195 result: current-head complexity and official-corpus verification

### Build, inventory, and evidence boundary

- The current dispatcher was rebuilt in `/data/my_storage/tmp/cycle131-build-libfuzzer` after the fresh gate. The binary SHA256 was `70994bd265fcb613ecb15e72985ed00b6733febda31185a0ce7343dddf7e2526`; CMake reported `BUILD_FOR_FUZZING=ON` with `address,undefined,fuzzer` sanitizers. The dispatcher registered 229 visible targets.
- Fuzz Introspector is not installed in this environment: `command -v fuzz-introspector` returned no path and importing `fuzz_introspector` raised `ModuleNotFoundError`. This is recorded as an external tooling limitation, not as a product finding. Static fallback evidence is the current object symbol size, source span, target edge budget from `-print_coverage=1`, callback/lambda reachability, and production-history risk.
- The local `/data/my_storage/qa-assets-sparse` checkout is intentionally sparse and only materializes four target directories. Its `origin/main` tree at `0772287676fdf3fcf87631b383b12442ab48ce75` nevertheless contains current entries for `txgraph`, `package_rbf`, `ephemeral_package_eval`, `tx_pool`, `tx_pool_standard`, `txdownloadman`, `txdownloadman_impl`, and `p2p_headers_presync`. Therefore local absence was not evidence of an upstream missing corpus.
- Remote tree counts and bytes from `git ls-tree -rl origin/main:fuzz_corpora` were: `txgraph` 4,214 files/1,925,069 bytes; `package_rbf` 1,111/93,668,976; `ephemeral_package_eval` 2,098/12,563,584; `tx_pool` 7,318 files; `tx_pool_standard` 2,857; `txdownloadman` 1,478; `txdownloadman_impl` 1,539; and `p2p_headers_presync` 726. The current `txgraph` corpus was archived into `/data/my_storage/tmp/cycle195-candidates/qa-current-txgraph` for isolated replay.

### Static risk ranking

| Target | Source span | Fuzz-target text size | Target edges | Corpus evidence |
| --- | ---: | ---: | ---: | --- |
| `txgraph` | `src/test/fuzz/txgraph.cpp`, 1,682 lines | 194,454 bytes (`0x2f796`) | 2,307 | 4,214 current upstream inputs |
| `partially_downloaded_block` | `src/test/fuzz/partially_downloaded_block.cpp`, 1,011 lines | 222,244 bytes (`0x36424`) | not sampled this cycle | 1,109 local sparse inputs |
| `package_rbf` | `src/test/fuzz/rbf.cpp`, 371 lines | 33,848 bytes (`0x8438`) | 388 | 1,111 current upstream inputs |
| `tx_pool_standard` | `src/test/fuzz/tx_pool.cpp`, 1,122 lines | 34,860 bytes (`0x882c`) | not sampled this cycle | 2,857 current upstream inputs |
| `ephemeral_package_eval` | `src/test/fuzz/package_eval.cpp`, 992 lines | 29,176 bytes (`0x71f8`) | 350 | 2,098 current upstream inputs |

`txgraph` is the highest-value distinct cell: `partially_downloaded_block` has the larger raw function body, but `txgraph` has the largest measured edge budget among the sampled stateful candidates, multiple local callbacks (`$_0` through `$_19` in the instrumented report), and calls through `TxGraph`, `cluster_linearize`, `SpanningForestState`, and block-builder paths. History shows recent correctness-sensitive changes on 2026-07-25 (`fb7bfd05ac`, `txgraph: canonicalize saturated chunk fee aggregation`), 2026-07-22 (`367c0823cc`, full-range fee saturation fuzzing), and several preceding TxGraph contract checks. This makes it a high-risk complexity target, but size alone is not a defect signal.

### Reachability and dynamic coverage probes

- Empty-start control: `FUZZ=txgraph .../fuzz empty-txgraph-5s -max_total_time=5 -seed=19518` completed with `cov 5320`, 322 executed units, five tiny retained corpus units, and no failure. A target-specific replay of that result reported only `60/2307` `txgraph_fuzz_target` edges; the operation callbacks at lines 399, 435, 533, 539, 558, and 573 remained uncovered.
- Structured reachability control: a 64-byte all-zero input, SHA256 `f5a5fd42d16a20302798ef6ed309979b43003d2320d9f0e8ea9831a92759fb4b`, replayed under the same sanitized binary with `-runs=1` and `-print_coverage=1`. It reached `196/2307` target edges, `cov 11221`, and callbacks for simulated graph comparison, level handling, input-set handling, and comparator paths. Three fixed-seed replays completed without sanitizer or assertion failures; overall counters varied by only a few process-level edges (`cov 11218`, `11218`, and `11221`).
- Current upstream corpus sample: the first 100 names in the archived remote corpus were replayed with `-seed=19525`. The 100 files were 4 to 1,732 bytes, 41,167 bytes total; the run completed in 14 seconds with `cov 31729`, and target-specific coverage was `1039/2307` edges. It exercised the main target 100 times, the operation callbacks at lines 399, 435, 458, 489, 499, 533, 539, 544, 552, 558, 564, 573, and 583, and the `SpanningForestState` linearization path. No sanitizer, assertion, or harness failure occurred.
- Full current upstream corpus: `FUZZ=txgraph .../fuzz qa-current-txgraph/fuzz_corpora/txgraph -runs=1 -seed=19524 -rss_limit_mb=1536 -timeout=5 -print_final_stats=1` completed with exit code 0. The captured log `/data/my_storage/tmp/cycle195-txgraph-qa-full.log` records 4,214 seed files, 5,217 executed units, final `cov 33308`, `ft 218785`, retained corpus `1780/555Kb`, 691 MiB peak RSS, and 14 executions per second over 351 seconds. There were no ASan, UBSan, assertion, or process failures. LibFuzzer disabled leak detection after mutation because this stateful target intentionally accumulates graph allocations during the run; that diagnostic is not a leak verdict.
- Adjacent controls were not the highest-value gap. `package_rbf` has 388 target edges and its five-second empty run reached `88/388`; a valid transaction-prefix fixture and a controlled escaped suffix reached package/RBF helpers and overall `cov 20691`, while 500 mutations completed at `cov 21563`. `ephemeral_package_eval` has 350 target edges; its empty five-second run reached `136/350` plus package-generation callbacks, so it did not show the same initialization blocker.

### Independent assessment and verdicts

1. **Upstream missing-corpus hypothesis: dismissed.** The sparse local checkout was misleading; the current remote tree contains substantial corpora for every candidate checked. No source-side corpus fallback or artificial input path is justified.
2. **`txgraph` empty-start reachability hypothesis: confirmed as an operational weakness, not a Bitcoin Core defect.** Empty-start fuzzing spends its short initial search in configuration/basic paths (`60/2307` edges), while a trivial 64-byte fixture and the current upstream corpus unlock graph operations. The existing `CoverageFuzz.cmake` policy already gives genuinely empty targets a bounded run; changing production oracles to manufacture state would be riskier than obtaining the maintained QA corpus.
3. **`txgraph` current corpus ineffectiveness hypothesis: dismissed.** The official sample covers 45% of the target edge budget and reaches the deep callback families; the full sanitized replay is long and memory-intensive but completed cleanly. The remaining uncovered edges require a more specific target-gap analysis, not a proven harness blocker.
4. **Fuzz Introspector call-depth blocker: external/inconclusive.** The named tool and Python module are unavailable, so a true Fuzz Introspector report could not be produced. The fallback static/dynamic matrix is sufficient to avoid a speculative source change, but the next pass should acquire the tool or generate equivalent LLVM call-depth data before claiming closure of the entire target inventory.

No Bitcoin Core source or test change is committed in this cycle. The strongest actionable handoff is the current QA corpus provenance, the 64-byte deterministic witness, the official 100-input edge report, and the full replay log. A source patch would either duplicate the external QA corpus or weaken the target's production-backed state machine. Cycle verdict: **journal-only, no confirmed repository defect; continue with a tool-enabled call-depth and full target-gap matrix in a later distinct cycle.**

### Reproduction ledger and next queue

- Build: `/data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz`; runtime scratch: `/data/my_storage/tmp/cycle195-fuzz-runtime`; all data and logs: `/data/my_storage/tmp/cycle195-candidates` and `/data/my_storage/tmp/cycle195-*.log`.
- Key reports: `/data/my_storage/tmp/cycle195-package-empty5-coverage.txt`, `/data/my_storage/tmp/cycle195-package-loop-coverage.txt`, `/data/my_storage/tmp/cycle195-txgraph-64-zero-coverage.txt`, `/data/my_storage/tmp/cycle195-txgraph-sample-coverage.log`, and `/data/my_storage/tmp/cycle195-txgraph-qa-full.log`.
- Next queue: install or otherwise provision Fuzz Introspector; produce a source-level static call-depth report for the 229 targets; compare target-specific edge/function coverage for the full current QA corpus rather than only the 100-input sample; then examine the highest-risk uncovered callback with a mutation-sensitive oracle. Preserve the QA ref and do not repeat the stale sparse-checkout absence hypothesis.

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
