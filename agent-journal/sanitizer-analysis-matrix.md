# Cross-tool sanitizer and static-analysis matrix

## Cycle 26: Clang/GCC sanitizer and analyzer comparison

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `36`
- Slug: `sanitizer-analysis-matrix`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at the cycle gate: `eaf1b625e35b2e3b9d71dbd9f8a70426b651bbec`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Gate: `git fetch origin master --quiet` passed; `origin/master...HEAD` was `2 822`; tracked source was clean; no source, test, fuzz, sanitizer, daemon, or profiling process was running at selection time.
- Existing `test/cache/` and agent-owned catalog/journal artifacts were left untouched. The unit test run created additional scratch files below `test/cache/`; they are not source changes.

### Tool and configuration inventory

Available: Clang/Clang++ 19.1.7, GCC/G++ 12.2.0, `scan-build-19`, and `llvm-symbolizer-19`. Missing from PATH: Valgrind, clang-tidy, cppcheck, Semgrep, and CodeQL. CMake also reported Valgrind unavailable while configuring the isolated builds.

Existing configurations supplied Clang and GCC TSan unit binaries and Clang/GCC ASan/UBSan fuzz binaries:

| Configuration | Compiler | Sanitizers | Wallet | IPC | Artifact |
|---|---|---|---|---|---|
| unit TSan | Clang 19 | `thread` | ON | OFF | `build_unit_tsan_clang19/bin/test_bitcoin` |
| unit TSan | GCC 12 | `thread` | OFF | ON | `build_unit_ipc_tsan_gcc/bin/test_bitcoin` |
| fuzz ASan/UBSan | Clang 19 | `fuzzer,address,undefined` | OFF | OFF | `build_fuzz_asan_clang19/bin/fuzz` |
| fuzz ASan/UBSan | GCC 12 | `address,undefined` | OFF | ON | `build_fuzz_ipc_asan_gcc/bin/fuzz` |

The fresh Clang unit build used `-DSANITIZERS=address,undefined`, `BUILD_TESTS=ON`, `ENABLE_WALLET=OFF`, `ENABLE_IPC=OFF`, and `WITH_CCACHE=OFF`. The fresh Clang static-analysis build used the same core-only feature scope. A first Clang configure with IPC enabled was rejected by the repository's explicit Cap'n Proto 0.9.2 and Clang 19.1.7 compatibility check; rerunning with IPC disabled is recorded as a dependency limitation, not a hidden configuration change.

### Runtime matrix

#### TSan unit slice

Both configurations ran the same command shape with the repository TSan and UBSan suppression files:

```text
TSAN_OPTIONS='suppressions=.../test/sanitizer_suppressions/tsan:halt_on_error=1:second_deadlock_stack=1:report_signal_unsafe=0' UBSAN_OPTIONS='suppressions=.../test/sanitizer_suppressions/ubsan:print_stacktrace=1:halt_on_error=1:report_error_type=1' <build>/bin/test_bitcoin --run_test=crypto_tests,logging_tests,net_tests --log_level=test_suite --report_level=short
```

- Clang 19: 63 cases, 173,304 assertions, all passed; no TSan, UBSan, runtime-error, or summary diagnostic appeared.
- GCC 12: 61 cases, 174,265 assertions, all passed; no TSan, UBSan, runtime-error, or summary diagnostic appeared.

The case-count difference is expected from wallet/IPC feature differences. The raw logs are `tsan-clang-unit.log` and `tsan-gcc-unit.log` under `/data/my_storage/tmp/sanitizer-analysis-matrix-cycle26/`.

#### ASan/UBSan fuzz-driver slice

The same three repository input files were passed to the common `bech32_roundtrip`, `addrman`, and `crypto` targets:

| Target | Input | Clang 19 | GCC 12 |
|---|---|---|---|
| `bech32_roundtrip` | `test/functional/p2p_compactblocks.py` | success | success |
| `addrman` | `test/functional/feature_addrman.py` | success | success |
| `crypto` | `test/functional/test_framework/crypto/chacha20.py` | success | success |

Clang used libFuzzer's `-runs=1 -seed=26036`; the GCC IPC fuzz driver accepts input paths directly and reported `succeeded against 1 files in 0s`. Every run used `ASAN_OPTIONS=abort_on_error=1:symbolize=1:detect_leaks=1` and the repository UBSan suppression/options. No ASan, LSan, UBSan, runtime-error, or crash marker appeared.

