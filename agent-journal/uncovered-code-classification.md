# Campaign #34 — uncovered-code-classification

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/uncovered-code. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): merkleblock.cpp unit coverage 81/86 -> 86/86, all 5 lines classified + closed (32d5d1dcc4)

### Draw
Random draw over the 56-goal eligible pool: raw=7159137639898091638,
index 14 -> #34.

### Coverage method (bounded, no full coverage build)
Disk budget (11 GB free) ruled out a full -DCMAKE_BUILD_TYPE=Coverage
tree. Single-object gcov overlay instead: recompile ONLY
merkleblock.cpp.o with --coverage (same flags otherwise), `ar r`
replace the member in libbitcoin_common.a (ninja re-archives would
recompile WITHOUT coverage — order matters), relink test_bitcoin with
--coverage for the gcov runtime, run the suite, gcov the source. The
overlay is removed afterward (ninja rebuild of object+archive+binary;
verified 0 __gcov_init in the restored binary, tests still green).
Cost: ~2 incremental rebuilds, no new build dir.

### Baseline measurement (merkleblock_tests unit suite)
81/86 lines. Uncovered:
- :60-61 IsRelevantAndUpdate match + vMatchedTxn append —
  MISSING SCENARIO (filter construction covered only functionally,
  p2p_filter.py).
- :115 fBad (traversal out of flag bits), :123 fBad (out of hashes),
  :140 fBad (identical subtree hashes, CVE-2012-2459 guard) —
  MISSING NEGATIVE SCENARIO (malformed trees lived only in fuzz
  targets + the rpc_txoutproof.py battery from campaign #6 c2).
No genuinely-dead or unreachable-because-of-bug lines found; no
harness artifacts (the suite itself is sound, just thin).

### Closure (32d5d1dcc4)
One test case driving three crafted serializations through
ExtractMatches (bits-exhaustion, hash-exhaustion, duplicate subtree)
+ well-formed control, and one case with a saturated tiny-bitfield
bloom filter covering the match/vMatchedTxn path.
Fixture subtlety (worth remembering): a packed byte is 8 flag bits —
exhausting the BIT budget needs an 8-tx tree with an all-parent left
half (0b00100111); the first fixture version (4-tx) tripped the HASH
budget at :123 instead, silently re-covering the wrong branch. gcov
branch output was used to detect that.
After closure: `Lines executed:100.00% of 86`, zero ##### markers.

### Verdict
- All 5 uncovered lines classified (missing-scenario family, none
  dead/unreachable/harness-artifact) and closed with narrow,
  assertion-rich tests. Campaign rule honored: tests target real
  behavior (malformed-tree rejection, filter matching), not line
  counts.

### Exact commands
- overlay: ninja -t commands | grep merkleblock.cpp.o, re-run with
  `--coverage` inserted before -o; `ar r lib/libbitcoin_common.a <obj>`;
  relink via ninja link command + `--coverage` before `-o bin/test_bitcoin`
- measure: `gcov -b -o src/CMakeFiles/bitcoin_common.dir/merkleblock.cpp.o
  /mnt/my_storage/bitcoin/src/merkleblock.cpp`
- restore: `ninja lib/libbitcoin_common.a bin/test_bitcoin`

### Limitations
- Unit-suite scope only; functional (rpc_txoutproof.py, p2p_filter.py)
  and fuzz coverage of the same file not merged into the number.
- Branch coverage (as opposed to line) not driven to 100% — the
  BitsToBytes padding Assume branches and some throw edges remain.
- Method is per-module; a line/range ledger for future cycles starts
  here: merkleblock.cpp DONE (unit). Next candidates by value:
  src/node/blockstorage.cpp load paths, src/coins.cpp.

### Next queue for this campaign
- blockstorage.cpp LoadBlockIndex guards (session context: fork-added
  corruption guards e83b385117/6c6d91ce99 — check their unit coverage).
- coins.cpp MoneyRange assert paths (61e8c5138d lineage).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): c1 backport (union with F2) + two coverage cells dismissed

