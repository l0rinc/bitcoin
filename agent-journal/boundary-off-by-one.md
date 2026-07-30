# Boundary and Off-by-One Audit: Cycle 21

## Cycle 150: flat-file position and chunk-allocation boundary

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `5`
- Selected slug: `boundary-off-by-one`
- Branch: `uber-cycle-150-boundary-off-by-one-20260730`
- HEAD before the cycle: `6d5af587439087ab9a488c254c9b3b44e129aaa6`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `1084 42` from `git rev-list --left-right --count HEAD...origin/master`.
- The tracked tree and index were clean; `git diff --check` passed. The catalog and uber-protocol hashes remained unchanged. PID `777094` (`test_bitcoin --run_test=wallet_tests`) and its parent were preserved. Existing untracked agent artifacts and scratch files were not touched.

### Scope and hypothesis

Cycle 129 closed the height, checkpoint, compact-filter, direct-fetch, and block-file practical cells. This cycle selected a fresh count/offset surface in `FlatFileSeq::Allocate()`, specifically:

```cpp
unsigned int n_new_chunks = CeilDiv(pos.nPos + add_size, m_chunk_size);
```

On a 32-bit build, `pos.nPos` (`uint32_t`) and `add_size` (`size_t`) can both be 32-bit unsigned values. At `pos.nPos == UINT32_MAX` and `add_size == 1`, the sum wraps to zero before chunk rounding. The corresponding 64-bit arithmetic reaches `2^32`, so this is a real platform-model difference rather than a comparison typo. The trust boundary is persisted/local file-position state and an allocation request, not a remotely supplied value.

### Production-bound analysis

The configured block-storage callers cannot reach the arithmetic boundary while retaining a valid file position. `MAX_BLOCKFILE_SIZE` is `128_MiB`; `FindNextBlockPos()` rotates when `nSize + nAddSize >= max_blockfile_size`, and the block-file cursor stores the resulting position. Undo files use `UNDOFILE_CHUNK_SIZE == 1_MiB`, and block-filter files rotate at `MAX_FLTR_FILE_SIZE == 16_MiB` before invoking the same helper. The block and undo additions are bounded serialized block sizes, while the filter path uses a `uint64_t data_size` and resets `pos.nPos` on file rollover. A position near `UINT32_MAX` therefore requires invalid/corrupt persisted metadata or an independent caller outside the configured production contracts.

The 32-bit arithmetic model was evaluated explicitly for `(pos, add_size, chunk_size) = (UINT32_MAX, 1, 16 MiB)`: the old chunk count is `256`, the wrapped 32-bit new count is `0`, and the widened 64-bit new count is `256`. This proves the portability concern, but not a reachable current Bitcoin Core defect. A 32-bit executable could not be built because the host lacks multilib C++ headers (`g++ -m32` failed in `<bits/c++config.h>`); no claim of executed 32-bit coverage is made.

### Verification

The existing flat-file unit command used a scratch `TMPDIR`:

```text
TMPDIR=/data/my_storage/tmp/cycle150-flatfile-unit /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=flatfile_tests --catch_system_errors=no --log_level=test_suite --report_level=short --color_output=false
```

It passed all 4 selected cases and 29 assertions, including exact allocation transitions at positions 0 and 99, read-only open behavior, and flush truncation. The fixed-seed libFuzzer replay used a separate scratch corpus and artifacts directory:

```text
FUZZ=flatfile /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz -runs=1000 -seed=15001 -rss_limit_mb=2048 -artifact_prefix=/data/my_storage/tmp/cycle150-flatfile-fuzz2/artifacts/ /data/my_storage/tmp/cycle150-flatfile-fuzz2/corpus -print_final_stats=1
```

It completed 1,000 runs, added 55 corpus units, and reported no sanitizer or contract failure. The fuzzer intentionally exercises positions only through 4096, so it is evidence for ordinary allocation contracts, not the unreachable 32-bit maximum. `git diff --check` passed.

### Verdict and handoff

Verdict: **dismissed for the configured production paths; portability limitation retained**. The arithmetic wrap is valid under a 32-bit platform model, but no supported current caller can supply a near-`UINT32_MAX` file position, and changing `out_of_space` semantics or adding a hard position rejection would be speculative. No source or permanent test change was made. The next boundary cycle should use a new public count/offset contract: `FindNextBlocks()`’s `count` versus prefilled `vBlocks`, `getnodeaddresses` zero/one/max semantics, or an explicit supported-32-bit build if the project matrix gains one. Do not repeat this flat-file or prior filter/block-file cell without new evidence.

