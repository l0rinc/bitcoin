# Fuzz-engine and property-framework differential

## Cycle 175 start: transport serialization engine comparison

### Selection and fresh gate

- The exact post-Cycle-174 selector was `shuf -i 0-98 -n 1` -> `80`
  (`fuzz-engine-differential`). The draw was retained because the previous
  Goal-80 cells are target-specific and this cycle selects the distinct
  transport serialization family; no closed target cell was rerolled.
- Branch: `uber-cycle-175-fuzz-engine-differential-20260730`.
- Start HEAD: `f71a04625cb23d54825a5d80a08b8cf729f6175c`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence:
  `origin/master...HEAD = 42 1132`.
- The fresh gate passed the post-close origin fetch, tracked/index diff
  checks, catalog/prompt/TSV/protocol hash checks, and persistent-process
  check. PIDs `777094` and `956381` are unrelated long-running tests and must
  not be stopped. Known untracked agent/user artifacts remain preserved. The
  root filesystem has about 102 MiB free; all new build, corpus, and log data
  must stay under `/data/my_storage/tmp` and broad builds are out of scope.

### Scope and exclusions

Cycles 8, 79, and 131 closed the exact `bech32_roundtrip`, `parse_numbers`,
and `descriptor_parse` engine/corpus cells. Cycle 171 closed the persistent
`process_messages`/`process_message` reset cell after fixing mocked-socket and
mempool state leakage. Cycle 139 used a single current-head
`p2p_transport_serialization` sanitizer replay as part of a network-state
campaign, but it did not compare libFuzzer, AFL++, and Honggfuzz on a shared
transport corpus; that replay is evidence for seed reuse only, not a closed
engine-differential cell. FuzzTest remains unavailable locally and has no
repository integration, so it will be recorded as an availability result.

The selected family is the current production transport harness in
`src/test/fuzz/p2p_transport_serialization.cpp`: the one-way V1 parser and
round-trip path, plus `p2p_transport_bidirectional`,
`p2p_transport_bidirectional_v2`, and `p2p_transport_bidirectional_v1v2`.
The falsifiable hypothesis is that engine scheduling, persistent-loop
handling, input corpus transfer, or harness initialization can expose a
transport assertion, crash, hang, timeout, or semantic round-trip mismatch
that is not found by one sanitizer fuzzer. Native coverage counters are
engine-specific and will not be treated as equal units.

### Cycle protocol and initial queue

1. Build current HEAD with Clang 19 libFuzzer/ASan+UBSan, AFL++, and
   Honggfuzz where local wrappers support the same dispatcher. Record compiler,
   target binary hashes, flags, target selector, and tool versions. Reuse
   existing `/data` build trees only after verifying they are rebuilt from the
   current source; do not use stale binaries as current-head evidence.
2. Create one small deterministic corpus covering empty/truncated headers,
   valid and invalid message types, checksum/magic combinations, fragmented
   bytes, zero/maximal payloads, V1 round trips, and V2 key/garbage/entropy
   paths. Use identical copies, max input, worker count, RSS limit, and fixed
   budgets for every engine. Separate startup/dry-run time from feedback-loop
   measurements.
3. Run the one-way target and the three bidirectional variants under each
   available engine. Preserve executions, native coverage/corpus signals, RSS,
   crashes, hangs, timeout diagnostics, and repeatability. Transfer every
   engine-produced input through a current Clang 19 ASan+UBSan oracle and
   compare the target's explicit round-trip assertions.
4. If a failure appears, minimize it and replay it in a fresh process and
   through the smallest production transport boundary. A source change needs a
   failing-before/passing-after regression or equivalent mutation proof. If no
   source defect appears, close with exact metrics, tool limitations, raw
   artifact paths, and a new untouched transport/property cell.

### Cycle 175 build, corpus, and tool matrix

The shared corpus is `/data/my_storage/tmp/cycle175-corpus/source`: 14 files,
2,834 bytes total, including one empty seed that the engines omit from their
nonempty seed count. Its manifest file hash is
`e92d3ca85604140cf3eefb486e9d5db58478104cecfcfce06a255749669f905b`; every
engine received the same copied files. The seeds cover empty/truncated input,
header fragments, checksum/magic selector bytes, known message text, long V2
key/garbage material, alternating bytes, and deterministic ramps.

All successful builds used current source HEAD, Clang 19.1.7 where applicable,
`BUILD_FOR_FUZZING=ON`, x86_64 Linux, wallet/IPC/ZMQ disabled, and the same
fuzz dispatcher. The current binaries and hashes were:

- libFuzzer with `SANITIZERS=address,undefined,fuzzer`:
  `/data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz`,
  `2218ed5b8e7992b980ebe819424143ca103cd3233d6ee0f957dc8ff785759f8f`.
