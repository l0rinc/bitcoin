## Cycle 214: failed wallet transaction load leaves a partial map entry

### Selection and fresh gate

- Exact selector: `shuf -i 0-98 -n 1` -> `27` (`error-path-state`); no reroll.
- Branch: `uber-cycle-214-error-path-state-20260731`.
- Cycle start HEAD: `d5d3c458f064941c84e1d1d71691f58842c6aef0`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1217 42`
  (`HEAD...origin/master`).
- The fresh gate fetched `origin/master`, preserved the known unrelated untracked
  artifacts, and found no tracked or staged changes before this cycle's edit.
  `git diff --check` passed. Protected wallet, util, script, and RPC test
  processes with PIDs `777094`, `956381`, `1138182`, and `1157959` remained
  alive and untouched.
- Catalog, prompt, corrected TSV, and protocol hashes remained respectively
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

### Distinct scope and contract

Goal 27 had already closed passphrase, address-book, spent-marker, descriptor
import, temporary-wallet, transaction/index, and persistent coin-lock cells.
This cycle mined recent wallet changes and audited `CWallet::LoadToWallet`, the
internal loader used by `LoadTxRecords` and watch-only/migration copy paths.

`LoadToWallet` inserts a placeholder under the database key before invoking its
callback. The callback owns filling the `CWalletTx` and returns `false` when the
record cannot be accepted. The contract is that a failed load must not publish a
new wallet transaction: neither `mapWallet` nor its ordered/spend indexes should
retain the rejected object. In the production database path,
`LoadTxRecords` deserializes a record and returns `false` when the transaction's
embedded hash differs from the serialized `DBKeys::TX` key. This is a reachable
persisted-corruption boundary, not a malformed callback invented only for the
test.

### Confirmed finding

`CWallet::LoadToWallet` emplaced a placeholder before calling `fill_wtx`, but on
callback failure returned without erasing a newly inserted map entry. The
production hash-mismatch path therefore left a `CWalletTx` keyed by the database
hash while its `tx` held the different embedded transaction. The function did
not yet add ordered or spend indexes, so the map entry was an inconsistent
caller-visible partial state that could affect later lookups and duplicate-load
classification during the same wallet load.

### Independent reproduction

The focused regression constructs a transaction whose hash differs from a
deterministic database key, emulates the `LoadTxRecords` callback by assigning
the transaction to the inserted `CWalletTx`, and returns `false`. Against the
unmodified code:

```text
TMPDIR=/data/my_storage/tmp /data/my_storage/tmp/cycle214-build/bin/test_bitcoin \
  --run_test=wallet_tests/load_to_wallet_failure_does_not_retain_transaction \
  --log_level=test_suite --report_level=short --color_output=false
```

exited `201` with `check !m_wallet.mapWallet.contains(database_hash) has failed`
(`3` assertions passed, `1` failed). The test is a direct state oracle for the
production helper and uses no timing, malformed process input, or default
wallet/datadir.

### Fix

After a failed fill callback, `LoadToWallet` now erases the entry only when the
call inserted it. Existing entries retain their established duplicate-load
behavior, while a rejected new record cannot remain in `mapWallet`. The ordered
and spend indexes are still untouched until the callback succeeds, so erasing at
this boundary restores the complete pre-call state with the smallest possible
change.

### Verification

- A separate wallet-enabled current-source build was configured at
  `/data/my_storage/tmp/cycle214-build` and rebuilt with
  `TMPDIR=/data/my_storage/tmp CCACHE_DIR=/data/my_storage/tmp/cycle214-ccache
  ninja -C /data/my_storage/tmp/cycle214-build test_bitcoin -j2`; all 547 build
  steps completed and linked `bin/test_bitcoin`.
- The repaired focused regression passed `1` case and `4` assertions.
- The repaired `wallet_tests` suite passed `26` cases and `220` assertions.
- `walletload_tests` passed `1` case and `6` assertions; `walletdb_tests` passed
  `2` cases and `5` assertions.
- `git diff --check` passed after the source, test, and journal edits.

The regression directly exercises the callback contract and the fix is backend-
independent. It does not create a full corrupt SQLite record through
`PopulateWalletFromDB`, exercise Berkeley DB, or test a power-loss boundary; the
production caller and exact hash-rejection branch were verified by source and
call-graph inspection. Existing callbacks that reject an already-present entry
return before mutating it, so no broader snapshot/copy refactor is justified by
this cycle.

### Verdict and handoff

- Confirmed and fixed: failed wallet transaction loads could retain a new map
  entry under the wrong hash, exposing partial persisted-corruption state after
  `LoadToWallet` returned failure.
- Source/test/journal are to be committed as one self-contained finding commit,
  authored as `Lőrinc <pap.lorinc@gmail.com>`. A separate state-only close commit
  must record its final hash, exact test results, limitations, and next queue.
- Do not reopen earlier Goal 27 cells without new backend/restart evidence. The
  next cycle must perform a fresh gate and exact random draw; queued Goal 27
  cells include descriptor top-up failure after prior cache writes, BaseIndex
  lifecycle state, and other newly changed status-returning wallet APIs.

## Cycle 192: persistent coin-lock failure-state audit

### Selection and fresh gate

- Exact selector: `shuf -i 0-98 -n 1` -> `27` (`error-path-state`). The prior Goal 27 cells closed passphrase changes, address-book publication, spent markers, `setlabel`, temporary-wallet cleanup, transaction/index transitions, and migration-specific records; this cycle selects the separate coin-lock state machine.
- Selected goal: `error-path-state` (error-path partial-state mutation audit).
- Branch: `uber-cycle-192-error-path-state-20260731`.
- Cycle start HEAD: `5d433662ba95eb432f013d0af05b3ef35327cd4f`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1174 42` (`HEAD...origin/master`).
- Fresh gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; known unrelated untracked artifacts were preserved. PIDs `777094` and `956381` were alive and untouched.
- Gate hashes: catalog `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, prompt `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, corrected TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, state at gate `3f92dc69a870324fa785548eee4b8fce538348bfadf757b959c346440a701270`.