## Cycle 129: scanblocks and compact-filter boundary recheck

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `5`
- Selected slug: `boundary-off-by-one`
- Branch: `uber-cycle-129-boundary-off-by-one-20260730`
- HEAD before the cycle: `36bcb3e32dc7c24990440319970ab2bbd3b386f9`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `1045 40` from `git rev-list --left-right --count HEAD...origin/master`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- The fresh fetch, tracked/index status, `git diff --check`, catalog/protocol hashes, and process check passed. PID `777094` (`test_bitcoin --run_test=wallet_tests`) and its Codex parent were preserved. Existing untracked agent artifacts, `node_modules/`, package files, and `test/cache/` were not touched.

### Scope and queued cells

The prior boundary cycles closed CompactSize, direct-fetch, transaction-download, wallet-count, tx-graph, filter-range, flat-file, and block-file cells. This cycle rechecked the remaining height-loop, checkpoint-cardinality, and count/offset surfaces rather than repeating those cells. The falsifiable hypotheses were that the compact-filter checkpoint construction or the `scanblocks` 10,000-block chunking mishandles an exact boundary, or that a reachable height loop overflows before its termination condition.

### Source review

`src/index/blockfilterindex.cpp` computes `results_size` as `stop_index->nHeight - start_height + 1` and walks the inclusive range with `height <= stop_index->nHeight`. A signed overflow is theoretically possible only at an `INT_MAX`-scale height, which is not a reachable Bitcoin chain state. The backward range uses a safe `height >= start_height` condition.

`src/net_processing.cpp` sizes compact-filter checkpoints as `stop_index->nHeight / CFCHECKPT_INTERVAL` and requests heights `(i + 1) * CFCHECKPT_INTERVAL`. This emits exactly 1000, 2000, and later checkpoints no greater than the stop height; an endpoint below 1000 correctly returns an empty vector. Existing `p2p_blockfilters.py` already exercises exact 1000 and 2000 checkpoint and filter-header behavior.

`src/rpc/blockchain.cpp` uses `amount_per_chunk = 10000` and an inclusive `end_range`. Therefore a height difference of exactly 10000 intentionally scans 10001 blocks in one range, while the next range begins after that endpoint. The relevant question is whether the returned interval and results preserve this documented inclusive behavior at both sides of the boundary.

### Evidence

The focused unit command initially exposed a harness setup issue because its `TMPDIR` did not exist; a multi-case invocation then collided with the test fixture's global argument state and was interrupted. This affected only the scratch invocation. After creating the temporary directory and selecting the single case, the exact command

```text
TMPDIR=/data/my_storage/tmp/cycle129-blockfilter-unit /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=blockfilter_index_tests/blockfilter_index_initial_sync --log_level=test_suite --report_level=short
```

passed 1 case with 1825/1825 assertions. The existing functional command

```text
python3 test/functional/p2p_blockfilters.py --configfile=/data/my_storage/tmp/cycle89-build/test/config.ini --tmpdir=/data/my_storage/tmp/cycle129-p2p-blockfilters --portseed=1290 --timeout-factor=2 --loglevel=INFO
```

passed, including exact-height 1000/2000 checkpoint checks. `rpc_scanblocks.py` first failed because the preserved untracked `test/cache/` contained no cached blocks; rerunning with a fresh scratch cache passed:

```text
python3 test/functional/rpc_scanblocks.py --configfile=/data/my_storage/tmp/cycle89-build/test/config.ini --cachedir=/data/my_storage/tmp/cycle129-functional-cache --tmpdir=/data/my_storage/tmp/cycle129-rpc-scanblocks-fresh --portseed=1292 --timeout-factor=2 --loglevel=INFO
```

Finally, a scratch regtest daemon generated 10001 blocks and synchronized the basic block-filter index. The wallet address appeared in every coinbase filter. `scanblocks start ["addr(bcrt1q0fvud2auh4ky77590vggvtj6zade9k26s5ug43)"] 0 10000 basic` returned `from_height=0`, `to_height=10000`, `completed=true`, and 10000 relevant blocks. The same scan through height 10001 returned `from_height=0`, `to_height=10001`, `completed=true`, and 10001 relevant blocks; the shorter result was an exact prefix of the longer result. The scratch daemon was stopped after the run.

