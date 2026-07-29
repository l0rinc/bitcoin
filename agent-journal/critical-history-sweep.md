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

## Cycle 4 (2026-07-29): fork-delta critical-defect pass over validation.cpp — prune-assumevalid cache/flush mechanisms SOUND; lineage/master divergence discovered

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=9203260543596453984, index 0 (of 2) -> #49 (fourth cycle; c3
queue cell "fork-delta critical-defect pass, validation.cpp first").
Branch: audit/critical-history-c4 from ac7dca6ed8 (#50 c5 journal
tip). AUDIT TARGET REF: `master` (b08815bbb5), NOT the worktree —
see the divergence note; all master code reads are ref-based
(git show master:...).

### LINEAGE DIVERGENCE (important for future cycles)
merge-base(cycle-lineage, master) = a8823c0996. master carries 34
commits the lineage lacks — the ENTIRE prune-assumevalid series
(1d90c0ddcb..b08815bbb5). The ledger lineage has 1071 own commits
(cycle journals/fixes on the older tree); build-before was built
from the lineage (34ef575632). Any cycle testing prune-assumevalid
BEHAVIOR must build from master, not from the lineage worktree.
The origin/master..master delta (validation.cpp: 10 commits, all
prune-assumevalid/prevout-pool) exists only on master.

### Mechanism 1: eligibility-boundary cache (0fa6f4829e + b51dc6955a) — SOUND
Hypothesis: the cached CanUsePruneAssumeValid boundary can go stale
across a best-header change or runtime condition change, marking a
block script-skip-eligible that upstream would verify.
Falsified by construction (master:src/validation.cpp:2344-2372):
- Cache key is the m_best_header POINTER; any best-header change
  (incl. reorg) forces recompute under the new header.
- All other conditions are init-time-const (prune_assumevalid
  option, IsPruneMode, assumevalid hash) or re-evaluated per call
  (IsInitialBlockDownload outside the cache).
- Membership is a TRUE ancestor test: eligibility_tip->GetAncestor(
  block.nHeight) == &block — identity comparison along the real
  chain, not height-only; CBlockIndex ancestry is immutable.
- Boundary derivation: LCA(assumevalid, best_header) walked down
  while work-time <= 2 weeks — same predicate as the uncached path
  (ASSUMEVALID_MIN_WORK_SECONDS hoist is value-identical).
- b51dc6955a's decision reuse: computed in ConnectTip for the same
  pindexNew under held cs_main; no interleaving possible.

### Mechanism 2: stripped-block cache side index (b67edf0cac + 6eb57cf954) — SOUND
Hypothesis: the hash map and the CBlockIndex* side index can
diverge (stale membership -> null deref or unbounded growth).
Falsified (master):
- Single erase helper EraseCachedPruneAssumeValidBlock
  (validation.cpp:2405-2411) erases BOTH containers (index
  unconditionally, map conditionally with byte accounting); the
  only 2 erase call sites (ConnectTip failure :3200, success :3241)
  route through it; no .clear()/raw .erase anywhere else.
- Insert site (AcceptBlock :4573-4582) emplaces map + inserts
  index together; overwrite path adjusts byte accounting first.
- m_prune_assumevalid_block_cache_bytes: Assume(>= old) on both
  subtract paths; blocks are shared_ptr<const CBlock> (immutable),
  so RecursiveDynamicUsage can't drift between insert and erase.

### Mechanism 3: relaxed prune flush (9ada3f46b7) — SOUND (accepted relaxation)
Hypothesis: skipping the forced coins write on prune events breaks
crash recovery (index/files/coins ordering).
Analysis: ordering invariant "block index (with cleared HAVE_DATA)
persisted BEFORE files are unlinked" is kept via
should_write_files = should_write || fFlushForPrune. Guards:
first-prune anchoring coins write while CoinsDB().GetBestBlock()
is null; LoadGenesisBlock genesis rewrite for the anchor-crash
window; IsInitialBlockDownload latch restores standard coupling
permanently post-IBD; manual pruning and historical role excluded.
The relaxed invariant (coins may lag pruned files during IBD with
the option on) is EXPLICIT and documented (b08815bbb5); the
accepted consequence is redownload/possibly -reindex in the tail,
surfaced in the ReplayBlocks message. Not a consensus or
corruption defect; liveness trade-off by design.

