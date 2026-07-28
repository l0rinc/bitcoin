# Cycle 18: error-path partial-state mutation audit

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
- Next queue: commit the source/test finding and close this cycle, then run a fresh gate and draw another distinct catalog goal. Do not reopen the cycle-39 address-book publication fix without new backend or restart evidence.
