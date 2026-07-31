# Fuzz-target gap and harness-realism audit

## Cycle 188 start: validation_load_mempool file-path reachability

### Fresh gate and selection

- `git fetch origin master` succeeded. The exact selector was
  `shuf -i 0-98 -n 1` -> `10`, `fuzz-target-gaps`; no reroll was needed.
- Dedicated branch:
  `uber-cycle-188-fuzz-target-gaps-20260731`. Start HEAD:
  `9472fdc21f5cc57cdc0b05615c0f84739965b55b`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence:
  `1166 42` from `git rev-list --left-right --count HEAD...origin/master`.
- Catalog SHA-256:
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
  Prompt SHA-256:
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
  Corrected TSV SHA-256:
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
  Protocol SHA-256:
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
  Uber-goal state SHA-256 at the gate:
  `24153f5a747b078b42cce6438dafdbf883da1c24314329f97a40c64fc63984aa`.
- The TSV has one header and 99 four-field records, IDs 0 through 98 exactly
  once. The tracked worktree was clean; known unrelated untracked artifacts
  were preserved and will not be staged. Root storage remained about 52 MiB
  free and `/data` about 49 GiB free. PIDs `777094` and `956381` were alive
  and were not touched.

### Distinct cell and working hypothesis

Cycle 147 closed the `p2p_private_broadcast` external-corpus gap and excluded
the earlier `process_messages` empty-corpus and `coins_view_db_resize_cursor`
cells. This cycle targets the separate `validation_load_mempool` production
fuzz target and its synthetic `FuzzedFileProvider`. The trust boundary is the
on-disk mempool dump and the fopen/read/write/seek/close failures that
`LoadMempool` and `DumpMempool` must handle without corrupting existing files or
violating mempool metadata contracts.

Working hypothesis: the target's initial `LoadMempool` call may have a
reachability or realism gap because it opens a path that is not pre-seeded;
short/empty inputs may mostly force open/read failure, while the later
round-trips exercise only dumps produced by the current serializer. Independently
measure static branch reachability and dynamic coverage for empty, structured,
and malformed file-operation seeds before proposing any harness change. Do not
call a deliberately injected I/O failure a missing path unless the target's
oracle observes the required file-preservation and state contract.

## Cycle 188 completion

### Baseline and evidence

The target is `src/test/fuzz/validation_load_mempool.cpp`; the production
surface is `src/node/mempool_persist.cpp`, including `LoadMempool`,
`DumpMempool`, transaction acceptance, priority metadata, unbroadcast state,
temporary-file commit, close, and rename behavior. I searched the target's
history, the fuzzer registration, the current `doc/fuzzing-findings.md` QA
notes, the sparse `bitcoin-core/qa-assets` tree, and prior Goal 10 cells.
The prior `p2p_private_broadcast` corpus gap, `process_messages` empty-corpus
cell, and `coins_view_db_resize_cursor` cell remain excluded as duplicates.

The current QA-assets tree is at sparse checkout
`0772287676fdf3fcf87631b383b12442ab48ce75`. Its
`fuzz_corpora/validation_load_mempool` directory contains 1,674 files,
122,866,874 bytes total, with sizes from 1 to 1,048,229 bytes. The existing
QA notes document sanitized replay of this corpus, including malformed
records, v1/v2 formats, metadata options, trailing parse failures, and dump
failure cases, but that does not by itself prove that the target's own
roundtrip pool contains a valid transaction.

I built an isolated Clang 19 profile target with `BUILD_FOR_FUZZING=ON`,
`BUILD_FUZZ_BINARY=ON`, `SANITIZERS=fuzzer`, and LLVM source coverage; IPC was
disabled because the installed Cap'n Proto 0.9.2 is incompatible with Clang
19. Before the change, fixed-seed profiles gave these results:

- Empty input: `mempool_persist.cpp` 66.46% lines and 55.88% branches.
- A stratified 16-file sample: 82.28% lines and 75.00% branches.
- The full 1,674-file corpus: 89.24% lines and 82.35% branches.

The full baseline profile had no successful `AcceptToMemoryPool` branch, no
`pool.get(txid) != nullptr` branch for restored unbroadcast transactions, and
no nonempty `DumpMempool` vinfo loop. The harness's later roundtrips were
serializing empty pools, so they could not exercise persistence of a valid
transaction, fee delta, or matching unbroadcast state. This is a harness
realism gap, not a production defect or a claim that malformed-input paths
were untested.

### Reproducer and source change

The target's one-time setup previously created a `const TestingSetup` and no
spendable fixture. I changed it to set mock time to the active tip, mine
`2 * COINBASE_MATURITY` blocks with `P2WSH_OP_TRUE` coinbase outputs, and sync
validation callbacks. The first 100 mature outputs are retained. After the
fuzzed initial load, the harness snapshots the pool and selects a mature
output not referenced by any already accepted input; this avoids making the
fixture assertion dependent on arbitrary fuzz data. It then accepts one
deterministic witness-valid transaction, applies a fee delta, adds its txid to
the unbroadcast set, checks the existing mempool contracts, and continues
through the existing v1/v2 and option roundtrips. The fixture is created only
in the harness; no production code or consensus rule changed.

