# Fuzz-engine and property-framework differential

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
