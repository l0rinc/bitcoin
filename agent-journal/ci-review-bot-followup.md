# Campaign #42 — ci-review-bot-followup

## Cycle 1 (2026-07-29): DrahtBot seam audit — 1 actionable bot finding (TSan race on upstream 35744); this tree NOT exposed

### Draw
Random draw over the 10-goal eligible pool (9 pending + 1 CYCLE-1,
#23 excluded as just-cycled): raw=2944374403711530272, index 2 ->
#42 (first cycle). Branch: audit/ci-review-bot-followup from
602481c1bc (#23 c2 bookkeeping; lineage anchor audit/resurrection @
5d0155254c). Start state: tracked-clean. Catalog note: #42's
campaign-focus block contains option-lifecycle text — same offset
artifact class as #49/#58/#80/#40; title+slug
ci-review-bot-followup authoritative.

### Scope / method
Cell: bot-flagged follow-ups (CI failures, sanitizer reports,
coverage regressions) on the l0rinc upstream seam — the population
whose code most resembles this fork's (#60 c1 seam rationale).
Fetched issues/{35714,35744,35754}/comments, filtered bot logins.

### Findings
- 35714, 35754: DrahtBot coverage/benchmark boilerplate only — no
  actionable follow-up.
- 35744 (coins cursor/resize, the upstream refinement of the race
  family this tree already fixed as L2): DrahtBot 2026-07-28T02:19Z
  "🚧 At least one of the CI tasks failed. Task TSan" — LLM summary:
  ThreadSanitizer data race in pthread_cond_destroy failing
  coins_tests. PR state: OPEN, updated 2026-07-28T03:49Z (after the
  report), head 38b84769608a — race unresolved at fetch time.

### In-tree exposure check (the audit question)
- The flagged race lives in the PR's unmerged shared-lock rework
  (shared_mutex + condition-variable machinery). This tree carries a
  DIFFERENT fix for the same underlying class: e049f064e1 holds a
  plain UniqueLock<Mutex> for the cursor's entire lifetime
  (txdb.cpp:231-232 m_db_lock member, :246-250 Cursor()
  construction). grep: zero condition_variable/shared_mutex in
  txdb.{h,cpp} — the pthread_cond_destroy shape cannot exist here.
- Regression evidence re-verified at HEAD:
  test_bitcoin --run_test=coins_tests/coins_db_resize_cursor ->
  No errors detected (the L2 battery: persist coin, live cursor,
  concurrent ResizeCache, resume scan; clean-master aborted in
  LevelDB per e049f064e1's message).

### Verdict
DISMISSED for this tree (no exposure; different, tested fix
in-tree). The bot finding is the fork author's upstream follow-up
(35744 still open) — recorded as watch, not local work. Other seam
PRs clean of actionable bot reports.

### Exact commands
- api.github.com/repos/bitcoin/bitcoin/issues/{35714,35744,35754}/
  comments (bot filter)
- api.github.com/repos/bitcoin/bitcoin/pulls/35744 (state/head)
- reads: txdb.cpp:225-265; grep condition_variable/shared_mutex
  src/txdb.*
- test_bitcoin --run_test=coins_tests/coins_db_resize_cursor

### Limitations / queue for cycle 2
- Only the 3-PR seam audited; a wider DrahtBot sweep (CI-failure
  comments repo-wide, or the bot's conflict reports on PRs whose
  code this tree carries) is the next cell.
- Coverage-bot diffs (the boilerplate comments link per-PR coverage
  deltas) not analyzed — low signal historically, but the
  coverage-DELTA endpoint is a candidate oracle source.
- The fork's own CI (ci/test/*.sh) bot-equivalent (no DrahtBot
  here) — local CI green state is assumed from the cycles' builds,
  not independently audited this cycle.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
