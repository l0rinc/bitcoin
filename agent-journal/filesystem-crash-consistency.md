# Journal: filesystem, power-loss, and crash-consistency injection (goal 72)

Uber-goal draw (cycle-327 fresh audit, raw=2399434307952902088 -> idx 72).
First campaign cycle (never run before; the flood's goal72 settings-
durability test f194e4482e was adopted independently).

## Cycle 1 (2026-08-04): blk*.dat crash-consistency — torn-write DETECTED fail-loud; mid-file bit-rot silent by design (trust boundary); CONFORM/DISMISSED

### Boundary and prior coverage
blk*.dat flat files (XOR-obfuscated in this lineage, key in
blocks/xor.dat). Neighboring coverage: LevelDB/chainstate (goals
125/126/127 CONFORM + goal-119 write family + F25/F26/F27),
settings (f194e4482e), banlist/peers (goal-41/63), snapshots
(F27). The flat-block-file torn-write/bit-rot cell was uncovered.

### Rig (scratch, deterministic)
/tmp/btc72: regtest, 45 blocks (46 records, blk00000.dat,
end-of-data 11813). Two fault variants via direct byte edits
(record offsets parsed through the XOR key):
- FLIP: single byte flipped inside record 20's payload
  (offset 4969; coinbase tx byte 68: 55->200).
- TRUNC: file cut at 4941 (mid-record of record 20; blocks 20-45
  unreachable incl. the tip).

### Results
- Control restart (pristine): 45 blocks, best 788f...5b38,
  block 20 served 497 chars. OK.
- FLIP restart: getblockcount 45, block 20 served 497 chars with
  the corrupted byte; debug.log has ZERO corruption detections
  (grep corrupt/truncat/invalid/error: only dummySeed.invalid
  lines). Record-20 storage diff vs pristine confirmed at byte 68.
- TRUNC restart: startup tip-read hits EOF:
  `[error] Read from block file failed: AutoFile::read: end of
  file ... FlatFilePos(nFile=0, nPos=11565)` then
  `Verification error: ReadBlock failed at 45` then
  `Corrupted block database detected.` — init ABORTS (RPC
  unreachable). Fail-loud, no false progress, bounded (operator
  reindex/resync).

### Analysis and verdict
- TRUNC (torn/short write of blk file): DETECTED at startup via
  the tip-block read; hard abort with an actionable message.
  Crash-consistency contract HOLDS. CONFORM.
- FLIP (bit-rot in a non-tip block payload): NOT detected —
  startup re-reads only the tip; the LevelDB index's stored
  validity flags stand. The corrupted block is served to RPC and
  peers (a receiver recomputing the merkle root rejects it).
  DISMISSED as a defect: the datadir is a trust boundary
  (write access to it is game-over by design), upstream carries
  no per-record checksum intentionally, and the XOR layer is an
  obfuscation, not an integrity scheme. Documented behavior, not
  a reachable defect. Severity note: a local attacker able to
  flip bytes in blk files can already replace the whole
  chainstate — this adds no new primitive.

### Exact commands
- mkdir /tmp/btc72; bitcoind -regtest -daemon; createwallet;
  generatetoaddress 45; stop; python3 record parser (xor.dat key);
  byte flip / truncate; restart + getblockcount/getblock/
  debug.log grep; storage diff via XOR-aware reader.

### Limitations / queue
- Startup tip-read coverage: only the TIP block is re-read at
  init (that's why mid-file damage is silent). A
  verify-on-read-every-serve mode is upstream design space, not
  a defect queue.
- Wallet DB and flat-file peers/banlist crash consistency:
  covered by goal-41/63/88 (referenced, not re-run).
- #72 queue: settings (f194e4482e adopted), blk files (this
  cycle). Remaining distinct surface: chainstate-LevelDB torn
  COMPACTION (goal-125/126 covered recovery semantics, not a
  torn compaction specifically) — candidate cell if drawn again.
