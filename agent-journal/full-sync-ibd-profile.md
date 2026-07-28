# Campaign #22 — full-sync-ibd-profile

Base: audit/resurrection @ f6c0e72598 (rotation ledger commit for #28 c2).
Branch: audit/full-sync-profile. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): -loadblock import replay — consistency machinery dominates again (import = reindex in profile shape)

### Workload / environment (recorded per campaign)
- Workload: `-loadblock` import of a 5000-block empty (coinbase-only)
  regtest chain (16 MiB blk00000.dat), fresh scratch datadir per run.
- Source-data gotcha (now documented empirically): `-loadblock` does NOT
  support the obfuscated blk*.dat of a live datadir (init.cpp:535 help text:
  "Obfuscated blocks are not supported" — verified: import silently accepts
  0 blocks, node idles at 0 with 0.23s user CPU). De-XORed with the
  source's xor.dat key (8-byte repeating); plain file starts with the
  regtest magic fabfb5da as expected. Script kept in shell history.
- Host/build: same as #21 (Cortex-A76, build-before gcc Release w/ asserts).
- Stop condition per run: getblockchaininfo = 5000 blocks,
  initialblockdownload=false, then stop. /usr/bin/time -v, foreground.

### Baseline (2 runs)
| run | wall | user | CPU% | RSS |
|---|---|---|---|---|
| base1 | 13.34s | 12.52s | 96% | 60 MB |
| base2 | 15.09s | 13.05s | 89% | 60 MB |
Both: 5000 blocks, IBD complete.

### Profile attribution (perf record -F 199 -g, one run)
b-initload thread: 58.45% base_uint<256>::CompareTo, 16.90%
ChainstateManager::CheckBlockIndex, 10.39% CBlockIndexWorkComparator,
2.24% CChain::SetTip — ~85-88% in block-index consistency/work-comparison
machinery. File read, deserialization, LevelDB, crypto: all <1% each.
(Only other visible line: TxConfirmStats::UpdateMovingAverages 1.71% on
b-scheduler — fee-estimator bookkeeping on coinbase-only chain.)