An initial attempted cross-driver command added `-runs=1000` to both invocations. Clang executed the one fixed input 1,000 times and explicitly reported that fuzzing was not performed. GCC treated `-runs=1000` as an input filename and failed at `test/fuzz/fuzz.cpp:268` with `assertion read_file(input_path, buffer)`. This is a driver-interface/configuration artifact, not a source finding; it was corrected by using one valid invocation per driver and retained in `asan-gcc-bech32.log`.

#### Fresh Clang ASan/UBSan unit build

The isolated build configured and linked `test_bitcoin` in 452 Ninja steps. It then ran:

```text
<build>/bin/test_bitcoin --run_test=argsman_tests,crypto_tests,fs_tests,logging_tests,net_tests,system_tests,threadpool_tests,util_tests --log_level=test_suite --report_level=short
```

With leak detection enabled, 180 test cases and 390,283 assertions passed. There were no ASan, LSan, UBSan, runtime-error, or sanitizer diagnostics. This directly exercises the `HexStr`, SHA256 autodetection, filesystem, logging, chain-argument, and thread-related paths implicated by the static warning set. The raw build and test logs are `asan-unit-build.log` and `asan-unit-focused.log`.

### Static-analysis matrix

#### Clang `scan-build`

The final command was:

```text
scan-build-19 --status-bugs -plist -analyze-headers -o /data/my_storage/tmp/sanitizer-analysis-matrix-cycle26/scan-reports --use-cc=clang-19 --use-c++=clang++-19 --exclude /data/my_storage/bitcoin/src/secp256k1 --exclude /data/my_storage/bitcoin/src/leveldb --exclude /data/my_storage/bitcoin/src/minisketch ninja -C /data/my_storage/tmp/sanitizer-analysis-matrix-cycle26/scan-clang bitcoin_crypto bitcoin_util
```

All 55 core crypto/util compilation steps and both static-library links completed with exit code 0. The report directory contained zero plist diagnostics. Third-party secp256k1, LevelDB, and minisketch paths were excluded and were covered separately by their own build/test evidence.

#### GCC `-fanalyzer`

The same `bitcoin_crypto` and `bitcoin_util` target scope was configured with GCC 12 `-fanalyzer` in both C and C++ flags, with IPC and wallet disabled. All 55 steps and links completed with exit code 0, but GCC emitted 29 warnings, all `-Wanalyzer-use-of-uninitialized-value` / CWE-457:

- `src/crypto/hex_base.cpp:43`: the analyzer does not model the initialized `std::string` plus the span-sized `memcpy` loop.
- `src/crypto/sha256_sse4.cpp:59`: `inp`, `inp_end`, and `xfer` are outputs/temporaries of the extended inline assembly. The source comment documents the real Clang/GCC ASan incompatibilities, and the Clang/GCC ASan crypto runs were clean.
- `src/crypto/sha256.cpp:587`: a false positive on `std::string ret = "standard"` during CPU-feature dispatch initialization.
- `src/util/chaintype.cpp:16,18,20`: false positives on valid enum-case string returns. The invalid-enum path is guarded by the existing assertion; callers obtain `ChainType` through the checked parser or fixed enum constants. No reachable untrusted cast to `ChainType` was found.
- `src/util/fs.h`, `src/util/fs.cpp`, and `src/util/tokenpipe.*`: warnings arise from standard-library filesystem wrappers, `errno`/syscall modeling, in-class member initialization, move construction, and the deliberately unreachable post-loop return in `TokenRead`. The Clang analyzer and the fresh runtime suite found no corresponding issue.
- `src/tinyformat.h` and `src/logging.cpp:222`: template/static-initialization false positives; the logging map initializes `out` before insertion and the ASan/UBSan logging suite passed.

The cross-tool disagreement, source inspection, valid-domain callers, and runtime controls make these warnings analyzer artifacts rather than confirmed source defects. No suppression was broadened and no production source change is justified. Keep the following lower-priority questions in the queue: add direct runtime coverage for `TokenPipe` and revisit `ChainTypeToString` only if a new FFI, deserialization, or unchecked enum conversion becomes reachable.

### Suppression audit and limitations

