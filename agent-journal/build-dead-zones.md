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

## Cycle 74: Distinct Conditional-Target Matrix

### Identity and exclusions

- Cycle: `74`
- Draw: `37`
- Branch: `uber-cycle-74-build-dead-zones-20260728`
- Gate HEAD: `ddde67072a`
- Prior cell excluded: cycle 20's GCC `BUILD_FOR_FUZZING=ON`, `ENABLE_IPC=ON`, wallet-enabled `wallet_bdb_parser` empty-buffer UBSan defect, fixed in `bbca305738`.

This re-entry does not repeat the wallet parser or GCC IPC fuzz build. It tests the remaining high-risk conditional zones: `ENABLE_WALLET=OFF`, `ENABLE_IPC=OFF` monolithic builds, test/bench registration under those flags, and generated/source-list parity when optional modules are absent.

### Active hypothesis

A feature-disabled configuration may compile only because an unrelated default target supplies a symbol or generated file, may leave a stale target in the build graph, or may omit a required test/bench/source path. The first pass will compare configuration summaries and target graphs, then execute the smallest meaningful build and runtime/registration controls in isolated build directories under `/data/my_storage/tmp/build-dead-zones-cycle74/`.

### Required evidence

Record each configure command and summary, source/target membership, generated files, build result, test/bench/fuzz registration, and runtime output. A source fix requires a clean failing configuration or runtime control, a minimal mutation or old-source reproduction, and a passing independent control after the fix. Do not treat a missing optional target as a defect unless the option contract and supported configuration require it.

## Cycle 74 Verification

### Configuration matrix

All build trees were isolated below `/data/my_storage/tmp/build-dead-zones-cycle74/`; `cmake`, Ninja, Clang 19.1.7, GCC 12.2.0, Cap'n Proto 0.9.2, and Qt 6.4.2 were available. The host root filesystem was 100% full, so all test and compiler temporary state used `/data` scratch directories. The relevant configure commands were:

```text
cmake -S /data/my_storage/bitcoin -B /data/my_storage/tmp/build-dead-zones-cycle74/wallet-off-ipc-off-clang -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 -DENABLE_WALLET=OFF -DENABLE_IPC=OFF -DBUILD_GUI=OFF -DBUILD_TESTS=ON -DBUILD_BENCH=ON -DBUILD_FUZZ_BINARY=ON -DBUILD_FOR_FUZZING=OFF -DWITH_EMBEDDED_ASMAP=OFF -DWITH_CCACHE=OFF
cmake -S /data/my_storage/bitcoin -B /data/my_storage/tmp/build-dead-zones-cycle74/wallet-on-ipc-off-clang -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 -DENABLE_WALLET=ON -DENABLE_IPC=OFF -DBUILD_GUI=OFF -DBUILD_TESTS=ON -DBUILD_BENCH=ON -DBUILD_FUZZ_BINARY=ON -DBUILD_FOR_FUZZING=OFF -DWITH_EMBEDDED_ASMAP=OFF -DWITH_CCACHE=OFF
cmake -S /data/my_storage/bitcoin -B /data/my_storage/tmp/build-dead-zones-cycle74/wallet-off-ipc-on-gcc -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DENABLE_WALLET=OFF -DENABLE_IPC=ON -DBUILD_GUI=OFF -DBUILD_TESTS=ON -DBUILD_BENCH=ON -DBUILD_FUZZ_BINARY=ON -DBUILD_FOR_FUZZING=OFF -DWITH_EMBEDDED_ASMAP=OFF -DWITH_CCACHE=OFF
cmake -S /data/my_storage/bitcoin -B /data/my_storage/tmp/build-dead-zones-cycle74/fuzz-wallet-off-ipc-off-clang -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 -DENABLE_WALLET=OFF -DENABLE_IPC=OFF -DBUILD_FOR_FUZZING=ON -DBUILD_GUI=OFF -DWITH_EMBEDDED_ASMAP=OFF -DWITH_CCACHE=OFF
cmake -S /data/my_storage/bitcoin -B /data/my_storage/tmp/build-dead-zones-cycle74/gui-wallet-off-ipc-off-clang -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 -DENABLE_WALLET=OFF -DENABLE_IPC=OFF -DBUILD_GUI=ON -DBUILD_TESTS=ON -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DBUILD_FOR_FUZZING=OFF -DWITH_EMBEDDED_ASMAP=OFF -DWITH_CCACHE=OFF
```

All five configurations completed successfully. The first two Clang monolithic builds completed 651 and 716 Ninja steps respectively with `test_bitcoin`, `bench_bitcoin`, and `fuzz`. The GCC IPC-on build completed 709 steps for `bitcoin-node`, `bitcoin-cli`, `test_bitcoin`, `bench_bitcoin`, and `fuzz`. The fuzz-only build completed 452 steps and linked only the effective `fuzz` graph. The GUI wallet-off build completed 452 steps and linked both `bitcoin-qt` and `test_bitcoin-qt`.