The source patch is limited to the harness includes, one-time chain setup,
the mature-output selection, and the deterministic transaction fixture. The
initial prototype used one output; the final patch's 100-output selection was
added after review of the input-dependent collision risk and was rebuilt and
replayed.

### Independent verification

Both the profile and ASan/UBSan/libFuzzer binaries rebuilt successfully after
the final patch. `git diff --check` passed. The final full profile command
was:

```text
LLVM_PROFILE_FILE=/data/my_storage/tmp/cycle188-final-full.profraw \
FUZZ=validation_load_mempool \
TMPDIR=/data/my_storage/tmp/cycle188-final-full-profile-runtime \
/data/my_storage/tmp/cycle188-coverage-build/bin/fuzz -runs=1 -seed=1891 \
  -rss_limit_mb=4096 \
  /data/my_storage/tmp/cycle188-qa-validation-corpus/fuzz_corpora/validation_load_mempool \
  --testdatadir=/data/my_storage/tmp/cycle188-final-profile-tests-full
```

It replayed all 1,674 files in 1,675 runs, exited 0 after 159 seconds, and
held about 1.07 GB RSS. The final LLVM report was:

- `mempool_persist.cpp`: 98.10% lines, 95.59% branches.
- `validation_load_mempool.cpp`: 90.40% lines, 79.41% branches.

The final profile recorded these previously absent or weakly exercised
branches: valid acceptance 6.70k times, already-present handling 1.67k,
fee-delta import 459k, nonempty metadata loops 1.06M, matching unbroadcast
lookups 5.07k, nonempty dump vinfo 5.03k, close failure 9, and rename failure
1.67k. It also retained the malformed v1/v2, expiry, parse-failure, disabled
metadata, and partial-I/O paths. The final profile was merged with
`/usr/bin/llvm-profdata-19` and reported with `/usr/bin/llvm-cov-19`.

The final sanitizer replay used the changed binary
`/data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz` with
`FUZZ=validation_load_mempool`, `-runs=1`, seed `1889`, and the stratified
16-file sample. It completed 17 runs in 7 seconds with no ASan, UBSan, or
libFuzzer diagnostic. An empty-input 1-run replay also passed. The fixed-input
100-run smoke command was reported by libFuzzer as replay-only, so it is not
counted as mutation evidence.

### Verdict and next queue

**Confirmed local fuzz-harness realism gap; fixed in the source commit for
this cycle.** The final patch gives the existing corpus a deterministic valid
state to persist while preserving the original untrusted file-operation
surface. The before/after profile and independent sanitized replay are
sufficient evidence for a harness improvement; no production fix is claimed.

The next distinct cells are the nonempty `DumpMempool` `file.Commit()` failure
branch, which remained at zero true executions in the final profile, and the
`LoadMempool` validation-interrupt branch. They require deterministic fault
injection and must prove existing-file preservation and mempool-state
contracts rather than merely forcing an exception. Do not reopen the
empty-corpus or single-fixture collision hypotheses.

## Cycle 147

