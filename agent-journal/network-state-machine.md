# Network state-machine audit

## Cycle 115 start

- Selected goal: 73, `network-state-machine`.
- Branch: `uber-cycle-115-network-state-machine-20260729`.
- Base/HEAD at gate: `43ce2453e6027eb853b877a20ab5bdd49f024b27`.
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Divergence (`origin/master...HEAD`): 40 commits behind, 1019 commits ahead.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Selector prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Goal TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Exact selector command/result: `shuf -i 0-98 -n 1` -> `73`.
- Tracked/index state passed the fresh gate; known untracked agent artifacts remain preserved. PID `777094` is the unrelated earlier `wallet_tests` process and is not to be modified.

This is a continuation of the existing network-state-machine journal, not a replay of Cycle 103. Cycle 103 already fixed and indexed the `DynSock` stack-ownership and EOF-readiness defects, and its V1 roundtrip, rejected-send, handshake, and send-accounting cells remain excluded. This cycle focuses on the next unchecked production cells: deterministic `SocketHandlerConnected` short-write/EOF/error sequences, V2 partial-handshake fallback and half-close transitions, and `GetBytesToSend`/`MarkBytesSent` `more` semantics across queued-message handoff and backpressure.

Initial falsifiable hypotheses:

1. `SocketHandlerConnected` may consume or retain the wrong bytes, close the node, or leave stale send state after a short write, EOF, or non-retryable socket error.
2. V2 `ShouldReconnectV1()` may recommend fallback after bytes or state have already made V1 impossible, or fail to recommend it after an incoming V1 prefix/half-close schedule.
3. A transport's `BytesToSend.more` result may disagree with the actual queue/transport state at exact header, payload, empty-payload, and final-message boundaries, causing missed write readiness or unnecessary send attempts.

Use deterministic socket shims or direct production transport tests, not sleeps or public peers. Search this journal, prior fixes, history, tests, and fuzz harnesses before each candidate. Require a minimal failing-before regression or an executable state/invariant proof before any source change.

## Cycle 115 completion

- The V1 and V2 transport source trace found no independently reproducible defect in the selected cells. `SocketHandlerConnected` preserves queued bytes after short writes and closes on EOF or non-retryable errors; the reviewed state transitions matched the socket contract. `V1Transport::GetBytesToSend` and `V2Transport::GetBytesToSend` reported `more` consistently with queued-message and handshake state across the exercised exact boundaries.
- A temporary focused `net_tests/transport_cycle115_more_and_v1_reconnect_boundaries` oracle exercised V1 empty/nonempty payloads with and without a following message, V2 initiator fallback after exactly 24 sent V1-header bytes, reset after one received byte, and V2 responder detection of a valid V1 prefix. It passed before removal; it was deliberately not retained because no production or test defect was found.
- Current validation after removing the temporary test: `TMPDIR=/data/my_storage/tmp/cycle115-net-tests /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=net_tests --log_level=message` passed all 34 cases, and the corresponding `netbase_tests` command passed all 27 cases. `git diff --check` passed and `src/test/net_tests.cpp` has no diff.
- The source-equivalent connman fuzz runner also passed the deterministic seven-file corpus:

  ```text
  FUZZ=connman /data/my_storage/tmp/cycle101-build-dead-zones/reduce-exports-fuzz-gcc/bin/fuzz src/net.cpp src/net.h src/test/fuzz/connman.cpp src/test/fuzz/util/net.cpp src/test/net_tests.cpp src/test/util/net.cpp src/test/fuzz/p2p_transport_serialization.cpp
  connman: succeeded against 7 files in 0s.
  ```

- One early test invocation omitted the pre-created `TMPDIR` and failed in fixture setup with `temp_directory_path: No such file or directory`; after creating `/data/my_storage/tmp/cycle115-net-tests`, the same target passed. This was a setup failure, not a repository finding. Attempts to pass libFuzzer-style options to the local runner were likewise classified as invocation errors because this runner treats arguments as input paths.
- Limitations: the available socket helpers do not expose a deterministic scripted short-write/errno sequence through the public `SocketHandlerConnected` path, and no public network was used. V2 partial-handshake behavior was tested directly at the transport boundary, while the fuzz run covered only the available seven-file corpus. No source or test change is justified by this cycle.
- Verdict: hypotheses 1-3 unconfirmed; no finding, source commit, or retained regression test. Next queue: select a distinct catalog goal after the fresh gate; revisit this network cell only when a scripted socket harness or new history/fuzz evidence makes the untested scheduler/error combinations actionable.

## Cycle 103 start

- Selected goal: 73, `network-state-machine`.
- Branch: `uber-cycle-103-network-state-machine-20260729`.
- Base/HEAD at gate: `e3b2cadf429f99a6383eb39e552af0bbd7084f43`.
- `origin/master`: `87bc4c74c4dff3e5e25abc294934a02f28027a45`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Divergence (`origin/master...HEAD`): 34 commits behind, 994 commits ahead.
- Tracked/index state was clean at the gate; `git diff --check` passed.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Selector prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Goal TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Exact selector command/result: `shuf -i 0-98 -n 1` -> `73`.
- Preserved unrelated untracked artifacts, including the existing journals and local tooling files.
- Existing unrelated long-running test: PID `777094`, an earlier `wallet_tests` process; it was not modified.

