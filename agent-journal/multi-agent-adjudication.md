# Independent Multi-Agent Disagreement and Adjudication

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
