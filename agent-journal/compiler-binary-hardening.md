# Compiler and binary-hardening configuration audit

## Cycle 122 start

- Initial selector: exact `shuf -i 0-98 -n 1` -> `35` (`mutation-testing`), rejected because the authoritative uber ledger closed that campaign in Cycle 107.
- Reroll selector: exact `shuf -i 0-98 -n 1` -> `91` (`compiler-binary-hardening`).
- Branch: `uber-cycle-122-compiler-binary-hardening-20260730`.
- Cycle-start HEAD: `ba54c3f1fa56430c81b5bbb47bef9132cb7b7065` (`uber-goal: close cycle 121 PSBT boundary audit`).
- Base: `origin/master` at `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; gate divergence `40 1033`.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Fresh `git fetch origin master`, tracked/index checks, and `git diff --check` passed. Persistent untracked catalogs, journals, probes, `node_modules/`, package files, and `test/cache/` are preserved and excluded.
- PID `777094` (`test_bitcoin --run_test=wallet_tests --log_level=test_suite`) and parent PID `725042` were observed and will not be touched.

## Scope and initial queue

Inventory compiler and linker hardening policy in CMake, Guix/depends, CI, and final binaries. Compare supported Linux release/developer configurations first, then inspect portable Windows/macOS/FreeBSD paths and optional targets for silent gaps. Prioritize mechanisms that are expected by the project contract but are absent, inconsistently applied, or only present in compile logs rather than the shipped artifact.

Initial cells:

1. CMake feature probes and target propagation for FORTIFY, stack protection, PIE, RELRO/NOW, separate-code, warnings, and optional hardening flags.
2. Guix/depends hardening versus local CMake release builds and final ELF program headers/symbols.
3. CI/release checks for hardening coverage and whether libraries, tests, tools, fuzzers, and daemons intentionally differ.
4. Compiler-version, optimization, LTO/PGO, shared/static, and cross-platform behavior where a flag silently disappears or changes semantics.

Do not add checkbox hardening. A production change requires a concrete missing diagnostic or blocked defect, a smallest supported patch, final-binary evidence, and build/test/performance compatibility checks. Treat a hardening mechanism as defense in depth unless the threat model and a mutation/fixture show a direct project-relevant gain. Keep negative controls and unsupported-platform limitations explicit.

## Cycle 122 result

### Hypothesis and provenance

The top-level CMake hardening probes added FORTIFY, stack protection, control-flow protection, stack-clash protection, and linker hardening to `core_interface`, but the source comment explicitly exempted the nested C-only libsecp256k1 project. `cmake/secp256k1.cmake` forwarded only `sanitize_interface` options plus explicit append variables. The historical parent of commit `77e553ab6a` used a separate `hardening_interface`; that is project precedent for keeping the flags reusable without copying platform conditions.

The clean Clang 19 Release baseline `/data/my_storage/tmp/cycle105-clang19-release` showed the gap in its generated commands: top-level C++ objects received `-fstack-protector-all` and `-fcf-protection=full`, while `secp256k1.c.o` and both precomputed C objects did not. The baseline `secp256k1.c.o` had no GNU x86 feature note, zero `endbr64` instructions, and no `__stack_chk_fail` import. This is a concrete target-propagation defect, not a claim that the whole binary lacked every hardening feature: the baseline daemon was already PIE, NX, RELRO/NOW, separate-code, and carried an IBT property from other linked objects.

### Fix

`CMakeLists.txt` now keeps the compiler/linker hardening options in `hardening_interface`, links that interface into `core_interface` to preserve existing top-level behavior, and documents the secp exception. `cmake/secp256k1.cmake` retrieves hardening and sanitizer options independently and appends both sets to libsecp CFLAGS/LDFLAGS. The nested C project therefore receives the same compiler-tested hardening options without inheriting unrelated C++ warning flags. Existing `APPEND_CPPFLAGS`, `APPEND_CFLAGS`, and `APPEND_LDFLAGS` remain last in their respective command rules.

### Independent verification

1. Clang 19 Release, IPC disabled because installed Cap'n Proto 0.9.2 is rejected with Clang 19.1.7: `cmake -S . -B /data/my_storage/tmp/cycle122-clang19-release -G Ninja -DENABLE_IPC=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/clang-19 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-19 -DBUILD_TESTS=OFF -DBUILD_GUI=OFF -DENABLE_WALLET=OFF -DWITH_ZMQ=OFF -DWITH_CCACHE=OFF -DBUILD_DAEMON=ON -DBUILD_CLI=ON -DBUILD_UTIL=ON -DBUILD_UTIL_CHAINSTATE=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF`. Configure printed `SECP256K1_APPEND_CFLAGS` with `-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -Wstack-protector -fstack-protector-all -fcf-protection=full -fstack-clash-protection`. `cmake --build /data/my_storage/tmp/cycle122-clang19-release --target bitcoind -j2 --verbose` completed 290/290 steps. The patched secp object has `x86 feature: IBT, SHSTK`, 91 `endbr64` instructions, and `__stack_chk_fail`, `__fprintf_chk`, and `__memcpy_chk` imports; each precomputed object also has the feature note. The final `bitcoind` is an ELF PIE with four separated LOAD segments, non-executable `GNU_STACK`, `GNU_RELRO`, `BIND_NOW`, `NOW PIE`, fortified imports, and 38,492 `endbr64` instructions versus 38,329 in the baseline. `/data/my_storage/tmp/cycle122-clang19-release/bin/bitcoind -version` exited 0.
2. GCC 12.2 Release: configure printed the same `SECP256K1_APPEND_CFLAGS`; `cmake --build /data/my_storage/tmp/cycle122-gcc-release --target secp256k1 -j2 --verbose` completed 4/4 steps, with the exact C compiler command ending in the hardening flags. GCC's `secp256k1.c.o` has `x86 feature: IBT, SHSTK`, 94 `endbr64` instructions, and the expected stack-canary/fortify imports.
3. Source and history verification: the current `core_interface` hardening probes and the pre-`77e553ab6a` dedicated-interface design agree that the options are intended as shared hardening policy. The unchanged final ELF feature note is explained by `/lib/x86_64-linux-gnu/Scrt1.o`, whose property contains IBT only; this environment cannot independently validate full SHSTK in the final executable. The production C objects are nevertheless now instrumented consistently with the executable's IBT policy.

### Gate and limitations

`git diff --check` passed. The local `contrib/guix/security-check.py` could not be executed because Python LIEF is unavailable. Its x86-64 control-flow check inspects only the first four bytes of `main`, so it would not detect an uninstrumented nested-library function; that residual checker coverage is recorded as a separate follow-up cell rather than folded into this source fix. No functional test source was changed. The normal build validation is sufficient for this CMake-only change, while full wallet/IPC configurations remain subject to the documented Cap'n Proto 0.9.2 versus Clang 19 incompatibility.

### Verdict and next queue

Confirmed and fixed: libsecp256k1 C objects were outside the unconditional hardening policy even though they are linked into hardened Bitcoin Core executables. The source patch is ready for one self-contained commit. Remaining distinct cells are the LIEF control-flow check's per-function/property coverage, Guix versus local toolchain parity, Windows/macOS hardening metadata, and LTO/PGO/BOLT behavior.
