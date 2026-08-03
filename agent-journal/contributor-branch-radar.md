# Campaign #65 — contributor-branch-radar

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/contributor-branch-radar. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): l0rinc branch radar — encryption branch supersedes local #38 fix shape; 4 seeds recorded

### Draw
Random draw over the 51-goal eligible pool: raw=1291732092793296637,
index 34 -> #65. Contributor: l0rinc (the fork author; upstream
reviewer/PR author per reviews/ lineage).

### Inventory
`git ls-remote --heads l0rinc`: 856 branches = 225 named + 631
detachedN CI artifacts. Fetched 7 overlap-relevant branches shallowly
(--depth=30, explicit namespaced refspecs; no upstream refs altered).

### Radar entries
1. l0rinc/wallet-encryption-write-failures (tip 4b243f486e,
   2026-07-23, ACTIVE) — upstream-PR-shaped fix for the same wallet
   encryption write-failure class the fork covers locally
   (b8fcf9ed17/3be812ef57/501bd2e263/b16c831eae) and campaign #38 c1
   extended (9894fb8b6c in-memory rollback). The branch RESTRUCTURES:
   RunWithinTxn(GetDatabase(), ...) makes WriteMasterKey + all
   spk_man->Encrypt atomic, and mapMasterKeys registration happens
   only AFTER the durable commit — the mixed-state class our rollback
   patched cannot arise. CONFLICTING ASSUMPTION FLAGGED: 9894fb8b6c is
   correct for the current local shape but is the inferior fix
   (register-then-rollback vs order-after-commit); if the branch lands
   upstream, drop the local family instead of rebasing it. Its
   descriptor-key-erase fix is NOT APPLICABLE locally:
   EraseDescriptorKey does not exist in this tree (no callers).
