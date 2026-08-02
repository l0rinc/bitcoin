# Campaign #28 — weak-test-oracles (mutation-survival audit)

Base: audit/resurrection @ 1812399b33 (rotation ledger commit for #19).
Branch: audit/weak-test-oracles. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-27): amount-compression oracle proven mutation-hard — DISMISSED (no weak oracle)

### Scope chosen
Priority areas per campaign: recent fixes, consensus/persistence-adjacent tests.
Two-pronged: (A) survey of classic weak-oracle shapes in recent/hot test code;
(B) an actual mutation battery against one consensus-feeding target
(`CompressAmount`/`DecompressAmount`, UTXO-set serialization) whose suite
(`compress_tests`) had not been mutation-probed before.

### (A) Weak-shape survey — all candidates dismissed as strong
- `BOOST_CHECK_NO_THROW` uses (40 across src/test): sampled sync_tests.cpp:56,
  util_check_tests.cpp:27 — both are the negative branch of debug-only
  behavior checks, paired with positive exception assertions. Strong.
- Recent fix regression tests:
  - bitcoin/bitcoin#35550 (sendcmpct announce field): test asserts both the
    debug-log reason AND the disconnect (`send_await_disconnect`). Strong.
  - bitcoin/bitcoin#35070 (FindMostWorkChain UB): dedicated coverage commits
    (ca4a380281, 0852925bd8). Strong.
- Sleeps in unit tests: blockfilter_index_tests.cpp:495 inside a
  deadline-bounded wait loop with BOOST_FAIL on timeout;
  checkqueue_tests.cpp:433 race-window widener with atomic-counter oracle;
  httpserver_tests.cpp sleeps are timing tolerances with state assertions.
  All legitimate.
- Broad catches (`catch (...)` in sighash_tests/transaction_tests): harness
  guards for malformed JSON test vectors, both BOOST_ERROR on hit. Legitimate.
- `5d651df9a4` (tolerate no-op tip updates): relaxed check still verifies the
  dangling window is the final tip or its disconnected descendant via
  LastCommonAncestor. Justified, oracle retained.

### (B) Mutation battery: CompressAmount/DecompressAmount vs compress_tests
Existing oracle (src/test/compress_tests.cpp): 6 exact TestPairs,
TestEncode round-trip over 4 ranges (1..100k sat; 0.01..100 BTC by CENT;
1..10000 BTC; 50..21M BTC by 50 BTC), TestDecode round-trip over encodings
0..99999, plus MAX_MONEY accept and MAX_MONEY+1/UINT64_MAX reject cases.

Mutants (one at a time, `cmake --build build-before --target test_bitcoin`,
`timeout 120 ./build-before/bin/test_bitcoin --run_test=compress_tests`,
revert via `git checkout src/compressor.cpp` between runs):

| mutant | location | result | kill mechanism |
|---|---|---|---|
| M1 `e < 9` -> `e < 8` | CompressAmount trailing-zero loop | KILLED (exit 124) | internal `assert(d >= 1 && d <= 9)` aborts; Boost catches SIGABRT, module red |
| M2 `(x % 9) + 1` -> `(x % 9)` | DecompressAmount digit recovery | KILLED (exit 201) | value failures across suite |
| M4 `+ 9` -> `+ 8` | CompressAmount e==9 branch | KILLED (exit 201) | value failures (rerun with Edit after a sed-escaping failure) |
| M5 `n = x+1` -> `n = x` | DecompressAmount e==9 branch | KILLED (exit 201) | value failures |
| M6 `while (e)` -> `while (e > 1)` | DecompressAmount multiplier loop | KILLED (exit 201) | value failures |

5/5 killed. The oracle (round-trip loops + exact pairs + internal assert) is
mutation-hard for branch-inversion, off-by-one, bound-alteration, and
skipped-update shapes. Clean-tree control run: exit 0, ~seconds.

### Verdict
- DISMISSED (no weak oracle found). Journal-only commit; no code change.
- The amount-compression round-trip suite earns a "strong" mark; no
  additional properties needed for the probed mutant classes.

### Unrelated lead (test infrastructure, not production)
Under M1 the internal assert fired repeatedly; after the Boost-caught
SIGABRT, the NEXT test case's `BasicTestingSetup` ctor deadlocked in
`ArgsManager::ClearPathCache` (futex word stuck at 2 on `gArgs.cs_args`,
single-threaded, `futex_wait_queue`; gdb bt:
ClearPathCache <- BasicTestingSetup ctor <- test invoker). Consistent with
the signal interrupting a cs_args-holding path and skipping the unlock.
Reproduced twice under the mutant; never on a clean binary. Only observable
when an internal assert already fired (suite already red), so impact is
limited to wall-clock (mitigated here by `timeout 120`). Not pursued further
this cycle; noted for the error-path/locking campaigns.

### Exact commands / key output
- Battery script: sed-apply -> build -> `timeout 120 test_bitcoin
  --run_test=compress_tests` -> classify by exit code -> `git checkout`
  revert. M4 rerun via Edit tool (BRE `*` escaping defeated sed).
- Battery log: `M1_elimit8: killed (exit 124; Assertion 'd >= 1 && d <= 9'
  failed.)` / M2,M5,M6 `killed (exit 201; failures are detected...)` /
  M4 `exit: 201`.
