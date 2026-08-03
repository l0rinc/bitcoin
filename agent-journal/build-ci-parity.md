# Build-system and CI parity audit

## Cycle 318 selection and scope

- Exact selector: `shuf -i 0-117 -n 1` -> `47` (`build-ci-parity`).
- Branch: `uber-cycle-318-build-ci-parity-20260802`.
- Cycle-start HEAD: `39eebea9546c197d2795d1b39139e23d0000a51d`.
- Selection commit: `f494232c98`.
- Catalog SHA: `82fe4d3eefbf266fa38a76d9949344be6e8c454a881c0411fa567fa010c34019`.

Prior build-matrix cells were searched before selecting this surface. The
chainstate/kernel option dependency was fixed in Cycle 311 and is not reopened
here. Prior reduced-export, sanitizer, fuzz-mode, supply-chain pinning, and
generic build-dead-zone cells are likewise excluded. This cycle compares the
actual CI jobs and helper scripts with current CMake target conditions, with
special attention to generated artifacts and feature-specific verification.

## Matrix and target map

The native Windows standard job invokes `.github/ci-windows.py` with
`BUILD_KERNEL_LIB=ON` and `BUILD_UTIL_CHAINSTATE=ON`. The `dev-mode` preset also
enables kernel tests, benchmarks, unit tests, wallet tools, GUI, and the other
standard targets. The Windows cross build uses the same preset and its manifest
checker validates every `.exe` except `fuzz.exe` and `bench_bitcoin.exe`.

The CMake target map attaches `add_windows_application_manifest` to
`bitcoin-chainstate` in the same conditional block that creates the executable:

```text
if(BUILD_UTIL_CHAINSTATE)
  add_executable(bitcoin-chainstate ...)
  add_windows_application_manifest(bitcoin-chainstate)
endif()
```

`cmake/module/AddWindowsResources.cmake` implements this function by generating
`bitcoin-chainstate.manifest` and a resource file whenever `WIN32` is true.
The target has had this call since the pure-kernel chainstate executable was
introduced in commit `7990463b1059ba5fc4ebe37fd1105a9e168ae20d` on 2024-06-14.

## Candidate: native Windows manifest coverage

`.github/ci-windows.py` nevertheless placed `bitcoin-chainstate.exe` in the
native standard job's `skips` set with the message `no manifest present`.
This exclusion was introduced when the checker was added in
`fa3f89acaa7ae3332f4bf13aa91fcff44902990c` on 2026-02-06. The later kernel-test
change removed the analogous stale `test_kernel.exe` exclusion, but left
`bitcoin-chainstate.exe` behind. The cross-build checker, moved to
`.github/ci-windows-cross.py` by `faf738946668b6ec16de37298e5a1da23ed77222`,
does not skip chainstate and therefore already exercises the expected check.

This is a CI-only coverage defect: native Windows builds produce a target that
CMake declares to contain a manifest, but the native verifier silently omits
it. It is not evidence that the executable lacks a manifest, and it is not a
claim that a Linux host can execute `mt.exe`.

## Fix and verification

The smallest repair removes only `bitcoin-chainstate.exe` from the native
checker skip set. Fuzz, benchmark, and Qt unit-test exclusions remain because
their current CMake target definitions do not attach application manifests.
The native and cross checkers now cover the same manifest-bearing chainstate
target. This repair is committed as `10aaa92d97`.

Static contract verification:

```text
git grep -n 'add_windows_application_manifest(bitcoin-chainstate)' -- src/CMakeLists.txt
git grep -n -A8 -B4 'function(add_windows_application_manifest' -- cmake/module/AddWindowsResources.cmake
git grep -n -A8 'skips = {' -- .github/ci-windows.py .github/ci-windows-cross.py
```

The first command finds the target resource attachment; the second confirms
the `WIN32` guard and generated resource; the third shows that neither Windows
checker skips chainstate after the change. `python3 -m py_compile
.github/ci-windows.py .github/ci-windows-cross.py` and `git diff --check` pass.

An actual MSVC/`mt.exe` run is unavailable on this Linux host. The independent
cross-script comparison and the CMake/history evidence are the verification
forms retained for this CI-only finding. A Windows CI run remains the required
platform execution check.

## Verdict

**Confirmed and fixed.** The native Windows manifest job had a stale exclusion
for a CMake target that is explicitly assigned an application manifest and is
already checked by the cross-build job. No production binary or runtime path
changed.

## Learning and next queue

This cycle adds a reusable parity rule: for each generated or embedded artifact,
compare the source target attachment, every native/cross checker, and each
exception list; a historical `no artifact` comment must be revalidated after
target creation changes. Next inspect other duplicated native/cross checks,
especially executable manifests, test target inventories, install/deploy
outputs, and feature flags that are present in one Windows job but not the
other. Exclude this chainstate manifest cell unless the target, generator, or
platform behavior changes.
