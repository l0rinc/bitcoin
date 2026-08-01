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

## Cycle 251

### Draw and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `2`
- Selected goal: `assertion-invariant-audit`
- Worktree: `/data/my_storage/bitcoin`
- Branch: `uber-cycle-251-assertion-invariant-audit-20260801`
- Cycle gate HEAD: `f8f17a4996c4f2c187b4d0e05e2c6377f08e423b`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `42 1287`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Protected long-running processes were preserved. The data volume was initially full; five stale, non-running build directories were removed from scratch storage before testing, leaving the source tree and protected runs unchanged.

### Scope and hypothesis

The previous selected cycle closed the GETBLOCKTXN read-failure assertion and older DifferenceFormatter/V2 cells. This cycle re-ranked current non-test `assert`, `Assume`, `CHECK_NONFATAL`, and `VERIFY` sites by whether an external message or persisted state can reach the assumption before the next dereference or state transition. The primary candidates were the two `PeerManagerImpl::SendMessages` header-relay `assert(pindex)` sites and the stateful `TxDownloadManagerImpl` peer/request/orphan cleanup assumptions.

The falsifiable hypothesis was that a peer-controlled hash, reorg/prune transition, disconnect sequence, or malformed transaction could violate one of these invariants, causing a release-relevant abort, an unsafe continuation after a disabled assertion, or retained per-peer state.

### Status

Cycle 251 is complete and dismissed: no current invariant defect or source/test change was established. The next queue remains open for a fresh assertion surface; this cycle must not reopen the GETBLOCKTXN, DifferenceFormatter, V2 transport, or already-covered txdownload contract cells without new evidence.

### Header-relay assertion audit

`PeerManagerImpl::UpdatedBlockTip()` is the sole production writer of `Peer::m_blocks_for_headers_relay`. It walks `pindexNew` back to `pindexFork`, caps the vector at `MAX_BLOCKS_TO_ANNOUNCE`, and appends each `CBlockIndex::GetBlockHash()` under `m_block_inv_mutex` (`src/net_processing.cpp:2330-2361`). The two later assertions at `src/net_processing.cpp:6323` and `6413` look up those hashes under the same peer/block state and assume a non-null `CBlockIndex`.

`BlockManager::LookupBlockIndex()` is a direct lookup in `m_block_index` (`src/node/blockstorage.cpp:222-234`). No production code erases or clears `m_block_index`; pruning removes block and undo files, while reorg changes active-chain membership only. Therefore a reorg can make the active-chain comparison false, which the surrounding code already converts to INV fallback, but cannot make a hash originating from `UpdatedBlockTip()` disappear from the block-index map. Delayed validation callbacks likewise retain stable map entries. Peer bytes do not write this vector. This is a rigorous caller/dataflow dismissal, not evidence that arbitrary `LookupBlockIndex()` calls are safe.

### Transaction-download invariant audit

`TxDownloadManagerImpl::DisconnectedPeer()` removes the peer from both `TxOrphanage` and `TxRequestTracker`, erases its registry entry, updates the wtxid-relay count, and then checks zero request count, orphan usage, announcements, latency score, and reconsideration work (`src/node/txdownloadman_impl.cpp:225-241`). The `CheckIsEmpty` methods cover the per-peer and global forms of the same contract. The existing history commit `6c5edd9f51` already added the production assumptions, fuzzer oracle, and deterministic disconnect test; the current production implementation is unchanged since that contract commit. The source-only diff after it is test-side coverage for a combined txid/wtxid NOTFOUND response, not a missing invariant guard.

The stateful fuzzer was run with the existing `cycle131-build-libfuzzer/bin/fuzz` binary and a fixed environment/seed:

- `FUZZ=txdownloadman_impl ... -runs=20 -max_len=256 -seed=25121` completed 20 executions, added 2 units, and reached coverage 7457 with no diagnostic; peak RSS was 2555 MiB.
- `FUZZ=txdownloadman_impl ... -max_total_time=30 -max_len=4096 -seed=25122 /data/my_storage/tmp/cycle131-corpus` completed 753 executions, added 158 units, reached final coverage 16737 / feature count 43079, and had no assertion, sanitizer, timeout, or artifact; peak RSS was 2557 MiB.
- `TMPDIR=... /data/my_storage/tmp/cycle243-build/bin/test_bitcoin --run_test=txdownload_tests` passed all 14 selected cases and 605 assertions.

The fuzz binary is an unstripped debug-instrumented ELF without an ASan runtime, so this cycle treats the run as a state-machine/assertion oracle rather than memory-safety evidence. The prior `6c5edd9f51` commit records independent ASan replay and a mutation proof: removing `EraseForPeer(nodeid)` makes the disconnect regression abort at the first orphan-usage assumption. No current failure survived the executable or caller/dataflow checks, and no commit is justified.

### Limitations and handoff

The 30-second seed corpus was a small existing corpus rather than a target-specific `txdownloadman_impl` corpus; it still exercised the target's peer, request, orphan, package, and workset branches, as shown by newly reached target symbols. The header-relay proof covers the current production callback/dataflow and does not claim that future test-only direct mutation of the peer vector would be safe. Host disk pressure prevented a new sanitized build this cycle; protected existing sanitizer processes were not disturbed.

Validation commands and key output are recorded above. No product or permanent test file changed. The next cycle must recheck disk/process state, draw a fresh goal, and choose a distinct unchecked invariant.
