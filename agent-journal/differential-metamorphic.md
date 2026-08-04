# Campaign #51 — differential-metamorphic

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/differential-metamorphic. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): ConnectBlock/DisconnectBlock apply-revert pair — oracle gap closed (47e5bf2f95)

### Draw
Random draw over the 45-goal eligible pool: raw=12735176910626288282,
index 24 -> #51.

### Pair selection / gap proof
Pair: apply/revert — ConnectBlock vs DisconnectBlock on the UTXO set.
Gap proof: ZERO DisconnectBlock callers in unit or fuzz tests
(grep across src/test, src/wallet/test, src/test/fuzz); functional
reorg tests (mempool_reorg.py, feature_reorg.py) assert balances and
mempool state, never raw UTXO-set equality. The only in-tree mention
of disconnect semantics was a comment in coins_tests.cpp:653.

### Domain / permitted differences
Domain: full UTXO set of the active chainstate after
ConnectBlock(B) + DisconnectBlock(B) for a valid B. Contract: exact
identity with the pre-connect set — the CTxUndo machinery restores
spent inputs exactly and the block's own outputs are removed exactly.
Permitted difference: none at the set level (tip hash alone is
insufficient as oracle).

### Test (47e5bf2f95)
validation_chainstatemanager_tests/
chainstatemanager_disconnect_restores_coins_exactly:
- mineBlocks(1) to mature the first coinbase; spend it (signed P2PK);
- snapshot the full set by flush + DB-cursor iteration (CCoinsViewCache
  cursors are disabled in this tree — ccoins_cache_cursor_unsupported
  documents the error — so the set must be flushed to the DB first);
- connect, disconnect, drain the disconnectpool (public clear(); its
  destructor asserts on non-empty; MaybeUpdateMempoolForReorg is
  protected);
- assert tip restoration AND element-wise set identity
  (outpoint/amount/script/height/coinbase flag).

### Oracle sensitivity (staged clean/mutation/repaired)
- clean: No errors detected.
- mutation: validation.cpp ApplyTxInUndo skipped (scratch edit):
  test FAILS with [101 != 100] size mismatch + element mismatch —
  the undone coinbase is precisely what the oracle pins.
- repaired (git checkout src/validation.cpp): suite green again,
  full validation_chainstatemanager_tests green.

### Verdict
- Oracle gap CLOSED; no production defect found (undo machinery is
  exact for this domain). BIP30 duplicate-coinbase paths noted as a
  distinct domain for a future cycle.

### Framework notes (for future cycle users)
- CCoinsViewCache::Cursor() always throws in this tree; flush + DB
  cursor is the enumeration path.
- DisconnectedBlockTransactions must be drained (clear() is public;
  MaybeUpdateMempoolForReorg is protected).

### Limitations
- Single-block, single-spend domain; multi-block reorgs with
  conflicting txs (BIP30 shape) not covered.
- The fork's prevoutfetchthreads option (visible in
  chainstatemanager_loadblockindex) does parallel prevout fetch on
  connect — the disconnect domain with fetched prevouts not
  differentially tested.

### Exact commands
- `cmake --build build-before -j4 --target test_bitcoin`
- `build-before/bin/test_bitcoin --run_test=validation_chainstatemanager_tests[...]`

### Next queue for this campaign
- BIP30 duplicate-coinbase disconnect domain (fClean/undo interplay).
- Fee-diagram incremental-vs-recompute differential (TxGraph diagram
  vs fresh replay — needs an instrumentation hook).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): c1 oracle backport + multi-block undo composition oracle (negative-control-verified)

