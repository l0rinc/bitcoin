# Whole-history incomplete-fix and migration mining

## Cycle 137: paired empty-node fix follow-up

- Goal: mine a historical partial fix and its follow-up for an omitted analogous current site. The dedicated branch is `uber-cycle-137-whole-history-migration-20260730`; the cycle started at HEAD `6c2042c0898c5462402f764551cd70630f1924d7`, with `origin/master` `9611a356035be531d62bfc40879f388d5dc359c4`, merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and divergence `1059 40`. The fresh gate passed tracked/index cleanliness, `git diff --check`, catalog/protocol/goal-TSV hashes, and preservation of wallet-test PID 777094. The first exact selector draw was `57`, a closed goal; the documented reroll `shuf -i 0-98 -n 1` returned `32`.
- Historical seed: `69465de447` ignored empty `-addnode` startup values, and `90ce21e21d` added the corresponding RPC rejection because an empty node was otherwise retried indefinitely. The apparent null-mempool sibling from `a99b27f192`/`6e2962e48c` was excluded: that fix is already present on `origin/master`, while this branch is 40 commits behind it.
- The sibling hypothesis was empty `-seednode` or `-connect` values. An isolated pre-fix regtest daemon with `-seednode=` consumed the empty seed once as an `ADDR_FETCH` request (`trying v2 connection (addr-fetch) to `) and then removed it from the seed span; it did not repeat the attempt, so that path was dismissed as a separate one-shot invalid-input issue. In contrast, the pre-fix `-connect=` path retained an empty string in `m_specified_outgoing`, and `ThreadOpenConnections` retried it in its persistent manual loop. With a refused scratch proxy, the four-second run logged four `trying v2 connection (manual) to ` attempts. The source at cycle start confirmed that `connect.size() != 1 || connect[0] != "0"` copied the raw list without validation.
- Fix: while preserving the existing explicit-`-connect` behavior that disables automatic addrman connections, filter whitespace-only entries with `TrimStringView` before populating `m_specified_outgoing`, and log one warning per ignored value. Valid connect targets and the special single `-connect=0` case retain their prior handling.
- Regression: `test_empty_connect` starts with both `-connect=` and `-connect= `, requires the warning, and rejects any manual connection attempt. The rebuilt daemon post-fix logged two warnings and zero manual attempts over the same four-second scratch run. `python3 test/functional/feature_config_args.py --configfile=/data/my_storage/tmp/cycle89-build/test/config.ini --tmpdir=/data/my_storage/tmp/cycle137-functional-empty-connect --test_methods test_empty_connect --loglevel=INFO` passed. The existing `test_connect_with_seednode` compatibility method also passed, covering valid manual connections, seednode suppression, `-connect=0`, and `-noconnect` behavior.
- Build/verification: the first `cmake --build /data/my_storage/tmp/cycle89-build --target bitcoind -j2` stopped at an environment-only missing `/root/.cache/ccache/tmp`; rerunning with `CCACHE_DIR=/data/my_storage/tmp/cycle137-ccache` built `bitcoind` successfully. `git diff --check` passed. The pre/post daemon runs used only scratch regtest datadirs and a refused loopback proxy; no default datadir, wallet, or external peer was used.
- Verdict: confirmed and fixed. This is a local configuration/resource correctness issue, not a remotely triggerable network defect. It prevents an invalid empty manual target from driving an unbounded retry loop while retaining the documented `-connect` mode semantics. The source/test change is ready for an independent commit authored as `Lőrinc <pap.lorinc@gmail.com>`.

## Cycle 58: current output-contract follow-up cluster

