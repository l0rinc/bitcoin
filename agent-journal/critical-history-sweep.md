# Campaign #49 — critical-history-sweep

Note: the catalog entry for #49 carries a template artifact — its
boilerplate journal path (fuzz-introspector-blockers.md) and
campaign-focus text are duplicated from #50. Authoritative identity
taken from the title/slug: "Critical whole-history must-fix sweep",
slug critical-history-sweep, this journal.

## Cycle 1 (2026-07-29): advisory-sweep — top remote/consensus advisories all present + guarded (1 dup)

### Draw
Random draw over the 18-goal eligible pool (14 pending + 4 CYCLE-1,
#101 excluded as just-cycled): raw=5200339953805149283, index 7 ->
#49 (first cycle). Branch: audit/critical-history-sweep from
340b8b0d33 (#101 c2 bookkeeping). Start state: tracked-clean.

### Scope / method
Cell: the highest-severity in-scope (consensus/P2P/validation)
entries from the official advisory list
(bitcoincore.org/en/security-advisories/, fetched 2026-07-29), minus
anything already covered by prior campaigns. Fork-base check first:
HEAD contains upstream's TRUE merge SHAs through at least
2026-05-06 (aa1d0d7cd7 = upstream merge of #35209, SHA-verified via
GitHub API), so every advisory fixed up to v30.x must be present.
Per candidate: identify fix PR(s) from the advisory page, verify fix
CONTENT in HEAD (not just merge presence), verify regression oracle,
run the oracle where cheap.

### C1 — CVE-2024-35202 (blocktxn FillBlock assertion, HIGH, remote crash) — GUARDED
Fix: PR #26898 (merged 2023-01-24, released v25.0).
Content present: net_processing.cpp:3537-3545 — second blocktxn for
the same block hits the header.IsNull() guard (previous FillBlock
wiped the header, blockencodings.cpp:276 "Make sure we can't call
FillBlock again") -> RemoveBlockRequest + Misbehaving instead of a
second FillBlock. Exactly the advisory's attack shape.
Oracle: test_multiple_blocktxn_response
(test/functional/p2p_compactblocks.py:612, wired into run_test
:1106). RUN this cycle: full p2p_compactblocks.py green
("Tests successful", ~3.5 min, /tmp/btc49 scratch deleted).

### C2 — CVE-2024-52911 (script-interpreter UAF, HIGH) — DUPLICATE, skipped
Already swept 3-layer-deep as #33 E1 (journal commit cdef922ca9):
both fix merges present (3867d2421a = PR 31112 covert fix;
aa1d0d7cd7 = PR 35209 root-cause cleanup, merged 2026-05-06) +
structural construction-order read. Not re-reported.

### C3 — CVE-2025-54605 (log-filling disk exhaustion, LOW) — GUARDED
Fix: PR #32604 (log rate-limiting, merged 2025-07-09, v29.1/v30.0).
Content present: BCLog::LogRateLimiter (logging.h:71-143,
logging.cpp:384+), wired default-on at startup
(init.cpp:1497-1503; DEFAULT_LOGRATELIMIT=true, 1 MiB per source
location per 1h window, logging.h:66-68).
Oracle: BOOST_AUTO_TEST_CASE(logging_log_rate_limiter)
(logging_tests.cpp:307). RUN this cycle:
`build-before/bin/test_bitcoin --run_test='logging_tests/*rate*'`
-> "No errors detected" (2 cases).

### C4 — CVE-2025-46598 (unconfirmed-tx validation CPU DoS, LOW) — GUARDED
Three mitigation PRs, all content-present:
- PR #32473 (legacy/p2sh/segwitv0 per-txin sighash midstate cache):
  merge a27430e259 in HEAD; cache in script/interpreter.h:253-268,
  used at interpreter.cpp:1642/1695.
- PR #33050 (don't punish peers for consensus-invalid txs): content
  present — net_processing.cpp has ZERO TxValidationResult::
  TX_CONSENSUS branches (the -34 punishment removal), and the
  follow-on rename it enabled is live
  ("block-script-verify-flag-failed", validation.cpp:2160/2655;
  #33183 merge 7b4a1350df; release-note commit c0d91fc69c).
  Topology note: GitHub's recorded merge_commit_sha for #33050
  (dadf15f88c) is NOT an ancestor of upstream master's later true
  merge aa1d0d7cd7 — the merge was evidently re-created; content
  verified present regardless (which is the check that matters).
- PR #33105 (witness-stripping detection without re-running Script
  checks): merge f679bad605 in HEAD.
Oracles: the PRs updated the functional rejection/punishment suite
(mempool_accept.py, p2p_invalid_tx.py, feature_*.py) — present in
tree; not re-run this cycle (presence + production-content proof
carries the verdict).

### Exact commands
- advisory list + 3 detail pages via curl/FetchURL (bitcoincore.org)
- fix-PR identification: GitHub search/pulls API (35209, 33788,
  33050, 32473, 33105, 26898, 32604)
- presence: git log --grep / merge-base --is-ancestor; content greps
  (net_processing.cpp:3537-3545, blockencodings.cpp:276,
  logging.h:66-68/71-143, init.cpp:1497-1503,
  interpreter.h:253-268, validation.cpp:2160)
- oracle runs: p2p_compactblocks.py (functional, --tmpdir=/tmp/btc49);
  test_bitcoin --run_test='logging_tests/*rate*'
- dup check: git show cdef922ca9 (#33 E1)

### Verdict
DISMISSED (sweep clean): all four in-scope candidates are present
AND oracle-guarded; the one previously-covered candidate (#33 E1)
was a verified dup. No missing must-fix found in this cell.

### Limitations / queue for cycle 2
- Cells not yet swept: CVE-2025-54604 (spoofed self-connections,
  v30.0), CVE-2025-46597 (32-bit-only crash — aarch64 host can't
  execute the repro; code-presence check only), CVE-2024-52922 /
  52921 (propagation hindering), CVE-2024-52913/52914 (0.21/0.18
  era), pre-2020 advisories (CVE-2018-17144 already known-guarded:
  tx_check.cpp:41-45 + checkblock tests, L4 context).
- Medium/Low advisories from the 2024 batch (UPnP, addr-spam) are
  dependency/config-adjacent — lower priority per scope note.

## Cycle 2 (2026-07-29): remaining advisory cells — 54604 fork-interaction clean, 46597 cap present, 4 older cells marker-verified

### Draw
Re-rank draw (last of the rebuilt 6-cell queue; #80 drawn at draw
17 left this singleton): #49 c2. Branch:
audit/critical-history-c2 from 9b3eff4eef (#80 c2 bookkeeping).

### CVE-2025-54604 (spoofed self-connections log-filling) — COVERED
Fix = PR #32604 log rate-limiting, already verified present in c1
(LogRateLimiter wired default-on, init.cpp:1497-1503, unit tests
green). FORK-SPECIFIC interaction checked: the fork's
PRIVATE_BROADCAST feature adds 8 LogDebug(BCLog::PRIVBROADCAST)
sites (net_processing.cpp:1691/1696/2326/2330/2333/3625/3631/3772)
— all flow through the same LogPrint -> rate limiter (per source
location, 1 MiB/h), so connection-spam log-filling is capped on
the fork's added paths too. No gap.

### CVE-2025-46597 (32-bit block-size overflow crash) — COVERED
Fix present: node/mempool_args.cpp:50-52 rejects -maxmempool above
MAX_32BIT_MEMPOOL_MB when sizeof(void*) == 4. aarch64 host cannot
execute the original repro (32-bit-only, plus needs a 3GB+ mempool
on a 4GiB machine — the advisory's own exploitability caveat);
code-presence verified.

### Older cells (presence-by-marker)
- CVE-2024-52922 (stalling peers): BLOCK_STALLING_TIMEOUT_DEFAULT
  2s / MAX 64s (net_processing.cpp:133-137) + per-peer stall timers.
- CVE-2024-52921 (mutated blocks): BLOCK_MUTATED case (:1969) +
  Misbehaving on mutated block (:4937-4938).
- CVE-2024-52913 (re-request censorship): m_txrequest with the
  documented exclusion invariants (:802-804).
- CVE-2024-52914 (orphan stall): bounded m_orphanage machinery.
- CVE-2018-17144: previously noted guarded (tx_check.cpp:41-45 +
  checkblock tests; L4 context).

### Verdict
DISMISSED (sweep complete): every consensus/remote advisory cell in
scope is fix-present or fork-interaction-clean. Remaining list
entries are dependency/config-adjacent (UPnP CVE-2015-20111/
52917, miniupnpc/SOCKS CVE-2017-18350, BIP70 CVE-2024-52918,
inv-buffer/headers CVE-2024-52915/52916, getdata CVE-2024-52920,
addr-spam CVE-2024-52919) — lower priority per the scope note;
queued, not swept this cycle.

### Exact commands
- advisory fetches: bitcoincore.org disclose-cve-2025-{54604,46597}
- greps: PRIVBROADCAST log sites; mempool_args.cpp:50-52;
  net_processing.cpp:133-137/429/1969/4937/802-804

### Limitations / queue
- Dependency-adjacent advisories (UPnP/SOCKS/BIP70 era) unswept —
  next cell if a cycle lands here.
- No adversarial re-triggering attempted for the two fresh CVEs
  (fixes are config/code-presence verified; reproducing the
  upstream attacks is upstream-CI's job).

## Rotation note
Two cycles; the consensus/remote advisory surface is swept. Not
marking exhausted (dependency cells remain).

## Cycle 3 (2026-07-29): remote-P2P advisory batch — 4 markers verified on HEAD incl. fork-interaction; advisory table fully dispositioned

### Draw
Re-rank draw over the remaining 2-cell queue (60-c6, 49-c3):
raw=471461078390554833, index 1 (of 2) -> #49 (third cycle; c2
queue cell "dependency-adjacent advisories", widened to the 4
remote-P2P entries the c2 lump deferred). Branch:
audit/critical-history-c3 from 3066000db1 (#50 c4 journal tip).
origin/master @ 7dea464d6b used as the upstream reference for
fork-delta analysis.

### Method
Per marker: mechanism from the bitcoincore.org disclosure, fix-PR
invariant, current-HEAD code read (not just version age), then
`git diff origin/master..master` on the touched files for
fork-interaction. Hypothesis per cell: "the fork's deltas bypass or
reopen the upstream mitigation".

### CVE-2024-52915 (inv-buffer blowup, <0.20.0, Medium) — COVERED
Fix = PR 18962 (single GETHEADERS per INV). HEAD: INV handler caps
vInv at MAX_INV_SZ (net_processing.cpp:4170, Misbehaving), the loop
only records a best_block pointer, and at most ONE getheaders is
sent after the loop (:4227-4244), additionally rate-gated by
HEADERS_RESPONSE_TIME inside MaybeSendGetHeaders (:2888-2896). The
per-item 50 MB send-buffer storm shape is structurally absent. Fork
delta: PRIVATE_BROADCAST INV additions are outbound-only (:3635).

### CVE-2024-52916 (low-difficulty headers OOM, 0.12-0.15, Medium) — COVERED
HEAD: AcceptBlockHeader refuses AddToBlockIndex unless
min_pow_checked (validation.cpp:4397-4401, BLOCK_HEADER_LOW_WORK),
and storage happens only after the gate (:4401). min_pow_checked is
set true only once the peer's HeadersSyncState reaches the
work-validated phase (net_processing.cpp:4959; headerssync.h
two-phase pre-sync present). Fork delta: `git diff origin/master..
master -- src/validation.cpp | grep min_pow_checked` = 0 lines; the
gate is upstream-inherited verbatim (origin/master:4245-4247).

### CVE-2024-52919 (addrman nIdCount int overflow, <22.0, High) — COVERED
Fix = PR 22387. HEAD: `using nid_type = int64_t` (addrman_impl.h:40);
nIdCount is int64 — 2^63 insertions is physically unreachable, the
2^32 assertion-crash shape is eliminated. Fork's addrman delta (9
lines, addrman.cpp only) rewrites one ResolveCollisions_ arm (empty
tried slot treated as no-longer-collision); it does not touch the id
counter or its type.

### CVE-2024-52920 (GETDATA CPU spin, <0.20.0, Low) — COVERED
Fix = PR 18808. HEAD: ProcessGetData always advances the queue
iterator and erases processed entries (net_processing.cpp:2592-2630);
the unknown-type case is consumed with the block slot, and the code
carries the advisory URL in a comment (:2623-2627). FORK-INTERACTION
checked: the fork's private-broadcast GETDATA branch (:4273-4298) is
MAX_INV_SZ-gated before entry, contains no loop, never touches
m_getdata_requests, and disconnects+returns on any mismatch — it
cannot reintroduce the non-progress shape. The standard path
(:4300-4304) is upstream-identical; the fork's only ProcessGetData-
area delta is the GetFetchFlags signature (request_without_witness).

### Dependency/config-adjacent cells (from c2 queue) — DISPOSITIONED
- CVE-2024-52918 (BIP70): component absent from tree (paymentrequest
  removed) — inapplicable.
- CVE-2015-20111 / CVE-2024-52917 (UPnP/miniupnpc): depends-package
  only, no in-src code — out of scope per the campaign scope note.
- CVE-2017-18350 (SOCKS5 buffer overflow): fixed v0.15.1-era; the
  netbase SOCKS5 handshake has been rewritten since; far-past-era
  marker only.

### Verdict
DISMISSED (advisory table complete): every remote-P2P advisory cell
is mitigation-present on HEAD with fork-interaction analysis, and
every dep/config cell is dispositioned. No missing must-fix found.
The advisory-sweep interpretation of this campaign is now CLOSED;
remaining campaign surface is the commit-range walk (campaign-focus:
"progress from initial commit to HEAD in recorded ranges").

### Exact commands
- disclosures fetched: bitcoincore.org disclose-inv-buffer-blowup,
  disclose-getdata-cpu, disclose-header-spam (search), disclose-cve-
  2024-52919 (+ NVD cross-refs)
- code: net_processing.cpp:4167-4246/2888-2896/2580-2649/4258-4306;
  validation.cpp:4354-4407; addrman_impl.h:40,189; addrman.cpp:273-402
- diffs: git diff origin/master..master -- src/{net_processing,
  validation,addrman*}.cpp (hunk inventory; ProcessGetData body
  untouched; min_pow_checked 0 delta lines)

### Limitations / queue for cycle 4
- Presence-by-code-read, not adversarial re-triggering (consistent
  with c1/c2: upstream attacks are upstream-CI's job).
- Commit-range walk not yet started: no checkpoints exist. Proposed
  first range: the fork's own delta set (origin/master..master,
  ~490+670 lines in validation/net_processing alone) IS the
  highest-risk "history" for this tree — a per-file critical-defect
  pass over the fork delta beats 2013-era archaeology for reachability.
- c4 cell: fork-delta critical-defect pass (validation.cpp first,
  then net_processing.cpp), one bounded file per cycle.

## Rotation note
Three cycles; advisory surface closed. Not exhausted (commit-range /
fork-delta walk opened as the c4 queue).
