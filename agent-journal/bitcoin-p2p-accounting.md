# Cycle 148: Bitcoin P2P private-broadcast retry accounting

## Selection and fresh gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `89` (`bitcoin-p2p-accounting`), revisited under the prior ledger's explicit allowance for new deterministic-coverage or lifecycle evidence.
- Branch: `uber-cycle-148-bitcoin-p2p-accounting-20260730`
- Gate and cycle-start HEAD: `c9f01db21582c0368e0402a213f11fb5f0c44e2e`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Gate divergence (`HEAD...origin/master`): `1080 42`
- Tracked and staged state was clean. The persistent wallet test process `777094` and unrelated untracked artifacts were preserved.
- Catalog, random-prompt, goals TSV, and uber-protocol hashes matched the authoritative values.

## Distinct scope and prior-finding exclusion

Cycle 15 fixed stale `m_network_conn_counts` after `CConnman::StopNodes()` and `ConnmanTestMsg::ClearTestNodes()`. That counter excludes `ConnectionType::PRIVATE_BROADCAST`, so this cycle did not repeat the same network-count defect. Cycle 118 separately bounded retained private-broadcast send history, and the later disconnect-removal fix covered `RemoveResult::NumUnstarted()` plus active-node tombstones. Those findings were used as contracts, not repeated.

The new hypothesis came from the current private-broadcast lifecycle exposed by Cycle 147's deterministic coverage audit: when `ReattemptPrivateBroadcast()` finds a stale transaction no longer valid, it removes the transaction but did not cancel the pending connection slots originally added by `InitiateTxBroadcastPrivate()`. The trust boundary is the local stale-transaction scheduler and its atomic `CConnman::PrivateBroadcast::m_num_to_open` work counter. The expected invariant is that removing an invalid transaction cancels exactly `RemoveResult::NumUnstarted(NUM_PRIVATE_BROADCAST_PER_TX)` slots, as the RPC-abort and received-TX paths already do.

## Independent failing-before proof

The regression creates a deliberately invalid empty-input transaction, queues it through the real `PeerManager::InitiateTxBroadcastPrivate()` path, and observes the initial counter value of three. A `FakeNodeClock` advances beyond `PrivateBroadcast::INITIAL_STALE_DURATION`; a real `CScheduler` runs `PeerManagerImpl::StartScheduledTasks()` and therefore the production `ReattemptPrivateBroadcast()` callback. The callback's validation rejects the transaction and removes it.

On the unmodified source:

```text
TMPDIR=/data/my_storage/tmp/cycle148-stale-removal-before \
  /data/my_storage/tmp/cycle89-build/bin/test_bitcoin \
  --run_test=net_tests/private_broadcast_stale_removal_cancels_pending_connections \
  --log_level=message --report_level=short --color_output=false

net_tests.cpp(398): error: check connman.PrivateBroadcastNumToOpenPublic() == 0 has failed [3 != 0]
3 assertions out of 4 passed; 1 failed
```

This is a deterministic accounting failure, not a network timing or malformed-message result: no private-broadcast peer needs to be opened for the invalid stale transaction, but all three scheduled slots remained queued.

## Fix

The invalid-stale branch now retains the `Remove()` result and accumulates `NumUnstarted(NUM_PRIVATE_BROADCAST_PER_TX)` in `connections_cancelled`. After the stale set is processed, `NumToOpenSub(connections_cancelled)` cancels those slots before the existing valid-stale `NumToOpenAdd(num_for_rebroadcast)` operation. The valid-stale behavior remains one new retry connection per scheduled stale transaction, and concurrent removal is handled by accepting a null `Remove()` result.

The test-only `ConnmanTestMsg::PrivateBroadcastNumToOpenPublic()` accessor exposes the existing atomic counter for this regression. The test builds a separate private-broadcast-enabled `PeerManager`, schedules the real callback before starting the scheduler thread, and queues the completion marker at 100 ms so callback ordering is deterministic.

## Validation

- `CCACHE_DIR=/data/my_storage/tmp/cycle148-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j2`: passed after the incremental source/test rebuild.
- Fixed regression with `--random=14802`: 1 selected case, 4/4 assertions passed.
- Combined `net_tests,private_broadcast_tests` with `--random=14803`: 46 cases, 165,233/165,233 assertions passed.
- `git diff --check` passed for all changed files.
- The first build attempt used the host default ccache path and failed before product compilation because `/root/.cache/ccache/tmp` was absent; the isolated cycle ccache rerun completed successfully. This is setup evidence, not a product failure.

