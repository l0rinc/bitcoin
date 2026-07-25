# Review: upstream merges 2026-07-25 — eviction capacity series + vsize RPCs

Range: 3a2c52f9d7..6b059d9dbd (10 commits). Verdict: 🟢 nothing critical.

## Eviction series (#28463: 1b76e04736, 87bca1c2ad, 0bd3d3dfa5, 3ed7f06418, 69ce0dba2a, cc59aee196, c11508406e)

- `EvictTxPeerIfFull`: counts inbound non-disconnecting tx-relaying peers under `m_nodes_mutex`; over cap (50% of `m_max_inbound`, default 200 → 100) → `AttemptToEvictConnection(evict_tx_relay_peer_only=true, protect_peer)`. Filtering is candidate-narrowing only (skip protect_peer, skip non-tx-relay); protection categories and network-diversity eviction order untouched.
- Accept-side accounting sound: `MaybeDisconnectForTxRelayCapacity` (net_processing.cpp:5194) keeps the new peer when eviction succeeded/unnecessary, else disconnects the *new* peer (`fDisconnect=true`) — the cap can't be leaked.
- Transition case closed: `3ed7f06418` re-runs the check when a peer upgrades to tx relay (bloom negotiation), covering peers accepted as block-relay-only that later become tx-relaying without a new connection event.
- Defaults: 125→200 total, tx-relay cap 100 (was ~117 effective) — intended shift toward cheap block-relay capacity; configurable via `0bd3d3dfa5`.
- Percentage math exact (0.5 binary-exact, floor via truncation); `max(0, …)` clamp present.

## vsize RPC series (#32800: eaef8d3111, 5d25a0c28d, 29b124416e)

Additive output fields (`vsize_adjusted` = sigop-adjusted, `vsize_bip141`) on mempool/getrawtransaction RPCs — backward-compatible; values computed by existing size helpers. Low risk.
