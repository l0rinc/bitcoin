# Campaign #41 — history-seed-archaeology

Base: a49a2ebe92 (journal commit for #108 cycle-1 on
audit/adversarial-artifacts; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/history-archaeology. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): fee_estimates.dat version gate — archaeology of the migration contract, empirically fails closed both directions

### Draw
Random draw over the 33-goal pool (20 pending + 13 CYCLE-1; #108
excluded as just-cycled): raw=13402773429125480578, seed masked to 63
bits (4179401392270704770), index 5 -> #41. Seed: persistence-format
evolution of fee_estimates.dat (this session's estimator work: #63
fix, fork guards 62f15c4ab9/44fcabc565/0cf655f1d6).

### Archaeology (contract as it stands)
- CURRENT_FEES_FILE_VERSION = 309900 (estimator .cpp:37-39, with the
  bump instruction on breaking change). Write serializes version,
  best-seen height, historical range, buckets, 3 TxConfirmStats.
- Read gate (Read at :1103): version > CURRENT -> throw "too high to
  be read" (caught non-fatally); version < CURRENT -> LogWarning
  "Incompatible old fee estimation data (non-fatal)" and read on
  (backward-compatible); == CURRENT -> full read with snapshot
  variables (44fcabc565's atomicity: partial reads never corrupt
  live state).
- Fork's read-side guards (62f15c4ab9 IsSaneEstimatorVector,
  0cf655f1d6) are additive; the file format is unchanged by them and
  by #63's m_all_zero (not persisted).

### Empirical differential (current -> v28.2)
Current node on regtest writes fee_estimates.dat (309269 B, version
bytes 8cba0400 = 309900 LE) after real txs; start v28.2 against a
copy: log reads
  "CBlockPolicyEstimator::Read(): unable to read policy estimator
   data (non-fatal): up-version (309900) fee estimate file"
  "Failed to read fee estimates ... Continue anyway."
v28.2 starts fresh and stays healthy (getblockcount OK).

### Verdict
- DISMISSED: the persistence-migration contract fails CLOSED in both
  directions — older files read with a non-fatal warning, newer files
  rejected non-fatally; no silent misinterpretation path exists.
- The fork's guard series and #63's change preserve the format
  (verified by the 309900 version byte and v28.2's rejection message
  naming it) — archaeology consistent with the code.

### Exact commands
- greps/seds: block_policy_estimator.cpp:37-39,464-510,1103-1160
- node run to produce the file + v28.2 read against a copy
  (/tmp/btc41_d, /tmp/btc41_old, removed after)

### Limitations / queue
- Downgrade WITH data (v28.2 file -> current) exercised by the code
  path (non-fatal old read) but not run this cycle — the warning log
  line is the same class; noted, not re-run.
- Other persistence artifacts (mempool.dat, anchors.dat, xor.dat)
  have their own version stories — queued as separate archaeology
  cells.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): mempool.dat version gate + error paths — fail-closed everywhere, tolerance exactly as documented; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=8742819288871107508, masked