2. l0rinc/txgraph-equal-feerate-prefix-overflow (2026-07-24) — the
   equal-feerate prefix tracking is already present locally
   (txgraph.cpp:558-559, 671, 1179-1185); the overflow guard belongs
   to the saturation class covered by 3ae78dbd25 (#25 c1 range).
   No action.
3. l0rinc/bloom-zero-elements (2026-07-27) — matches PR 35818,
   reviewed locally yesterday (reviews/2026-07-27-pr-35818-*:
   correct, nFPRate=0 leftover noted). No action.
4. l0rinc/wallet-redact-txids-infolog (2026-01-19, aging) — moves
   txid logging behind -debug=wallet. PRIVACY SEED for campaign #30's
   queue (txids at INFO are not secrets, but the privacy posture is
   consistent with that campaign's class).
5. l0rinc/verify-assumeutxo-hashes (2026-05-22) — consensus-guard
   seed; assumeutxo hash verification. Watch for upstream landing.
6. l0rinc/warn-dirty-coin-coint (2026-02-08) — coins invariant seed
   (61e8c5138d lineage territory).
7. l0rinc/wallet-bound-bdb-overflow-chains (2026-07-20) — likely the
   source PR of local 71cf0ba593 (BDB overflow chain bound);
   consistent, no action.

### Verdict
- Radar established; one actionable conflict-of-assumptions flagged
  (entry 1) and 4 seeds recorded for campaign queues. No unpublished
  work copied; all comparisons by content (recreated local history
  makes hash ancestry useless across the remote boundary too).

### Limitations
- 7 of 225 named branches inspected (overlap-ranked by name);
  optimization branches (IBD-optimizations, block-serialization,
  leveldb-to-rocksdb) unassessed — queued for a perf-oriented pass.
- knots remote not scanned this cycle.

### Exact commands
- `git ls-remote --heads l0rinc`
- `git fetch -q --depth=30 l0rinc refs/heads/l0rinc/<b>:refs/remotes/l0rinc/<b>`
- content checks: git show <branch>:<file> | grep (above)

### Next queue for this campaign
- Perf-branch pass: IBD-optimizations, block-serialization-
  optimizations, tune-leveldb-options, write-chainstate-every-hour —
  overlap with #20/#21/#23/#25 campaign measurements.
- knots remote scan (maintenance backports, adjacent to #66).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-28): perf-oriented pass — CheckBlock dup-check (1.85x claim) locally absent, equivalence assessed; 2 more perf branches recorded

Base: 021cdca6c0 (journal commit for #2 cycle-1 on
audit/assertion-invariant; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/contributor-radar-c2 (c1 journal carried).
Start state: clean (untracked scratch only).

### Draw
Random draw over the 52-goal pool (32 pending + 20 CYCLE-1; #2
excluded as just-cycled): raw=9142664693120658118, index 42 -> #65.
Queued from c1: "optimization branches unassessed — perf-oriented
pass". Radar refresh: 226 named branches (was 225; +1 since c1).

### Radar entries (perf seam; fetched --depth=30, content-compared)
1. l0rinc/optimize-CheckBlock-input-duplicate-check (f3cc8fd27d tip,
   2025-01) — restructures the CVE-2018-17144 duplicate-prevout check
   in CheckTransaction: skip coinbase (single Null prevout) and
   1-input txs, direct-compare 2-input txs, sorted-vector +
   adjacent_find for 3+ instead of std::set. LOCALLY ABSENT: this tree
   still inserts every prevout into a std::set for every transaction
   including coinbases (src/consensus/tx_check.cpp:41-45). Claimed
   effect: CheckBlockBench 335.9 -> 181.9 us (1.85x, AppleClang).
   Equivalence assessment: coinbase/1-input skips are sound (a dup is
   impossible there; the Null-prevout rejection for non-coinbase is
   preserved in the branch's 2-input arm — but I did NOT verify where
   the branch puts the null check for 1-input txs, so equivalence is
   PLAUSIBLE, not proven). This is a consensus-check restructure —
   NOT absorbed (rotation records, never adopts unmerged consensus
   changes; it is the fork author's own upstream-PR-shaped work, so
   the decision sits with him). Duplicated on l0rinc/IBD-optimizations
   as f07c79f034/cb8c012b87 (rebased pair).
2. l0rinc/IBD-optimizations (b6b4235c14 tip) — prevector inline size
   28 -> 36 so P2WSH/P2TR/P2PK scripts (34-36B) stay on the stack
   instead of heap (prevector size dates to 2015, pre-SegWit);
   Massif-verified lower stable memory after flushes per commit
   message. LOCALLY ABSENT (local prevector still 28B inline). Perf
   seed for the memory/profile campaigns; trade-off documented (+8B
   per CScript object base size).
3. lorinc/block-serialization-optimizations (41ef25fcba tip) — single
   byte write fast paths (AutoFile 4k buffer avoidance, VectorWriter/
   DataStream memcpy avoidance), SizeComputer specialization merge,
   static-extent spans, template-bloat reduction. LOCALLY ABSENT.
   Serialization hot-path seed for #20-style micro-opt review.

### Verdict
- Radar extended to the perf seam: 3 branches assessed, 0 adopted,
  0 conflicts with fork hardening found. One upstream-applicable
  optimization (entry 1) flagged with an explicit equivalence caveat
  for the fork author's adoption decision.
- No unpublished work copied; all comparisons by content.

### Exact commands
- git ls-remote --heads l0rinc (226 named)
- git fetch -q --depth=30 l0rinc refs/heads/l0rinc/IBD-optimizations
  refs/heads/l0rinc/optimize-CheckBlock-input-duplicate-check
  refs/heads/lorinc/block-serialization-optimizations
- git log/show per branch; local grep/sed of
  src/consensus/tx_check.cpp:36-70

### Limitations / queue
- rocksdb-brute/rocksdb-build-fix and lorinc/leveldb-to-rocksdb not
  assessed (large DB-engine experiment; needs its own cycle with the
  #95 differential method) — queued.
- leveldb knob branches (tune-leveldb-options, -block-cache-write-
  buffer, -ibd-threshold-benchmarks, -options-logging,
  -validation-bench-knobs, paplorinc/leveldb-upgrade) unassessed —
  queued as one batch.
- knots remote still unscanned.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-29): leveldb knob branch batch — premise of the block-cache knob VERIFIED against vendored LevelDB

### Draw
Re-rank draw over the 5 remaining CYCLE-2+ open cells:
raw=8499369058190745183, index 3 -> #65 (third cycle; c2 queue cell
"leveldb knob branches ... unassessed — queued as one batch").
Branch: audit/contributor-radar-c3 from b0f624c057 (#21 c3
bookkeeping).

### Batch assessment (6 branches, l0rinc remote, in-repo refs)
1. l0rinc/tune-leveldb-options (4 commits): write_buffer_size
   ladder 1/2/4/8 MiB, no messages — benchmark scratch. Not
   adoptable without measurements.
2. l0rinc/leveldb-block-cache-write-buffer (2 commits): zero the
   LevelDB block cache on 64-bit (ecb530c63d) + reassign that half
   of the DB cache budget to write_buffer (a479f8a071). PREMISE
   VERIFIED locally: this tree's vendored LevelDB serves
   uncompressed (dbwrapper.cpp:145 kNoCompression) mmap-backed
   table blocks directly and marks them NON-cachable —
   table/format.cc:106 "Do not double-cache" when Read() returns a
   pointer into the mmap region (data != buf); the block_cache
   insert gate at table/table.cc:182 (contents.cachable &&
   options.fill_cache) therefore never fires on 64-bit. The
   configured block cache (half the budget) is indeed unused here;
   the knob's premise is exactly right for this tree. (Orthogonal
   corroboration: iterator read options already use
   fill_cache=false.)
3. l0rinc/leveldb-ibd-threshold-benchmarks (4 commits): bench-only
   I/O-mode toggles (force mmap reads / force permanent fd / raise
   limits) — benchmark branches, no production intent.
4. l0rinc/leveldb-options-logging (1 commit): dump non-default
   LevelDB options at init under -debug=leveldb — trivially sound
   observability, upstreamable shape.
5. l0rinc/leveldb-validation-bench-knobs (4 commits): memtable
   compaction target ladder (L1/L3/L5) + 256 MiB L1 — bench-only.
6. paplorinc/leveldb-upgrade (2 commits): vendored LevelDB bump to
   latest + GetOptions adjustment — high-touch vendored change;
   needs its own build/test matrix before any assessment; not
   attempted this cycle.

### Verdict
Journal-only assessment (rotation records, never adopts the fork
author's own work): the batch decomposes into one PRINCIPLE-
VERIFIED knob (block-cache->write-buffer, premise proven against
vendored source with file:line evidence), one diagnostic
(options-logging), three benchmark ladders, and one vendored bump
needing a matrix. No local defect; nothing to adopt locally. The
block-cache premise verification is the durable yield — any future
adoption decision by the fork author can cite it.

### Exact commands
- git ls-remote --heads l0rinc (branch set); git log/show per
  branch (messages, diff stats)
- reads: src/leveldb/table/format.cc:84-139, table/table.cc:155-208,
  dbwrapper.cpp:139-154

### Limitations / queue
- rocksdb-brute / rocksdb-build-fix / lorinc/leveldb-to-rocksdb
  remain the heavy engine-swap cell (own cycle, #95 method).
- No local IBD A/B of the verified knob (rotation rule: the fork
  author's adoption decision, not ours).
- knots remote still unscanned.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-07-29): rocksdb-brute assessment — bulk-fetch class subsumed by the fork's own prevout pool; branch is stale WIP

### Draw
Re-rank draw over the rebuilt 3-cell queue:
raw=2099837013193560362, index 2 -> #65 (fourth cycle; c3 queue
"rocksdb-brute bulk-ops — depends on the swap"). Branch:
audit/contributor-radar-c4 from 62bd4923d2 (#58 c3 bookkeeping).

### Branch shape (l0rinc/rocksdb-brute, Oct 2024, tip d2cd534521)
Stack: engine swap (8d4e3fd666, supersedes #95's build-fix lineage)
+ bulk coin ops: inline HaveCoin/FetchCoin into HaveInputs
(7bec5e7b5f), bulked HaveInputs (1c2427bcf1), optional getcoin
(8420bd95bb), bulk spentness (83384a1ad2), bulked AddCoins
(d896fda203), tuning (2cc6f0efc1 nCacheSize/parallelism),
PreChecks input-existence (d2cd534521). TODOs left in the diff
("TODO bulk", "TODO how can this not be inserted", "TODO ret ==
cacheCoins.end()") — experimental WIP, not production-shaped.

### Assessment
- The bulk-fetch class (batch missing-input reads) is
  ENGINE-INDEPENDENT (CCoinsViewCache level) — and largely
  SUBSUMED by the fork's shipped -prevoutfetchthreads
  (5bf1c32008, parallel prevout-fetch pool validated in #43 c1):
  the same latency class, already productionized.
- Distinct remnants: bulked AddCoins / write batching and the
  spentness-in-bulk check — small deltas over the shipped pool,
  on a stale (Oct 2024) base.
- No local adoption (author's own work; rotation records).

### Verdict
Assessment complete, journal-only: rocksdb-brute is the WIP
precursor of the shipped prevout-fetch class; nothing actionable
beyond the author's own roadmap.

### Exact commands
- git log/show remotes/l0rinc/l0rinc/rocksdb-brute (12 commits,
  stat + diff of 1c2427bcf1)

### Limitations / queue
- The rocksdb-swap line itself is covered by #95 c3/c4 (build,
  reindex, durability).
- knots remote still unscanned.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 5 (2026-07-29): knots remote scan — 29.x-knotsfixes delta inventoried; secp256k1 fixes present, two tor fixes dispositioned

### Draw
Re-rank draw over the remaining 4-cell queue:
raw=8930217931205760543, index 3 (of 4) -> #65 (fifth cycle; c4
queue cell "knots remote still unscanned"). Branch:
audit/contributor-radar-c5 from d96a7614b2 (#24 c2 journal tip).
Remote: knots https://github.com/bitcoinknots/bitcoin.git
(59 branches + tags; latest v29.3.knots20260508).

### Method
git fetch --depth=40 knots 29.x-knots 29.x-knotsfixes; the fixes
delta (20+ topic merges) triaged by scope: core/consensus/crypto/
net in-scope; qt/guix/doc/rpc/wallet/win inventoried only.

### fix_secp256k1_bugs-29 (d941a2618b) — DISMISSED (present)
Two upstream secp256k1 PRs knots vendored:
- #1821 (fed5dd96cd): ellswift_xdh overflow flag overwritten by
  `overflow = secp256k1_scalar_is_zero(&s)` (should be |=) — keys
  >= n silently reduced instead of rejected. PRESENT in-tree:
  src/secp256k1/src/modules/ellswift/main_impl.h:557 already has
  `overflow |=`. (Master-relative reachability would have been low
  anyway: BIP324's XDH secret is self-generated, always < n.)
- #1731 (3594134e4b): schnorrsig nonce buffer rename+clear.
  PRESENT: schnorrsig/main_impl.h:128 nonce32[32].
Our vendored subtree is newer (b7f9178976 "Update secp256k1 subtree
to latest master"); knots' fixes are backports of what we already
carry.

### fix_tor_common_bind-29.2 family — two fixes
- 85a13e943a (torcontrol: map bind-any to loopback before
  StartTorControl): NOT present in-tree (init.cpp:2219 passes
  onion_service_target through unchanged) NOR in origin/master.
  Reachability: -listenonion + torcontrol + bind-any target;
  connect("0.0.0.0") works on Linux (our platform), fails on
  Windows. Severity: low (Windows-only, config-dependent).
  Verdict: DISMISSED for this tree (platform/scope); recorded as an
  upstream-watch seed (knots hash above) — if upstream takes a
  bind-any guard, take theirs.
- 23071773f6 (net: treat connections to the first normal bind as
  Tor when appropriate): targets the PRE-REWORK binding model.
  Our tree has the dedicated -bind=addr:port=onion machinery
  (init.cpp:575/2175, net.h:1115 m_onion_binds classification) that
  supersedes it. Verdict: DISMISSED (superseded by rework).

### Inventory (not deep-scanned, scope note)
qt/guix/doc/win merges (fix_qt_*, fix_win_exclopen, restore_guix_,
docfix), extsigner_sanitychk_fingerprint (wallet/HWW),
fix_rpc_mixed_params_edgecases (RPC), rm_dnsseed_pt (policy),
plus __base_29 policy features (assumeunconf, datacarriercost,
permitephemeral, rdts_consent_prompt) — Knots policy surface, not
must-fix class.

### Verdict
DISMISSED (scan complete): no must-fix seeds from the knots fixes
line; the two net-adjacent items are platform-scoped or superseded.
Knots remote now scanned; radar cell closed.

### Exact commands
- git ls-remote --heads/--tags knots
- git fetch --depth=40 knots 29.x-knots 29.x-knotsfixes
- git log --oneline knots/29.x-knots..knots/29.x-knotsfixes
- git show d941a2618b fed5dd96cd 3594134e4b 85a13e943a 23071773f6
- greps: ellswift main_impl.h:557, schnorrsig:128, init.cpp:2219,
  origin/master init/torcontrol IsBindAny

### Limitations / queue
- knots syslibs branches and pre-29 lines unexamined (historical;
  low value).
- qt/rpc/wallet knotsfix topics inventoried only — scan them only
  if a core-reachability concern appears.

## Rotation note
Five cycles; knots cell closed. Not exhausted (new upstream
contributor branches appear continuously — periodic re-scan).

## Cycle 6 (2026-07-29): l0rinc re-scan — txindex storage-format series force-updated (IN SCOPE); wallet-export crash branch (wallet scope)

### Draw
Re-rank singleton (last queue cell): #65 (sixth cycle; c5 queue
cell "periodic re-scan"). Branch: audit/contributor-radar-c6 from
ed0daeee0a (#80 c7 journal tip). Remote refreshed: git fetch
l0rinc --prune.

### Seed 1 (in scope — indexes/storage): txindex_optimization
Force-updated (716cf096f4...e2dc767418): a txindex STORAGE-FORMAT
series on top of 367-commits-ahead master:
- 590cd56bf6 txindex: skip bloom filters and legacy lookups for new
  databases
- 20d99d411a txindex: hash key prefixes and pack block positions
- f8cbd6424f txindex: pass the full block to DB::WriteTxs
- 15dae813cc tests: cover txindex hash prefix collisions and
  legacy fallback
- 088b0b4a89 doc: release notes; e2dc767418 contrib benchmark
Sibling branches: txindex-benchmark-variants,
txindex-block-fetch-proxy (same series).
The critical-review question when a cycle lands here: does the new
prefixed/packed key format read OLD databases byte-correctly
(legacy fallback), and does skipping bloom/legacy lookups change
any observable result? Format migrations are exactly the
critical-history class. Recorded, not assessed (multi-hour cell).

### Seed 2 (wallet scope — recorded, not pursued):
l0rinc/wallet-export-source-id (NEW branch): 5744e18ff8 "test:
characterize watch-only export crash" + 7cdd1a9eee "wallet:
preserve source ID during export". A wallet export crash
characterization + fix. Wallet-scope per the campaign scope note;
no core-reachability claim apparent. Recorded for the wallet-owning
campaigns.

### Also noted
l0rinc/master advanced to 7dea464d6b (upstream merges past our
local master's b08815bbb5 — no fork-feature content; our master
tracking is fine).

### Verdict
Radar cell complete: two seeds recorded with provenance and the
exact review questions; nothing requiring immediate preemption
(no critical-class surprise in the new commits' shapes).

### Exact commands
- git fetch l0rinc --prune; git ls-remote --heads l0rinc (860)
- git log master..l0rinc/master (upstream-only)
- git log refs/remotes/l0rinc/txindex_optimization (-6);
  l0rinc/l0rinc/wallet-export-source-id (-2)

### Limitations / queue
- txindex format-migration assessment (legacy fallback byte-
  correctness; bloom-skip equivalence) — the one substantial cell,
  needs its own cycle.
- knots fixes-line re-scan on next radar cycle.

## Rotation note
Six cycles; re-scan cell closed. Not exhausted (txindex series).

## Cycle 7 (2026-07-29): knots fixes-line re-scan — no new activity since c5 (same tip)

### Draw
Re-rank singleton (last queue cell): #65 (seventh cycle; c6 queue
cell "knots fixes-line re-scan"). Branch:
audit/contributor-radar-c7 from d2a3b053b8 (#49 c8 journal tip).

### Result
git fetch --depth=40 knots 29.x-knots 29.x-knotsfixes: the
knotsfixes tip is UNCHANGED from c5 (d941a2618b Merge
fix_secp256k1_bugs-29). The delta count (875) is a shallow-clone
artifact — the --depth=40 window excludes the merge-base, so the
delta appears to cover all visible history; the tip comparison is
the authoritative check.

### Verdict
No new knotsfix topics since c5; nothing to assess. Radar cell
closed.

### Exact commands
- git fetch --depth=40 knots 29.x-knots 29.x-knotsfixes
- git log --oneline knots/29.x-knots..knots/29.x-knotsfixes
  (tip d941a2618b, unchanged; count artifact explained)

### Limitations / queue
- Next radar cycle: l0rinc re-scan (new branches appear
  continuously; txindex siblings from c6 may update).

## Rotation note
Seven cycles; radar quiet. Not exhausted (periodic re-scans).

## Cycle 8 (2026-07-29): txindex sibling branches — block-fetch-proxy is a substantive IN-SCOPE seed; benchmark-variants is revert-heavy WIP

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=1875611079037645143, index 1 (of 2) -> #65 (eighth cycle; c6
queue cell "txindex siblings"). Branch:
audit/contributor-radar-c8 from aec66d1264 (#50 c10 journal tip).

### Seed: l0rinc/txindex-block-fetch-proxy (6-commit series)
Pruned-block retrieval layered on the new txindex format:
- 482d9d9d7e blockstorage: keep cursor files from pruning
- 477ce33c4b net, rpc: fetch and retain pruned blocks
- 5f5a08f570 txindex: read transactions from pruned blocks —
  proxy fetch ONLY for "explicitly trusted local callers", REST
  lookups stay disk-only; legacy physical-position DBs rejected
  (they cannot identify pruned blocks by height/hash — requires
  the 20d99d411a format assessed in #49 c8).
- c79ee794ae rpc: gettxoutproof reads the located block through
  the proxy when enabled; disk-only path and errors preserved
  otherwise.
- 990865587e wallet: rescan pruned blocks through proxy;
  019b499da6 doc.
Review questions when a cycle lands here:
1. Is the proxy-fetched block hash-verified against the block
   index before use (fetch-by-hash from the local index vs
   fetch-by-height)?
2. gettxoutproof: is the merkle root of a refetched block
   re-verified against the indexed header before the proof is
   served (a wrong-block fetch must not yield a valid-looking
   proof to a light client)?
3. Is the "trusted local caller" gate consistent across RPC paths
   (gettransaction vs gettxoutproof vs REST)?
4. Cursor-file retention vs the prune disk bound.
### Also scanned
txindex-benchmark-variants: benchmark scaffolding with paired
Reverts (cache-reallocation, no-block-cache, 16 KiB blocks) —
revert-heavy WIP, no review value this cycle.

### Verdict
Radar cell complete: block-fetch-proxy recorded as the top
in-scope seed with four concrete review questions; no preemption
warranted (the branch is the author's own WIP, not yet proposed).

### Exact commands
- git fetch l0rinc 'refs/heads/l0rinc/txindex-*'
- git log --oneline master..refs/remotes/l0rinc/l0rinc/
  txindex-{block-fetch-proxy,benchmark-variants}
- git show --stat c79ee794ae 5f5a08f570

### Limitations / queue
- The four review questions above are the assessment cell if a
  future cycle lands here (static first; the branch builds
  against a master we already built for #49 c5).
- knots fixes-line on next radar cycle.

## Rotation note
Eight cycles; siblings scanned. Not exhausted (the proxy seed).

## Cycle 9 (2026-07-29): block-fetch-proxy assessment — all four review questions SOUND

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=16381284910993384311, masked 7157912874138608503, index 1
(of 2) -> #65 (ninth cycle; c8's proxy seed). Branch:
audit/contributor-radar-c9 from 24b9fb0b4b (#50 c11 journal tip).
Static reads on refs/remotes/l0rinc/l0rinc/txindex-block-fetch-proxy.

### Q1: hash verification of fetched blocks — SOUND
Requests are by-hash from the LOCAL block index
(block_index.GetBlockHash()); the response passes
IsBlockMutated (witness-root when segwit-active; mismatch ->
Misbehaving + caller woken with error) and then full
ProcessNewBlock(force_processing=true) validation before
storage. A wrong block cannot be stored.

### Q2: merkle-root re-verification for proofs — SOUND
gettxoutproof's block comes from disk (validated at connect) or
the proxy (validated at ProcessNewBlock). The proof anchors to
the INDEX header hash; GetTransaction checks
block_index->GetBlockHash() == block_hash. A non-active-chain
result is still cryptographically consistent (the client checks
the block hash itself).

### Q3: trusted-caller gate consistency — SOUND
Two independent flags (allow_block_fetch, allow_local_only):
REST gets (false, false) — fully disk-only; gettransaction,
utxoupdatepsbt prevout, gettxoutproof get fetch=true with
local_only defaulting true. Matches the commit's stated
contract; no RPC path fetches for untrusted callers.

### Q4: cursor retention vs prune bound — SOUND
Fetched blocks append to the CURRENT block file (per cursor —
both cursors handled for assumeutxo), retained only until normal
file rotation; the LOCAL_ONLY marker is cleared when the
containing file is eventually pruned. No unbounded cache;
no separate disk/memory store.

### Also noted
Legacy physical-position txindex DBs are rejected at startup
with an explicit InitError (they cannot identify pruned blocks
by height/hash) — forces the #49 c8-assessed format. The series
carries its own feature_blockfetchproxy.py coverage.

### Verdict
DISMISSED (assessment clean): the pruned-block-retrieval design
holds under all four review questions. The block-fetch-proxy
seed is closed; if the branch is proposed upstream, the R/M
template predicts the review surface (trust-boundary questions
are already answered in-code).

### Exact commands
- git show 477ce33c4b 5f5a08f570 c79ee794ae 482d9d9d7e
  (--stat and net_processing.cpp/txindex.cpp/transaction.cpp/
  rest.cpp/rawtransaction.cpp/txoutproof.cpp)

### Limitations / queue
- Static only; the series' own functional test was not run (it
  lives on the branch, not in this tree).
- knots fixes-line on next radar cycle.

## Rotation note
Nine cycles; the top in-scope seed is assessed and closed.
Radar returns to periodic scanning.

## Cycle 10 (2026-07-29): knots fixes-line re-scan — still quiet (same tip and tag)

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=15737651929817465930, masked 6514279892962690122, index 0
(of 2) -> #65 (tenth cycle; periodic knots scan). Branch:
audit/contributor-radar-c10 from d4752cd265 (#80 c11 journal tip).

### Result
knots/29.x-knotsfixes tip unchanged (d941a2618b); latest tag
unchanged (v29.3.knots20260508). No new fixes topics since c5/c7.

### Verdict
Nothing to assess. Radar cell closed.

### Exact commands
- git fetch --depth=40 knots 29.x-knots 29.x-knotsfixes;
  git log -3 knots/29.x-knotsfixes; git ls-remote --tags knots

### Limitations / queue
- Next radar interval: l0rinc + knots re-scan.

## Rotation note
Ten cycles; radar quiet.

## Cycle 11 (2026-07-30): periodic re-scan — both remotes quiet

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=17777575239433090151, masked 8554203202578314343, index 1
(of 2) -> #65 (eleventh cycle; periodic radar). Branch:
audit/contributor-radar-c11 from 01e2eb7549 (#69 c4 journal tip).

### Result
- l0rinc/master: unchanged (same upstream merges; no new fork
  commits). 859 branches.
- knots 29.x-knotsfixes: tip unchanged (d941a2618b); latest tag
  unchanged (v29.3.knots20260508).

### Verdict
Nothing new to assess. Radar cell closed.

### Exact commands
- git fetch l0rinc --prune; git ls-remote --heads l0rinc
- git fetch --depth=40 knots 29.x-knots 29.x-knotsfixes

### Limitations / queue
- Next radar interval per rotation.

## Rotation note
Eleven cycles; radar quiet.

## Cycle 12 (2026-08-02, draw 198, raw=3654393603145900293 (63-bit), idx 28/35): radar — NEW author branch txgraph-retained-entry-usage assessed: memory-accounting gap REAL at HEAD (mechanism verified), bounded by peak watermark; author's fix in flight; 🟡 URGENT entry

### Fetch results
- git fetch l0rinc --prune: ONE new branch (862 total):
  l0rinc/l0rinc/txgraph-retained-entry-usage (2 commits,
  2026-08-01 18:25 -0700 — fresh).
- knots: new tag v29.4.knots20260508rc1 (rc of the next knots
  release; release-pipeline event, not a code-assessment cell).
- l0rinc/master tracks origin/master (556988790a).

### Assessment (the author's new WIP)
Claim: GetMainMemoryUsage charges Entry storage by LIVE txcount
while Compact retains m_entries capacity — churn leaves allocated
memory outside DynamicMemoryUsage and -maxmempool accounting.
- HEAD verification: TxGraphImpl::Compact's removal path
  (txgraph.cpp:1875-1900) swaps to end + pop_back — capacity
  RETAINED (no shrink on m_entries; :1458-1459 shrink_to_fit
  covers m_linearization/m_mapping only). Charge formula at
  :3762-3775 is sizeof(Entry)*txcount. GAP CONFIRMED REAL.
- Bound: retained capacity <= peak tx watermark since startup —
  retained-old memory, not unbounded growth; RSS can exceed the
  -maxmempool-derived expectation persistently after churn.
  No consensus/remote primitive; accounting accuracy class.
- Branch quality (static): 475ab49da6 charges
  memusage::DynamicUsage(m_entries) (capacity-aware) + early
  zero-when-empty guard preserving the empty-graph contract;
  b793679ed5 adds the churn characterization test. Sound shape.

### Verdict
RADAR FINDING: real, bounded, author-fix-in-flight. No local
action (author's WIP; radar does not duplicate). 🟡 URGENT entry
added (adopt/review when landed; RSS-vs-accounting re-check on
adoption). Pruned the archived load_wallet ✅ to keep cap 10.

### Exact commands
- git fetch l0rinc --prune; git ls-remote --heads l0rinc (862);
  git show 475ab49da6; sed txgraph.cpp:1456-1460, 1859-1900,
  3762-3775.

### Limitations / queue
- The churn test on the branch was read, not executed (author's
  CI is the natural runner; adoption cycle can re-run it).
- knots v29.4 rc1 not diffed (release process, separate watch).

## Cycle 13 (2026-08-02, draw 236, raw=17935444686604899945, masked 8712072649750124137, idx 1/2): radar — NEW author branch package-weight-accumulator assessed: narrowing real, wrap UNREACHABLE under the count cap; hygiene/diagnosis-ordering fix, no acceptance path; no URGENT change

### Fetch
- git fetch l0rinc --prune: ONE new branch (863 total):
  l0rinc/l0rinc/package-weight-accumulator (2 commits,
  2026-08-01). knots: unchanged (v29.4.knots20260508rc1).

### Assessment
Claim: std::accumulate(..., 0, ...) narrows the accumulator to
int; a wrapping package skips 'package-too-large' and reaches
later checks.
- HEAD verification: the literal-0 accumulate is present at
  policy/packages.cpp:87 (total_weight int64_t, init int).
- REACHABILITY bound: count is capped FIRST
  (MAX_PACKAGE_COUNT=25, packages.h:19) before the accumulate,
  and consensus weight per tx caps at 4M WU
  (MAX_STANDARD_TX_WEIGHT; packages.h:25 static_assert chains
  MAX_PACKAGE_WEIGHT=404,000 to it). Max possible sum =
  25 x 4M WU = 100M WU — 21x SHORT of the 2.147G WU needed to
  wrap int32. The wrap is UNREACHABLE under policy bounds.
- Even at the unreachable extreme: skipping the too-large
  rejection only reorders diagnosis — per-tx PreChecks weight
  checks still reject oversized members downstream. No invalid
  acceptance path exists (the branch's own test delta is the
  reject REASON: 'package-contains-duplicates' ->
  'package-too-large').
- Upstream context: #35473 proposed the same fix, closed over
  its regression test; the author pairs the one-liner with a
  focused sanitization case (de08e28551) — correct hygiene.

### Verdict
RADAR NOTE: mechanism technically real (accumulator narrowing),
impact unreachable (count-capped sum can't wrap) and
diagnosis-ordering at worst. Branch is sound hygiene; NOT a
security cell — no URGENT entry, no adoption urgency. Watch
continues.

### Exact commands
- git fetch l0rinc --prune (863); git show 2481dc2aec;
  sed packages.cpp:80-92, packages.h:19-29.

### Limitations / queue
- The arithmetic bound assumes the count check precedes the
  accumulate (verified in the same function, :84-87).
- knots v29.4 rc1 still not diffed (release-process watch).

## Cycle 14 (2026-08-02, draw 244, raw=8935267519944944495, n=1): radar — NEW branch rpc-deduplicate-scan-objects assessed: perf nicety (skip exact-repeat scan-object expansion), semantics-preserving by set-dedup design; package-weight-accumulator force-updated (author rebase); no URGENT change

### Fetch
- git fetch l0rinc --prune: ONE new branch (864 total):
  l0rinc/l0rinc/rpc-deduplicate-scan-objects (2 commits,
  2026-08-01). package-weight-accumulator force-updated to
  4eaf3c6595 (author's own rebase, expected WIP flow).

### Assessment
- Mechanism: descriptor scan RPCs (scantxoutset :2421,
  scanblocks :2656, getdescriptoractivity :2815) expand every
  request object independently; exact repeats re-parse/re-derive
  with no result change (derived scripts land in SETS).
- Fix shape: track serialized scanobject.write() in an
  unordered_set, skip exact repeats pre-expansion; different
  spellings deriving the same scripts keep the existing path
  (commit's own scoping note).
- Correctness: semantics-preserving by construction (set-based
  script collection; response shapes unchanged). Scope:
  authenticated local RPC, self-inflicted CPU only — no
  security/consensus angle, no acceptance-path impact.
- Branch quality: minimal diff (3 call sites + helper + test
  9567eb8c99 covering duplicate scans). Sound.

### Verdict
RADAR NOTE: perf nicety, no defect class. No URGENT entry; no
local action.

### Exact commands
- git fetch l0rinc --prune (864); git show 14ac2d4f18 (diff
  refs above); git log package-weight-accumulator (4eaf3c6595).

### Limitations / queue
- Dedup-by-serialized-form misses semantically-equal-different-
  spelling objects (the commit's documented scoping; fine).

## Cycle 15 (2026-08-02, draw 249, raw=173496863651005843, n=1): radar — zero delta (864 branches, no new, no force-updates); quiet

### Fetch
- git fetch l0rinc --prune: nothing new (864 branches, same as
  c14; the 3 recently-assessed branches unmoved since their
  assessments).

### Verdict
Quiet cycle; radar interval produced nothing.

### Exact commands
- git fetch l0rinc --prune; branch count above.

## Cycle 16 (2026-08-02, goal-resumed cycle 263): ADOPT the retained-capacity fix — 475ab49da6 cherry-picked onto the lineage (clean; churn test already present from #23 c5); flipped test GREEN; mempool-scale profile unchanged within noise (retained capacity at 1,600-tx peaks is ~100KB, under RSS noise); 🟡 -> ✅ locally verified

### Adoption (audit/adopt-retained-capacity @ 28ba79168b)
- Cherry-pick 475ab49da6 onto agent/all-findings tip: clean
  (the characterization test was already in-lineage from #23
  c5's fc48fd6345; the fix's 2-line test edit applied).
- Verification: test_bitcoin txgraph_tests FULL suite green
  including the FLIPPED expectation (churned usage now GT fresh
  — capacity-aware charging active: DynamicUsage(m_entries)
  counts retained vector capacity).
- Mempool-scale before/after (#22 c4 profile rerun, /tmp/btc263):
  usage_drained returns to ~0 both ways (as designed — the
  charge now includes retained capacity, which at 1,600-tx
  peaks is ~100KB); RSS delta 3.1MB vs 3.3MB pre-fix — the
  residual RSS retention is allocator slack, not the vector
  (expected: the fix charges accounting, it cannot shrink RSS
  at this scale; its value is accounting truth at LARGE churn
  scales).

### Verdict
✅ ADOPTED + VERIFIED locally: the author's fix works as
designed (unit-level flipped test green; accounting now
includes retained capacity; no regression in the full suite or
the churn profile). The author's branch remains the upstream
vehicle; our lineage carries it for the next profiles.

### Suspicion-mining (new protocol)
- S1 (recorded): DynamicUsage(m_entries) charges retained
  capacity until graph teardown — by design (charge the
  allocation, not the count); overcount direction is the SAFE
  one (conservative accounting).
- S2 (recorded): RSS-vs-accounting gap at small scale is
  allocator slack — a massif run would attribute it, but the
  unit test already pins the vector term; not queued.

### Exact commands
- cherry-pick line above; test run above; profile rerun above.

### Limitations / queue
- 1,600-tx scale hides the vector term in RSS; a maxmempool-
  scale churn would show the accounting delta directly (disk/
  time-bounded out).

## Cycle 17 (2026-08-02/03, draw 267, raw=18287398478253595800, suspicion-mined from the 26-PR sweep): PR 35839 empty-headers sync slot — stall CONFIRMED at HEAD with the PR's own test (failing-before) + ADOPTED (passing-after green)

### Defect (liveness, headers-sync layer)
During IBD a sync peer may answer valid EMPTY headers (truthfully
at/behind our height, or adversarially). At HEAD: the empty
response does NOT release the peer's sync slot —
IsContinuationOfLowWorkHeadersSync returns success=false for
empty input (headersync.cpp:76-77), m_last_getheaders_timestamp
is not cleared, and the slot-release check at
net_processing.cpp:6300 only fires when a replacement exists
(m_num_preferred_download_peers - fPreferredDownload >= 1). So
IBD stalls on a slot the peer will never fill.
- FAILING-BEFORE: the PR's new test ('Test empty headers
  response during initial sync') FAILS at unfixed HEAD
  (AssertionError not(True == False)) — the stall is live in
  our lineage, proven by upstream's own regression test.

### Adoption (audit/adopt-empty-headers, 5 commits)
- Stack picked: 297c0f7ca7 (name stale check), 7de4fba5b8
  (extract cleanup), 79aca0b97f (main fix), 92a98ffb30
  (characterize), 6d10e8e193 (continue pending disconnect).
- Conflict resolutions recorded: (a) ReleaseHeadersSyncSlot
  decl+impl kept — 297c0f7ca7 deletes it as an EARLIER stack
  commit applied last by my order (the helper survives to the
  branch tip, 4 mentions); (b) test-file wholesale take after a
  bad block-union mixed helpers (labeled repair commit).
- PASSING-AFTER: p2p_initial_headers_sync.py Tests successful
  (old timeout cases + new empty-slot + eviction cases);
  bitcoind builds clean.

### Verdict
CONFIRMED + ADOPTED: the empty-headers stall is real at HEAD
(liveness class, IBD-relevant); the author's state-machine fix
(release the slot, select another peer) is carried with
upstream's own test as the regression oracle. Upstream vehicle:
PR 35839 (open, 2026-07-29).

### Suspicion-mining
- S9: stack commits can DEPEND on sibling commits' deletions —
  always verify helper survival to the branch tip before
  resolving (rule added to the adoption checklist).
- S10: the in-code :5977-5981 pprev mitigation only re-requests
  from one-earlier — it does NOT release the slot (the PR's own
  scoping comment matches).

### Exact commands
- git fetch l0rinc l0rinc/p2p-empty-headers-sync; failing-before
  run (/tmp/btc267, AssertionError above); cherry-picks +
  resolutions above; passing run (/tmp/btc267c).

### Limitations / queue
- The noban-peer eviction arm relies on the PR's test (not
  separately driven).

## Cycle 18 (2026-08-03, draw 268, raw=1992446166920522805, suspicion-mined): PR 35195 hash-code cache tradeoff — premise MEASURED on this host (~1% runtime cost of noexcept recalc); deliberate documented tradeoff, NOT a defect; no adoption

### Assessment
- Mechanism (#16957, hasher.h:63-68): noexcept hasher ->
  libstdc++ recomputes instead of caching hash codes in nodes
  (documented: ~1.6% slower vs ~9.4% RSS saved).
- Host A/B (ComplexMemPool, --min-time=1500, gcc 13.3 aarch64):
  noexcept (HEAD): 183.88 ms/op (5.44 op/s, err 0.8%);
  throwing (hash-cached nodes): 182.12 ms/op (5.49 op/s, err
  0.2%). Delta -1.0% time — direction matches the documented
  penalty, smaller on this bench (within ~2x err bars).
- RSS arm NOT measured here (bench harness reports no RSS;
  #16957's 9.4% was on the original machine class).

### Verdict
Premise CONFIRMED-directional; NOT a defect — a documented
memory-vs-speed tradeoff whose reversal is an upstream design
call. NO adoption (flipping it locally would contradict
#16957's measured memory win with no correctness stake).

### Suspicion-mining
- S11: the A/B was nearly ruined by a restore-during-build race
  (the #75-c5 modified-during-compile trap, hit and redone
  deterministically — recorded).
- S12: hash-cached nodes cost sizeof(size_t) per cache entry —
  at ~500k UTXO-cache entries that's ~4MB, the real #16957
  number to beat with the 1% runtime gain.

### Exact commands
- sed toggle + rebuild x3 (noexcept/throwing/restore);
  bench lines above.

### Limitations / queue
- RSS arm unmeasured (needs /usr/bin/time long-run, out of
  cycle); the tradeoff's RSS side stays #16957's number.
