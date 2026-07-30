# Local Reasoning Domain and Relationship Audit

## Cycle 135: index file-position and publication relationships

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `57` (`local-reasoning-domain`); no reroll was needed because this was a new cycle, not a repeated draw within the cycle.
- Worktree: `/data/my_storage/bitcoin`
- Branch: `uber-cycle-135-local-reasoning-domain-20260730`
- HEAD at cycle start: `a0df83a228482ecca561dd223cd62520cc3dd804`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `origin/master...HEAD = 40 1056`.
- The fresh gate passed: origin fetch, catalog/protocol hash checks, tracked-worktree cleanliness, and `git diff --check`. PID `777094` (the unrelated wallet test) remained untouched. All unrelated untracked artifacts were preserved and excluded from cycle commits.

### Scope and hypothesis

This cycle selected a new local relationship cell after excluding the prior AddrMan classifier, BaseIndex callback-publication, transaction-download ownership, Taproot key-identity, and wallet replacement-state findings. The primary hypothesis was that `BlockFilterIndex` could let its physical filter-file cursor, per-height `DBVal` entries, `DB_FILTER_POS`, and `DB_BEST_BLOCK` describe different logical tips after a write, reorg, failed commit, or restart. The secondary control was the analogous CoinStats relationship between its per-height `DBVal`, in-memory MuHash/counters, `DB_MUHASH`, and `DB_BEST_BLOCK`.

The required invariant was: a committed index tip must have a readable, hash-checked filter or CoinStats row; the cursor/state committed for that tip must describe the next append position or the same UTXO set; and an interrupted or failed update must be replayable from the last committed best block without publishing an invalid relationship.

### Source and history trace

- `BlockFilterIndex::WriteFilterToDisk()` writes the encoded block hash and filter at the current `FlatFilePos`, only advances `m_next_filter_pos` after a successful close, and records the row's old position before the advance (`src/index/blockfilterindex.cpp:194-249,276-291`). File rotation truncates and commits the old file before moving to the next file.
- `BlockFilterIndex::CustomCommit()` commits and closes the current filter file before adding `DB_FILTER_POS` to the same batch as `BaseIndex::DB_BEST_BLOCK` (`src/index/blockfilterindex.cpp:136-158`, `src/index/base.cpp:286-308`). A failed commit therefore does not publish the cursor or best-block locator; stale physical bytes are not referenced by the old database cursor and are overwritten or bypassed on replay.
- Reorg removal copies the disconnected height row to the hash index and persists the current cursor before changing the cached previous header (`src/index/blockfilterindex.cpp:293-313`). The unchanged height row is intentionally retained until the replacement chain overwrites that height, while hash lookup preserves the stale branch filter.
- CoinStats appends update the in-memory counters and MuHash, persist a height row, and deliberately defer `DB_MUHASH` until `CustomCommit()` batches it with `DB_BEST_BLOCK` (`src/index/coinstatsindex.cpp:200-213,262-313`). On restart, `CustomInit()` verifies the stored MuHash against the row selected by the committed block reference before restoring the counters.
- History explicitly documents and tests the relevant boundaries: `3679f1ecf5` prevents commits ahead of flushed chainstate, `33fe1e3282` introduced the batched index-write design in its historical branch, and `80a1947178` added explicit write-file close checks. The current tree's `baseindex_no_commit_ahead_of_flush`, block-filter reorg/crash tests, and CoinStats unclean-shutdown test cover the publication ordering.

### Verification

The corrected focused command was:

    mkdir -p /data/my_storage/tmp/cycle135-test-tmp
    TMPDIR=/data/my_storage/tmp/cycle135-test-tmp /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=baseindex_tests,blockfilter_index_tests,coinstatsindex_tests --log_level=test_suite

It ran 8 cases and exited 0 with `*** No errors detected`. This included `baseindex_no_commit_ahead_of_flush`, block-filter initial sync, initialization/destruction, null-reference rejection, reader reinitialization, reorg-crash handling, CoinStats initial sync, and CoinStats unclean shutdown/reload. The isolated test process used the current cycle-134-built binary; no production source changed during this cycle.

