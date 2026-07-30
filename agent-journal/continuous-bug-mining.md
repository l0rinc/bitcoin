# Continuous Evidence-First Bug Mining

## Cycle 142: chainstate metadata after block-file flush failure

- Selected index: `0`
- Selected slug: `continuous-bug-mining`
- Selector: `shuf -i 0-98 -n 1`
- Branch: `uber-cycle-142-continuous-bug-mining-20260730`
- Gate HEAD: `178b7e160b26fe4974baefdd4a810dfdfdc4bdfc`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `1068 40`
- Catalog, goal TSV, and uber-protocol hashes matched the authoritative values in `uber-goal-state.md`.

### Scope and prior evidence

The cycle searched the accumulated bug-mining journals, recent history, TODOs, and related goal ledgers before selecting a new cell. Previously closed cells were excluded: wallet `setlabel` write failure, duplicate descriptor expansion, RPC authentication-cookie replacement, and earlier chainstate/index persistence audits. The remaining persistence queue included flat block/undo flush ordering, metadata publication after a failed flush, and ignored directory-sync errors.

The strongest seed was the live TODO in `Chainstate::FlushStateToDisk()` immediately after `FlushChainstateBlockFile()`. Historical commits `f0207e0030` and `f562856d02` added the return value and explicitly described the risk of writing block-index metadata after an I/O failure, but the current code still logged the failure and continued. `BlockManager::FlushBlockFile()` calls `flushError()` on block or undo failure, yet the synchronous caller continued to write block-index and chainstate metadata and could emit `ChainStateFlushed`.

### Hypothesis and trust boundary

An untrusted or local filesystem failure while flushing `blk*.dat` or `rev*.dat` can leave the durable block data behind the in-memory chain tip. The chainstate flush must return an error before `WriteBlockIndexDB()`, the coin cache flush, and `ChainStateFlushed`; publishing those later states would claim durability that was not established. The trust boundary is the filesystem result consumed by block validation and persistence code, not network input.

### Independent reproduction

The production binary was exercised on a scratch regtest datadir copied from a 110-block baseline. `/data/my_storage/tmp/cycle142_fail_block_sync.so`, loaded with `LD_PRELOAD=...`, returned `EIO` from `fsync`/`fdatasync` for `blk*.dat` and `rev*.dat`. A forced shutdown logged failed block and undo commits, the fatal flush notifications, and the old `FlushStateToDisk: Failed to flush block file` warning while the synchronous call continued. The hook, datadirs, and logs are outside the repository under `/data/my_storage/tmp/cycle142-*`; no default datadir, wallet, or production database was used.

The permanent regression uses Linux's `/sys/kernel/uevent_seqnum` as a deterministic open-failure target. It moves the test fixture's `blk00000.dat` aside, creates a symlink at the original path, and restores the file with an RAII guard. The normal blocks directory remains present, so disk-space checks pass; opening the symlink returns false without throwing. A validation subscriber records whether `ChainStateFlushed` was emitted.

### Before/after evidence

- Pre-fix build: `CCACHE_DIR=/data/my_storage/tmp/cycle142-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j4` passed.
- Pre-fix command: `set -o pipefail; TMPDIR=/data/my_storage/tmp/cycle142-test-tmp-prefixed /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=chainstate_write_tests/flush_failure_stops_metadata_publication --log_level=test_suite 2>&1 | grep -E 'Entering test case|Leaving test case|error:|fatal error|Failure|failure|Error:|No errors'`.
- Pre-fix result: exit `201`; `!flushed`, `state.IsError()`, and `!sub->m_did_flush` each failed. The log included `Flushing block file to disk failed. This is likely the result of an I/O error.`
- Fix: `Chainstate::FlushStateToDisk()` now returns `state.Error("Failed to flush block file.")` immediately after logging a false `FlushChainstateBlockFile()` result. The existing notification has already reported the underlying I/O failure, so no second fatal notification is introduced.
- Post-fix build: the same `test_bitcoin` target passed.
- Post-fix command: the same focused command with the fixed binary returned exit `0`, logged the expected flush error, and reported `*** No errors detected`.
- Broader checks: `--run_test=chainstate_write_tests --log_level=test_suite` passed with no errors; `--run_test=validation_block_tests/processnewblock_new_block_flag_write_failure --log_level=test_suite` passed with no errors. The latter emitted its expected filesystem-fault diagnostic from the existing test.

### Change and verification

The source change is in `src/validation.cpp`; the focused Linux regression is in `src/test/chainstate_write_tests.cpp`. The fix prevents block-index/coin-state publication and the flush callback after a failed block or undo-file flush. The focused test checks the return value, error state, and callback absence rather than only checking a log message.

`git diff --check` passed after the final source/test edits. The test binary was rebuilt after both the pre-fix and post-fix source states. No full current-tree suite was started because the persistent unrelated wallet test process with PID `777094` must remain untouched; the relevant chainstate and block-write suites were run in isolated test temp directories.

### Dismissed and inconclusive candidates

- The existing `net_processing.cpp` optimistic compact-block TODO was reviewed but remains an intentional best-effort path; no distinct failure contract was proven.
- `DirectoryCommit()` still ignores directory `fsync()` failure. This is a separate filesystem-ordering hypothesis and remains queued; this cycle did not claim that the block-file fix covers directory metadata durability.
- Wallet address-book enrichment and descriptor label failures remain contract-sensitive candidates, not reopened findings.
- The duplicate-descriptor resource-amplification fix remains closed for exact duplicates; semantically equivalent but textually different descriptors need a separate non-flaky oracle.

### Verdict and handoff

**Confirmed and fixed.** The old code continued the persistence sequence after a false block-file flush result. The next cycle must perform a fresh gate, preserve all unrelated untracked artifacts and PID `777094`, execute exactly `shuf -i 0-98 -n 1`, and choose a distinct evidence cell. The next persistence queue is directory-commit failure ordering, followed by other atomic rename and cross-index crash-symmetry paths.