- AFL++ 4.04c PCGUARD build:
  `/data/my_storage/tmp/cycle131-build-afl19d/bin/fuzz`,
  `4b4999aadd04660f713036ff5031b096e1ebe6827ac4c2e48d3cecc75fbd5f56`.
- Honggfuzz 2.6 build:
  `/data/my_storage/tmp/cycle131-build-honggfuzz19/bin/fuzz`,
  `1089efb7afc95a5e6db510f2bd7e2fe8852e65a889d9417d043c655d06cf0983`.
  The existing wrapper initially selected Clang 14 and failed on the current
  `std::source_location` headers. That setup failure was quarantined; the
  same tree rebuilt successfully with `HFUZZ_CXX_PATH=/usr/bin/clang++-19`.
  FuzzTest was not installed and no repository integration was found.

The first AFL++ map probe was also quarantined because it used `afl-showmap`
against the persistent shared-memory entry point and captured zero tuples. The
corrected `afl-fuzz` run used `AFL_NO_FORKSRV=1` and
`AFL_SKIP_CPUFREQ=1`; it detected the persistent binary, completed its dry run,
and produced stable feedback. The host governor was not changed.

### Cycle 175 engine results: one-way transport

The libFuzzer control was:

    TMPDIR=/data/my_storage/tmp/cycle175-runs/lib-tmp FUZZ=p2p_transport_serialization /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/tmp/cycle175-runs/lib-serial-input -max_total_time=12 -seed=17511 -max_len=4096 -rss_limit_mb=2048 -timeout=2 -artifact_prefix=/data/my_storage/tmp/cycle175-runs/lib-serial-artifacts/ -print_final_stats=1

It completed 13,319 executions in 13 seconds, reached `cov 4231` and
`ft 9110`, added 120 units to a 45-entry minimized corpus, and used 817 MiB
peak RSS. No sanitizer diagnostic, assertion, crash, timeout, or artifact was
reported. The one-way AFL++ control was:

    FUZZ=p2p_transport_serialization AFL_NO_FORKSRV=1 AFL_SKIP_CPUFREQ=1 afl-fuzz -m none -i /data/my_storage/tmp/cycle175-corpus/source -o /data/my_storage/tmp/cycle175-runs/afl-serial2 -V 12 -- /data/my_storage/tmp/cycle131-build-afl19d/bin/fuzz

AFL++ completed 204 executions at 15.66 executions/sec, found 4 new queue
inputs for 17 total, measured 939 edges, 100.00% stability, and used 2 MiB
peak RSS. It saved zero crashes and zero hangs. Honggfuzz used:

    FUZZ=p2p_transport_serialization /data/my_storage/tmp/cycle131-tools/honggfuzz-build/honggfuzz -i /data/my_storage/tmp/cycle175-corpus/source -o /data/my_storage/tmp/cycle175-runs/hong-serial -W /data/my_storage/tmp/cycle175-runs/hong-work -n 1 -t 2 --run_time 12 -v -- /data/my_storage/tmp/cycle131-build-honggfuzz19/bin/fuzz ___FILE___

It completed 15,092 iterations in 13 seconds, added 37 units, reported 300,507
guards and 0% in its non-normalized branch metric, and used 29 MiB peak RSS.
It reported zero crashes and zero timeouts. Honggfuzz has no fixed mutation
seed control in this build, so its run is fixed-corpus evidence rather than a
byte-for-byte deterministic replay.

The current libFuzzer ASan/UBSan oracle replayed all 17 AFL++ queue inputs and
all 59 Honggfuzz `.honggfuzz.cov` inputs. The AFL++ replay completed 18 runs
and the Honggfuzz replay 60 runs, both with exit status 0, no diagnostic, and
819 MiB peak RSS. No engine-produced input exposed a target semantic mismatch.

### Cycle 175 engine results: mixed V1/V2 transport

The same matrix was repeated for `p2p_transport_bidirectional_v1v2`. The
libFuzzer run (`-max_total_time=10 -seed=17531`) completed 958 executions,
reached `cov 10660` and `ft 28489`, added 213 units, and used 820 MiB peak
RSS. AFL++ (`-V 12`) completed 199 executions at 15.87 executions/sec,
measured 2,712 edges with 100.00% stability, found 12 new inputs for 25 total,
and saved no crashes or hangs. Honggfuzz completed 205 iterations in 13
seconds, added 66 units, reported 300,507 guards, and used 31 MiB peak RSS;
its crash and timeout counts were zero.

