# Build matrix and module-configuration audit

## Cycle 311 start

- Selected by exact `shuf -i 0-110 -n 1` -> `17` (`build-matrix-modules`).
- Branch: `uber-cycle-311-build-matrix-modules-20260802`.
- Cycle-start/base HEAD: `9ac0b27e2ec97d612941a9c9fdd1ab33c8f66edf`; selection commit: `afe1acd689`.
- Goal 17 prior cells were searched in `agent-journal/build-dead-zones.md`: wallet/IPC/GUI-off target registration, GCC wallet-enabled IPC fuzzing, and reduced-export monolithic/fuzz/IPC graphs are excluded. New evidence must use a different configuration, compiler/dependency, generated/install artifact, or source interaction.
- Trust boundary: supported build options and CI/release recipes must select the intended sources, generated artifacts, target graph, hardening, and runtime tests consistently. A successful default build is not evidence for omitted feature combinations.

## Initial matrix questions

1. Which current CMake options and target guards are not represented in CI or prior executed matrices?
2. Do CMake, Autotools, depends/Guix, fuzz, bench, shared/static, and install/export recipes agree on module and generated-source ownership?
3. Are option aliases, cache defaults, or feature combinations silently ignored, forced, or left with stale generated files?

Keep all new builds in `/data/my_storage/tmp/cycle311-build-matrix-*`, use separate build directories, and avoid default datadirs, wallets, keys, or production databases. Record exact configure commands, cache values, generated target membership, build/link results, and focused runtime output. Do not treat unavailable platform/toolchain execution as a source defect; preserve it as a limitation and use compile-time or graph evidence instead.

## Cycle 311 investigation

### Candidate: chainstate option dependency

The current option declarations make `BUILD_UTIL_CHAINSTATE` default `OFF` and `BUILD_KERNEL_LIB` default to the chainstate value, but both remain independently user-settable. `src/CMakeLists.txt` creates `bitcoin-chainstate` only inside `if(BUILD_KERNEL_LIB)`, even though the summary and test configuration use `BUILD_UTIL_CHAINSTATE` as the executable capability flag. The executable also links directly to `bitcoinkernel`, so `BUILD_UTIL_CHAINSTATE=ON;BUILD_KERNEL_LIB=OFF` cannot be a meaningful build.

Historical review: commit `7990463b105` moved the pure kernel chainstate executable into the `BUILD_KERNEL_LIB` block while retaining the independent options and did not add an invalid-combination check. The original executable addition in `13bc96b9ec`/`bb1a450dcb` placed its guard independently, so the later nesting changed the configuration contract.

### Pre-fix reproduction

Exact command, in scratch build `/data/my_storage/tmp/cycle311-chainstate-without-kernel-1`:

```text
cmake -S /data/my_storage/bitcoin -B /data/my_storage/tmp/cycle311-chainstate-without-kernel-1 -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_UTIL_CHAINSTATE=ON -DBUILD_KERNEL_LIB=OFF -DBUILD_KERNEL_TEST=OFF -DBUILD_TESTS=OFF -DBUILD_TX=OFF -DBUILD_UTIL=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DBUILD_GUI=OFF -DENABLE_WALLET=OFF -DENABLE_IPC=OFF -DWITH_EMBEDDED_ASMAP=OFF -DWITH_CCACHE=OFF -DWITH_ZMQ=OFF -DWITH_USDT=OFF
```

Configure exited 0 and printed `bitcoin-chainstate ... ON` with `libbitcoinkernel ... OFF`. The generated `test/config.ini` contained `ENABLE_BITCOIN_CHAINSTATE=true`, but `cmake --build ... --target help` listed no chainstate or kernel target. `cmake --build ... --target bitcoin-chainstate` then exited 1 with `ninja: error: unknown target 'bitcoin-chainstate'`. This is both a misleading build summary and a functional-test capability false positive.

### Fix and independent verification

Added an early option-interaction check in `CMakeLists.txt`:

```cmake
if(BUILD_UTIL_CHAINSTATE AND NOT BUILD_KERNEL_LIB)
  message(FATAL_ERROR "BUILD_UTIL_CHAINSTATE requires BUILD_KERNEL_LIB=ON.")
endif()
```

The same invalid command in `/data/my_storage/tmp/cycle311-chainstate-without-kernel-2` now exits 1 at `CMakeLists.txt:205` with the exact dependency error, before a misleading summary or test config can be generated. A valid configure in `/data/my_storage/tmp/cycle311-chainstate-valid-1` with both options ON exits 0, prints both capabilities ON, writes `ENABLE_BITCOIN_CHAINSTATE=true`, and lists `bitcoin-chainstate`, `bitcoinkernel`, `libbitcoinkernel`, and `libbitcoinkernel.a` in the generated target graph. `git diff --check` passes.

The full compile/link was deliberately not run because the workspace and root filesystems are at 100% usage and the valid graph would require a new kernel build; prior Cycle 278 artifacts already cover the wallet-off, IPC-off kernel compilation cell. The configuration-graph proof is sufficient for this option-contract defect. No protected process was stopped.

### Verdict

Confirmed local CMake configuration defect. One self-contained source commit will add the guard, and the journal will be included in that commit. The invalid cell is now prevented rather than silently advertised as testable.

### Next queue

1. `kernel-chainstate-config-parity`: inspect install components, pkg-config/header visibility, CTest/functional registration, Windows and cross-build recipes, and `BUILD_KERNEL_TEST` interactions under the repaired dependency relation.
2. Compare `BUILD_KERNEL_LIB=ON` with chainstate OFF against chainstate ON for generated install manifests without compiling new large artifacts.
3. Revisit CMake/Autotools/depends parity only after the kernel-specific graph is checked.
