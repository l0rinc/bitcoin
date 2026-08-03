# Untrusted-Interface Resource-Exhaustion Variant Analysis

## Cycle 130: duplicate ranged descriptor expansion in scan RPCs

### Selection and gate

- The first post-cycle gate selector was `shuf -i 0-98 -n 1` -> `36` (`sanitizer-analysis-matrix`). Its queued MSan/instrumented-dependency, TokenPipe, and analyzer-warning cells were already closed by Cycle 78, so it was rejected without a branch. The required reroll command returned `7` (`resource-exhaustion-variants`).
- Branch: `uber-cycle-130-resource-exhaustion-variants-20260730`
- Cycle-start HEAD: `890dcd7e202ffc3e362126e090378d682790be10`; `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1046 40` from `git rev-list --left-right --count HEAD...origin/master`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`; prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`; TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`; uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- The fresh fetch, tracked/index status, `git diff --check`, catalog/protocol hashes, and process check passed. PID `777094` (`test_bitcoin --run_test=wallet_tests`) and its Codex parent were preserved. Existing untracked agent artifacts, `node_modules/`, package files, and `test/cache/` were not touched.

### Scope and hypothesis

Cycles 57, 82, and 104 close the cfilters, BIP35, locator, relay-backlog, receive-buffer, transport-send-queue, and REST `getutxos` allocation cells. This cycle followed the remaining RPC scan and request-derived queue leads. P2P address and GETDATA state was rechecked first: `MAX_ADDR_TO_SEND=1000`, address token-bucket accounting, `MAX_INV_SZ=50000`, per-peer GETDATA draining/backpressure, block-in-flight removal, and `txdownloadman.DisconnectedPeer()` all provide explicit bounds or disconnect cleanup. No new P2P retention defect was established.

The distinct hypothesis was in the shared descriptor-scan input path. `EvalDescriptorStringOrObject()` permits a range of up to one million positions per scan object, but `scantxoutset`, `scanblocks`, and `getdescriptoractivity` independently expand every array element before inserting scripts into sets. Exact duplicate scan objects have no semantic effect because all three consumers deduplicate their resulting scripts, yet the old code repeated descriptor parsing/derivation for each duplicate. The resource equation is:

```text
request CPU = one chain/filter/activity scan + N scan objects * descriptor parsing and range expansion
```

The HTTP RPC body limit is 32 MiB, but none of these methods imposed an aggregate scan-object or expansion budget. Therefore a small authenticated request can multiply the already expensive ranged derivation work without changing the result.

### Independent pre-fix reproduction

The existing unmodified `rpc_scantxoutset.py` suite passed with a fresh cache. A scratch regtest daemon at height 10001 was then queried over authenticated JSON-RPC with the same valid ranged descriptor repeated exactly, using `range=1000` and no matching outputs. The request was generated in memory with Python `urllib`, so no production or repository fixture was changed. Results were:

| Exact duplicate objects | JSON request bytes | Elapsed | `txouts` | matches |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 228 | 0.530 s | 10001 | 0 |
| 10 | 1597 | 3.712 s | 10001 | 0 |
| 50 | 7677 | 18.130 s | 10001 | 0 |
| 100 | 15278 | 36.024 s | 10001 | 0 |
| 200 | 30478 | 72.656 s | 10001 | 0 |

The output was identical for every request, while the cost scaled linearly with the redundant object count. This is an authenticated RPC resource-exhaustion issue, not an unauthenticated P2P claim; it matters when an RPC credential is exposed to an untrusted caller and is also a deterministic local-work amplification in the RPC worker pool.

### Fix and verification

`src/rpc/blockchain.cpp` now serializes each scan object once into a temporary `std::set<std::string>` and skips exact duplicates before calling `EvalDescriptorStringOrObject()`. The same guard is applied to all three descriptor-scan RPCs. This is semantics-preserving: the old downstream `std::set<CScript>`/GCS element set/script watch set already removed duplicate derived scripts. `test/functional/rpc_scantxoutset.py` adds a regression comparing the complete result for one ranged object with the result for that object repeated twice.

The first rebuild attempt failed before compiling the changed file because the configured ccache launcher could not create `/root/.cache/ccache/tmp`. The isolated retry used `CCACHE_DIR=/data/my_storage/tmp/cycle130-ccache`:

```text
CCACHE_DIR=/data/my_storage/tmp/cycle130-ccache cmake --build /data/my_storage/tmp/cycle89-build --target bitcoind test_bitcoin -j2
```

It completed successfully. The fixed binary produced the following matched control:

```text
objects=1   payload=228   elapsed=0.459s txouts=10001 matches=0
objects=200 payload=30478 elapsed=0.462s txouts=10001 matches=0
```

The fixed `rpc_scantxoutset.py`, `rpc_scanblocks.py`, and `rpc_getdescriptoractivity.py` functional tests all exited 0 with `Tests successful`. The focused unit command

```text
TMPDIR=/data/my_storage/tmp/cycle130-rpc-unit /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=rpc_tests --log_level=test_suite --report_level=short
```

passed 22 cases and 311 assertions. `git diff --check` passed.

### Classification and limitations

Classification: confirmed local authenticated-RPC resource-exhaustion amplification. The old path accepted a 30 KB request that consumed 72.656 seconds while an equivalent one-object request consumed 0.530 seconds; the fix removes the exact redundant multiplier without changing valid output. No arbitrary new limit was introduced and distinct descriptor objects remain supported.

The reproduction used GCC 12 and a 10001-block scratch chain, not a full mainnet UTXO set or a maximum 32 MiB request. The measurements establish the mechanism and linearity, not a production-wide wall-time bound. Semantically equivalent but textually different objects are not deduplicated, and a single large ranged descriptor remains intentionally expensive and documented. The next resource cycle should not repeat exact duplicate descriptor expansion; remaining work should target distinct descriptor-equivalence normalization, a new parser/queue boundary, or a P2P cleanup state with an independent source-to-sink proof.

## Cycle 57

- Selected by the uber loop: `shuf -i 0-98 -n 1` -> `7`
- Goal: `resource-exhaustion-variants`
- Started from HEAD: `e1933776ba36f5812fb42e4efc536f208c6e5110`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence at start: `origin/master...HEAD` = `2 882`
- Dirty-state gate: tracked and staged state clean; only the known agent-owned untracked artifacts remain
- Process gate: no relevant build, test, daemon, fuzz, sanitizer, or profiling process running
- Catalog/protocol/TSV hashes: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`

