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
