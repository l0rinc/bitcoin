# Campaign #47 — build-ci-parity

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/build-ci-parity. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): registration/manifest/preset parity — 4 cells, all clean

### Draw
Random draw over the 43-goal eligible pool: raw=1520125998177041934,
index 21 -> #47.

### Cells (mechanical set comparisons, not eyeball)
1. Functional test registration: test/functional/*.py (274) vs
   test_runner.py BASE_SCRIPTS quoted entries (272) + const-registered
   TOOL_BENCH_SANITY_CHECK (96/100) + framework exclusions
   (test_runner, combine_logs, create_cache, gen_hint): 274/274
   accounted. REGISTERED-BUT-MISSING-FILE: none (no stale entries).
   First pass missed the constant-referenced script — extraction
   artifact, corrected by including '^[A-Z_]+ = "..."' assignments.
2. Fuzz sources: src/test/fuzz/*.cpp (138) vs CMakeLists entries:
   all listed except util.cpp, which is the test_fuzz utility library
   via src/test/fuzz/util/CMakeLists.txt (confirmed built there:
   test_fuzz.dir/__/util.cpp.o). No unregistered fuzz target.
3. Bench sources: src/bench/*.cpp (58) vs src/bench/CMakeLists.txt:
   58/58 listed.
4. Presets vs CI: ci/test/00_setup_env_*.sh uses --preset=dev-mode
   (arm/freebsd/i686 jobs); CMakePresets.json provides dev-mode with
   the full expected cache-variable set (BUILD_TESTS, BUILD_BENCH,
   BUILD_FUZZ_BINARY, BUILD_KERNEL_LIB, BUILD_UTIL_CHAINSTATE...).
   vs2026(-static) and libfuzzer(-nosan) presets have no CI consumer
   but are documented developer entry points — intentional asymmetry.

### Verdict
- DISMISSED: no silently-skipped tests, no stale lists, no
  preset/CI drift on this base.
- Context note (#66 lineage): the fork's own added test registrations
  (wallet_rescan_failure_log.py, wallet_rescan_timesmart.py,
  wallet_rescan fuzz target) live on side branches and were
  registered atomically with their tests by construction; parity on
  this base is unaffected.

### Exact commands
- python3 set comparisons (journal history): functional files vs
  test_runner.py literals + ALL_CAPS constants; fuzz/bench CMake
  source lists; CMakePresets.json names vs ci/test/*.sh --preset usage.

### Limitations
- Install/export manifests (cmake install rules vs packaged file
  lists) not diffed this cycle — queued.
- Cross/emulator job evidence: not run locally (labels per campaign:
  these cells are native, static).

### Next queue for this campaign
- install(EXPORT)/package file parity: cmake_install.cmake target
  lists vs debian/contrib packaging expectations.
- vs2026 preset validation on a Windows host (artifact-only here).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-28): install/export manifest parity — declarative single source, exact match

Base: 9b5b1ab957 (journal commit for #9 cycle-2 on audit/hit-freq-c2;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/build-ci-parity-c2 (c1 journal carried in the carry
commit). Start state: clean (untracked scratch only).

### Draw
Random draw over the 63-goal repaired pool (40 pending + 23 CYCLE-1;
#9 excluded as just-cycled): raw=17593814728281041831, seed masked to
63 bits (8370442691426266023), index 48 -> #47. Queued cell from c1:
"install(EXPORT)/package file parity".

### Method
`cmake --install build-before --prefix /tmp/btc47_inst` (no rebuild;
build-before = gcc Release, wallet/IPC/bench/kernel ON, GUI OFF,
BUILD_UTIL_CHAINSTATE OFF) -> 17 installed files. Declared rules:
cmake/module/InstallBinaryComponent.cmake (ONE function:
install_binary_component(<t> [HAS_MANPAGE] [INTERNAL]); INTERNAL ->
libexec else bin; manpage iff INSTALL_MAN AND HAS_MANPAGE) + kernel
export install (src/kernel/CMakeLists.txt:125: lib + header +
libbitcoinkernel.pc).

### Parity results (declared vs installed, this config)
- bin + manpage: bitcoin, bitcoind, bitcoin-cli, bitcoin-tx,
  bitcoin-util, bitcoin-wallet — 6/6 declared, 6/6 installed,
  manpages 6/6 (doc/man has bitcoin-qt.1 additionally; installed only
  in GUI configs — correct).
- libexec (INTERNAL, no manpage): bench_bitcoin, bitcoin-node,
  test_bitcoin — 3/3 (bitcoin-chainstate and qt internals off in this
  config).
- Kernel export: include/bitcoinkernel.h + lib/libbitcoinkernel.a +
  lib/pkgconfig/libbitcoinkernel.pc; .pc has configure-time
  prefix=/usr/local (standard CMake behavior for --install --prefix
  overrides; version 31.99.0 matches the tree).
- Built-but-not-installed: test_kernel only (10 built vs 9 installed)
  — no install_binary_component call for it; shipped tests are
  intentionally limited to test_bitcoin (+qt variant). Intentional.
- Packaging consumers: Guix security/symbol checks run over
  INSTALLPATH/bin/* + libexec/* (build.sh:200-203) — matches the
  INTERNAL split exactly. NSI (share/setup.nsi.in) packages the whole
  dist tree, enumerating no binaries — nothing to drift.
  contrib/debian retains only a copyright stub (no install manifests
  to drift).

### Verdict
DISMISSED: the install/export manifest is declarative-single-source
(one function, INTERNAL/HAS_MANPAGE flags); observed install set
matches the declared rules exactly for this config; consumers align;
no stale, missing, or leaky entries.

### Exact commands
- cmake --install build-before --prefix /tmp/btc47_inst
- find /tmp/btc47_inst -type f | sort  (17 files)
- cat cmake/module/InstallBinaryComponent.cmake;
  grep -rn install_binary_component src/ (12 call sites)
- cat /tmp/btc47_inst/lib/pkgconfig/libbitcoinkernel.pc
- grep audit: contrib/debian/, share/setup.nsi.in, doc/man/,
  contrib/guix/libexec/build.sh:200-203

### Limitations / queue
- GUI-config install parity (bitcoin-qt, bitcoin-gui, test_bitcoin-qt,
  bitcoin-qt.1) not verifiable on this headless host — noted, not a gap
  in the rules themselves.
- Export-set consumer check (a downstream project actually linking
  libbitcoinkernel via the .pc) not done — queued for a kernel-API
  cycle.
- vs2026 preset validation still artifact-only (Windows host needed).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
