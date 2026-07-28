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
