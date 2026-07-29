# Recent Performance-Regression Bisect

## Cycle 86 start

- The first exact selector draw was `shuf -i 0-98 -n 1` -> `71` (`deterministic-simulation`); it was rejected because Cycle 84 closed that campaign and no new schedule or caller evidence exists.
- The accepted exact reroll was `shuf -i 0-98 -n 1` -> `25` (`performance-regression-bisect`).
- Branch: `uber-cycle-86-performance-regression-bisect-20260729`.
- Cycle-start HEAD: `1d1db50f3f5fe1002c0701a7b1a8fe78ec9338aa` (`journal: close loupe pipeline cycle 85`).
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` is `2 959`.
- Dirty state: only preserved unrelated untracked agent/user artifacts and `test/cache/` are present; no tracked edits at initialization. No relevant process is running.
- Catalog/protocol/manifest hashes: catalog `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.

## Campaign contract

For each workload, identify the claimed production behavior, stable input and
environment, metric, and acceptable noise before comparing commits. A real
regression requires interleaved repeated measurements, a justified commit
range, a reproducible first-bad boundary or equivalent causal evidence, and a
correctness control. Do not call a benchmark noise, machine drift, build-mode
change, cache-state change, data-preparation difference, or network variance a
source regression. A repair must preserve the original correctness intent and
show the expected metric movement on the same workload.

## Initial hypotheses

1. A recent performance-sensitive commit may have increased measured CPU, wall,
   allocation, I/O, or retained-state cost on a stable local workload, but the
   regression may be hidden by benchmark setup or cache-state variance.
2. A benchmark or profile recipe may compare unlike builds, data, warm/cold
   state, or timing regions and publish a false regression or false improvement.
3. A historical performance fix may have an untested adjacent workload whose
   regression is still present after the original fix.

## Initial queue

- Inventory `agent-journal/*profile*`, `*benchmark*`, performance-regression,
  build/perf artifacts, benchmark definitions, and recent performance commits.
- Search history and review discussion for measured regressions, held-out
  workloads, bisects, and known noise controls before selecting a range.
- Choose one reproducible local workload with fixed data, release-like flags,
  stable scratch storage, and a measurable production metric. Compare a
  justified range, interleave repeated last-good/last-bad runs, and bisect only
  after a regression exceeds noise.
- Keep benchmark-harness defects, source regressions, and inconclusive host
  variance as separate verdicts. Preserve raw samples, profiles, commands,
  environment, and the next queue in this journal.

## Cycle 86 investigation: prevout-fetch merge boundary

### Candidate and source evidence

- The strongest distinct recent candidate was merge commit `c0e91efdb31f`
  (`validation: fetch block input prevouts in parallel during ConnectBlock`),
  with first parent `f0da26cfc8a4`. The merge changes `CoinsViewOverlay`,
  `ConnectTip`, configuration, functional tests, fuzz harnesses, and unit
  tests. Its release description claims 1.18x to over 3x IBD improvements on
  disk-miss workloads, a default of eight fetch workers capped at 16, and
  `-prevoutfetchthreads=0` as the serial control.
- The historical boundary was checked out in detached worktrees:
  `/data/my_storage/tmp/cycle86-parent` at `f0da26cfc8` and
  `/data/my_storage/tmp/cycle86-child` at `c0e91efdb3`. Both used GCC 12.2.0,
  `RelWithDebInfo`, `BUILD_TESTS=OFF`, `BUILD_BENCH=OFF`, wallet/GUI/IPC/BDB/
  ZMQ disabled, the same separate build directories, and the same `/data`
  backed ccache/TMPDIR. Both `bitcoind` and `bitcoin-cli` targets built at
  `298/298`.
- `src/bench/connectblock.cpp` was explicitly excluded: it calls
  `Chainstate::ConnectBlock` directly and does not exercise the `ConnectTip`
  prefetch boundary. The current functional test is also an imperfect
  source-bisect workload: the parent uses the framework-generated
  `prevoutfetchthreads=1`, while the child commit adds
  `-prevoutfetchthreads=8` to `feature_block.py`. Its large reorg mostly spends
  recently created outputs and does not establish a cold chainstate read
  regression by itself.

### Controlled functional comparison

The test runner was invoked from each historical worktree with the matching
generated `test/config.ini`, fixed `--randomseed=8601`, isolated scratch roots,
different port seeds, and no default datadir. The authoritative command shape
was:

    time -p python3 test/functional/test_runner.py feature_block.py --tmpdirprefix=<isolated-root> --quiet --portseed=<seed> --randomseed=8601

All six runs passed `feature_block.py`, and a process scan after each completed
with no remaining `bitcoind`, test runner, or feature-test process.

| revision | run | wall seconds | user seconds | system seconds | result |
|---|---:|---:|---:|---:|---|
| `f0da26cfc8` parent | 1 | 57.25 | 40.22 | 5.86 | pass |
| `c0e91efdb3` child | 1 | 56.52 | 40.74 | 5.39 | pass |
| `f0da26cfc8` parent | 2 | 56.62 | 40.39 | 5.72 | pass |
| `c0e91efdb3` child | 2 | 59.64 | 40.56 | 5.57 | pass |
| `f0da26cfc8` parent | 3 | 56.72 | 40.49 | 5.60 | pass |
| `c0e91efdb3` child | 3 | 58.41 | 40.62 | 5.84 | pass |

Parent mean wall time was 56.863 seconds with 0.339 seconds sample standard
deviation. Child mean wall time was 58.190 seconds with 1.572 seconds sample
standard deviation: 1.327 seconds, or 2.33%, slower on this harness. The
parent median was 56.72 seconds and child median 58.41 seconds. The child
outlier and unequal worker setting prevent this from being a confirmed source
regression. The runs provide no failing correctness result, sanitizer trace,
or causal profile.

### Cycle verdict and handoff

- Verdict: inconclusive performance signal; no source change is justified.
- Dismissed as evidence of a bug: the direct `ConnectBlock` benchmark and the
  six passing functional runs do not prove a regression in the production
  prefetch path.
- Retained hypothesis: rerun the exact parent/child boundary with identical
  worker settings (`0`, `1`, and `8`) and a deterministic transaction-heavy
  chainstate workload that evicts or bypasses the UTXO cache, then collect
  repeated wall/CPU/I/O measurements and a profile. Only a stable first-bad
  effect with matching configuration and correctness evidence should produce a
  fix. The next cycle should select a distinct unchecked goal after this
  journal-only close.