- Goal: mine the recent history of partial output/failure fixes for analogous current sites, while excluding the cycle-43 wallet migration write-return omission and the cycle-56 `dbwrapper` decode/output-on-failure fixes already independently verified.
- Repository state at draw: branch `fuzz-contract-cluster-oracles-20260709`; base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; HEAD `75b1f55d251ea4cab3ebd827ece57eb6a8c41969`; `origin/master` `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; divergence from `origin/master` was `2 884`; tracked and staged state was clean, with only the known untracked agent artifacts. Catalog, uber-protocol, and goal TSV hashes matched the recorded values.
- Selector: exact `shuf -i 0-98 -n 1` draw `32`, `history-incomplete-fixes`. The previous wallet-migration cell, direct database decode cell, and block-filter range-output cell are out of scope for this cycle.

### Historical seed set

- `4691fb15f0` moved `ProcessNewBlock`'s `new_block` publication after block-file persistence, because a write failure left a caller-visible success flag set.
- `bb1070b55b`, `b14660d64e`, and `1bcf9f86dd` reset or preserve wallet lookup, transaction-index, and mining interface outputs across failure paths.
- `738dbb50c7`, `4699d1c562`, `8ea2383ef2`, `7962a26adf`, and `9dd598ca72` established the same contract pattern for key, script, and address diagnostics: a reused output must describe the current call, including failure.
- `bfc576a855`, `8570724e78`, `1cc215adb6`, and `b8487da6d0` extended the pattern to parser state, stream positions, cache identities, and rejected cache mutations. These are evidence seeds, not proof that every nearby function is defective.

### Cycle hypotheses and scope

1. A current block/index/database lookup still writes caller-owned output before a later fallible operation and leaves a stale or partial value on failure.
2. A public mining, parser, or diagnostic interface has a historical output contract but an analogous wrapper still fails to reset or preserve its outputs.
3. A recent fix covered only one variant of a stateful operation, leaving a sibling or alternate build/module path with a different failure contract.

For each candidate I will trace the authoritative contract through callers, tests, blame, and the originating fix; use a pre-seeded output or state snapshot; inject the earliest realistic failure; and require a failing-before/passing-after regression or a proof that no later fallible write exists. A source change is justified only for a distinct confirmed omission. Otherwise the exact tested negative control and next history cell remain in this journal.

### Confirmed finding: undo lookup published data before checksum verification

- `BlockManager::ReadBlockUndo` deserialized directly into its caller-owned `CBlockUndo`, then read and verified the trailing checksum. A corrupted undo record could therefore return `false` while replacing a previously seeded output with a partial or otherwise untrusted decoded value.
- The analogous `ReadBlock` routine and the recent `LookupFilterRange`, `FindTx`, and database output fixes establish the relevant rule: disk data is not published to the caller until all integrity checks for the operation pass. The `ReadBlockUndo` callers already treat a false result as an unusable read, so this is a local output-atomicity and corruption-handling defect, not a consensus change.
- Deterministic regression: `blockmanager_readblockundo_preserves_output_on_checksum_failure` reads a real scratch-chain undo record, flips one byte in its stored checksum, seeds the output with one `CTxUndo` and one previous-output slot, and requires the failed read to preserve both sizes. On the old implementation the test failed with `output.vtxundo.size() == before.vtxundo.size()` reported as `[0 != 1]`.
- Fix: deserialize into a local `decoded_blockundo` and move it into `blockundo` only after checksum verification. The test restores the original checksum byte before completing so the fixture remains clean.
- Verification: `git diff --check`; `cmake --build build_unit_clang19 --target test_bitcoin -j4`; focused block-manager regression passed with 11 assertions; full `blockmanager_tests` passed with 12 cases and 128 assertions. The old-source mutation was independently observed before the production edit.
- Impact and limits: this prevents stale/partial caller output after a failed local undo-file read. It does not make corrupted undo data valid, alter the on-disk format, change recovery policy, or claim a remote trigger; callers still fail and report the underlying corruption.

### Cycle 58 completion and handoff

- Source/test commit: `3e4ec4e7ef0f216c09c10b1d577fc1517a043434`, authored as `Lőrinc <pap.lorinc@gmail.com>`.
- Verification completed after the commit: `git diff --check`; `cmake --build build_unit_clang19 --target test_bitcoin -j4`; the focused checksum-failure regression with 11 assertions; the full `blockmanager_tests` suite with 12 cases and 128 assertions. ASan was unavailable in the local build environment. No source, test, daemon, fuzz, sanitizer, or profiling process remains running.
- Cycle verdict: confirmed and fixed. The next run must draw a fresh goal from `0..98`, search the full history and current risk map before selecting a distinct hypothesis, and avoid reopening this undo-output cell unless a new caller, backend, recovery mode, or recurrence provides independent evidence.

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