The first attempt supplied a non-existent `TMPDIR`, causing a filesystem `temp_directory_path` failure before the fixtures ran; it was terminated while unwinding after the setup abort. That command failure is recorded as an environment/setup issue, not product evidence. The rerun created the scratch directory and passed. A second independent check of the source/history relationship found no reachable interleaving that could invalidate the two `BaseIndex::Commit()` tip loads: validation callbacks for one subscriber are serialized, and the sync thread exits after its final commit before steady-state callbacks publish later updates.

### Verdict and handoff

Dismissed. No source, test, or documentation defect was confirmed. The apparent cursor/row/locator mismatches are intentional recovery stages: uncommitted rows and stale file bytes are replayable from the old best-block locator, while committed cursor and MuHash state are published with the corresponding best-block state. No permanent test or source change was justified. Remaining open cells are cross-index database failure injection, physical filter-file corruption detection beyond existing checks, and concurrent index restart under a separately instrumented sanitizer run; those are distinct from this cycle's closed relationship cell.

## Cycle 65: AddrMan network classification relationship

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `57`
- Selected goal: `local-reasoning-domain`
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- HEAD at cycle start: `d7109ee6a310bbfeac419e3f0833910ee2454570`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence: `origin/master...HEAD = 2 901`
- Catalog/protocol/TSV hashes matched the authoritative values.
- Tracked and staged state was clean; only known agent-owned untracked artifacts were present. No relevant process was running.

### Scope and hypothesis

Audit local reasoning domains where related APIs use different address relationships. The selected candidate is AddrMan's network-filter path: `m_network_counts` and `Select_` use `CNetAddr::GetNetwork()`, while `GetAddr_` and the public `Select` contract use `CNetAddr::GetNetClass()`.

The falsifiable hypothesis is that a valid, routable linked-IPv4 address, such as RFC3964 6to4 or RFC6052 IPv4-embedded IPv6, can be counted as `NET_IPV6` but returned as `NET_IPV4`. If so, `Size`, `Select`, `GetAddr`, and their public contracts disagree for a reachable address domain. The trust boundary is a caller requesting peers by network; no unsupported internal object state is needed.

### Evidence plan

1. Confirm the classification difference for each supported linked-IPv4 representation and trace the history that introduced network-filtering and per-network counts.
2. Add a disposable focused unit assertion for a valid linked-IPv4 address in AddrMan, covering all relevant table states and both network filters.
3. Run the old-source control and a temporary candidate repair independently. Require a failing-before contract assertion and a passing-after result, then run the full AddrMan suite and sanitizer/fuzz smoke where available.
4. Check all callers and review precedent before deciding whether the correct repair is to use `GetNetClass()` consistently or to document an intentional distinction. Remove disposable scaffolding unless the regression is justified for retention.

### Initial history evidence

- `CNetAddr::GetNetClass()` intentionally maps routable IPv4, RFC6145, RFC6052, RFC3964, and RFC4380 forms to `NET_IPV4` through `HasLinkedIPv4()`.
- `AddrMan::GetAddr()` already filters with `GetNetClass()` and its public postcondition asserts the same classification.
- The per-network-count change (`d35595a78a`) and network-selected `Select` change (`6b229284fd`) count/filter with `GetNetwork()`.
- The multi-network change (`829becd990`) preserved that `GetNetwork()` filter, while later contract checks added a `GetNetClass()` postcondition. This is the primary suspected cross-layer drift.

### Contract result

The suspected product-wide classifier mismatch is intentional and must remain split:

- `Size(network)` and `Select(network)` use `GetNetwork()`, the transport-level network used by reachable-network and `-onlynet` logic.
- `GetAddr(network)` uses `GetNetClass()`, the legacy public address-list classification. A linked IPv4 address is therefore included by `GetAddr(NET_IPV4)` even though its transport network is `NET_IPV6`.

The current-branch defect was the postcondition added by the earlier AddrMan contract campaign: `AddrManImpl::Select()` asserted `GetNetClass()` against a selection performed with `GetNetwork()`. The same wrong relationship was duplicated in the AddrMan deterministic test oracle and fuzzer oracle.

### Before and after evidence