- Control: clean rebuild `green exit: 0`.

### Limitations
- Battery covered 5 hand-picked mutants on one file; not a systematic Mull
  run (Mull not installed on this host — named prerequisite for a future,
  broader mutation campaign).
- TestDecode covers encodings only to 99999; larger non-canonical encodings
  are exercised only via TestEncode round-trips. Acceptable: no mutant
  reached for that gap.

### Next queue for this campaign
- Mutation-probe another consensus-feeding suite with a different shape
  (e.g. `src/test/merkleblock_tests.cpp` partial-Merkle-tree edge branches,
  or `src/test/coins_tests.cpp` serialization paths) — one battery per cycle.
- If a broader sweep is wanted: defer until Mull or equivalent is available
  (prerequisite: mutation-toolchain install on this host).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-28): merkleblock extraction-guard battery — 2 SURVIVORS fixed with regression tests, 1 equivalent mutant

### Target
CPartialMerkleTree::ExtractMatches (merkleblock.cpp) — consensus-feeding,
runs on ATTACKER-CONTROLLED trees from the P2P merkleblock message.
Pre-cycle suite: 4 test cases (reject-invalid-refs, bit/byte padding,
construct-found, construct-not-found) — thin on extraction guards.

### Battery (one mutant at a time; build-before; `--run_test=merkleblock_tests`)
| mutant | guard | result |
|---|---|---|
| M1 remove sibling-dup check (TraverseAndExtract right==left -> fBad) | duplicate-txid merkle ambiguity (2014, 012598880cf; CVE-2012-2459 lineage) | SURVIVED (exit 0) |
| M2 remove bit-consumption check (CeilDiv equality, line 203) | trailing bits beyond byte padding | SURVIVED (exit 0) |
| M3 remove early vBits<vHash guard (line 189) | redundant? | SURVIVED but EQUIVALENT (see proof) |
| M4 CalcHash right-copy break | control | not run (battery complete at 2 findings) |

M3 equivalence proof: every vHash consumption in TraverseAndExtract is
preceded by a vBits consumption (lines 118 then 126), so bits < hashes
always trips either the traversal-time bits-overflow (fBad) or the
hash-consumption check (line 206). No test needed; recorded separately
per campaign rule for equivalent mutants.

### Fixes (one commit, 50e9d14750 — same guard family, same file)
- merkleblock_reject_duplicate_branch_hashes: 2-tx identical-txid tree
  and 4-tx duplicated-inner-subtree tree; asserts ExtractMatches null.
  With M1: 2 errors at merkleblock_tests.cpp:132,139. Reverted: green.
- merkleblock_reject_trailing_bits_beyond_padding: deserializes an
  otherwise-valid 2-tx tree (nTransactions=2, vHash=[x,y], vBits bytes
  {0x03,0x00} = valid 3 bits + full trailing byte); asserts null.
  With M2: exactly 1 error at merkleblock_tests.cpp:162. Reverted: green.
- Whole suite green on HEAD: `*** No errors detected`.

### Verdict
- CONFIRMED x2 oracle gaps on consensus-reachable untrusted-input guards.
  FIXED test-side; production code byte-identical (mutants reverted).
- Two verifier forms for the remote-reachable class: mutant survival
  evidence + crafted-input failing-before/passing-after tests.

### Next queue for this campaign
- coins_tests serialization-path battery (CTxOutCompressor/undo formats)
  — note: #18 c2 already closed the Coin composition round-trip; remaining
  is undo-data (CTxOutUndo) shapes.
- script_flags / sighash flag-combination guards (consensus-feeding).

## Rotation note
Two bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-08-02, draw 218, raw=8871967550067354166 (63-bit), idx 5/13): sighash flag-combination guard mutation sweep — mutant KILLED 409x by the script vector corpus; guard thoroughly covered; DISMISSED

### Hypothesis
The consensus-feeding hashtype guard (interpreter.cpp:202-211
IsDefinedHashtypeSignature: mask ANYONECANPAY, require
ALL..SINGLE) might be under-guarded by the two visible tests
(script_tests.cpp:642/:648).

### Mutation evidence
M: range upper bound weakened (nHashType > SIGHASH_SINGLE ->
nHashType > 5, silently accepting invalid types 4 and 5):
- script_tests: 409 FAILURES — the hashtype-4/5-accepting
  mutant breaks hundreds of script vector cases (the corpus's
  positive vectors with DER+hashtype tails depend on the exact
  range, not just the two named reject tests).
- Restore -> 'No errors detected'; tree byte-identical
  (interpreter.cpp.bak restored, git status clean).

### Verdict
DISMISSED: the sighash flag guard is among the most densely
covered guards in the suite (vector-corpus depth, 409x kill
factor). No oracle gap; no battery warranted.

### Exact commands
- sed mutant (backup/restore recorded); cmake build;
  --run_test=script_tests both directions above.

### Limitations / queue
- The mask arm (dropping ~ANYONECANPAY) not separately mutated
  (the positive 0x81-0x83 vectors across the corpus cover it by
  the same depth).
- undo-data shapes cell remains queued (f11c3ec1a2 lineage).
