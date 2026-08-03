# Whole-history incomplete-fix and migration mining

## Cycle 261: GUI wallet-migration load-policy follow-up

### Fresh selection and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `32` (`history-incomplete-fixes`).
- Branch: `uber-cycle-261-whole-history-migration-mining-20260802`.
- Start HEAD: `8a826f6659d6c11657a70c6d65ec67d107bdb953`; `origin/master` after
  the fresh fetch: `556988790a7f961693a8fd93f73725baea66476a`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence `1311 45`.
- The tracked worktree/index was clean at the draw. `git diff --check`, the
  catalog/prompt/goals TSV/protocol hash gate, and the protected-process check
  passed. Existing untracked agent/user artifacts, package files,
  `node_modules/`, and `test/cache/` were preserved.

### Historical seed and distinct hypothesis

Recent history contains two separate migration changes: `4acd063ba6`
(`wallet: make loading the wallet after migrating optional`) added the core
`MigrateLegacyToDescriptor(..., bool load_wallet)` path and the RPC's
`load_wallet` option; `492a715d78` (`gui: Adds option to not load the wallet
after migration`) then propagated that policy through the GUI interface. The
second change is not an ancestor of this branch.

Before this cycle, the current GUI contract still declared
`WalletLoader::migrateWallet(name, passphrase)` without a load-policy
parameter. `WalletLoaderImpl::migrateWallet` called the core function with its
default `true`, and `MigrateWalletActivity::do_migrate` unconditionally passed
`res->wallet` to `getOrCreateWallet`. Thus the GUI could not use the already
supported no-load migration mode. This is a distinct interface/lifecycle
omission from the earlier wallet migration write, cleanup, best-block, and
HTTP queue cells.

The existing `wallet_migration.py` contract supplies an independent failure
case: `test_no_load_after_migration` verifies that a successful
`migratewallet(load_wallet=False)` leaves a migrated SQLite wallet on disk but
does not load it; `unsynced_wallet_on_pruned_node_fails` verifies that loading
an unsynced migrated wallet can fail on a pruned node while migration with
`load_wallet=False` still succeeds. The GUI had no way to select that behavior.

### Fix

- Add `bool load_wallet` to the public wallet-loader migration interface and
  forward it to `MigrateLegacyToDescriptor`.
- Add a checked-by-default GUI option for normal migration, pass its value
  through the asynchronous activity, and avoid constructing a wallet model
  when the user chooses not to load it. Report that the wallet can be opened
  later from the File menu.
- Keep restore-and-migrate explicitly `load_wallet=true`, preserving its
  existing behavior and making the policy visible at the call site.

### Validation

The following clean configuration and builds passed:

```text
TMPDIR=/data/my_storage/tmp/cycle261-qt-config-tmp \
cmake -S . -B /data/my_storage/tmp/cycle261-qt-build \
  -DBUILD_GUI=ON -DENABLE_WALLET=ON -DBUILD_TESTS=OFF \
  -DBUILD_BENCH=OFF -DWITH_ZMQ=OFF -DCMAKE_BUILD_TYPE=Debug

TMPDIR=/data/my_storage/tmp/cycle261-qt-build-tmp \
CCACHE_DIR=/data/my_storage/tmp/cycle261-qt-ccache \
cmake --build /data/my_storage/tmp/cycle261-qt-build --target bitcoin-qt -j2

TMPDIR=/data/my_storage/tmp/cycle261-daemon-build-tmp \
CCACHE_DIR=/data/my_storage/tmp/cycle261-qt-ccache \
cmake --build /data/my_storage/tmp/cycle261-qt-build \
  --target bitcoind bitcoin-cli -j2
```

`bitcoin-qt`, `bitcoind`, and `bitcoin-cli` all built successfully. The full
`wallet_migration.py` run was attempted with the v28.2 previous-release
fixture, but the Debug daemon stopped before the target migration checks at
the existing `txgraph.cpp:3781` assertion:

