# Stateful Contract-Fuzzer Expansion

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