The repository TSan file contains an intentional test deadlock, Qt/ZMQ/external-library patterns, and linked historical race suppressions. The UBSan file distinguishes dependency overflows from expected in-tree arithmetic and crypto behavior. The cycle did not add or alter suppressions. Passing sanitizer runs are evidence for the exercised paths, not proof of all paths. MSan was not attempted because the available environment lacks the instrumented libc++/dependency setup used by CI; Valgrind and clang-tidy/cppcheck/Semgrep/CodeQL are not installed. IPC remained off for the Clang static and ASan unit builds because Cap'n Proto 0.9.2 is rejected with Clang 19.

### Verdict and handoff

Verdict: dismissed as a production defect for this cycle; the matrix found no confirmed memory, undefined-behavior, race, leak, or Clang static-analysis issue. The GCC analyzer warning cluster is a useful reproducible tooling artifact with explicit source-level explanations and cross-tool/runtime controls. No source or test change was needed. Raw evidence remains under `/data/my_storage/tmp/sanitizer-analysis-matrix-cycle26/`. No process remains running.

Reopen this goal for MSan with instrumented dependencies, Valgrind when installed, a direct TokenPipe test, a new analyzer warning class, or a new source/configuration path that invalidates the current classifications.

## Cycle 78: distinct sanitizer and analyzer reopen

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Initial draw: `2` (`assertion-invariant-audit`), rejected because cycle 68 already closed the selected GETBLOCKTXN assertion cell.
- Retry draw: `36` (`sanitizer-analysis-matrix`), accepted as a distinct reopen of the queued MSan/instrumented-dependency, Valgrind, direct TokenPipe, new analyzer-warning, and new source/configuration cells.
- Branch: `uber-cycle-78-sanitizer-analysis-matrix-20260728`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at cycle start: `b9bb18435221ef31a7e6bf9a9b761c080ea39af7`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: `git fetch origin master --quiet` passed; `origin/master...HEAD` was `2 935`; tracked source was clean apart from known agent-owned artifacts and `test/cache`; no relevant test, fuzz, sanitizer, daemon, or profiling process was running.

### Reopen scope and exclusions

Cycle 26 already covered Clang/GCC TSan unit slices, Clang/GCC ASan/UBSan fuzz slices, a Clang ASan/UBSan unit slice, Clang `scan-build`, and the existing GCC `-fanalyzer` CWE-457 warning cluster. This cycle must not repeat those same configurations or reclassify the same warnings. It instead asks whether a currently unavailable or omitted diagnostic path exposes a real defect: MSan with an instrumented dependency boundary, Valgrind/Memcheck if the tool is available, direct production-like `TokenPipe` execution and failure coverage, a new analyzer warning class, or a newly reached source/configuration path.

### Hypothesis and required evidence

The earlier cross-tool agreement may be incomplete at an omitted boundary rather than proving the whole subsystem safe. Inventory available compilers, runtimes, suppression files, dependencies, and build flags first. For each new report, identify the first invalid operation or exact dataflow, reproduce it on clean HEAD, and independently classify it as a source defect, test/harness defect, dependency/tool artifact, unsupported instrumentation, or intentional contract. Any source change requires a failing-before/passing-after oracle, preserved raw diagnostic, and focused then broad validation; do not add a suppression to hide a new report.

### MSan and direct TokenPipe execution

Clang 19's compiler-rt includes MSan and CMake accepted `-DSANITIZERS=memory`. A fresh wallet-disabled, IPC-disabled configuration built `bitcoin_util` and `bitcoind` in 290 Ninja steps, including `tokenpipe.cpp` and `signalinterrupt.cpp`, with no MSan compile or link diagnostic. Running the resulting daemon with `--version` stopped at the first uninstrumented system-library boundary: `std::ctype<char>::_M_widen_init()` in `/lib/x86_64-linux-gnu/libstdc++.so.6` called an MSan-intercepted `memcmp` with uninitialized bytes during static initialization. The raw report is `/data/my_storage/tmp/sanitizer-analysis-matrix-cycle78/msan-bitcoind-version.log`, with exit code 86. This is an instrumentation/dependency limitation, not a Bitcoin source finding.