The disposable `addrman_linked_ipv4_network_contracts` test exercised four valid, routable forms: RFC6145, RFC6052, RFC3964, and RFC4380. Each had `GetNetwork()==NET_IPV6` and `GetNetClass()==NET_IPV4`.

- Clean pre-fix build: the test first showed `Size(NET_IPV4)==0`, `Size(NET_IPV6)==4`, and `Select({NET_IPV4})` empty while `GetAddr(NET_IPV4)` returned all four. Calling `Select({NET_IPV6})` then aborted at `src/addrman.cpp:1208` because the postcondition required `GetNetClass()==NET_IPV6`.
- Repair: changed only the `Select` postcondition and the matching deterministic/fuzzer oracles from `GetNetClass()` to `GetNetwork()`. The focused test passed 29 assertions; the full AddrMan suite passed 28 cases and 14,346 assertions.
- Mutation: temporarily restored the old `GetNetClass()` postcondition. The focused test again aborted at `src/addrman.cpp:1208` with exit 134. The mutation was restored.

The normal libFuzzer `addrman` corpus replay was independently attempted with `-runs=1000 -seed=6501`; it stopped at execution 117 on an existing `AssertSerializationRoundTrip` assertion at `src/test/fuzz/addrman.cpp:206`, before this classifier path was established. That result is preserved as a separate fuzz-oracle limitation, not attributed to this fix. A companion `addrman_serdeser` replay with `-runs=500 -seed=6502` reached 732 executions at about one execution per second without a diagnostic, then was interrupted after roughly ten minutes at the execution boundary; libFuzzer reported 1,837 MiB peak RSS. An empty-corpus `addrman` smoke with `-runs=100 -seed=6503` completed cleanly, adding three units at about 50 executions per second with 1,850 MiB peak RSS. The interrupted corpus replay is inconclusive only for that large corpus, while the focused unit, full AddrMan suite, mutation, and empty-corpus smoke provide the selected-path evidence.

### Status

Confirmed and repaired in the current branch by `b2d858ae4e` (`addrman: match Select contract to transport network`). The production behavior remains transport-network based; only the invalid postcondition and duplicated test/fuzzer relationship were corrected. The focused and full unit builds, fuzz-target rebuild, mutation control, and clean empty-corpus smoke passed. The large corpus replay was stopped at a documented resource boundary, and its unrelated serialization assertion is retained as a limitation.

## Cycle 77: relationship and state-domain follow-up

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `57`
- Selected goal: `local-reasoning-domain`
- Worktree: `/data/my_storage/bitcoin`
- Branch: `uber-cycle-77-local-reasoning-domain-20260728`
- HEAD at cycle start: `76a22401689d56e337474da9003114b013e6fbd6`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence: `origin/master...HEAD = 2 933`
- Catalog/protocol/TSV hashes matched the authoritative values.
- Tracked state was clean; known agent-owned untracked artifacts and `test/cache/` were preserved. No relevant process was running.

### Scope and exclusions

This is a fresh relationship-domain cycle, not a reopening of cycle 65. Exclude the already fixed AddrMan `GetNetwork()` versus `GetNetClass()` relationship, linked-IPv4 classification, `Size()`, `Select()`, `GetAddr()`, and their duplicate test/fuzzer oracles unless a new caller or regression signal changes the contract. Also exclude cycle 48's wallet-rescan reservation ordering and cycle 76's compact-block read-failure publication path.

Inventory local assumptions where a helper's result is combined with a related value from another object, lifecycle, lock, queue, or namespace. Prioritize relationships that cross a trust boundary or determine peer/accounting state, persistence, wallet ownership, consensus selection, or cryptographic validity. Do not treat naming symmetry as proof: state the mathematical or state-machine relationship before testing it.

### Initial hypothesis queue

1. P2P connection and peer-accounting helpers may compare a transport/network identity from one representation against a policy/permission identity from another, causing valid peers to be omitted, miscounted, or retained after lifecycle transitions.
2. Chainstate, index, or mempool selection helpers may assume that a returned object belongs to the same active snapshot/transaction view as the metadata used to validate or publish it.
3. Wallet/descriptor helpers may pair an object identifier with a key, script, or ownership domain that is equivalent only after normalization, creating a false negative or stale state.

### Falsifiable cycle protocol