## Scope and Prior Evidence

This cycle mines realistic denial-of-service shapes at public or attacker-influenced interfaces. The required evidence is an explicit bound or a demonstrated accounting failure for CPU, memory, disk, network, descriptors, queue length, retries, or retained state. A large allocation or slow operation alone is not a finding when the protocol or local policy already imposes a documented bound.

The prior ledger already closes the exact duplicate global relay-backlog cell: cycle 54 proved that duplicate ForceRelay transaction announcements could retain repeated wtxids, and commit `4f867fc8a3` deduplicated the global backlog while preserving per-peer trickling. Cycles 29 and 32 also closed several integer-domain option cells (`-maxsigcachesize`, `-limitclustersize`, `-dbcrashratio`, and `-dbbatchsize`). This cycle must therefore seek a different source-to-sink shape or a recurrence in another queue/accounting domain.

## Initial Hypotheses

1. A public P2P message may decode a bounded wire vector but retain or expand an auxiliary queue without applying the same bound, allowing repeated low-cost messages to create unbounded memory or work.
2. A request/response path may bound each message but multiply the bound across peers, retries, or duplicate identities without a global or per-peer conservation rule.
3. A parser may reject an oversized payload after allocating or reserving based on the advertised length, creating a CPU, memory, or descriptor spike even though the final object is rejected.
4. An existing limit may be applied to admission but not to cleanup, disconnect, restart, or permission transitions, allowing retained state to outlive the trust boundary.

## Required Verification

- Inventory the relevant P2P/RPC/mempool/persistence entry points and write an explicit resource equation before testing.
- Trace attacker-controlled fields through decode, allocation, queue insertion, retry, timeout, cleanup, and restart paths; include permission and duplicate-identity variants.
- Prefer deterministic socket/message shims, functional tests, unit-level state models, or low resource limits over uncontrolled network load.
- For each candidate, measure operation count, bytes, allocations/RSS, queue length, retries, and cleanup state at a fixed small input; state the extrapolated bound and its assumptions.
- Search prior findings, history, issues, and reviews before reporting. Use a failing-before regression or equivalent first-invalid-operation proof for any source fix.
- Run narrow then broad validation, and preserve raw traces and minimized transcripts under `/data/my_storage/tmp/cycle57-resource-exhaustion/`.

## Evidence Ledger

### P2P admission and relay queues

- `GETBLOCKTXN` accepts the full strictly increasing `uint16_t` index domain: 65,536 entries. The current-history boundary test (`src/test/blockencodings_tests.cpp`, `TransactionsRequestDeserializationCardinalityBoundaryTest`) confirms 65,536 indexes decode and 65,537 is rejected by the differential formatter. `SendBlockTransactions` allocates one shared-reference slot per requested index, so the maximum request-side slot vector is about 512 KiB on this build, plus roughly 65 KiB of wire indexes. An out-of-range index causes `Misbehaving`, then disconnect, so it cannot be repeated indefinitely on one connection. Classified dismissed as a new resource finding; retained as a bounded expensive-operation surface.
- Incoming `GETDATA` is deserialized under the 4 MiB `MAX_PROTOCOL_MESSAGE_LENGTH` and is rejected above `MAX_INV_SZ=50,000` entries. The per-peer queue is drained before new messages are polled, and `ProcessGetData` stops at `fPauseSend` while processing transaction and block responses. `ProcessMessages` also refuses to poll a new message while the queue is nonempty. Existing `src/test/net_tests.cpp:cnode_send_queue_memory_usage_contracts` passed. Classified dismissed for unbounded growth.
- Block `GETBLOCKS` and header announcement vectors are followed by one message-handler `SendMessages` pass; `GETBLOCKS` contributes at most 500 hashes per request and `INV` serialization chunks at 50,000 entries. The handler's `fPauseSend` gate stops further inbound polling once queued output crosses the configured watermark. No independent retained-state or cleanup failure was reproduced.

