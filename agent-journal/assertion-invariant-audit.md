# Campaign #2 — assertion-invariant-audit

Base: 12ccbdc860 (journal commit for #21 cycle-2 on
audit/rebuild-recovery-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/assertion-invariant. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): fork-added production Assumes — all construction-tautologies; cache-overflow fix verified correct

### Draw
Random draw over the 53-goal pool (33 pending + 20 CYCLE-1; #21
excluded as just-cycled): raw=9227947000855944028, seed masked to 63
bits (4574964001168220), index 0 -> #2.

### Method / audit set
Enumerated assertions added by fork commits (git log
--author=pap.lorinc, +-lines matching Assume/Assert/CHECK_NONFATAL,
test-dir lines excluded) and selected the two PRODUCTION sets for
deep reachability analysis (the rest are fuzz-target or unit-test
assertions, out of this campaign's release-path scope):
A. 0231239131 (interfaces mining lookup arity): Assume(results.size()
   == txids.size()) x2, wtxid x2 in src/node/interfaces.cpp
   getTransactionsByTxID/WitnessID.
B. 21a97c2721 (node: avoid cache allocation percentage overflow): 5
   Assumes at the end of CalculateCacheSizes (src/node/caches.cpp).

### Claim A: mining IPC positional contract — SAFE (fixed-by-construction)
Before the commit, both methods returned {} whenever NodeContext had
no mempool, violating the one-nullable-per-requested-id positional
contract (mining IPC clients align ids to results). The fix constructs
results(n) up front and fills per index; the Assumes restate the
arity. Falsification attempt: results is size-fixed at construction
(std::vector<CTransactionRef> results(txids.size())); no path resizes
it; the Assume is unfalsifiable (the good kind — it fires only if a
future edit breaks the arity). Reachability of the surrounding
behavior change: kernel/mining clients without a mempool now get N
nulls instead of 0 — contract-restoring, covered by the commit's
miner_tests.cpp additions (25 lines).

### Claim B: cache-budget Assumes — SAFE; the fixed overflow verified
The pre-commit code computed index budgets as total_cache * 10 / 100
and * 5 / 100 (multiply-first). With an extreme -dbcache the
intermediate multiply can wrap (total_cache * 10 overflows uint64
once total_cache > ~1.8e18 B; guarded upstream only by the
CalculateDbCacheBytes cap). The commit rewrites to division-first
budgets (tx_index_budget = total/10, secondary = total/20),
eliminating the wrap entirely, and adds 5 Assumes. Each audited:
 1. tx_index <= tx_index_budget — std::min construction. Tautology.
 2. txospender_index <= secondary_index_budget — same.
 3. filter_cache <= secondary_index_budget — (max/n)*n <= max <=
    budget (integer division only rounds down). Holds.
 4. total_cache == initial - filter - tx_index - txospender_index —
    exact arithmetic identity of the three subtractions; no underflow
    since the parts sum to <= 20% of initial. Holds.
 5. kernel_sizes.block_tree_db + coins_db + coins == total_cache —
    kernel::CacheSizes (src/kernel/caches.h:29-36) splits by tracked
    subtraction (block_tree_db=min(total/8,MAX); coins_db=
    min(rest/2,MAX); coins=rest) — lossless by construction. Holds.
Config-reachability: all five are unfalsifiable by any -dbcache /
index-flag combination; they are edit-guards, not input validation.

### Class check (campaign's key question)
No assertion in the audited set serves as untrusted-input validation:
A's inputs are IPC request vectors handled data-agnostically; B's
inputs are startup config reduced by std::min/divide-first before any
Assume. Fuzz-target asserts (fb6810ac8c chainstate_lifecycle,
0a2deeea1d chainstate delete) and unit-test asserts (ae2bf49412,
77c5a11526, a6e490ab5b, 4c106d58c9) are test-oracle scope, noted but
not in the release-path audit set.

### Verdict
DISMISSED: no invalid assumption, no missing validation, no misleading
contract in the fork's production Assume sets A/B. The one real defect
in the area (multiply-first overflow) was already fixed by the fork
with the correct division-first transformation; its Assumes are
unfalsifiable-by-construction edit-guards.

### Exact commands
- git log --format=%h --author='pap.lorinc' -- src/ (40 commits);
  per-commit -U0 +-line grep for Assume/Assert/CHECK_NONFATAL
- git show 0231239131 / 21a97c2721 (full diffs + commit rationales)
- src/kernel/caches.h:21-37 (kernel split losslessness)

### Limitations / queue for cycle 2
- Fuzz-only Assumes (G_ABORT_ON_FAILED_ASSUME blocks in txgraph,
  signet, net_processing) not audited this cycle — those are
  oracle-scope; a falsification pass over the txgraph saturation
  family (3ae78dbd25 lineage) is queued.
- Upstream-side assertions (assert/Assert in production pre-fork code)
  untouched — separate large cell, queued.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): txgraph falsification pass — CONFIRMED config-validation gap: -limitclustercount=0 accepted (fix 5e0a80ade5)

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=12410924129186828647, masked 3187552092332052839, index 4
(of 5) -> #52 (second cycle; c1 queue cell "txgraph saturation
family falsification"). Branch: audit/assertion-invariant-c2 from
5223346741 (#50 c12 journal tip).

### Hypothesis
A txgraph Assume is falsifiable from config/input: the
max_cluster_count bound Assume (txgraph.cpp:699-700) can be
violated by an unclamped -limitclustercount value.

### Trace (confirmed)
- txgraph.cpp:699-700: Assume(max_cluster_count >= 1) and
  Assume(<= MAX_CLUSTER_COUNT_LIMIT=64) at TxGraphImpl
  construction (txmempool.cpp:222, no intermediate clamp;
  MakeTxGraph forwards verbatim, txgraph.cpp:3800-3814).
- Validation chain: mempool_args.cpp:35 takes -limitclustercount
  raw (no lower clamp); :110 rejects ONLY > 64. 0 slips through;
  negative values wrap huge and are caught by :110.
- Empirical: build-before (Release) starts fine with
  -limitclustercount=0 (Assume compiled out) — mempool then
  rejects every transaction (txgraph.cpp:2143 total_count > 0).
  In assert-enabled builds the same input aborts at
  MakeTxGraph. Upstream master has the byte-identical gap
  (7dea464d6b).

### Fix (one buildable commit, failing-before/passing-after)
5e0a80ade5: symmetric lower-bound error in node/mempool_args.cpp
("limitclustercount must be at least 1"), boundary cases in
mempool_cluster.py (0 and 65 both rejected with clear messages;
full suite green). R14-style boundary validation, upstreamable.

### Verdict
CONFIRMED defect (small): config validation accepted a nonsense
cluster count whose effect is either an unfriendly startup abort
(assert builds) or silent total mempool rejection (release
builds). FIXED with deterministic regression evidence.

### Exact commands
- grep chain: txgraph.cpp:699-700/2143/3800, txmempool.cpp:222,
  mempool_args.cpp:33-42/108-113, kernel/mempool_limits.h:20,
  policy/policy.h:72
- build-before/bin/bitcoind -regtest -limitclustercount=0
  -datadir=/tmp/btc_lc0 (accepted pre-fix; rejected post-fix)
- git show origin/master:src/node/mempool_args.cpp (identical)
- python3 test/functional/mempool_cluster.py --configfile=
  build-before/test/config.ini --tmpdir=/tmp/btc_mcl (green)

### Limitations / queue
- The remaining txgraph Assume families (ref-graph invariants,
  sequence uniqueness, locator presence) are local invariants
  with no config/input surface found this cycle; a dynamic
  saturation-driven falsification (cluster stress on the mempool)
  is a heavier future cell.
- Upstream-side assertion sweep remains a separate large cell.

## Rotation note
Two cycles; the config-surface falsification is closed with a
fix. Not exhausted (dynamic saturation, upstream sweep).

## Cycle 3 (2026-07-30): txgraph Assume families under dynamic cluster stress — all hold; two tripwires proven; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=11926377350913041393,
masked=2703005314058265585, n=5, idx=0) -> assertion-txgraph-assume
-> #2 (third cycle; c2 queue cell "dynamic saturation-driven
falsification (cluster stress on the mempool)"). Branch:
audit/assertion-invariant-c3 from f697931c68 (#107 c2 tip).

### Method (assertion-trapping driver)
build-before is Release (Assume/assert compiled out), so the
driver compiles txmempool.cpp + txgraph.cpp INTO the binary with
-DABORT_ON_FAILED_ASSUME (check.h:28-32 gate) and without NDEBUG,
linking release libs for the rest — every Assume/assert in the
mempool/graph families aborts on violation; pool.check() (which
runs m_txgraph->SanityCheck, txmempool.cpp:499) runs after every
batch. Any abort = invariant failure.
Build: g++ -O1 -std=c++20 -DABORT_ON_FAILED_ASSUME -I src
-I build-before/src /tmp/cluster_stress.cpp src/txmempool.cpp
src/txgraph.cpp -ltest_util -lbitcoin_node -lbitcoin_common
-lbitcoin_consensus -lbitcoin_util -lbitcoin_crypto
-lbitcoin_clientversion -lleveldb -lcrc32c -lunivalue -lsecp256k1
(full link line in /tmp/cluster_stress build log).

### Phases (TestChain100Setup + 300 mined blocks; signed helper txs
via CreateValidMempoolTransaction; production ProcessTransaction /
ProcessNewBlock paths)
1. 64-chain: accepted with pool.check green every 10; the 65th
   member rejected 'too-large-cluster' (exact cluster-limit
   contract); check green at 64.
