# Assertion, Assume, and Invariant Reachability Audit

## Cycle 68

### Draw and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `2`
- Selected goal: `assertion-invariant-audit`
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Cycle gate HEAD: `0c8c5ad2d8ffcf1468d46339608efd42d37fb1c1`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `HEAD...origin/master` was `908 2` at the gate.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Tracked and staged state was clean except known untracked agent artifacts; no relevant process was running.

### Scope and initial hypothesis

The prior AddrMan `Select` assertion-oracle correction, the MuSig argument-ordering fix, and earlier lock/lifetime cells are excluded. This cycle opens the distinct release-reachable question of whether an `Assume`, `assert`, `CHECK_NONFATAL`, or `VERIFY` statement is being used as the only guard for data arriving through RPC, configuration, network, persisted state, fuzz entry points, or optional-module boundaries.

The falsifiable hypothesis is that at least one current assumption is reachable with an invalid or merely unusual production input, or that a release-disabled assertion removes a check required by a later dereference, cast, indexing operation, state transition, or public output contract. The trust boundary includes malformed serialized data, rejected-but-parsed API values, partially initialized objects, restart/recovery state, and callers compiled with different debug/VERIFY/configuration settings.

### Status

Cycle 68 is complete: one release-reachable assertion defect was confirmed and fixed; the other inspected candidates were dismissed or left as bounded contracts.

### Method

For each candidate, record the exact invariant, all callers, the build mode in which it is active, and the first operation that relies on it. Compare source, history, tests, docs, fuzz harnesses, and release configuration. Try to falsify the invariant with boundary values, malformed persisted/network inputs, error returns, and lifecycle transitions. A candidate cannot become a finding without a failing-before proof, first-invalid-operation trace, mutation-sensitive regression, or a rigorous caller/dataflow proof.

## Next queue

1. Inventory non-test `Assume`, `assert`, `CHECK_NONFATAL`, `VERIFY`, and unreachable branches, then rank by untrusted reachability and consequence.
2. Verify the highest-risk release and optional-module cells with focused tests and one independent mutation or negative control.
3. Lock each candidate as confirmed, dismissed, or inconclusive before selecting the next distinct invariant.

## Candidate ledger and evidence

### DifferenceFormatter and transaction-request invariants

The `TransactionsRequest` deserializer and `DifferenceFormatter` paths validate increasing indexes, the CompactSize cardinality limit, and the shift-width boundary before indexing or shifting. The focused command
`TMPDIR=/data/my_storage/tmp/assertion-invariant-cycle68-tests build_unit_clang19/bin/test_bitcoin --run_test=blockencodings_tests/TransactionsRequest* --random=6806`
passed five cases covering round-trip serialization, non-increasing indexes, the maximum index, the 65536/65537 cardinality boundary, and shift overflow. The corresponding `blocktransactionsrequest_deserialize` libFuzzer target completed 1,000 runs with coverage 490 and no diagnostic. Removing the `m_shift > std::numeric_limits<I>::max()` guard was a negative control: `TransactionsRequestDeserializationOverflowTest` aborted at the increasing-index assertion. The guard is therefore a valid internal invariant and is not the selected finding.

### V2 transport assertions

The inspected V2 transport state assertions are downstream of the transport state machine and its authenticated handshake transitions, not direct validation of peer-controlled payload bytes. Existing network tests and the `v2_transport` fuzz path cover invalid framing and transition failure. No independent release-reachable assertion failure was reproduced in this cycle; this candidate is dismissed for the current evidence and remains closed unless a new state-construction path appears.

### GETBLOCKTXN block-read assertion

The selected defect was in `PeerManagerImpl::ProcessMessage`, `GETBLOCKTXN`. After releasing `cs_main`, the code read a recent block and asserted `ret` because the block could not be pruned at that height. That argument excludes pruning only; `BlockManager::ReadBlock` can still return false for local I/O failure, truncated or corrupted bytes, invalid proof-of-work, a bad signet solution, or a block-hash mismatch. The trust boundary is a network peer request combined with local persisted block data, and the first invalid operation was the `assert(ret)` itself in a release-reachable P2P handler.

History identified commit `613a45cd4b5` as the introduction of the post-`cs_main` assertion. The existing `blockmanager_readblock_hash_mismatch` unit test independently proves that `ReadBlock` can fail on an indexed on-disk hash mismatch, while `ProcessGetBlockData` already handles the same class of failure by logging and disconnecting the peer. A scratch functional probe generated two regtest blocks, XOR-decoded the block file, flipped one header byte, and issued `getblocktxn` for the affected block. On the unpatched binary the node exited after receiving the request; the debug log contained `received: getblocktxn (34 bytes)`. This is a failing-before proof of a remotely triggerable daemon abort when the local block file is damaged.

The fix in `src/net_processing.cpp` removes the assertion and applies the established failure contract: log `Cannot load block from disk`, mark only the requesting peer for disconnect, and return without sending transactions. `test/functional/p2p_compactblocks.py` now preserves the generated block file's obfuscation, flips a decoded header byte, asserts the error log and peer disconnect, and checks that the daemon process remains alive. The same direct probe against the fixed binary printed `Node remained alive after getblocktxn: True` and completed successfully.

Validation:

- `cmake --build build_unit_clang19 --target test_bitcoin -j2` passed.
- `cmake --build build_func_clang19 --target bitcoind -j2` passed.
- `build_unit_clang19/bin/test_bitcoin --run_test=blockmanager_tests/blockmanager_readblock_hash_mismatch* --random=6815` passed.
- `build_fuzz_libfuzzer_clang19/bin/fuzz -runs=1000 -seed=6807 /data/my_storage/tmp/qa-assets/fuzz_corpora/difference_formatter` passed with coverage 574.
- `build_fuzz_libfuzzer_clang19/bin/fuzz -runs=1000 -seed=6808 /data/my_storage/tmp/qa-assets/fuzz_corpora/blocktransactionsrequest_deserialize` passed with coverage 490.
- `p2p_compactblocks.py --randomseed=6817` passed the full scenario, including the new corruption regression.
- `python3 -m py_compile test/functional/p2p_compactblocks.py` and `git diff --check` passed. The repository Python lint wrapper exited 0 while reporting that `lief` was unavailable and linting was skipped.

The source/test change is intentionally local to the P2P error path; it does not change consensus, pruning policy, or block validation. The regression uses a scratch functional datadir and local file corruption, so it proves daemon crash safety for persisted-read failure but does not claim to model every filesystem fault. ASan was not required for this non-memory-safety defect. The source/test commit and final cycle handoff are recorded in `uber-goal-state.md`.

## Next queue

1. Recheck the gate, hashes, dirty state, worktrees, and running processes.
2. Draw the next goal with `shuf -i 0-98 -n 1`, excluding only the closed cells documented in this journal and the ledger.
3. Start a new branch/cycle journal, search prior findings and review history, and continue with a distinct hypothesis.
