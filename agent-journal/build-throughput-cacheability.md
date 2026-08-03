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

## Cycle 2 (2026-07-29): header-cost attribution via clang -ftime-trace on validation.cpp

Base: 92458c9398 (journal commit for #59 cycle-2 on
audit/supply-chain-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/build-throughput-c2 (c1 journal carried).
Start state: clean (untracked scratch only).

### Draw
Random draw over the 26-goal pool (13 pending + 13 CYCLE-1; #59
excluded as just-cycled): raw=6091627946289443426, index 24 -> #75.
Queued cell from c1: "header-cost (-ftime-report/-ftime-trace)".

### Method
clang++-18 with the build's exact validation.cpp command line
(ccache stripped, gcc-only flags removed: -fno-extended-identifiers,
-fstack-reuse=none), -ftime-trace=/tmp/btc75_trace, real compile
(-fsyntax-only would suppress the trace): 24.91s wall, 30.34s
source-attributed in the trace JSON.

### Header cost ranking (validation.cpp frontend time)
| header | ms | note |
|---|---|---|
| validation.h | 6097 | the TU's own template-heavy header, ~20% |
| chain.h | 2503 | CBlockIndex inline |
| params.h | 1463 | consensus params templates |
| feerate.h | 1404 | policy flag header |
| verify_flags.h | 1403 | policy flag header |
| flatfile.h | 981 | blockstorage |
| coins.h | 620 | |
| arith_uint256.h | 583 | |
| hashed_index.hpp | 573 | multi_index machinery |
| strencodings.h | 542 | |
The ranking is shape-consistent with the fan-out map from #75 c1
(leaf headers broad, local header dominant for its own TU). No
anomaly: nothing cheap-looking ranks above the owner's own header,
and the two 1.4s policy-flag headers are the only mild surprise
(worth a pass only if the same cost shows up in many TUs — not
measured this cycle).

### Verdict
- DISMISSED: header costs are dominated by the TU's own template
  surface; no broad-header pathology beyond the c1 map. The
  -ftime-trace method is validated for future per-TU cost work.

### Exact commands
- ninja -C build-before -t commands | grep validation.cpp.o (exact
  flags) -> sed ccache/gcc-only out, add -ftime-trace
- python3 traceEvents['Source'].dur aggregation

### Limitations / queue
- Single TU (validation.cpp); a top-5-TU table (net_processing,
  txmempool, blockstorage, wallet) would rank shared headers — queued.
- Clean-build wall (ccache-warm fresh dir) still open from c1 queue.
- gcc -ftime-report alternative cell (no per-header, per-pass only) —
  clang's trace is the right tool, confirmed.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-29): clean-build wall measured (cold 23.6 min vs warm 1.0 min, 23.8x); CI cache keys assessed sound

### Draw
Re-rank draw over the 2 remaining CYCLE-2+ open cells:
raw=203410285419299450, index 0 -> #75 (third cycle; c1/c2 queue
cell "clean-build wall, CI cache keys"). Branch:
audit/build-throughput-c3 from 4340ba0f87 (#45 c3 bookkeeping).

### Clean-build wall (this host, aarch64 A76 4-core, Ninja, Release)
- Cold: fresh dir + CCACHE_DISABLE=1 (real compile cost, c1's
  method note): 1418.2s wall (23.6 min), 631 edges, exit 0.
- Warm: fresh dir + session-hot ccache (same flags/content as
  build-before): 59.6s wall, exit 0.
- Ratio 23.8x. The warm residual is link time + the secp256k1
  subtree recompiles (c2's absolute-I key divergence — secp calls
  miss the content cache and recompile every fresh dir; the warm
  build's last edge was the secp noverify_tests link, consistent).
- Both dirs deleted post-measurement (disk 100%).

### CI cache keys (GitHub Actions ci.yml:164-194)
Key = job + job-type + run_id with restore-prefix job+job-type;
save only on default branch on miss (the actions/cache update
workaround). Correctness does NOT depend on the key: ccache's own
content key includes the compiler binary (CCACHE_COMPILERCHECK
mtime) and flags, so a stale-prefixed restore can only waste space,
never serve wrong objects. Design sound (and upstream-identical per
#59 c2's byte-parity).

### Verdict
Findings of fact: the cold/warm walls are now measured on-record
(23.6 min / 1.0 min); ccache is a 23.8x lever here and its CI key
design cannot serve wrong artifacts. No defect.

### Exact commands
- cmake -B build-cold -G Ninja -DCMAKE_BUILD_TYPE=Release;
  CCACHE_DISABLE=1 cmake --build build-cold -j4 (1418.2s)
- same for build-warm without the disable (59.6s)
- reads: .github/workflows/ci.yml:162-194, ci/test/00_setup_env.sh
  :51-61, ci/test/03_test_script.sh:113-145

### Limitations / queue
- Single cold sample (no variance run; build hosts are noisy).
- The 45-uncacheable ccache calls STILL unitemized (c2/c3 queue;
  needs CCACHE_DEBUG rebuild — disk-bound).
- IPC/capnp generated-code build share not split out.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-07-29): the "45 uncacheable calls" itemized — 57/58 are failed compilations (rotation's own mutants)

### Draw
Re-rank draw (last of the rebuilt 3-cell queue; singleton):
#75 (fourth cycle; c2/c3 queue "45-uncacheable itemization").
Branch: audit/build-throughput-c4 from cd67d779a6 (#80 c3
bookkeeping).

### Itemization (ccache -s -v)
Uncacheable calls: 58/31598 (0.18%) — "Compilation failed: 57/58
(98.28%)", "Preprocessing failed: 1/58". No capnp/IPC-generated
class at all. The count grew 45 -> 58 over the session as the
rotation's intentional failure experiments accumulated (serialize.h
mutant syntax slip, probe link/compile failures, aborted builds).
ccache counts failed compilations as uncacheable by construction.

### Verdict
DISMISSED: the uncacheable class is the rotation's own
expected-failure residue, not a cacheability defect. The c2
hypothesis (IPC/capnp-generated) is refuted by the breakdown. No
action; cache posture otherwise healthy (58.25% hit rate, 3.4/50
GB).

### Exact commands
- ccache -s; ccache -s -v (uncacheable breakdown)

### Limitations
- The 1 preprocessing failure is unitemized (likely one of my
  broken probes; cosmetically unresolved, immaterial).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 5 (2026-08-02, draw 196, raw=1891301078475215477 (63-bit), idx 33/37): post-session cache posture — uncacheable 58->85 still 96.5% own-failure residue; one NEW minor class (6x input-modified-during-compilation = own concurrent edits); hit rate steady ~58%; DISMISSED

### Hypothesis
The session's variant builds (clang -g0, kernel-shared, many
probe compiles) could have introduced a new uncacheable class
or cache pressure beyond c4's finding.

### Evidence (ccache -s / -s -v)
- Uncacheable 85/41907 (0.20%): 'Compilation failed' 82 (96.5%),
  'Preprocessing failed' 3 — same own-probe-failure class as c4
  (optdiff/instantiate/merkle-driver link attempts etc.); no
  generated-code/IPC class appeared.
- NEW minor category: Errors 6/41907 = 'Input file modified
  during compilation' (100%) — traces to this session's
  concurrent editing while background builds ran (stash/pop +
  sed-on-live-tree); benign and self-inflicted, recorded so a
  future reading doesn't misattribute it to the build system.
- Hit rate 57.89% steady; cache 5.0/50 GB (10.08%) — no
  pressure. Disk free 3.8 GB (build dirs are the pressure, not
  ccache).

### Verdict
DISMISSED: cacheability posture unchanged in class; the only new
entries are the rotation's own failure/edit residue. No action.

### Exact commands
- ccache -s; ccache -s -v; df -h.

### Limitations
- The 6 modified-during-compile instances are not individually
  attributed (timestamps not kept); class-level attribution
  suffices (no daemon/CI ran concurrently).

## Cycle (2026-08-03, cycle-324 r55, raw=6487531322071883467 -> idx 75): CI cache-key arm assessed — no defect

ccache restore (ci.yml:169-173): primary key
job+job-type+ccache+run_id (unique per run), prefix restore-key
(rolling cache). Content-addressed ccache makes stale restores
harmless; per-job-type isolation intact; no cross-branch poisoning
path (restored entries only accelerate). vcpkg binary/downloads
caches keyed by computed primary keys (steps.*.outputs).
actions/cache@v5 current. Verdict: no defect; queue arm closed.