### Distinct scope and contract

The public `lockunspent` contract says persistent locks are written to the wallet database, unlock removes both forms, and the RPC implementation labels the multi-output update as atomic. The lower-level `CWallet` methods return status after database operations and therefore must not leave runtime `m_locked_coins` and durable `LOCKED_UTXO` records disagreeing on failure.

The hypothesis is that a write/erase failure or a later failure in a multi-output request leaves a false runtime lock, a stale durable lock, or a partial batch despite the returned failure. The trust boundary is the public RPC and wallet interface, `m_locked_coins`, serialized `DBKeys::LOCKED_UTXO` records, SQLite/legacy wallet backends, and subsequent restart/coin-selection observations. Existing functional lock tests cover successful and ordinary validation paths, not injected database failures or a failure after an earlier output in the same request.

### Investigation plan

1. Trace `LockCoin`, `UnlockCoin`, `UnlockAllCoins`, `lockunspent`, database key serialization, callers, and restart loading contracts.
2. Inject deterministic insert/delete failures for exact `LOCKED_UTXO` rows and record complete pre/post memory and database state for one and multiple outputs.
3. Verify the failure oracle against the public RPC and direct wallet methods, including persistent-to-temporary transitions, all-coins unlock, and a second-output failure.
4. If confirmed, make the smallest state-first/rollback-preserving change, add focused tests, run wallet and RPC validation, and check the diff against the documented atomicity contract.

### Cycle 192 findings and fix

The audit confirmed four related defects in the persistent coin-lock state machine:

- `CWallet::LockCoin` called `LoadLockedCoin` before `WriteLockedUTXO`, so an injected insert failure returned false while `m_locked_coins` still contained the output.
- `CWallet::UnlockCoin` erased the runtime entry before `EraseLockedUTXO`, so an injected delete failure returned false while the durable row remained.
- `CWallet::UnlockAllCoins` used `success = success && batch.EraseLockedUTXO(coin)`, which short-circuited after the first failed row, then cleared every runtime entry regardless of database results.
- `LoadLockedCoin` used `emplace`; promoting an already-temporary lock with `LockCoin(output, true)` wrote a durable row but left its map value false, so a later unlock did not erase that row.

The RPC loop also claimed atomicity in its source comment while invoking one-operation batches through the lower-level methods. A failure on the second output could therefore leave the first output locked. The existing `WalletBatch` commit-listener pattern used by nearby wallet state publication provides the contract needed here: write or erase first, publish runtime state only after commit, and keep failed rows unchanged. The RPC now uses one explicit transaction and the wallet exposes batch-aware lock/unlock helpers. `UnlockAllCoins` copies the keys, attempts every row without short-circuiting, and removes only successful entries. Promotion assigns the persistence bit instead of using `emplace`.

### Independent pre-fix evidence