### Compact-filter response amplification

- `PrepareBlockFilterRequest` limits one `getcfilters` request to 1,000 blocks (`src/net_processing.cpp:185-186, 3610-3628`). `LookupFilterRange` reads every filter into a vector (`src/index/blockfilterindex.cpp:417-437`), and `ProcessGetCFilters` then calls `MakeAndPushMessage` once per filter (`src/net_processing.cpp:3656-3665`) without checking `node.fPauseSend` inside the loop.
- The stored filter reader accepts a CompactSize vector bounded by `MAX_SIZE=0x02000000` (32 MiB) in `src/serialize.h`; the filter-file rollover constant is 16 MiB but is a file-chunk threshold rather than a strict per-record assertion. Therefore the source-level worst case for one validly shaped request is approximately 1,000 * 32 MiB of filter payload plus the temporary range vector and per-peer send queue. This is a protocol-state bound, not an attacker-controlled arbitrary allocation: the filters must already exist in the local index and the request range is capped.
- A temporary measurement-only unit probe constructed a BASIC GCS filter from 300,000 distinct 4-byte elements and reported `encoded_bytes=789470`; it was removed after the measurement. This extrapolates to about 789 MB for 1,000 similarly shaped filters before transport overhead, but does not prove that an active-chain block can realize that distribution. No sanitizer, crash, or allocation failure occurred.
- `p2p_blockfilters.py` passed all cfilter/cfheaders/cfcheckpt and invalid-range cases; the block-filter unit/index suite passed 13 cases, and a 735-input production `FUZZ=blockfilter` replay added no units and produced no diagnostics. The fetched upstream tip retains the same 1,000-filter loop, with no historical byte-cap or continuation design found. Verdict: inconclusive resource amplification. Do not patch without an implementable response-continuation or refusal contract, because simply stopping at the send watermark would silently truncate a valid BIP157 response.

### BIP35 mempool response

- A permitted `MEMPOOL` request sets `m_send_mempool`; the next `SendMessages` call snapshots all `m_mempool.infoAll()` entries and emits 50,000-entry `INV` chunks without an inner `fPauseSend` check (`src/net_processing.cpp:5264-5291, 6429-6465`). The request is restricted to nodes advertising `NODE_BLOOM` or peers with explicit `Mempool` permission and is also subject to the upload target for ordinary peers.
- The response is bounded by the configured mempool and the 50,000-entry chunk size, but several chunks can be queued in one pass, so a slow peer can exceed the nominal per-connection send watermark. Existing BIP37/mempool functional coverage (`p2p_filter.py`) passed, including relevant and irrelevant mempool transactions; the generic send-queue contract passed. No large-mempool, low-watermark failure-level reproducer was run, and changing this would require a resumable snapshot/iterator contract to avoid dropping or duplicating announcements. Verdict: inconclusive; carry forward as the highest-priority response-queue hypothesis.

### Other bound checks

- `MAX_ADDR_TO_SEND=1000`, the address token bucket, `MAX_GETCFHEADERS_SIZE=2000`, `MAX_GETCFILTERS_SIZE=1000`, `MAX_BLOCKTXN_DEPTH=10`, `MAX_INV_SZ=50000`, and the 4 MiB transport message limit were traced to their cleanup and disconnect paths. No limit bypass or stale retained queue was demonstrated in this cycle.
- History search found the 2024 receive-buffer and getdata CPU disclosures, the current block-filter range-output fixes, and no later cfilter response byte cap. Cycle 54's duplicate global relay backlog finding remains distinct: current `std::set<Wtxid>` backlogs deduplicate identities and `ExtractBestByMiningScoreWithTopology` drops missing entries.

### Commands and key output