### Dispositioned by class (not deep-dived this cycle)
- 0d72be5374 (100-minute chainstate writes): flush-cadence policy;
  larger crash-redownload window, accepted design.
- a681a7e674 / 6eb57cf954 (request-count / cache-byte caps):
  liveness bounds, not safety.
- fccdc098d3 / b3b0d49aad / 67cad6779e (CoinsViewOverlay thread
  pool + same-block-spend filter): correctness hinges on parallel
  coin reads being identical to the sequential path — QUEUED as the
  c5 cell (needs a differential experiment, not a read).

### Verdict
DISMISSED for the deep-dived mechanisms: no reachable critical
defect (consensus divergence, invalid acceptance, corruption,
crash) in the prune-assumevalid cache/flush machinery on master.
The feature's riskiest surface remaining is the prevout-pool
read-equivalence — queued.

### Exact commands
- git log --oneline origin/master..master -- src/validation.cpp
- git show 0fa6f4829e b51dc6955a b67edf0cac 9ada3f46b7
- git show master:src/validation.cpp (ref-based reads :2344-2415,
  :3164-3241, :3966, :4560-4590); git grep on master for all
  container mutation sites

### Limitations / queue for cycle 5
- Prevout-pool read-equivalence differential (parallel overlay vs
  sequential base view over a tx-heavy chain) — c5 cell.
- The other fork-delta file (net_processing.cpp, 670 changed
  lines) unpassed — c6 cell.
- Reads are static; no dynamic crash-injection of the relaxed
  flush was attempted (the author's own functional test
  9365dbc6d9 covers the behavior shape).

## Rotation note
Four cycles; validation.cpp delta passed (3 deep, rest
dispositioned). Not exhausted (prevout-pool differential,
net_processing delta).