Using `/data/my_storage/tmp/cycle84-build/bin/test_bitcoin` against the unmodified implementation and deterministic SQLite `BEFORE INSERT`/`BEFORE DELETE` triggers:

- `wallet_tests/locked_coin_write_failure_preserves_state`, seed `19201`, exited 201: `check !wallet.IsLockedCoin(coin) has failed`.
- `wallet_tests/locked_coin_erase_failure_preserves_state`, seed `19202`, exited 201: `check wallet.IsLockedCoin(coin) has failed`.
- `wallet_tests/unlock_all_coins_failure_preserves_state`, seed `19203`, initially had a test-fixture parse error; after correcting the fixture to `uint256{2}`, it exited 201 with the failed coin cleared and the later row still present.
- `wallet_tests/lock_coin_promotion_clears_persistent_record`, seed `19204`, exited 201: the persistent row remained after promotion followed by unlock.

The exact row keys were serialized through `DBKeys::LOCKED_UTXO`; the failures were deterministic and did not depend on timing, malformed RPC data, or a live wallet directory.

### Fix and verification

- Added batch-aware `LockCoin` and `UnlockCoin` methods. Database mutation precedes runtime mutation; active transactions register commit callbacks with explicit no-op abort callbacks.
- Changed direct wrappers to use the batch-aware methods, made persistent promotion overwrite the map value, and made `UnlockAllCoins` preserve failed rows while continuing with later rows.
- Wrapped explicit-output `lockunspent` updates in one `WalletBatch` transaction. Abort paths roll back database changes and commit callbacks prevent partial runtime publication.
- Added four direct wallet regressions and one two-output RPC regression. The RPC regression injects failure on the second persistent insert and checks both memory and both database rows.
- The first post-fix RPC run exposed an empty abort `std::function` (`bad_function_call`); explicit no-op abort lambdas fixed that path before final validation.
- `git diff --check`: passed.
- `ninja -C /data/my_storage/tmp/cycle84-build test_bitcoin -j2`: passed after the final source/test changes.
- `TMPDIR=/data/my_storage/tmp/cycle192-wallet-tests-after-full /data/my_storage/tmp/cycle84-build/bin/test_bitcoin --run_test=wallet_tests --log_level=message --report_level=short --random=19208`: passed, 25 cases and 216 assertions.
- `TMPDIR=/data/my_storage/tmp/cycle192-wallet-tests-after-rpc2 /data/my_storage/tmp/cycle84-build/bin/test_bitcoin --run_test=wallet_tests/lockunspent_write_failure_is_atomic --log_level=message --report_level=short --random=19207`: passed, 1 case and 11 assertions.
- `test/functional/wallet_basic.py --configfile=/data/my_storage/tmp/cycle192-functional-config.ini --tmpdir=/data/my_storage/tmp/cycle192-functional-wallet --cachedir=/data/my_storage/tmp/cycle192-functional-cache --randomseed=19209 --loglevel=INFO`: passed, including mining, sendmany, reindex, wallet-broadcast, and descriptor checks.

The focused fault hooks use the mock SQLite backend; Berkeley DB and an externally interrupted on-disk transaction were not exercised. The existing restart persistence path passed through `wallet_basic.py`; the new promotion and injected failure contracts are covered directly in the wallet suite. No unrelated tracked changes were staged or reverted.

### Verdict and handoff

- Confirmed and fixed: coin-lock API failures could desynchronize runtime and durable state, bulk unlock could discard failed locks and skip later work, promotion could strand durable rows, and multi-output `lockunspent` was not atomic despite its contract comment.
- Prior Goal 27 cells for passphrases, address-book publication, transaction/index transitions, spent markers, `setlabel`, temporary-wallet cleanup, and migration remain closed and were not reopened.
- Commit the source, tests, and this journal as one self-contained finding commit. Then create the separate state-only close commit with the exact final hashes, checks, and next queue. The next cycle must use a fresh gate and a new random draw.

## Cycle 18: error-path partial-state mutation audit
## Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `27`
- Selected slug: `error-path-state`
- Timestamp: `2026-07-28T02:04:29Z`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD before this cycle: `ffda33a38f5fddab57e4618775d22ce31d8eda09`
- Worktree: source was clean at the cycle gate; existing agent artifacts and `test/cache/` were preserved.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Source/test/journal commit: `55eaf087c189ae871878692fb20a90ac3533084d`

The cycle gate refreshed `origin/master`, confirmed the branch and dirty state, verified that no daemon, fuzz, sanitizer, benchmark, or test process was running, and searched the catalog, prior journals, history, tests, and review evidence before selecting a new wallet failure surface.