### Hypothesis and test
H (carried from #21, now generalized): the wall time is dominated by the
regtest-default block-index consistency machinery
(DefaultConsistencyChecks()==true on regtest), not by import work.
Test: identical runs with -checkblockindex=0.

| run | wall | user |
|---|---|---|
| nocbi1 | 2.23s | 0.81s |
| nocbi2 | 2.24s | 0.76s |

Movement: wall 13.3-15.1s -> 2.23s (6.0-6.7x), user ~13s -> ~0.8s (~16x).
CONFIRMED, reproducible. Final results match: 5000 blocks imported,
IBD done, same source chain in both modes (the consistency check is a
read-only audit; skipping it cannot change the imported chainstate).

### Verdict
- Profile hypothesis CONFIRMED for the import path too: consistency-check
  domination is a GENERAL property of regtest rebuild paths (reindex #21,
  external import #22), not reindex-specific.
- No code defect; the regtest default is deliberate. The import path's
  residual 2.2s (after disabling) matches reindex's residual — file
  read + AcceptBlock + LevelDB batches.
- Empirical -loadblock obfuscation constraint documented (see gotcha
  above): feeds the doc-accuracy campaigns; help text already states it,
  no doc fix needed.

### Exact commands
- prep: generate 5000-block chain in /tmp/btc_src22; de-XOR blk00000.dat
  with xor.dat key -> /tmp/blk_plain.dat
- measure: `/usr/bin/time -v bitcoind -regtest -datadir=/tmp/btc_dst22
  -loadblock=/tmp/blk_plain.dat [-checkblockindex=0]` (foreground) +
  CLI poll to 5000/False + stop; fresh datadir per run
- profile: `perf record -F 199 -g` on one import run

### Limitations / leads
- Empty blocks: no script/tx validation cost; a tx-heavy import (or real
  mainnet replay) is a different profile. Two baseline runs only
  (spread 13% — larger than #21's 3%; medians used).
- Network-bound true IBD not attempted on this host (no fixed peer
  serving blocks; that would need a second node + pinned peers — queued).

### Next queue for this campaign
- True two-node P2P IBD replay on regtest (fixed -connect peer serving
  the prepared chain): separates download vs validation per the campaign
  text; measure with -checkblockindex=0 to expose the real pipeline.
- tx-heavy chain import (500 blocks x 100 txs): expect coins/LevelDB and
  script shares to appear.
- Disk/watch: scratch removed after cycle (datadirs, plain blk, perf data).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-28): two-node P2P IBD replay — bloom reset-per-tip-change dominates; clean-flag fix (c8f53e58d9)

Base: 414aa24aec (journal commit for #75 cycle-1 on
audit/build-throughput; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/full-sync-ibd-c2 (c1 journal carried over in
51530863b1). Start state: clean (untracked scratch only).

### Draw
Random draw over the 31-goal eligible pool (32-pool minus #75):
raw=1670674105687797563, seed masked to 63 bits, index 6 -> #22.
This was also the queued item from c1 ("True two-node P2P IBD replay").

### Workload / environment
- Server: 5000-block empty regtest chain (generatetoaddress, 43s),
  -checkblockindex=0, -bind=127.0.0.1:28444, wallet "prep" only for
  block generation. Client: fresh datadir per run,
  -connect=127.0.0.1:28444 -listen=0, default dbcache/prune/threads.
- Host/build: as c1 (Cortex-A76 4 cores, build-before gcc Release,
  asserts on). Harness: /tmp/btc22_run.sh (prep/serve/run/stop).
- Stop condition: client getblockchaininfo = 5000 blocks AND
  initialblockdownload=false, then stop; /usr/bin/time -v foreground.
- Harness gotcha: this tree's bitcoind does NOT auto-create a missing
  -datadir (init exits "Specified data directory ... does not exist");
  mkdir first. bitcoin-cli needs a created wallet for generatetoaddress.

### Baselines (fresh client datadir each)
| run | mode | wall | user | sys | RSS |
|---|---|---|---|---|---|
| nocbi1-3 | -checkblockindex=0 | 2.96/2.96/2.97s | 3.26/3.08/3.24 | ~0.55 | 57 MB |
| cbi_on | default checks | 17.02s | 17.71 | 0.87 | 57 MB |
Network volume (getnettotals): 1.76 MB recv / 295 KB sent for 5000
empty blocks (~350 B/block). All runs: final tip
45006056e6a6...e5bd6c49 height 5000, matching the server.
- cbi_on confirms c1's generalization on the true P2P path: the
  regtest-default block-index consistency machinery dominates wall
  (17.0s vs 2.96s, 5.7x) — now shown for download+validation, not just
  reindex/import.

### Pre-fix profile (perf record -F 199 -g on client, nocbi)
- 42.05% __memset_zva64 on b-msghand, 40.63% of it from
  CRollingBloomFilter::reset() <- TxDownloadManagerImpl::ActiveTipChange
  <- PeerManagerImpl::ActiveTipChange <- ValidationSignals per accepted
  block.
- 15.95% TxConfirmStats::UpdateMovingAverages (fee estimator, per
  block, b-scheduler). 2.19% sha512 (BIP324 v2 transport crypto —
  legitimate; -connect peers negotiate v2).
Mechanism (the falsifiable part): net_processing.cpp:2104 already gates
the reset on !is_ibd, BUT IsInitialBlockDownload() latches false after
the first connected block on regtest (minimum chain work = 0, fresh
blocks have current timestamps, tip within 24h), so blocks 1..4999 each
pay a double zero-fill: 2 x ~863 KiB (120000-entry, fp 1e-6, 2
bits/entry rolling filters) = ~1.7 MiB memset per block. Reachability
bound: test networks (regtest/signet fresh chains) and the short
mainnet catch-up tail after the latch flips (~144 blocks); steady-state
mainnet = one 1.7 MiB memset per ~10-min block = noise.
CONFIRMED as a profile defect on reachable (test-network) paths.

### Fix evaluation and design constraint
reset() also re-randomizes nTweak (hash re-salt, DoS hardening) and
resets generation counters — a naive "skip reset" would trade a
privacy/DoS property, which the campaign forbids. The applied fix keeps
the re-salt and counters, and skips ONLY the zero-fill when the filter
is already all-zero (new m_all_zero flag: set by constructor/reset,
cleared by any insert). When the flag is set the fill is the identity,
so the post-state is bit-identical; per-block re-salt cadence
unchanged. 9 insertions, 1 deletion in src/common/bloom.{h,cpp}.

### After-fix controls (same harness, same chain)
| run | wall | user |
|---|---|---|
| fixed1-3 | 1.65/1.33/1.68s | 1.37/1.31/1.37s |
Movement: user -58%, wall -46% vs nocbi baseline. Final tip hash
identical. Post-fix perf: CRollingBloomFilter::reset() below the 3%
report limit (was 42%); new top = TxConfirmStats::UpdateMovingAverages
21.71%. Deterministic control: test_bitcoin --run_test=bloom_tests
passes (No errors detected).

### Verdict
- CONFIRMED + FIXED (c8f53e58d9): per-tip-change rolling-bloom
  zero-fill dominated regtest P2P IBD CPU (40.6%); clean-flag skip
  removes it with provably identical post-state and no DoS/privacy
  tradeoff.
- Campaign-level hypothesis (c1) re-confirmed on the P2P path:
  consistency checks dominate wall when enabled (17.0s vs 2.96s).
- Download-vs-validation split (campaign requirement): with checks off,
  validation share for empty blocks is tiny; pre-fix CPU was dominated
  by net-processing bookkeeping (bloom reset) and fee estimation, not
  block validation or I/O (disk read, LevelDB, deserialization all
  below the 3% report limit).

### Exact commands
- /tmp/btc22_run.sh prep|serve|run <label> [-checkblockindex=0]|stop
  (bitcoind -regtest -datadir=... -connect=127.0.0.1:28444 -listen=0)
- /usr/bin/time -v per run; getnettotals per run; getbestblockhash
  comparison vs server
- perf record -F 199 -g (pre and post fix), perf report --no-children
  [-g none --percent-limit]
- build-before/bin/test_bitcoin --run_test=bloom_tests

### Why existing tests missed it
Pure performance issue: bloom_tests assert insert/contains/reset
semantics (unchanged, pass); the fork's txdownloadman fuzz oracle
asserts filter emptiness after ActiveTipChange (holds either way);
no benchmark covers regtest IBD net-processing bookkeeping.

### Repair ordering / masking note
Clean controls (3x nocbi + cbi_on + pre-fix perf) were all taken BEFORE
the patch; the patch was then applied and rebuilt (173-edge cold
rebuild — bloom.h reaches ~190 TUs via net.h), then after-fix runs used
the identical harness/chain/server. No interleaving; server tip never
changed (45006056...).

### Limitations / queue
- Fee-estimator UpdateMovingAverages per connected block (21.7%
  post-fix, 16% pre-fix) is the next exposed bottleneck — queued for
  the next #22 cycle (check whether IBD/latch gating applies there
  too; BlockUntilSyncedToCurrentChain/estimateSmartFee interactions).
- tx-heavy chain import (500 blocks x 100 txs) still queued from c1:
  expect coins/LevelDB/script shares.
- The lazy-guard in TxDownloadManagerImpl::ActiveTipChange (don't even
  allocate the filters on first call) was considered and folded into
  the bloom-level fix (allocation happens once; the fill was the cost).
- Fuzz-build differential on txdownloadman not run post-patch (long
  rebuild; equivalence argued from the identity property + bloom_tests).
- Server datadir kept for nothing; all scratch removed
  (/tmp/btc22_{srv,cli}, perf data). No daemons left running.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted (2 leads
queued above).
