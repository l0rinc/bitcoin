# Untrusted-Interface Resource-Exhaustion Variant Analysis

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