## Verdict and limitations

**Confirmed and fixed:** invalid stale private-broadcast removal leaked pending connection work, causing future private-broadcast attempts to open connections for a transaction already discarded by the scheduler. The correction is local to the missing accounting edge and follows the existing removal contract.

The regression does not run a live Tor/I2P socket or a full daemon scheduler. It does execute the actual `PeerManager` stale callback with a real `CScheduler`, fake time, chain validation, private-broadcast queue, and CConnman counter. Valid stale reattempts and live disconnect replacement remain covered by the existing private-broadcast tests and prior lifecycle/fuzz evidence. The full unit suite was not rerun in this cycle; the focused network/private-broadcast suites are the relevant validation boundary.

## Next queue

Audit the remaining private-broadcast lifecycle edges with new evidence only: valid stale reattempt plus a concurrent abort, `StopThreads()` interruption token cleanup, semaphore grants across `StopNodes()` reuse, and v1/v2 disconnect finalization. Do not repeat the fixed stale-removal, retained-history, tombstone, or generic network-count findings.

# Cycle 15: Bitcoin P2P transport, permission, and peer accounting

## Selection and baseline

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `89`
- Selected slug: `bitcoin-p2p-accounting`
- Goal: audit P2P connection lifecycle, transport state, permissions, peer quotas, and accounting across fragmented I/O, reconnect, and teardown paths.
- Timestamp: `2026-07-28T00:47:59Z`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD before this cycle: `fdceb1cb047512657a4d22b3023c0e5b59cf527c`
- Catalog: `agent-journal/reusable-continuous-agent-goals.md`
- Worktree at inspection: only the three cycle files were modified; existing untracked files and directories were agent-owned artifacts. `git diff --check` passed.

The relevant network state is `m_network_conn_counts`, an array guarded by `m_nodes_mutex`. `ConnmanTestMsg::AddTestNode()` increments it for manual and full-outbound nodes. `DisconnectNodes()` decrements it, but `CConnman::StopNodes()` swapped and deleted all live nodes without decrementing or clearing it. The public `MultipleManualOrFullOutboundConns()` predicate consumes the stale value, and `net_processing.cpp` uses that predicate when deciding whether a stale-tip peer is protected from eviction.

## Evidence map

- `src/test/util/net.h:62-68`: test-node insertion increments the per-network count.
- `src/net.cpp:2009-2010`: ordinary disconnect removes the count.
- `src/net.cpp:2592-2596`: the count determines whether more than one manual/full-outbound peer exists on a network.
- `src/net_processing.cpp:5683-5706`: stale-tip eviction uses that predicate, so a phantom count can alter the peer-management oracle in a later fuzz input.
- `src/net.cpp:3775-3786`: `StopNodes()` removed the nodes but left the count behind.
- `src/test/fuzz/p2p_handshake.cpp:132`, `src/test/fuzz/cmpctblock.cpp:755`, and `src/test/fuzz/p2p_headers_presync.cpp:65`: fuzz lifecycles stop and recreate test peers on the same connection manager.
- `src/test/fuzz/connman.cpp:333,445`: the connman fuzz targets use `ClearTestNodes()` to delete generated peers, and that helper also left the count behind.

Open PR #35808 (`fd403f947432067678a9d5e866d06df3e59ad0e1`, fetched into `FETCH_HEAD`) adds `connman.Reset()` after `StopNodes()` in `p2p_handshake` and `cmpctblock`. That is useful evidence that sticky `CConnman` state is an active concern, but the current `ConnmanTestMsg::Reset()` clears address and private-broadcast state only; it does not clear `m_network_conn_counts`. The lifecycle invariant belongs at the point where `CConnman` removes its node set, so the local fix covers all `StopNodes()` callers, including headers presync, rather than relying on selected target resets.

## Hypothesis and independent reproduction

Hypothesis: after `StopNodes()` deletes the only full-outbound IPv4 peer, adding one new full-outbound IPv4 peer leaves the counter at two and makes the production multiple-peer predicate return true. The equivalent test-helper teardown path should also leave a zero count after `ClearTestNodes()`.

The focused regression in `src/test/net_tests.cpp:328-367` creates initialized full-outbound IPv4 peers, checks the predicate, stops the first peer, re-adds a peer, finalizes and clears the second peer through `ClearTestNodes()`, and re-adds a third peer. On the unmodified source, the first re-add check failed:

```text
build_unit_clang19/bin/test_bitcoin --run_test=net_tests/connman_stop_nodes_resets_network_connection_counts --log_level=all --report_level=short
exit 201
net_tests.cpp(357): error: check !connman.MultipleManualOrFullOutboundConnsPublic(Network::NET_IPV4) has failed
```

This is a deterministic two-peer accounting trace, not a timing or malformed-packet claim.

## Fix

- `CConnman::StopNodes()` now swaps the node list and zeroes `m_network_conn_counts` under `m_nodes_mutex` before deleting the detached nodes.
- `ConnmanTestMsg::ClearTestNodes()` now zeroes the same count after deleting its test nodes.
- The test-only `MultipleManualOrFullOutboundConnsPublic()` wrapper exposes the locked production predicate for the regression without weakening production visibility or lock annotations.

The smallest correct behavior is to reset the aggregate when the authoritative node collection is emptied. No protocol, permission, transport, or eviction policy was changed.

## Validation

Builds and tests were run from separate scratch build directories or deterministic corpora:

```text
cmake --build build_unit_clang19 --target test_bitcoin -j2
```

After the fix, the focused test passed with 4/4 assertions. The complete `net_tests` suite passed with 31 cases and 167,549/167,549 assertions:

```text
build_unit_clang19/bin/test_bitcoin --run_test=net_tests/connman_stop_nodes_resets_network_connection_counts --log_level=message --report_level=short
build_unit_clang19/bin/test_bitcoin --run_test=net_tests --log_level=error --report_level=short
```

The rebuilt libFuzzer binary was exercised with eight inputs from each relevant qa-assets corpus, `-runs=100`, fixed seeds, and `-max_len=4096`:

```text
cmake --build build_fuzz_libfuzzer_clang19 --target fuzz -j2
FUZZ=p2p_handshake build_fuzz_libfuzzer_clang19/bin/fuzz /data/my_storage/tmp/bitcoin-p2p-accounting-cycle15/p2p_handshake -runs=100 -max_len=4096 -seed=1501 -print_final_stats=1
FUZZ=cmpctblock build_fuzz_libfuzzer_clang19/bin/fuzz /data/my_storage/tmp/bitcoin-p2p-accounting-cycle15/cmpctblock -runs=100 -max_len=4096 -seed=1502 -print_final_stats=1
FUZZ=p2p_headers_presync build_fuzz_libfuzzer_clang19/bin/fuzz /data/my_storage/tmp/bitcoin-p2p-accounting-cycle15/p2p_headers_presync -runs=100 -max_len=4096 -seed=1503 -print_final_stats=1
FUZZ=connman build_fuzz_libfuzzer_clang19/bin/fuzz /data/my_storage/tmp/bitcoin-p2p-accounting-cycle15/connman -runs=100 -max_len=4096 -seed=1504 -print_final_stats=1
```

All four completed without crashes or assertions. Final smoke summaries were: `p2p_handshake` 100 runs, coverage 6225, peak RSS 856 MB; `cmpctblock` 100 runs, coverage 21742, peak RSS 856 MB; `p2p_headers_presync` 100 runs, coverage 8636, peak RSS 856 MB; and `connman` 100 runs, coverage 6408, peak RSS 856 MB.

## Verdicts and limitations

- Confirmed: stale per-network connection accounting after both `StopNodes()` and `ConnmanTestMsg::ClearTestNodes()`.
- Confirmed: the bug is reachable by reusable fuzz/test connection-manager lifecycles and can affect later peer-management decisions and coverage/oracle behavior.
- Dismissed for this cycle: no additional transport fragmentation, permission transition, quota, reconnect, or partial-I/O defect was proven by the four smoke campaigns.
- Inconclusive: the full qa-assets corpora, sanitizer fuzz build, deterministic coverage comparison, and production daemon shutdown/restart were not rerun in this cycle. The smoke campaigns are state-isolation evidence, not a claim of exhaustive P2P coverage.
- This is primarily a lifecycle/test-harness correctness defect. Normal daemon shutdown destroys the connection manager, but the public `StopNodes()` contract should still leave its aggregate state consistent for reuse and tests.

## Next queue

Re-check peer lifecycle and permission accounting around `DisconnectNodes()`, `m_nodes_disconnected`, transport v1/v2 reconnects, and stale-tip eviction with a sanitized stateful sequence. Inspect the remaining open PRs for analogous teardown omissions before drawing a new distinct goal. Preserve the cycle-15 smoke corpora and the failing-before output as regression evidence.
