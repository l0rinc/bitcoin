# Independent Multi-Agent Disagreement and Adjudication

## Cycle 156: null-mempool chainstate deletion and kernel wipe lifecycle

### Gate and scope

- Exact selector: `shuf -i 0-98 -n 1` -> `15`; goal 15 was already closed for the current descriptor-validation cell, so the exact reroll -> `40`.
- Selected goal: `multi-agent-adjudication` (goal 40).
- Worktree branch: `uber-cycle-156-multi-agent-adjudication-20260730`.
- HEAD at cycle start: `6eeda975a0b31d13cd024533d8635d1e768b34b7`; `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `origin/master...HEAD = 1095 42`.
- Catalog, prompt, and goals TSV hashes matched the authoritative values. Tracked/index state was clean at the gate, known unrelated untracked artifacts were preserved, and PID `777094` (wallet test, parent `725042`) was not touched.
- The earlier goal-40 cells for ForceRelay late-recipient state and explicit transaction GETDATA from block-relay-only peers are closed and excluded. This is a new lifecycle surface seeded by upstream commit `a99b27f19209e444e895a0e8721c9166f64d7cee` and merge `6e2962e48c`.

### Falsifiable disagreement

Investigator A treats `ChainstateManager::DeleteChainstate()` as required to accept the nullable mempool contract already used by the kernel API. The current assertion dereferences `prev_chainstate->m_mempool` before checking whether it exists. A's hypothesis is that a kernel manager with an existing AssumeUTXO chainstate and `wipe_chainstate_db=true` can reach the assertion and abort instead of returning a status.

Investigator B treats the null pointer as a possible sign of a larger ownership contract defect. B's hypothesis is that allowing deletion alone might mask a later null dereference, an invalid mempool transfer, or a lock-annotation/API mismatch in the same no-mempool lifecycle. B independently audits every production `m_mempool` use, the chainstate load/delete callers, and the activation lock path before accepting a one-line repair.

The adjudication requires both a pre-fix regression proof and an independent lifecycle/dataflow audit. A source change is justified only if the null state is an intended reachable kernel configuration, the failure is reproducible, and the broader audit finds no additional required repair.

### Investigator A: reachability and pre-fix reproduction

The current branch's `node::LoadChainstate()` initializes the validated chainstate with `options.mempool`. The default `ChainstateLoadOptions::mempool` is null, and the kernel wrapper preserves that default. The same function loads an AssumeUTXO chainstate as `std::make_unique<Chainstate>(nullptr, ...)`. When `options.wipe_chainstate_db` is true, it clears the validated target and calls `DeleteChainstate(*assumeutxo_cs)`. The old deletion code then executed:

```cpp
assert(prev_chainstate->m_mempool->size() == 0);
```

The existing `AddChainstate()` transition already accepts null mempools with `assert(!prev_chainstate.m_mempool || prev_chainstate.m_mempool->size() == 0)`, which establishes the intended ownership invariant: transfer an existing empty pool, or transfer no pool.

For an isolated reproducer, A added the smallest focused unit case `chainstatemanager_delete_chainstate_no_mempool`. It initializes both validated and snapshot chainstates with null mempools, clears the validated target as `LoadChainstate()` does, and calls `DeleteChainstate()`. The pre-fix build used:

```
CCACHE_DIR=/data/my_storage/tmp/cycle156-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j2
/data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=validation_chainstatemanager_tests/chainstatemanager_delete_chainstate_no_mempool --log_level=test_suite
```

The test reached the old assertion and reported a memory access violation at address `0x00000058`, with Boost test exit code `201`. This is a deterministic first-invalid-operation proof on the branch's clean pre-fix source.

### Investigator B: independent null-state and caller audit

B searched all current `m_mempool` uses in `src/validation.cpp` and `src/validation.h`. The only unguarded deletion dereference was the assertion above. The other dereferences occur in `MaybeUpdateMempoolForReorg()`, which returns immediately when the pointer is null, or in activation helpers whose runtime bodies guard lock assertions and mempool operations. `DisconnectTip()` and `ConnectTip()` use conditional lock assertions and conditional mutation. `ActivateBestChainStep()` checks the pointer before the final mempool check.

The apparently suspicious `LOCK(MempoolMutex())` calls are intentional: `MempoolMutex()` returns null for a kernel chainstate without a mempool, and the pointer overload of `UniqueLock` returns without locking when passed null. Therefore the activation path does not require a fake mutex or a hidden dereference. B also checked the `AddChainstate()` swap, `LoadAssumeutxoChainstate()`, `LoadChainstate()`, and `btck_chainstate_manager_create()` call chain. No second null-state defect or invalid ownership transfer was found.

B's remaining dissent is that the compact direct test does not create the snapshot directory or call the complete public kernel wrapper. That is a test-strength limitation, not evidence against the source repair: the source trace proves the production call path, and the existing chainstate-manager suite exercises the surrounding snapshot/restart machinery with normal mempool state.

### Fixed verification

The source was changed to preserve the empty-pool invariant while accepting no pool:

```cpp
assert(!prev_chainstate->m_mempool || prev_chainstate->m_mempool->size() == 0);
```

The same focused case passed after rebuilding. The full suite initially hit the host root filesystem's `59 MiB` free-space limit because test temporary data defaulted to `/tmp`; that was recorded as environment noise, not a code result. Re-running with `TMPDIR=/data/my_storage/tmp/cycle156-test-tmp` passed all 23 `validation_chainstatemanager_tests` cases, including snapshot activation and restart cases. The separate `chainstatemanager`, `chainstatemanager_ibd_exit_after_loading_blocks`, and focused null-mempool cases also passed with that isolated temporary directory. `git diff --check` passed.

### Final adjudication

Final verifier verdict: **confirmed; source fix justified**. Investigator A's crash is reachable from the intended no-mempool kernel configuration and is independently reproduced before the repair. Investigator B's broader audit found no additional required null guard or ownership change, so the smallest correct repair is the guarded assertion plus the focused regression test. The bug is a local availability failure: an operator-triggered kernel reindex/wipe with an existing snapshot could abort during chainstate deletion before the API returned. It is not a consensus or remote network finding.

The upstream PR was used as a seed, not as proof. The branch was independently tested before and after the change. Existing tests missed the issue because they did not combine a null validated mempool, a null AssumeUTXO mempool, and the deletion transition. The source/test commit records the exact mechanism, reachability, and regression; no broader cleanup is warranted.

### Cycle handoff

- Source finding: guard the nullable previous-chainstate mempool assertion and add `chainstatemanager_delete_chainstate_no_mempool`.
- Evidence: pre-fix focused test exit `201` with fault address `0x58`; post-fix focused test and all 23 chainstate-manager tests passed using the cycle-local `TMPDIR`.
- Limitation: the first broad attempt used the full root filesystem and failed before a valid test verdict; the rerun used an isolated data filesystem. No sanitizer result is claimed.
- The selected goal-40 candidate is closed. After the separate close snapshot, perform a fresh gate and exact selector draw; do not reopen this candidate without new callers, history, or regression evidence.

## Cycle 111: explicit transaction GETDATA from block-relay-only peers

### Gate and scope

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `40`
- Selected goal: `multi-agent-adjudication`
- Worktree: `/data/my_storage/bitcoin`
- Branch: `uber-cycle-111-multi-agent-adjudication-20260729`
- HEAD at cycle start: `a51e47bb0c905b5e3664cd949f1846e00bfa6aca`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence: `origin/master...HEAD = 40 1011`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The fresh gate passed fetch, tracked/index cleanliness, `git diff --check`, catalog hashes, and process checks. PID `777094` (`wallet_tests`, parent `725042`) was preserved.

Cycle 66's ForceRelay late-recipient disagreement is closed and excluded. This cycle selects the adjacent but distinct `ProcessGetData` contract for an explicit `GETDATA(tx)` from a block-relay-only connection. The source branch at `src/net_processing.cpp:2869-2873` discards the request when `Peer::GetTxRelay()` is null. The current functional test covered transaction rejection, INV rejection, RPC non-relay, and relay-permission behavior, but did not cover this explicit request. The later upstream commit `278710a88d8` adds exactly this focused assertion, so it is a source/history seed rather than an unquestioned oracle.

### Falsifiable disagreement

Investigator A treats an explicit `GETDATA(tx)` as a stronger per-message request than the peer's `relay=false` announcement preference. Under that interpretation, the node should answer a known transaction or return `NOTFOUND`; silently dropping the request creates an asymmetric API where the peer can ask but receives no protocol response. A's trust boundary is the remote peer's message and the transaction relay policy, with privacy and bandwidth as possible counterarguments.

Investigator B treats `m_tx_relay == nullptr` as the protocol boundary for the whole transaction-relay capability. A block-relay-only peer must not receive transaction data or inventory, and `-blocksonly` explicitly opts out of receiving and relaying transactions except for `relay` permission. Under that interpretation, returning `NOTFOUND` would disclose transaction knowledge and make a forbidden request observable, while silently consuming it is intentional. B's trust boundary is the connection type/relay negotiation, not the individual request.

The adjudication must compare source comments and permission definitions, historical rationale, the existing full lifecycle, the isolated request schedule, and a temporary mutation that changes the request outcome. A source change requires evidence that the current contract is wrong, not merely that A's alternate behavior is observable.

### Initial source and history evidence

- `PeerManagerImpl::ProcessGetData()` obtains the optional `TxRelay` state at `:2853`. It consumes transaction requests from the front of the queue at `:2861-2867`; at `:2869-2873`, null relay state means a block-relay-only peer or a peer that asked not to receive transaction announcements, and the request is consumed without lookup or `NOTFOUND`.
- `PeerManagerImpl::RejectIncomingTxs()` at `src/net_processing.cpp:5971-5978` independently treats block-relay-only connections as unable to send transactions and requires `relay` permission in `-blocksonly` mode.
- `src/net_permissions.h` defines `NetPermissionFlags::Relay` as allowing relay and acceptance “even if `-blocksonly` is true”; `src/net_permissions.cpp` describes it as relay even in blocksonly mode. No permission grants transaction responses to a block-relay-only connection.
- `doc/reduce-traffic.md:42-53` describes `-blocksonly` as turning off transaction relay and says only a `forcerelay` peer remains an exception. `doc/reduce-memory.md:30` is broader: blocksonly opts out of receiving and relaying transactions except from a peer with `relay` permission.
- `fb821731eb` introduced the ignore branch in 2020 as “ignore tx GETDATA from blocks-only peers.” The later test-only commit `278710a88d` states the same special case and adds an assertion that no `NOTFOUND` is sent. The historical evidence favors B, while A's concern remains recorded because the wire protocol normally answers unknown `GETDATA` with `NOTFOUND`.

### Baseline execution

The current `p2p_blocksonly.py` passed with a freshly generated 199-block cache, Clang 19 functional binaries from `/data/my_storage/tmp/cycle105-clang19-release`, seed `7111`, and scratch data under `/data/my_storage/tmp/cycle111-multi-agent-adjudication-blocksonly-current2/`. It covered both ordinary blocksonly mode and relay-permission acceptance/forwarding, and exited 0. An earlier attempt using the incomplete `/data/my_storage/tmp/cycle104-cache` initialized only genesis and failed the framework's expected-height assertion; it was discarded as cache setup noise and not used as code evidence.

A temporary copy of the test added the focused schedule from upstream: restart in normal mode, create a block-relay-only outbound peer, send `GETDATA(MSG_WTX, 0x12345)`, assert `relaytxes == false`, and assert no `NOTFOUND`. The run used seed `7112` and scratch data `/data/my_storage/tmp/cycle111-multi-agent-adjudication-blocksonly-getdata/`; all lifecycle steps and the new request check passed with exit 0. This confirms the current behavior is deterministic and matches the documented special case, but does not alone settle whether the contract is desirable.

### Mutation and independent controls

The node was rebuilt after a temporary mutation changed the null-relay branch from “consume and ignore” to “append the request to `vNotFound` and continue.” The focused temporary harness then failed at its no-`NOTFOUND` assertion with exit 1, while the rest of the lifecycle completed. This proves the regression oracle is sensitive to the disputed behavior. The mutation was restored with `apply_patch`, `src/net_processing.cpp` is clean, and a clean-after rebuild of `bitcoind` passed.

The clean-after focused run used seed `7114` and scratch data `/data/my_storage/tmp/cycle111-multi-agent-adjudication-blocksonly-clean-after/`; it passed the ordinary lifecycle, the unknown `GETDATA(MSG_WTX, 0x12345)` no-response check, and a second temporary check that requested a known mempool transaction by `MSG_TX` from the same block-relay-only peer and observed neither `TX` nor `NOTFOUND`. The full-relay control `p2p_leak_tx.py` used seed `7115` and scratch data `/data/my_storage/tmp/cycle111-multi-agent-adjudication-leak-control/`; it passed and retained the ordinary `NOTFOUND` behavior for a full-relay inbound peer's unannounced transaction. A first known-transaction harness attempt was stopped after it tried to exceed the block-relay-only connection capacity; that was a harness error, not a node verdict, and the corrected reuse-of-existing-peer run passed with seed `7117`.

### Independent investigator reports

**Investigator A, dissent:** The wire protocol normally uses `NOTFOUND` to complete a `GETDATA` request, and `p2p_leak_tx.py` demonstrates that an ordinary peer can request an unannounced transaction and receive that response. A argues that a block-relay-only peer's explicit request should be answered consistently, at least with `NOTFOUND`, and that silent consumption can strand a peer waiting for a response. A could not identify a permission, BIP, or caller that promises transaction data to a block-relay-only peer, and acknowledged that answering a known transaction would defeat the connection's privacy/bandwidth purpose.

**Investigator B, ruling:** `GetTxRelay() == nullptr` is the negotiated capability boundary, not merely an announcement filter. The node separately rejects transactions received from block-relay-only peers, does not announce mempool transactions to them, documents blocksonly as opting out of receiving and relaying transactions, and defines `relay` permission as the exception. Returning `NOTFOUND` for a forbidden request would disclose that the request reached transaction handling and would diverge from the established 2020 decision to ignore these requests. The historical commit `fb821731eb` and current permission/comment evidence support consuming the message without a response.

### Final adjudication

Final verifier verdict: **dismissed; no production change justified**. A's request-completion concern is technically coherent but is outweighed by the explicit relay-capability contract, privacy/bandwidth rationale, historical implementation decision, current full-relay contrast, and the deterministic known-transaction control. The later upstream test-only commit `278710a88d` confirms this is a worthwhile regression oracle, but it is not a missing production fix and is already available as upstream evidence. No source or permanent test commit was made in this cycle.

### Cycle close

- Source diff: none; the temporary mutation was restored and `git diff --check` passed.
- Functional evidence: current `p2p_blocksonly.py` passed with seed `7111`; the focused unknown-request harness passed with seed `7112`; its mutation failed with seed `7113`; the clean-after focused harness passed with seed `7114`; full-relay `p2p_leak_tx.py` passed with seed `7115`; and the known-transaction reuse harness passed with seed `7117`.
- All Cycle 111 functional daemons were stopped. Persistent PID `777094` was untouched. Temporary harnesses and datadirs are outside the repository; no temporary source was staged.
- Next queue: draw another goal after the close snapshot. Do not reopen this block-relay-only request contract without new protocol, permission, or implementation evidence.

## Cycle 66: global relay queue and ForceRelay transition

### Gate and scope

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `40`
- Selected goal: `multi-agent-adjudication`
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- HEAD at cycle start: `4f68ee5f8455f0279b929e1d04b986b72c8bc104`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence: `origin/master...HEAD = 2 904`
- Catalog/protocol/TSV hashes matched the authoritative values.
- Tracked/staged state was clean except known agent artifacts; no relevant process was running.

### Candidate and falsifiable question

The cycle-54 duplicate-backlog and cycle-38 `-txsendrate` metadata cells are closed and excluded. The fresh candidate is the no-eligible-recipient transition in `PeerManagerImpl::ProcessInvBacklog` for a transaction initiated by a ForceRelay peer:

1. A transaction already in the mempool is force-relayed by a permitted peer.
2. At that instant, the inbound or outbound recipient set is empty, handshaking, non-relaying, fee-filtered, bloom-filtered, or already knows the transaction.
3. `ProcessInvBacklog` refunds the reservation and may clear the selected bucket.
4. A recipient later becomes eligible.

The disagreement to adjudicate is whether clearing the unannounced identity loses a required ForceRelay delivery, or whether it is an intentional best-effort boundary because peers that arrive later obtain ordinary mempool synchronization and the original caller does not own a durable relay obligation.

Trust boundaries are the remote ForceRelay peer, peer permission/relay state, the global token buckets, per-peer known-inventory filters, and the local mempool's unbroadcast set. The expected contract must be stated before using a failing test: the public documentation says ForceRelay transactions are received and relayed, while the global queue comments explicitly allow clearing when there is no current recipient.

### Independent evidence plan

- Investigator A: trace the production state machine from ForceRelay `TX` handling through `InitiateTxBroadcastToAll`, bucket selection/refund/clear, peer handshake and filter transitions, `SendMessages`, and the unbroadcast retry path. Treat the call-site promise as authoritative and attempt to construct a lost-delivery schedule.
- Investigator B: independently mine history, release notes, tests, comments, and the original global-queue design. Treat the queue's resource-bound and privacy contracts as authoritative and attempt to construct a valid later-recipient scenario that requires replay.
- Adjudication: run a deterministic live-node schedule with a transaction already in the mempool, no current eligible recipient, a ForceRelay duplicate, and a later recipient. Repeat with an unbroadcast local submission as a control. Use the observed state and the contract evidence to lock confirmed, dismissed, or inconclusive. Preserve dissent and any missing semantic evidence.

### Investigator A: production-state analysis

The production trace supports a lost-event hypothesis under a strong reading of ForceRelay:

- `ProcessMessage()` calls `InitiateTxBroadcastToAll()` for an already-mempool transaction received from a ForceRelay peer. That call adds the wtxid to both global sets and immediately invokes `ProcessInvBacklog()`.
- `ProcessInvBacklog()` selects from the mempool, reserves count and serialized-size tokens, then builds current recipient sets only from handshake-complete peers with a live trickle timer and `m_relay_txs=true`. It checks known inventory, fee filters, and BIP37 relevance while queuing.
- If no recipient is queued, the current branch refunds both reservations. It then clears the direction's backlog when the eligible peer vector is empty. The selected wtxid is not reinserted.
- `ReattemptInitialBroadcast()` retries only `CTxMemPool::m_unbroadcast_txids`, which are populated by local `MEMPOOL_AND_BROADCAST_TO_ALL` submissions. A remote ForceRelay duplicate is not added to that set.

Therefore, the live sequence can be expressed exactly: a transaction already in the mempool is sent again by a permitted peer whose VERSION has `relay=0`; the source is accepted as an input but is not an announcement recipient; the reservation is refunded and the identity disappears; a later relay-enabled peer receives no inventory. If “always relay” includes a future peer that was not connected or eligible at the event, this is a real delivery gap. If it means best-effort relay to currently eligible peers, the same trace is expected.

### Investigator B: history and contract analysis

The independent history pass supports intentional best-effort semantics:

- The original global-queue commit `df31ee57aa` introduced the current clear behavior. Blame on the exact lines says clearing an empty direction “reduces wasted memory” and “avoids having the bucket artificially empty for when future peers do connect.” This is an explicit design choice to discard old announcements rather than hold a durable future-peer queue.
- The current `p2p_tx_relay_rate_limit_known.py` regression describes a local submission with no eligible peer as something that must neither spend the global budget nor leave a backlog “that cannot be delivered.” This test treats no-recipient state as a completed no-op, not a deferred ForceRelay obligation.
- The release note for PR #34628 describes a global backlog as a rate-limited resource-control mechanism and does not promise historical replay to peers that connect later. The pre-existing unbroadcast retry path is explicitly limited to locally submitted transactions and retries after a randomized 10-15 minute interval.
- `doc/reduce-traffic.md` says ForceRelay peers' transactions are still received and relayed, but the existing `p2p_permissions.py` check exercises delivery to a connected recipient and contains no future-peer or durable-delivery promise. The wording is therefore compatible with current-recipient best effort, though it is not maximally explicit.

Investigator B's verdict is that retaining every no-recipient ForceRelay wtxid would contradict the resource and privacy rationale, create a new unbounded/long-lived obligation, and change an established queue contract without an authoritative protocol requirement.

### Live independent schedule

Temporary script `test/functional/p2p_force_relay_late_recipient.py` used Clang 19 functional binaries, a clean regtest chain, fixed mocktime `1700000000`, and random seed `6601`:

1. A relay-enabled seed peer submitted a transaction and was disconnected, leaving the transaction in the mempool without local unbroadcast bookkeeping.
2. A whitelisted ForceRelay peer completed VERSION with `relay=0` and sent the same transaction. The node had no eligible recipient; both buckets remained exactly `count_tok=30`, `size_tok=12000000`, `backlog=0` before and after.
3. A new relay-enabled `P2PTxInvStore` peer connected and synchronized. It received no `INV` for the transaction.

The test passed with exit 0 and no daemon errors. This is an observed state transition, not by itself a failing-before regression, because the two investigators disagree about whether future-peer delivery is in the contract. The existing `p2p_permissions.py` current-recipient control and the known-peer rate tests remain the independent controls to run next.

### Controls and independent verification

- The current-recipient control `p2p_permissions.py` passed with the existing Clang 19 functional daemon. Its repeated ForceRelay burst left one deduplicated backlog identity and completed successfully, demonstrating that a currently connected eligible recipient is served.
- `p2p_tx_relay_rate_limit_known.py` passed with the current daemon, covering no peer, `relay=0`, nonmatching BIP37, high-fee-filter, known-inventory, inbound, and outbound bucket states.
- The current `process_messages` libFuzzer replay with `-runs=200 -seed=6604` over 153 retained inputs completed 200 executions, added 5 corpus units, reached coverage 11,516, and exited 0 without an assertion or sanitizer report. The first attempt used a nonexistent root-filesystem `TMPDIR` and failed before fuzzing; the corrected run used `/data/my_storage/tmp/multi-agent-adjudication-cycle66-fuzz-tmp`.
- A temporary source mutation reinserted no-recipient identities and suppressed the documented empty-peer clear. With mocktime advanced for the next `SendMessages` pass, the same late-recipient script then failed because the new peer received the transaction's wtxid. Restoring the production code, rebuilding `bitcoind`, and rerunning the unmutated script passed with unchanged buckets and no `INV`. This proves the proposed durable behavior is observable, but does not prove it is required.
- The normal `util_tests` control passed 78 cases and 3,985 assertions after creating its scratch `TMPDIR`; the initial missing-directory failure is recorded as harness setup, not a code result.

### Final adjudication

The disagreement is substantive and retained:

- Investigator A's dissent: the live schedule demonstrates that a ForceRelay-triggered announcement is not replayed to a peer that becomes eligible later, and the remote ForceRelay path is not covered by `m_unbroadcast_txids`. A stronger “always relay” interpretation would require a durable obligation or a documented retry rule.
- Investigator B's ruling: the original PR explicitly clears no-recipient backlog state to avoid retaining work for future peers; the current tests and resource model define the global queue as current-recipient, best-effort relay; and no protocol, release note, or review evidence promises historical replay for later peers. Extending the queue would add retained state and alter the established resource/privacy contract.

Final verifier verdict: **dismissed; no production change justified**. The observed late-recipient loss is intentional under the current contract, not a proven defect. The `doc/reduce-traffic.md` ForceRelay sentence is broad enough to merit a future documentation-contract review, but this cycle has no independent evidence that operators or protocol peers rely on future-peer replay. The exact live schedule, dissent, mutation result, and all controls are retained for any future review or contract change.

### Cycle close

- Source diff: none.
- Temporary functional harness removed after the live schedule and mutation controls.
- No source, daemon, fuzzer, or test process remains running.
- Next queue: draw another distinct goal; do not reopen this relay transition unless a protocol/review contract, a caller with durable delivery semantics, or a new peer-state path supplies independent evidence.
