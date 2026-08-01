# Build dead-zone and conditional-compilation audit

## Cycle 470 - build options and module guards

### Handoff metadata

- Catalog goal: 37, `build-dead-zones`.
- Base: Bitcoin Core `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`.
- Worktree: `/mnt/my_storage/bitcoin-goal37-build-dead-zones`.
- Branch: `codex/goal37-build-dead-zones`.
- The protected Bitcoin Core worktree was not modified. The disposable worktree was clean before the CMake change below.
- Host evidence: Linux x86_64, GNU 16.1.0, CMake, out-of-tree builds under `/mnt/my_storage`.

The cycle mapped the main top-level CMake options, their summary entries, source-list guards, and the relevant Windows CI generation options. `BUILD_UTIL_CHAINSTATE` and `BUILD_KERNEL_LIB` are declared independently at `CMakeLists.txt:113-115`, while `src/CMakeLists.txt:401-418` creates the chainstate target only inside `if(BUILD_KERNEL_LIB)`. The existing `BUILD_KERNEL_TEST` option is correctly dependent on `BUILD_KERNEL_LIB`. The fuzzing option interaction at `CMakeLists.txt:170-201` intentionally disables both kernel options before dependency checks.

### Hypothesis A - external signer disabled leaves an enabled wallet test or build dead zone

Configuration:

```text
cmake -S /mnt/my_storage/bitcoin-goal37-build-dead-zones \
  -B /mnt/my_storage/bitcoin-build-goal37-external-off \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_GUI=OFF -DBUILD_TESTS=ON \
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DENABLE_WALLET=ON \
  -DENABLE_EXTERNAL_SIGNER=OFF -DENABLE_IPC=OFF -DWITH_ZMQ=OFF \
  -DWITH_EMBEDDED_ASMAP=OFF -DWITH_USDT=OFF -DWITH_CCACHE=OFF
```

The configure summary reported wallet support ON and external signer OFF. `bitcoind`, `bitcoin-wallet`, and `test_bitcoin` built successfully. The generated functional-test configuration commented out `ENABLE_EXTERNAL_SIGNER`, and:

```text
python3 /mnt/my_storage/bitcoin-build-goal37-external-off/test/functional/test_runner.py wallet_signer.py
```

exited 0 with `wallet_signer.py skipped (external signer support has not been compiled.)`. The feature-off path has an explicit test skip and no build omission was found. Verdict: dismissed.

### Hypothesis B - kernel and chainstate targets form a dead zone when wallet support is disabled

Configuration used `ENABLE_WALLET=OFF`, `BUILD_KERNEL_LIB=ON`, `BUILD_KERNEL_TEST=ON`, and `BUILD_UTIL_CHAINSTATE=ON`, with GUI, IPC, ZMQ, ASMap, and USDT disabled. The summary showed all three experimental kernel values enabled and wallet support disabled. The requested target name `kernel-test` has no generated target, but the CMake target is actually `test_kernel`, matching `src/test/kernel/CMakeLists.txt`.

```text
cmake --build /mnt/my_storage/bitcoin-build-goal37-kernel-nowallet \
  --target bitcoin-chainstate test_kernel --parallel 8
```

completed successfully, and:

```text
ctest --test-dir /mnt/my_storage/bitcoin-build-goal37-kernel-nowallet \
  --output-on-failure -R '^test_kernel$'
```

passed 1/1. The earlier `--target kernel-test` failure was only a target-name mismatch, not a missing module. Verdict: dismissed.

### Hypothesis C - explicitly disabling the kernel library silently disables the requested chainstate utility

Before the fix, this configuration was accepted:

```text
cmake -S /mnt/my_storage/bitcoin-goal37-build-dead-zones \
  -B /mnt/my_storage/bitcoin-build-goal37-chainstate-no-kernel \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_GUI=OFF -DBUILD_TESTS=OFF \
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DENABLE_WALLET=OFF \
  -DENABLE_IPC=OFF -DWITH_ZMQ=OFF -DWITH_EMBEDDED_ASMAP=OFF \
  -DWITH_USDT=OFF -DBUILD_KERNEL_LIB=OFF -DBUILD_UTIL_CHAINSTATE=ON \
  -DWITH_CCACHE=OFF
```