same, n=3, idx=1) -> history-persistence-artifacts -> #41 (second
cycle; c1 queue cell "mempool.dat ... own version stories").
Branch: audit/history-seed-c2 from 405008c571 (#50 c13 tip).

### Hypothesis
The mempool.dat persistence path could accept an incompatible
version, accept garbage from a corrupted body, or crash/assert on
truncated input — the persistence archaeology class from c1
(fee_estimates.dat fails closed).

### Mechanism map (src/node/mempool_persist.cpp)
- Version gate (:46-104): v1 (no obfuscation) / v2 (header
  obfuscation key) — anything else returns false before any entry
  is read (zero mutation).
- Entry loop (:110-178): one try block; per-entry ATMP with
  failed/already_there/expired counters; a deserialization
  failure anywhere aborts the FILE at that point with
  'Continuing anyway' + return false (:180-184); entries already
  accepted stay (partial import, documented).
- importmempool RPC (rpc/mempool.cpp:1146-1200): throws
  RPC_MISC_ERROR when LoadMempool returns false.

### Experiment (driver /tmp/mp_arch.py; regtest, PortSeed 322)
MiniWallet 101+10 mature blocks, 6 fan-out txs, savemempool ->
2379 B v2 file; restart with cleared mempool.dat, then:
- ok-v2: import OK, '6 succeeded, 0 failed, 0 expired, 0 already
  there' — full load.
- badver-999 (version field patched): NO 'Loading' log line —
  rejected before any entry read; pool unchanged (zero mutation);
  RPC error.
- flip-60% (8 bytes flipped mid-entry, framing intact): NO
  exception — '0 succeeded, 1 failed, 5 already there' — the
  corrupted entry is a normal ATMP policy failure, the valid
  prefix dedupes against the existing pool; no garbage accepted.
- trunc-40%: 'AutoFile::read: end of file: iostream error.
  Continuing anyway.' -> RPC error; pool unchanged.
- Node stopped clean after all four.

### Verdict
DISMISSED: the version gate fails closed with zero mutation, the
v2 obfuscated format round-trips fully, corrupt bodies degrade
exactly as documented (per-entry policy failure, never garbage
acceptance), truncation aborts tolerantly with the documented log
line. The mempool.dat version story is consistent with the code.

### Exact commands
- python3 /tmp/mp_arch.py (output above); mechanism reads
  node/mempool_persist.cpp:40-185, rpc/mempool.cpp:1146-1200.
- Harness note: at height 101 only coinbase[0] is mature — mine
  >=101+k for k spendable coinbases (same lesson as #2 c3).

### Limitations / queue
- The obfuscation-key portability across nodes (v2 file moved
  between datadirs) is by design (key embedded); not separately
  re-run (same class as the ok case).
- anchors.dat / xor.dat remain as their own cells.

## Rotation note
Two cycles; fee_estimates.dat and mempool.dat both fail closed.

## Cycle 3 (2026-07-30): obfuscation-key (xor) archaeology — every corruption class fails LOUD; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=17581368771181834455,
masked=8357996734327058647, n=4, idx=3) -> xor-dat -> #41 (third
cycle; c1 queue cell "xor.dat version story"). Branch:
audit/history-seed-c3 from 963a50267e (#41 c2 journal tip).

### Mechanism map (fork-specific, dbwrapper.cpp:276-292)
- The obfuscation key lives as a LevelDB record INSIDE each DB:
  record key = compactsize(14) + "\000obfuscate_key" (0e prefix),
  value = 08 + 8 key bytes (chainstate obfuscates by default,
  validation.cpp:1959).
- Open path: Read fails + Exists -> throw dbwrapper_error
  ('Invalid obfuscation key in <path>') — the designed fatal
  gate. Key missing on non-empty DB -> null key (reads plain).
- Stock-LevelDB tooling sees only raw bytes; the compactsize
  prefix is what naive exact-match probing misses (my v1 tool
  reported 'NO obfuscation key record' — recorded as a tool
  lesson: the fork prefixes the name with its serialized length).

### Experiment (tool /tmp/xor_tool on a scratch obfuscated
chainstate, fresh copies per case, background starts)
1. manglelen (value length byte broken): '[error] Invalid
   obfuscation key in <path>' -> startup abort — the designed
   dbwrapper_error gate fires exactly.
