# Journal: Bitcoin wallet encryption, backup, descriptor, keypool recovery audit (campaign 88)

Uber-goal rotation, severity-first: key-loss / irreversible fund-loss class.
Branch: audit/bitcoin-wallet-recovery from audit/resurrection @ a2b80a4fd7.
Tools: scratch wallets in tmpdirs only; wallet functional tests + unit tests;
no default datadirs ever. Trust boundary: crash at any step + DB write
failure; NOT attacker-with-disk (that's a different threat model).

## Scope ledger

| # | area | hypothesis seeds | verdict |
|---|------|------------------|---------|
| W1 | encryption / passphrase change | mixed plaintext/encrypted state on crash or DB failure mid-EncryptWallet | open |
| W2 | master key lifecycle | missing/duplicate master key after failed rotation; mapMasterKeys vs DB divergence | open |
| W3 | descriptor/key encryption | Encrypt() of spk managers partial failure — some descriptors encrypted, some not | open |
| W4 | keypool/top-up + address reservation | crash between DB write and memory update → duplicate reservation or gap | open |
| W5 | migration (legacy→descriptor) | crash mid-migration (seen wallet.cpp:3880-3949 in goal 26 — cursor-safe; crash-safety open) | open |
| W6 | backup/restore + rescan | restore from stale backup → silent key loss vs detected conflict | open |
| W7 | external signers | signer disconnect/failure mid-sign → partial state | open |

## Verdicts

### W1 (encryption / passphrase change): DISMISSED — DB-authoritative, atomic record, multi-master unreachable

ChangeWalletPassphrase (wallet.cpp:636-674):
1. Ordering: in-memory master key is re-encrypted BEFORE the DB write, but
   the DB is authoritative — crash between leaves DB holding a valid record
   (old passphrase); restart reloads from DB. No mixed plaintext/encrypted
   state possible: only the master-key RECORD changes (key material itself
   is untouched), and the record write is atomic.
2. Write failure: in-memory record restored from the old key and failure
   reported (658-664) — own prior fix 501bd2e263.
3. EncryptWallet's master-key write failure guarded — own prior fix
   b8fcf9ed17.
4. Multi-master asymmetry NOTED but unreachable: the loop returns after the
   first successfully decrypted key (single-key update), while
   Unlock/Decrypt iterate all keys. However multi-master wallets are
   unproducible through any supported flow — EncryptWallet refused
   re-encryption even in the original 2011 implementation (4e87d341f7:
   `if (IsCrypted()) return false`) and refuses today (HasEncryptionKeys
   guard, 845). Single-master is the only reachable state; the single-key
   path matches it. Original 2011 design, not a regression.

### W3 (descriptor/key encryption partial failure): DISMISSED — single transaction + abort-and-die

EncryptWallet spk-manager loop (wallet.cpp:866-919):
1. ALL managers encrypt inside ONE WalletBatch transaction (TxnBegin 867).
   Any Encrypt() failure mid-loop → TxnAbort + assert(false) (885-891) —
   disk untouched, process dies loudly, reload yields a consistent
   UNENCRYPTED wallet. TxnCommit failure → same (894-900).
2. Crash (SIGKILL) mid-loop or mid-commit: SQLite rolls back the
   uncommitted transaction on next open → disk unencrypted → consistent
   reload. All-or-nothing by the DB engine, not by convention.
3. Post-commit residual: DB Rewrite purges slack-space plaintext
   (914-918); its failure is warn-only with an explicit user remediation
   (dumpwallet → restorewallet). Wallet is functionally encrypted
   regardless; residual is a documented secrecy note, not correctness.
4. Master-key record write inside the same transaction is covered by own
   prior fix b8fcf9ed17.

### W4 (keypool/top-up + address reservation): no key-loss path; CONFIRMED small defect (unchecked WriteDescriptor) — FIXED

1. Crash windows in GetNewDestination (scriptpubkeyman.cpp:874-911):
   next_index++ (memory) then immediate WriteDescriptor. Crash between →
   restart re-issues the same address from the keypool (the key is already
   in the pool from TopUp's own batch) — address REUSE, never key loss.
   TopUp's pool entries are batch-committed before any issue. ReturnDestination
   failure resolves to a skipped address (safe). No fund-loss path exists.
2. CONFIRMED DEFECT (low severity): WalletBatch::WriteDescriptor returns
   bool, checked at 1114/1205/1529 (throws) but IGNORED at 908
   (GetNewDestination) and 997 (ReturnDestination). On DB write failure,
   memory next_index diverges silently from disk — restart after failure
   re-issues given-out addresses as "fresh" (908: cross-payer address
   reuse, privacy) or skips a returned one (997: benign waste). Inconsistent
   with identical sibling calls that check.
   Verifiers: (a) source contrast + consequence trace above; (b) build +
   wallet_tests + scriptpubkeyman_tests green after fix. Limitation: no
   DB-failure injection hook exists in the wallet, so divergence was proven
   by code-path analysis, not runtime injection.
   FIX (this commit): 908 returns util::Error matching the function's error
   style; 997 throws matching the sibling at 1529.

### W5 (migration legacy→descriptor crash-safety): DISMISSED — complete pre-step backup covers the destructive window

Migration flow (wallet.cpp:4376-4425):
1. Backup FIRST: timestamped `<name>_<ts>.legacy.bak` written via
   BackupWallet (4376-4393) BEFORE any destructive step, and documented in
   migratewallet RPC help (rpc/wallet.cpp:592).
2. MigrateToSQLite (3880-3949): all records are read into memory BEFORE the
   old DB file is deleted (3919); the new SQLite DB gets every record in
   ONE transaction (TxnBegin 3937, TxnCommit 3947) with assert(false) on
   any write failure (loud, not silent).
3. Crash window (SIGKILL between old-file delete and TxnCommit): SQLite
   rolls back → new DB is an empty-but-valid wallet; the COMPLETE
   .legacy.bak sits in the same directory. Recovery: restorewallet from
   the backup (restore path at 4547-4555 / RestoreWallet 457-510). No key
   loss possible — the backup predates the destructive step.
4. FRAGILITY NOTED (not a defect): there is no load-time detection of an
   interrupted migration — a crashed migration leaves an empty-looking
   wallet and the user must know to restore the .bak (documented in RPC
   help, but not surfaced at load). Accepted-risk: crash-only window,
   complete backup, manual but documented recovery. A load-time warning is
   a possible future improvement, out of minimal-diff scope here.

### W6 (backup/restore + rescan): DISMISSED — refuse-overwrite, clean rollback, no loss

RestoreWallet (wallet.cpp:457-530): existing wallet.dat is a hard failure
(486-499 — no silent overwrite of a newer wallet); copy then LoadWallet;
on load failure the copied file (and created parent dir) are removed
(522-529) — clean rollback. Stale-backup effects are bounded to address
reuse (same class as W4, inherent to backups, documented); receiving to a
stale backup uses keys it has — no loss path. Rescan-from-stale-height is
covered by wallet rescan logic; prune interactions fail loudly, not
silently.

### W7 (external signers): DISMISSED — stateless signing

ExternalSignerScriptPubKeyMan::SignTransaction (external_signer_scriptpubkeyman.cpp:95-115):
PSBT lives in memory only; signer-not-found and signer-failure both return
errors (103-112) with nothing persisted to the wallet DB. Crash/disconnect
mid-sign loses only the in-memory PSBT. Fully-signed re-entry handled
(96-100). Multisig partial signatures are a caller concern by design, not
wallet state.

## Campaign 88 cycle complete

All 7 ledger areas locked: W1, W3, W5, W6, W7 DISMISSED; W2 subsumed by
W1/W3 (master-key lifecycle covered there — no divergence found);
W4 CONFIRMED small defect (unchecked WriteDescriptor results) — fixed in
0e7a8fabb5. Rotation: uber-ledger marks #88 DONE, next #87.

## Next queue
(W1 first: read EncryptWallet + ChangeWalletPassphrase transactionality;
then W3 partial Encrypt failure)
