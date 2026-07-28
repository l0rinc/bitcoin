# Build Dead-Zone and Conditional-Compilation Audit

## Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `37`
- Selected goal: `build-dead-zones` (Build dead-zone and conditional-compilation audit)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at cycle start: `238b809e9af1189c3ad0163f350162339d32cd07`
- Catalog: `agent-journal/reusable-continuous-agent-goals.md`
- Gate: fresh `git fetch origin master --prune` succeeded; tracked source was clean; `git diff --check` passed; no active build, daemon, fuzz, or test process.

## Scope and Hypothesis

The highest-risk local interaction was `BUILD_FOR_FUZZING=ON` with `ENABLE_IPC=ON` and wallet support retained. This mode intentionally disables ordinary executables and unit tests, but must still compile generated IPC schemas, the IPC fuzzer, wallet code, wallet fuzzers, shared node code, and the ordinary fuzz target registry. The falsifiable hypothesis was that one of those conditional source or generated-artifact zones was omitted, incorrectly linked, or contained a sanitizer-visible harness failure that ordinary wallet-disabled fuzz builds could not reach.

The source map covered top-level option overrides in `CMakeLists.txt`, target/source conditions in `src/CMakeLists.txt`, `src/ipc/CMakeLists.txt`, `src/test/fuzz/CMakeLists.txt`, `src/wallet/test/fuzz/CMakeLists.txt`, test configuration generation, and target data-source/install helpers. The relevant expected contract is: fuzz mode has no `test_bitcoin` or daemon target, but the `fuzz` target includes IPC when `ENABLE_IPC=ON`, wallet fuzzers when `ENABLE_WALLET=ON`, and all generated Cap'n Proto dependencies needed by those targets.

## Configuration Evidence

The exact Clang 19 plus system Cap'n Proto configure was attempted with:

```text
cmake -S . -B /data/my_storage/tmp/build-dead-zones-cycle20/fuzz-ipc -G Ninja \
  -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_FOR_FUZZING=ON \
  -DENABLE_IPC=ON -DWITH_EXTERNAL_LIBMULTIPROCESS=OFF \
  -DSANITIZERS=address,undefined,fuzzer -DWITH_CCACHE=OFF
```

It stopped during dependency validation because the installed Cap'n Proto was `0.9.2`, which the repository explicitly rejects with Clang 19.1.7/C++20. This is an environment/toolchain blocker, not a source finding; the diagnostic recommends Cap'n Proto 1.0+ or GCC.

The GCC control used the same feature interaction and completed configuration:

```text
cmake -S . -B /data/my_storage/tmp/build-dead-zones-cycle20/fuzz-ipc-gcc -G Ninja \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_FOR_FUZZING=ON \
  -DENABLE_IPC=ON -DWITH_EXTERNAL_LIBMULTIPROCESS=OFF \
  -DSANITIZERS=address,undefined -DWITH_CCACHE=OFF
```

The configure summary reported:

- ordinary `bitcoin`, `bitcoind`, CLI, TX, util, kernel, and `test_bitcoin` targets OFF;
- fuzz binary ON;
- wallet support ON and IPC ON;
- embedded ASMap OFF;
- `FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION` defined.

`cmake --build /data/my_storage/tmp/build-dead-zones-cycle20/fuzz-ipc-gcc --target fuzz -j4` completed all 510 steps and linked `bin/fuzz`. The generated build graph contains both `src/ipc/test/fuzz/ipc.cpp` and `src/wallet/test/fuzz/wallet_bdb_parser.cpp`, plus the generated `ipc_fuzz.capnp` sources. The target inventory printed `ipc`, `wallet_bdb_parser`, `asmap`, and `p2p_private_broadcast`; IPC and wallet fuzz zones were therefore executed, not merely configured.

GCC emitted existing third-party or optimization warnings from libstdc++/Cap'n Proto and fuzz utility code. They did not stop the build and did not identify a missing conditional source.

## Confirmed Finding

The first empty-input smoke test was:

```text
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
FUZZ=wallet_bdb_parser \
.../build-dead-zones-cycle20/fuzz-ipc-gcc/bin/fuzz /dev/null
```

It exited 1 with:

```text
src/streams.cpp:102:24: runtime error: null pointer passed as argument 1, which is declared to never be null
#0 AutoFile::write(...) src/streams.cpp:102
#3 wallet_bdb_parser_fuzz_target(...) src/wallet/test/fuzz/wallet_bdb_parser.cpp:38
```

The harness constructed `std::span{buffer}` from an empty vector. Its `data()` pointer was null, and `AutoFile::write` passed it to `fwrite` even though the requested size was zero. This is a local fuzz-harness defect exposed only when wallet support is retained in the conditional fuzz build; it is not evidence of a production wallet parser failure.

The minimal fix guards the serialization operation with `if (!buffer.empty())`. No production code, parser behavior, or build option was changed.

## Verification

After the one-line fix, the same scratch target rebuilt in 5 steps. These controls passed with `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`:

```text
FUZZ=wallet_bdb_parser .../bin/fuzz /dev/null
wallet_bdb_parser: succeeded against 1 files in 0s.
exit=0

FUZZ=wallet_bdb_parser .../bin/fuzz src/wallet/test/fuzz/wallet_bdb_parser.cpp
wallet_bdb_parser: succeeded against 1 files in 0s.
exit=0

FUZZ=ipc .../bin/fuzz /dev/null
ipc: succeeded against 1 files in 0s.
exit=0
```

`git diff --check` passed. The source diff is one conditional write, and the fix is independently buildable in the exact configuration that exposed the issue. The pre-fix failing trace, post-fix passing trace, target graph, and Clang dependency limitation are preserved in this journal.

## Verdict and Handoff

**Confirmed and fixed:** conditional fuzz build exposed a UBSan-invalid empty-buffer write in `wallet_bdb_parser`; the guard is committed with this journal. No other build dead zone was confirmed in the tested IPC/wallet/ASMap matrix.

Next queue: draw another eligible goal. Preserve `/data/my_storage/tmp/build-dead-zones-cycle20/` as scratch evidence, and revisit this matrix only with a different compiler, Cap'n Proto version, wallet/IPC feature combination, generated-file mismatch, or sanitizer result.