### Verdict and handoff

No reachable off-by-one, checkpoint-cardinality, or result-interval defect was confirmed. The `INT_MAX` arithmetic concern is theoretical and lacks a reachable chain fixture; the unsigned `headers.size() - 1` conversion for an empty checkpoint vector is harmless on the supported execution path and did not justify a style-only change. No source or permanent test change was made. The next boundary cycle should select a genuinely new count/offset surface and retain the height-loop limitation instead of repeatedly rerunning the same practical cases.

## Handoff

- Goal draw: catalog index 5, `boundary-off-by-one`.
- Branch: `fuzz-contract-cluster-oracles-20260709`.
- Base at cycle start: `48c182a2170d1423085a2304fb81f3ca11fcf6e5`.
- Comparison tip: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`.
- Scope: arithmetic and range contracts at network block-fetch, CompactSize, tx-graph, wallet count, and related resource boundaries. The concrete finding is in `HeadersDirectFetchBlocks()`.
- Source commit and state commit are intentionally left for the final lines after validation and commit.

## Risk map and search ledger

| Surface | Boundary checked | Result |
| --- | --- | --- |
| `ReadCompactSize()` | canonical prefixes and `MAX_SIZE` | Existing code and tests cover the documented domain; no source defect established this cycle. |
| `HeadersDirectFetchBlocks()` | zero, one, 15, 16, and 17 fetchable blocks; already-requested ancestors | Confirmed off-by-one in the direct-fetch limit and fixed below. |
| `TxDownloadManagerImpl` | exact announcement and in-flight caps | `>=`/`>` behavior is consistent with retaining at most the configured count; no reproducible defect. |
| wallet coin enumeration | `max_count == 0`, one, and exact result count | Existing `result.Size() >= max_count` stop condition matches the contract; no source defect. |
| tx-graph cluster limits | exact count/size versus over-limit transitions | Existing `>` checks preserve equality; no independent failing oracle found. |

History and source searches included recent CompactSize/cardinality, prevout-worker, relay-budget, tx-download, and tx-graph boundary commits. The current functional test `p2p_sendheaders.py` covered a nearby 18-header case, but two blocks were already in flight, so it did not isolate the 16/17 walk boundary. No external issue or PR was needed to establish the finding; the historical direct-fetch implementation and its local functional test were sufficient evidence.

## Contract and boundary table

`MAX_BLOCKS_IN_TRANSIT_PER_PEER` is 16. The loop must gather no more than 16 blocks that are neither already available nor already requested. It must continue walking over blocks that do not need a request so that a valid path with 16 new blocks and already-requested ancestors still reaches the active chain. If the next block would be the 17th fetchable block, `pindexWalk` must remain off the active chain and the function must take the existing large-reorg fallback instead of requesting a partial path.

| Fetchable blocks still needed | Expected walk/request result |
| ---: | --- |
| 0 | Reach active ancestor; no direct-fetch request. |
| 1, 15 | Reach active ancestor; request all fetchable blocks, subject to existing in-flight capacity. |
| 16 | Reach active ancestor when the remainder is already requested/available; request the available subset up to the per-peer cap. |
| 17 | Stop before adding the 17th fetchable block; classify as large for direct fetch and rely on parallel download. |

## Hypothesis and trust boundary

Hypothesis: the old condition

```cpp
vToFetch.size() <= MAX_BLOCKS_IN_TRANSIT_PER_PEER
```

allows a 17th missing/unrequested block into `vToFetch`. For a chain that is exactly 17 blocks ahead and otherwise fetchable, the walk then reaches the active ancestor, so the large-reorg branch is skipped. The later send loop truncates the request to 16 because the peer cap is full, silently leaving the newest block unrequested. This is a locally reachable P2P synchronization correctness/liveness and performance defect, not a consensus or cryptographic defect.

The trust boundary is the peer-supplied connected headers chain. Header processing has already validated the headers, but the chain length and request state are adversarially selected. The invariant is local policy: direct fetch must not request beyond its per-peer in-flight cap and must use the existing fallback when the connected path has more fetchable blocks than that cap.

## Failing-before evidence

The test was first added without changing production code. Command:

```text
test/functional/p2p_sendheaders.py --configfile=/data/my_storage/bitcoin/build_func_clang19/test/config.ini --tmpdir=/data/my_storage/tmp/p2p-sendheaders-cycle21-before2 --portseed=923 --timeout-factor=2
```

The existing Parts 1-4 passed. The new 16-block case received exactly the expected request. The 17-block case failed at `test/functional/p2p_sendheaders.py:552` with `AssertionError` because `test_node.last_message` contained `getdata`. This is the independent failing-before proof for the old `<=` implementation.

A direct replacement of `<=` with `<` was tested as an intermediate mutation. It rejected the existing valid case with 16 new fetchable blocks plus two already-requested ancestors, failing at the pre-existing request assertion on line 510. That negative control established that the fix must distinguish fetchable from already-requested/available ancestors rather than simply terminate after 16 loop iterations.

## Fix

The final loop computes `can_fetch` once per walked index. It stops only when the current index is fetchable and the vector already contains 16 entries; it continues walking through data-present, already-requested, or witness-ineligible entries. Thus:

- a 17th fetchable index leaves `pindexWalk` off-chain and selects the existing large-reorg fallback;
- the pre-existing 16-new-plus-two-requested path still walks to its common ancestor and requests the 14 slots available to the peer;
- no request is added above `MAX_BLOCKS_IN_TRANSIT_PER_PEER`.

The regression test covers exactly 16 and 17 newly announced extension blocks, asserts the 16-block request, asserts no direct request for 17, then supplies the held blocks so the rest of the existing state machine remains valid.

## Verification

Production build:

```text
cmake --build build_func_clang19 --target bitcoind -j4
```

completed after rebuilding `net_processing.cpp` and linking `bin/bitcoind`.

Full functional verification:

```text
test/functional/p2p_sendheaders.py --configfile=/data/my_storage/bitcoin/build_func_clang19/test/config.ini --tmpdir=/data/my_storage/tmp/p2p-sendheaders-cycle21-after3 --portseed=926 --timeout-factor=2
```

passed Parts 1-5 and ended with `Tests successful`.

Independent unit/build verification:

```text
cmake --build build_unit_clang19 --target test_bitcoin -j4
build_unit_clang19/bin/test_bitcoin --run_test=net_tests --catch_system_error=no --log_level=test_suite
```

The build completed and all 31 `net_tests` cases completed with `*** No errors detected`.

Additional controls: `git diff --check` passed; no daemon, functional test, unit test, build, or fuzz process remained active after verification. The failing-before log is preserved at `/data/my_storage/tmp/p2p-sendheaders-cycle21-before2/test_framework.log`; the passing log is at `/data/my_storage/tmp/p2p-sendheaders-cycle21-after3/test_framework.log`.

## Verdict and next queue

Verdict: **confirmed and fixed**. The old comparison admitted one extra fetchable block and could issue a partial direct-fetch request for an exactly over-limit chain. The final patch is limited to `src/net_processing.cpp` plus its functional regression coverage. No unrelated source changes are included.

Next unchecked boundary cells: direct-fetch paths with mixed `BLOCK_HAVE_DATA` and witness-service eligibility; `MAX_BLOCKS_IN_TRANSIT_PER_PEER` interactions with pre-existing in-flight capacity; CompactSize `MAX_SIZE-1/MAX_SIZE/MAX_SIZE+1` behavior at non-vector call sites; and block/file offset limits under 32-bit-relevant arithmetic. Recheck this journal and the uber state before selecting a duplicate hypothesis.

## Cycle 102 start

- Goal draw: catalog index 5, `boundary-off-by-one` (exact selector: `shuf -i 0-98 -n 1` -> `5`).
- Branch: `uber-cycle-102-boundary-off-by-one-20260729`.
- Base at cycle start: `f10f8b67d3ae63a443768b91269297a4a4d1841e`.
- `origin/master`: `87bc4c74c4dff3e5e25abc294934a02f28027a45`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence from the merge-base: 34 local commits and 993 remote commits.
- Gate: tracked/index clean, `git diff --check` passed, catalog/prompt/TSV hashes unchanged, and the pre-existing `test_bitcoin` process under PID 777094 was left untouched.
- Reused-surface exclusions: CompactSize canonical/max, direct-fetch block limits, transaction-download caps, wallet coin enumeration, tx-graph cluster limits, and the Cycle 21 direct-fetch fix are not selected again.
- New scope: inclusive request cardinalities, filter/index file-size and position limits, block-file offsets, and adjacent persistence boundaries not covered by the prior cells.

### Initial hypothesis: compact-filter exact maximum

`PrepareBlockFilterRequest()` checks `stop_height - start_height >= max_height_diff` while the requested range is inclusive, with item count `stop_height - start_height + 1`. The constants document maximum counts of 1000 filters and 2000 filter headers. The current functional test builds a chain through height 1000 and requests heights 1 through 1000, receiving 1000 filters; the same predicate rejects a 1001-item range. The exact-maximum hypothesis is therefore dismissed, pending no source change: the apparent subtraction is intentional because the comparison is against the inclusive range distance.

The next unchecked cell is filter-index file growth and offset arithmetic in `src/index/blockfilterindex.cpp`, followed by block-file position checks if no independent failure is found.

## Cycle 102 result

### Boundary ledger

| Surface | Below / exact / above check | Verdict |
| --- | --- | --- |
| Compact-filter and filter-header requests | 999 / 1000 / 1001 filters; the existing functional test requests heights 1 through 1000 and receives 1000 | Dismissed: `stop_height - start_height >= max_height_diff` rejects the 1001-item inclusive range and accepts the documented maximum. |
| Filter flat files | `pos.nPos + data_size` below, exactly at, and above `MAX_FLTR_FILE_SIZE` | Dismissed: strict `>` permits an exact fill and rotates only when the next record would exceed 16 MiB; the sum is widened by `uint64_t data_size`. |
| `FlatFileSeq::Allocate()` | exact chunk boundary and one byte beyond | Dismissed: `CeilDiv(pos.nPos + add_size, m_chunk_size)` uses `size_t` arithmetic, and the existing flat-file tests cover allocation and finalization. |
| Block flat files | exact `MAX_BLOCKFILE_SIZE` transition and oversized fast-prune block | Dismissed: `FindNextBlockPos()` rolls before `nSize + nAddSize >= max_blockfile_size`; the test-only oversized-block case raises the temporary limit before the assertion, preventing the historical infinite loop. |
| Berkeley wallet page scan | `last_page == 0`, ordinary inclusive final page, and theoretical `UINT32_MAX` wrap | No actionable finding: the inclusive loop correctly checks the final page; the wrap requires a valid approximately 16 TiB file with all page LSNs reset, beyond a practical deterministic reproducer for this cycle. |

### Verification

The first build attempt failed before compilation because the configured ccache symlink `/root/.cache/ccache` points to a missing directory. Re-running with `CCACHE_DIR=/data/my_storage/tmp/cycle102-ccache` built the existing `test_bitcoin` target successfully. The first block-filter test invocation also failed at fixture setup because its `TMPDIR` did not exist; that runner was interrupted after it hung in the failed fixture, then rerun with the created scratch directory.

Successful commands:

```text
CCACHE_DIR=/data/my_storage/tmp/cycle102-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j4
TMPDIR=/data/my_storage/tmp/cycle102-boundary-test /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=blockfilter_index_tests --log_level=test_suite
TMPDIR=/data/my_storage/tmp/cycle102-boundary-test /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=blockmanager_tests --log_level=test_suite
TMPDIR=/data/my_storage/tmp/cycle102-boundary-test /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=flatfile_tests --log_level=test_suite
```

The three focused suites completed with `*** No errors detected`. `git diff --check` passed. No production or test source was changed in this cycle; only this journal section was added. No independent source defect, failing-before oracle, or safe minimal patch was established, so no finding commit was created.

### Verdict and next queue

Verdict: **dismissed/inconclusive with no source change**. The compact-filter hypothesis was conclusively dismissed by the inclusive cardinality calculation and a passing exact-boundary test. Storage and page-scan concerns were inspected and tested, but the only remaining theoretical wrap is not sufficiently reachable to justify a patch. Next cells: height-loop overflow at impossible integer endpoints, checkpoint vector cardinality at exact interval boundaries, and unreviewed count/offset contracts outside the excluded direct-fetch, CompactSize, wallet-count, and transaction-graph surfaces. Re-read this cycle before selecting a new hypothesis.
