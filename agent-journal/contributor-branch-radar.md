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
