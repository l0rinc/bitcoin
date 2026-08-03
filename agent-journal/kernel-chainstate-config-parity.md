# Kernel library and chainstate configuration parity

## Seeded from Cycle 311

Cycle 311 Goal 17 found and fixed a CMake option-contract defect: `BUILD_UTIL_CHAINSTATE=ON` with `BUILD_KERNEL_LIB=OFF` configured successfully, advertised `bitcoin-chainstate`, and generated `ENABLE_BITCOIN_CHAINSTATE=true`, but omitted the target because `src/CMakeLists.txt` nested it under the kernel-library guard. The fix now rejects that impossible combination early.

This follow-up must rebase on the fix and audit the remaining kernel/chainstate matrix. Keep configuration-only experiments under `/data/my_storage/tmp/cycle-*`; the host filesystems are full, so do not start a large clean kernel build unless an existing artifact can be reused safely.

## Initial questions

- Does every accepted combination expose exactly the targets and functional capability flags it promises?
- Do `BUILD_KERNEL_TEST`, `BUILD_UTIL_CHAINSTATE`, `REDUCE_EXPORTS`, `BUILD_FOR_FUZZING`, and cross/Windows presets preserve dependency closure?
- Do install components, `libbitcoinkernel.pc`, public headers, and generated manifests agree with target availability?
- Are CMake, CI, and depends/Guix recipes consistent about the experimental library and utility?

## Required evidence

Record each cache vector, configure result, summary, target-help membership, generated `test/config.ini`, install manifest or component graph, exact warnings/errors, and any reused build artifact. Compare accepted and rejected cells against history and prior journals. Treat unavailable toolchains as limitations, not source defects. A finding needs an exact reproducible mismatch plus an independent target, metadata, or runtime verifier; otherwise classify it and leave the next matrix cell queued.

## Next queue

1. `BUILD_KERNEL_LIB=ON` with chainstate OFF versus both ON: compare test/kernel, install, pkg-config, and public-header outputs.
2. Exercise `BUILD_KERNEL_TEST` cache overrides, fuzz forced-off behavior, and reduced-export/shared or cross configurations.
3. Check Windows and functional-test runner metadata for stale binary paths or component names.
