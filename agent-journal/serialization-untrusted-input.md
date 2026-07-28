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