- Selected index: `10`
- Selected goal: `fuzz-target-gaps`
- Selector: `shuf -i 0-98 -n 1` -> `10`
- Branch: `uber-cycle-147-fuzz-target-gaps-20260730`
- Gate HEAD: `7e1b355db86329667fdfa4f5071c346dfbc19938`
- Gate `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Gate merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Gate divergence (`HEAD...origin/master`): `1078 42`
- Gate state: tracked/staged files clean; unrelated untracked agent artifacts were preserved. The pre-existing wallet test process remained alive and was not touched.
- Catalog, protocol, and goals TSV hashes were checked and unchanged.

## Scope and prior-finding search

The existing fuzz-target-gap journal did not exist. I searched the target inventory, CMake registration, `test/fuzz/test_runner.py`, the current coverage-runner change, recent fuzz history, `fuzz-introspector.md`, sanitizer notes, and related private-broadcast journals before choosing a cell. The earlier `process_messages` empty-corpus coverage-script finding and the `coins_view_db_resize_cursor` candidate were excluded as repeats. The selected distinct hypothesis was the production-backed P2P private-broadcast harness and its external corpus coverage.

## Hypothesis and static evidence

`src/test/fuzz/p2p_private_broadcast.cpp` has a target-specific reachability gap when its QA-assets directory is absent. The trust boundary is the fuzz input/corpus that drives `PeerManager`, `PrivateBroadcast`, connection handshakes, and inbound/outbound message processing.

The empty-input path is structurally limited:

- `ConsumeBool()` at line 60 returns false, so the test remains in IBD.
- `ConsumeIntegralInRange(0, 3)` at line 80 returns its lower bound when no bytes remain, so no transaction is seeded and `InitiateTxBroadcastPrivate` at lines 82-85 is skipped.
- `ConsumeBool()` at line 111 makes the private-broadcast peer non-relaying, intentionally exercising only the connected-in-vain disconnect assertion at lines 169-171.
- `ConsumeIntegralInRange(0, 2)` at line 175 creates no extra peers.
- `LIMITED_WHILE` at line 188 does not execute, so none of the guided inbound message, transaction broadcast, GETDATA, PONG, or non-private-peer TX paths at lines 199-237 run.

The production paths missed by this empty execution include `PeerManagerImpl::PushPrivateBroadcastTx` and the nontrivial `PrivateBroadcast::Add`, `PickTxForSend`, `GetTxForNode`, and `NodeConfirmedReception` state transitions. The target is registered in `src/test/fuzz/CMakeLists.txt`, but the latest QA-assets tree does not contain `fuzz_corpora/p2p_private_broadcast`.

## Corpus inventory and dynamic verification

The QA-assets repository was fetched into the disposable worktree `/data/my_storage/qa-assets-sparse`; its inspected `origin/main` is `6ea645d13b09bb5be644f154e50a9aa84f929ecb`. It contains `fuzz_corpora/private_broadcast` but no `fuzz_corpora/p2p_private_broadcast`. The sibling model target corpus was used only as an independent control, not treated as a target-specific corpus.

The existing libFuzzer build `/data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz` was run with fixed seeds. The target binary hash was `a94e84a143d6e465004d5292ee752f458b8ebd34e38912a797f194b62ab4b7b1` and the current P2P harness source hash was `8c1d9cecd27c77a83794690daac93ab8c90a8f840fd203758e563d75f5f4d999`.

Empty target corpus, one replay:

```text
FUZZ=p2p_private_broadcast .../bin/fuzz -runs=1 -seed=14711 -print_final_stats=1 <empty-dir>
status=0; executed=2; cov=15344; feature_count=15293; peak_rss=323 MB
```

The full coverage report for that run showed `PushPrivateBroadcastTx` at `0/103` edges, `PrivateBroadcast::Add` at `0/28`, `PickTxForSend` at `0/76`, `GetTxForNode` at `0/22`, `NodeConfirmedReception` at `0/52`, and `GetStale` at `0/55`. All six guided operation lambdas at lines 199-237 were uncovered.

Independent sibling-seed control, using `fuzz_corpora/private_broadcast/0018d62465abdbd362eaf01c9c256527d8baa2e6` (15,337 bytes):

```text
FUZZ=p2p_private_broadcast .../bin/fuzz -runs=1 -seed=14710 -print_final_stats=1 <sibling-seed-dir>
status=0; executed=3; cov=21357; feature_count=21311; peak_rss=329 MB
```

Its coverage report reached `PushPrivateBroadcastTx` with `20/103` edges, `PrivateBroadcast::Add` with `13/28`, `PickTxForSend` with `30/76`, `GetSendStatusByNode`, and `GetTxForNode`. The same seed passed the `private_broadcast` target as a control (`status=0; executed=2; cov=5690; peak_rss=242 MB`). A sanitizer-configured replay of the P2P target passed (`status=0; executed=3; peak_rss=336 MB`) with no ASan, UBSan, or leak diagnostic.

The runner's bounded empty-corpus mode can eventually discover additional paths, but that is not equivalent to deterministic replay. A fresh empty directory with `-max_total_time=2` reached `cov=17726` after 284 executions and peaked at 554 MB; the generated 19-file mutation corpus replay reached `cov=17730` after 25 executions. This confirms that the missing seed is a coverage and resource-efficiency gap, not an assertion that the harness is unreachable.

## Verdict

**Confirmed external fuzz-corpus gap; no local source fix.** The P2P harness intentionally preserves a meaningful zero-transaction/connected-in-vain case, so forcing a transaction or loop iteration into every input would weaken the test's contract. The correct remediation is a target-specific, minimized `p2p_private_broadcast` corpus in `bitcoin-core/qa-assets`, preserving at least one relay-enabled handshake, seeded transaction, peer-message sequence, and confirmation/disconnect path. The locally generated mutation files and the sibling corpus seed are evidence and candidate material only; they were not added to Bitcoin Core or the external repository.

## Limitations and handoff

The latest QA-assets commit was inspected through its Git tree; no external PR was opened. The reused fuzzer build was not rebuilt in this cycle, although the P2P source had no later change in the selected target path after the build lineage. Coverage counters are comparative evidence, not a complete reachability proof. Future work should submit/minimize a dedicated corpus externally, rerun the target under the current sanitized and high-throughput builds, and verify that the corpus remains useful after future private-broadcast protocol changes.
