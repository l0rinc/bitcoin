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

## Cycle 2 (2026-07-31): wtxid-vs-txid inventory across the v0.21 boundary — all three protocol layers match BIP339; DISMISSED

### Draw
RE-RANK draw 148 over the 4-cell queue: raw=7620657173068370897
(already 63-bit) -> idx 1 -> #89 tx-heavy/mempool-relay
differential (c1 queue). Branch: audit/release-diff-c2 from
audit/release-diff.

### Setup (isolated regtest, -capturemessages on current HEAD)
N = build-before HEAD (capture), A = v0.20.1 (pre-wtxid), B =
v0.21.0 (BIP339). N mines 101 blocks, connects to both, wallet tx
sent on N (fee_rate=2 named arg; the fork's node does not
auto-create wallets and regtest needs explicit fee_rate).

### Captured differential (message-capture parser)
- NEGOTIATION: wtxidrelay handshake present only with B (2 msgs);
  zero with A.
- ANNOUNCEMENT: N->A inv type 1 (MSG_TX, txid 9cd5b0e3...);
  N->B inv type 5 (MSG_WTX, wtxid 348818ba...) — same tx,
  wtxid != txid as expected with a witness.
- REQUEST: A's getdata type 1073741825 (MSG_WITNESS_TX);
  B's getdata type 5 (MSG_WTX) — each side fetches in the
  negotiated format.
All three layers agree with the BIP339 contract on both sides of
the v0.21 boundary.

### Verdict
DISMISSED: wtxid/txid relay selection is negotiated, announced,
and fetched correctly per peer version. No cross-version
divergence.

### Exact commands
- three-node regtest rig above; capture parse via
  contrib/message-capture/message-capture-parser.py (types
  verbatim above); harness notes: cli needs -rpcport per node,
  argv/port traps as before.

### Limitations / queue
- Remaining queued: chainstate/blocks downgrade read (current
  datadir opened by v28.2 and v0.20.1); functional
  feature_backwards_compatibility with these exact binaries.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-08-01): downgrade read — v28.2 opens HEAD datadir clean; v0.20.1 aborts LOUD on XOR-obfuscated block files (upstream #28052, default-on); forward-compat of its mutations verified; DISMISSED

### Draw
RE-RANK draw 155 over the 7-cell pool: raw=7423517245362505699
(already 63-bit) -> idx 6 -> #67 chainstate/blocks downgrade read
(c1 queue). Branch: audit/release-diff-c3 from 82cc86ce7e.

### Fixture
HEAD writes a 150-block regtest datadir (blocks are
XOR-obfuscated per -blocksxor default true, upstream PR #28052
merged 2024-08-05, first in v28.0; xor.dat key in blocksdir).

### Matrix
- v28.2: clean open, 150/150 blocks+headers, only the known
  non-fatal "up-version (309900) fee estimate file" skip (#41 c1
  contract).
- v0.20.1: ReadBlockFromDisk "Errors in block header" at the tip
  -> VerifyDB "***" -> "Corrupted block database detected.
  Please restart with -reindex or -reindex-chainstate" -> ABORT.
  Retry WITH -reindex-chainstate fails identically (fatal) — the
  abort is deterministic and loud, not a silent misread.
- blk-file analysis: blk00000.dat carries no fabfb5da magic — it
  is XOR-obfuscated (key in blocksdir/xor.dat); v0.20.1 predates
  the feature entirely.
- v0.20.1's writes before aborting (banlist.dat recreate,
  peers.dat v1, mempool.dat, settings.json) are FORWARD-SAFE:
  HEAD reopens the same datadir at 150 with zero corruption
  (banlist.dat warning is the #41 c6 contract; old peers.dat
  readable per #41 c5).

### Verdict
DISMISSED: the downgrade boundary is exactly at the blocksxor
feature release (v28.0), fails LOUD with correct reindex advice,
and cross-mutations are benign. No silent corruption, no
fork-specific deviation (feature and default are upstream).

### Exact commands
- fixture + 3-binary matrix above; blk magic scan (xxd/rfind);
  framework CBlock parse attempt (magic absent -> obfuscation
  proof); v0.20.1 plain + -reindex-chainstate attempts; HEAD
  reopen check.

### Limitations / queue
- Network-resync recovery with the old binary not run (needs a
  peer; the abort's reindex advice is verified literal on the
  file level).
- Remaining #67 cell: feature_backwards_compatibility with these
  exact binaries.

## Rotation note
Cycle 3 complete; rotating per uber-goal policy. Not exhausted.
