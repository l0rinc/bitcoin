# Campaign #63 — loupe-style-pipeline

Base: 0bd9b11894 (journal commit for URGENT.md #24-c1 item on
audit/disk-io; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/loupe-pipeline. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): four-stage pipeline on the fee-estimator lead — CONFIRMED + FIXED; estimator waste eliminated from zero-state IBD

### Draw
Random draw over the 57-goal pool (38 pending + 19 CYCLE-1; #24
excluded as just-cycled): raw=17607707707087155929, seed masked to 63
bits (8384335670232380121), index 18 -> #63. Lead taken from URGENT.md
(⚪ fee-estimator UpdateMovingAverages per-block cost, #22 c2 queue).

### Stage 1 — SCOUT (this agent, no fix yet)
Lead: TxConfirmStats::UpdateMovingAverages = 21.7% of post-bloom-fix
regtest IBD CPU (#22 c2 perf). Scout read: processBlock
(block_policy_estimator.cpp:730) has no is_ibd gate (unlike the bloom
reset family); call path ConnectTip -> removeForBlock
(validation.cpp:3144) -> signal (txmempool.cpp:461) -> processBlock,
all unconditional. Scout hypothesis: with zero tracked state the
bucket sweeps are pure waste (decay of zeros is zero).

### Stage 2 — VERIFIER (independent subagent, clean HEAD, no fix seen)
- CONFIRMED claim 1: no gate anywhere; upstream master structurally
  identical (fetched raw).
- CONFIRMED claim 2 (reproduced): UpdateMovingAverages = 20.43% of
  1279 samples (perf -F 199 -g, two-node regtest IBD, 20000 empty
  blocks, b-scheduler); processBlock total 20.93%; ClearCurrent 0.41%.
- REFINED claim 3: naive skip-if-empty predicate is UNSAFE —
  removeTx()'s failAvg branch can dirty stats without
  firstRecordedHeight, and Read() restores nonzero averages with
  firstRecordedHeight==0. Exact-skip requires an explicit dirty flag
  cleared by Record(), the removeTx failAvg branch, and Read().
- Bonus: fork's own estimator hardening (6 commits) reviewed;
  debunked a small-sample NodeClock artifact; noted perf host had
  auto-lowered perf_event_max_sample_rate to 1 (fixed, documented).

### Stage 3 — FIXER (minimal, behavior-identical by construction)
TxConfirmStats::m_all_zero (init true): cleared in Record() (after
the blocksToConfirm<1 early-return), in the removeTx failAvg branch,
and conservatively at the end of Read(); UpdateMovingAverages()
returns early when set. Decay of all-zero doubles is bit-identical
zero, so no observable state changes (nBestSeenHeight, ClearCurrent,
processBlockTx, firstRecordedHeight, persistence all untouched).
19 insertions, 1 file. Fixes the fork's mission fit: the ~38.4k
double multiplies per block x3 stats are skipped until the first
relevant datum exists (all of IBD for a fresh node; the estimator
tracks only mempool-seen transactions).

### Stage 4 — REVIEWER (rerun failing-before/passing-after)
- Before (verifier, clean HEAD): UpdateMovingAverages 20.43% of IBD
  samples; user CPU 1.31-1.37s (5000-block runs, #22 c2 lineage).
- After (this branch, patched): UpdateMovingAverages 0 samples (below
  report floor) in the same 5000-block P2P IBD; top symbol is BIP324
  sha512 7.15%; user CPU 0.89s / wall 1.63s (-34% user vs the
  1.31-1.37s post-bloom baseline); client tip == server tip
  (4b4f49d6...e437).
- Functional control: feature_fee_estimation.py (real estimates with
  real transactions) — TESTS SUCCESSFUL.
- No dedicated estimator unit suite exists (functional coverage only)
  — recorded as a test-gap note, not expanded this cycle.

### Verdict
- CONFIRMED + FIXED (performance): zero-state per-block estimator
  waste eliminated with a provably behavior-identical fast path;
  measured -34% user CPU on the 5000-block regtest IBD profile, 0
  residual UpdateMovingAverages samples.
- Master-relative severity: low (test-network/young-chain IBD CPU;
  mainnet IBD pays the same waste until the first mempool-relevant
  block but the absolute cost is small). Not correctness.

### Exact commands
- verifier subagent report (agent-0): code + upstream raw fetch +
  two-node IBD perf (vfy63_* scratch)
- fix: edit src/policy/fees/block_policy_estimator.cpp (m_all_zero)
- build: ninja -C build-before bin/test_bitcoin bin/bitcoind
- profile: server 5000 empty blocks; client perf record -F 199 -g
  -connect=127.0.0.1:28971 -listen=0 -checkblockindex=0
- control: python3 test/functional/feature_fee_estimation.py
  --configfile=build-before/test/config.ini --tmpdir=/tmp/btc63_func

### Limitations / queue
- ClearCurrent not gated (0.41% — not worth the map-emptiness
  invariant analysis per verifier).
- The verifier's FlatFileSeq::Open 12.79% (per-block file-handle
  churn) observation goes to the #24 queue.
- generatetoaddress throughput decay (108 -> 19 blocks/s by height
  20000, wallet-growth vs write path) unpinned — queued as a scout
  lead for #21/#24.
- No estimator unit suite (coverage via functional tests only) —
  queued as a #19/#9 test-gap item.

## Rotation note
Cycle 1 complete with a confirmed fix; rotating per uber-goal policy.
Not exhausted (pipeline composition pass over accumulated findings
queued).
