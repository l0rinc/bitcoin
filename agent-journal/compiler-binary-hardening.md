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

## Cycle 223 - relocation-referenced control-flow targets

### Identity and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `91` (`compiler-binary-hardening`);
  no reroll. Branch: `uber-cycle-223-compiler-binary-hardening-20260731`.
  Start HEAD was `ee5e8b6e54500181f5c1ec6c70c43cda0f202709`; `origin/master`
  was `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base was
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `42 1232`.
- Catalog SHA-256 was
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`;
  prompt, TSV, and uber-protocol hashes were unchanged. Tracked state was
  clean at the gate apart from known untracked agent artifacts. Protected
  PIDs `777094`, `956381`, `1138182`, and `1157959` remained alive.
- The Cycle 122 libsecp256k1 propagation finding was closed and excluded.
  This cycle selected its independent remaining cell: release checker
  coverage for control-flow hardening.

### Hypothesis and applicability

`contrib/guix/security-check.py` checked x86-64 control-flow instrumentation by
reading four bytes only at the `main` symbol. A release binary could therefore
retain an `endbr64` at `main` while an address-taken function lacked an
`endbr64`, and the release gate would still pass. This is a real configuration
failure, not only a pattern-match concern: an indirect call to that function
is exactly the control-flow transfer that Intel CET IBT is intended to guard.

The repository pins LIEF `0.17.5` in `ci/lint/requirements.txt`; that wheel was
installed only under `/data/my_storage/tmp/cycle223-lief`. Existing release-like
`bitcoind` and `bench_bitcoin` binaries were used as positive controls. The
current checker returned exit 0 for both before and after the change.

### Independent regression probe

The scratch probe
`/data/my_storage/tmp/cycle223-hardening/indirect-nocf-target` was compiled
with GCC using `-O2 -fcf-protection=full -fstack-protector-all -fPIE -pie
-D_FORTIFY_SOURCE=3 -Wl,-z,relro,-z,now,-z,ibt`. It contains a
`noinline,nocf_check` function stored in a volatile callback and invoked by an
indirect call. The binary has an IBT GNU property and these entry bytes:

- `main` at `0x1080`: `f3 0f 1e fa` (`endbr64`)
- `unprotected` at `0x11e0`: `48 83 ec 38` (no `endbr64`)
- relocation `R_X86_64_RELATIVE` at `0x4010` materializes addend `0x11e0`.

The unmodified checker returned exit 0 for this binary. A second
`-fcf-protection=none` binary failed the existing `main` check, proving the
negative control was sensitive to the original condition. The counterexample
also contains indirect `call *%rax` instructions. Its process returned 0 on
this host, so no claim is made about kernel CET enforcement; the ELF property,
relocation, instruction bytes, and call target establish the checker false
negative independently.

### Fix

`check_ELF_CONTROL_FLOW` still checks `main`, then builds the set of LIEF
function addresses and resolves each remaining relocation's `addend` plus
symbol value. When a relocation materializes a known function address, its
first four bytes must also be `endbr64`. This avoids rejecting `_start`, CRT
helpers, and PLT stubs that are not represented by address materialization,
while catching the callback shape above. The implementation is intentionally
limited to the existing x86-64 check; it does not pretend to prove every
possible jump-table or architecture-specific indirect target.

Source commit: `guix: check relocated control-flow targets`, authored as
`Lőrinc <pap.lorinc@gmail.com>`. The diff is 21 insertions and 4 deletions in
`contrib/guix/security-check.py`; no production Bitcoin runtime code changed.

### Verification

- `PYTHONPATH=/data/my_storage/tmp/cycle223-lief python3
  contrib/guix/security-check.py /data/my_storage/tmp/cycle122-clang19-release/bin/bitcoind`
  exited 0 after the change.
- The same command on
  `/data/my_storage/tmp/cycle105-clang19-release/bin/bench_bitcoin` exited 0.
- The indirect callback probe exited 1 with `failed CONTROL_FLOW` after the
  change; the fully uninstrumented `no-cf-main` probe also exited 1.
- Direct Python `compile(...)` of the script and `git diff --check` passed.
  `test/lint/lint-python.py` exited 0 but skipped mypy because it is not
  installed; LIEF was present at the pinned 0.17.5 version.
- No full Guix cross-build, PE build, Mach-O build, or ARM64 ELF artifact was
  available in this environment. These remain explicit limitations rather
  than inferred passes.

### Verdict and handoff

- **Confirmed and fixed:** the release security gate had a control-flow
  instrumentation false negative for relocation-referenced indirect targets.
  The fix is self-contained and preserves positive and negative controls.
- Remaining distinct cells are ARM64 ELF branch-protection/property checking,
  Guix-versus-host linker metadata, Windows/macOS artifact coverage, and
  LTO/PGO/BOLT behavior. The next selection must avoid reopening this closed
  x86 relocation cell.

## Cycle 236 - ARM64 ELF branch-protection gate

### Identity and scope

