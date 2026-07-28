# Campaign #75 — build-throughput-cacheability

Base: dc34a4a699 (journal commit for #45 cycle-1 on audit/constant-time;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/build-throughput. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): no-op stability / generator cascade / header fan-out / incremental cost — all clean

### Draw
Random draw over the 32-goal eligible pool
(0 1 2 4 7 9 22 24 28 32 35 40 42 44 46 48 50 54 55 60 61 63 64 67 70 75
76 78 79 80 92 93): raw=5726162901932444281, seed masked to 63 bits,
index 25 -> #75.

### Prior-art check (avoid re-running sibling campaigns)
- #47 c1 (build-ci-parity): registration/manifest/preset parity — clean.
- #37 c1 (build-dead-zones): dead/polarity/orphan cells — clean.
- #17 c1 (build-matrix): option surface vs CI coverage — clean.
None measured build *throughput*: no-op determinism, generated-file
stability, rebuild fan-out, or incremental cost. This cycle takes those.

### Environment (recorded per campaign)
Host: Cortex-A76 (4 cores), gcc 13, CMAKE_BUILD_TYPE=Release, Ninja
generator, build-before (453 MB, WITH_CCACHE=ON). Disk 99% full — no new
build dirs created this cycle; all measurements in-place.

### Cell 1: no-op build determinism — STABLE
Hypothesis H1: a no-op `ninja` does no real work and rewrites nothing.
- 3 consecutive runs: 0.19s wall each (0.12 user / 0.08 sys); only edge
  executed: "Generating bitcoin-build-info.h".
- sha256 of build-before/src/bitcoin-build-info.h identical before/after
  (d09e7c26f687...). Content: `#define BUILD_GIT_COMMIT "dc34a4a699b1"`
  — correct for HEAD.
VERDICT: no unstable generated file. CONFIRMED-stable.

### Cell 2: build-info generator cascade — INTENDED, restat-bounded
Mechanism (src/CMakeLists.txt:46-51): `generate_build_info` is an
add_custom_target (always out of date) with bitcoin-build-info.h as
BYPRODUCT; cmake/script/GenerateBuildInfo.cmake:107-109 rewrites the
header ONLY if the first line changed (content compare, no timestamp
embed). Ninja restat then skips the downstream cascade.
- Dry-run fan-out when the header *would* change: 14 edges =
  1 generate + 1 compile (clientversion.cpp) + 12 links (10 executables
  incl. bitcoind/bitcoin-node/test_bitcoin/bench_bitcoin + 2 libraries).
- That cascade fires only on real git-state change (commit/dirty flip),
  where the version string genuinely changes — correct, not fan-out waste.
- Real per-build cost: the generator's git probes (~0.1s of the 0.19s
  no-op). generate_build_info is the ONLY always-run custom target in the
  default build (translate and libbitcoinkernel phony targets are not in
  ALL). No fork-authored changes to the generator or src/CMakeLists.txt
  (git log --author='pap.lorinc' empty).
VERDICT: DISMISSED as defect; documented as intended design.

### Cell 3: header fan-out table (ninja -t deps inversion, 652 objects)
Reverse-dependency counts over all project headers (method validated:
touch(validation.h) + dry-run = 98 compile edges = 97 predicted + 1
clientversion; mtime restore returned dry-run to the 1-edge baseline,
no real rebuild needed).
Top: attributes.h 536, util/string.h 514, util/check.h 510, span.h 484,
util/strencodings.h 464, compat/byteswap.h 463, compat/endian.h 462,
tinyformat.h 456, crypto/common.h 453 — all small leaf utility headers,
broad by design. Heaviest non-leaf: net.h 191, chain.h 127,
txmempool.h 119, validation.h 97, node/blockstorage.h 98 — none
pathological; no heavy header with project-wide reach.
VERDICT: no broad-header defect. DISMISSED.

### Cell 4: incremental rebuild cost (touch largest TU, validation.cpp)
| run | wall | user | sys | edges |
|---|---|---|---|---|
| ccache-warm | 2.85s | 4.67 | 2.40 | 2 compiles + 5 links |
| CCACHE_DISABLE=1 | 30.78s | 58.32 | 4.42 | 2 compiles + 5 links |
ccache stats: 60.91% hit rate, 98.46% of hits direct (content-based);
mtime-only touches are served from cache. 2 compiles: validation.cpp is
built once for bitcoin_node AND once for bitcoinkernel.

### Cell 5: duplicate compilation inventory (parser fixed after a
break-bug undercount; validated against deps dump)
69 of 554 TUs compile twice: 68 are `X -> {node|common|consensus|util}
+ bitcoinkernel` pairs (libbitcoinkernel is a standalone library target
with its own object set — upstream kernel-isolation design, not a
missing OBJECT-library optimization to "fix" without trading the
kernel's independence), plus bitcoind.cpp -> {bitcoind, bitcoin-node}
(IPC/multiprocess wiring) and secp256k1 tests.c -> {tests,
noverify_tests} (intentional). No triple compiles, no fork-authored
duplication.
VERDICT: DISMISSED as defect; quantified for the record.

### Exact commands
- `ninja -C build-before` x3 + sha256sum build-before/src/bitcoin-build-info.h
- `ninja -C build-before -n` (dry-run edge enumeration, 14-edge cascade)
- `ninja -C build-before -t deps > /tmp/btc75_deps.txt` + python3 reverse
  index (obj_re `^(\S+\.o): #deps`, project headers `../src/*.h`)
- touch + `ninja -n | grep -c 'Building CXX'` + `touch -d "$M"` restore
  (validation.h: 98 -> 1)
- `/usr/bin/time -f ... ninja -C build-before` after `touch src/validation.cpp`,
  warm and CCACHE_DISABLE=1
- `ccache -s`; `grep -rn add_custom_target --include=CMakeLists.txt src/ cmake/ test/`

### Verdict (cycle 1)
DISMISSED: no unstable generated files, no unnecessary rebuild fan-out,
no serialized custom-command bottleneck, no cache-key/pathology on this
base. Build hygiene is upstream-intentional everywhere this cycle looked.
Deliverable is the reproducible profile/fan-out table above.

### Limitations / queue for cycle 2
- Clean-build wall time (ccache-warm, fresh dir, ~450 MB scratch) not
  measured — queued.
- CI cache keys (.github/workflows + ci/ actions) vs irrelevant inputs —
  not audited this cycle — queued.
- Header *cost* (not just fan-out): gcc -ftime-report / clang
  -ftime-trace on the largest TUs — queued.
- Docker/guix layer reuse: not inspected (no docker on host).
- Functional-test build assumptions (test_runner cache dir
  test/cache/) untouched — pre-existing scratch, left alone.
