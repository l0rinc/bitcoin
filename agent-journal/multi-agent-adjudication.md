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

### Candidate status

Pending the two independent analyses and the live schedule. No production change is justified at the start of the cycle.