- `TMPDIR=/data/my_storage/tmp build_unit_clang19/bin/test_bitcoin --run_test=blockfilter_tests,blockfilter_index_tests ...`: exit 0, 13 cases, no errors.
- `FUZZ=blockfilter .../fuzz ... -runs=0 -print_final_stats=1`: exit 0, 735 corpus units, `new_units_added:0`, peak RSS 1718 MB, no diagnostics.
- `python3 build_func_clang19/test/functional/p2p_blockfilters.py --configfile=build_func_clang19/test/config.ini ...`: exit 0; active/stale filters, cfheaders, cfcheckpt, and invalid requests passed.
- `TMPDIR=/data/my_storage/tmp build_unit_clang19/bin/test_bitcoin --run_test=net_tests/cnode_send_queue_memory_usage_contracts ...`: exit 0, one case, no errors.
- `python3 build_func_clang19/test/functional/p2p_filter.py --configfile=build_func_clang19/test/config.ini ...`: exit 0; BIP37 filtering, mempool announcements, filter limits, and CVE-2013-5700 regression checks passed.
- `git show origin/master:src/net_processing.cpp` confirmed the fetched upstream tip still uses the same per-filter `getcfilters` response loop. `git diff HEAD..origin/master` showed no upstream response-byte fix to transplant.

## Handoff

Cycle 57 conclusion: no source fix is justified. The cfilters and BIP35 response loops are bounded by local indexed/mempool state but can exceed the soft send watermark; both require a protocol-compatible continuation or refusal design before changing behavior. The next cycle should begin with the BIP35 response-queue hypothesis, then inspect analogous multi-message response paths and historical backpressure decisions. Preserve the exact command/output ledger above and do not rerun the removed measurement probe as a product test.

## Cycle 82 start

- Selected by the uber loop: exact `shuf -i 0-98 -n 1` -> `7` (`resource-exhaustion-variants`). This is a re-selection of the goal, so cycle 57's cfilters and BIP35 response-watermark cells are excluded unless a new caller, bound, or failure mode is independently demonstrated.
- Branch: `uber-cycle-82-resource-exhaustion-variants-20260728`.
- Cycle-start HEAD: `d86caff0a23f0ba042ced945520dffb1ee224164` (`journal: close serialization input cycle 81`).
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence `origin/master...HEAD` is `2 945`.
- Gate: tracked source was clean after the cycle-81 source and journal commits; known untracked agent artifacts and `test/cache` are preserved and excluded. Catalog/protocol/TSV hashes match the recorded values. No relevant process is running.

## Cycle 82 scope and exclusions

Audit attacker-influenced CPU, memory, disk, network, descriptor, queue, retry, and retained-state costs at P2P, RPC/REST, wallet, persistence, and parser boundaries. A source finding requires a realistic low-limit reproducer or an explicit violated resource/accounting bound; a large but already bounded operation is only a lead. Do not repeat cycle 57's `getcfilters` 1,000-filter response loop, BIP35 mempool snapshot watermark, global relay backlog, integer-domain option, or previously fixed receive-buffer cells without distinct evidence.

## Cycle 82 hypotheses

1. A multi-message P2P response path may snapshot or enqueue a bounded request's full result before checking the peer's send watermark, with a deterministic slow-peer schedule that demonstrates retained bytes beyond the intended bound.
2. A public REST/RPC endpoint may parse or construct a large bounded object before applying its endpoint-specific limit, causing avoidable allocation or CPU amplification for a low-cost request.
3. A disconnect, permission, retry, or restart transition may leave request-derived queues, descriptors, or temporary storage retained after the initiating peer or caller is gone.
4. Historical DoS fixes may have analogous current paths in a different protocol or persistence subsystem; each candidate must have a distinct source-to-sink trace and a separate minimized reproducer.

## Cycle 82 evidence plan

Start with history/advisory and current-code mining, then write an explicit equation for each candidate: attacker operations, bytes, allocations, queue entries, retries, and cleanup state. Prefer direct unit state models, deterministic socket shims, functional tests, fixed `RLIMIT`/cgroup limits, and scratch datadirs. Measure before/after queue length, RSS, CPU, disk, and cleanup; do not infer an unbounded condition from a single slow run. Preserve exact commands, raw output, minimized transcripts, and rejected hypotheses in this journal.

## Cycle 82 confirmed finding

### GETBLOCKS/GETHEADERS locator count checked after allocation

