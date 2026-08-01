# Local Reasoning Domain and Relationship Audit

## Cycle 180 start: cross-index persistence and restart relationships

### Fresh selection and gate

- The exact post-Cycle-179 selector was `shuf -i 0-98 -n 1` -> `70`
  (`compiler-optimization-differential`), but Goal 70's current cell is
  explicitly closed by Cycle 105. The required exact reroll
  `shuf -i 0-98 -n 1` -> `57` selected this goal. Cycle 174's Goal 57 work
  left distinct cross-index persistence-failure and concurrent-restart cells
  open, so this is a permitted new relationship scope rather than a repeat
  of its `Chain::hasBlocks` fix.
- Branch: `uber-cycle-180-local-reasoning-domain-20260731`.
- Start HEAD: `205803b23c8846666feeeb4fb0cd556634b53d00`; origin/master:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence:
  `origin/master...HEAD = 42 1147`.
- Fresh-gate hashes were catalog
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  prompt
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  corrected TSV
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`,
  protocol
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc`, and
  start state
  `046d7176ae529feb4499c3f5ebcbe98dd3dfa9af5d977fe9a6e5742fca80f257`.
  The TSV schema check reported 99 records with IDs 0 through 98. Tracked
  and staged state was clean; known untracked artifacts were preserved. The
  root filesystem had about 99 MiB free and `/data` about 49 GiB free.
  PIDs `777094` and `956381` are unrelated long-running tests and remain
  preserved.

### Scope and exclusions

Do not reopen Cycle 65's AddrMan `GetNetwork()` versus `GetNetClass()` or
linked-IPv4 relationship, Cycle 77's BaseIndex callback serialization and
transaction-download ownership, Cycle 97's wallet replacement rollback,
Cycle 135's index file-position/publication relationship, or Cycle 174's
`Chain::hasBlocks` empty-range fix. Also exclude Cycle 179's empty-`addnode`
parser boundary. A candidate must use a different object pair, backend,
failure point, or lifecycle transition with independent evidence.

Initial queue:

1. Cross-index durable state: compare `BaseIndex` best-block publication,
   per-index database rows, chainstate flush state, and restart/recovery when
   two index implementations observe the same block transition.
2. Database failure symmetry: inject a write, batch, flush, or reopen failure
   at one index boundary and check whether sibling indexes, their in-memory
   snapshots, and public query results remain mutually consistent.
3. Physical filter-file corruption and concurrent index restart, but only if
   the current checks and serialized callback model leave a reachable gap.

For each candidate define the cross-object invariant and exact valid domain,
then trace callers, locking/serialization, commit ordering, database and
filesystem boundaries, history, tests, and recovery code. Require a
deterministic failure/restart fixture, an independently verifiable
failing-before oracle, and a repaired or invariant-preserving control before
changing production code. Keep all scratch datadirs and databases under
`/data/my_storage/tmp`.

## Cycle 180 finding: stale synced flag during index reinitialization

### Candidate, contract, and caller trace

The reinitialization relationship is `(validation registration, m_synced,
m_best_block_index, subclass state)`. A stopped or reinitializing index must
not advertise itself as synchronized until `CustomInit()` has restored its
database-backed state and `Init()` has completed. If it reports false, the
public caller may return an index-unavailable response; it must not block on a
partially initialized index or classify a failed lookup as index corruption.

The current `BaseIndex::Stop()` unregisters the validation interface and joins
the sync thread but leaves `m_synced` unchanged. On restart, `Init()` reads the
persisted best-block locator, holds `cs_main` while it calls `CustomInit()`,
and only assigns the new `m_synced` value after `CustomInit()` returns. Thus a
previously synced index can remain visibly synced while its subclass state is
being reconstructed. The snapshot completion callback in `src/init.cpp`
explicitly performs `Interrupt()`, `Stop()`, `Init()`, and
`StartBackgroundSync()` while RPC/REST services remain available. Relevant
readers include the block-filter REST endpoints, `gettxoutsetinfo`, the
mempool spender lookup, raw-transaction/PSBT/txout-proof paths, and the index
readiness RPC summary.