```text
./txgraph.cpp:3781 virtual size_t {anonymous}::TxGraphImpl::GetMainMemoryUsage():
Assertion `(usage == 0) == (m_main_clusterset.m_txcount == 0)' failed.
```

The resulting `RemoteDisconnected` was a test-environment abort, not evidence
against this GUI change. No Qt runtime test infrastructure was enabled in the
clean build; the compile covered the changed interface implementation and
both asynchronous GUI call paths. `git diff --check` passed.

### Verdict and residual queue

Verdict: confirmed and fixed. The old GUI path had a reachable feature and
operability omission: it forced the core migration to load the result even
when the user needs migration without a historical rescan/load. The fix is
one self-contained interface/UI commit and preserves the default behavior.
The failed Debug functional run must not be presented as a passing full-suite
result. Continue mining different recent compatibility or migration cells;
do not repeat the GUI load-policy, core no-load, HTTP parsed-request queue,
or prior wallet migration cells without a new source, backend, or lifecycle.

## Cycle 259: HTTP pipelined-request retention follow-up

### Fresh selection and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `32` (`history-incomplete-fixes`).
- Branch: `uber-cycle-259-whole-history-migration-mining-20260802`.
- Start HEAD: `5cc4012b2b84a6a1c2e5c5ec05b23d0d10526601`; `origin/master` after
  the fresh fetch: `556988790a7f961693a8fd93f73725baea66476a`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence `1307 45`
  from `git rev-list --left-right --count HEAD...origin/master`.
- The tracked worktree/index was clean at the draw. `git diff --check`, the
  catalog/prompt/goals TSV/protocol hash gate, and the protected-process check
  passed. Existing untracked agent artifacts, package files, `node_modules/`,
  and `test/cache/` were preserved. The protected PIDs were observed and not
  stopped or rebuilt.
- The REST `/getutxos` count-bound cell was rejected as a repeat: it is already
  recorded in `agent-journal/resource-exhaustion-variants.md` Cycle 104 and
  the bounded formatter is present in the current source. The HTTP pipeline
  history cell below had no prior finding in the journal search.

### Historical seed and distinct hypothesis

Upstream commit `97b882113e02a5149396c9ed6879e46e6daa1f13`,
`http: only read one HTTPRequest at a time per client`, is a follow-up to the
HTTP request-dispatch implementation currently present at the cycle start.
The old code parsed requests in `while (!client->m_recv_buffer.empty())`,
appended every complete request to `m_req_queue`, and checked `m_req_busy` only
after that loop. A slow or blocked first handler therefore allowed a client to
retain an unbounded number of parsed `HTTPRequest` objects, each keeping a
shared pointer to the client and potentially a body, before the worker could
make progress. The accepted upstream patch is corroborating history, not the
sole proof for this cycle.

The resource equation is:

```text
retained per client = (complete pipelined requests parsed before dispatch)
                      * (HTTPRequest + body + queue node)
```

The request body has an individual limit, but the old parsed-request count had
no per-client bound and the receive loop could continue across I/O iterations.
The falsifiable hypothesis was that the first handler would observe all
pipelined bytes already removed from `m_recv_buffer`, whereas a one-request
handoff would leave the next request buffered until the first handler completes.

### Independent failing-before evidence

`http_server_pipelined_request_backpressure` sends two complete requests in a
single `DynSock` receive, blocks the first request handler, and then inspects
the client receive buffer before releasing the handler. The measured 48 bytes
are the second request's exact serialized form. In the
pre-fix worktree, configured with Clang 19 and `-DENABLE_IPC=OFF`, the focused
command exited 201 and reported:

```text
check client->m_recv_buffer.size() == remaining.size() has failed [0 != 48]
check std::ranges::equal(client->m_recv_buffer, remaining) has failed
2 assertions out of 5 failed
```

This observes the old loop's actual consumption, not an inferred allocation
from source inspection. The two-request fixture is intentionally small; the
same ordering permits additional requests to accumulate while the handler is
busy.

### Fix and passing evidence

- Replace the unbounded per-client `std::deque<std::unique_ptr<HTTPRequest>>`
  with one pending `std::unique_ptr<HTTPRequest>`. Parse only when no request
  is pending, return on incomplete input, and dispatch the single pending
  request in order.
- Release a pending request during both graceful and forced client removal so
  its back-reference cannot keep a disconnected socket alive.
- Keep the existing receive-buffer and request-body parsing contracts; this
  change bounds parsed pending request state rather than claiming to solve
  every raw receive-buffer or worker-queue resource issue.
- Add the deterministic socket regression described above.

Validation on the fixed worktree:

```text
git diff --check
TMPDIR=/data/my_storage/tmp/cycle259-http-build-tmp \
CCACHE_DIR=/data/my_storage/tmp/cycle259-http-ccache \
cmake --build /data/my_storage/tmp/cycle243-build --target test_bitcoin -j2
TMPDIR=/data/my_storage/tmp/cycle259-http-test-tmp \
/data/my_storage/tmp/cycle243-build/bin/test_bitcoin \
  --run_test=httpserver_tests --log_level=test_suite \
  --report_level=short --color_output=false