## Scope and exclusions

Exercise fragmented and coalesced reads, short writes, EOF, reconnects, half-closes, stalls, backpressure, duplicate messages, and handshake state with deterministic socket shims. This cycle excludes the prior `bitcoin-p2p-accounting` lifecycle-count finding, prior V1 transport roundtrip and rejected-send tests, prior handshake-contract tests, prior send-queue accounting work, compact-block read-failure and locator-size fixes, and the Cycle 98 locking/threading surfaces. Do not treat a transport behavior as a new finding without checking those prior results first.

Initial queue:

1. Trace V1 and V2 transport byte-consumption and byte-production state transitions under partial spans, coalesced messages, and exact-boundary sends.
2. Trace `CNode::ReceiveMsgBytes` and the socket-handler EOF/short-I/O paths for state changes that differ between a partial read, half-close, and reconnect.
3. Compare the `more`/queue semantics of `GetBytesToSend` and `MarkBytesSent`, including rejected or zero-length messages, against callers and tests.
4. Inspect history and fuzz harnesses for already-fixed variants before selecting a falsifiable hypothesis.

No source or test changes have been made for this cycle at journal creation.

## Cycle 103 candidate: deterministic socket shim EOF and ownership

The first focused test exposed two independent `src/test/util/net.cpp` defects. `DynSock::Wait()` calls `ev.emplace(this, ...)` even though `EventsPerSock` stores owning `std::shared_ptr<const Sock>` keys. With a stack-allocated `DynSock`, returning from `Wait()` destroys an owning shared pointer to the stack object and aborts with `double free or corruption`. After that lifetime issue is isolated, the EOF case should check the second hypothesis: `DynSock::WaitMany()` peeks with `Pipe::GetBytes()`, which returns `0` for EOF, but only treats `== 1` as readable. Real `poll`/`select` reports a closed stream as readable, so the shim must surface `RecvEvent` for `0` as well.

The initial regression command was:

```text
TMPDIR=/data/my_storage/tmp/cycle103-test /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=netbase_tests/dynsock_reports_eof_as_receive_readiness --log_level=test_suite
```

The test built successfully, then aborted in the new test with `double free or corruption (out)`, before reaching the EOF assertion. The first fix will use a non-owning `shared_ptr` control block for the temporary map key and a separate regression that supplies one readable byte. The second fix will change the readiness check to accept both a positive byte count and the EOF result, with the EOF regression retained separately.

### Candidate A verdict: confirmed

`DynSock::Wait()` constructed an owning `shared_ptr` from its stack-owned `this` pointer. The focused readable-byte regression failed before the fix with the double-free abort. The fix uses a non-owning `shared_ptr<const Sock>` control block only for the temporary `EventsPerSock` key. After the fix, the focused ownership test passed and the test was kept as the regression for this finding.

With the ownership fix applied but the EOF check unchanged, the separate EOF regression ran to the assertion and failed because `RecvEvent` was absent. That isolates Candidate B from the lifetime bug. Candidate A is ready for its own source/test commit; Candidate B remains the next independent fix.

### Candidate B verdict: confirmed

## Cycle 103 completion

- Findings committed independently as `604ae6c294` (`test: avoid taking ownership of stack DynSock`) and `d272f68ea3` (`test: report DynSock EOF as readable`), both authored as `Lőrinc <pap.lorinc@gmail.com>`.
- Validation passed with `CCACHE_DIR=/data/my_storage/tmp/cycle103-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j4`. The full `netbase_tests` suite passed 27 cases. `net_tests/v2transport_test`, `net_tests/transport_v1_roundtrip_message_contract`, and `net_tests/transport_rejected_send_preserves_message` each passed; the two new `netbase_tests/dynsock*` regressions passed together; and `git diff --check` passed.
- No local fuzz executable was available under the searched build roots, so transport fuzzing was not rerun in this cycle. The unrelated PID `777094` wallet test remained untouched.
- Production `src/net.cpp` received no changes. Review of `CNode::ReceiveMsgBytes`, V1/V2 byte framing, `SocketHandlerConnected`, `SocketSendData`, `GenerateWaitSockets`, and `ShouldReconnectV1` did not yield another independently reproducible defect after excluding prior transport and lock-order findings.
- Next unchecked queue: scripted `SocketHandlerConnected` short-write/EOF/error combinations; V2 partial-handshake `ShouldReconnectV1` and half-close schedules; and `GetBytesToSend`/`more` predictions during backpressure and queued-message handoff. Preserve the two shim fixes and do not repeat their now-indexed cells without new evidence.

`DynSock::WaitMany()` now treats the `Pipe::GetBytes()` result as readiness whenever it is nonnegative. A positive result represents queued bytes and zero represents EOF, matching the production `poll`/`select` contract that a closed stream is readable. The focused pair

```text
TMPDIR=/data/my_storage/tmp/cycle103-test /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=netbase_tests/dynsock* --log_level=test_suite
```

passed both the non-owning lifetime regression and the EOF readiness regression after the second fix. The EOF regression failed before this change after Candidate A was applied, so it is independently attributable. Candidate B is ready for its own source/test commit.