To isolate the production TokenPipe code from that startup boundary, `agent-journal/tokenpipe_cycle78_probe.cpp` was compiled with Clang 19, libc++, and MSan against the current `src/util/tokenpipe.cpp`. The probe covers token values 0 and 255, EOF after the writer closes, move construction and assignment, closed-endpoint state, and a writer after its reader closes. On the pre-fix current branch the broken-writer case returned `TS_ERR` and the probe exited 16. With the fix it exited 0 under `MSAN_OPTIONS=halt_on_error=1:exit_code=86:report_umrs=1` without a diagnostic.

### Confirmed finding: EPIPE was classified as generic I/O failure

`TokenPipeEnd::TokenWrite()` documented `TS_EOS` for end-of-stream but returned `TS_ERR` for POSIX `write()`/`EPIPE` after the last reader closed. `src/init.cpp` deliberately ignores `SIGPIPE`, so this path is observable rather than terminating the process. `util::SignalInterrupt::operator()` propagates the nonzero status as failure, and daemon setup also depends on the endpoint status contract. A separate local branch contained an analogous unmerged EPIPE mapping, but it was treated only as a search lead; the current branch reproduced the mismatch independently.

The smallest fix maps `EPIPE` to `TS_EOS` before the generic error branch. The POSIX-only `util_tests/tokenpipe_end_of_stream` regression checks successful token transfer, reader EOF after writer close, and writer EOF after reader close while restoring the process `SIGPIPE` handler on setup failure. The fixed focused normal run passed 6/6 assertions, the full `util_tests` suite passed 79 cases and 3,991 assertions, and the focused TSan/UBSan run passed 6/6 assertions without diagnostics. Temporarily removing only the EPIPE mapping rebuilt the parent implementation and made the new regression fail at `-1 != -2`, exit 201, with the other five assertions passing; the mapping was restored before validation continued.

The fixed translation unit passed direct `scan-build-19 --status-bugs` with zero plist diagnostics and GCC 12 `-fanalyzer -fsyntax-only` with no warning. Valgrind, clang-tidy, cppcheck, Semgrep, and CodeQL are unavailable in this environment. No suppression was changed. The finding is confirmed and fixed; no other new sanitizer/analyzer defect was identified in the reopened cells. No relevant process remains running.

## Cycle 217: wallet-enabled sanitizer and lock-contract matrix

### Selection, gate, and distinct scope

- Exact selector: `shuf -i 0-98 -n 1` -> `36` (`sanitizer-analysis-matrix`); no
  reroll. Branch: `uber-cycle-217-sanitizer-analysis-matrix-20260731`.
- Start HEAD: `e4e2bc805c2d5ed7cb422c698a928fb98dc4227b`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was 42 behind
  and 1221 ahead. The fresh gate passed and protected processes remained
  untouched. Catalog, prompt, TSV, and protocol hashes matched the prior gate;
  state hash at entry was
  `5f7d1a05249026ee4d3638da159312091f31fdbb4984f2e75fc84ba96d298f38`.
- Cycles 26 and 78 already covered the core-only Clang/GCC sanitizer slices,
  the original GCC analyzer cluster, and the MSan/TokenPipe repair. This cycle
  selected the current wallet-enabled Clang ASan/UBSan path and a wallet-module
  static-analysis comparison, excluding those exact cells. IPC stayed off
  because the prior Clang 19 matrix recorded the Cap'n Proto compatibility
  boundary.

### Runtime sanitizer matrix

The isolated configuration used Clang 19.1.7, RelWithDebInfo, wallet and
external signer enabled, IPC disabled, tests enabled, and
`-DSANITIZERS=address,undefined`. The 510-step build completed and linked
`/data/my_storage/tmp/cycle217-sanitizer-wallet/bin/test_bitcoin` with no ASan,
UBSan, LSan, or runtime-error marker. The build log is
`/data/my_storage/tmp/cycle217-sanitizer-wallet/build.log`.

Before the source change, separate deterministic runs passed under
`ASAN_OPTIONS=abort_on_error=1:symbolize=1:detect_leaks=1:check_initialization_order=1`
and the repository UBSan suppressions:

| Suite | Cases | Assertions |
| --- | ---: | ---: |
| `wallet_tests` | 26 | 220 |
| `walletdb_tests` | 2 | 5 |
| `walletload_tests` | 1 | 6 |
| `wallet_crypto_tests` | 3 | 3335 |
| `wallet_transaction_tests` | 1 | 50 |
| `wallet_rpc_tests` | 1 | 9 |
| `scriptpubkeyman_tests` | 20 | 191 |
| `spend_tests` | 6 | 42 |