```

The build passed. The complete `httpserver_tests` suite passed 9 cases and
352 assertions, including the new regression and the existing
`http_socket_error_tests` pipeline/response-state case. The independent
pre-fix build used the same source revision and test, with IPC disabled after
the documented Clang 19/Cap'n Proto configure incompatibility; only the
production/header files were pre-fix in that worktree.

### Verdict and residual queue

Verdict: confirmed and fixed. The old path had a reachable per-client
parsed-request retention defect caused by an incomplete follow-up to the HTTP
request state machine. The fix preserves request order and valid pipelining,
removes the unbounded parsed-request deque, and explicitly releases pending
ownership on disconnect. The regression does not establish a global memory
bound for raw socket receive buffers, worker scheduling, or endpoint work; those
remain separate cells and must not be conflated with this finding.

Next Goal 32 queue: mine the remaining recent HTTP/RPC follow-ups and
compatibility migrations for a different source-to-sink omission, then move to
non-HTTP history. Do not repeat the REST count, HTTP parsed-request queue, or
previous wallet/descriptor/best-block/undo migration cells without a new
caller, backend, lifecycle, or recurrence.

## Cycle 193 start: persisted-field and restart-transition mining

### Fresh selection and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `32` (`history-incomplete-fixes`).
- Branch: `uber-cycle-193-history-incomplete-fixes-20260731`.
- Start HEAD: `c5989c28f9e55455f73de9a1ff3aae6c5a6b418e`; `origin/master`:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1176 42`.
- `git fetch origin master` passed; tracked/index state was clean;
  `git diff --check` passed; known untracked agent/user artifacts were
  preserved. Protected PIDs `777094`, `956381`, and `1138182` were alive and
  untouched.