### Draw
Random draw over the 9-goal eligible pool (8 pending + 1 CYCLE-1,
#42 excluded as just-cycled): raw=2032011409874351500, index 1 ->
#34. Ledger had NO row; audit/uncovered-code holds a complete c1
(32d5d1dcc4 + journal 4a9eef4b4b) stranded off-lineage. Branch:
audit/uncovered-code-c2 from ba01969939 (#42 c1 bookkeeping).

### Backport with semantic conflict resolution
Cherry-pick of 32d5d1dcc4 CONFLICTED in merkleblock_tests.cpp: HEAD
already carried F2's two tests (84a3913096, #66 c2 backport:
duplicate-branch-hashes, trailing-bits-beyond-padding) and c1's
commit added its own at the same anchor. Resolution: UNION, because
the shapes are distinct, not duplicative — F2 covers identical-txid
and duplicated-subtree cases; c1 adds traversal starvation
(out-of-bits/out-of-hashes fBad arms), the crafted
right-subtree-hash-equals-computed-left case (Hash(h,h)), and the
bloom-filter match-all path (IsRelevantAndUpdate). Rebuilt;
test_bitcoin --run_test=merkleblock_tests -> No errors detected.
Journal cherry-picked as d794142ede (usual uber-rotation.md
resolution).

### Cell 1 (c1 queue): blockstorage corruption guards — COVERED, dismissed
Both fork guards carry dedicated regression tests, present and green
at HEAD: blockmanager_negative_last_block_file_rejected
(blockmanager_tests.cpp:470) and
blockmanager_permuted_disk_heights_rejected (:498); full
blockmanager_tests suite -> No errors detected. No coverage gap.

### Cell 2 (c1 queue): coins.cpp MoneyRange assert paths — GUARDED, dismissed
The three assertion sites (coins.cpp:103 AddCoin, :164, :301
BatchWrite; 61e8c5138d lineage) are caller-contract guards. The
untrusted-input boundary — corrupt coins-DB records — cannot reach
them with an invalid amount: AmountCompression::Unser throws
"amount out of range" for any decompressed value > MAX_MONEY
(compressor.h:115-117), so DB reads fail by exception before any
invalid Coin exists (domain-enforced, per #98's audit). The fuzz
targets enforce the contract at generation (commit message's
coins_view* verification). No coverage gap; a death-test for the
asserts would add nothing (they are unreachable by construction).

### Exact commands
- git cherry-pick 32d5d1dcc4 (conflict) -> union resolution ->
  068152320f; git cherry-pick 4a9eef4b4b -> d794142ede
- test_bitcoin --run_test=merkleblock_tests / blockmanager_tests
- reads: compressor.h:102-118, coins.cpp:103/164/301,
  blockmanager_tests.cpp:470/498

### Limitations / queue for cycle 3
- c1's method note stands: unit-suite scope only; functional/fuzz
  coverage of merkleblock.cpp not merged into the 86/86 number.
- Branch coverage (BitsToBytes padding Assume arms) still <100%.
- New candidates: compressor.cpp decompress paths under corruption
  (unit-level), dbwrapper record-boundary reads.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 3 (2026-07-29): ScriptCompression malformed-stream arms — 3 arms closed, mutation-verified

### Draw
Re-rank draw over the rebuilt 9-cell queue (after #23 c3):
raw=523542359493460158, index 6 -> #34 (third cycle; c2 queue cell
"compressor.cpp decompress paths under corruption"). Branch:
audit/uncovered-code-c3 from 54e0d54ef8 (#23 c3 bookkeeping).

### Cell
ScriptCompression::Unser (compressor.h:81-101) has three
corruption arms with no oracle: (a) nSize beyond MAX_SCRIPT_SIZE
degrades to OP_RETURN + ignore (upstream-intended: corrupt record
becomes unspendable, never a 4GB allocation); (b) the same
oversized id on a truncated stream must throw (SpanReader::ignore
end-of-data); (c) truncated special-script body must throw.
DecompressScript's invalid-pubkey and bad-id arms were already
covered (compress_tests.cpp:160-192); the VARINT reader overflow
arms are O2's battery.

### Change (test-only)
compress_tests.cpp: new script_compression_malformed_stream_arms
with the three arms (OP_RETURN substitution + full consumption;
truncated-oversized throws; truncated-special throws).
Harness lesson: `ss << junk` writes a CompactSize length prefix —
raw bytes need `ss << std::span{junk}` (the first draft failed
ss.empty() by exactly the 3-byte prefix).

### Verification
- compress_tests green (8 cases).
- Mutation: dropping the MAX_SCRIPT_SIZE guard (if(false)) makes
  arm 1 fail exactly at the OP_RETURN check (unguarded path
  resized to 10001 bytes); restored, re-ran green.

### Verdict
CONFIRMED oracle delivered (3 previously-unpinned arms,
mutation-verified). No production defect: all three paths were
already safe-by-construction (bounded reads, throw-on-short) —
this closes the coverage classification.

### Exact commands
- cmake --build build-before -j4 --target test_bitcoin
- test_bitcoin --run_test=compress_tests[/
  script_compression_malformed_stream_arms]
- mutation: compressor.h guard drop -> :206 failure -> revert

### Limitations / queue
- DecompressScript special types 0x02/0x03 (compressed-pubkey
  bodies) get no distinct malformed arm (fixed 32-byte read; the
  truncated-special arm covers the class).
- dbwrapper record-boundary reads (c2 queue) still open.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-07-29): dbwrapper record-boundary reads — layers safe; warmup asymmetry = author's PR 35654 pending

### Draw
Re-rank draw over the rebuilt 7-cell queue:
raw=4758704522666985144, index 2 -> #34 (fourth cycle; c2 queue
cell "dbwrapper record-boundary reads"). Branch:
audit/uncovered-code-c4 from 8b553eee19 (#43 c3 bookkeeping).

### Boundary layers (all safe-by-construction, verified across cycles)
- LevelDB record framing + CRC (vendored, upstream domain).
- CoinEntry key parse (txdb.cpp:49-55): key(uint8) + hash + VARINT
  — fixed-layout + O2-batteried reader; SpanReader throws on short.
- Coin value parse: TxOutCompression — AmountCompression throws on
  out-of-range (#98/c3), ScriptCompression degrades/throws (c3).

### The one asymmetry (documented, parked)
Cursor() warmup (txdb.cpp:253-258) sets keyTmp.first = entry.key
UNCONDITIONALLY when the leveldb iterator is valid, ignoring
GetKey's decode-failure return; Next() (:289-296) invalidates
(keyTmp.first = 0) on the same failure. So a corrupt-chainstate
edge — a malformed first DB_COIN key — reports a valid cursor
position with a degenerate cached key, inconsistent with
mid-iteration behavior. This is exactly upstream PR 35654
(l0rinc/txdb-malformed-first-coin-cursor-key, commit 3837d9192a,
with unit test), pending upstream. The rotation does not adopt the
fork author's own PR-shaped work; recorded as the reference.
Master-relative severity: none (corrupt-DB-only; the value read
would hit the compression guards anyway).

### Verdict
DISMISSED: boundary reads are safe; the warmup inconsistency is
small, corrupt-only, and owned by the author's pending PR.

### Exact commands
- reads: txdb.cpp:40-57/246-296, dbwrapper.h:166
- git show 3837d9192a (branch remotes/l0rinc/l0rinc/txdb-
  malformed-first-coin-cursor-key; remotes/pr/35654)

### Limitations / queue
- Branch coverage of merkleblock.cpp padding Assume arms (c1 note)
  remains the only open #34 cell.
- The 35654 test should be re-run if the PR lands upstream.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 5 (2026-07-31): merkleblock BitsToBytes padding arms — dedicated arm-level test, mutation-killed; cell CLOSED

### Draw
RE-RANK draw 135 over the 7-cell queue: raw=12699529126198122192,
masked 3476157089343346384 -> idx 4 -> #34 merkle Assume arms cell.

### Cell
Branch coverage of BitsToBytes' padding invariant arms
(merkleblock.cpp:23-26, fork-added Assume): both arms of
`if (used_bits)` (bit-count % 8 == 0 vs != 0) plus round-trip
identity and padding-decodes-false.

### Work
- New test bits_bytes_padding_arms (merkleblock_tests.cpp): sizes
  0..17, forced set bit adjacent to padding, asserts output size ==
  CeilDiv, padding mask clear, BytesToBits round-trip, padding
  positions decode false. Behavior-pinned independently of the
  Assume macro's build-mode semantics (Release NDEBUG makes Assume a
  hint; the BOOST checks guard the invariant directly).
- Suite: merkleblock_tests green (build-before, Release).
- MUTATION CHECK: ret initialized 0xff instead of 0 (padding bits
  set) -> killed by BOTH the new test (129 failures incl. the mask
  check [254 != 0]) AND the pre-existing c1 test
  merkleblock_bit_byte_roundtrip_padding. So the invariant was
  already behaviorally covered; the c1 "branch coverage <100%" note
  was sancov Assume-line granularity, not a behavioral gap. The new
  test closes it as an explicit, named arm-level test.
- Mutant reverted; suite re-verified green; tree carries only the
  test addition.

### Verdict
Cell CLOSED (coverage gap was instrumentation-granularity, now
pinned by a dedicated mutation-verified test). No production defect.

### Limitations / queue
- sancov re-measurement not run (build cost; the mutation kill is
  the stronger evidence form).
- #34 queue now: only "re-run 35654 test if the PR lands upstream"
  (external watch). All in-tree cells closed.

## Rotation note
Cycle 5 complete; rotating per uber-goal policy. Effectively
exhausted modulo the external 35654 watch.