## Scope and prior evidence

The campaign compares complete pre/post state for APIs that return failure after changing wallet objects, caches, database rows, files, counters, or caller-visible outputs. Existing history already covered many descriptor, address reservation, transaction output, signing, parsing, and encryption rollback paths, including:

- `b14660d64e` clearing failed `FindTx` outputs;
- `6e67919fa6`, `8b9e10c544`, `21f215670b`, and related descriptor/database atomicity fixes;
- `c355695b00` preserving address reservation state on write failure;
- `4691fb15f0`, `bb1070b55b`, `1bcf9f86dd`, and `4691d1c562` clearing failed output objects;
- the existing `encrypt_wallet_master_key_write_failure_preserves_state` regression in `scriptpubkeyman_tests.cpp`.

`BlockManager::ReadBlock` was also checked as a possible partial-output candidate. Its tests intentionally preserve the parsed block version when a later proof-of-work validation fails, so that behavior is an established contract and was dismissed for this cycle.

## Confirmed finding

`CWallet::ChangeWalletPassphrase` decrypted the old master key, unlocked the wallet, encrypted the new passphrase directly into the `CMasterKey` stored in `mapMasterKeys`, and then ignored the return value of `WalletBatch::WriteMasterKey`. A database failure therefore returned success while leaving the in-memory passphrase changed and the persisted passphrase unchanged. When the wallet started locked, the old code also only restored the lock after the unchecked write, so later failure paths could expose a different lock state.

The failure is reachable through the public wallet passphrase-change path. It does not require malformed data or unsupported internal state.

## Independent reproduction

The regression creates a descriptor wallet, encrypts it with `old passphrase`, serializes the exact `DBKeys::MASTER_KEY` row key, and installs an SQLite `BEFORE INSERT` trigger that raises `ABORT` only for that row. This exercises the production `INSERT OR REPLACE` operation used by `WriteMasterKey`.

With the old implementation, after the injected write failure the test reported:

```text
check !keystore.ChangeWalletPassphrase(old_passphrase, new_passphrase) has failed
check keystore.Unlock(old_passphrase) has failed
check !keystore.Unlock(new_passphrase) has failed
exit 201
```

The old implementation consequently returned true, rejected the old passphrase, and accepted the new passphrase despite the database write being aborted. This is a deterministic failing-before trace, not a timing-dependent database race.

## Fix

`ChangeWalletPassphrase` now:

1. copies the stored `CMasterKey` into a temporary value;
2. encrypts the new passphrase into the temporary value;
3. checks `WriteMasterKey` and returns false on failure;
4. publishes the temporary value to `mapMasterKeys` only after the write succeeds; and
5. restores the original locked state on encryption or database-write failure.

The successful path preserves the previous behavior for wallets that were already unlocked and relocks wallets that were locked before the operation.

## Verification

- `git diff --check`: passed.
- `cmake --build build_unit_wallet_clang19 -j2 --target test_bitcoin`: passed after the fix.
- Before/after control: the new regression failed against the original implementation with exit 201 and the three failures above.
- `./build_unit_wallet_clang19/bin/test_bitcoin --run_test=scriptpubkeyman_tests/change_wallet_passphrase_write_failure_preserves_state --log_level=message`: passed after the fix with no errors.
- `./build_unit_wallet_clang19/bin/test_bitcoin --run_test=scriptpubkeyman_tests --log_level=test_suite`: passed all 18 cases with no errors after the fix.

The test uses in-memory SQLite and one descriptor manager. It does not exercise Berkeley DB, an on-disk restart, or a failure inside key-derivation/encryption itself. Those are limitations of the deterministic local fault hook, not reasons to weaken the rollback contract.

## Why existing tests missed it

Existing tests injected failures into initial wallet encryption and descriptor key writes, but there was no test for a failed master-key overwrite during `ChangeWalletPassphrase`. The unchecked return value had survived both the refactoring that introduced `EncryptMasterKey` and the current wallet failure-path suite.

## Verdict and next queue

- Confirmed and fixed: passphrase changes could report success and desynchronize in-memory and persisted master-key state after a database write failure.
- Dismissed for this cycle: the intentionally partial `ReadBlock` output contract and the previously covered descriptor/address/encryption rollback paths.
- Next queue: draw a distinct catalog goal after recording the source/test commit and this handoff; do not reopen passphrase-change failure handling unless a new backend, restart, or encryption-failure witness appears.

