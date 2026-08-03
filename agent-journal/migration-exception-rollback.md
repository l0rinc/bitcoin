# Migration exception and rollback-boundary audit

This seed was created from Cycle 324. `DoMigration()` threw after an auxiliary descriptor database commit failed, while `MigrateLegacyToDescriptor()` only restored the legacy backup when the helper returned `false`. The fixed watch-only descriptor path is recorded in `history-incomplete-fixes.md`; this journal is for distinct exception-versus-error boundaries elsewhere.

Initial queue:

1. Search migration, import, reload, backup, settings, and auxiliary-wallet paths for `throw` or exception-producing helpers between durable state changes and their caller cleanup blocks.
2. Map every created file/database, in-memory registration, lock release, and restart expectation before injecting a failure.
3. Reuse daemon-only fault injection on scratch fixtures and add only minimal source/regression changes for independently confirmed omissions.

Do not reopen the fixed auxiliary descriptor loops or the intentional `MigrateToSQLite()` assertion without new callers, build modes, or recurrence evidence.