- Exact selector: `shuf -i 0-98 -n 1` -> `91` (`compiler-binary-hardening`);
  no reroll. Branch: `uber-cycle-236-compiler-binary-hardening-20260731`.
  Cycle-start HEAD was `7057a8079a14ca7613386cd4539741ee6dacc2bc` (`agent:
  close cycle 235 handoff`); `origin/master` was
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base was
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `42 1255`.
  The fresh gate passed, catalog/prompt/TSV/uber-protocol hashes were exact,
  tracked/index state was clean, and protected PIDs `777094`, `956381`,
  `1138182`, and `1157959` remained alive.
- Cycle 122's libsecp256k1 C-object propagation finding and Cycle 223's x86-64
  relocation-referenced ENDBR finding were excluded. This cycle targets the
  separate ARM64 ELF release-check coverage cell.

### Contract and old behavior

The current CMake hardening policy adds `-mbranch-protection=standard` on
non-Darwin AArch64 targets. The Guix GCC package is also configured with
`--enable-standard-branch-protection=yes`. For the resulting Linux ARM64 ELF,
the linker property `GNU_PROPERTY_AARCH64_FEATURE_1_AND` must advertise both
BTI (bit 0) and PAC (bit 1). `readelf -n` on a direct Clang 19
`-mbranch-protection=standard` artifact reported `AArch64 feature: BTI, PAC`;
the same source with `-mbranch-protection=none` had no AArch64 feature note.

Before the fix, `contrib/guix/security-check.py` mapped
`lief.Header.ARCHITECTURES.ARM64` to `BASE_ELF` only. Its positive and
negative ARM64 PIE artifacts both passed every registered check, so a release
binary could omit the branch-protection property while the release gate still
reported success. This is a configuration-gate false negative: the build
policy already requests the defense, but final-artifact verification did not
test it.

### Deterministic independent verification

The artifacts were produced without an ARM64 sysroot using Clang 19's
AArch64 target and LLD, with the same relevant final-binary controls:

```text
clang --target=aarch64-linux-gnu -O2 -fPIE -fstack-protector-all \
  -D_FORTIFY_SOURCE=3 -mbranch-protection=standard -nostdlib -nostartfiles \
  -nodefaultlibs -fuse-ld=/usr/lib/llvm-19/bin/ld.lld -Wl,-e,main \
  -Wl,-z,relro,-z,now,-z,separate-code \
  -Wl,--unresolved-symbols=ignore-all -x c \
  -o /data/my_storage/tmp/cycle236-aarch64-hardened /dev/stdin
clang --target=aarch64-linux-gnu -O2 -fPIE -fstack-protector-all \
  -D_FORTIFY_SOURCE=3 -mbranch-protection=none -nostdlib -nostartfiles \
  -nodefaultlibs -fuse-ld=/usr/lib/llvm-19/bin/ld.lld -Wl,-e,main \
  -Wl,-z,relro,-z,now,-z,separate-code \
  -Wl,--unresolved-symbols=ignore-all -x c \
  -o /data/my_storage/tmp/cycle236-aarch64-unprotected /dev/stdin
```

The source used a retained `--monolithic` marker so the existing FORTIFY
exception applied; both artifacts were PIE, RELRO/NOW, NX-stack, and
stack-canary controls. Before the source change, running the checker with the
pinned LIEF 0.17.5 environment exited 0 for both binaries. After the change,
the hardened artifact exited 0 and the unprotected artifact exited 1 with
`failed BRANCH_PROTECTION`. Direct calls to the new predicate returned
`True` and `False` respectively. The repaired ARM64 artifact passed all seven
registered checks; existing Clang 19 x86 `bitcoind` and `bench_bitcoin`
artifacts also passed unchanged.

The implementation parses GNU property-note descriptors as little-endian
`type`, `size`, and padded data records, requires the AArch64 feature property
to be exactly four bytes with both BTI and PAC bits, and rejects truncated or
zero-sized unknown records. Synthetic malformed-note controls returned false
without hanging. The property note is the linker-aggregated AND contract, so
the check verifies the final artifact rather than relying only on compile-log
flags.

### Fix, validation, and limits

`check_ELF_BRANCH_PROTECTION` is now registered for ARM64 ELF outputs. No
runtime Bitcoin code or CMake policy was changed: this closes the release
checker gap while preserving the existing build policy. `python3 -m
py_compile contrib/guix/security-check.py`, `git diff --check`, and
`test/lint/lint-python.py` completed; the latter intentionally skipped mypy
because it is not installed. No full Guix cross-build, ARM64 execution,
Windows PE artifact, macOS artifact, or LTO/PGO/BOLT build was available.
Those remain separate queue cells. The current artifact uses an unresolved
libc/sysroot-independent link only to isolate the checker contract; it is not
presented as a release binary.

Verdict: **confirmed and fixed**. The next selection must not repeat this
ARM64 ELF property cell; remaining Goal 91 cells are Guix-versus-host linker
metadata, Windows/macOS artifact coverage, and LTO/PGO/BOLT behavior.
