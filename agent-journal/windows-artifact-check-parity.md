# Windows native and cross-build artifact-check parity audit

## Seed from Cycle 318

Cycle 318 selected the broader `build-ci-parity` goal and found that the native
Windows manifest checker skipped `bitcoin-chainstate.exe` even though CMake
attached an application manifest and the cross-build checker validated it.
The stale exclusion was removed in commit `10aaa92d97` and is prior evidence,
not a target for rediscovery.

This goal narrows the next campaign to duplicated Windows artifact checks.
Compare target creation and generated resources in CMake with native MSVC,
MinGW cross-build, install/deploy, and generated test-configuration consumers.
Focus on a distinct target or artifact contract, or on a checker asymmetry that
survives the chainstate repair. Use source history to explain every exception,
and preserve the exact target/skip map in the journal.