2. corruptkey (whole-record XOR, also broke the shape): same
   designed gate ('Invalid obfuscation key' -> 'Error opening
   coins database.').
3. delkey (record deleted on obfuscated data): 'Using obfuscation
   key ... 0000000000000000' (null key) -> 'Error initializing
   block database.' — loud downstream deserialization failure, NOT
   silent garbage loading.
4. wrongkey (valid shape, 8 key bytes flipped): opens with the
   wrong key 'c8a5f6b36dc77461' -> 'Error initializing block
   database.' — garbage never silently accepted.
5. control (pristine copy): starts clean with the original key.

### Verdict
DISMISSED: every corruption/deletion of the obfuscation key
produces a loud startup failure — the designed gate for shape
corruption (1-2), loud load failure for missing/wrong keys (3-4).
Nothing fails silently; the xor-key story is consistent with the
code.

### Exact commands
- /tmp/xor_tool (modes dump/corruptkey/manglelen/delkey/wrongkey/
  flipcoin) vs build-before libleveldb; case datadirs copied from
  a 12-block scratch chainstate, starts via
  bitcoind -regtest -datadir=<case> -daemon + log greps above.
- Setup note: generatetoaddress on a walletless node fails (no
  default wallet) — the scratch chain is genesis-only, which does
  not affect the key mechanics (recorded as a setup flaw; the
  flipcoin case was dropped for it in v2).

### Limitations / queue
- blocks/index and blocksdir keys (separate artifacts, seen as
  0000000000000000 / '8a8ef6f08e2ac8d0' in the logs) — same record
  family; the chainstate cases cover the mechanics.
- anchors.dat remains as its own (smaller) cell.

## Rotation note
Three cycles; fee_estimates.dat, mempool.dat, and the xor-key
story all fail closed/loud.

## Cycle 4 (2026-07-30): anchors.dat archaeology — all-or-nothing + read-and-delete everywhere; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=1453947281365744417, masked
same, n=4, idx=1) -> anchors-dat -> #41 (fourth cycle; c3 queue
cell "anchors.dat as its own smaller cell"). Branch:
audit/history-seed-c4 from 94e708cd8c (#63 c4 journal tip).

### Mechanism map (addrdb.cpp:228-245, :54-133)
- DumpAnchors: SerializeFileDB('anchors', path, CAddress::V2_DISK
  (anchors)) — temp-file + rename (atomic) into
  GetDataDirNet()/<chain>/ first.
- ReadAnchors: DeserializeFileDB; ANY exception -> anchors.clear()
  (empty, fail-closed) then fs::remove — read-and-delete, so a
  corrupt file can never wedge startup or repeat-fail.

### Experiment (driver /tmp/anchors_probe.cpp linking addrdb.cpp,
3-anchor files + variants)
1. round-trip: 3 read back, file removed.
2. flipped-60%: 0 read (all-or-nothing), file removed.
3. truncated-50%: 0 read, file removed.
4. trailing-junk (64 B appended): 3 read — TRAILING BYTES IGNORED
   (the vector deserializes by count; tolerance recorded), file
   removed.
5. zero-anchors: 0 read, file removed.
6. missing file: 0 read.

### Verdict
DISMISSED: anchors.dat fails closed under every corruption class
(all-or-nothing deserialization + read-and-delete), tolerates
trailing junk, round-trips exactly, and never wedges startup.
The version story is consistent with the code.

### Harness lessons (recorded)
- SerializeFileDB stages its temp file in
  GetDataDirNet()/<chain>/ — the driver needs SelectParams(REGTEST)
  + ForceSetArg datadir + the regtest subdir created, or the dump
  silently no-ops (fopen NULL -> LogError only).
- My probe then SEGV'd on fseek(NULL) of the missing file —
  driver-side, logged for the replay trail.

### Exact commands
- g++ -O2 -std=c++20 -I src -I build-before/src -I
  src/univalue/include /tmp/anchors_probe.cpp src/addrdb.cpp
  -lbitcoin_node -lbitcoin_common -lbitcoin_consensus
  -lbitcoin_util -lbitcoin_crypto -lbitcoin_clientversion
  -lleveldb -lcrc32c -lsecp256k1 -lunivalue; output above.

### Limitations / queue
- peers.dat (the richer LoadAddrman version story with
  InvalidAddrManVersionError + .bak backup) is the natural next
  archaeology cell if one lands here.
- #41's c1 queue is otherwise closed (fee_estimates, mempool,
  xor-key, anchors).

## Rotation note
Four cycles; four persistence artifacts, all fail closed.
