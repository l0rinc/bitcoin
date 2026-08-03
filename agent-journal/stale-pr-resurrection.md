# Journal: stale PR critical-fix resurrection audit (campaign 56)

Uber-goal rotation. Branch: audit/stale-pr-resurrection from
audit/resurrection @ f112f38d99. Seed: reviews/2026-07-25-closed-pr-revival-candidates.md
(18 closed PRs evaluated; 1 YES, 1 MAYBE).

## Verdicts

### PR 33916 (fuzz: wallet TransactionCanBeBumped target) — GAP STANDS, revival path confirmed

- Still closed-unmerged (updated 2026-07-24); no bump/RBF fuzz target
  exists in tree (grep src/test/fuzz + src/wallet/test/fuzz for
  TransactionCanBeBumped → zero hits). Fund-loss-adjacent coverage gap.
- The flagged blocker — UBSan "downcast" at package_eval.cpp:213 — is
  HARNESS-MOCK-ONLY: line 213 today is `LOCK(tx_pool.cs)` on a REAL
  CTxMemPool (constructed at 196-207), and the same harness runs that
  exact line in CI fuzz jobs daily without issue. A master-live downcast
  here would fail every CI fuzz run. The original hit came from the
  mock-mempool variant the review already said to drop.
- Revival path (unchanged from review): non-mocked mempool (as
  package_eval does), direct add-tx helpers for descendant state.
- QUEUED (not done this turn): writing the target is a new-harness
  artifact, not a minimal fix — scheduled as a follow-up cycle.

### PR 35740 (http linger-close after parse errors) — still MAYBE

Still closed-unmerged (2026-07-22). Protocol-robustness improvement, not
critical; no new review. Stands as low-priority revival for anyone
willing to own it.

### Fresh scan (closed-unmerged since 2026-07-25): nothing to revive

- 35816 "test: Add P2SH sigop counting test for segwit blocks" — closed
  by author as DUPLICATE of own open PR 35164 (P2SH sigop coverage in
  test_witness_sigops). Coverage exists via the canonical PR; no revival.
- 35815 — spam (".").

## Next queue
(write the TransactionCanBeBumped fuzz target as a dedicated cycle
(per revival path); then rotate per ledger: #96 TODO/FIXME challenge)

## Cycle 324-r5 (2026-08-03, uber draw raw=9136888865343352248 -> idx 56): new-window scan (EMPTY) + PR 33916 revival DELIVERED (target written, verify in flight)

### Reopen trigger
Last fresh scan covered closed-unmerged through ~2026-07-26; nine days
of new candidates possible. Reopen test PASSED on the window question.

### Fresh scan (closed-unmerged since 2026-07-26): EMPTY
GitHub search `is:pr is:closed is:unmerged closed:>2026-07-26` ->
total 0. Positive control: widening to >=2026-07-25 returns 6
(35816/35815 as previously assessed + 4 borderline): 35806, 35804,
35801, 35799 — all refactor/doc (structured bindings, comment
formatting, const references, typedef->using). No defect mechanism,
no revival candidates. Window stays a watch: next scan
closed:>2026-08-03.

### PR 33916 revival: wallet_transaction_can_be_bumped target WRITTEN
- src/wallet/test/fuzz/feebumper.cpp (+ CMake registration).
- Drives EVERY precondition arm of TransactionCanBeBumped:
  mapWallet presence, TxState depth (Inactive/InMempool/Confirmed/
  BlockConflicted), m_replaced_by_txid, descendant-in-wallet,
  descendant-in-mempool, foreign input (AllInputsMine false arm).
- Descendant-in-mempool arm driven through the REAL reset mempool
  (addUnchecked base + child on TestingSetup's CTxMemPool;
  node/interfaces.cpp:680 hasDescendantsInMempool -> mempool->
  HasDescendants). NO production-side virtual — the original PR's
  HasDescendants-virtual was the closure-adjacent smell and is
  unnecessary; the review's "non-mocked mempool" path taken.
- Oracle derived ONLY from fuzz choices; production predicates
  (GetTxDepthInMainChain, HasWalletSpend, pool.HasDescendants) used
  exclusively as positive-control harness Asserts. Plus a
  never-inserted-txid probe (must be false).
- Verify: build + FUZZ=wallet_transaction_can_be_bumped -runs=2000
  in flight (bash-doc6rl96); 20k campaign queued behind it.

### Harness-repair chain (3 crashes, 3 harness bugs; production NEVER implicated)
1. EMPTY input -> CheckGlobalsImpl teardown abort (check_globals.cpp:54):
   wallet code reads NodeClock; target seeded RNG but never set a mock
   clock. SAME class as txospenderindex 70f5b19656. Repair:
   SetMockTime(1231006505) after seeding.
2. e37af3a8 (in_map+wallet_desc+replaced+mempool_desc all true) ->
   descendant direct-emplaced into mapWallet does NOT update
   mapTxSpends, so the HasWalletSpend control read false against a
   true expectation. NOTE: FuzzedDataProvider consumes integrals from
   the END — first decode attempt (front-consumption) inverted every
   flag; decode against the end-model before touching logic. Repair:
   descendant via wallet.AddToWallet (the spend.cpp funding-tx idiom).
3. f89f87f2 (in_map=FALSE, wallet_desc=TRUE) -> control coupled
   expectation to base's mapWallet presence: HasWalletSpend depends
   only on a wallet-side spender of base's outputs (base need not be
   in mapWallet). Repair: control condition == wallet_desc alone.
Each crash was reproduced standalone, decoded against the consumption
model, and root-caused to the harness contract BEFORE the next run;
seeds preserved in agent-journal/artifacts/
(canbebumped-crash-seed-{empty-da39a3ee,addtowallet-e37af3a8,
mapspend-f89f87f2}). The production function agreed with the
choice-derived oracle on every crash path.
4. e33d219e (TxStateBlockConflicted arm) -> CWallet::
   GetTxDepthInMainChain ASSERTED conflicting_block_height >= 0
   (wallet.cpp:3409): the harness generated out-of-contract
   negative heights. The production contract-assert did its job —
   caught the invalid state at the earliest point, NOT a reachable
   defect (real wallet paths set actual chain heights). Repair:
   height range (0, 100'000). Seed preserved
   (canbebumped-crash-seed-blockconflict-e33d219e).
