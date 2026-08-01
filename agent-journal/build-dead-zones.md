# Build dead-zone audit

## Cycle 498: kernel test default versus global test switch

This cycle used disposable Core worktree
`/mnt/my_storage/bitcoin-goal37-conditional-20260801`, branch
`codex/goal37-conditional-20260801`, based at protected Core HEAD
`00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`. The protected checkout was not
modified. Earlier Goal 37 cells for the chainstate/kernel dependency pair and
the libsecp module guards were excluded from this audit.

Hypothesis: `BUILD_TESTS=OFF` leaves the experimental `test_kernel` target in
the default graph because `BUILD_KERNEL_TEST` depends only on
`BUILD_KERNEL_LIB`. That would make a no-tests build compile a test executable
and would leave its `add_test` registration inert because top-level CTest is
enabled only when `BUILD_TESTS` is on.

The relevant history introduced `BUILD_KERNEL_TEST` with a default derived
from `BUILD_KERNEL_LIB` in `2cf136dec4ce16c8a7c47b35c7c9244dfc3b6da8` and
later changed it to `cmake_dependent_option(... ON "BUILD_KERNEL_LIB" OFF)` in
`fe1815d48f0cee57d2f1af50b377c7f9e462369e`. The neighboring GUI and wallet
test options derive their defaults from `BUILD_TESTS`. The current CI
no-wallet workflow reference points to the existing
`ci/test/00_setup_env_native_nowallet.sh`; the deleted historical
`...nowallet_libbitcoinkernel.sh` name was only present in the parent commit,
so no separate CI-file defect was reported.

### Baseline

The pre-fix configuration used GCC 16.1.0, CMake/Ninja, RelWithDebInfo,
`BUILD_TESTS=OFF`, `BUILD_KERNEL_LIB=ON`, `BUILD_UTIL_CHAINSTATE=OFF`, and all
unrelated GUI, wallet, IPC, ZMQ, benchmark, fuzz, tx, and util targets off.
Without specifying `BUILD_KERNEL_TEST`, the configure summary reported:

    libbitcoinkernel (experimental) ..... ON
    kernel-test (experimental) .......... ON
    test_bitcoin ........................ OFF

The cache contained `BUILD_KERNEL_TEST:BOOL=ON`; Ninja exposed `test_kernel`,
and `ctest -N` reported `Total Tests: 0`. Building the target completed 142/142
steps and the binary ran all 18 kernel cases with `*** No errors detected`.
The target therefore was built even though the global test switch was off, but
its CTest registration could not be used in that configuration.

### Fix and verification

The smallest fix changes the default argument to `${BUILD_TESTS}` while keeping
the dependency condition as `BUILD_KERNEL_LIB`:

    cmake_dependent_option(BUILD_KERNEL_TEST "Build tests for the experimental bitcoinkernel library." ${BUILD_TESTS} "BUILD_KERNEL_LIB" OFF)

This makes a no-tests configuration omit the kernel test by default without
preventing an explicit `-DBUILD_KERNEL_TEST=ON` request. A fresh default
configuration reported `BUILD_KERNEL_TEST:BOOL=OFF`; the target was absent from
Ninja and `cmake --build ... --target test_kernel` failed with the expected
`unknown target 'test_kernel'`. A separate fresh configuration with
`BUILD_TESTS=OFF -DBUILD_KERNEL_TEST=ON` retained the target, built all 142
steps, and its 18 kernel cases passed. `git diff --check` passed.

Verdict: confirmed configuration dead zone and repaired. The change affects
only the default selection; explicit kernel-test builds and the normal
`BUILD_TESTS=ON` developer configuration retain their behavior.

Limitations: native Linux x86_64 with GCC 16.1.0 was executed. Windows,
macOS, cross builds, shared libraries, and sanitizer configurations were not
repeated because the defect is in CMake option defaulting. No runtime or
consensus behavior changed.

Handoff: next Goal 37 cycle should choose a different conditional target or
platform cell and must not repeat the kernel-test default interaction.
