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

## Cycle 2 (2026-07-29): wider DrahtBot sweep (15 PRs) — 7 flagged, none implicating this tree

### Draw
Re-rank draw over the rebuilt 4-cell queue:
raw=8478006373347410799, index 3 -> #42 (second cycle; c1 queue
cell "wider DrahtBot sweep repo-wide"). Branch:
audit/ci-review-bot-c2 from 9c48365efe (#95 c3 bookkeeping).

### Method
GitHub search `type:pr commenter:DrahtBot updated:>2026-07-20`
(15 PRs), then per-PR comment fetch filtering "CI tasks failed"
markers + LLM reason lines.

### Findings (7 flagged)
- 34566, 35354: "test ancestor commits" — rebase hygiene, not
  code defects.
- 35735 (Add state to HTTPRequest): ASan/LSan/UBSan — memory leaks
  in httpserver_tests (test 251). In the PR's OWN new state code;
  not in-tree.
- 35793 (BIP54): macOS native — bip54_tests assertion failure in
  CExtKey::SetSeed (seed.size() outside 16-64) from the PR's own
  fixture. The IN-TREE assert fired correctly (working as
  designed); the fixture bug is the PR's.
- 35084 (ipc nonunix): FreeBSD Cross — platform cross-build, PR
  domain (deprioritized).
- 35768 (wallet): lint. 35751 (validation parallel prevout
  fetching in TestBlockValidity): iwyu. Both trivial; 35751 noted
  as fork-adjacent (overlaps the fork's own -prevoutfetchthreads
  feature — author's radar territory).

### Verdict
DISMISSED for this tree: no bot-flagged failure implicates in-tree
code; the two sanitizer/assert failures are PR-owned, and one of
them (35793) actually demonstrates the in-tree assertion working
as designed. Bot-signal hygiene: DrahtBot's LLM reason lines were
accurate in both checked cases (leak location; assert site).

### Exact commands
- api.github.com search/issues commenter:DrahtBot
- issues/{15 PRs}/comments filtered for CI-failure markers

### Limitations / queue
- 15-PR window (2026-07-20+); older flags stale by definition.
- Coverage-delta endpoint (boilerplate comments' links) still
  unanalyzed (c1 queue).
- 35751's overlap with the fork's prevoutfetchthreads: watch for
  the author's own coordination, no local action.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 3 (2026-07-29): coverage-delta endpoint (corecheck.dev) analyzed — real oracle source; flags regressions on upstream 35744

### Draw
Re-rank draw over the rebuilt 2-cell queue:
raw=3639072378229674997, index 1 -> #42 (third cycle; c1/c2 queue
cell "coverage-delta endpoint"). Branch: audit/ci-review-bot-c3
from 0b552321a2 (#51 c3 bookkeeping).

### Endpoint analysis
DrahtBot's "Code Coverage & Benchmarks" boilerplate links to
corecheck.dev/bitcoin/bitcoin/pulls/<n>, which provides per-PR:
coverage deltas (uncovered-new, lost-master, newly-covered,
included-code) and benchmark comparisons with significance flags.
Sampled 35744 (the fork author's coins cursor/resize PR):
- Coverage: "No coverage data" (pending).
- Benchmarks, significant: ComplexMemPool +15.58% slower,
  OrphanageEraseForBlock +13.11%, OrphanageEraseForPeer +33.33%,
  OrphanageSinglePeerEviction +11.14%, FindByte +4.60%.

### Assessment
- The endpoint is a REAL oracle source: it flagged measurable
  regressions on the sampled PR with per-bench significance.
- The flagged regressions belong to the upstream shared-lock PR,
  not to this tree (the fork carries the different, lock-only fix
  e049f064e1; unaffected). The information is still useful for the
  fork author's radar: 35744 now has a TSan failure (#42 c1) AND
  CI benchmark regressions (+15.6%/+33%) — his PR is in trouble on
  two independent axes.
- No action for this tree.

### Verdict
DISMISSED for the tree; the endpoint is recorded as an oracle
source (per-PR coverage+bench deltas) and the 35744 numbers are
recorded as upstream-watch data.

### Exact commands
- api.github.com/issues/35744/comments (boilerplate body)
- fetch corecheck.dev/bitcoin/bitcoin/pulls/35744

### Limitations / queue
- Single-PR sample; a periodic corecheck sweep of the fork-
  relevant PR set is the natural cron-sized follow-up.
- The benchmark environment (corecheck's CI hosts) differs from
  this host — deltas are directional, not absolute.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-07-29): corecheck sweep — deltas are noise-shaped (same magnitudes on unrelated PRs); c3's regression reading corrected

### Draw
Re-rank draw (last of the rebuilt 2-cell queue; singleton): #42
(fourth cycle; c3 queue "periodic corecheck sweep"). Branch:
audit/ci-review-bot-c4 from c6dd950cd3 (#80 c5 bookkeeping).

### Sweep result (15 fork-relevant open PRs, corecheck.dev)
- 35620 (leveldb block-cache knob): only WalletBalanceWatch -8.82%
  (noise-level, unrelated) — benchmarks clean.
- 35818 (bloom sizing — UNRELATED to mempool): ComplexMemPool
  +13.85%, OrphanageEraseForBlock +13.52%, OrphanageEraseForPeer
  +31.19%, BlockFilterIndexSync +10.25%.
- (35744 from c3: ComplexMemPool +15.58%, OrphanageEraseForPeer
  +33.33%.)

### The critical observation
A BLOOM-SIZING PR cannot plausibly affect mempool/orphanage
benchmarks — yet it shows the same +10-33% deltas in the same
benchmarks as 35744. The deltas must be CI benchmark drift/noise
(shared-runner variance of the mempool benchmarks at +-10-35%),
not PR-caused effects. c3's reading of the 35744 deltas as PR
regressions is CORRECTED here: the bench numbers are noise-shaped
and can't carry regression weight on their own. The 35744 TSan
failure (#42 c1) stands as the real signal.

### Verdict
- Corecheck coverage/bench endpoints remain useful ORACLES for
  coverage deltas (deterministic) — but their benchmark deltas
  need a baseline-drift control before treating a +-10-35% mempool
  delta as evidence. Recorded as method guidance.
- The author's own bench measurements remain the authoritative
  perf evidence for his branches (this rotation's A/B protocol).

### Exact commands
- corecheck.dev/bitcoin/bitcoin/pulls/{35620,35818} (FetchURL
  extraction); raw-curl sweep over 15 PRs (structure note: the
  raw HTML needs the FetchURL-style extraction, not regex)

### Limitations / queue
- Only 3 PRs deeply read (35620/35818/35744); the rest showed no
  significant rows or pending coverage.
- A true noise floor would need repeated per-PR runs (corecheck
  doesn't publish variance).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 5 (2026-07-30): periodic upstream watch — watched PRs static; F13/F14 re-verified offerable; #35753 fork-safe

### Draw
Harvested-queue draw (seed_raw=18209190156073205447,
masked=8985818119218429639, n=8, idx=7) -> upstream-watches cell ->
#42 (fifth cycle). Branch: audit/ci-review-bot-c5 from db049ea864
(#13 c2 bookkeeping tip).

### Hypotheses and verdicts (all DISMISSED / no-action)
- H1 #35744 (coins cursor/resize, the URGENT 🔴 watch) state change:
  still OPEN, head UNCHANGED 38b84769608a, last update 2026-07-28.
  In-tree UniqueLock fix e049f064e1 stands; no DrahtBot-visible new
  revision to re-assess. DISMISSED (watch continues).
- H2 #35818 (bloom sizing) landed: still OPEN, head c4302ddf8060.
  sink-reverse-reachability queue rule stands (take upstream's if it
  lands). DISMISSED.
- H3 #35620 (leveldb block-cache knob) landed: still OPEN, head
  a479f8a071a6, idle since 2026-07-14. DISMISSED.
- H4 F14 duplicate search: upstream master @ 9611a35603 still has NO
  ~LevelDBContext (raw penv/options members, dbwrapper.cpp:197ff) and
  zero dbwrapper.cpp commits since 2026-07-20 -> our fix 73a6798206
  is NOT duplicated; still offerable. DISMISSED.
- H5 F13 duplicate search: upstream node/mempool_args.cpp (file lives
  under src/node/ upstream; fork copy src/mempool_args.cpp) still
  validates only the upper bound (:35 read, :111 'less than or equal
  to'; no lower-bound error) -> fix 5e0a80ade5 still offerable.
  DISMISSED.
- H6 qrencode.mk changed upstream: unchanged (dead fukuchi.org path,
  same pinned sha256 da448ed4...71e8e). DISMISSED; fallback anchor
  unchanged.
- New-seed sweep, merges 7dea464d6b..9611a35603 (38 commits, 10
  merges): only in-scope consensus/validation merge is #35753
  (a99b27f192, null-mempool DeleteChainstate assert, fork-owner
  authored). Fork ALREADY carries fix + test
  (src/validation.cpp:6480 new-form assert;
  validation_chainstatemanager_tests.cpp:293
  chainstatemanager_delete_chainstate_no_mempool). NOT EXPOSED.
  Remaining merges: util LineReader, addnode-empty, test-only
  getdata, gui/guix/build/ipc — out of scope.

### Exact commands
- git fetch origin master (7dea464d6b..9611a35603)
- curl api.github.com/repos/bitcoin/bitcoin/pulls/{35744,35818,35620}
- git show origin/master:src/dbwrapper.cpp | grep '~LevelDBContext'
- git show origin/master:src/node/mempool_args.cpp | grep -i limitclustercount
- git show origin/master:depends/packages/qrencode.mk
- git log --oneline [--merges] 7dea464d6b..9611a35603 [-- in-scope paths]
- grep prev_chainstate->m_mempool src/validation.cpp;
  grep chainstatemanager_delete_chainstate_no_mempool src/test/...
- curl api.github.com/.../commits?path=src/{txdb,dbwrapper}.cpp&since=2026-07-20

### Verdict
Watch cycle complete; every cell dismissed with evidence, no new
work triggered. F13/F14 offerability re-confirmed against
9611a35603. No URGENT severity changes.

### Limitations / queue
- API sweeps are point-in-time; heads recorded above are the resume
  anchors. Next watch per rotation; corecheck noise-floor caveat
  from c4 still applies to any future bench-delta reading.

## Rotation note
Five cycles; watches quiet, duplicate searches negative.

## Cycle 6 (2026-08-02, draw 184, raw=6415546748680638711 (63-bit), idx 11/50 -> #24 STALE (journal COMPLETE 2026-07-30, handoff entry added); redraw raw=3428502195126078629 (63-bit) idx 20/49): upstream watch — 5 commits since 9611a35603, nothing in-scope; tracked PRs static; F13/F14/F16 offerability re-confirmed at 556988790a

### Watch (anchors from c5)
- git fetch origin master: 9611a35603..556988790a = 5 commits,
  2 merges: #35592 (http rpcallowip check at accept — RPC layer,
  out of scope, noted), #35838 (qa gui macOS — out of scope).
- In-scope paths (validation, txdb, dbwrapper, txmempool,
  net_processing, consensus, coins, mempool_args): ZERO merges.
- Tracked PRs (api.github.com): 35744 (coins resize/cursor),
  35859 (KDF rounds), 35818 (bloom sizing), 35620 (leveldb cache
  budget) — ALL open, unmerged, unchanged.
- F13/F14 offerability at 556988790a: origin/master dbwrapper.cpp
  still has NO ~LevelDBContext (count 0); mempool_args.cpp still
  has no 'at least 1' bound (count 0). F16 is covered by open
  #35859 (fork fix mirrors it).

### Verdict
Watch cycle complete; every cell quiet with evidence. No new
work triggered, no URGENT severity changes.

### Exact commands
- git fetch origin master; git log [--merges] range queries;
  curl api.github.com/repos/bitcoin/bitcoin/pulls/{...};
  git show origin/master:<path> | grep -c counts above.

### Limitations / queue
- Point-in-time sweep; 556988790a is the new resume anchor.
- corecheck noise-floor caveat (c4) still applies to bench reads.

## Cycle 7 (2026-08-02, draw 237, raw=4535641261288490616, n=1): upstream watch — master static at 556988790a; F13/F14/F17 offerability re-confirmed; accumulate-narrowing also present upstream (author's hygiene branch would be an offer); PRs static

### Watch (anchors c6)
- origin/master: 556988790a, 0 new commits since c6.
- Offerability at the current head:
  F14 dbwrapper dtor: still absent (count 0).
  F13 limitclustercount bound: still absent (count 0).
  F17 null-destroy: still unguarded (:1126-1129) — newly
  offerable this cycle (#16 c4 fix in-lineage).
- c13's accumulate narrowing: upstream packages.cpp:87 has the
  same literal-0 accumulate — the author's package-weight-
  accumulator branch is a valid upstream offer (hygiene class).
- Tracked PRs 35744/35859/35818/35620: all open, unmerged.

### Verdict
Watch complete; the offerable set grew by F17 (and the hygiene
accumulate note); nothing else moved.

### Exact commands
- git fetch/log above; git show origin/master greps above;
  api.github.com PR states above.

### Limitations / queue
- CI-failure content for 35859/35818 (c10 flags) not pulled
  (upstream-side; next watch if they advance).

## Cycle 8 (2026-08-02, draw 243, raw=17911194993721673680, masked 8687822956866897872, idx 0/2): upstream watch — static (master 556988790a, 4 PRs open, F13/F14/F17 still absent); quiet

### Watch
- origin/master: 0 new commits since c7.
- Tracked PRs 35744/35859/35818/35620: all open, unmerged.
- F13/F14 offerability: upstream still lacks ~LevelDBContext
  (0) and the limitclustercount bound (0). F17 unguarded
  (c7 check, unchanged).

### Verdict
Quiet cycle; nothing moved.

### Exact commands
- git fetch/log; api.github.com PR states; git show greps.

### Limitations / queue
- Same as c7.

## Cycle 9 (2026-08-02, draw 248, raw=13484172206762621838, masked 4260800169907846030, idx 0/2): variant watch — author has 26 open upstream PRs; the radar branches (retained-capacity, package-weight, rpc-dedup) have NO PRs yet; tracked set static

### Variant sweep (beyond the static master check)
- origin/master: still 556988790a (0 new).
- Author's open upstream PRs: 26 (search author:l0rinc
  is:open). The tracked set (35744, 35818, 35620, 35654, 35859)
  is present and open.
- The three radar-assessed branches (txgraph-retained-entry-
  usage, package-weight-accumulator, rpc-deduplicate-scan-
  objects) have NO upstream PRs yet — the 🟡 (L3) rides the
  author's branch until PRed; the adoption watch stays on the
  branch, not the PR queue.
- Oldest open: 31868 (IBD block-serialization, 2025-02);
  newest: 35839 (p2p empty-headers reselection, 2026-07-29).

### Verdict
Watch complete: the upstream queue is large but static on our
tracked cells; the radar branches remain pre-PR.

### Exact commands
- git fetch/log; api.github.com search author:l0rinc (26
  listed above).

### Limitations / queue
- PR-body alignment of the 26 against our journals not swept
  (only the tracked 5 + radar branches checked).

## Cycle 10 (2026-08-02, draw 258, raw=12724671350429090044): event-trigger sweep — all three quiet (master 556988790a, radar 864 unchanged, qa-assets pin 918cdd3 still == upstream HEAD)

### Sweep
- origin/master: static (0 new since c8).
- l0rinc branches: 864, no new, no force-updates since c15.
- qa-assets upstream HEAD: still 918cdd3 == the CI pin (#59
  c3's pin remains current; no review-forward needed).

### Verdict
Quiet across all triggers; nothing actionable.

### Exact commands
- git fetch origin master / l0rinc / qa-assets (lines above).

## Cycle 11 (2026-08-02, draw 262, raw=4778902433768003794): trigger re-check — all quiet (master 556988790a, radar 864, qa-assets 918cdd3)

### Sweep
- origin/master: static. l0rinc: 864 branches, nothing new.
- qa-assets upstream: still 918cdd3 (pin current).

### Verdict
Quiet; nothing actionable.
