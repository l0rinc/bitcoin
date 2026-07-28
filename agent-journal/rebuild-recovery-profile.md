# Campaign #21 — rebuild-recovery-profile

Base: audit/resurrection @ 26eaf1e179 (rotation ledger commit for #17).
Branch: audit/rebuild-recovery-profile. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): regtest reindex dominated by consistency checks — profiled + hypothesis confirmed

### Workload / environment (recorded per campaign)
- Workload: `-reindex` over a 5000-block empty (coinbase-only) regtest
  chain, 24 MiB datadir, page-cache resident.
- Host: Cortex-A76 (4 cores), 15GB RAM, NVMe (99% full — scratch kept tiny),
  kernel perf 6.8.12, perf_event_paranoid=4 (user-space sampling OK).
- Build: build-before, gcc, CMAKE_BUILD_TYPE=Release (asserts enabled;
  this tree does not pass -DNDEBUG).
- Data prep: fresh datadir, `generatetoaddress 5000` (deterministic empty
  blocks), stop. Stop condition per run: getblockchaininfo = 5000 blocks,
  initialblockdownload=false, then bitcoin-cli stop. /usr/bin/time -v on a
  foreground bitcoind (daemon mode defeats measurement — noted).

### Baseline (3 identical runs)
| run | wall | user | CPU% | peak RSS |
|---|---|---|---|---|
| base1 | 13.73s | 11.98s | 89% | 61 MB |
| base2 | 13.87s | 12.04s | 88% | — |
| base3 | 14.18s | 11.98s | 86% | — |
Median wall 13.87s (spread ±3%); ~2.7 ms/block. Classification: CPU-bound
(user 12.0s vs sys 0.31s), single dominant thread (b-initload).

### Profile attribution (perf record -F 199 -g, b-initload thread)
- 61.95% base_uint<256u>::CompareTo (self)
- 17.86% ChainstateManager::CheckBlockIndex (self)
- 11.24% CBlockIndexWorkComparator::operator()
- ~92% total in block-index consistency/work-comparison machinery;
  sha512 0.59%, sha256 0.08%, LevelDB/obfuscation/memset all <1%.
  I/O, serialization, crypto: negligible.

### Hypothesis and test
H: the wall time is ~all startup block-index consistency machinery, which
runs by default on regtest (`DefaultConsistencyChecks()` true on regtest,
false on mainnet; init.cpp:669), NOT the reindex work itself.
Test: identical rerun with `-checkblockindex=0`, expecting a large wall drop.

| run | wall | user |
|---|---|---|
| nocbi1 | 2.23s | 0.56s |
| nocbi2 | 2.23s | 0.50s |

Movement: wall 13.87 -> 2.23s (6.2x), user 12.0 -> 0.5s (24x). CONFIRMED,
reproducible. Correctness: identical final state (5000 blocks, IBD done,
same chain/datadir) in both modes; the consistency check is a read-only
audit, so skipping it cannot alter the rebuilt index.

### Verdict
- Profile hypothesis CONFIRMED with reproducible metric win
  (configuration-level). No code defect: the consistency check is a
  deliberate regtest default; mainnet default is off.
- Noteworthy magnitude: ~2.4 ms/block of pure index-invariant comparison
  on a perfectly linear chain (arith_uint256 work comparisons dominate) —
  relevant context for any future CheckBlockIndex optimization work
  (e.g. mainnet -checkblockindex=1 runs at 880k+ blocks).

### Exact commands
- prep: `bitcoind -regtest -datadir=/tmp/btc_r21 -daemon -server
  -fallbackfee=0.00001 -dbcache=300`; `bitcoin-cli ... generatetoaddress 5000`
- measure: `/usr/bin/time -v bitcoind -regtest -datadir=... -reindex
  [-checkblockindex=0]` (foreground) + CLI poll to 5000/False + stop
- profile: `perf record -F 199 -g -o /tmp/r21.perf bitcoind ... -reindex`;
  `perf report --stdio --no-children`

### Limitations / leads
- Empty blocks: tx validation, script checks, and UTXO-cache pressure are
  absent by design; this profiles the block-index/chainstate-rebuild path
  only. A tx-heavy chain (or mainnet replay) is a different workload.
- Phase attribution inside the residual 2.23s not split (file read vs
  LevelDB batch write) — small; queued.
- Reproducibility under 3 runs only; no cross-host comparison.

### Next queue for this campaign
- tx-heavy regtest chain reindex (e.g. 2000 blocks × 500 txs): re-run the
  same protocol; expect LevelDB/coins shares to grow.
- Wallet rescan profile (rescanblockchain over the prepared chain).
- assumeutxo snapshot load profile (loadtxoutset) if a scratch snapshot
  can be produced within disk budget.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.
