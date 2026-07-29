# Journal: serialization, deserialization, and untrusted-input sweep (campaign 6)

Uber-goal rotation. Branch: audit/serialization-untrusted-input from
audit/resurrection @ fc8c783fb5. Method: trace length/tag fields from
bytes to allocations/loops/casts/mutation; check failure-state contracts.
Prior coverage: goal 89 P1 (deserialization guards), campaign 98
(deserialize fuzz harness + FeeFrac exclusion), fuzz corpus tree-wide.

## Cycle 1 verdicts

### PartiallyDownloadedBlock::InitData (compact blocks): DISMISSED — provably bounded

- Total entries capped (blockencodings.cpp:74, MAX_BLOCK_WEIGHT/MIN_WEIGHT).
- Prefilled index arithmetic: per-index uint16 (no overflow, comment 103);
  accumulator checked per iteration (104, > uint16_max → INVALID); the
  106 check (index > shorttxids.size()+i → INVALID) guarantees
  lastprefilledindex ≤ BlockTxCount()-1, so the txn_available write (112)
  is always in bounds — vector sized at 83 before any write.
- fail_init (84-95) fully resets header/vector/counters — failure leaves
  the documented clean state; re-InitData rejected (77).
- Anti-DoS: bucket-distribution statistical cap (126-137) rejects
  adversarial short-id collisions (READ_STATUS_FAILED, not crash);
  duplicate shorttxids detected via map-size mismatch (139).

### CNetAddr::UnserializeV2Stream (addrv2/BIP155): DISMISSED — bounded reads, defined invalid state

- address_size > MAX_ADDRV2_SIZE → throw (netaddress.h:431-434); unknown
  BIP155 net id → bounded skip via s.ignore (469).
- SetNetFromBIP155Network validates net-vs-size consistency (438).
- Invalid/unrecognized inputs map to default-constructed !IsValid()
  (472-475) — a defined safe state, never partially-mutated garbage.

## Cycle 1 note on prior coverage
Persisted-state parsers (coins DB AmountCompression/ScriptCompression)
were audited in campaign 98 (domain-enforced, range-throwing); network
deserialization failure handling in goal 89 P1. Remaining: wallet record
keys (low exposure — local files) and index DBs (covered by PR 35654
first-cursor-key lineage). Cycle 1 complete.
## Cycle 2 (2026-07-28): txoutproof RPC composition — DISMISSED, negative-oracle battery committed

### Draw
Random draw over the 68-goal eligible pool: seed=4632181313553342032
(od -N8 /dev/urandom), index 4 -> #6.

### Scope choice
The binary parser layers (merkle_block_deserialize,
partial_merkle_tree_deserialize, FUZZ_TARGET(merkleblock)) are fuzzed; the
RPC composition gettxoutproof -> mutate -> verifytxoutproof is not a fuzz
target, and rpc_txoutproof.py carried an explicit TODO for more invalid
variants. Trust boundary: verifytxoutproof consumes fully
attacker-controlled hex on an authenticated-but-untrusted-content RPC.

### Code reading (bounds proof sketch)
CPartialMerkleTree deserialization (merkleblock.h:90-98): nTransactions is
attacker-arbitrary uint32, vHash/vBits bounded by input size (HTTP body
limit). ExtractMatches (merkleblock.cpp:166-209) rejects nTransactions==0,
> MAX_BLOCK_WEIGHT/MIN_TRANSACTION_WEIGHT (66666), vHash.size() >
nTransactions, vBits.size() < vHash.size(); traversal consumes bits/hashes
with per-step bounds checks and recursion depth <= ceil(log2(66666)) = 17;
rejects unconsumed bits/hashes and right==left subtrees (CVE-2012-2459).
verifytxoutproof (rpc/txoutproof.cpp:147-170): ExtractMatches-failure
returns uint256(0) which can only "match" a zero hashMerkleRoot, but the
chain-membership check (LookupBlockIndex + ActiveChain().Contains) then
throws, so partially-filled vMatch never escapes — output-on-failure safe.

### Experiment (scratch probe /tmp/r6_txoutproof_probe.py, regtest, 5-tx block)
Positive control + 9 mutations, all graceful (full log /tmp/r6_probe_result.txt):
- m1 flip included-hash bit -> [] ; m2 header nTime+1 -> -5 Block not found
  in chain ; m3 nTx=0 -> [] ; m4 nTx=0xffffffff -> [] ; m5 truncated ->
  -1 SpanReader end of data ; m7 extra bits -> [] ; m8 extra hash -> [] ;
  m9 nTx+1 -> [].
- m6 trailing garbage appended -> ACCEPTED (same txids). Non-canonical
  acceptance, same as DecodeHexBlk/submitblock (no full-consumption check);
  contrasts with DecodeTx which explicitly rejects trailing data
  (core_io.cpp:162). Recorded as a consistency observation, not a defect:
  the proof commits to the same txids, no caller contract violated.

