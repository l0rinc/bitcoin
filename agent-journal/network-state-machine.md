# Network state-machine audit

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

`DynSock::WaitMany()` now treats the `Pipe::GetBytes()` result as readiness whenever it is nonnegative. A positive result represents queued bytes and zero represents EOF, matching the production `poll`/`select` contract that a closed stream is readable. The focused pair

```text
TMPDIR=/data/my_storage/tmp/cycle103-test /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=netbase_tests/dynsock* --log_level=test_suite
```

passed both the non-owning lifetime regression and the EOF readiness regression after the second fix. The EOF regression failed before this change after Candidate A was applied, so it is independently attributable. Candidate B is ready for its own source/test commit.
