# Whole-history incomplete-fix and migration mining

## Cycle 43: initial history sweep

- Goal: mine historical partial fixes, follow-ups, reverted work, and migrations for omitted analogous sites on current code.
- Repository state at draw: branch `fuzz-contract-cluster-oracles-20260709`; base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; HEAD before this cycle `8e215f0e99`; working tree had only the pre-existing untracked `agent-goals/`, `agent-journal/`, and `test/cache/` artifacts.
- Selector: `shuf -i 0-98 -n 1` -> `32`.
- Scope: upstream history for wallet migration and current `src/wallet/wallet.cpp`, `src/wallet/scriptpubkeyman.cpp`, and `test/functional/wallet_migration.py`.

### Historical evidence

- `af041c405756d3b8bb04cb2ebd8c32cf237ac2a9` added an unconditional transaction rewrite in `CWallet::ApplyMigrationData` because loading can change transaction metadata such as `nOrderPos`.
- `c98fc36d094a08d44f3c95431db2c5f34a96cc73` consolidated external wallet writes into long-lived transactions, making each write's success relevant until the final commit.
- `a277f8357ad8b0eb26f33fc36f919d868c06847b` fixed migration persistence for empty labels, establishing that address-book fields copied in memory must also be checked against their durable representation.
- `fd44d48b24b9153e76ffd9a023aafe522e815c7b` changed missing ancient best-block records from fatal to a valid empty locator and explicitly checked creation of replacement locator records.
- `0301c758ea0d0b95090d7492f1e5d30e6b447b9` and `de92208c2b508b40fa690624d026c775ed876606` fixed duplicate HD seed migration in two successive forms, showing that migration follow-ups must be checked at each analogous data transformation.

### Current candidate and proof

`WalletBatch::WriteTx`, `WriteOrderPosNext`, `WritePurpose`, `WriteName`, `WriteAddressReceiveRequest`, and `WriteAddressPreviouslySpent` all return `bool`. In `ApplyMigrationData`, the transaction rewrite added by `af041c...`, the watch-only transaction copy, the watch-only order-position record, and all address-book field writes ignored those return values. A failed write could therefore be followed by a successful transaction commit and an in-memory migration result that was not durable. The local transaction rewrite is especially destructive: legacy records are removed before the rewrite, so a swallowed failure can lose the transaction after restart.

The regression test `migration_transaction_write_failure_is_reported` creates a legacy wallet in the mock SQLite database, adds a wallet transaction, installs a key-specific SQLite abort trigger, obtains migration data, and runs `ApplyMigrationData` inside a transaction. The test requires the migration to return failure and rolls back the outer transaction. This is the smallest production-path proof for the historical rewrite omission; the same return-value propagation is applied to the analogous external transaction/order/address-book writes.

### Changes

- Propagate failures from all migration transaction and external address-book writes in `src/wallet/wallet.cpp`.
- Add the focused SQLite fault-injection regression test in `src/wallet/test/wallet_tests.cpp`.

### Verification

- `git diff --check`: passed.
- `cmake --build build_unit_clang19 --target test_bitcoin -j2`: passed after the final source/test state.
- `build_unit_clang19/bin/test_bitcoin --run_test=wallet_tests/migration_transaction_write_failure_is_reported --log_level=message`: passed (`*** No errors detected`).
- Mutation proof: temporarily restored the historical `local_wallet_batch.WriteTx(*wtx);` call without checking its return, rebuilt `test_bitcoin`, and reran the focused test. It failed at `wallet_tests/migration_transaction_write_failure_is_reported` because `!wallet.ApplyMigrationData(...)` was false. The check was restored, rebuilt, and rerun successfully.
- `build_unit_clang19/bin/test_bitcoin --run_test=wallet_tests --log_level=message`: blocked by the host environment, not this change. The run exited 201 with 11 scan-test failures after logging `Disk space is too low!`; at the time `/` had 81M free and was 100% full, while `/data` had 41G free. The focused test ran successfully under the same build.
- The functional migration test was not run because the root filesystem is full and prior functional wallet/daemon setup hit the same low-disk guard. No production datadir or wallet was used.

Verdict: confirmed. The historical transaction rewrite introduced an unchecked failure path, and the analogous watch-only transaction, order-position, and address-book writes had the same contract omission. The patch is ready for an independent source/test commit.

## Next queue

1. Review remaining migration-side writes and historical follow-ups for any unchecked persistence operation outside `ApplyMigrationData`.
2. Compare current address-book and transaction migration behavior with old `c98fc...`, `7c9076...`, and `342c45...` transaction-boundary changes.
3. Re-rank against non-wallet history cells after this cycle; do not repeat prior secret-lifetime, address-book-state, or public-validation campaigns.