For each candidate, record the local domain, related values, invariant, caller path, and failure consequence. Compare implementation, callers, tests, documentation, blame, and historical rationale. Build a deterministic fixture or model that exercises the smallest valid and invalid relationship pairs. Require a failing-before contract oracle, a restored passing result, and an independent mutation, alternate implementation, sanitizer trace, or exhaustive boundary check before changing production code. Dismiss candidates where the apparent mismatch is an intentional domain boundary and preserve the exact reason and next unchecked cell.

Status: active; no source finding claimed yet.

### Cycle 77 candidate review and evidence

#### Candidate 1: P2P transport identity versus public network class

This was used as a scope guard rather than a reopening of cycle 65. `CNode::ConnectedThroughNetwork()` intentionally reports the peer's connected network, including the inbound-onion override, while `m_network_conn_counts` and `MultipleManualOrFullOutboundConns()` use `CNetAddr::GetNetwork()` for transport-level outbound-slot accounting. The current comments in `src/net.h`, the linked-IPv4 AddrMan regression from cycle 65, and the callers of both helpers support the split. No new caller was found that combines the two identities without an explicit privacy or transport purpose. The cycle-77 `net_tests` run passed 32 cases and 132,878 assertions.

Verdict: dismissed as an intentional domain boundary; the linked-IPv4 and AddrMan cell remains closed.

#### Candidate 2: BaseIndex commit snapshot versus flushed chainstate

The apparent relationship gap is the second load of `m_best_block_index` in `BaseIndex::Commit()`: the first load is checked against `GetLastFlushedBlock()`, while the later load supplies the locator hash. The relevant invariant is that `CustomCommit()` and locator publication must describe the same index tip and must not be ahead of durable chainstate.

The caller/lifecycle trace does not make the two loads independently mutable in supported operation. `BaseIndex::Sync()` keeps `m_synced` false until its final commit and the locked tip check complete. After synchronization, `BlockConnected()` and `ChainStateFlushed()` are delivered through the per-subscriber validation queue; `CValidationInterface` documents that each callback completes before the next callback for that subscriber, and `ValidationSignalsImpl::Iterate()` enforces that serialization. `CustomCommit()` implementations do not update `m_best_block_index`; `BlockConnected()` publishes it only after processing, and `ChainStateFlushed()` checks the locator is on the current best chain before calling `Commit()`.

The isolated `baseindex_tests,blockfilterindex_tests,coinstatsindex_tests` run passed 3 cases and 26 assertions. No production-reachable interleaving can invalidate the first tip check without violating the callback contract, so adding a lock or replacing the second load would be speculative and could obscure the existing lifecycle invariant.

Verdict: dismissed; retain as a future candidate only if index commits become callable concurrently or a new callback path bypasses the validation queue.

#### Candidate 3: transaction-download peer ownership and orphan state

The relationship under test is `(transaction hash, peer)` ownership across `TxRequestTracker`, `TxOrphanage`, and `TxDownloadManagerImpl`. `ReceivedNotFound()` is called from the `NOTFOUND` message path while the `CNode` is still live; stale responses are harmless because `ReceivedResponse()` searches the exact `(peer, hash)` pair and does nothing when it no longer exists. `ReceivedTx()` explicitly handles a missing peer before touching request state. On teardown, `DisconnectedPeer()` removes orphan announcements and request-tracker entries before erasing the peer record, and `CheckIsEmpty()` verifies all per-peer and global counters.

The focused `txdownload_tests` run passed 14 cases and 605 assertions. The full `net_tests` run passed 32 cases and 132,878 assertions. Existing duplicate-connect, disconnected-orphan, multi-announcer, and multi-entry-NOTFOUND controls cover the valid/invalid state transitions; no stale peer-to-transaction relationship was reproduced.

Verdict: dismissed; no source change justified.

#### Candidate 4: descriptor spelling, x-only script identity, and private-key ownership

The downstream relationship after the cycle-75 Taproot fix was checked independently: a full compressed key in a Taproot descriptor retains its textual parity spelling, script construction normalizes it to x-only bytes, and private-key lookup uses `SigningProvider::GetKeyByXOnly()`, which checks both parity-derived `CKeyID` values. `ExpandPrivate()`, wallet signing-provider assembly, and backup/migration consumers all use the resulting full public-key identity for key storage while Taproot signing uses x-only lookup. No consumer was found that requires the descriptor's full-key spelling to equal the script's x-only identity.

