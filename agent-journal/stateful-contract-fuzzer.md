# Stateful Contract-Fuzzer Expansion

## Cycle 184 gate and scope

- Date: 2026-07-31 UTC
- Goal index: `61`
- Slug: `stateful-contract-fuzzer`
- Selector: exact `shuf -i 0-98 -n 1` -> `61`
- Branch: `uber-cycle-184-stateful-contract-fuzzer-20260731`
- HEAD at cycle start: `5d4a0bb99800f903fe1ce8c276996b9e31024f14`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence from `git rev-list --left-right --count HEAD...origin/master`: `1158 42`
- Tracked worktree: clean; known untracked agent artifacts preserved and excluded from staging.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Corrected TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Uber-state SHA256: `4dfb2ab62602803197e5f6b1619f881740fbe3c439f464534a1cedfec0f5d590`
- TSV schema: 99 records, four tab-separated fields, IDs 0 through 98 in order.
- Storage gate: `/` had about 94 MiB free; `/data` had about 49 GiB free.
- Protected processes still alive: PIDs `777094` and `956381`; neither was touched.

The previously selected AddrMan intermediate-round-trip cell is explicitly closed in this journal and the uber-state. This cycle therefore targets a different stateful API and evidence source: the production-backed `tx_pool` fuzz sequence, whose existing graph assertions use mempool-maintained ancestry helpers rather than an independent raw-transaction closure model.

## Cycle 184

### Scope, hypothesis, and prior-finding search

The selected distinct cell was the existing production-backed `tx_pool` and `tx_pool_standard` operation-sequence targets in `src/test/fuzz/tx_pool.cpp`. They exercise transaction acceptance, prioritisation, reorg removal/reinsertion, expiry, eviction, block construction, and cleanup, but their graph checks obtain ancestry and descendant results from the mempool's maintained `TxGraph` indexes. The falsifiable hypothesis was that a graph/index defect could make those checks self-consistent while raw transaction inputs described a different transitive closure.

The trust boundary is the set of accepted mempool transactions and their `CTxIn::prevout.hash` relationships. The expected oracle intentionally uses only the current transaction map, a direct parent/child map rebuilt from inputs, iterative closure, and independent size/modified-fee accumulation. It does not call `GetParents`, `GetChildren`, `GetAncestors`, `GetDescendants`, `GetCluster`, or any mempool graph lookup while constructing expected values. The earlier AddrMan serialization checkpoint was excluded as an explicitly closed cell. The prior Cycle 182 mempool campaign was also searched: its unit test independently modeled graph accounting, but did not execute that model inside the production-backed libFuzzer acceptance/reorg sequence, so this is a distinct fuzz-execution oracle rather than a duplicate finding.

### Harness change

Added `CheckIndependentMempoolGraph` to `src/test/fuzz/tx_pool.cpp`. For every current mempool entry it:

1. Rebuilds direct parent and child sets from transaction inputs.
2. Computes self-inclusive ancestor and descendant closures with a local work list.
3. Recomputes aggregate transaction size and saturating modified fees from those closures.
4. Compares count, size, and fee totals with `CalculateAncestorData` and `CalculateDescendantData`.

The check runs after each `tx_pool` and `tx_pool_standard` acceptance iteration and at the reorg, removal, expiry, final block-builder, and cleanup checkpoints in `Finish`. It is a test-only change; production mempool behavior and limits are unchanged. The input remains a libFuzzer byte stream, so shrinking removes operation bytes and retains a replayable sequence.

### Build and verification

The existing current-source Clang 19 build tree was reused from `/data/my_storage/tmp/cycle131-build-libfuzzer`. Its cache reports `CMAKE_CXX_COMPILER=/usr/bin/clang++-19` and `SANITIZERS=address,undefined,fuzzer`. Both the initial clean build and each disposable production mutation rebuild passed:

```text
cmake --build /data/my_storage/tmp/cycle131-build-libfuzzer --target fuzz -j2
initial: [1/11] through [11/11] passed
mutation rebuilds: [1/6] through [4/4] passed
```

The clean source hashes after restoration were:

