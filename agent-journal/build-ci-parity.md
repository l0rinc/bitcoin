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

## Cycle 3 (2026-08-01): export-set consumer check — downstream compiles, statically links, and RUNS against the installed kernel via its .pc; DISMISSED

### Draw
RE-RANK draw 152 over a re-harvested 10-cell pool: raw=
1749937186513557252 (already 63-bit) -> idx 2 -> kernel export-set
consumer check (c2 queue). Branch: audit/build-ci-parity-c3 from
cf949ccfea.

### Experiment
- cmake --install build-before to a scratch prefix: static
  libbitcoinkernel.a (14 MB) + include/bitcoinkernel.h +
  lib/libpkgconfig/libbitcoinkernel.pc.
- .pc is byte-identical to upstream master (no Libs.private
  upstream either); prefix=@CMAKE_INSTALL_PREFIX@ (standard
  pkg-config semantics — installing elsewhere than the configured
  prefix breaks paths; the honest flow is to configure the prefix,
  which I did via -DCMAKE_INSTALL_PREFIX re-set + reinstall).
- Consumer (/tmp/btc47c3/consumer.c, preserved): btck_context_
  create(NULL) -> btck_chainstate_manager_options_create (with
  data/blocks dirs) -> set_worker_threads_num(0) -> destroy both.
- Link with ONLY `pkg-config --cflags --libs libbitcoinkernel`:
  gcc fails with 12,992 undefined refs (all C++ stdlib — expected:
  the kernel is C++; the .pc correctly assumes a C++ link driver);
  g++ links with ZERO undefined references — the static lib is
  fully self-contained (all dependency objects folded in, no
  Libs.private needed).
- Run: CONSUMER-OK, rc=0, chain/blocks dirs auto-created.

### Verdict
DISMISSED: the installed export set is complete and consumable
end-to-end (headers, static lib closure, .pc flags). The gcc-vs-g++
layer is a standard C++-library property, not a defect.

### Exact commands
- cmake --install build-before --prefix (after
  -DCMAKE_INSTALL_PREFIX=<prefix> reset); consumer.c compile lines
  above; link logs /tmp/btc47c3_link{,2}.log.

### Limitations / queue
- Shared-kernel-lib consumer variant not run (this config builds
  the static lib; #91 c3 measured the shared variant's exports).
- Remaining #47 cells: none queued.

## Rotation note
Cycle 3 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 4 (2026-08-01, draw 170, raw=16461226049798957217, masked 7237854012944181409, idx 1/3 -> #10 DESCOPED-confirmed (wallet-only, zero non-wallet refs, same ruling as draw 104a); redraw raw=6266662857617590820 (63-bit) idx 0/2): shared-kernel-lib consumer — downstream compiles, dynamically links with full closure, and RUNS against the installed libbitcoinkernel.so via its .pc; DISMISSED; campaign COMPLETE

### Hypothesis
H: the SHARED kernel variant's installed export set (header, .so,
.pc) is incomplete for a downstream consumer (missing symbols,
under-exported API, bad .pc). Falsifiable by install + compile +
ldd -r closure + run, mirroring c3's static variant.

### Evidence
- Build: cmake -B build-kernel-shared -G Ninja Release -g0,
  BUILD_SHARED_LIBS=ON, BUILD_KERNEL_LIB=ON (defaults OFF via
  BUILD_UTIL_CHAINSTATE; option at CMakeLists.txt:114), wallet/
  IPC/tests off. Target: ninja bitcoinkernel ->
  lib/libbitcoinkernel.so.
- Install quirk recorded: full `cmake --install` fails wanting
  bin/bitcoin (not built in this minimal config); component
  install works: --component libbitcoinkernel (components named
  at src/kernel/CMakeLists.txt:123-137) -> .pc + .so + header.
- .pc identical in shape to c3's static one (prefix=/usr/local
  from configure time; standard pkg-config semantics; used
  --define-variable=prefix to override).
- Consumer: c3's preserved /tmp/btc47c3/consumer.c re-pointed to
  /tmp/btc47c4 (context create -> chainstate_manager_options
  create with data/blocks dirs -> set worker threads 0 ->
  destroy both).
  g++ -O1 consumer.c $(pkg-config --cflags --libs) ->
  links clean. ldd -r with LD_LIBRARY_PATH: 0 undefined symbols
  (full closure through the .so). nm -D: 134 btck_* exports.
  Run: CONSUMER-OK rc=0, chain/blocks dirs auto-created.
- g++ driver needed exactly as in c3 (C++ runtime); not a defect.

### Verdict
DISMISSED: shared export set is complete and consumable
end-to-end, matching the static variant's c3 result and #91 c3's
export measurement.

### Campaign #47: COMPLETE
c1 registration/preset cells; c2 install manifest single-source;
c3 static consumer; c4 shared consumer. No cells remain queued.

### Exact commands
- cmake/ninja/install lines above; consumer compile/run lines
  above; ldd -r / nm -D counts above.

### Limitations
- build-kernel-shared kept (Release -g0); delete on disk squeeze.
- Consumer exercises create/options/destroy only (same scope as
  c3); full validation-drive through the C API is a kernel-test
  matter, not an export-parity one.