## Cycle 22: transaction-download and index failure-state audit

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `27`
- Selected slug: `error-path-state`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- HEAD at gate: `d2c2ef6a4ccf03a50a22917d100b70a3294cdb93`
- `origin/master` at gate: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Source was clean at the gate; existing agent artifacts and `test/cache/` were preserved.
- `git diff --check`: passed.
- No daemon, test, fuzz, sanitizer, or benchmark process was running at the gate.

This was an intentional repeat of goal 27 on a different evidence cell. The cycle did not reopen the wallet passphrase path fixed in cycle 18. The catalog and prior journals were searched before selecting transaction-download and index transitions.

### Candidate ledger

| Surface | Hypothesis | Evidence | Verdict |
|---|---|---|---|
| `TxDownloadManager` response, rejection, orphan, and disconnect paths | A normal failure could leave request, orphan, peer, or accounting state mutated | Existing contracts and postconditions cover unknown peers, response cleanup, request issuance, orphan rejection, disconnect cleanup, and duplicate peer connection. `txdownload_tests` passed all 14 cases, including missing-input, rejection, and package-retry cases. | Dismissed; no new partial-state defect |
| `CoinStatsIndex::CustomAppend` | `m_total_subsidy` is incremented before a previous-block-hash mismatch returns false | The mismatch is dispatched only through `BaseIndex::ProcessBlock`, which treats failure as a fatal index error. `BlockConnected` and `Sync` provide contiguous chain transitions, while `CustomInit` rejects a corrupt persisted tip before append. No supported production transition reached the mismatch. | Dismissed as a reachable production defect; retain as an internal-contract hardening lead |
| `BlockFilterIndex` append/remove persistence | A write or filter-header failure could expose a recoverable partial index state | The suspicious writes can precede later failure, but the failure path is terminal node shutdown and no externally observable restart inconsistency was reproduced in this cycle. | Dismissed for this cycle; revisit only with a fault-schedule witness |

The transaction-download state machine already has direct unit and fuzz coverage from earlier cycles. `GetNodeStateStats` can return false after filling an output if its peer disconnects between state lookups, but its RPC caller explicitly ignores the failure and the current test suite did not establish a caller-visible contract violation. It remains a lower-priority future queue item rather than a finding for this cycle.

### Verification

- `build_unit_clang19/bin/test_bitcoin --run_test=txdownload_tests --catch_system_error=no --log_level=test_suite`: all 14 cases passed; `*** No errors detected`.
- `build_unit_clang19/bin/test_bitcoin --run_test=coinstatsindex_tests,baseindex_tests --catch_system_error=no --log_level=test_suite`: all 3 cases passed; `*** No errors detected`.
- Normal fuzz replay, isolated under `/data/my_storage/tmp/cycle22-fuzz-work`, used 128 deterministic seeds selected from `qa-assets` and `-jobs=1 -workers=1 -runs=1`. It completed 129 runs in 68 seconds, exit code 0, 8,203 coverage edges, peak RSS 109 MB, and no artifact.
- ASan/UBSan fuzz replay, isolated under `/data/my_storage/tmp/cycle22-fuzz-asan`, used the identical 128 seeds and the same fixed run count. It completed 129 runs in 309 seconds, exit code 0, 26,028 coverage edges, peak RSS 693 MB, and no crash artifact. Four `slow-unit-*` files were emitted by libFuzzer's slow-unit reporting; they are not failure inputs.

The earlier full-corpus attempt used two parallel workers in the repository checkout, which caused `fuzz-0.log` and `fuzz-1.log` path collisions. It was stopped after the configured time budgets because seed processing dominated. Ctrl-C produced an ASan `DEADLYSIGNAL` stack rooted in `fuzzer::Fuzzer::InterruptExitCode()`; this is an interrupted libFuzzer driver trace, not a production crash. The clean isolated subset above is the authoritative fuzz evidence for this cycle.

### Handoff

No source change is justified. Cycle 22 is a journal-only dismissal: transaction-download failure edges remain covered, and the CoinStats mutation is behind an internal invariant violation that currently terminates index processing rather than returning a recoverable failure to a caller. The next cycle must run a fresh gate and draw another catalog goal, avoiding wallet passphrase handling, the immediate direct-fetch boundary cell, and already-closed generic P2P lifecycle surfaces unless new evidence changes their priority.