```text
src/test/fuzz/tx_pool.cpp 3487f1fff70cdcb28ae34c889a9f6ee8e7fafd1b43123fb05cbf06437c2e3bf9
src/txmempool.cpp         4a11f4ca681204adf7ce67960f81c723003afe2402e21f29a4c3013a36bb82a3
fuzz binary               411c60548b30b745f460b9f1dee0b82ba084005ea93679d39241c6b53c3b6c7b
```

The retained `tx_pool_standard` corpus contained 109 files from `/data/my_storage/fuzz_corpora_runs/tx_pool_standard_20260627_1252_corpus`, 1 to 14 bytes each. Its clean replay passed 110 corpus executions before the new scratch seeds were added:

```text
FUZZ=tx_pool /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/fuzz_corpora_runs/tx_pool_standard_20260627_1252_corpus -runs=60 -max_len=256 -seed=18401 -print_final_stats=1
Done 110 runs in 0 second(s); cov 26629; ft 29920; peak RSS 979 MB
```

To reach the deeper setup and operation sequence deterministically, the scratch corpus `/data/my_storage/tmp/cycle184-txpool-seeds` used all-`0xff` 256-byte and 1024-byte inputs, all-`0x55` 128-byte input, and an alternating 8-byte input. The first clean scratch controls passed:

```text
FUZZ=tx_pool /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/tmp/cycle184-txpool-seeds -runs=8 -max_len=256 -seed=18402 -print_final_stats=1
Done 8 runs; cov 29758; ft 40673; peak RSS 980 MB

FUZZ=tx_pool_standard /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/tmp/cycle184-txpool-seeds -runs=8 -max_len=256 -seed=18403 -print_final_stats=1
Done 8 runs; cov 47926; ft 72008; peak RSS 980 MB

ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:symbolize=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
FUZZ=tx_pool /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/tmp/cycle184-txpool-seeds -runs=8 -max_len=256 -rss_limit_mb=1536 -seed=18404 -print_final_stats=1
Done 8 runs; cov 29755; ft 40671; peak RSS 981 MB
```

The first clean replay attempt without a temporary-directory override failed before target execution because the root filesystem had only about 56 MiB free and `LoadVerifyActivateChainstate` reported `Disk space is too low`. Replaying with `TMPDIR=/data/my_storage/tmp/cycle184-txpool-runtime` isolated test datadirs on the large filesystem. The restored clean binary then passed:

```text
ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:symbolize=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
TMPDIR=/data/my_storage/tmp/cycle184-txpool-runtime ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:symbolize=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 FUZZ=tx_pool_standard /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/tmp/cycle184-txpool-seeds/all-ff-1024 -runs=8 -max_len=2048 -rss_limit_mb=1536 -seed=18410 -print_final_stats=1
Done 8 runs in 1564 ms; peak RSS 989 MB

ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:symbolize=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
TMPDIR=/data/my_storage/tmp/cycle184-txpool-runtime ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:symbolize=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 FUZZ=tx_pool /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/tmp/cycle184-txpool-seeds/all-ff-1024 -runs=8 -max_len=2048 -rss_limit_mb=1536 -seed=18411 -print_final_stats=1
Done 8 runs in 332 ms; peak RSS 990 MB

TMPDIR=/data/my_storage/tmp/cycle184-txpool-runtime ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:symbolize=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 FUZZ=tx_pool_standard /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/tmp/cycle184-txpool-seeds -runs=12 -max_len=2048 -rss_limit_mb=1536 -seed=18412 -print_final_stats=1
Done 12 runs in 1 second; cov 48261; ft 85424; 3 new units; peak RSS 990 MB

TMPDIR=/data/my_storage/tmp/cycle184-txpool-runtime ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:symbolize=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 FUZZ=tx_pool /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/tmp/cycle184-txpool-seeds -runs=12 -max_len=2048 -rss_limit_mb=1536 -seed=18413 -print_final_stats=1
Done 12 runs in 0 second(s); cov 30524; ft 51754; peak RSS 991 MB
```

The raw terminal commands and outputs are retained in the session transcript. No ASan, UBSan, leak, assertion, crash, or hang occurred in the restored runs.