### Verdict
- DISMISSED: no wrong-txid acceptance, no crash, no partial-output escape.
- Committed the battery (minus m6, whose acceptance is intentional-looking
  and shared with submitblock) into rpc_txoutproof.py, closing the
  in-tree TODO. Test-only change; no implementation defect found.
  `python3 test/functional/rpc_txoutproof.py --configfile=build-before/test/config.ini`
  -> Tests successful.

### Why existing tests missed (nothing)
Nothing to miss — the composition was already sound; only the negative
oracle was thin (1 variant + TODO). Now 9.

### Limitations
- m6 canonicality asymmetry (DecodeTx vs SpanReader-callers) is tree-wide;
  unifying it is a design decision beyond this campaign's defect bar.
- BIP37 serving side (bloom filter matching) not re-audited this cycle;
  bloom parser fuzzed (bloomfilter_deserialize, bloom_filter targets).

### Next queue for this campaign
- Wallet record key parsing (low exposure, local files) — last unrun cell.

## Cycle 3 (2026-07-29): c2 backport into lineage + wallet-record cell — campaign EXHAUSTED

### Draw
Random draw over the 17-goal eligible pool (14 pending + 3 CYCLE-1,
#49 excluded as just-cycled): raw=5880676013471384719, index 13 ->
#6. Ledger said CYCLE-1, but history shows a c2 commit
(9d1244e6b1 on audit/serialization-untrusted-input, 2026-07-28)
whose row was never restored — so this is cycle 3 plus retro
bookkeeping. Branch: audit/serialization-untrusted-input-c3 from
c86a6e847a (#49 c1 bookkeeping).

### Stranded-work recovery (the #66 problem, recurring)
The c2 deliverable (txoutproof negative-oracle battery, 9 mutations
closing the in-tree TODO in rpc_txoutproof.py) and the c1+c2 journal
existed ONLY on the side branch; HEAD still had the TODO at :109.
Backported the full c2 commit into this branch as 4b8fa7c937
(cherry-pick; journal conflicts resolved theirs-for-campaign-journal,
ours-for-historical uber-rotation.md). Verification at HEAD:
`python3 test/functional/rpc_txoutproof.py
--configfile=build-before/test/config.ini --tmpdir=/tmp/btc6` ->
Tests successful.

### Cell: wallet record key parsing — DISMISSED (machinery-bounded, seam fuzzed)
Hypothesis: an unguarded untrusted-input deserialization defect in
wallet record loading, distinct from the load_wallet harness.
Analysis:
- Modern LoadWallet (walletdb.cpp:1102-1162) is per-class loaders
  (Legacy/Descriptor/AddressBook/ActiveSPKMs/DecryptionKeys/Tx)
  inside a catch-all -> DBErrors::CORRUPT; no crash path.
- Record keys/values deserialize via the same bounded streams
  machinery audited across this campaign (CompactSize battery O1,
  ReadVarInt overflow battery O2); fixed-size types (CPubKey/CKeyID/
  uint256) throw on short reads.
- The record-application seam is already fuzzed by the O5 harness
  (load_wallet, #10 c2: FLAGS/VERSION/NAME/descriptor/TX/unknown
  records, DBErrors-classification + round-trip asserts); descriptor
  strings additionally have their own fuzz target
  (descriptor_parse.cpp) plus script_descriptor_cache.cpp.
- Trust boundary is local-only (wallet files are local state;
  corruption yields CORRUPT classification, never silent acceptance
  — harness-asserted).
- Unfuzzed value classes (crypted keys, ACTIVE*SPK, BESTBLOCK)
  differ in value type, not machinery; widening them is O5's queued
  next step under campaign #10, not a #6 gap.

### Campaign exhaustion (all cells accounted)
- compact-block (c1): bounded, dismissed.
- addrv2/BIP155 (c1): bounded, dismissed.
- gettxoutproof/verifytxoutproof (c2): battery delivered, backported
  4b8fa7c937, green at HEAD.
- wallet record keys (c3, this cycle): dismissed above.
- persisted-state coins DB (AmountCompression/ScriptCompression):
  covered by campaign #98 (c1 note).
- network deserialization failure handling: covered by #89 P1 (c1
  note).
- index DBs: PR 35654 first-cursor-key lineage (c1 note).
No untrusted-input serialization cell remains without an owner.
Marking EXHAUSTED; reopen if new parsers/callers appear (the O1/O2
batteries and O5 harness are the standing oracles).

### Exact commands
- git cherry-pick 9d1244e6b1 (conflicts: theirs for campaign journal,
  ours for uber-rotation.md) -> 4b8fa7c937
- rpc_txoutproof.py at HEAD -> Tests successful
- reads: walletdb.cpp:485-550/958-1034/1068-1102/1102-1162,
  fuzz target lists (descriptor_parse, load_wallet:45)

## Rotation note
Campaign EXHAUSTED as of cycle 3; see exhaustion evidence above.