The isolated `descriptor_tests` run passed 13 cases and 30,437 assertions, including the compressed-key, opposite-parity, NUMS, nested `multi_a`, and Miniscript controls. This confirms the downstream relationship but does not reopen or modify the cycle-75 source cell.

Verdict: dismissed as a verified continuation of the already fixed Taproot identity boundary.

#### Cycle result

No new source, test, or documentation defect was confirmed. `git diff --check` passed. The first combined test invocation failed because its explicitly supplied `TMPDIR` directory had not been created and the multi-suite invocation then hit duplicate global argument registration; this was a command setup error, not a product result. The isolated reruns used `/data/my_storage/tmp/local-reasoning-cycle77-tests/{net,txdownload,descriptor,index}` and passed. No relevant process remained running. The next unchecked relationship cell should come from a fresh catalog draw and should not reopen AddrMan linked-IPv4, BaseIndex callback serialization, txdownload peer cleanup, or the cycle-75 Taproot key-lookup fix without new evidence.

Status: dismissed; journal-only close, no source change justified.

## Cycle 97: local-reasoning domain and relationship audit

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `57`
- Selected goal: `local-reasoning-domain`
- Worktree: `/data/my_storage/bitcoin`
- Branch: `uber-cycle-97-local-reasoning-domain-20260729`
- HEAD at cycle start: `9031d02ee52aff27ca3fc7636ee8d0ce7923dc7a`
- `origin/master`: `9b38d077f894d27ea76413b1db1cb040e25dc296`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence: `origin/master...HEAD = 29 984`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; no relevant Bitcoin or libsecp test process was running. Known untracked artifacts remain preserved and excluded from cycle commits.

### Scope and exclusions

Challenge local assumptions that combine values from different objects, snapshots, namespaces, ownership domains, locks, or lifecycle stages. State the expected relationship before testing it, then trace every caller and state transition. This is a fresh cycle, not a reopening of Cycle 65 or Cycle 77: exclude AddrMan `GetNetwork()` versus `GetNetClass()`, linked-IPv4 classification, BaseIndex callback serialization, transaction-download peer cleanup, and Taproot x-only ownership unless new independent evidence changes the contract.

Initial queue:

1. Persistence and index relationships: database cursor position versus decoded key/value domain, chainstate flush tip versus index publication, and block/undo file position versus metadata ownership.
2. Wallet and descriptor relationships: destination/key ownership, descriptor normalization versus lookup identity, transaction state versus durable record state, and keypool reservation versus address publication.
3. P2P and mempool relationships: peer identity versus request ownership, package graph versus accounting snapshot, and permission/relay state versus queue cleanup.
4. Cross-module API relationships: caller-provided context/capability versus helper assumptions, output lifetime versus owner, and optional-module state versus public result.

For each candidate require a deterministic minimal fixture or a rigorous call/dataflow proof. Do not infer a defect from two different names alone: distinguish an intentional domain boundary from a missing precondition, stale snapshot, or publication-order error. Search earlier journals, history, tests, and review discussion before reporting a candidate.

### Evidence ledger

#### Candidate: wallet replacement metadata was published before a failed write

The wallet queue's transaction-state cell was selected after excluding the earlier
passphrase, descriptor, address-book, and `SetAddressPreviouslySpent` write-failure
fixes. The local relationship is `(CWalletTx::m_replaced_by_txid,
CWalletTx::m_state) <-> DBKeys::TX`: the two in-memory fields must describe the
same durable transaction row after `MarkReplaced` returns. A failed `WriteTx`
must therefore leave both fields at their pre-call values.

The source-to-sink trace is concrete:

- `src/wallet/wallet.cpp:998-1031` takes `cs_wallet`, finds the existing wallet
  transaction, writes `m_replaced_by_txid`, calls `RefreshMempoolStatus`, and
  then persists the entire `CWalletTx` with `WalletBatch::WriteTx`.