- Trust boundary: an unauthenticated peer controls the payload of `getblocks` and `getheaders` messages. Both handlers previously deserialized `CBlockLocator` with the generic `VectorFormatter`, then compared `locator.vHave.size()` with the protocol limit `MAX_LOCATOR_SZ=101`.
- Old path: `CBlockLocator` first reads the advertised CompactSize count. The generic formatter reserves in `MAX_VECTOR_ALLOCATE=5,000,000` byte batches before reading each `uint256`. The deterministic probe used count `MAX_SIZE / sizeof(uint256) = 1,048,576` with no hash bytes. The old source therefore reserved `156,250` hashes, about `5,000,000` bytes, before the first read failed; `ProcessMessages` caught that truncation exception without disconnecting the peer. A complete message under `MAX_PROTOCOL_MESSAGE_LENGTH=4,000,000` could instead make the old path read roughly 125,000 hashes before the later 101-entry check.
- Independent failing-before evidence: `TMPDIR=/data/my_storage/tmp ./build_unit_clang19/bin/test_bitcoin --run_test=net_tests/oversized_locator_disconnects_before_hash_deserialization --log_level=message` ran both message types and failed both `node.fDisconnect` assertions on the unmodified source. The first attempt without `TMPDIR` was discarded because the root filesystem was full before fixture setup.
- Fix: `ReadBlockLocator` reads the locator version and CompactSize count directly, rejects counts above `MAX_LOCATOR_SZ` before resizing or reading hashes, and preserves the existing explicit disconnect/log behavior for both handlers. Valid locators still deserialize their bounded hashes and `hashStop` through the normal path.
- Passing evidence: `git diff --check`; `ninja -C build_unit_clang19 test_bitcoin -j2`; the focused regression passed for both message types; the complete normal `net_tests` suite passed 33 cases; the rebuilt Clang 19 TSan target passed the focused regression with no diagnostic.
- Classification: confirmed local resource-exhaustion and parser-boundary defect. The input is remotely reachable, the old allocation is avoidable, the protocol's existing 101-entry rule is the intended contract, and the patch does not alter valid locator semantics.

## Cycle 104 start