## Cycle 39: address-book write failure state and transaction symmetry

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `27`
- Selected slug: `error-path-state`
- Timestamp: `2026-07-28T02:04:29Z`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD before this cycle: `0d2d85e181dbf9a37d4c7c5b603b02327b829ca0`
- `origin/master...HEAD` after refresh: `2 845`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`

The cycle gate refreshed `origin/master`, confirmed the branch and tracked/staged state, checked that no relevant process was running, and searched the catalog, prior error-path cells, history, callers, tests, and review evidence. The earlier goal-27 cells covered wallet passphrase persistence and transaction-download/index transitions. This cycle selected the distinct address-book write path.

### Candidate and contract

Before the fix, `CWallet::SetAddressBookWithDB` changed `m_address_book` before calling `WalletBatch::WritePurpose` and `WriteName`. A failed write returned false but left the new label and purpose in memory. The ordinary `SetAddressBook` wrapper also created a batch without `TxnBegin`, so a successful purpose write could remain persisted when the later name write failed. The public `setlabel` RPC still ignores the bool return; that is a separate queued error-propagation candidate because this cycle's regression exercises the wallet API directly.

The expected contract is all-or-nothing for the address-book operation: on either write failure, the complete prior in-memory record and all persisted address-book rows remain unchanged, with no change notification. On success, publication and notification occur after the database commit.

### Independent reproduction

The first regression version installed an SQLite `BEFORE INSERT` trigger for the exact `DBKeys::NAME` row, called `SetAddressBook` for an existing address, and checked the old label in memory and the database. Against the original implementation, the command

```text
build_unit_wallet_clang19/bin/test_bitcoin --run_test=scriptpubkeyman_tests/set_address_book_write_failure_preserves_state --log_level=all
```

exited 201. The write returned false and the database check retained `old label`, but the in-memory assertion failed with `[new label != old label]`. This is a deterministic SQLite fault, not a race or malformed-input artifact.

The final regression covers both failure edges. It supplies an address purpose, injects a name-row abort after the purpose write, and then injects a purpose-row abort before the name write. Each path asserts the old label, old database name, and absence of the new purpose. The purpose-plus-name case also proves that the ordinary wrapper must use one transaction.

### Fix

`SetAddressBookWithDB` now stages a copy of the address-book record, performs both writes while holding the wallet lock, and publishes the staged record only after both writes succeed. It registers a transaction listener so publication and `NotifyAddressBookChanged` occur only on commit. `SetAddressBook` now runs the operation through `RunWithinTxn`, making purpose and name writes atomic and ensuring an aborted transaction does not publish staged state.

### Verification

- `git diff --check`: passed.
- `cmake --build build_unit_wallet_clang19 --target test_bitcoin -j2`: passed after the fix.
- `build_unit_wallet_clang19/bin/test_bitcoin --run_test=scriptpubkeyman_tests/set_address_book_write_failure_preserves_state --log_level=message`: passed 1 case with no errors.
- `build_unit_wallet_clang19/bin/test_bitcoin --run_test=scriptpubkeyman_tests,wallet_tests,wallet_rpc_tests --log_level=message`: passed 35 cases with no errors.

The test uses the mock SQLite backend and does not exercise Berkeley DB, an on-disk restart, or a commit failure after all statements have succeeded. The `setlabel` RPC's ignored false return remains a separate confirmed code-path lead for a later cycle or follow-up commit; no RPC fault-injection harness was added here.

### Verdict and handoff

- Confirmed and fixed: address-book writes could return failure while exposing a new in-memory record, and purpose/name writes could persist asymmetrically.
- Related queued lead: `src/wallet/rpc/addresses.cpp` ignores `SetAddressBook` failure and can report RPC success after the wallet operation fails. Keep it distinct from the transaction/state fix.
- Next queue: perform a fresh gate and draw another eligible catalog goal. Do not reopen this address-book cell unless a new backend, restart, commit-failure, or RPC-level reproduction supplies independent evidence.

## Cycle 69: public address-book failure propagation

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `27`
- Selected slug: `error-path-state`
- Branch: `uber-cycle-69-error-path-state-20260728`
- HEAD at gate: `f9b2760a5fc00c588d4e4502bc026e7681d160bd`
- `origin/master` at gate: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `911 2`.
- The catalog, uber protocol, and 99-row TSV hashes matched their recorded values. Tracked and staged state was clean except for the known untracked agent artifacts; no Bitcoin Core, test, fuzz, sanitizer, or benchmark process was running.
- Earlier cycle-18 passphrase, cycle-22 transaction/index, and cycle-39 address-book state-publication cells are excluded. This cycle mines the still-open RPC caller contract from cycle 39 and then checks adjacent public wallet result paths without assuming that all ignored metadata writes are defects.

### Candidate ledger

| Surface | Hypothesis | Evidence | Verdict |
|---|---|---|---|
| `setlabel` RPC | A failed address-book database write is discarded, so the RPC reports success even though the requested label was not stored. | `SetAddressBook` returns false on a `WritePurpose`/`WriteName` failure and, after the cycle-39 fix, leaves memory and persisted state unchanged. The current RPC ignored that return. An exact SQLite `NAME`-row abort preserved `old label`; the unpatched RPC test returned success (exit 201 under the new error oracle), while the fixed path raised `RPC_WALLET_ERROR`. | Confirmed; fix in progress |
| `CWallet::GetNewDestination` / `getnewaddress` | A label write failure is also ignored after descriptor index advancement. | History describes labeling as a side effect of address generation, and the RPC maps all `util::Result` errors to keypool exhaustion. Changing this contract would require a distinct typed error and a policy for an already-persisted descriptor index; no standalone fix is justified by the current evidence. | Contract-sensitive follow-up; not changed |
| `AddToWalletIfInvolvingMe` address-book enrichment | A failure to add a restored receive address could leave transaction accounting metadata incomplete. | The write is explicitly secondary enrichment performed before the transaction write, and callers do not expose a partial transaction result. No direct user-visible invariant or restart inconsistency was reproduced in this cycle. | Deferred; best-effort boundary |
| `ImportDescriptor` label application | Ignoring a label failure may report an import error after descriptor state has already changed. | Import labels are optional metadata and the operation can add descriptors/cache state before applying them. Returning a late error would create a larger partial-import contract; current code and history provide no rollback promise. | Deferred; requires an import transaction contract |

### Verification

The source/test changes make `setlabel` derive its purpose once, check `SetAddressBook`, and raise `RPC_WALLET_ERROR` with `Failed to set address label` on failure. The focused regression uses `MockableSQLiteDatabase`, an exact serialized `DBKeys::NAME` trigger, a pre-existing label, and a direct RPC request; it also checks that the old in-memory label remains visible.

- `cmake --build build_unit_clang19 --target test_bitcoin -j2`: passed.
- `TMPDIR=/data/my_storage/tmp/error-path-state-cycle69-unit-final build_unit_clang19/bin/test_bitcoin --run_test=wallet_tests/setlabel_write_failure_is_reported --catch_system_error=no --log_level=test_suite --random=6931 -- -testdatadir=/data/my_storage/tmp/error-path-state-cycle69-testdata-final`: one case passed, `*** No errors detected`.
- `TMPDIR=/data/my_storage/tmp/error-path-state-cycle69-wallet-suite-final build_unit_clang19/bin/test_bitcoin --run_test=wallet_tests --catch_system_error=no --log_level=message --random=6932 -- -testdatadir=/data/my_storage/tmp/error-path-state-cycle69-wallet-suite-testdata`: all 17 cases passed, `*** No errors detected`.
- A source mutation restoring the discarded `SetAddressBook` return rebuilt successfully, then the same focused test with seed `6922` exited `201` because `raised` was false. Restoring the fix and rebuilding returned the test to passing. This is an independent oracle-sensitivity check.
- Wallet-enabled `build_unit_wallet_clang19/bin/bitcoind` rebuilt successfully. `wallet_labels.py` passed with `--randomseed=6927` and a fresh scratch temporary directory. The wallet-disabled functional build was skipped because `ENABLE_WALLET=OFF`.
- `git diff --check`: passed.

### Verdict and handoff

- Confirmed and fixed: a database failure in the standalone `setlabel` operation was silently converted into RPC success. The wallet layer preserved the prior record, but the caller falsely reported that the requested label had been applied.
- Deferred: `getnewaddress`, `ImportDescriptor`, and incoming-transaction address-book enrichment treat labels as secondary metadata and have different partial-state/lifecycle contracts. They require a typed error or transaction design before any propagation change; this cycle did not change them.
- Limitation: the regression uses the mock SQLite backend and does not exercise Berkeley DB or a power-loss/restart between the database failure and RPC return. The fixed path relies on the already-tested transactional `SetAddressBook` contract.
- Source/test commit: `ca5f4d5279` (`wallet: report setlabel address-book write failures`), authored as `Lőrinc <pap.lorinc@gmail.com>`.
- Next queue: close this cycle, then run a fresh gate and draw another distinct catalog goal. Do not reopen the cycle-39 address-book publication fix without new backend or restart evidence.

## Cycle 89: temporary-wallet and public failure-state audit

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `27`
- Selected slug: `error-path-state`
- Branch: `uber-cycle-89-error-path-state-20260729`
- HEAD at gate: `e005d70cebd00aa20a4d8c8ac73ad9e5720530f7`
- `origin/master` at gate: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `2 966`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The tracked worktree passed `git diff --check`; known unrelated untracked agent artifacts remain preserved and excluded from cycle commits. No relevant Bitcoin Core, test, fuzz, sanitizer, or benchmark process was running at the gate.

This cycle excludes the earlier goal-27 wallet passphrase, transaction-download/index, address-book publication, and `setlabel` RPC cells. It also excludes the already-documented descriptor PSBT invalid-signature path and the `submitSolution` template mutation: current functional coverage explicitly treats a rejected reconstructed block as inspectable state and then retries a valid solution on the same template. The new evidence queue is temporary-wallet export/reopen behavior, public wallet rescan reservation failure, and adjacent status-returning lifecycle paths introduced or changed in recent history.

### Candidate ledger

| Surface | Hypothesis | Verdict |
|---|---|---|
| `BlockTemplateImpl::submitSolution` | A rejected solution leaves a mutated template that should have been rolled back. | Dismissed: `interface_ipc_mining.py` explicitly checks that the rejected reconstructed block is visible through `getBlock()` and then retries a valid solution on the same template. |
| Temporary wallet export | A failed backup or intermediate-wallet write leaks partial state into the source wallet or leaves a failed destination behind. | Dismissed: export uses an in-memory intermediate wallet, a cleanup handler for the destination, and sets success only after the backup completes. |
| `CWallet::SetAddressPreviouslySpent` | The `used` metadata write can fail after `m_address_book` has been changed, leaving runtime and durable spent state inconsistent. | Confirmed and fixed. |

### Confirmed finding

`SetAddressPreviouslySpent` updated `m_address_book[dest].previously_spent` before calling `WalletBatch::WriteAddressPreviouslySpent`. On a failed insert, the caller continued adding the transaction while the runtime address was marked spent and the `DESTDATA/used` row was absent. The `used=false` erase path had the symmetric ordering defect: an erase failure could leave the runtime value false while the durable row remained true. The path is reachable through `CWallet::SetSpentKeyState` from `AddToWallet` when `WALLET_FLAG_AVOID_REUSE` is set.

The fix writes the database record first. It publishes the in-memory value only after a standalone write succeeds, or from an `on_commit` listener when the caller supplied an active database transaction. Abort and failed-commit callbacks do not publish state. This preserves the existing best-effort secondary-metadata contract while preventing a failed write from creating a false runtime/durable split.

### Verification

- Final build: `CCACHE_DIR=/data/my_storage/tmp/ccache-cycle89 cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j2`: passed and linked `bin/test_bitcoin`.
- Independent pre-fix control: the old source rebuilt successfully; the focused command with `--random=8901` exited `201` and reported `check !wallet.IsAddressPreviouslySpent(destination) has failed` at `wallet_tests.cpp:220`.
- Fixed focused regression: the same command and seed exited `0` with `*** No errors detected`.
- Fixed wallet suite: `--run_test=wallet_tests --catch_system_error=no --log_level=message --random=8902` ran 18 cases and exited `0` with `*** No errors detected`.
- `git diff --check`: passed after the final source/test edits.

The regression uses `MockableSQLiteDatabase`, an exact serialized `DBKeys::DESTDATA`/`used` trigger, and a query that asserts the failed marker is absent. It does not separately inject the symmetric `used=false` erase failure, Berkeley DB behavior, or a power-loss/restart boundary; the source change covers both boolean directions and active-transaction commit/abort behavior.

### Verdict and handoff

- Confirmed and fixed: failed spent-marker persistence no longer mutates the in-memory address-book record first.
- The caller still ignores the boolean because this marker is secondary wallet metadata; a future cycle may separately assess whether multi-input marker updates need an explicit transaction or fail-closed policy. That is outside this focused state-publication fix.
- Source/test/journal commit: `600afa95995f5aaa50c23b6b6c2f940dc61674bb` (`wallet: publish spent state after successful write`), authored as `Lőrinc <pap.lorinc@gmail.com>`.
- Next queue: close this cycle, run a fresh gate, and draw another distinct catalog goal. Do not reopen the earlier goal-27 wallet passphrase, transaction/index, address-book publication, or `setlabel` cells without new backend, restart, commit-failure, or caller evidence.