## Cycle 6 (2026-07-29): net_processing fork-delta critical-defect pass — stripped-request machinery SOUND; count indices balanced

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=1440765120485782254, index 4 (of 5) -> #49 (sixth cycle; c5
queue cell "net_processing.cpp fork-delta pass"). Branch:
audit/critical-history-c6 from 2658b23484 (#101 c3 journal tip).
Audit target ref: master (b08815bbb5), ref-based reads (see c4
lineage note). Delta: 6 commits, all prune-assumevalid/download.

### Mechanism 1: stripped block requests (4974124e32) — SOUND
Hypothesis: a block can be accepted without required witness data
(witness-commitment unchecked where upstream checks), or a stale
stripped response punishes an honest peer.
Analysis (master:src/net_processing.cpp):
- Send side: per-request flag recorded at send time
  (QueuedBlock.request_without_witness; GetFetchFlags(peer, block)
  returns 0 flags exactly when ShouldRequestStripped... at the same
  lock/instant as BlockRequested records it — consistent pair).
  Non-witness peers are asked ONLY for stripped-eligible blocks
  (:1526-1530); compact-block upgrade suppressed for stripped
  (:2948).
- Receive side: prune_assumevalid = !has_witness &&
  requested_without_witness && CanUsePruneAssumeValid(*block_index)
  — the LIVE eligibility predicate is re-checked at receipt, and the
  witness-mutation check skip is conditioned on it. A stale stripped
  response (asked stripped, no longer eligible) is ignored WITHOUT
  Misbehaving and with only this peer's request removed; an
  unsolicited or unflagged stripped block falls through to normal
  rules; a full block answering a stripped request is processed
  normally. Defense in depth holds.
- Trust-model note (not a defect): below the assumevalid boundary
  the witness commitment is unverifiable on stripped blocks — the
  same opt-in trust extension as assumevalid itself; a
  commitment-invalid block cannot sustain on the most-work chain
  because full-checking nodes reject it.
- GetBlockDownloadWindow: stripped path uses the NARROWER window
  (PRUNE_ASSUMEVALID_BLOCK_DOWNLOAD_WINDOW = 16x8=128 <
  BLOCK_DOWNLOAD_WINDOW 1024) — bounds the transient in-memory
  cache; still >0, no stall shape (download-timeout machinery
  unchanged).

### Mechanism 2: request-decision reuse (87765f3960) — SOUND
Any staleness in the cached request-side decision can only produce
a stale stripped RESPONSE, which Mechanism 1's receive-side double
check ignores safely. Bounded by defense in depth.

### Mechanism 3: in-flight request count indices (df83a0058b, fb954e3111) — SOUND
Hypothesis: a removal path misses the decrement -> phantom
"in flight" count suppresses re-request forever (sync stall).
Falsified: increments only in BlockRequested (:1342); decrements at
both removal paths — RemoveBlockRequest per erased entry (:1308)
and FinalizeNode per disconnecting peer entry (:1816); re-request
removes-then-adds (:1332); zero-erase on --count==0; Assume-guarded
underflow (index_count_it != end, >0).

### Dispositioned by class
- a681a7e674 / 6eb57cf954 (request-count / cache-byte caps):
  liveness bounds; cap accounting follows the same paired-mutation
  pattern verified above.

### Verdict
DISMISSED: no reachable critical defect in the net_processing
prune-assumevalid delta on master. The download-side feature
machinery (validation.cpp from c4 + net_processing.cpp here) is
sound as read; the remaining feature surface is the prevout-pool
read-equivalence differential (c5 queue, needs a master build).

### Exact commands
- git log --oneline origin/master..master -- src/net_processing.cpp
- git show 4974124e32 fb954e3111; git show master:src/
  net_processing.cpp (:1238-1342, :1800-1830, :4861-4920);
  git grep master for RemoveBlockRequest/Decrement callers

### Limitations / queue
- Static reads; no dynamic multi-peer scenario run (the author's
  functional test from 4974124e32 covers startup/stripped/out-of-
  order/restart shapes).
- c7 cell candidates: prevout-pool differential (needs master
  build); 0d72be5374 flush-cadence interaction with the 100-min
  schedule under crash-injection.

## Rotation note
Six cycles; both fork-delta files passed. Not exhausted
(prevout-pool differential remains the one dynamic cell).

## Cycle 5 (2026-07-29): prevout-pool read-equivalence differential — 3-way muhash IDENTICAL (lineage / master+pool / master-noop)

### Draw
Re-rank draw over the remaining 3-cell queue:
raw=6717978107659154466, index 0 (of 3) -> #49 (fifth cycle; c4
queue cell "prevout-pool read-equivalence differential"). Branch:
audit/critical-history-c5 from a6e7606679 (#50 c6 journal tip).
NEEDS a master build (pool exists only on master) — done via a
disposable git worktree (master-wt @ b08815bbb5, removed after) +
build-master (reduced targets: no gui/wallet/tests/bench/fuzz/ipo).

### Hypothesis
CoinsViewOverlay's parallel prefetch (fccdc098d3 + b3b0d49aad +
67cad6779e) changes the coins ConnectBlock sees vs the sequential
path (ordering, missing coins, duplicate fetches) — observable as a
UTXO-set divergence after a dependency-heavy chain.

### Experiment (three arms, one deterministic chain)
Same 250-block chain (110 warmup + 250) on three binaries:
- run0: build-before bitcoind (lineage — NO overlay at all)
- run1: master bitcoind (overlay + thread-pool prefetch)
- run2: master with a scratch StartFetching no-op patch
  (if (true) return CreateResetGuard(); — overlay present, every
  coin fetched sequentially; binary preserved as
  /tmp/bitcoind_master_pool for the pool arm, patch never committed)
Chain per block: txA (newest mature utxo -> 2 wallet outputs),
txB (spends txA:0 INTRA-BLOCK), txC (spends txB:0, 2-deep intra-
block), txD (spends OLDEST wallet utxo, cross-block); amount guards
skip sub-10000-sat branches. Oracle: gettxoutsetinfo("muhash") +
txouts at h=360.

### Result — CLEAN
run0: txouts=859 muhash=34ef9380e7387668fa5a5288b8bb99a7161c81b
db420dd882b27ef2829dc591e bogosize=73015
run1: IDENTICAL. run2: IDENTICAL. The pool's parallel reads are
outcome-equivalent to sequential reads over intra-block dependency
chains, cross-block spends, and skip-branch variation.