The sanitizer oracle replayed all 25 AFL++ queue inputs and all 96 Honggfuzz
coverage inputs for this target. The replays completed 26 and 97 runs,
respectively, with no assertion, sanitizer diagnostic, crash, timeout, or
artifact. A separate V2-initiator libFuzzer run
(`FUZZ=p2p_transport_bidirectional_v2`, `-max_total_time=8`, seed `17551`)
completed 658 executions, reached `cov 12690` and `ft 30807`, added 138 units,
and used 823 MiB peak RSS without a diagnostic.

The production control
`/data/my_storage/tmp/cycle170-mempool-build/bin/test_bitcoin --run_test=net_tests`
passed all 36 selected cases and 152,142 assertions. This independently
exercises the V1/V2 transport contracts outside the fuzz dispatcher.

### Cycle 175 verdict and handoff

Dismissed for a new repository defect. The one-way transport and mixed V1/V2
targets exercised the same current production transport implementation under
libFuzzer, AFL++, and Honggfuzz. Differences in execution rate, corpus growth,
guard/edge counts, and RSS are expected from the engines' instrumentation and
scheduling models. Cross-engine sanitizer replay, the V2-initiator run, and the
focused production suite found no crash, hang, timeout, assertion, corpus
corruption, or semantic round-trip mismatch. No source or permanent test
change is justified. The Clang 14 setup failure and the zero-map showmap probe
remain tool-invocation limitations, not product evidence. Raw artifacts are
under `/data/my_storage/tmp/cycle175-corpus` and
`/data/my_storage/tmp/cycle175-runs`.

The transport serialization family is closed for this evidence cell. The
broader Goal 80 remains eligible for a future distinct target, compiler, engine
adapter, or property-framework comparison; do not repeat this target family
without new evidence.

## Cycle 171 start: stateful process_messages engine comparison

### Selection and fresh gate

- The post-Cycle-170 selector first returned `53` (`statistical-timing`),
  which was explicitly closed in the authoritative ledger; the required exact
  reroll `shuf -i 0-98 -n 1` returned `80` (`fuzz-engine-differential`).
