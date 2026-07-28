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