`BUILD_FOR_FUZZING=ON` intentionally overrides ordinary target variables after cache initialization. Its cache still displays requested values such as `BUILD_TESTS:BOOL=ON`, but the configure summary and generated graph correctly show tests, bench, node, CLI, wallet tool, and GUI OFF and only fuzz ON. This behavior is documented in `CMakeLists.txt` and `doc/fuzzing.md`; it is not a stale-target defect.

### Target and registration parity

- Wallet-off IPC-off Clang: `bitcoin-wallet` is an unknown target; the generated header undefines `ENABLE_WALLET`; wallet tests and wallet fuzzers are absent; the wallet-specific benchmark names are absent.
- Wallet-on IPC-off Clang: `bitcoin-wallet` resolves; wallet tests, wallet fuzzers (`wallet_bdb_parser`, `wallet_create_transaction`, and `wallet_fees`), and wallet benchmarks register.
- Wallet-off IPC-on GCC: `bitcoin-node`, generated Cap'n Proto/multiprocess libraries, `ipc_tests`, and the `ipc` fuzz target resolve; `bitcoin-wallet` and wallet fuzzers remain absent.
- Wallet-off IPC-off fuzz-only Clang: `fuzz` resolves; `test_bitcoin`, `bench_bitcoin`, `bitcoin-node`, and `bitcoin-wallet` are unknown targets; the compiled registry contains `process_messages` and no wallet or IPC target.
- Wallet-off GUI Clang: `bitcoin-qt` and `test_bitcoin-qt` resolve; `bitcoin-wallet` is unknown and `ENABLE_WALLET` is undefined in the generated header.

The wallet-off Clang tests registered DB, network, and chainstate suites but no wallet or IPC suites. The wallet-on tests registered the wallet suites. The IPC-on GCC test registry contained `ipc_tests` and no wallet suite. Wallet-related benchmark names appeared only in the wallet-on build, apart from the shared `ExpandDescriptor` benchmark.

### Runtime controls

With pre-created `/data` `TMPDIR` directories, the following passed:

```text
wallet-off IPC-off: dbwrapper_tests/dbwrapper, dbwrapper_tests/dbwrapper_constructor_failure_cleanup, netbase_tests/netbase_splithost
wallet-on IPC-off: dbwrapper_tests/dbwrapper, dbwrapper_tests/dbwrapper_constructor_failure_cleanup, wallet_tests/wallet_interface_missing_tx_outputs, walletdb_tests/walletdb_readkeyvalue, wallet_crypto_tests
wallet-off IPC-on: ipc_tests (2 cases, 44 assertions)
wallet-off GUI: all four Qt test groups (15 total test cases) under QT_QPA_PLATFORM=offscreen
```

The fuzz-only `process_messages` and `tx_in` empty-input smokes passed with `/data` scratch state. Its wallet target was rejected as `No fuzz target compiled for wallet_bdb_parser`. The release-like Clang and GCC fuzz binaries correctly refused execution with `Must compile with -DBUILD_FOR_FUZZING=ON or in Debug mode`, matching the documented release-build contract. The wallet-on release-like registry still listed its wallet fuzz targets, and the IPC-on registry listed `ipc` and `process_messages`.

One early combined test command used a non-existent `TMPDIR` and consequently reported a missing temp directory; its chainstate fixture also encountered an existing assertion before the process was stopped. It was discarded as a malformed environment control and rerun with individual Boost filters and pre-created scratch directories. No source behavior was inferred from that run.

## Verdict and Handoff

**Dismissed for a new source defect:** the distinct wallet-off, IPC-off/monolithic, IPC-on wallet-off, fuzz-only, and GUI wallet-off conditional matrix compiled and registered exactly the sources and targets required by the option contracts. Focused runtime controls passed, and the only negative results were the full-root temporary-directory setup and the documented release-build fuzz refusal. No production or test source change is justified.

Evidence logs and build trees remain under `/data/my_storage/tmp/build-dead-zones-cycle74/`. The prior cycle-20 wallet-enabled GCC fuzz cell remains excluded. Future re-entry should use a different compiler/dependency version, sanitizer combination, generated-file perturbation, or feature interaction.

## Cycle 101: Reduced-export visibility configuration

### Identity and exclusions

- Cycle: `101`
- Draw: `37`
- Branch: `uber-cycle-101-build-dead-zones-20260729`
- Gate HEAD: `d6014b70e8a15b8c4e77111adcccbfc6fd0e363e`
- `origin/master` at gate: `67998e15c8ddc5b84a4709a6485fd8dcfb9350d2`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `31 992`
- Prior cells excluded: cycle 20's GCC wallet-enabled IPC fuzz target, cycle 74's wallet/IPC/GUI-off target matrix, and the wallet parser empty-buffer UBSan fix. Those cells do not test `REDUCE_EXPORTS` or reduced-visibility cross-target linkage.

### Active hypothesis

