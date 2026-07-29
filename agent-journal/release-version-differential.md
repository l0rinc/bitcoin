# Campaign #67 — release-version-differential

Base: 4d53c28785 (journal commit for URGENT.md #48-c1 item on
audit/property-oracle; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/release-diff. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): v28.2 + v0.20.1 ↔ current-HEAD regtest P2P/sync differential — no divergence on any axis

### Draw
Random draw over the 59-goal pool (40 pending + 19 CYCLE-1; #48
excluded as just-cycled): raw=3281368855708499593, index 21 -> #67.

### Setup (local artifacts, no downloads)
releases/v28.2/bin and releases/v0.20.1/bin (official aarch64 release
binaries already in-tree, verified by --version) vs build-before
(current HEAD 4d53c28785, v31.99.0 fork). Regtest, isolated datadirs,
fixed ports. Harnesses: /tmp/btc67_run.sh, /tmp/btc67_v20.sh.

### Cell 1: v28.2 ↔ current (both directions)
- v28.2 serves 500 empty blocks; current syncs: 500/500, tip hash
  identical (2348d094...5916f). Handshake: v2 (BIP324) transport
  negotiated BOTH ways (getpeerinfo transport_protocol_type=v2 on both
  sides), relay=True, bip152_hb=True.
- current serves 500; v28.2 syncs: 500/500, tip identical
  (3e12d1fa...20d64), v2 transport both ways again.

### Cell 2: v0.20.1 ↔ current (pre-wtxidrelay/addrv2/BIP324 era)
- current serves 300 empty blocks; v0.20.1 syncs: 300/300, tip
  identical (4f4d75cc...a12f). Transport: v1 (expected — v0.20.1
  predates BIP324; the current node correctly falls back).
- Service flags: current advertises 0xc09 (NETWORK|WITNESS|
  NETWORK_LIMITED|P2P_V2) to the v0.20.1 peer; v0.20.1 offers 0x409
  (NETWORK|WITNESS). Unknown service bits ignored as required — no
  negotiation error, no disconnect, full sync.

### Verdict
- DISMISSED: no behavioral or consensus differential between v28.2 /
  v0.20.1 and current HEAD on the regtest empty-chain P2P path:
  handshake, feature negotiation, BIP324 fallback, compact blocks,
  and block relay all interoperate; tips bit-identical in all four
  syncs.
- P2P_V2 service flag handling by old nodes confirmed graceful.

### Exact commands
- /tmp/btc67_run.sh prep_old|sync_cur|sync_old (v28.2 pair)
- /tmp/btc67_v20.sh (v0.20.1 pair)
- getpeerinfo/getbestblockhash/getblockcount comparisons per side

### Harness lesson (recorded)
Cleanup raced the nodes: datadirs were rm -rf'd while two bitcoinds
from cell 1 were still up (my script's stop covered only cell 2);
RPC stop then fails (cookie gone). Fix sequence: stop first, verify
pgrep empty, THEN delete. Leftover daemons were SIGTERM'd cleanly.

### Limitations / queue
- Empty chains only: tx-heavy/mempool-relay differential (wtxid vs
  txid inventory across the v0.21 boundary) queued.
- Chainstate/blocks disk-format downgrade read (current-written
  datadir opened by v28.2 and v0.20.1) queued — block files are
  format-stable, but chainstate obfuscation and leveldb versions
  differ.
- Functional-suite back-compat (test/functional/feature_backwards_
  compatibility.py with these exact binaries) not re-run this cycle.
- All scratch datadirs removed; harness scripts kept in /tmp.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