The annotation-only rebuild then completed six steps, and the post-change
wallet, walletdb, walletload, wallet RPC, and scriptpubkeyman suites passed:
`26/220`, `2/5`, `1/6`, `1/9`, and `20/191`. No sanitizer diagnostic appeared
in any run. Each suite used a separate `TMPDIR` below
`/data/my_storage/tmp/cycle217-sanitizer-annotated-*`.

### Static-analysis finding: transaction-listener lock contract was implicit

The first current wallet `scan-build-19` pass built all 32 wallet-library
steps and linked `libbitcoin_wallet.a`. It deposited zero plist reports, but
Clang emitted seven `-Wthread-safety` warnings at `src/wallet/wallet.cpp`:

- `LockCoin(WalletBatch&, ...)`'s `on_commit` lambda read and wrote
  `m_locked_coins` without an annotated lock contract at lines 2798-2799;
- `UnlockCoin(WalletBatch&, ...)` had the same issue at lines 2826-2827;
- `SetAddressPreviouslySpent(...)`'s callback called the lock-required
  `LoadAddressPreviouslySpent` and accessed `m_address_book` at lines
  2957-2960.

The warnings were not a runtime failure. The source contracts declare all
three functions `EXCLUSIVE_LOCKS_REQUIRED(cs_wallet)`. `WalletBatch::TxnCommit`
invokes registered listeners synchronously before returning, and current
registrations are made while that lock is held: `lockunspent` takes
`LOCK(pwallet->cs_wallet)`, `AddToWallet` takes `LOCK(cs_wallet)`, and the
existing `RemoveTxs` callback uses the same annotated-lambda pattern. The
nearby address-book callback already takes an explicit lock. Thus the warning
identified an unexpressed static contract, not an observed unlocked execution.

The smallest fix adds `() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet)` to the three
callbacks. It changes no runtime behavior and adds no suppression. The
incremental scan rebuilt `wallet.cpp` and the wallet library with zero compiler
warnings, zero plist reports, and exit 0. The direct GCC 12 check of the
changed translation unit used `-fsyntax-only -fanalyzer -O0`; it exited 0 with
zero analyzer warnings. Raw logs are:

- `/data/my_storage/tmp/cycle217-scan-wallet/scan-build.log` (before);
- `/data/my_storage/tmp/cycle217-scan-wallet/scan-build-annotated2.log` (after);
- `/data/my_storage/tmp/cycle217-gcc-wallet/wallet-fsyntax.log` (after).

### GCC whole-target limitation and classification

A separate GCC 12 `-fanalyzer` wallet configuration reached the wallet RPC
translation units but was killed by the system at `src/wallet/rpc/spend.cpp`
while `cc1plus` was analyzing at `-O2` with `-j2`. The final Ninja status was
`subcommand failed`; this is a resource limit, not a compiler diagnostic. The
completed warnings were grouped as standard-library modeling artifacts:
throwing `operator new` modeled as nullable in signal-handler wrappers,
allocator/vector relocation paths in `migrate.cpp`, `prevector.h`, and
`signingprovider.h`, and custom secure-allocator initialization at
`secure.h:72`. The Clang scan plist set was empty, ASan/UBSan wallet suites
were clean, and the changed `wallet.cpp` GCC analyzer pass was clean. No
suppression was broadened. The full GCC log is
`/data/my_storage/tmp/cycle217-gcc-wallet/build.log`.

### Verdict and handoff

The lock-contract warning cluster was **confirmed as a static-analysis gap and
fixed** by the three annotations. No memory-safety, undefined-behavior, leak,
or Clang `scan-build` finding was confirmed. The fix and this journal are one
self-contained change; the source change is ready for a single authored commit.

Remaining distinct queue: a wallet-enabled Clang TSan run with IPC disabled,
wallet fuzz targets under ASan/UBSan, a lower-complexity per-translation-unit
GCC analyzer pass, and MSan/Valgrind when instrumented dependencies or tools
become available. Do not reopen the prior core-only, TokenPipe, or original
GCC-warning cells without a changed source/configuration boundary.