`REDUCE_EXPORTS=ON` changes C++ visibility and executable linker behavior. A feature combination that builds with default visibility may hide an inline or cross-library symbol needed by the wallet, crypto, IPC-stub, test, or generated-code paths. The first experiment will use GCC 12, wallet enabled, IPC disabled, GUI disabled, tests enabled, and embedded ASMap disabled, then build and run `test_bitcoin` in an isolated `/data` tree. The configuration summary, compile/link flags, exported-symbol surface, and runtime registration will be recorded.

### Required evidence

A source finding requires a clean failing configuration or runtime control, an independent old-source or flag-mutation reproduction, and a passing control after the smallest fix. A successful build is evidence only for the tested target graph; it does not establish shared-library, IPC-on, GUI, or alternate-toolchain parity. Preserve raw configure/build/test output and leave unresolved cells in the next queue.

## Cycle 101 Verification

### Reduced-export monolithic graph

The first current-source configuration used GCC 12.2.0, `RelWithDebInfo`, `REDUCE_EXPORTS=ON`, `ENABLE_WALLET=ON`, `ENABLE_IPC=OFF`, `BUILD_GUI=OFF`, `BUILD_TESTS=ON`, `BUILD_BENCH=ON`, `BUILD_FUZZ_BINARY=ON`, `WITH_EMBEDDED_ASMAP=OFF`, `WITH_ZMQ=OFF`, `WITH_USDT=OFF`, and `WITH_CCACHE=OFF`. CMake applied `-fvisibility=hidden` and `-Wl,--exclude-libs,ALL`; the `-no_exported_symbols` probe was unavailable on GNU ld and was correctly omitted. The build completed all 716 Ninja steps and linked `test_bitcoin`, `bench_bitcoin`, and `fuzz`.

The full command was:

```text
cmake --build /data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-gcc --target test_bitcoin bench_bitcoin fuzz -j4
```

The reduced-export `test_bitcoin` ran all 1,208 cases and exited 0 with `*** No errors detected`. The suite emitted the expected diagnostics from argument-boundary and injected filesystem-failure tests; none became a failed case. `bench_bitcoin --list` enumerated the benchmark registry. The reduced executable had 62 defined dynamic symbols versus 183 in the default `/data/my_storage/tmp/cycle89-build` test binary, confirming that the visibility option affected the produced binary rather than only the CMake summary.

The RelWithDebInfo fuzz multiplexer compiled the wallet and P2P target sources, including `wallet_bdb_parser` and `process_messages`, but refused execution with the documented `Must compile with -DBUILD_FOR_FUZZING=ON or in Debug mode` guard. `PRINT_ALL_FUZZ_TARGETS_AND_ABORT=1` listed both targets, so this was classified as the expected release-build execution contract, not a hidden-target failure.

### Reduced-export fuzz graph

A second current-source configuration used GCC 12.2.0, `Debug`, `BUILD_FOR_FUZZING=ON`, `REDUCE_EXPORTS=ON`, wallet enabled, IPC disabled, GUI disabled, embedded ASMap/ZMQ/USDT disabled, and ccache disabled. CMake intentionally disabled all non-fuzz targets and retained wallet support; the reduced-export flags remained active. The fuzz target completed all 491 Ninja steps. These representative controls both exited 0:

```text
TMPDIR=/data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-fuzz-gcc-tmp FUZZ=wallet_bdb_parser .../bin/fuzz /dev/null
wallet_bdb_parser: succeeded against 1 files in 0s.

TMPDIR=/data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-fuzz-gcc-tmp FUZZ=process_messages .../bin/fuzz /dev/null
process_messages: succeeded against 1 files in 0s.
```

### Reduced-export IPC graph

The first tree was reconfigured with `ENABLE_IPC=ON` and `WITH_EXTERNAL_LIBMULTIPROCESS=OFF`, retaining wallet, tests, bench, fuzz registration, and `REDUCE_EXPORTS=ON`. Cap'n Proto schemas and generated proxy sources were rebuilt; the incremental `bitcoin-node` and IPC-enabled `test_bitcoin` build completed 203 steps. `ipc_tests` passed 2 cases with no errors, and `bitcoin-node --version` exited 0 with the expected `bitcoin-node` identity. The IPC target therefore has no current reduced-visibility link or startup failure in this GCC configuration.

## Verdict and Handoff

**Dismissed for a new source defect:** the current reduced-export monolithic, fuzz-only, and IPC-on graphs compiled and ran their relevant controls. No hidden symbol, missing generated source, target-registration, wallet/P2P fuzz, IPC linkage, or startup defect was confirmed. No production or test source change is justified.

Evidence build trees and scratch state remain under `/data/my_storage/tmp/cycle101-build-dead-zones/`. Limitations are shared libraries, GUI/Qt, ZMQ/USDT, alternate compilers, cross-architectures, and the upstream `CMAKE_VISIBILITY_INLINES_HIDDEN` change now present on `origin/master` but not merged into this cycle branch. A future re-entry should select one of those cells or a generated-file/install/export mismatch rather than repeat reduced-export GCC coverage. The unrelated PID `777094` remained untouched.