### Determinism work (the real cost; recorded as harness lesson)
Three failed oracle iterations before the clean result:
1. Raw runs gave DIFFERENT muhashes even for the SAME binary twice
   (txouts 851-858 across runs). Root cause found by chain
   inspection (block-hash diff is time-only; tx input comparison):
   the framework's ECKey.sign_ecdsa (key.py:167-178) uses python
   `random` for the ECDSA nonce when rfc6979=False — the DEFAULT —
   so every MiniWallet signature (and txid) is random per process.
   FIX: random.seed(0x49) at the top of each run; the seeded stream
   replays identically (verified: two same-binary runs then produce
   byte-identical muhash; the 3-way run reproduced the same muhash
   again).
2. bad-txns-vout-negative: halving-recycled outputs undercut zero —
   amount guards.
3. dust: sub-threshold branches — wutxo filters <10000 sats.
4. KeyError 'muhash': gettxoutsetinfo defaults to
   hash_serialized_3; muhash needs the explicit arg.
Also: MiniWallet.generate() rescans utxos from the node EVERY block
(incl. mempool) — manual _utxos edits are wiped; and standalone
TestNode needs initialize_datadir + matching PortSeed for rpcport.

### Verdict
DISMISSED: no read-equivalence defect in the prevout pool over the
tested shapes. The parallel/sequential equivalence question is now
evidence-backed, not assumed.

### Exact commands
- git worktree add master-wt master; cmake -B build-master -S
  master-wt -G Ninja (Release, reduced targets); cmake --build
  build-master --target bitcoind -j4
- patch master-wt/src/coins.h (StartFetching no-op), incremental
  rebuild; cp build-master/bin/bitcoind /tmp/bitcoind_master_pool
  (BEFORE the patch, for the pool arm)
- python3 /tmp/btc49_diff.py <build-before> <pool> <noop>
  (seeded; RESULT lines above)
- cleanup: git worktree remove --force master-wt; rm -rf
  build-master /tmp/btc49_run*

### Limitations / queue
- 250 blocks / ~1000 txs / one dependency shape; deeper DAGs,
  reorgs, and mempool-eviction races untested.
- The differential compares OUTCOMES, not internal fetch ordering;
  a timing-only anomaly (e.g., rare stale-read under reorg) would
  not appear here.
- build-master deleted (disk); reproduction = the recorded cmake +
  patch recipe (~45 min).

## Rotation note
Five cycles; the prevout-pool dynamic cell is closed. #49's
remaining surface: 0d72be5374 flush-cadence crash-injection (low).