### Independent mutation proof

The disposable first mutation changed only `CTxMemPool::CalculateAncestorData` from `ancestors.size()` to `ancestors.size() + 1`. A populated `tx_pool_standard` fixed-input replay failed first at the older `GetTransactionAncestry` assertion `ancestors <= cluster_count` in `src/txmempool.cpp:1176`; that result was classified as an existing oracle and not used as proof for this change. The mutation was restored.

The independent mutation then changed only `CTxMemPool::CalculateDescendantData` from `descendants.size()` to `descendants.size() + 1`. The same `all-ff-1024` replay failed at the new raw-input oracle before any later result:

```text
FUZZ=tx_pool_standard /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz /data/my_storage/tmp/cycle184-txpool-seeds/all-ff-1024 -runs=1 -max_len=2048 -rss_limit_mb=1536 -seed=18409 -print_final_stats=1
test/fuzz/tx_pool.cpp:246: Assertion `actual_descendant_count == expected_descendants.size()` failed
descendant-mutated-run-exit=77
```

The production mutation was restored, the clean `src/txmempool.cpp` hash was rechecked, and the clean binary passed the final replays above. This proves the new oracle is sensitive to a graph-derived result that the old ancestry checks did not independently cover.

### Verdict and handoff

The hypothesis is **confirmed as a stateful fuzz-oracle gap, not as a production defect**. The raw-input model is now part of the `tx_pool` fuzz target and catches a descendant-accounting mutation independently. No production source change is justified. The intended source/test/journal change is limited to `src/test/fuzz/tx_pool.cpp` and this journal.

Limitations: no target-specific current qa-assets corpus was available; the deterministic scratch inputs reached the production sequence and the final libFuzzer replay produced three new units, but this is not a complete coverage claim. A full unit suite was not rerun because the relevant production code was unchanged and the fuzz target rebuild plus sanitizer replay supplied the focused validation. Root-volume pressure is an environment constraint; future fuzz runs must keep `TMPDIR` under `/data`.


## Cycle 13