- Catalog, prompt, corrected goals TSV, protocol, and state hashes are
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`,
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, and
  `692f79f74b72d63c4727711ad6ac269d9ac41c9e77bf36db0f7bd98abe4eba51`.

### Distinct scope and initial queue

Prior Goal 32 cells are closed and excluded: legacy best-block corruption
classification, auxiliary-wallet startup settings after failed migration,
empty `-connect` retry handling, undo output publication, and the earlier
unchecked wallet migration writes. This cycle mines a different evidence
source: historical commits that added fields, compatibility readers, or
restart-visible state, then compares every current writer, reader, serializer,
cleanup path, and alternate module.

Initial queue:

1. Recent persisted-field and migration follow-ups in wallet, chainstate,
   indexes, and settings; search for a writer or reader that was updated in
   only one lifecycle or backend.
2. Reverted or follow-up fixes involving cache publication, load/restart,
   optional modules, and compatibility flags; seek a current sibling with the
   same invariant but a distinct caller or source format.
3. Historical bug-fix pairs involving accounting or cleanup; use current
   tests and fault injection to distinguish intentional partial progress from
   a real incomplete fix.

Each candidate must have a current call path, a source/history rationale, a
complete expected contract, and an independent reproducer or proof. Do not
reopen the excluded cells merely because a similar symbol appears; the new
cell must differ by source, backend, lifecycle, or trust boundary.

### Cycle 193 candidate: descriptor-cache follow-up omitted the import transaction

- Historical seed: `a1c83789a7` (`wallet: Write new descriptor's cache in
  AddWalletDescriptor`). The fix added persistence for the cache supplied by
  watch-only descriptor exports, but the current code published the new
  `DescriptorScriptPubKeyMan` before writing that cache. `CreateFromImport`
  also wrote the descriptor and generated cache through a separate batch, so a
  cache write failure could return an error after leaving a descriptor row and
  a live manager in memory. This is a distinct source/lifecycle cell from the
  earlier descriptor update/top-up rollback work already recorded elsewhere.
- Current path and contract: `CWallet::AddWalletDescriptor` receives a
  serialized descriptor plus cache, calls `DescriptorScriptPubKeyMan::CreateFromImport`,
  writes the supplied cache, then labels and persists the descriptor. On any
  initial import failure, neither the descriptor manager nor any of its
  descriptor, derived-cache, or key records should become visible. A retry
  after the injected failure must behave like a fresh import.
- Prior-finding search: `git grep` across `agent-journal`, source, tests, and
  history found no existing finding for `a1c83789a7` or this new-cache import
  failure path. Existing cells cover descriptor updates, top-ups, encryption,
  and migration writes, but not the supplemental cache write introduced by
  this commit.
- Independent pre-fix reproducer: added
  `scriptpubkeyman_tests/add_wallet_descriptor_cache_write_failure_preserves_state`.
  It imports a ranged `wpkh(xpub/*)` descriptor with a parent cache and uses a
  SQLite trigger to abort the exact `walletdescriptorcache` row. On the
  unmodified implementation, the test failed at
  `!wallet.GetDescriptorScriptPubKeyMan(descriptor)`; 5/6 assertions passed,
  while the descriptor row remained and the cache row was absent. This proves
  both premature memory publication and durable partial state, rather than a
  theoretical ordering concern.
- Fix: pass one active `WalletBatch` from `AddWalletDescriptor` through
  `CreateFromImport`, write the supplied cache in that transaction, abort on
  any failure, and add the manager only after commit. `TopUpWithDB` now delays
  `TopUpCallback` until transaction commit and uses an explicit no-op abort
  callback, preventing a failed import or update from publishing a dangling
  script-cache pointer. The regression drops the trigger, retries the same
  descriptor, and verifies the descriptor and cache rows are then present.
- Post-fix evidence: `ninja -C /data/my_storage/tmp/cycle84-build
  test_bitcoin -j2` passed; the focused regression passed 1 case/11
  assertions with seed `19302`; `scriptpubkeyman_tests` passed 20 cases/191
  assertions with seed `19303`; and `wallet_tests` passed 25 cases/216
  assertions with seed `19304`. Adjacent `wallet_rpc_tests` passed 1 case/9
  assertions with seed `19305`, `walletload_tests` passed 1 case/6 assertions
  with seed `19306`, and `walletdb_tests` passed 2 cases/5 assertions with
  seed `19307`. `git diff --check` passed. Protected test processes remained
  alive and untouched. The rebuilt wallet functional test
  `wallet_basic.py` also passed with the scratch `cycle193-wallet-basic`
  datadir and no cleanup errors.
- Verdict: confirmed and fixed. The mechanism was an omitted transaction
  boundary in the historical cache follow-up, with reachable wallet-import
  impact: an injected storage failure could leave restart-visible descriptor
  state and stale in-memory publication despite a failed operation. Continue
  Goal 32 from a fresh historical cell after this commit; do not reopen the
  already closed descriptor-update or migration cells.

## Cycle 173 start: distinct follow-up and compatibility mining

### Fresh selection and gate

- The exact post-Cycle-172 selector returned `30`, which was explicitly closed
  by Cycle 164. The required exact reroll returned `32`
  (`history-incomplete-fixes`).
- Branch: `uber-cycle-173-history-incomplete-fixes-20260730`.
- Start HEAD: `2adf2d2ba43e7688a589a18f8866686c77b94d13`; origin/master:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1127 42`.
- `git fetch origin master`, tracked/index diff checks, and the input-hash
  gate passed. Known untracked agent/user artifacts remain outside scope.
  PIDs `777094` and `956381` are persistent unrelated unit tests and must not
  be stopped.
- Catalog, prompt, corrected goals TSV, and protocol hashes remain
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

### Exclusions and initial hypothesis

This is a new goal-32 cell. Do not reopen Cycle 167's auxiliary-wallet
settings transaction boundary, Cycle 137's empty `-connect` retry loop, Cycle
58's `ReadBlockUndo` output atomicity, or Cycle 43's wallet migration write
return. Also exclude later source commits already reviewed in those journals.

Mine historical partial fixes, follow-up commits, reverted changes, and
compatibility migrations for a current sibling with a different source,
caller, trust boundary, or lifecycle. Prioritize fixes that changed only one
variant, moved a check without moving publication/cleanup, introduced a new
field without updating a serializer/index/accounting path, or preserved old
behavior in one build/module while changing another. A candidate must have a
current call path and a concrete failure or invariant mismatch; history is a
seed, not proof.

Initial queue:

1. Recent follow-up pairs involving error returns, cleanup, persistence, or
   compatibility options; compare every sibling call site on current HEAD.
2. Reverted or partially reverted changes whose original rationale still
   applies to a current alternate path or feature configuration.
3. Migration/format changes outside wallet settings, especially readers and
   writers that accept both legacy and current state across restart.

For every candidate, record the seed commit and rationale, current source and
callers, trust boundary, expected contract, exact reproducer or proof, prior
finding search, and an independent verdict before changing code. Keep the
smallest self-contained commit discipline from the uber protocol and continue
to a new historical cell after each verdict.

### Cycle 173 candidate: migration collapsed a corrupt locator into an ancient wallet

- Historical seed: `fd44d48b24` (`wallet: fix ancient wallets migration`), merged
  by `5486ef8cc2`. The change correctly stopped treating a missing best-block
  locator as an error because wallets predating PR #152 do not contain either
  locator record; the resulting empty locator intentionally causes a rescan.
  The current `CWallet::ApplyMigrationData()` still ignores the boolean result
  at `src/wallet/wallet.cpp`, while `WalletBatch::ReadBestBlock()` returns the
  same `false` value for a missing key, a database read failure, and a
  deserialization failure.
- Current contract and trust boundary: a legacy wallet database is local input;
  an absent locator is valid compatibility state, a valid empty locator is a
  valid rescan request, and an existing undecodable locator is corrupt state
  that must stop migration. `WriteBestBlock()` intentionally writes an empty
  `BESTBLOCK` record plus the actual `BESTBLOCK_NOMERKLE` record, so the former
  cannot be treated as corruption merely because its locator is empty. Current
  callers of the boolean API are export, chain selection/rescan, and wallet
  tests; the migration caller is the only one that needs to distinguish absence
  from corruption.
- Independent source verification: `DatabaseBatch::Read()` in
  `src/wallet/db.h` returns false for both `ReadKey()` failure and stream decode
  exceptions, while `DatabaseBatch::Exists()` checks the serialized key. The
  SQLite and Berkeley read implementations retain the raw record/key
  distinction, so a present malformed value can be detected without changing
  the existing boolean callers. SQLite low-level I/O errors that make both
  `ReadKey()` and `HasKey()` fail remain a documented limitation; this fix is
  specifically for a present undecodable record.
- Failing-before reproduction: the new
  `wallet_tests/migration_corrupt_best_block_is_reported` inserts a raw
  `BESTBLOCK` value `X'01'`, which encodes a truncated nonempty locator. On the
  pre-fix binary, the exact command
  `/data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=wallet_tests/migration_corrupt_best_block_is_reported --log_level=test_suite --report_level=short --color_output=false`
  exited 201 at `wallet_tests.cpp(180)` because
  `!wallet.ApplyMigrationData(batch, *data)` was false. The malformed record
  was therefore treated like an ancient missing record and migration returned
  success.
- Fix: add `BestBlockReadResult::{FOUND, NOT_FOUND, ERROR}` and
  `WalletBatch::ReadBestBlockResult()`. It preserves empty-`BESTBLOCK` fallback
  behavior, reports a present undecodable `BESTBLOCK` or
  `BESTBLOCK_NOMERKLE` as `ERROR`, and leaves `ReadBestBlock()` as a boolean
  compatibility wrapper. `ApplyMigrationData()` rejects `ERROR` before any
  in-memory or on-disk migration mutation, preserving the legacy script manager
  when the database is corrupt.
- Regression tests: `wallet_tests/migration_corrupt_best_block_is_reported`
  now passes 8/8 assertions and confirms the legacy manager remains present;
  `walletdb_tests/walletdb_best_block_read_result` covers absent and valid-empty
  locator states with 3/3 assertions. The existing
  `wallet_tests/migration_transaction_write_failure_is_reported` remains green
  with 8/8 assertions, confirming ancient/no-locator compatibility and the
  later write-failure path.
- Build and commands: `CCACHE_DIR=/data/my_storage/tmp/cycle170-ccache make -C
  /data/my_storage/tmp/cycle89-build -j2 test_bitcoin` passed after the source
  change; `git diff --check` passed. The first build attempt failed only because
  the pre-existing build configuration pointed ccache at missing
  `/root/.cache/ccache/tmp`; rerunning with the `/data` cache succeeded. No
  default wallet, datadir, key, or production database was used. The two
  unrelated long-running wallet tests (`777094`, `956381`) were preserved.

Verdict: confirmed and fixed. This is a local wallet migration/corruption
handling defect: the historical compatibility fix was incomplete because it
discarded the information needed to reject a present malformed locator. The
patch does not alter the on-disk format or the boolean behavior of existing
readers, and it does not claim that low-level I/O failures are fully
distinguishable from absent records. Commit the source/test/journal finding as
one self-contained change, then record the cycle state separately and continue
with a fresh goal draw.

## Cycle 167: migration-side persistence follow-up

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `32`
- Slug: `history-incomplete-fixes`
- Branch: `uber-cycle-167-history-incomplete-fixes-20260730`
- Gate HEAD: `c95f1fd976f0fe4914ac2ad8393269cdc8f2ab4c`; `origin/master` at `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD=42 1115`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Corrected goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The fresh gate had no tracked modifications and preserved all pre-existing untracked artifacts and unrelated processes. Cycles 43, 58, and 137 were searched before accepting this draw; their migration write-return, undo-output, and empty-`-connect` cells are excluded.

### Scope and hypothesis queue

This cycle mines historical persistence and migration fixes for a current sibling outside the already-fixed `CWallet::ApplyMigrationData()` writes. I will first map every migration-side write, transaction boundary, failure return, and restart-visible effect, then compare them with the historical commits and tests that motivated the earlier fixes. A candidate must demonstrate a distinct unchecked write, wrong transaction ownership, stale in-memory publication, or incomplete migration/recovery transition with an isolated fault-injection reproducer. The trust boundary is local wallet/database state plus user-supplied legacy or migration data; no default wallet or production database will be used.

Initial queue:

1. `WalletBatch` writes and transaction ownership in migration helpers not covered by `ApplyMigrationData()`.
2. Address-book, descriptor, keypool, and metadata migration paths that commit state in more than one durable transaction.
3. A non-wallet historical partial-fix shape only if the migration map yields no distinct current omission.

### Confirmed finding: failed migration left a deleted auxiliary wallet in startup settings

- Historical source: `0bf7b38bff` introduced the auxiliary-wallet settings writes inside `DoMigration()` before `RunWithinTxn()` applied the main-wallet migration. `4acd063ba6` changed those writes to honor `load_on_startup` but preserved their pre-transaction position. The auxiliary wallets are created with an `empty_context`, so failure cleanup only resets them locally and does not call `RemoveWallet()`, leaving any already-written setting behind.
- Focused functional reproduction: the existing `test_failed_migration_cleanup` created a watch-only wallet, then deliberately blocked creation of `failed_solvables` with a pre-existing wallet database. On the unmodified binary, the test itself passed its existing cleanup assertions, but `/data/my_storage/tmp/cycle167-focus-pre-4/node0/regtest/settings.json` retained `"wallet": ["default_wallet", "failed_watchonly"]` even though `failed_watchonly` had been deleted. The scratch run used the v28.4 wallet-enabled release binary with BDB migration support and a current wallet-enabled binary; no default datadir was used.
- Fix: remove both pre-transaction settings writes and perform them only after `RunWithinTxn()` returns success. A failed migration therefore cannot advertise an auxiliary wallet that cleanup removes. The existing `load_on_startup` behavior is preserved for successful migrations, including the no-load path.
- Regression: extend `test_failed_migration_cleanup` to read `settings.json` and require both failed auxiliary names to be absent. The permanent test checks persisted state, not only `listwallets()` or filesystem cleanup.
- Verification: `git diff --check` passed. The wallet-enabled v28.4 depends/configure/build completed with BDB and SQLite support. `CCACHE_DIR=/data/my_storage/tmp/cycle167-current-ccache cmake --build /data/my_storage/tmp/cycle89-build --target bitcoind -j2` passed after the source edit. The post-fix focused functional run at `/data/my_storage/tmp/cycle167-focus-final` passed and left only `"wallet": ["default_wallet"]`; the temporary pre-fix ordering mutation rebuilt successfully and failed at the new `failed_watchonly` assertion, with the old setting restored. No owned daemon processes remain.

Verdict: confirmed and fixed. This is a local migration/restart correctness defect: a failed migration could cause a later startup to attempt loading a wallet directory that cleanup had removed. The next cycle must draw a fresh goal and avoid reopening this settings-transaction cell unless a distinct migration mode or recurrence supplies new evidence.

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

## Cycle 277: SQLite migration commit failure is an intentional fail-fast path

### Selection and gate

- Selector: exact `shuf -i 0-98 -n 1` draw `32`, `history-incomplete-fixes`.
- Branch: `uber-cycle-277-whole-history-migration-20260802`.
- Gate HEAD: `290da047ed09c1bfe4cc94765b918558bd3eb749`; `origin/master` at `556988790a7f961693a8fd93f73725baea66476a`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence `1343 45`.
- Catalog, protocol, and corrected goal TSV hashes matched the uber state. The prior migration cells for `ApplyMigrationData` write returns, auxiliary settings cleanup, corrupt best-block handling, GUI load policy, empty `-connect`, and HTTP request retention were searched and excluded.

### Hypothesis and independent evidence

The historical migration bridge in `CWallet::MigrateToSQLite()` uses assertions for new-database creation, transaction begin, record writes, and the final `TxnCommit()`. The initial hypothesis was that a failed SQLite commit could fall through as success in a release build, allowing `MigrateLegacyToDescriptor()` to continue with a partially written replacement wallet. This would be a distinct migration-side persistence failure outside `ApplyMigrationData()`.

The source and build policy falsify the release-fallthrough premise. `cmake/module/ProcessConfigurations.cmake` removes `NDEBUG` from every C++ configuration, and `src/util/check.h` has a compile-time error when `NDEBUG` is defined. The supported RelWithDebInfo binary therefore retains the assertion. The original implementation commit `e7b16f925ae` also explicitly documents each assertion as a critical condition where the original database is already deleted but a backup exists and execution must not continue. No later history or journal entry changes that contract.

The product-path fault injection independently exercised the final commit failure. A legacy BDB wallet copied from the v28.2 migration fixture was opened in a fresh current regtest datadir. A daemon-only `LD_PRELOAD` shim returned `SQLITE_IOERR` for the first `COMMIT TRANSACTION`, and logged `commit=1 sql=COMMIT TRANSACTION`. The exact command used the wallet-enabled RelWithDebInfo `bitcoind` and `bitcoin-cli`; no default datadir or production wallet was touched. The daemon logged `SQLiteBatch: Failed to commit the transaction`, then the foreground run exited 134 with:

`bitcoind: ./wallet/wallet.cpp:4020: bool wallet::CWallet::MigrateToSQLite(bilingual_str&): Assertion `committed' failed.`

The migration RPC consequently returned EOF because the process intentionally terminated. The original BDB remained at `legacy_1785673789.legacy.bak`; the replacement `legacy/wallet.dat` was a SQLite file left by the failed transaction. This confirms the documented fail-fast behavior and the backup invariant, not a silent success or continuation bug. The first broad functional preload attempt was discarded as harness-only evidence because it preloaded the Python test runner and stopped before the target wallet; the corrected daemon-only run avoided that contamination.

### Verdict and handoff

Verdict: dismissed as a new defect. The observed process termination is severe for an injected local I/O failure, but it is the explicit fail-fast contract of the historical migration implementation, and supported builds cannot disable the assertion. Replacing it with a recoverable RPC error would be a policy change requiring a separate wallet recovery design, not an evidence-backed incomplete-fix patch for this cycle. No source or test commit is warranted. The next cycle should mine a different historical migration or compatibility cell and should not reopen these assertion lines unless a new supported build mode or a recurrence changes the contract.

Next queue:

1. Review historical migration changes outside `MigrateToSQLite()` and `ApplyMigrationData()`, especially reload, backup, path, and cleanup follow-ups not covered by cycles 167, 173, 193, 259, or 261.
2. Compare current wallet migration tests with failure schedules that stop after the replacement database is created but before descriptor loading, excluding the already closed settings and auxiliary-wallet cleanup cell.
3. Re-rank against the whole-history queue after the next fresh selector draw; do not manufacture a fix when the only evidence is an intentional assertion policy.

## Cycle 324: auxiliary descriptor exceptions bypassed migration rollback

### Selection and gate

- Selector: exact `shuf -i 0-123 -n 1` draw `32`, `history-incomplete-fixes`.
- Branch: `uber-cycle-324-history-incomplete-fixes-20260802`.
- Selection commit: `fd66e34f5b`.
- The pre-cycle catalog had 124 contiguous goals (`0..123`); the prior migration cells for unchecked writes, settings cleanup, best-block recovery, GUI load policy, SQLite fail-fast behavior, and HTTP retention were searched before selecting this distinct exception boundary.

### Historical hypothesis and reproduction

The migration implementation added by the historical legacy-to-descriptor work (`0bf7b38bff`) creates auxiliary watch-only and solvable wallets before applying the main migration transaction. `DoMigration()` converted a false `AddWalletDescriptor()` result into `std::runtime_error`, while `MigrateLegacyToDescriptor()` only restored the Berkeley DB backup and removed created wallets when `DoMigration()` returned `false`. An exception from an auxiliary descriptor write could therefore skip the caller's cleanup block.

The pre-fix path was reproduced with a real v28.4 Berkeley DB wallet containing an imported watch-only public key. A daemon-only `LD_PRELOAD` SQLite shim was configured to fail the third `COMMIT TRANSACTION`, the commit used by `legacy_watchonly->AddWalletDescriptor()`. The exact current daemon command used scratch datadir `/data/my_storage/tmp/cycle324-migration-exception-fail3`, RPC port `32405`, and `FAIL_SQLITE_COMMIT_NUMBER=3`; it returned `Unable to write descriptor cache` but left `wallets/legacy/wallet.dat` as SQLite, left `legacy_1786003000.legacy.bak`, and left `legacy_watchonly/wallet.dat`. The daemon log recorded the failed commit followed by release of both wallets. This is a real product-path cleanup failure, not a test-only exception.

### Fix and independent verification

- Replace the duplicated parse/add loops with a local helper that preserves the existing descriptor invariants, converts a false result to `error`, catches descriptor-storage `std::exception` failures, and returns `false`. The outer migration code then executes its established auxiliary-wallet removal and legacy-backup restoration path.
- Rebuilt the wallet-enabled `bitcoind` from the changed source with `cmake --build /data/my_storage/tmp/cycle246-wallet --target bitcoind -j2`.
- Replayed the same third-commit fault on fresh datadir `/data/my_storage/tmp/cycle324-migration-exception-fixed`. RPC returned error code `-4` with `Unable to write descriptor cache`; `file` identified both `legacy/wallet.dat` and `legacy_1786003001.legacy.bak` as Berkeley DB, their SHA-256 values matched the original fixture (`fb589cdbdb8d25d7aca016b4c692640cada2c7dc7e123ea48090d9f7884ca9df`), and `legacy_watchonly` was absent. `listwalletdir` reported `legacy` as a legacy wallet requiring migration.
- Ran a no-fault control on the same watch-only fixture in `/data/my_storage/tmp/cycle324-migration-exception-success`. `migratewallet legacy` returned both `legacy` and `legacy_watchonly`; `file` identified both as SQLite and the backup as Berkeley DB; `listwallets` loaded both wallets and `getwalletinfo` confirmed `legacy_watchonly` is a descriptor, private-key-disabled wallet.
- `git diff --check` passed and all seven protected processes remained alive. The rebuilt focused `wallet_tests,scriptpubkeyman_tests` invocation was attempted but could not complete because the host low-disk guard fired during fixture setup (`/data` had 499M free and `/` had 0 bytes free); it was stopped after the unrelated fixture assertions and is recorded as an environment limitation, not passing evidence.

### Verdict and handoff

Verdict: confirmed and fixed. The old exception boundary could strand a partially migrated main wallet and an auxiliary wallet. The change is limited to error translation in `DoMigration()` and does not alter successful migration format, descriptor parsing, or the intentional fail-fast behavior inside `MigrateToSQLite()`.

Source/finding commit and journal commit are pending. Before closing the cycle, add a dedicated campaign for exception-versus-error rollback boundaries in migration and persistence code. Its first queue should cover other `throw` sites reached after file creation, database replacement, settings mutation, reload, or auxiliary-wallet registration, with the same fault-injection and restart invariants.

Next queue:

1. Search current migration and recovery helpers for remaining throws that bypass a caller's cleanup or backup-restore contract; exclude the fixed descriptor loops and the intentional `MigrateToSQLite()` assertion unless new build evidence changes the contract.
2. Compare auxiliary-wallet creation, settings registration, reload, and path cleanup across watch-only, solvable, descriptor, and legacy compatibility modes.
3. Preserve the exact fixed and failed fixture commands, then re-rank against non-wallet historical incomplete fixes after the next selector.
