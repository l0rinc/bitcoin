# BIP35 relay-limit adoption (PR 35874) — patch artifacts

Base-relative adoption: the fix requires the inbound tx-relay capacity
machinery (`MaybeDisconnectForTxRelayCapacity`, `m_relays_txs`), which this
archive's pre-rebase base lacks — so the commits cannot be cherry-picked
here without falsifying content. Full patches preserved instead.

## Provenance

- Upstream PR: bitcoin/bitcoin#35874 (open at adoption time)
- Source branch: `audit/adopt-bip35-relay-limit`
  - `be0d60c51d` test: characterize BIP35 relay capacity
  - `12fc6ec039` test: refactor BIP35 request checks
  - `a13e00bdc6` p2p: enforce relay limit for BIP35 requesters
- Integration branch copies: `03e137f25e` / `4ced0a8cb6` / `dbf71405d7`
  (audit/transplant-index-fuzz)

## Evidence

- Failing-before: `test/functional/p2p_connection_limits.py` (with the PR's
  new BIP35 case) → AssertionError on the unfixed source (ASan build-after).
- Passing-after: same suite "Tests successful" with the fix.
- Mechanism: MEMPOOL handler served fRelay=false inbound peers without
  charging tx-relay capacity; the fix sets `m_relays_txs` and runs the
  capacity check before queuing the first BIP35 response.
- Reachability: gated on non-default `-peerbloomfilters` (NODE_BLOOM).
  Master-relative severity low-to-medium (bandwidth-amplification
  hardening), adopted for lineage parity.

Patches apply cleanly to `audit/transplant-index-fuzz` as of cycle 358.
