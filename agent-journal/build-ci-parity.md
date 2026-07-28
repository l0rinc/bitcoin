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
