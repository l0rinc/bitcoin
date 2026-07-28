# Boundary and Off-by-One Audit: Cycle 21

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