- Branch: `uber-cycle-171-fuzz-engine-differential-20260730`.
- Start HEAD: `ad4a3cf1577b22bc9793b124922dd26e53052560`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`;
  `git rev-list --left-right --count HEAD...origin/master`: `1123 42`.
- The fresh post-close gate fetched `origin master`, passed both
  `git diff --check` and the cached equivalent, and found no tracked changes.
  Existing untracked agent/user artifacts remain preserved. PIDs 777094
  (`wallet_tests`) and 956381 (`util_tests`) were present and must not be
  stopped. Catalog/prompt/TSV/protocol hashes were unchanged at
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

### Scope and exclusions

Cycles 8, 79, and 131 already compared `bech32_roundtrip`, `parse_numbers`,
and `descriptor_parse` respectively; those exact target/corpus/result cells
are closed. FuzzTest is still not installed and has no repository integration,
so it is an availability result rather than a framework comparison. This
cycle selects the distinct stateful `process_messages` production harness,
which creates one to three peers, feeds up to 30 messages through the tested
connection manager and peer manager, drains each message, then exercises the
guided transaction-relay slice. The target resets chainstate and peer state,
checks send-queue accounting, relay-bucket invariants, and cleanup.

The hypothesis is that an engine's persistent-loop, input scheduling, or corpus
transfer behavior may expose a state-reset, timeout, sanitizer, or harness
realism difference that the earlier stateless/parser targets did not. Native
coverage counters are engine-specific and will not be ranked as equal units.
Only reproduced target behavior, transferred inputs, sanitizer diagnostics,
crashes, hangs, exit status, and explicit target assertions are cross-engine
evidence. No production change is justified by throughput or corpus size alone.

Use one shared deterministic seed corpus and identical target selector,
maximum input length, worker count, fixed wall-clock budget, memory limit, and
scratch locations. Compare Clang 19 libFuzzer and AFL++ when their targets
build; use Honggfuzz if its local binary/toolchain is available. Replay every
engine-produced input through a Clang 19 ASan+UBSan target. If the full
`process_messages` target is too expensive or one engine cannot initialize its
stateful loop, record that as a tool/harness limitation and use the distinct
single-message `process_message` target only with a new scope ledger entry.

### Initial queue

1. Inventory the current `process_messages` harness, its initialization/reset
   boundaries, target build identity, available fuzz engines, and a small
   semantic seed corpus covering unknown messages, handshake/order errors,
   fragmented payloads, multiple peers, relay permissions, and shutdown.
2. Run a fixed-budget libFuzzer/AFL++ comparison, preserving native metrics,
   executions, peak RSS, corpus growth, crashes, hangs, timeout diagnostics,
   and repeatability. Keep engine startup and corpus dry-run time separate from
   feedback-loop measurements.
3. Transfer minimized or queue inputs between engines and replay them through
   the sanitizer oracle. Inspect every failure for a production source path,
   a target assertion, a state-reset bug, an invalid harness assumption, or an
   engine/toolchain artifact.
4. If a real mismatch appears, reduce it to the smallest input and independently
   reproduce it in a fresh process and through the target's production boundary.
   A source change requires a failing-before/passing-after regression or an
   equivalent mutation result. Otherwise close with exact metrics, tool limits,
   raw artifact paths, and the next untouched target/toolchain cell.

## Cycle 171 evidence and completion

### Discovery and independent verification

The target inventory confirmed that both `process_messages` and the distinct
single-message `process_message` harness use persistent process state. Each
input reseeds the deterministic random source and resets chainman and peer
objects, but neither target reset the process-global mocked socket descriptor
counter in `src/test/fuzz/util/net.cpp`. The counter is consumed by `FuzzedSock`
and therefore made later inputs observe different descriptor values even when
their bytes and the target's deterministic seed were unchanged. Existing
network fuzzers such as `connman`, `http_request`, `i2p`, `pcp`, and `socks5`
already call `ResetFuzzedSockMockedFds()` at input start.

The same two targets also retained mempool state between inputs. The
`cmpctblock` harness provides an existing local precedent for reconstructing
`CTxMemPool` during per-input reset. The process-message targets reset their
peer manager before rebuilding it, so reconstructing the mempool at that same
boundary is safe and makes the stateful harness model one independent input.

The falsifiable hypothesis was tested with AFL++ calibration and repeated
sanitizer execution. Before the source change, an isolated `06-inv` input for
`process_messages` produced AFL++'s `instability detected during calibration`
warning, `variable` behavior, 99.55% stability, 11,998 map entries, and 25
executions in a five-second run. The single-message target showed the same
calibration warning. After adding only `ResetFuzzedSockMockedFds()`, the same
isolated `process_messages` input had no warning, `variable 0`, 12,000 map
entries, and the single-message target likewise had no warning and `variable
0` with a 6,062-entry map.

On the shared 13-file corpus, the descriptor-only patch left six variable
calibration inputs. Recreating the mempool as well reduced the five-second
corpus run to one variable input and produced no crash or hang. The remaining
`sendcmpct` calibration variation reproduced in isolation, including a valid
payload-shaped variant, after both resets. It is therefore retained as an
inconclusive AFL++ cold/lazy malformed-message or instrumentation limitation,
not claimed as a source defect. The 20-second final AFL++ run still had no
crashes or hangs; its final metrics are recorded below.

### Engine matrix and transferred-input evidence

The deterministic corpus had 13 files and 142 bytes, copied identically to
libFuzzer, AFL++, and Honggfuzz. Current-source engine binaries were rebuilt
with Clang 19 after both resets. Their SHA256 values were:

- libFuzzer ASan/UBSan: `a90b87c5c72ceb74dd015751717f44a148e96b5e107bf3e2199329bd41eade93`
- AFL++: `ddcbdc22efcd0f6c3b47d5226951a90ba9a9d442804749c29d74678254aac5b3`
- Honggfuzz: `e66645729b48bb306d330d77867d33db28aff5070d109e87c43abd7d25e97a03`

The fixed-budget engine results were:

| Engine | Budget/result | Native signal | Failures | Peak RSS |
|---|---|---|---|---:|
| libFuzzer, seed 17104 | 20 seconds, 159 executions | `cov 39049`, `ft 45334`, 67 new units | 0 sanitizer diagnostics/artifacts | 745 MiB |
| AFL++ 4.04c, no-forkserver persistent shmem | 20 seconds, 346 executions, 17.22 exec/s | 12-input corpus, 0.15% bitmap, 99.65% stability, 12,650 edges | 0 crashes, 0 hangs | 1 MiB |
| Honggfuzz, one worker | 21 seconds, 474 iterations | 56 new units, 300,504 guards, 2% reported branch coverage | 0 crashes, 0 timeouts | 92 MiB |

AFL++'s normal forkserver launch failed before accepting the corpus with
`Unable to request new process from fork server (OOM?)`; the successful
measurement used `AFL_NO_FORKSRV=1 -m none` and is explicitly tool-limited
evidence. The AFL++ final queue contained 14 files and its crashes and hangs
directories were empty. Honggfuzz emitted 72 `.honggfuzz.cov` files and the
final log reported `crashes_count:0 timeout_count:0`.

All 14 AFL++ queue files and 76 files from the Honggfuzz output directory were
replayed through the Clang 19 ASan/UBSan libFuzzer target. The AFL++ replay
completed 14 runs with no new units and 747 MiB peak RSS. The Honggfuzz replay
completed 79 runs with no sanitizer diagnostic, assertion, crash artifact, or
timeout and 747 MiB peak RSS. A separate corpus run of the patched
`process_message` target also completed cleanly. FuzzTest was not installed
and no repository integration was found, so no FuzzTest result is claimed.

### Verdict and fix

The confirmed finding is test-harness state leakage: persistent
`process_messages` and `process_message` executions did not restore the mocked
socket descriptor allocator or mempool to the per-input state implied by their
deterministic reset logic. This can create engine-dependent calibration and
corpus behavior and weakens reproducibility; it is not a Bitcoin production
behavior defect. The fix adds `ResetFuzzedSockMockedFds()` to both targets and
reconstructs `CTxMemPool` after the peer manager is released. The focused
before/after AFL++ calibration result, both target builds, the cross-engine
reruns, and sanitizer replay independently verify the change. No production
finding was confirmed.

The source/test/journal commit is `fuzz: reset process message harness state
between inputs`, authored as `Lőrinc <pap.lorinc@gmail.com>`. A separate
state-only close commit records the cycle in the authoritative uber-goal
ledger. The broader goal remains eligible for a future distinct engine,
target, compiler, or property-framework evidence cell; do not repeat the
closed `bech32_roundtrip`, `parse_numbers`, or `descriptor_parse` cells without
new evidence.

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

## Cycle 248 start: minisketch engine and property-framework comparison

- Exact selector: `shuf -i 0-98 -n 1` -> `80` (`fuzz-engine-differential`); no reroll.
- Branch: `uber-cycle-248-fuzz-engine-differential-20260801`.
- Start HEAD: `68de73058c82aedc0a4c8f2eb5c5b93875b59dd0`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence:
  `origin/master...HEAD = 42 1283`.
- Fresh gate: tracked/index state was clean, authoritative catalog/prompt/TSV/protocol hashes matched, and protected PIDs `777094`, `956381`, `1138182`, `1157959`, `1312049`, and `1312050` remained alive and untouched. Known untracked agent/user artifacts are preserved. `/` is full; all new build, corpus, and log data stays under `/data/my_storage/tmp`.
- Exclusions: prior Goal 80 cells for `bech32_roundtrip`, `parse_numbers`, `descriptor_parse`, `process_messages`, and transport serialization are closed and will not be repeated. FuzzTest remains an availability question until a local integration or package exists. Prior minisketch sanitizer/static-analysis observations are seeds only; this cycle compares engines on the current production fuzz harness.
- Scope and hypothesis: compare libFuzzer, AFL++, and Honggfuzz on `src/test/fuzz/minisketch.cpp`, which selects supported 32-bit implementations, constructs paired sketches, exercises seed setting and duplicate cancellation, serializes/deserializes, merges in both orders, decodes bounded differences, and asserts capacity/implementation/serialization/algebraic invariants. The falsifiable hypothesis is that an engine, persistent-loop adapter, input-corpus transfer, or harness initialization exposes a crash, hang, sanitizer error, invalid state, or reproducible semantic mismatch not found by the other engines.
- Protocol: build a current Clang 19 ASan+UBSan+libFuzzer oracle plus unsanitized current Clang 19 AFL++ and Honggfuzz targets in isolated scratch directories. Create one shared deterministic corpus covering empty/truncated inputs, capacity and implementation selectors, duplicate entries, boundary values, zero/nonzero seeds, serialized-looking buffers, and long operation streams. Use one worker, fixed input limits, fixed wall-clock budgets where supported, and no default datadir/wallet/key/database. Transfer every engine-produced input through the sanitizer oracle and compare exit status, diagnostics, and target assertions. Treat native coverage counters as engine-specific, not as equivalent units.
- Do not change production or test code for coverage/throughput differences alone. A source finding requires a minimized failing input, independent sanitizer or semantic reproduction, a failing-before/passing-after regression, and narrow-to-broad validation. FuzzTest is recorded as unavailable if the repository and installed paths still contain no supported integration.

## Cycle 248 close: minisketch engine and property-framework comparison

### Build and corpus evidence

- The selected target was `src/test/fuzz/minisketch.cpp`. The exact target path was chosen after excluding the prior Goal 80 cells for `bech32_roundtrip`, `parse_numbers`, `descriptor_parse`, `process_messages`, and transport serialization.
- The Clang 19 ASan+UBSan+libFuzzer build used `cmake -S . -B /data/my_storage/tmp/cycle248-build-libfuzzer-minisketch -G Ninja -DENABLE_IPC=OFF -DBUILD_FOR_FUZZING=ON -DSANITIZERS=address,undefined,fuzzer -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/usr/bin/clang-19 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-19 -DWITH_CCACHE=OFF` followed by `ninja -C /data/my_storage/tmp/cycle248-build-libfuzzer-minisketch fuzz -j2`; all 491/491 build steps passed. IPC was disabled because the installed Cap'n Proto 0.9.2 headers are incompatible with this Clang 19 C++20 configuration; this is an external build prerequisite limitation.
- The current-source AFL++ tree `/data/my_storage/tmp/cycle131-build-afl19d` rebuilt with `ninja -C /data/my_storage/tmp/cycle131-build-afl19d fuzz -j2`; all 202/202 steps passed. The current-source Honggfuzz tree `/data/my_storage/tmp/cycle131-build-honggfuzz19` initially selected a stale Clang 14 wrapper, then rebuilt successfully with `HFUZZ_CC_PATH=/usr/bin/clang-19 HFUZZ_CXX_PATH=/usr/bin/clang++-19 ninja -C /data/my_storage/tmp/cycle131-build-honggfuzz19 fuzz -j2`; all 200/200 steps passed. The stale-wrapper diagnostics were quarantined as scratch-toolchain noise.
- Binary hashes were `d5b214b1904aa3b07436be6ac1e4ec34178243ab43169c22baa48a8f55d43fa8` for libFuzzer, `b70757636b870284e5f2a843c5961d9bac5bc447f141bc5e7fe0fea7adfb4156` for AFL++, and `dcc7e2896b7aa2af42fb5e15dbf0054bf4eb07b6023bf80da7bd5b257d0e458e` for Honggfuzz.
- The initial deterministic corpus had 15 files, 9,122 bytes, and manifest SHA256 `258728ff941324afefa0ac50ef7fe64e1796cb9e3e6103ce5c6a015e1136f34d`. It covered empty/truncated input, zero and maximum byte patterns, capacity selectors, structured values, digest-derived values, duplicate-looking repeated streams, and 256/4096-byte operation streams. The fair per-engine copied-input manifest SHA256 was `29ccb02bfd1d01cadda9582a94a9021eadab69aa7a7709390dbaf79382034971`.
- FuzzTest is unavailable: repository search found only unrelated `RPCFuzzTestingSetup` identifiers, and installed-path search found only `/usr/lib/python3/dist-packages/samba/tests/smbd_fuzztest.py`; no FuzzTest package, library, or integration was found.

### Fixed-budget engine comparison

All runs selected `FUZZ=minisketch`, used one worker, scratch paths, and a 15-second wall-clock budget where supported. Native counters are engine-specific and are not equivalent coverage percentages.

| Engine/run | Input set | Executions | Native signal | Corpus signal | Failures |
|---|---|---:|---|---|---|
| libFuzzer, seed `24801` | initial 15 files | 4,462 | coverage 4,537 | 360 new units; 313 final in-memory corpus entries | 0 sanitizer errors |
| AFL++ forkserver | libFuzzer-expanded 316-file directory, accidental control | calibration aborted on `14-capacity-high` | forkserver process request failed with `OOM?` | none | mode startup failure |
| AFL++ no-forkserver | fair 14 non-empty files | 228 | 1,360 edges; 0.02% bitmap; 100% stability | 25 queue entries; 11 found | 0 crashes, 0 hangs |
| Honggfuzz, one thread | fair 15 files | 15,293 | 301,388 guards; branch metric 0% | 164 new units; 184 coverage files | 0 crashes, 0 timeouts |
| Honggfuzz transfer | libFuzzer-expanded corpus | native count not emitted in quiet mode | 157 coverage files | 157 transferred outputs | exit 0 |

The first AFL++ run accidentally used the libFuzzer input directory after libFuzzer had appended generated units. It is retained only as a control: 315/316 generated inputs were calibrated, two new corpus items were found, and no crash or hang occurred, but the result is not a fair 15-seed comparison. The corrected AFL++ no-forkserver run is the comparison result. A single-seed default forkserver rerun with `14-capacity-high` reproduced the same `Unable to request new process from fork server (OOM?)` abort, while the no-forkserver mode processed that seed normally. This is an AFL++ forkserver/toolchain-mode limitation in the scratch build, not a target failure.

### Transfer and independent verification

- The ASan+UBSan libFuzzer oracle replayed 315 libFuzzer-generated files, 25 AFL++ queue files, 184 isolated Honggfuzz coverage files, and 157 Honggfuzz outputs generated from the libFuzzer corpus. Every replay exited zero with no sanitizer error, assertion, timeout, or crash artifact.
- Key oracle results were `Done 316 runs`, `Done 26 runs`, `Done 185 runs`, and `Done 158 runs`, respectively. Peak RSS was 305, 278, 300, and 292 MiB. Raw logs are `/data/my_storage/tmp/cycle248-replay-libfuzzer.log`, `/data/my_storage/tmp/cycle248-replay-afl.log`, `/data/my_storage/tmp/cycle248-replay-honggfuzz.log`, and `/data/my_storage/tmp/cycle248-replay-hong-transfer.log`.
- The libFuzzer run log is `/data/my_storage/tmp/cycle248-libfuzzer.log`; AFL++ fair and anomaly logs are `/data/my_storage/tmp/cycle248-afl-fair-nofork.log`, `/data/my_storage/tmp/cycle248-afl-fair.log`, and `/data/my_storage/tmp/cycle248-afl-single.log`; Honggfuzz logs are `/data/my_storage/tmp/cycle248-hong.log` and `/data/my_storage/tmp/cycle248-hong-transfer.log`. Build logs are `/data/my_storage/tmp/cycle248-hong-build.log` and `/data/my_storage/tmp/cycle248-afl-build.log`.

### Verdict

The production hypothesis is **dismissed**. No engine exposed a minisketch crash, hang, invalid state, semantic mismatch, sanitizer failure, or corrupt transferred input. The AFL++ default forkserver abort is independently reproduced by one seed and avoided by no-forkserver mode; it is classified as an engine/toolchain integration limitation because the same current target and seed pass under the AFL++ no-forkserver path and the ASan oracle. Native coverage/corpus differences are expected engine behavior. No production or test change is justified.

### Handoff

- Close this cell with a journal/state-only commit; do not claim Goal 80 is exhausted. A future distinct cell may compare another target, engine version, compiler, or corpus with new evidence.
- Preserve the initial manifest, isolated per-engine copies, binary hashes, AFL forkserver/no-forkserver distinction, FuzzTest absence, and all replay logs above. Do not use the libFuzzer-mutated common directory as a fair initial corpus without restoring the 15-file manifest.
- Next cycle must recheck the repository/process/storage gate and draw a fresh exact selector from all 99 catalog rows. Do not reopen minisketch without new engine, toolchain, corpus, or regression evidence.

## Cycle 250: p2p_headers_presync engine comparison

### Selection, scope audit, and gate

- Exact selector `shuf -i 0-98 -n 1` drew goal `80` (`fuzz-engine-differential`); no reroll.
- Branch: `uber-cycle-250-fuzz-engine-differential-20260801`.
- Start HEAD: `87c371da45f84e0848221a43ed77289e180ba210`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `42/1287`.
- The tracked clean gate, authoritative catalog/prompt/TSV/protocol hashes,
  disk/process checks, and branch creation passed. All six protected earlier
  test processes remained alive. The root filesystem is full, so every fuzz
  datadir, log, and corpus was directed to `/data/my_storage/tmp`.

An initial exploratory run selected the singular `process_message` target. The
prior Cycle 171 record was re-read before accepting its result and explicitly
closes both the `process_messages` and `process_message` reset cell. That
exploratory run is retained under `/data/my_storage/tmp/cycle250-fuzz-engine-process-message`
but is excluded from this cycle's verdict. The final scope is the untouched
Goal 80 target `p2p_headers_presync`; no prior Goal 80 cycle compared this
target across engines. Other closed cells remain `bech32_roundtrip`,
`parse_numbers`, `descriptor_parse`, transport serialization,
`process_messages`/`process_message`, and `minisketch`.

### Target and hypothesis

The selected production harness is `src/test/fuzz/p2p_headers_presync.cpp`.
It constructs a main-chain testing setup with outbound full-relay, block-relay,
and inbound peers; sends generated `HEADERS`, `CMPCTBLOCK`, and `BLOCK`
messages; keeps generated work below `MinimumChainWork`; and asserts that
low-work pre-sync inputs do not mutate the block index, best header, or active
tip. It also checks that every peer's node-state statistics overwrite stale
`presync_height` and in-flight-height output. The falsifiable hypothesis was
that one engine, persistent-loop adapter, corpus transfer, or initialization
path could expose a crash, timeout, state mutation, sanitizer diagnostic, or
target assertion missed by the others.

The current Clang 19 ASan+UBSan libFuzzer binary was
`/data/my_storage/tmp/cycle248-build-libfuzzer-minisketch/bin/fuzz`, with hash
`d5b214b1904aa3b07436be6ac1e4ec34178243ab43169c22baa48a8f55d43fa8`. The
optimized AFL++ and Honggfuzz binaries were respectively
`/data/my_storage/tmp/cycle131-build-afl19d/bin/fuzz` with hash
`b70757636b870284e5f2a843c5961d9bac5bc447f141bc5e7fe0fea7adfb4156` and
`/data/my_storage/tmp/cycle131-build-honggfuzz19/bin/fuzz` with hash
`dcc7e2896b7aa2af42fb5e15dbf0054bf4eb07b6023bf80da7bd5b257d0e458e`.
The target source was unchanged since these current-source builds, so reuse
was valid. All runs were x86_64 Linux, one worker, `max_len=4096`, and no
default datadir, wallet, key, or production database. FuzzTest remains
unavailable: no supported package or repository integration exists locally.

The identical initial corpus had 15 files and 9122 bytes. Its manifest
`/data/my_storage/tmp/cycle250-fuzz-engine-headers-presync/initial-manifest.tsv`
has SHA256
`09f69175de81a2f0f7d18f4b560e12ecf851ee30a387a2488366038d3a935f6f`.
It covered empty, zero, all-ones, short, boundary-sized, repeated, ramp, and
structured byte streams. Each engine received a separate copy.

### Fixed-budget engine results

The sanitizer run used fixed seed `25021`, `-max_total_time=30`,
`-rss_limit_mb=4096`, and `-timeout=10`. AFL++ used
`AFL_NO_FORKSRV=1`, `AFL_SKIP_CPUFREQ=1`, `-V 30`, `-m none`, and one worker;
its log nevertheless reported that no `-t` option was recognized and used a
60 ms execution timeout. This is retained as an engine configuration
limitation, not hidden as target evidence. Honggfuzz used one persistent worker,
`--run_time 30`, and a 10-second per-input timeout.

| Engine | Executions | Native signal | Corpus signal | Peak RSS | Failures |
| --- | ---: | --- | --- | ---: | --- |
| libFuzzer ASan+UBSan | 503 | `cov 26103`, `ft 75523` | 125 new units; 138 final files | 695 MiB | 0 artifacts |
| AFL++ 4.04c no-forkserver | 553 | 7551 edges; 0.09% bitmap; 100% stability | 14 queue files, 0 found | not reported | 0 crashes, 0 hangs |
| Honggfuzz 2.6 | 1815; 58/sec | 301388 guards; 1% branch metric | 51 new units; 75 `.cov` files | 88 MiB | 0 crashes, 0 timeouts |

The exact raw logs are `cycle250-fuzz-engine-headers-presync/libfuzzer.log`,
`afl.log`, and `hong.log` under `/data/my_storage/tmp`. No crash, hang, or
sanitizer artifact directory contained a file. Native coverage and corpus
counts are engine-specific and are not treated as equal percentages.

### Independent replay and controls

The Clang 19 ASan+UBSan oracle replayed every retained engine output. The
libFuzzer corpus replay completed 180 runs with zero new units and 634 MiB
peak RSS. The 14 AFL++ queue inputs completed 23 runs with zero new units and
402 MiB peak RSS. The 75 Honggfuzz coverage inputs completed 104 runs with
zero new units and 534 MiB peak RSS. All three exited zero without an
assertion, sanitizer diagnostic, timeout, or artifact. Raw replay logs are
`oracle-libfuzzer.log`, `oracle-afl.log`, and `oracle-hong.log` in the same
scratch directory.

The focused current-source-equivalent control
`test_bitcoin --run_test=headers_sync_chainwork_tests --log_level=test_suite`
passed all 7 cases, including `presync_summary_tracks_headers` and
`too_little_work`, with `*** No errors detected`. The adjacent `net_tests`
control used during the discarded duplicate-target audit also passed all 36
cases. The target source's last relevant commit is
`ba4411648a` (`fuzz: check low-work presync headers stay unknown`); no source
change was made in this cycle.

### Verdict and limits

The hypothesis is **dismissed** for a new repository defect. Current
libFuzzer, AFL++, and Honggfuzz exercised the same header pre-sync harness and
corpus without a crash, hang, timeout, state mutation, target assertion,
sanitizer failure, or transferred-input mismatch. The unit suite independently
covered the node-state and low-work chainwork contracts. Engine-specific
throughput, coverage, and calibration behavior does not justify a production
or permanent test change.

Evidence is x86_64-only and does not establish ARM, 32-bit, big-endian,
LTO/PGO, or cross-toolchain equivalence. The reused binaries were validated
against unchanged target source, but no fresh full fuzz build was required.
The root-disk-full condition required `TMPDIR` redirection and is an
environment limitation. Next queue: a fresh Goal 80 target such as
`p2p_private_broadcast`, `txdownloadman`, or `package_eval`; do not reopen
`p2p_headers_presync` without a source, engine, compiler, corpus, or
reproducible-regression change.
