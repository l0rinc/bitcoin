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