- Selected by the uber loop: exact `shuf -i 0-98 -n 1` -> `7` (`resource-exhaustion-variants`) after Cycle 103 closed goal 73. This is a re-selection, so the Cycle 57 cfilters/BIP35 response-watermark cells and the Cycle 82 locator-allocation finding are excluded unless a new caller, bound, or failure mode is independently demonstrated.
- Branch: `uber-cycle-104-resource-exhaustion-variants-20260729`.
- Cycle-start HEAD: `4324dc11e05818edb2e3f718dcf278059929ccdc`; `origin/master`: `87bc4c74c4dff3e5e25abc294934a02f28027a45`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence `origin/master...HEAD` is `34 997`.
- Gate: `git diff --check` passed; tracked/index state was clean; the catalog, prompt, and TSV hashes matched `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, and `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`; known untracked artifacts were preserved. The unrelated PID `777094` wallet test was observed and left untouched.

## Cycle 104 scope and hypotheses

Audit a new set of attacker-influenced resource equations across public RPC/REST and P2P request paths, excluding prior locator, cfilter, BIP35, relay-backlog, receive-buffer, direct-fetch, and transport-send-queue cells. Prioritize count/byte limits applied after construction, per-request work multiplied by retries or peers, and request-derived state that survives disconnect or failure. A source finding requires a deterministic low-limit reproducer or an explicit violated bound; a large but documented and conserved operation remains a lead.

Initial queue:

1. REST/RPC endpoints that parse a large bounded object or serialize an unbounded result before their endpoint-specific limit or pagination boundary.
2. P2P request state whose retry, timeout, disconnect, permission, or duplicate-peer transition can retain work or descriptors past the initiating request.
3. Historical DoS/accounting fixes with analogous current call sites in a different subsystem, proved by a distinct source-to-sink path and minimized input.

For each candidate, write the equation `attacker operations x per-operation cost + retained state`, trace cleanup and restart behavior, and use a unit-level state model, deterministic message shim, functional RPC test, or fixed resource limit. Preserve exact commands, raw output, and rejected hypotheses below. Do not change behavior without a failing-before regression or equivalent first-invalid-operation proof.

## Cycle 104 confirmed finding: REST getutxos count checked after allocation

- Trust boundary and contract: when REST is enabled, an HTTP client controls the binary `/rest/getutxos.bin` body. The endpoint's documented/API contract allows at most `MAX_GETUTXOS_OUTPOINTS = 15` outpoints, but the old path deserialized `std::vector<COutPoint>` with the generic `VectorFormatter` and checked `.size()` only afterward.
- Minimal old input: the five HTTP body bytes `fe 40 42 0f 00` declare one million outpoints and contain no entries. The legacy `DataStream oss << strRequestMutable` framing adds the body-length byte before the endpoint reads it, so the parser sees `05 fe 40 42 0f 00`: a true `fCheckMemPool` byte followed by the one-million CompactSize count. This is well below the HTTP `MAX_BODY_SIZE` and requires no outpoint bytes.
- Independent pre-fix probe command: `g++ -std=c++20 -O2 -Isrc -Idepends -o /data/my_storage/tmp/rest_getutxos_probe agent-journal/rest_getutxos_probe.cpp src/support/cleanse.cpp && /data/my_storage/tmp/rest_getutxos_probe`. With the old generic formatter it reported `encoded=6 capacity=138888 size=1 remaining=0 error=DataStream::read(): end of data: iostream error`; `138888 * sizeof(COutPoint)` is `4,999,968` bytes, immediately below the generic 5,000,000-byte allocation batch. The bounded formatter on the same count reported `limited capacity=0 size=0 remaining=0 error=Vector length limit exceeded: iostream error`. The probe was deleted after the measurement.
- Fix: deserialize with `LIMITED_VECTOR(vOutPoints, MAX_GETUTXOS_OUTPOINTS)`, so the CompactSize is rejected before reserve or element construction. Translate the specific limit exception to the endpoint's existing `max outpoints exceeded` error; malformed/truncated entries retain the existing generic parse error.
- Regression: `interface_rest.py` posts `bytes.fromhex('fe40420f00')` to `/getutxos.bin` and asserts HTTP 400 with `Error: max outpoints exceeded (max: 15)`. On the old path the same fixture reaches the missing first outpoint and returns `Parse error`, so the assertion fails; on the fixed path it rejects at the count.
- Validation: `CCACHE_DIR=/data/my_storage/tmp/cycle104-ccache cmake --build /data/my_storage/tmp/cycle89-build --target bitcoind test_bitcoin -j4` passed; `TMPDIR=/data/my_storage/tmp/cycle104-test /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=rest_tests --log_level=test_suite` passed one case; and `python3 test/functional/create_cache.py --configfile=/data/my_storage/tmp/cycle89-build/test/config.ini --cachedir=/data/my_storage/tmp/cycle104-cache --tmpdir=/data/my_storage/tmp/cycle104-cache-tmp --loglevel=INFO` followed by `python3 test/functional/interface_rest.py --configfile=/data/my_storage/tmp/cycle89-build/test/config.ini --cachedir=/data/my_storage/tmp/cycle104-cache --tmpdir=/data/my_storage/tmp/cycle104-rest3 --loglevel=INFO` passed. `git diff --check` passed.
- Classification: confirmed local resource-exhaustion and input-boundary defect. A remote request can force an avoidable multi-megabyte vector reservation and element read with a six-byte serialized stream while the endpoint's count contract is 15. The fix preserves valid requests and makes the rejection happen at the trust boundary. The earlier failed functional attempts were environment/setup issues: the repository cache had no pre-mined chain, and the first fixture accidentally made the wrapped stream's count byte zero; both are recorded in the cycle transcript, not treated as product behavior.

## Cycle 104 rejected and remaining queue

- The existing `INV`/`GETDATA` post-deserialization `MAX_INV_SZ` checks and `ADDR`/`ADDRV2` `MAX_ADDR_TO_SEND` check remain bounded protocol-message leads, but the prior resource and memory-pressure ledgers already cover their generic per-connection deserialization allocation. No new caller or cleanup failure was established this cycle, so they are not repeated as findings.
- Next unchecked cells: inspect RPC/REST scan and serialization endpoints for work multiplied by caller-controlled ranges; then review P2P address/request retry state for retained descriptors or queues after disconnect. Preserve the REST count fix and do not repeat the locator, cfilter, BIP35, relay-backlog, receive-buffer, or transport-send-queue cells without new evidence.

## Cycle 234: BIP37 filter vector bounds before deserialization

### Selection and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `7` (`resource-exhaustion-variants`).
- Branch: `uber-cycle-234-resource-exhaustion-variants-20260731`.
- Cycle-start HEAD: `b6819c245b819577ac484b0c16b712309c12b18f` (`agent: close cycle 233 handoff`).
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence from `git rev-list --left-right --count origin/master...HEAD`: `42 1251`.
- The fresh gate passed: tracked source/index state was clean, `git diff --check` passed, all catalog/prompt/TSV/protocol hashes matched the uber-goal records, and the protected long-running test processes were observed and preserved. Known untracked agent artifacts, package files, `node_modules/`, and test caches were not touched.

### Scope and hypothesis

The prior resource cycles closed the generic INV/GETDATA and ADDR/ADDRV2 message leads, oversized locators, REST `getutxos`, exact duplicate descriptor expansion, cfilter response-watermark, BIP35 response-watermark, relay backlog, receive-buffer, and transport send-queue cells. The new hypothesis was that BIP37's endpoint-specific vector limits were checked after generic vector deserialization. A peer that has negotiated bloom-filter support controls the vector count in `filterload` and `filteradd` messages.

The relevant limits are:

- `MAX_BLOOM_FILTER_SIZE = 36,000` bytes for the filter payload;
- `MAX_SCRIPT_ELEMENT_SIZE = 520` bytes for one `filteradd` element; and
- `MAX_PROTOCOL_MESSAGE_LENGTH = 4,000,000` bytes for a complete P2P payload.

Before this cycle, `CBloomFilter` serialized `vData` through the generic `VectorFormatter`, and `FILTERADD` read `std::vector<unsigned char>` through that same formatter. `VectorFormatter` reserves in 5 MiB batches based on the CompactSize count before reading elements. The resource equation for a count-only malformed message was therefore:

```text
attacker bytes = 5-byte CompactSize(4,000,000)
old vector capacity = 4,000,000 bytes
valid filterload/filteradd capacity = 36,000/520 bytes
```

The old handler then attempted the first element read, caught the EOF at the outer message boundary, and did not record the protocol's existing `Misbehaving` result. The limited formatter rejects the count before reserve, leaving vector capacity at zero; the handler translates only that limit exception to the existing penalty. A complete oversized payload is still bounded by the 4 MB transport limit, but the avoidable post-receive multi-megabyte allocation and parse are removed.

### Independent pre-fix evidence

- A temporary formatter probe compiled with:

  ```text
  g++ -std=c++20 -O2 -Isrc -Idepends -o /data/my_storage/tmp/filter_bounds_cycle234_probe agent-journal/filter_bounds_cycle234_probe.cpp src/support/cleanse.cpp && /data/my_storage/tmp/filter_bounds_cycle234_probe
  ```

  For both `filterload` (limit 36,000) and `filteradd` (limit 520), the generic formatter reported `declared=4000000 capacity=4000000 size=4000000 remaining=0 error=DataStream::read(): end of data: iostream error`. The limited formatter reported `capacity=0 size=0 remaining=0 error=Vector length limit exceeded: iostream error`. The temporary probe was removed after the measurement.

- The functional regression was added to `test/functional/p2p_filter.py`. It sends a count-only 5-byte payload declaring 4,000,000 elements for each message type and expects a `Misbehaving` log. On the unmodified binary, the test failed with `Expected message(s) ['Misbehaving'] not found`; the node log instead showed `ProcessMessages(filterload, 5 bytes): Exception 'DataStream::read(): end of data: iostream error' caught`. This establishes the old allocation/catch path independently of the post-fix behavior.

### Fix

- `src/common/bloom.h` now deserializes `CBloomFilter::vData` with `LIMITED_VECTOR(obj.vData, MAX_BLOOM_FILTER_SIZE)`, preserving the existing wire format for valid filters while rejecting the count before allocation.
- `src/net_processing.cpp` adds a narrow check for the `Vector length limit exceeded` exception. `FILTERLOAD` maps it to `Misbehaving(peer, "too-large bloom filter")`; `FILTERADD` maps its bounded vector rejection to `Misbehaving(peer, "bad filteradd message")`. Other deserialization failures still propagate to the existing generic message-processing catch, preserving malformed/truncated input behavior.
- `test/functional/p2p_filter.py` covers both count-only oversized declarations before the existing complete-payload size tests. No production helper or broad protocol limit was added.

### Validation

- `env TMPDIR=/data/my_storage/tmp/cycle234-filter-unit-tmp /data/my_storage/tmp/cycle214-build/bin/test_bitcoin --run_test=bloom_tests --log_level=test_suite --report_level=short`: passed 14 cases and 37,687 assertions.
- `env TMPDIR=/data/my_storage/tmp/cycle234-net-unit-tmp /data/my_storage/tmp/cycle214-build/bin/test_bitcoin --run_test=net_tests --log_level=test_suite --report_level=short`: passed 36 cases and 159,095 assertions.
- `mkdir -p /data/my_storage/tmp/cycle234-filter-build-tmp /data/my_storage/tmp/cycle234-filter-ccache && env TMPDIR=/data/my_storage/tmp/cycle234-filter-build-tmp CCACHE_DIR=/data/my_storage/tmp/cycle234-filter-ccache ninja -C /data/my_storage/tmp/cycle214-build bitcoind test_bitcoin -j2`: passed all 173 build steps. Existing unrelated warnings appeared in `httpserver_tests.cpp` and `util_tests.cpp`.
- `env TMPDIR=/data/my_storage/tmp/cycle234-functional-fixed-tmp python3 test/functional/p2p_filter.py --configfile=/data/my_storage/tmp/cycle214-build/test/config.ini --cachedir=/data/my_storage/tmp/cycle234-functional-fixed-cache --tmpdir=/data/my_storage/tmp/cycle234-functional-fixed-tmp/run --loglevel=INFO`: passed all BIP37 size, filtering, mempool, and CVE-2013-5700 checks.
- `git diff --check` passed after the source and test changes.

### Classification and limitations

Classification: confirmed local remotely reachable resource-exhaustion and input-boundary defect in BIP37 message deserialization. A peer can use five payload bytes to make the old parser materialize four million byte-sized elements, despite valid protocol object limits of 36,000 and 520 bytes; the new parser rejects at the declared count and retains the existing misbehavior accounting. Valid filter serialization and normal BIP37 behavior are unchanged.

The count-only input is intentionally truncated so the old path's allocation can be isolated without sending 4 MB of data. The transport layer still buffers a complete message up to its existing 4 MB limit before dispatch, so this fix does not claim to remove all message-buffer cost. It removes the additional object allocation/element construction and prevents repeated malformed filter attempts from bypassing the existing penalty. No new filter protocol or permission policy was introduced.

### Handoff

The source/test changes are ready for one self-contained finding commit. After committing, append the source commit and exact validation to `agent-journal/uber-goal-state.md`, make the separate state-close commit, then perform the fresh gate and exact selector draw for the next cycle. Do not repeat the closed BIP37 vector cell without a distinct caller, limit, or cleanup failure.

## Cycle 312: GETBLOCKTXN request count before allocation

### Selection and scope

- Exact selector: `shuf -i 0-111 -n 1` -> `7` (`resource-exhaustion-variants`).
- Branch: `uber-cycle-312-resource-exhaustion-variants-20260802`.
- Cycle-start/base HEAD: `260f3a2081b3893120c09eb8d8fe11da7802a755`.
- Selection commit: `b57a9f0abe` (`journal: select cycle 312 resource exhaustion variants`).
- The earlier cfilters, BIP35, locator, relay-backlog, receive-buffer, transport-send-queue, REST `getutxos`, duplicate descriptor, BIP37 filter, and generic INV/GETDATA or ADDR/ADDRV2 allocation cells remain excluded. This cycle selected a distinct count-before-allocation path using `DifferenceFormatter`.

### Hypothesis and source trace

`GETBLOCKTXN` is a public P2P request. `BlockTransactionsRequest` deserialized its `indexes` through the generic `VectorFormatter`, which reads a CompactSize count and reserves the vector before `DifferenceFormatter` validates the strictly increasing `uint16_t` domain. The request can contain at most the complete `uint16_t` index domain, 65,536 entries, so a count-only malformed message should be rejected before reserve, element construction, or differential decoding.

The minimal old input was a 32-byte block hash followed by a five-byte CompactSize count declaring 2,500,000 indexes. The old parser accepted the count, reserved capacity for 2,500,000 `uint16_t` values (5,000,000 bytes), constructed one default element, and then failed at EOF. The complete attacker input was only 37 bytes; no index delta bytes were needed. The old probe output was:

```text
error=DataStream::read(): end of data: iostream error capacity=2500000 size=1 remaining=0
```

The valid maximum is 65,536 entries `[0, 65535]`, already exercised by the existing zero-delta boundary test. The prior 65,537-entry test proves the differential domain overflow, but it did not prevent a much larger count from reaching the generic allocator.

### Fix and regression

`BlockTransactionsRequest` now uses `LimitedVectorFormatter<65536, DifferenceFormatter>`. This preserves the compact differential wire format and rejects a count above the representable index domain before vector allocation. The regression serializes only the block hash and an oversized count, then asserts an exception, empty output, and zero capacity. The fixed probe output is:

```text
error=Vector length limit exceeded: iostream error capacity=0 size=0 remaining=0
```

This fixes the avoidable allocation and partial construction while preserving valid requests and the existing malformed-message handling. It does not claim to remove the already bounded network message buffer or to change the peer penalty policy for malformed `GETBLOCKTXN` input.

### Validation

- `mkdir -p /data/my_storage/tmp/cycle312-getblocktxn-build-tmp && CCACHE_DISABLE=1 TMPDIR=/data/my_storage/tmp/cycle312-getblocktxn-build-tmp ninja -C /data/my_storage/tmp/cycle246-wallet test_bitcoin -j2`: passed, including all header-dependent rebuild steps.
- `TMPDIR=/data/my_storage/tmp/cycle312-getblocktxn-test /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=blockencodings_tests/TransactionsRequestDeserializationRejectsOversizedCount --log_level=test_suite --report_level=short`: passed 1 case and 3 assertions.
- `TMPDIR=/data/my_storage/tmp/cycle312-getblocktxn-fulltest /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=blockencodings_tests --log_level=message --report_level=short`: passed 31 cases and 372 assertions.
- The first verbose full-suite attempt was blocked by its output pipe after the test runner had started; it was stopped as the cycle-owned process and replaced with the low-output run. The earlier focused attempt failed only because its `TMPDIR` did not exist; creating the scratch directory made it pass. Neither was a product failure.
- `git diff --check` passed. The independent pre-fix and fixed probe binaries were compiled outside the repository; the source probe remains untracked and is not part of the finding commit.

### Classification and next queue

Classification: confirmed remotely reachable P2P resource-exhaustion and input-boundary defect. A peer could use a count-only 37-byte `GETBLOCKTXN` payload to force a five-megabyte vector reservation before the index-domain check. The fix moves the bound to the count trust boundary and leaves valid serialization, differential decoding, and existing 65,537 overflow behavior intact.

Limitations: no fresh daemon socket functional test was run because the host filesystems are full and the existing long-running workloads were preserved. The serializer regression, old/new allocation probes, source trace, and full block-encoding suite independently establish the mechanism and fix. The next learned queue is to audit every custom difference/varint/CompactSize vector formatter for the same mismatch between a representable element domain, a declared count, allocation timing, and malformed-input accounting; do not repeat this repaired caller without a distinct formatter or contract.