Configure exited 0 and printed:

```text
bitcoin-chainstate (experimental) ... ON
libbitcoinkernel (experimental) ..... OFF
```

The source guard then omitted the executable, and the requested target failed with:

```text
gmake: *** No rule to make target 'bitcoin-chainstate'.  Stop.
```

This is a real configuration contract failure: the summary claims the requested utility is enabled, but no target is generated and no configure diagnostic explains the dependency. `git blame` shows both options came from the chainstate utility change, while current source still nests the executable under the kernel-library guard. The nearest analogous options use `cmake_dependent_option` or explicit option interaction.

### Fix and independent verification

Added this validation immediately after the `BUILD_FOR_FUZZING` option interaction and before dependency checks:

```cmake
if(BUILD_UTIL_CHAINSTATE AND NOT BUILD_KERNEL_LIB)
  message(FATAL_ERROR "BUILD_UTIL_CHAINSTATE requires BUILD_KERNEL_LIB=ON.")
endif()
```

This preserves the documented fuzzing override: a configuration with `BUILD_FOR_FUZZING=ON`, deliberately conflicting `BUILD_UTIL_CHAINSTATE=ON`, `BUILD_KERNEL_LIB=OFF`, GUI, wallet, IPC, and ZMQ options was accepted after the override, reported chainstate and kernel OFF, and produced the fuzz binary. A no-op rebuild of the target completed with:

```text
[100%] Built target fuzz
```

The invalid chainstate/kernel configuration now exits 1 during configure with:

```text
CMake Error at CMakeLists.txt:203 (message):
  BUILD_UTIL_CHAINSTATE requires BUILD_KERNEL_LIB=ON.
```

The valid wallet-off kernel configuration was reconfigured after the change; `bitcoin-chainstate` and `test_kernel` rebuilt successfully and the focused CTest passed 1/1. This is the smallest correction because it rejects only the inconsistent user-requested state and leaves valid combinations unchanged.

### Cycle verdict

Hypotheses A and B were dismissed. Hypothesis C was confirmed and fixed in the disposable branch. The production finding is a CMake configuration diagnostic, not a runtime or consensus issue. No unrelated source or generated files were changed.

### Commands and evidence index

- `git blame -L 100,135 -- CMakeLists.txt`
- `git log --oneline --all -S 'BUILD_UTIL_CHAINSTATE' -- CMakeLists.txt`
- `rg -n -C 3 'BUILD_UTIL_CHAINSTATE|BUILD_KERNEL_LIB|BUILD_KERNEL_TEST|bitcoin-chainstate' CMakeLists.txt src doc .github`
- Valid kernel build: `cmake --build /mnt/my_storage/bitcoin-build-goal37-kernel-nowallet --target bitcoin-chainstate test_kernel --parallel 8`
- Valid kernel test: `ctest --test-dir /mnt/my_storage/bitcoin-build-goal37-kernel-nowallet --output-on-failure -R '^test_kernel$'`
- Invalid dependency configure: the `cmake` command in Hypothesis C, exit 1 after the fix.
- Fuzz override configure and build: `/mnt/my_storage/bitcoin-build-goal37-fuzz-override`, final `cmake --build ... --target fuzz --parallel 8` exit 0.

### Limitations and handoff

No cross-compiled or native Windows, macOS, BSD, ARM, big-endian, or alternate-generator build was run. This cycle did not claim complete conditional-compilation coverage. The next selection must use the controller's eligible pool and compare this journal before opening another configuration cell.

## Next queue

The remaining queue is tracked in `agent-journal/uber-goal-state.md`. Preserve the exact worktree, base, commands, and verdicts above when resuming; do not retest the three cells without a new hypothesis.