2. 40 parallel 4-tx clusters with fee tiers (224 txs): check green.
3. Trim: own pool filled to 4,064,136 B (310 txs, 500-output
   fan-outs), TrimToSize(4.05M) -> 4,039,032 B; small-pool check
   green after trim.
4. REAL block: 20 topologically-selected pool txs mined via
   ProcessNewBlock (full ConnectTip path: CoinsTip + mempool
   update + removeForBlock internals), checks green; 5 fresh-root
   re-adds green.
5. PrioritiseTransaction churn across all 209 entries (fee-diagram
   mutation): check green.

### Tripwire controls (the falsification machinery provably fires)
- v3: txgraph.cpp:571 Assume(false) in CompareMainTransactions
  FIRED — backtrace (gdb): TryAddToMempool -> ChangeSet::Apply ->
  CommitStaging -> MoveToMain -> ChunkOrder insert. Mechanism: my
  coinbase cycle wrapped at 60 while ~200 roots were inserted,
  creating SAME-TXID duplicates in two singleton clusters — equal
  feerate+prefix+fallback hits the 'strong ordering unreachable'
  branch. This is the graph's 'one tx, one ref' precondition
  (txgraph.h:247) enforced exactly as designed; production ATMP
  rejects duplicate txids before insertion. Harness bug, correctly
  distinguished; fixed with unique roots (300 mined, 280-cycle).