This is distinct from `bc3db5ef52` (Cycle 174's earlier restart audit), which
published `m_chainstate` under `cs_main` and protected block-file opens but did
not reset the lifecycle flag. The existing index reinit reader stress tests
only exercised data races and did not force a reader to cross the
`CustomInit()` readiness boundary.

### Independent pre-fix reproduction

A disposable `ReinitGateIndex` test subclass persisted a best block at height
100, stopped a synced index, and blocked the second `CustomInit()` while it
held `cs_main`. A concurrent reader called
`BlockUntilSyncedToCurrentChain()`. With the production change absent, the
reader could not complete during reinitialization because the stale true flag
made it wait for `cs_main`; after the gate was released, it returned true even
though the read began before initialization completed. The bounded test then
failed both contract checks: `query_completed_during_init` and
`!query_result.load()`.

The final pre-fix command was:

    TMPDIR=/data/my_storage/tmp/cycle180-test-tmp /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=baseindex_tests/baseindex_reinit_not_synced_during_custom_init --catch_system_errors=no --color_output=false --log_level=test_suite --report_level=short

It exited 201 with the two expected assertion failures. An earlier direct
call was intentionally discarded as a harness setup result: it blocked on
the same `cs_main` lock and was replaced by the bounded reader control.

### Repair and verification

`BaseIndex::Init()` now clears `m_synced` before registration, database
loading, and subclass initialization. `BaseIndex::Stop()` clears it before
unregistration and again after the sync-thread join, covering both the
stop/restart interval and a worker that exits while stopping. The regression
test asserts false after `Stop()` and checks that a concurrent readiness call
completes false while gated `CustomInit()` is still active.

The source/test diff was checked with `git diff --check` and built with:

    CCACHE_DIR=/data/my_storage/tmp/cycle180-ccache TMPDIR=/data/my_storage/tmp/cycle180-build-tmp cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j2

Normal focused suites passed: `baseindex_tests` 2 cases/17 assertions,
`coinstatsindex_tests` 2/14, `txindex_tests` 3/129,
`txospenderindex_tests` 3/1,086, and `blockfilter_index_tests` 5/1,851.
The current Clang 19 TSan build passed the new gated test with 5 assertions
and the existing `txindex_reinit_reader_race`,
`txospenderindex_reinit_reader_race`, and
`blockfilter_index_tests/index_reinit_reader_race` controls with 2, 3, and 2
assertions respectively, with no TSan diagnostic. The Clang 19 UBSan build
passed the new test and all five complete index suites with the same assertion
counts and no sanitizer diagnostic. The full normal unit run used seed 180:
1,232 cases passed, one existing filesystem-injection warning was reported,
and all 27,115,698 assertions passed.

### Verdict and limits

Confirmed and fixed as a local lifecycle/readiness availability defect. The
trust boundary is authorized local snapshot/index restart activity plus
concurrent RPC/REST callers; no unauthenticated network trigger, consensus
effect, wallet/key loss, or persisted-index corruption was demonstrated. The
test models the production callback's gated subclass initialization rather
than running a full assumeutxo download/restart integration. The remaining
distinct Goal 57 cells are cross-index database failure injection and
physical filter-file corruption beyond the existing block-hash/checksum
checks. The source, regression test, and this evidence record are ready for
one self-contained commit.

## Cycle 174 start: cross-domain lifecycle and snapshot relationships

### Fresh selection and gate

- The exact post-Cycle-173 selector was `shuf -i 0-98 -n 1` -> `57`
  (`local-reasoning-domain`). No reroll was needed: the prior goal-57 cells
  are closed by scope, but this journal records remaining open relationship
  cells rather than exhausting the goal.
- Branch: `uber-cycle-174-local-reasoning-domain-20260730`.
- Start HEAD: `607fb909086c54abb17244996dd34e706e301b68`; origin/master:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `42 1129`.
- The fresh gate passed `git fetch origin master`, tracked/index diff checks,
  the four input-hash checks, and the persistent-process check. PIDs `777094`
  and `956381` remain unrelated long-running tests and must not be stopped;
  known untracked artifacts remain outside this cycle's commits.

### Exclusions and initial queue

Do not reopen Cycle 65's AddrMan `GetNetwork()` versus `GetNetClass()` and
linked-IPv4 classification, Cycle 77's BaseIndex callback serialization and
transaction-download peer cleanup, Cycle 97's wallet `MarkReplaced` rollback,
Cycle 135's index file-position/publication relationships, or Cycle 173's
wallet best-block corruption handling. A new candidate needs a different
object pair, caller, backend, or lifecycle transition with independent proof.

Mine relationships where values from different objects, snapshots, namespaces,
locks, queues, or lifecycle stages are combined. Prioritize current code touched
by recent history and cross-layer state that can publish stale, mismatched, or
unowned data. Initial queue:

1. Chainstate, block storage, and validation relationships between active tip,
   flushed tip, block-index metadata, undo/block-file positions, and restart
   state, excluding the already-reviewed index publication path.
2. Mempool and P2P relationships between peer/request ownership, transaction
   graph metadata, package accounting, permissions, and removal/eviction state.
3. Wallet, descriptor, and RPC relationships between normalized identifiers,
   object lifetime, public result state, and durable records, excluding the
   recent migration and replacement-write cells.
4. Kernel, IPC, and optional-module relationships between callback context,
   capability/lifetime, output ownership, and feature-disabled behavior.

For every candidate state the exact domain and invariant first, then trace
callers, history, tests, docs, locks, and failure transitions. Require a
deterministic fixture or rigorous dataflow proof, independent verification, and
a failing-before/passing-after oracle before changing production code.

## Cycle 174 finding: `Chain::hasBlocks` empty-range boundary

### Candidate and contract

The chain-interface queue produced a distinct boundary candidate: a caller can
ask whether the ancestors of a block are present in an inclusive height range
that starts above that block. For `block_hash = active[4]`, `min_height = 10`,
and `max_height = 20`, the requested intersection is empty. The `hasBlocks`
contract in `src/interfaces/chain.h:183-186` says that all ancestors in the
specified range must have data; an empty intersection therefore satisfies the
universal condition. A missing `BLOCK_HAVE_DATA` bit below the range must not
change the result. The trust boundary is a direct Chain-interface, IPC, or
future partial caller; no malformed internal block index is required.

### Source, history, and caller trace

Before the fix, `ChainImpl::hasBlocks` entered its status loop before checking
whether the current block was below `min_height`. It therefore returned false
for a missing starting block even when no block in the requested range existed.
The implementation was introduced by `2a26771d81`; history and blame showed no
prior fix for this boundary. The nearby implementation comment promises that a
`min_height` that is too low will not change the result, but the public header
does not impose a `min_height <= block_hash height` precondition. The normal
wallet `rescanblockchain` caller (`src/wallet/rpc/transactions.cpp:883-897`)
validates nonnegative, ordered bounds and caps them at the wallet tip, so this
is an API correctness/robustness issue for direct and partial callers rather
than a demonstrated wallet or consensus failure.

The related settings candidate was dismissed: `updateRwSetting` reports a
failed settings-file write after mutating the in-memory map, but restoring a
snapshot would race with another update and there is no rollback contract.
The block-storage candidate was a duplicate of the prior unlinked-block
publication finding (`8e40da2f31` and upstream `0e4b0bacecf`/`fb47793b99`).
Mempool/P2P and optional-interface scans found no new relationship defect;
their reviewed cells remain closed.

### Independent reproduction and fix

The regression was added to `interfaces_tests/hasBlocks` by clearing the
`BLOCK_HAVE_DATA` bit in `active[4]->nStatus` and asserting
`chain->hasBlocks(active[4]->GetBlockHash(), 10, 20)`. With the test-only
regression and the old production implementation, this exact control was run:

    mkdir -p /data/my_storage/tmp/cycle174-hasblocks-old
    TMPDIR=/data/my_storage/tmp/cycle174-hasblocks-old /data/my_storage/tmp/cycle170-mempool-build/bin/test_bitcoin --run_test=interfaces_tests/hasBlocks --catch_system_errors=no --color_output=false --log_level=test_suite --report_level=short

It exited 201 with one failure at `test/interfaces_tests.cpp:162` and
23/24 assertions passed. The minimal production repair first clamps an
optional `max_height`, returns false if that clamp produces no ancestor, then
returns true when the clamped block is already below `min_height`, before
inspecting `BLOCK_HAVE_DATA`. The inclusive lower-bound behavior remains in the
existing loop, so a block exactly at `min_height` is still required to have
data.

The repaired release build was rebuilt with:

    CCACHE_DISABLE=1 cmake --build /data/my_storage/tmp/cycle170-mempool-build --target test_bitcoin -j4

The focused test then passed 1 case and 24 assertions. The broader interface
suite passed 6 cases and 65 assertions. An independent UBSan build was rebuilt
with:

    CCACHE_DISABLE=1 cmake --build /data/my_storage/tmp/cycle106-clang19-ubsan --target test_bitcoin -j4

The UBSan focused replay used `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`
and passed 1 case and 24 assertions with no diagnostic. The UBSan build emitted
only its known `object-size sanitizer has no effect at -O0` warning.

### Verdict and handoff

Confirmed and fixed as one self-contained source/test/journal finding. The
change is low severity and affects an under-specified partial interface domain;
no consensus, normal wallet, persistence, or secret-data path was shown to be
reachable. The exact source/test diff is ready for commit. Remaining work in
this cycle is commit verification, a state-only close record, the fresh gate,
and a new random goal draw.

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

## Cycle 243: physical filter-file validation during index reinitialization

### Cycle identity and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `57` (`local-reasoning-domain`); no
  reroll. Branch: `uber-cycle-243-local-reasoning-domain-20260731`.
- Cycle-start HEAD: `0b1fc5085ba440e00ae254f8266533fa8c627f6f`; current
  `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence:
  `origin/master...HEAD = 42 1269`.
- The catalog, random prompt, TSV, and protocol hashes matched the durable
  values. The tracked worktree was clean at the cycle start; unrelated
  untracked probes and generated artifacts were preserved. Root storage was
  full, so all build, test, and temporary data stayed under
  `/data/my_storage/tmp`. Protected PIDs `777094`, `956381`, `1138182`,
  `1157959`, `1312049`, and `1312050` remained alive and untouched.

### Scope and contract

Earlier Goal 57 cells closed BaseIndex readiness during restart, the
cursor/row/locator publication ordering, and chainstate/index crash symmetry.
This cycle uses a different persisted relationship: the block-filter DB's
best-block and `DBVal` row versus the flat filter bytes referenced by that row.
The required restart invariant is that a successfully initialized and
synchronized block-filter index can serve the filter for its persisted tip.
The RPC and REST callers explicitly classify a ready index whose filter lookup
fails as unexpected index corruption; initialization should therefore fail
closed when the persisted current filter is missing or unreadable.

`BlockFilterIndex::CustomInit()` previously read `DB_FILTER_POS` and the last
filter header row, but did not read the filter bytes for the persisted tip.
Consequently, deleting `fltr00000.dat` after a durable sync left
`Init()` successful and `BlockUntilSyncedToCurrentChain()` true, while
`LookupFilter()` returned false. Existing lookup-time hash and block-hash
checks detect corruption only after the index has been published as ready.

### Independent reproduction and fix

The temporary probe first used a memory-only DB and was discarded as a fixture
setup result: no best-block locator survived the stop/reinit boundary, so the
post-reinit readiness check correctly returned false. The corrected probe used
an on-disk scratch index, forced the chainstate flush, drained validation
callbacks, removed the current `fltr00000.dat`, stopped, and reinitialized.
With the unmodified source, the probe passed `Init()` and readiness but failed
the filter lookup, establishing the mismatch.

The permanent regression is
`blockfilter_index_tests/blockfilter_index_reinit_rejects_missing_current_filter`.
With a temporary source mutation that removed only the new physical-filter
validation, the exact regression command exited 201 at the `!filter_index.Init()`
assertion. The mutation was restored with `apply_patch` before final builds.

`CustomInit()` now asks `ReadFilterHeader()` to also decode and hash-check the
persisted tip's `DBVal` position through the existing `ReadFilterFromDisk()`.
An absent file, short/corrupt record, block-hash mismatch, or filter-hash
mismatch returns false before the index becomes initialized. The default
header-only path used during reorg removal is unchanged, so this adds no disk
read to steady-state block processing. The repair is one source relationship
check and one durable restart regression; no rebuild or broad scan was added.

### Verification

- Clang 19 Release build, configured in the isolated
  `/data/my_storage/tmp/cycle243-build` with IPC and wallet disabled, rebuilt
  `test_bitcoin` successfully after the source and test changes.
- The repaired focused command passed 1 case and 3 assertions. The temporary
  pre-fix mutation of the same source path failed 1 assertion with exit 201.
- The repaired adjacent matrix passed 17 cases and 3,103 assertions:
  `baseindex_tests` (3/20), `blockfilter_index_tests` (6/1,854),
  `coinstatsindex_tests` (2/14), `txindex_tests` (3/129), and
  `txospenderindex_tests` (3/1,086).
- `git diff --check` passed. No default datadir, wallet, key, or production
  database was used.

### Verdict and limits

Confirmed and fixed as a local persistence/readiness defect. The test covers
the current persisted tip and proves fail-closed restart behavior. It does not
scan every historical filter file, so corruption in an older file can still
surface lazily when that block is queried; a full startup audit would impose a
different performance and recovery policy. It also does not claim a database
engine fault or power-loss-specific root cause. Those remain separate evidence
cells.

Status: confirmed and repaired in this cycle. Commit the source, regression
test, and this journal entry together. After the finding commit, update the
uber-goal state with the commit id and fresh-gate result, then select a new
catalog draw. Do not reopen the closed cursor/row publication or readiness
cells without new backend or restart evidence.