- Date: 2026-07-27 UTC
- Goal index: 61
- Slug: `stateful-contract-fuzzer`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `c83969dae01c70eef95cf50904e95225404fc2ed`
- Selector command/result: `shuf -i 0-98 -n 1` -> `61`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`

### Scope and hypothesis

The selected campaign required production-backed operation-sequence fuzzers with model invariants, failure-state checks, deterministic shrinking, and replayable minimized inputs. Existing `src/test/fuzz/addrman.cpp` already had an `addrman` operation-sequence target and a separate `addrman_serdeser` target, but the sequence target only serialized its final state. It did not check whether intermediate mutated states survived serialization, or whether a lookup result was preserved across that state transition.

The falsifiable hypothesis was that the stateful fuzzer had a contract gap: a serialization or lookup defect could be introduced and later masked by another operation, while the final-only checks still passed. The trust boundary is the persisted `peers.dat` representation and the `FindAddressEntry` test-only API used by address-manager tests. This is a test-harness improvement; no production behavior was assumed to be wrong.

### Discovery and prior-finding search

The existing targets were read before changing code. The `addrman` sequence exercises collision resolution, tried-collision selection, add, good, attempt, connected, and service updates, followed by size, `GetAddr`, `Select`, and global entry invariants. `addrman_serdeser` fills one manager and checks one final serialize/deserialize result. Recent history already contains targeted `GetEntries`, `Select`, and `GetAddr` contract checks, so those were not duplicated.

The exact new helper and operation names were searched across journals, source, history, and current PR context; no prior intermediate-round-trip campaign was found. Open PR #35825 was reviewed as a related current net-state seed. Its author describes moving an unused connection count under the limit check, explicitly states there is no behavior change, and the only non-bot discussion is a minimal patch/benchmark follow-up. It did not provide a competing AddrMan oracle or a relevant defect.

### Harness change

Added `AssertSerializationRoundTrip` to the existing production-backed `addrman` target. It now:

1. Consumes a query address and records `FindAddressEntry`.
2. Serializes the live manager.
3. Deserializes into a deterministic fresh manager.
4. Compares the serialized byte stream after restoration, covering fields that the existing deterministic equality helper intentionally omits.
5. Compares the in-memory deterministic table state and the query lookup result.

The sequence runs a checkpoint after every 64 operations, capped at 16 intermediate checkpoints, and always performs one final checkpoint. The input remains a normal libFuzzer operation sequence, so shrinking removes operations and preserves a replayable byte input rather than relying on sleeps or external state.

### Build and verification

The normal deterministic fuzz build passed:

```text
cmake --build build_fuzz_libfuzzer_clang19 --target fuzz -j2
```

The ASan/UBSan deterministic fuzz build also passed:

```text
cmake --build build_fuzz_asan_clang19 --target fuzz -j2
```

The full qa-assets `addrman_serdeser` corpus was used as a seed corpus for the expanded sequence target:

```text
FUZZ=addrman build_fuzz_libfuzzer_clang19/bin/fuzz /data/my_storage/tmp/qa-assets/fuzz_corpora/addrman_serdeser -runs=2000 -max_len=4096 -seed=13061 -print_final_stats=1
Done 2605 runs in 67 second(s)
stat::number_of_executed_units: 2605
stat::new_units_added: 4
stat::peak_rss_mb: 851
```

No assertion, sanitizer, crash, or hang occurred. The final bounded replay after strengthening the byte-stream oracle used a small scratch corpus:

```text
FUZZ=addrman build_fuzz_libfuzzer_clang19/bin/fuzz /data/my_storage/tmp/stateful-contract-fuzzer-cycle13/control-one -runs=100 -max_len=1024 -seed=6117 -print_final_stats=1
Done 100 runs in 3 second(s)
stat::number_of_executed_units: 100
stat::new_units_added: 13
stat::peak_rss_mb: 851
```

The final rebuilt binary reported 298,696 inline counters for the target module, and the sequence replay completed through the added final checkpoint without an assertion. The checkpoint is unconditionally executed after the final `AddrMan` queries, so every completed `addrman` input exercised the new oracle; the `NEW_FUNC` lines in the output were existing operation lambdas and are not cited as proof of the helper itself.

The existing serialization-only target was run as a control on a single 74-byte seed:

```text
FUZZ=addrman_serdeser build_fuzz_libfuzzer_clang19/bin/fuzz /data/my_storage/tmp/stateful-contract-fuzzer-cycle13/control-one -runs=100 -max_len=4096 -seed=6115 -print_final_stats=1
Done 100 runs in 24 second(s)
stat::number_of_executed_units: 100
stat::new_units_added: 21
stat::peak_rss_mb: 851
```

An attempted 500-run control over a copied 32-seed corpus became resource-heavy after libFuzzer added large mutated seeds. It was interrupted at 230 executions after reaching approximately one execution per second; it produced no failure before interruption. That raw partial result is retained in the terminal output, and the bounded single-seed control above is the replayable completed control.

The final ASan/UBSan replay used the same small seed directory and strict abort settings:

```text
ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 FUZZ=addrman build_fuzz_asan_clang19/bin/fuzz /data/my_storage/tmp/stateful-contract-fuzzer-cycle13/control-one -runs=20 -max_len=1024 -seed=6118 -print_final_stats=1
Done 35 runs in 5 second(s)
stat::number_of_executed_units: 35
stat::new_units_added: 0
stat::peak_rss_mb: 851
```

No ASan, UBSan, leak, assertion, or libFuzzer failure was reported.

### Verdict

The hypothesis was confirmed as a test-oracle gap, not as a production defect. Intermediate round-trip and lookup state are now checked by the existing deterministic production-backed fuzzer. The change is limited to `src/test/fuzz/addrman.cpp`; no production source or persisted-format behavior changed.

Changed files:

- `src/test/fuzz/addrman.cpp`
- this journal

`git diff --check` passed before staging. Remaining limitation: the ASan run used the bounded corpus rather than the full 1,437-file qa-assets corpus because the full corpus consumed approximately 851 MB and the serialization control was resource-heavy. Next work must draw a distinct catalog hypothesis after the uber-goal state is updated.
