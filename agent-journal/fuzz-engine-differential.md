# Fuzz-engine and property-framework differential

## Cycle 131: descriptor_parse engine and toolchain comparison

### Selection and fresh gate

- Selector: exact `shuf -i 0-98 -n 1` -> `80` (`fuzz-engine-differential`); no reroll was needed because the previous cycle was goal 7.
- Branch: `uber-cycle-131-fuzz-engine-differential-20260730`.
- Gate HEAD: `b1f32f6fc4263e46b92998b447fe64e5502eeabf`.
- Gate `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `1048 40`.
- Tracked state was clean, `git diff --check` passed, all catalog/protocol hashes matched the authoritative values, and PID 777094 was preserved. Known unrelated untracked artifacts remain outside the cycle scope.
- Catalog hashes: reusable goals `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, random prompt `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, goals TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, uber protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

### Scope and exclusions

Cycle 8 already compared `bech32_roundtrip` under libFuzzer, AFL++, and Honggfuzz. Cycle 79 compared `parse_numbers` with the same three engines and cross-engine sanitizer replay. Those target/corpus/result cells remain closed. FuzzTest is still not integrated in the repository and was not found locally; it will be recorded as unavailable rather than represented by another framework.

The distinct target for this cycle is `descriptor_parse`, which prior evidence queued but did not execute in an engine differential. Its production harness parses each input both with and without a required checksum, asserts that equivalent successful results serialize identically, expands descriptors, checks inferred scripts, and applies range/solvability/key-count invariants. The cycle hypothesis is that engine instrumentation or input scheduling may expose a parser-state, corpus-transfer, crash, timeout, or harness-realism difference that the earlier parser targets did not show.

The comparison will use one shared deterministic seed corpus, identical target selection and process limits, Clang 19 libFuzzer with sanitizer replay, AFL++ persistent mode, and Honggfuzz feedback mode. Native coverage counters will remain engine-specific; only transferred inputs, exit status, sanitizer diagnostics, crashes, hangs, and target output will be treated as cross-engine evidence. All builds and runs use scratch directories and no default datadir, wallet, key, or database.

### Cycle 131 build, corpus, and tool matrix

- The source corpus was /data/my_storage/tmp/cycle131-corpus/source: 20 text seeds, 2,269 bytes total, manifest SHA256 d5f4508c15750de5d11719d278eb8129a0741d8842b7658cb685d8daca87ae4a. It covered valid and invalid public descriptors, ranged and multipath extended keys, multisig, miniscript, taproot, checksum, bounds, truncation, unknown syntax, and empty-input cases. The same copied 20-file corpus was supplied to each engine.
- All builds used current source HEAD, Clang 19.1.7, Debug, BUILD_FOR_FUZZING=ON, wallet/IPC disabled, x86_64, FUZZ=descriptor_parse, and isolated /data/my_storage/tmp/cycle131-* paths. The libFuzzer build used ASan+UBSan+libFuzzer. AFL++ and Honggfuzz throughput builds were unsanitized and every generated input was replayed through the ASan+UBSan libFuzzer oracle.
- The libFuzzer target built in /data/my_storage/tmp/cycle131-build-libfuzzer. The AFL++ target built in /data/my_storage/tmp/cycle131-build-afl19d with AFL++ commit ad5304010ae3be9d5cdc1ba51b09e14169c5cb87, PCGUARD, -fno-lto, and LLVM 19 lld; afl-showmap independently captured 575 tuples from a seed and reported a 300,555-edge target map. The Honggfuzz target built in /data/my_storage/tmp/cycle131-build-honggfuzz19 with Honggfuzz commit cf8b66a4d09f4d4d786d96e3c46d9141fb4e98e2.
- The first AFL++ launch placed env FUZZ=descriptor_parse after --, so AFL++ checked /usr/bin/env and rejected it as uninstrumented. The corrected launch inherited FUZZ before the fuzzer command and used the instrumented Bitcoin target. This was a scratch invocation error and is excluded from product evidence. Earlier AFL link attempts using host LLVM 14 and the project's -fcf-protection=full flag were also quarantined as scratch toolchain integration failures; the successful build filtered that wrapper-detected flag and used matching LLVM 19 lld.
- FuzzTest was not installed and no repository integration was found. No substitute property framework was counted as a FuzzTest result.

### Cycle 131 fixed-budget comparison

Each engine used one worker and a 15-second budget. Native counters are engine-specific and are not comparable as equal coverage units.

| Engine | Executions | Native coverage/corpus signal | Peak RSS | Crashes/hangs |
|---|---:|---|---:|---|
| libFuzzer, seed 13101 | 36,935 | 799 new units; 1,900 final coverage; 16 retained corpus files | 357 MiB | 0/0 |
| AFL++ 5.03a | 233,494 | 2,668 edges; 0.89% bitmap; 98.39% stability; 470 new queue entries | 22 MiB | 0/0 |
| Honggfuzz 2.6, one thread | 61,163 | 290 new units; 325 .honggfuzz.cov outputs; 300,358 guards; branch metric reported 0% | 30 MiB | 0/0 |
| libFuzzer, seed 13102 | 36,935 | 325 new units; 8,537 final coverage; 280-entry in-memory corpus | 352 MiB | 0/0 |

The libFuzzer corpus-only control loaded all 20 seeds and completed 21 executions with peak RSS 242 MiB. The fixed-budget runs used -rss_limit_mb=4096, -max_len=4096, and ASan/UBSan halt-on-error settings. Honggfuzz ended with crashes_count:0, timeout_count:0, and new_units_added:290. AFL++ ended with saved_crashes:0, saved_hangs:0, 233,494 executions, and 15,549.68 executions/sec. Its native queue is under /data/my_storage/tmp/cycle131-runs/afl-out2/cycle131/; Honggfuzz's generated corpus is under /data/my_storage/tmp/cycle131-runs/hong-out/.

### Cycle 131 transfer and sanitizer verification

- The ASan/UBSan libFuzzer replay consumed 490 AFL++ queue files and 325 Honggfuzz .cov files, 815 files total, and completed 816 executions with exit status 0. It reported no sanitizer diagnostic, assertion, crash artifact, or timeout; peak RSS was 265 MiB. Log: /data/my_storage/tmp/cycle131-runs/cross-replay.log.
- The initial libFuzzer replay of all 20 seeds exited 0. Both fixed-seed libFuzzer runs exited 0, as did the corrected AFL++ and Honggfuzz sessions. Logs are /data/my_storage/tmp/cycle131-runs/libfuzzer-13101.log, /data/my_storage/tmp/cycle131-runs/libfuzzer-13102.log, /data/my_storage/tmp/cycle131-runs/afl2.log, and /data/my_storage/tmp/cycle131-runs/hong.log.
- AFL++'s crashes and hangs directories contain zero files. Honggfuzz emitted 325 coverage corpus files and no crash or timeout files. The two crash/timeout text matches in its log are its startup configuration line and final zero-valued summary, not failures.

### Cycle 131 verdict

The bounded hypothesis is dismissed for a new repository defect. The distinct descriptor_parse target completed current-head libFuzzer, AFL++, and Honggfuzz runs from the same corpus, and all engine-produced inputs survived ASan/UBSan replay. Coverage and corpus-growth differences are expected consequences of each engine's mutation, persistence, shared-memory, instrumentation, and scheduling model. No crash, hang, sanitizer failure, corpus corruption, or target semantic mismatch was reproduced. No production or test change is justified.

### Cycle 131 handoff

- Close this cycle with journal/state-only commits; no source finding was confirmed.
- Preserve the corpus manifest, tool commits, corrected AFL invocation, quarantined scratch-toolchain failures, native engine metrics, and cross-engine replay log above.
- The broader goal remains eligible for a future distinct target or compiler/engine/toolchain evidence cell; do not repeat bech32_roundtrip, parse_numbers, or this descriptor_parse cell without new evidence.

## Cycle 8

- Selector: `shuf -i 0-98 -n 1`
- Draw: `80`
- Selected goal: `fuzz-engine-differential`
- Date: 2026-07-27 UTC
- Repository HEAD: `1dcc2da988ee625fbc5d7d55eb6f894c1103ec52`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Working tree: unchanged tracked files; only agent-owned untracked directories are present

## Hypothesis

Using different fuzz engines on the same production target and fixed qa-assets corpus could expose an engine-specific harness gap, coverage-preservation problem, or reproducible crash missed by the other engines. The selected target was `bech32_roundtrip`, chosen because it is a current production parser/round-trip harness with a pinned local corpus of 75 files totaling 304 KiB.

## Build and tool matrix

All scratch builds used the current source tree, Clang 19.1.7, Debug, `BUILD_FOR_FUZZING=ON`, wallet/IPC/ZMQ/Qt disabled, x86_64 assembly enabled, and `SANITIZERS=` for throughput comparability. Scratch state is under `/data/my_storage/tmp/fuzz-engine-differential/`.

| Engine | Build and instrumentation | Result |
|---|---|---|
| libFuzzer | Existing `/data/my_storage/bitcoin/build_fuzz_libfuzzer_clang19/bin/fuzz`; `SANITIZERS=fuzzer` | Built target identity points at this source tree and Clang 19 |
| AFL++ 5.03a | Fresh `build_afl19b`; AFL LLVM mode through `afl-clang-fast++`; matching LLVM 19 `lld` used because host `gold` is LLVM 14 | 452/452 build steps passed and binary contains AFL persistent-loop symbols |
| Honggfuzz 2.6 | Fresh `build_honggfuzz19`; `hfuzz-clang++` PCGuard/trace coverage wrapper; matching scratch BFD/libunwind development headers and explicit archive paths | 452/452 build steps passed; CMake compiler probe and target link passed |
| FuzzTest | Searched the repository and installed paths | No project integration or installed FuzzTest package was found; no fair FuzzTest run was possible |

The AFL `gold` failure and Honggfuzz missing host development headers were toolchain issues resolved entirely in scratch state. They did not involve Bitcoin Core source changes. The engine coverage values below are engine-specific counters and must not be treated as directly equivalent percentages.

## Fixed-budget runs

The same 75 files from `/data/my_storage/tmp/qa-assets/fuzz_corpora/bech32_roundtrip` were copied into isolated run directories. Each engine was given a 15-second budget, one worker, and no default datadir, wallet, key, or database.

| Engine/run | Executions | Coverage signal | Corpus signal | Memory | Failures |
|---|---:|---|---|---:|---|
| AFL++ repeat 1 | 110,577 | 919 edges; 0.31% bitmap; 100.00% stability | 225 entries, 150 found | 16 MiB | 0 crashes, 0 hangs |
| AFL++ fixed seed `80` | 100,950 | 919 edges; 0.31% bitmap; 100.00% stability | 232 entries, 157 found | 16 MiB | 0 crashes, 0 hangs |
| libFuzzer seed `80` | 72,565 | 898 coverage units | 506 new units; final corpus 219 entries / 10,166 bytes | 59 MiB | 0 crash artifacts |
| Honggfuzz | 18,740 | 305,081 guards; 32 new units; branch metric reported 0% | Dynamic coverage files emitted | 31 MiB | 0 crashes, 0 timeouts |

The libFuzzer run completed in 16 seconds including startup, and Honggfuzz in 16 seconds including its corpus/dry-run phases. Honggfuzz exposes no deterministic mutation-seed option in this build, so its run is fixed-input-corpus evidence but not a byte-for-byte deterministic engine replay. AFL++ repeated the same 919-edge result with different corpus/throughput outcomes; this is normal engine scheduling variation, not a target divergence.

## Evidence and verification

- AFL++ loaded all 75 seeds, detected persistent mode, completed its dry run, and reported no crashes or hangs. Raw stats: `run-afl-bech32-01/outputs/default/fuzzer_stats` and `run-afl-bech32-02/outputs/default/fuzzer_stats`.
- libFuzzer loaded the copied corpus, reached `DONE`, and reported `stat::number_of_executed_units: 72565`, `stat::new_units_added: 506`, and no error or artifact. Raw log: `logs/libfuzzer_bech32_02.log`.
- Honggfuzz loaded the copied corpus, entered feedback-driven mode, and ended with `crashes_count:0`, `timeout_count:0`, `new_units_added:32`. Raw log: `logs/honggfuzz_bech32_01.hf.log`.
- The AFL and Honggfuzz binaries both enumerate the current fuzz dispatcher and the `bech32_roundtrip` target. The three builds exercise the same source target and 75 initial files; only the engine runtime/instrumentation differs.
- A process gate after the runs found no active Bitcoin, fuzz, sanitizer, profiler, or build workload.

## Verdict

The bounded hypothesis is **dismissed**: no engine-specific crash, hang, corpus corruption, or reproducible target behavior difference was found. The metric differences are expected consequences of persistent shared-memory AFL execution, libFuzzer's mutation/corpus model, and Honggfuzz's runtime/instrumentation model. There is no confirmed repository finding and no production change to commit.

## Limitations and next evidence

- This was one parser/round-trip target and one short budget. Coverage counters are not normalized across engines.
- FuzzTest could not be compared because neither repository integration nor a local installation exists.
- Honggfuzz's engine RNG is not exposed as a fixed seed in the selected release; repeat comparison would require a controlled random source or accepting statistical variation.
- A future fresh cycle can compare `process_message`, `parse_numbers`, or `descriptor_parse` with sanitizer builds, shared minimized corpora, and normalized coverage extraction. Do not treat this target as exhausted.

## Artifacts

- AFL++ build: `/data/my_storage/tmp/fuzz-engine-differential/build_afl19b/`
- Honggfuzz build: `/data/my_storage/tmp/fuzz-engine-differential/build_honggfuzz19/`
- AFL runs: `/data/my_storage/tmp/fuzz-engine-differential/run-afl-bech32-01/`, `/data/my_storage/tmp/fuzz-engine-differential/run-afl-bech32-02/`
- libFuzzer run: `/data/my_storage/tmp/fuzz-engine-differential/run-libfuzzer-bech32-02/`
- Honggfuzz run: `/data/my_storage/tmp/fuzz-engine-differential/run-honggfuzz-bech32-01/`
- Logs: `/data/my_storage/tmp/fuzz-engine-differential/logs/`

## Cycle 79: distinct parse_numbers engine comparison

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `80` (`fuzz-engine-differential`).
- Branch: `uber-cycle-79-fuzz-engine-differential-20260728`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at cycle start: `42303632902bb1fb2c423b87be1c3bb2ce059bef` (`journal: close sanitizer-analysis cycle 78`).
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: `git fetch origin master --quiet` passed; `origin/master...HEAD` was `2 938`; tracked source was clean apart from known agent-owned artifacts and `test/cache`; no relevant process was running.

### Scope and exclusions

The existing Cycle 8 section in this journal compared libFuzzer, AFL++, and Honggfuzz on `bech32_roundtrip`; that exact target, corpus, and result set are excluded. FuzzTest is still absent from the repository and installed paths. This cycle uses the distinct `parse_numbers` production parser target and its 220-file, 5.2 MiB qa-assets corpus. It will build current HEAD in isolated `/data` directories, use one shared copied corpus and fixed wall-clock budgets, and retain each engine's native coverage/corpus signal without pretending the counters are directly equivalent.

### Hypothesis

Different engines may reach different parser states or preserve different boundary inputs on the same `parse_numbers` target, exposing a harness/engine adapter gap, a corpus transfer loss, or a reproducible crash/hang. Compare libFuzzer, AFL++, and Honggfuzz with identical initial inputs, target arguments, one worker, and sanitizer replay for every failure. Record throughput, coverage signal, corpus growth, memory, crashes, hangs, determinism, and unsupported features. Do not modify production code for metric differences alone.

### Cycle 79 build and tool matrix

- The initial source corpus was `/data/my_storage/tmp/qa-assets/fuzz_corpora/parse_numbers`: 220 files, 4,568,572 bytes, and manifest SHA256 `79d0fa63f4272d5490e240e3f626ec57f5fd094b3058720c4c2a272286de0253`. The AFL++ and Honggfuzz input copies matched this manifest before mutation; libFuzzer's input directory retained new units during its run, so its post-run directory is reported separately.
- All throughput builds used current HEAD `4230363290` or later journal-only state, Clang 19.1.7, Debug, `BUILD_FOR_FUZZING=ON`, wallet/IPC/ZMQ/Qt disabled, x86_64, and `FUZZ=parse_numbers`. Each engine used one worker and no default datadir, wallet, key, or database.
- The current-head libFuzzer target rebuilt with `ninja -C build_fuzz_libfuzzer_clang19 fuzz -j2` (20/20). The current-head AFL++ target rebuilt with `ninja -C /data/my_storage/tmp/fuzz-engine-differential/build_afl19b fuzz -j2` (186/186), using `afl-clang-fast++` and `lld-19`; host LLVM 14 gold-plugin warnings were non-fatal static-library diagnostics.
- Honggfuzz initially compiled current-head objects with `HFUZZ_CC_PATH=/usr/bin/clang-19` and `HFUZZ_CXX_PATH=/usr/bin/clang++-19`, but its final link exposed three unresolved `nsEnter`/`nsIfaceUp`/`nsMountTmpfs` references from a stale embedded `libhfcommon` archive. The verified build-tree archives were supplied through `HFUZZ_LHFUZZ_PATH`, `HFUZZ_LHFCOMMON_PATH`, and `HFUZZ_LHFNETDRIVER_PATH`; the target then linked successfully. This was an external scratch-toolchain issue, not a Bitcoin source result.
- FuzzTest was searched in the repository and installed paths and remains unavailable; no substitute property framework was silently counted as a FuzzTest run.

### Cycle 79 fixed-budget comparison

The same original 220-file corpus was used for each first run. Native counters are engine-specific and are not comparable as equal coverage units.

| Engine | Executions | Throughput | Native coverage/corpus signal | Peak RSS | Crashes/hangs |
|---|---:|---:|---|---:|---|
| libFuzzer, seed `7901`, 15 seconds | 6,110 | 381/sec | 920 coverage units, 45 new units; 261 files remained on disk after the run | 63 MiB | 0/0 |
| AFL++ 5.03a, seed `7901`, 15 seconds | 61,202 | 4,074.70/sec | 958 edges, 0.32% bitmap, 100% stability; 248 queue entries and 28 found | 21 MiB | 0/0 |
| Honggfuzz 2.6, one thread, 15 seconds | 5,839 | 364/sec | 305,559 guards, 52 new units; 202 `.cov` inputs emitted; branch metric reported 0% | 0 MiB reported | 0/0 |

The libFuzzer log's final in-memory minimized corpus was 149 entries; its retained input directory was 261 files because the engine appended new files to the copied directory. AFL++ raw stats are in `/data/my_storage/tmp/fuzz-engine-differential-cycle79/runs/afl/default/fuzzer_stats`; libFuzzer and Honggfuzz raw logs are `runs/libfuzzer/run.log` and `runs/honggfuzz/run.log` under the same cycle directory. Honggfuzz exposes no fixed mutation-seed option in this build, so its result is fixed-corpus evidence but not a byte-for-byte deterministic replay.

The first libFuzzer `-runs=1` corpus-only control hit its default 2,048 MiB RSS limit after 220 seeds, using 2,141 MiB and exiting 71. The preserved artifact is `runs/libfuzzer/oom-corpus-limit`. Repeating the full corpus with `-rss_limit_mb=8192` and the fixed 15-second budget completed normally at 63 MiB peak RSS. This is a libFuzzer resource-guard sensitivity, not a parser failure.

### Cycle 79 transfer and sanitizer verification

- Current Clang 19 ASan replay completed all retained libFuzzer inputs (`INFO: 261 files found`, runner reported 262 executions), all 249 AFL++ queue inputs, and all 202 Honggfuzz `.cov` inputs. The exact logs are `runs/asan-libfuzzer/replay.log`, `runs/asan-afl/replay.log`, and `runs/asan-hong-cov/replay.log`; none contains an ASan error, UBSan-style assertion, timeout, or crash artifact.
- A short cross-engine transfer consumed 249 AFL++ queue inputs in libFuzzer for 4,068 executions, 21 new units, and 64 MiB peak RSS without failure. AFL++ consumed 202 Honggfuzz `.cov` inputs and found 38 new queue items without crashes or hangs. Honggfuzz's five-second transfer control ended during dry-run before feedback counters initialized and is excluded from comparison; the repeated 15-second transfer consumed 249 AFL++ inputs for 4,202 iterations, 34 new units, and 305,559 guards without crashes or timeouts.
- The stale-build attempts made while the current-head AFL++/Honggfuzz links were still running are quarantined under `runs/afl-stale-build` and `runs/honggfuzz-stale-build`; none is used as evidence.

### Cycle 79 verdict

The hypothesis is **dismissed** for a new repository defect. Current-head libFuzzer, AFL++, and Honggfuzz all exercised the same production parser and initial corpus; their native coverage and corpus-growth differences are expected engine behavior, not a semantic divergence. No crash, hang, sanitizer failure, corpus corruption, or target-state mismatch survived cross-engine transfer and ASan replay. No production or test change is justified. This parser cell is closed, but the broader `fuzz-engine-differential` goal remains eligible for a future distinct target or engine/toolchain evidence cell.

### Cycle 79 handoff

- Close the cycle with a journal/state-only commit; no source finding was confirmed.
- Preserve the initial corpus manifest, current-head build commands, engine-specific metrics, Honggfuzz archive override, stale-build exclusion, default-RSS OOM control, and all ASan/transfer logs above.
- Next cycle must re-check the repository gate and draw a distinct eligible goal from all 99 catalog rows. Do not repeat cycle 8's `bech32_roundtrip` comparison or this cycle's `parse_numbers` cell without new compiler, engine, corpus, or regression evidence.
