# Campaign #70 — compiler-optimization-differential

## Cycle 1 (2026-07-29): LTO + -Wodr build differential — (result pending)

### Draw
Random draw over the 7-goal eligible pool (6 pending + 1 CYCLE-1,
#106 excluded as just-cycled): raw=726564917869240643, index 4 ->
#70 (first cycle). Branch: audit/compiler-optimization-differential
from 8577a2628f (#106 c1 bookkeeping; lineage anchor
audit/resurrection @ 5d0155254c). Start state: tracked-clean.
Catalog note: #70's campaign-focus block holds scheduler/fault-
injection text — same offset artifact class as the 40s/50s/106
region; title+slug authoritative.

### Cell selection / dedup
#36 c2 already did clang-18 -O2 vs gcc-13.3 -O2 full-suite
differential (green, 4 by-design warnings). Distinct cell: LTO.
Cross-TU optimization is where ODR violations surface — the classic
defect class in trees with vendored subtrees (secp256k1 via
subtree, leveldb, crc32c) and header-heavy templates. gcc -Wodr is
the ODR detector; IPO adds whole-program inlining that can also
expose latent strict-aliasing/UB divergence in the unit suite.

### Design
- build-lto: Ninja, Release -O2, gcc 13.3,
  CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON, C/CXX flags -Wodr.
- Baseline: build-before (gcc 13.3 -O2, no LTO), same source,
  suite green.
- Cells: (a) -Wodr warnings at link (count + triage: real ODR
  violation vs known benign, e.g. test fixtures, third-party);
  (b) LTO build success; (c) full unit suite pass/fail differential
  vs baseline.

### Results
- Build: 539 edges, ~23 min (-j4, cold ccache for the new flags),
  exit 0. **Zero -Wodr warnings** at any link (bitcoind,
  test_bitcoin): no ODR violations across main/subtree/vendored TUs.
- Warnings seen: exactly 2, both pre-existing test-code classes,
  not ODR, not LTO-specific (-Woverloaded-virtual in
  test/util/net.h:204; -Wrange-loop-construct in
  test/util_tests.cpp:994 — both fire in non-LTO builds too).
- Suite: full test_bitcoin under the LTO binary -> "No errors
  detected" (exit 0), matching the non-LTO baseline.
- Binary size: test_bitcoin 79.6 MB (LTO) vs 40.3 MB (baseline) —
  LTO object/debug heft as expected on this configuration; size is
  not a defect signal.
- Cleanup: build-lto deleted post-measurement (disk 100%).

### Verdict
DISMISSED: LTO build is green, no ODR violations, no behavioral
divergence vs the -O2 baseline. The vendored-subtree ODR risk class
(secp256k1/leveldb/crc32c template/header duplication) is clean on
this tree at gcc 13.3 -O2 -flto.

### Exact commands
- cmake -B build-lto -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON -DCMAKE_CXX_FLAGS=-Wodr
  -DCMAKE_C_FLAGS=-Wodr
- cmake --build build-lto -j4 --target bitcoind test_bitcoin
- build-lto/bin/test_bitcoin (full suite)

### Limitations / queue for cycle 2
- gcc-only LTO cell; clang LTO (thin) and cross-compiler mixed-LTO
  not covered.
- Functional-suite under LTO not run (unit suite only); the
  functional layer is Python-driven and insensitive to codegen, so
  expected yield is low — queued only if a future signal appears.
- PGO/BOLT cells untouched (profile-build cost on a 100%-full disk;
  needs headroom).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-08-02, draw 199, raw=13766814895528030814, masked 4543442858673255006, idx 30/34): clang thin-LTO cell — HOST-BLOCKED at link (LLVMgold.so absent, no lld); compile side proven; ⚪ with resume condition

### Attempt
cmake -B build-clang-lto (clang-18, Release -g0, IPO=ON,
wallet/tests/IPC off): thin LTO CONFIRMED in the compile rules
(-flto=thin in rules.ninja / -t commands); all objects compiled.
Link: /usr/bin/ld cannot load /usr/lib/llvm-18/lib/LLVMgold.so
(file simply absent — llvm-18 package ships no gold plugin);
-fuse-ld=lld rejected ('invalid linker name' — no lld installed);
only ld.bfd/ld.gold present. Same wall as #44 c2's no-link-LTO
note — consistent host gap, now documented at both points.

### Verdict
⚪ BLOCKED (host toolchain, not a tree defect): clang-LTO linking
is impossible on this host. c1's gcc -O2 -flto differential
(green build, zero ODR warnings, suite green) remains the LTO
cell of record. The compile side of the clang thin-LTO path is
proven (bitcode objects built clean, zero warnings escalated).

### Resume condition
Install lld (or the LLVM gold plugin package) and rerun:
ninja -C build-clang-lto bitcoind (config preserved on disk,
~109 MB of thin-LTO objects staged; relink is the only missing
step), then the #36-c6 6-test functional subset as the
behavioral differential.

### Exact commands
- cmake line above; ninja -t commands grep -flto=thin; link
  failure log (LLVMgold.so absent); -fuse-ld=lld rejection;
  which ld.lld/ld.gold census above.

### Limitations / queue
- PGO/BOLT cells remain disk-blocked (3.8 GB free).
- build-clang-lto kept for the resume (delete on disk squeeze;
  recreate with the cmake line, ~15 min).