- v4: txmempool.cpp:590 check() 'mempoolDuplicate.HaveCoin' FIRED —
  my v4 removed txs 'as if mined' without the ConnectBlock half
  (no UTXOs added to the coins view), leaving children with
  unresolvable inputs. Also harness-side; fixed with the real
  ProcessNewBlock path.
- v2: premature-coinbase rejection (fixture never mined past 100)
  — third harness-side lesson.

### Verdict
DISMISSED: ref-graph invariants, sequence uniqueness, locator
presence, fee-diagram bookkeeping, and cluster-limit enforcement
all hold under dynamic cluster stress with assertion trapping;
both induced violations were harness-contract bugs caught by the
very tripwires under test (which is the strongest available
evidence the tripwires work).

### Exact commands
- build/run as above; gdb -batch -ex run -ex 'bt 25' for the
  tripwire backtrace (recorded above).

### Limitations / queue
- The driver exercises the fixture's default limits (64-cluster,
  101 kvB cluster size); the DEBUG_TEST -limitcluster* variants
  (F13's config surface) are separate.
- Persistent harness at /tmp/cluster_stress.cpp; promotion to a
  unit test optional (mempool_tests covers check(); the unique
  part is the trapping-compile recipe + cluster-stress shapes).
- Upstream-side assertion sweep remains a separate large cell.

## Rotation note
Three cycles; fork Assumes, config validation (F13), and dynamic
cluster stress all closed.
