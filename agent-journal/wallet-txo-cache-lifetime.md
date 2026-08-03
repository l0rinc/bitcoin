# Wallet TXO cache lifetime and replacement audit

## Seed from Cycle 322

The static analyzer warning at `src/wallet/spend.cpp:529` led to a separate
wallet cache trace. `WalletTXO` stores references to a `CWalletTx` and its
`CTxOut`, while `CWallet::AddToWallet()` can replace a witness-stripped
`CWalletTx::tx` with a witness-bearing transaction having the same txid.
`RefreshTXOsFromTx()` previously skipped existing cache entries, leaving a
reference into the old transaction. The new Cycle 322 regression
`wallet_tests/wallet_txos_follow_witness_upgrade` failed before the fix on
distinct cached/current output addresses and passes after the cache entry is
rebound.

## Continuing protocol

Start each future cycle from a fresh gate and dedicated branch. Search this
journal, the selected goal journal, history, and prior wallet cache findings
before choosing a new transition. Inventory every reference-bearing cache and
every refresh, replacement, import, migration, reload, reorg, and erase path.
Use same-object and replacement-object pointer identity checks, deterministic
fixtures, ASan/UBSan when available, and restart or process-boundary tests.
Keep the witness-upgrade regression as a permanent seed. A stale reference,
use-after-free, or stale value requires a failing-before/passing-after oracle
and an independent ownership or sanitizer argument before a fix is committed.
