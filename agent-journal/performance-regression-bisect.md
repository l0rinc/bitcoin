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