## Cycle 7 (2026-07-29): 100-minute chainstate writes — math/mechanism/test all consistent; no crash-injection needed

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=15543061168184443242, masked 6319689131329667434, index 4
(of 5) -> #49 (seventh cycle; c6 queue cell "0d72be5374
flush-cadence crash-injection"). Branch: audit/critical-history-c7
from ddc46bce53 (#9 c3 journal tip). Ref-based reads on master.

### Hypothesis
The 100-minute write interval (0d72be5374) or its companion
compaction-ratio retune breaks a bound: crash-recovery window,
compaction frequency, or a stale constant elsewhere.

### Checks (all green)
- Interval mechanics (master:src/validation.cpp:2878, :2953-2955):
  fPeriodicWrite fires on m_next_write; reschedule = now + 90min +
  uniform(0,20min) — matches the 90/110 constants and the anti-
  synchronization intent of the randomized window.
- Compaction math (ShouldCompactChainstate, flush_ratio 320 -> 200):
  frequency preserved: 320 x 60min = 13.3 days vs 200 x 100min =
  13.9 days. The commit's one-in-a-million claim verified:
  (199/200)^N = 1e-6 -> N = 2756 flushes = 191 days ~= "roughly
  200-day" (honest).
- Consumers: chainstate_write_tests.cpp carries its own 90/110
  constants and probes the MIN-1min boundary (:42) — a stale-value
  break would fail that test; it was updated with the change.
- Relaxed-flush bound (c4's 9ada3f46b7 analysis): the periodic
  write is independent of the relaxed prune path; the documented
  redownload-window bound holds with 90-110min (the doc commit
  b08815bbb5 cites "the regular periodic" write, no stale number).

### Verdict
DISMISSED: the flush-cadence change is consistent and covered.
Dynamic crash-injection adds nothing beyond the static closure
(interval mechanics + verified math + boundary test + c4's relaxed
analysis); recorded as available if a future signal appears.

### Exact commands
- git show 0d72be5374 b08815bbb5; git grep master for
  DATABASE_WRITE_INTERVAL / m_next_write (validation.cpp:98-99,
  2878, 2953-2955; validation.h:899; test/chainstate_write_tests)
- python3 one-in-a-million / frequency computations (above)

### Limitations / queue
- #49's remaining surface is thin: advisory table closed (c3),
  validation+net_processing deltas passed (c4-c6), prevout-pool
  differential clean (c5), flush cadence closed (c7). Next #49
  cycles need a fresh signal (new fork commits) — mark the
  campaign QUEUE-EMPTY after this cycle.

## Rotation note
Seven cycles; campaign queue-empty (see verdict). New fork commits
reopen it.

## Cycle 8 (2026-07-29): txindex format-migration assessment (l0rinc/txindex_optimization) — migration design SOUND on static analysis

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=193349491238106314, index 0 (of 2) -> #49 (eighth cycle; the
#65 c6 radar seed, reassigned here per the partial-migration
mandate). Branch: audit/critical-history-c8 from f4f67ee04c
(#50 c9 journal tip). Subject: Andrew Toth's txindex series
(co-authored Wuille/l0rinc/Towns) at refs/remotes/l0rinc/
txindex_optimization @ e2dc767418.

### The series
- 20d99d411a: 5-byte salted-SipHash key prefixes (replacing full
  txid keys) + (block_seq, tx_offset) packed into the key; iterator
  collision scan; legacy fallback.
- 590cd56bf6: skip bloom filters + legacy lookups when the DB has
  no 't' entries at open (m_has_legacy peek).
- f8cbd6424f: full-block WriteTxs (stated non-functional refactor).
- 15dae813cc: author tests for prefix collisions + legacy fallback.

### Checks (all green)
- Wrong-tx impossibility: every candidate from the collision scan
  is deserialized and hash-verified before return
  (FindHashedTx) — failure mode is false-NEGATIVE only, which
  falls back to the legacy path when present.
- Prefix-scan correctness: BlockTxPosition packs (block_seq,
  tx_offset) as 3-byte BIG-ENDIAN (txindex_key.h) — lexicographic
  iteration over the collision group is ordered and complete;
  offset bounded by static assert vs MAX_BLOCK_SERIALIZED_SIZE;
  seq bounded 16.7M blocks (~318 years, documented in the struct
  comment).
- Reorg semantics: reconnecting blocks keep their original
  sequence (Exists(BlockHashKey) skip); candidates sort
  active-chain-first then later-seq-first; a tx in two competing
  branches resolves to the active branch; both-inactive is no
  worse than legacy last-write-wins.
- Legacy coexistence: m_has_legacy = !f_memory && !f_wipe &&
  HasKeyStartingWith('t') at open; no other key type shares the
  't' prefix ('x','s','h', named keys, 'B'); legacy fallback and
  bloom fire exactly for databases that need them
  (FindTx: `if (!result && m_db->m_has_legacy)`).
- Bloom-skip safety: bloom consulted only by point reads; hashed
  reads go through iterators (bypass bloom) — documented in the
  constructor comment and verified against the read paths.
- Downgrade story: acknowledged in the startup LogInfo (old
  releases can't read the rebuilt index — user-facing, explicit).

### Verdict
DISMISSED (no defect): the migration design holds under the
critical shapes — collisions, reorgs, mixed/old/new databases,
downgrades. The author's collision+fallback tests cover the
dynamic arms; no local experiment needed absent a concrete
hypothesis (building the 367-commit-ahead branch is the
escalation path if one emerges).

### Exact commands
- git log/show refs/remotes/l0rinc/txindex_optimization
  (20d99d411a, 590cd56bf6, f8cbd6424f, 15dae813cc)
- git show e2dc767418:src/index/txindex_key.h

### Limitations / queue
- Static only; the branch was not built (40-min escalation path).
- The 's'/'h' point-read claim ("at most one per block against a
  tiny keyspace") taken from the code comment, not measured.

## Rotation note
Eight cycles; the txindex radar seed is assessed and closed.
#49 reopens on new fork commits.
