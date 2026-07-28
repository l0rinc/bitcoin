# Cross-subsystem bug shapes: cycle 48

## Gate

- Goal draw: `26`, `cross-subsystem-bug-shapes`, selected with `shuf -i 0-98 -n 1`.
- Branch: `fuzz-contract-cluster-oracles-20260709`.
- Gate HEAD: `0056520a3f23bb928179334e00b1ca782301588d`.
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Divergence at the gate: `2` local commits and `864` commits on `origin/master`.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Corrected TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- The TSV was mechanically corrected before selection from literal `\\t` text to byte `0x09` delimiters. The catalog and protocol were unchanged.
- No relevant process was running at entry; unrelated untracked agent artifacts and `test/cache` were preserved.

## Historical seed and scope

The seed was upstream merge `e3554bf361ff6979b09fdedfdcbebf687590cd1c`, containing `336f5a738b3d89abb8dd7ebdc4b31ac12a6de85f` (`wallet: reserve walletrescan before checking wallet is at the tip`) and test commit `9e62e4b1f346f70eaa23e83eff769b6e6cc04630`. Its PR description identifies a race in `importdescriptors`: caller A reserves, then holds `cs_wallet` during descriptor processing; caller B blocks in `BlockUntilSyncedToCurrentChain()`, and can reserve after A's scan releases the flag.

The fixed analogue in `src/wallet/rpc/backup.cpp` reserves at lines 384-387 before the synchronization wait at lines 389-391. `src/wallet/rpc/transactions.cpp` instead waited at lines 864-866 and reserved at lines 868-871. The latter was the unchecked cross-subsystem variant.

## Hypothesis

Two concurrent `rescanblockchain` calls can both succeed. A reserves, validates, and enters `ScanForWalletTransactions()`, which takes and releases `cs_wallet` while processing blocks. B enters `BlockUntilSyncedToCurrentChain()`, blocks acquiring `cs_wallet`, and resumes after A has finished and its `WalletRescanReserver` destructor has cleared `fScanningWallet`; B then reaches `reserve()` and succeeds. The trust boundary is two concurrent authenticated RPC callers sharing one loaded wallet. The contract is that only one wallet rescan may be active and a competing call returns `RPC_WALLET_ERROR` with the existing message.

## Verification

### Source and lock trace

- `CWallet::BlockUntilSyncedToCurrentChain()` reads `m_last_block_processed` under `cs_wallet`, then waits for validation notifications without holding that mutex.
- `ScanForWalletTransactions()` takes `cs_wallet` around each block's `SyncTransaction()` and releases it before advancing or returning.
- `WalletRescanReserver` owns the atomic `fScanningWallet` reservation and clears it in its destructor. There is no shared lock coupling the destructor with the other caller's `BlockUntilSyncedToCurrentChain()` return.
- The schedule is therefore valid: A reserves, B blocks on `cs_wallet`, A releases the final scan lock and destroys its reserver, B returns from the wait and reserves.

### Clean controls

- `cmake --build build_unit_wallet_clang19 --target bitcoind -j2` passed after the repair.
- New `test/functional/wallet_rescan.py` disables the block-filter fast path, generates 1,000 wallet-relevant blocks, and runs ten pairs of concurrent rescan RPCs over separate authenticated connections. The repaired build exited 0 with one success and one `-4` conflict in every pair.
- The same normal workload against the pre-repair ordering also exited 0 in this environment. It did not hit the rare scheduler ordering, so it is retained as a workload control rather than treated as a dismissal.
- The existing wallet-enabled `test/functional/wallet_importdescriptors.py` completed successfully before this source change, including its historical concurrent-import regression.

### Independent forced schedule

To remove scheduler luck, a disposable build-only probe temporarily added a sleep and log marker while `ScanForWalletTransactions()` held `cs_wallet`, and delayed the second caller after its synchronization wait. With the old `transactions.cpp` ordering, `wallet_rescan.py` failed at its assertion with `successes=2` (`AssertionError: not(2 == 1)`). After moving the reservation before the wait, the identical forced control exited 0 with one success and one conflict. The temporary include, counter, sleep, and log were removed and `git diff` confirms no instrumentation remains.

This is an independent schedule proof, not a production timing assumption: it directly exercises the lock/reservation ordering and preserves the exact RPC boundary and error contract.

## Fix and verdict

Verdict: **confirmed**. Move `WalletRescanReserver` construction and `reserve()` before `BlockUntilSyncedToCurrentChain()` in `rescanblockchain`, matching the already-fixed `importdescriptors` contract. The smallest source change is the four-line move in `src/wallet/rpc/transactions.cpp`; no behavior changes for a single caller beyond reserving earlier, and invalid parameter validation still occurs after synchronization as before.

The regression test is `test/functional/wallet_rescan.py`. Source/test commit: pending. Journal/state commit: pending.

## Limitations and handoff

- The ordinary pre-fix stress workload did not reproduce the race within ten pairs; the forced schedule did. The race is scheduler-dependent without a test hook.
- The disposable instrumentation was not committed. The durable test validates the public mutual-exclusion contract but is not solely relied on for the proof.
- Narrow source build and functional validation passed. A broader wallet functional suite remains in the next validation stack.
- Next cycle must re-check branch/base/HEAD, worktree, processes, hashes, journals, and history before drawing a distinct goal.