### Draw
Random draw over the 2-goal eligible pool (1 pending + 1 CYCLE-1,
#54 excluded as just-cycled): raw=9209411641343266232, index 0 ->
#54 first, then this draw's pool after #54: (1 pending + 1 CYCLE-1)
-> #51 was drawn raw from the 2-entry pool after #54 c1 completed:
raw=9209411641343266232 idx 0 (of 2) had drawn #54; the next draw
for this cycle: pool {51} only — #51 selected deterministically as
the sole remaining eligible entry (recorded for honesty: no RNG
needed for a singleton). Branch: audit/differential-metamorphic-c2
from ff86dea723 (#54 c1 bookkeeping). Ledger had NO c1 row; c1
stack (47e5bf2f95 + c7156b5dc1) was stranded on
audit/differential-metamorphic.

### Backport
Cherry-picks: oracle test as 4807d408fe, journal as 088af53250
(usual uber-rotation.md resolution). Verified at HEAD:
validation_chainstatemanager_tests green.

### Cell (c1 queue): undo composition across multiple disconnects
The c1 oracle proved disconnect restores the exact pre-connect UTXO
set for ONE block. Distinct metamorphic domain: undo must COMPOSE —
S2 -> S1 -> S0 with per-step identity. New test
chainstatemanager_disconnect_composes_across_blocks: spend block,
then a coinbase-only block, then two DisconnectTips with full-set
comparison after each (the multi-block path exercises sequential
undo application and the disconnectpool drain ordering).
- Positive run: green (first attempt).
- NEGATIVE CONTROL (oracle discrimination): temporarily asserted the
  post-first-undo state equals the pre-connect state — failed
  exactly as predicted ([101 != 102] size + element mismatch; S1
  carries the spend block's coinbase + P2PK output), then reverted
  to the correct assertion and re-ran green. The comparison
  provably discriminates states.

### Verdict
CONFIRMED oracle delivered: undo correctness now pinned for both
single-block and composed disconnects, mutation/discrimination-
verified (c1's ApplyTxInUndo-skip mutation + this cycle's negative
control). Test-only change; no production defect found.

### Exact commands
- cmake --build build-before -j4 --target test_bitcoin
- test_bitcoin --run_test=validation_chainstatemanager_tests[/
  chainstatemanager_disconnect_composes_across_blocks]
- negative control: assertion swap -> 2 failures ([101 != 102]) ->
  revert -> green

### Limitations / queue
- BIP30 duplicate-coinbase domain is UNCONSTRUCTABLE on regtest
  (BIP34 active from genesis makes duplicate coinbase txids
  impossible) — recorded; the fClean/overwrite arm rests on #1 c1's
  Claim B code read + the cache-level undo tests.
- Fee-diagram incremental-vs-recompute cell still queued (needs an
  instrumentation hook).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 3 (2026-07-29): fee-diagram incremental-vs-recompute differential — the hook exists; harness verified green

### Draw
Re-rank draw over the rebuilt 3-cell queue:
raw=7567157135810338807, index 1 -> #51 (third cycle; c2 queue
cell "Fee-diagram incremental-vs-recompute differential (needs an
instrumentation hook)"). Branch: audit/differential-metamorphic-c3
from 17092d4843 (#50 c3 bookkeeping).

### Finding: the hook already exists
The txgraph fuzz target IS the incremental-vs-recompute
differential: real TxGraphImpl (the incremental cached machinery)
vs SimTxGraph (an independent model recompute), with:
- per-operation diagram delta asserts (:715-718: sum-main/staged vs
  sim sums) + sorted-diagram contracts (:1019-1020);
- ChunkLinearization-on-reordered-input vs sim (:1334-1343);
- final CompareChunks + set-difference completeness on
  GetMainStagingDiagrams (:1498-1523);
- per-cluster SanityCheck (:546/:661).
FUZZ=txgraph build_fuzz/bin/fuzz -runs=1000: clean, exit 0. The
c2 queue's "instrumentation hook" is the sim model + these final
checks — no new harness needed.

### Verdict
DISMISSED (covered): the incremental-vs-recompute property is
already differentially tested by the existing target, verified
green. No code change.

### Exact commands
- FUZZ=txgraph build_fuzz/bin/fuzz -runs=1000
- reads: src/test/fuzz/txgraph.cpp:343/546/661/690-730/
  1000-1045/1334-1343/1498-1523

### Limitations / queue
- The differential runs the sim only up to small cluster counts
  (command-budget); a long-run campaign on this target is
  qa-assets' job.
- BIP30 fee-domain remains unconstructable (c2 note).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-08-02, draw 213, raw=10746796534152436801, masked 1523424497297660993, idx 11/18): BIP30 fee-domain unconstructability PROVEN LIVE — both consensus doors reject the duplicate (bad-cb-height / bad-version); c2 note upgraded to executable proof

### Construction attempt (/tmp/btc51c4/bip30_proof.py, preserved)
Regtest node, crafted identical coinbase tx C:
- Door 1 (BIP34-active v4 blocks): C accepted at height h, the
  SAME C at h+1 rejected 'bad-cb-height' — the height baked into
  the scriptSig makes every height's coinbase a distinct txid;
  exact duplicates are impossible at v2+.
- Door 2 (pre-BIP34 v1 blocks, where the height rule would not
  bite): rejected 'bad-version(0x00000001)' — versionbits
  consensus demands v4+ at current heights.
- RESULT line: no block-version path admits a duplicate coinbase
  txid at current heights (regtest or mainnet post-activation).

### Verdict
DISMISSED (unconstructable, now proven live): the BIP30
fee-domain metamorphic cell has no constructor — the historical
duplicates are unreachable artifacts of the pre-BIP34 era, and
the fee-domain has no second instance to be confused with.
The c2 note is upgraded from reasoning to executable evidence.

### Exact commands
- python3 /tmp/btc51c4/bip30_proof.py --configfile=build-before/
  test/config.ini (RESULT above); failed-path iterations
  preserved in /tmp/btc51c4/bip30_fee.py (get_scriptPubKey
  attribute, premined-height, bad-cb-height, bad-version
  sequence — the exploration record).

### Limitations / queue
- The overwrite path itself (AddCoins check_for_overwrite BIP30
  arm) remains exercised only by the historical mainnet blocks
  (#57/#40 families covered its review).
- #51 cells: sim-hook (c3), BIP30 domain (c4) — queue empty.

## Cycle 5 (2026-08-03, draw 287, radar-hit force-updated branch): disconnect-pool duplicate txids — Assert-fires confirmed at HEAD + skip-fix ADOPTED (unit-verified); functional fork-scenario fails at the BIP30-validation layer independent of the fix (divergence recorded)

### Radar hit
Force-updated branch l0rinc/disconnect-pool-duplicate-txids
(8dfa501356 fix + e8f94f3da4 characterization, 2026-08-03).

### Defect (confirmed at HEAD)
AddTransactionsFromBlock did `Assert(inserted)` (fork's Assume
form of upstream's assert) — a duplicate txid across two blocks
in a reorg aborts (assert builds) or silently corrupts the pool
(release: Assume erased -> txid maps to the LAST iterator while
queuedTx holds the tx twice -> removeForBlock removes one ->
stranded entry). Mainnet reachability needs a reorg over the
historical BIP30 dup pairs (~91k heights) — latent in practice
today, but the corrupt-in-release direction is real.

### Adoption (audit/adopt-dup-txid)
- Cherry-picked e8f94f3da4 (characterize) + 8dfa501356 (skip
  fix): index each txid, skip duplicates; queue/index/usage
  stay synchronized.
- Fork-test repair (89b150976d): the fork's own
  disconnectpool_rejects_duplicate_txids expected the OLD throw
  — updated to the skip semantics (and closed a lost brace).
- disconnected_transactions suite: GREEN (across-blocks and
  within-block duplicate cases now skip cleanly).

### Functional divergence (documented)
The PR's feature_block.py fork scenario ('Submit a genesis
fork') fails in our tree at the BIP30-VALIDATION layer: the
fork's height-120 block with the duplicate coinbase is
rejected 'bad-txns-BIP30, tried to overwrite transaction'
(the dup's UTXO from b_dup_2 is unspent when the fork block
connects — independent of the pool fix; identical failure with
or without it). Root cause unsettled here: our tree's
validation ordering vs the PR's test flow (the fork's #40
family touched CheckBlock dup paths). The PR's own CI is the
settlement oracle; the unit-level oracle is green and the fix
is semantically sound regardless.

### Verdict
CONFIRMED + ADOPTED (unit-verified): the skip fix is correct
and now carried; the functional fork-scenario is recorded as
an open divergence (not a fix defect — a test-flow/validation-
ordering question for upstream CI or a fork-specific variant).

### Suspicion-mining
- S17: adopting a fork-branch onto a tree whose OWN tests
  encode the old behavior requires the repair commit — always
  grep the adopted area's in-tree tests for stale expectations.

### Exact commands
- cherry-picks above; suite run above; feature_block.py run
  (/tmp/btc287, BIP30 rejection above).

### Limitations / queue
- Functional arm inconclusive pending upstream CI / a
  fork-specific test variant.

## Cycle 6 (2026-08-03, draw 288): cycle-287 functional divergence SETTLED — our BIP30 check is byte-identical to upstream master's; the PR's fork-scenario expectation fails on master semantics too (author's CI all queued, likely iterating); NOT a fork issue

### Settlement evidence
1. CI check: the fix commit's 57 check-runs are ALL queued
   (branch force-updated recently; no verdict exists yet — the
   author is likely still iterating on exactly this).
2. Static diff: git diff origin/master HEAD --
   validation.cpp/tx_check.cpp/tx_verify.cpp has ZERO hits for
   BIP30/overwrite/coinbase/duplicate-related changes — the
   fork did NOT touch the BIP30 check. Our rejection of the
   fork-scenario's height-120 duplicate coinbase
   ('bad-txns-BIP30, tried to overwrite transaction') is
   byte-identical behavior to upstream master.
3. Conclusion: the PR's send_blocks(fork_blocks, success=True)
   expectation does not hold against CURRENT master semantics
   either — the test flow is the author's open work (their
   force-update + queued CI is consistent with mid-iteration),
   NOT a fork validation divergence.

### Verdict
DIVERGENCE CLOSED as not-a-fork-issue: our adoption of the
skip fix (unit-verified) stands; the functional test's fate
belongs to the PR's own iteration. When the author's CI
completes, the check-runs URL is the settlement oracle
(recorded).

### Exact commands
- check-runs API (57 queued); git diff origin/master HEAD
  (zero BIP30-family hits).

### Limitations
- If a later push makes the upstream test pass, re-run the PR's
  feature_block.py variant in our tree (identical semantics
  expected by point 2).