- `src/wallet/feebumper.cpp:371-380` first commits and broadcasts the new bumpfee
  transaction, then calls `MarkReplaced`. A failed marker write is reported in
  `errors` but the replacement creation has already succeeded.
- `src/wallet/feebumper.cpp:44-47` refuses another bump when the in-memory marker
  is present; `src/wallet/spend.cpp:381-395` uses replacement metadata when
  deciding whether an unconfirmed input is safe; and
  `src/wallet/rpc/transactions.cpp:65-69` exposes the marker. The value is also
  serialized in `src/wallet/transaction.h:267-325`.

Before the fix, `MarkReplaced` changed the marker and potentially changed
`m_state` from `TxStateInMempool` to `TxStateInactive` before `WriteTx`. The error
branch only set `success = false`, so a database failure returned false while
leaving the caller-visible object changed. The current branch did not already
cover this path: the earlier `98d5cdae66` conversion made replacement fields
explicit members but retained the mutation-before-write ordering. Recent
`55eaf087c1`, `6e67919fa6`, `8b9e10c544`, `21f215670b`, and `600afa9599` fixes
establish the repository's write-before-publication/rollback precedent for
wallet metadata; no earlier journal entry covered `MarkReplaced`.

#### Independent before/after proof

The regression test creates a mock SQLite wallet, stores a transaction in
`TxStateInMempool`, and installs a transaction-row `BEFORE INSERT` trigger that
raises `SQLITE_ABORT` for the exact serialized `DBKeys::TX` key. It then calls
`MarkReplaced` and checks that the call fails, `m_replaced_by_txid` is empty, and
the prior `TxStateInMempool` state is retained.

For the old-source control, the rollback lines were temporarily removed from
`src/wallet/wallet.cpp` in the working tree, the same target was rebuilt, and
this command was run:

    TMPDIR=/data/my_storage/tmp/cycle97-markreplaced-control \
      /data/my_storage/tmp/cycle93-build/bin/test_bitcoin \
      --run_test=wallet_tests/replaced_write_failure_preserves_state \
      --log_level=test_suite

It exited 201 with one failure at `wallet_tests.cpp:251`: the assertion that
`m_replaced_by_txid` remained empty failed. The rollback patch was immediately
restored with `apply_patch`; the temporary control was never committed.

The repaired code snapshots `m_state`, performs the existing in-memory update,
and on `WriteTx` failure resets `m_replaced_by_txid` and restores the snapshot.
The focused after command exited 0 with `*** No errors detected`. The related
wallet run

    TMPDIR=/data/my_storage/tmp/cycle97-markreplaced-control \
      /data/my_storage/tmp/cycle93-build/bin/test_bitcoin \
      --run_test=wallet_tests,feebumper_tests,wallet_transaction_tests,walletdb_tests \
      --log_level=message

passed 22 cases. The complete unit command

    TMPDIR=/data/my_storage/tmp/cycle97-markreplaced-control \
      /data/my_storage/tmp/cycle93-build/bin/test_bitcoin --log_level=message

passed all 1,210 test cases and exited 0. `CCACHE_DISABLE=1 cmake --build
/data/my_storage/tmp/cycle93-build --target test_bitcoin -j4` passed after both
the source and test changes, and `git diff --check` passed.

#### Verdict and limits

Confirmed and fixed. The smallest repair is local to `MarkReplaced`; it preserves
the existing error return and notification behavior while preventing a failed
transaction-row write from publishing replacement metadata or a refreshed
mempool state. The test proves a real SQLite write failure and an independent
source-to-sink relationship, but it does not exercise Berkeley DB, an on-disk
restart after a crash, or the full RPC bumpfee path. Those are not required to
establish this in-memory publication-order defect. The full suite's intentional
filesystem-error diagnostics and the existing unset script-assets data warning
were non-failures.

Status: confirmed and repaired in this cycle. Commit the source, regression
test, and this journal entry together. After the finding commit, update the
uber-goal state with the commit id and fresh-gate result, then select a new
catalog draw. Do not reopen AddrMan linked-IPv4, BaseIndex callback ordering,
transaction-download peer cleanup, Taproot key identity, or this
`MarkReplaced` write-failure cell without new evidence.
