# Campaign #37 — build-dead-zones

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/build-dead-zones. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): parity / empty-arm / #if 0 / DEBUG_LOCK cells — all clean

### Draw
Random draw over the 34-goal eligible pool: raw=17712693127661080655,
index 11 -> #37.

### Prior-art check (avoid re-running sibling campaigns)
#17 c2 (stale preprocessor guard sweep: macro definitions vs guards —
clean) and #29 c1-c3 (dead code — clean) cover the definition-side.
This cycle takes the reachability-side: what no supported build
compiles, polarity mistakes, and declaration/definition parity.

### Cells
1. Declaration/definition parity (free functions declared in
   production headers with no definition or use anywhere):
   1018 candidate declarations extracted; 49 without external hits;
   EVERY one resolved as inline-in-same-header, macro-expanded name,
   or internal lambda/template detail (subprocess.h helpers,
   sync.h criticalblockN macro variables, cuckoocache/prevector
   inline templates, cpuid/byteswap macro aliases). No orphaned
   declaration.
2. Empty-arm conditional blocks (#if/#else/#endif with one empty arm —
   the polarity-mistake shape) across production src: ZERO.
3. Literal dead zones (#if 0 / #if FALSE / #if DEBUG) in production
   src: only DEBUG_LOCKCONTENTION/DEBUG_LOCKORDER and the
   init/common.cpp debug-build version marker — all INTENTIONAL and
   WIRED: documented in doc/developer-notes.md:345-359,
   -DCMAKE_BUILD_TYPE=Debug auto-defines them (CMakeLists.txt:285),
   and the native ASAN CI job passes -DDEBUG_LOCKORDER explicitly
   (ci/test/00_setup_env_native_asan.sh:37). Revivable by design,
   not dead.

### Verdict
- DISMISSED: no dead zones, no polarity mistakes, no orphaned
  declarations, no undocumented disabled features on the audited
  production surface.
- Unsupported-combination note (per campaign labeling):
  Windows/macOS-only sources (compat/*-windows, init/*-darwin) are
  uncheckable on this aarch64 host — recorded as unsupported here,
  not as project contracts.

### Exact commands
- python3 parity extractor (journal history; 1018 decls -> 49 -> 0)
- python3 empty-arm scanner (0 hits)
- `grep -rn '#if 0\|#if FALSE\|#ifdef DEBUG' src/ (filtered)`

### Limitations
- CMake source-list conditions all evaluated for THIS platform; the
  wallet=OFF fuzz cell (#17 c3) confirmed conditional inclusion works
  both ways already.
- qt/ excluded from scope (its own moc/uic machinery).

### Next queue for this campaign
- ipc/libmultiprocess subtree conditional zones (own build system).
- moc/uic generated-source freshness (qt subtree, if a Qt build ever
  lands on this host).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): config-dead production zones — all OFF features properly gated, nothing compiled-but-unrunnable

Base: 707c46a4b2 (journal commit for #39 cycle-2 on
audit/generated-artifact-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/build-dead-zones-c2 (c1 journal carried).
Start state: clean (untracked scratch only).

### Draw
Random draw over the 36-goal pool (23 pending + 13 CYCLE-1; #39
excluded as just-cycled): raw=1786609377964331090, index 26 -> #37.

### Cell: features OFF in build-before but present in-tree
c1 covered parity/dead-arms/#if 0/declaration-reachability. This
cycle checks the CONFIG axis: sources for disabled features must be
excluded from the build, not compiled-and-unreachable.
- ZMQ (WITH_ZMQ=OFF): 0 zmq objects in build.ninja; src/zmq/*
  sources uncompiled; init.cpp ENABLE_ZMQ ifdef gates runtime.
  CLEAN.
- USDT (WITH_USDT=OFF): no usdt/sys/sdt objects. CLEAN.
- bitcoin-chainstate (BUILD_UTIL_CHAINSTATE=OFF): the source
  src/bitcoin-chainstate.cpp exists in-tree but the add_executable is
  gated at src/CMakeLists.txt:406; no .o in build.ninja. CLEAN.
- QREncode: no objects (off in this config). CLEAN.

### Verdict
- DISMISSED: no compiled-but-unrunnable zones; every OFF feature is
  gated at the CMake level, and the only uncompiled sources
  correspond exactly to disabled features. Config-axis dead zones
  none.

### Exact commands
- grep -c zmq.*\.o / usdt / qrencode / chainstate.cpp objects in
  build-before/build.ninja; src/CMakeLists.txt:406-414 gate;
  CMakeCache WITH_ZMQ/USDT/BUILD_UTIL_CHAINSTATE values

### Limitations / queue
- Runtime-dead (compiled but config-inert) paths inside compiled
  files (e.g., -zmq* options accepted but inert when OFF — the init
  arg-check for unknown-but-feature options) not swept this cycle —
  queued.
- Windows-only/mac-only sources unassessed (not compilable on this
  host; labels per campaign: native cells only).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
