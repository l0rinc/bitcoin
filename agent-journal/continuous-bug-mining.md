# Continuous Evidence-First Bug Mining

## Cycle 260: reject disallowed RPC clients before HTTP request parsing

- Selected index: `0`
- Selected slug: `continuous-bug-mining`
- Selector: `shuf -i 0-98 -n 1`
- Branch: `uber-cycle-260-continuous-bug-mining-20260802`
- Start HEAD: `b46b53bb73c49ce71cda1c86e5853ce734c1ebd9`
- `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `1309 45` (`HEAD...origin/master`)
- Catalog, goal TSV, prompt, and uber-protocol hashes matched the authoritative values in `uber-goal-state.md`.
- Gate: tracked worktree and index were clean; `git diff --check` passed; existing untracked artifacts were preserved. The protected long-running test processes were left untouched. Scratch builds, datadirs, and temp directories were under `/data/my_storage/tmp` because the root filesystem is full.

### Scope and prior evidence

The only prior entry in this journal was Cycle 142, whose directory-commit queue is already closed by `50c503d0dcb7086d940fb2638ea19251c561adfc`. A fresh history scan found no current journal entry closing the early RPC allowlist cell. The prior stale-PR ledger explicitly dismissed #35772 as a partial/duplicate direction but retained #35592, and the new `origin/master` history contains its follow-up commit `d1ed2a6e25d7e64943fbff5e3a7053c55c0617e4`, `http: check rpcallowip immediately after accepting connection`. That commit is not an ancestor of the current branch.

The current tree still had a file-scope `rpc_allow_subnets` and a `ClientAllowed()` check only in `MaybeDispatchRequestToWorker()`. `SocketHandlerConnected()` reads network bytes into the client receive buffer, `ReadRequest()` parses control data and headers and calls `LoadBody()`, and only after a complete request is available does the worker path issue `403 Forbidden`. Therefore a client outside `-rpcallowip` can be connected, send a slow or large body, and consume parsing, buffering, and request-body allocation work before authorization rejects it. The allowlist also could not be tested at accept time because it was not owned by `HTTPServer`.

### Hypothesis and trust boundary

An unauthorised network peer should be rejected immediately after the operating system supplies its source address. Deferring the address check until after HTTP parsing violates the RPC exposure/resource boundary: the peer is not allowed to consume HTTP request parsing and body resources, and it should not receive a protocol-level `403` response that proves the server accepted the connection. The trust boundary is the remote socket address and the HTTP request bytes; this is a remotely reachable resource-exhaustion and exposure-hardening issue, not a consensus change.

### Independent reproduction

The pre-fix functional baseline used the current old binary `/data/my_storage/tmp/cycle243-build/bin/bitcoind` and the original `rpc_bind.py`. The non-loopback test completed successfully, including the disallowed case, because the old server returned `403 Forbidden` after parsing the request.

The stronger pre-fix oracle was run against a disposable binary built from the exact start commit `b46b53bb73` in `/data/my_storage/tmp/cycle260-pre-fix-build`. The current test script was changed to accept connection-level network errors for the disallowed case. Command:

`python3 test/functional/rpc_bind.py --configfile=/data/my_storage/tmp/cycle260-pre-fix-build/test/config.ini --tmpdir=/data/my_storage/tmp/cycle260-rpc-bind-prefix --cachedir=/data/my_storage/tmp/cycle260-cache-prefix --nonloopback --loglevel=INFO`

It failed at the negative assertion with `test_framework.util.JSONRPCException: non-JSON HTTP response with '403 Forbidden'` and left the old behavior observable. The pre-fix binary was built in a separate worktree from the start HEAD; no current source or test changes were used to build it.

### Fix and verification

The minimal fix moves the allowlist and its parsing into `HTTPServer`, initializes it before socket threads, checks `ClientAllowed(addr)` in `AcceptConnection()`, destroys and returns the accepted socket for a forbidden address, and removes the later worker-level `403` check. Direct socket fixtures initialize the list and force the mocked peer address `5.5.5.5` into `-rpcallowip`. The cycle-259 pipelined fixture is also a direct `HTTPServer` user, so it receives the same explicit initialization. The functional test centralizes acceptable connection-error exceptions and asserts that allowed clients succeed while a disallowed client does not complete an RPC exchange.

- Post-fix build: `TMPDIR=/data/my_storage/tmp/cycle260-http-build-tmp CCACHE_DIR=/data/my_storage/tmp/cycle260-http-ccache cmake --build /data/my_storage/tmp/cycle243-build --target test_bitcoin bitcoind -j2`; passed.
- Python validation: `python3 -m py_compile test/functional/rpc_bind.py test/functional/interface_http.py test/functional/test_framework/netutil.py`; passed.
- Focused socket tests, each in an independent scratch temp directory: `http_server_socket_tests` passed 20 assertions; `http_server_pipelined_request_backpressure` passed 6 assertions; `http_socket_error_tests` passed 5 assertions.
- Full HTTP unit suite: `TMPDIR=/data/my_storage/tmp/cycle260-http-suite-tmp /data/my_storage/tmp/cycle243-build/bin/test_bitcoin --run_test=httpserver_tests --log_level=message --report_level=short --color_output=false`; passed all 9 cases and 355 assertions.
- Post-fix functional command: `python3 test/functional/rpc_bind.py --configfile=/data/my_storage/tmp/cycle243-build/test/config.ini --tmpdir=/data/my_storage/tmp/cycle260-rpc-bind-post2 --cachedir=/data/my_storage/tmp/cycle260-cache-post2 --nonloopback --loglevel=INFO`; passed. The allowed request succeeded and the disallowed request raised an accepted network error, so the new negative oracle returned false as intended.
- Adjacent functional command: `python3 test/functional/interface_http.py --configfile=/data/my_storage/tmp/cycle243-build/test/config.ini --tmpdir=/data/my_storage/tmp/cycle260-interface-http-post --cachedir=/data/my_storage/tmp/cycle260-cache-interface --loglevel=INFO`; passed all HTTP persistence, malformed-input, authentication, size-limit, and timeout checks.
- Review evidence: upstream `d1ed2a6e25` independently records the same accept-time contract and changes the expected `rpc_bind.py` behavior from `403` to a network error. The current implementation was compared against that patch and against the direct mock-socket tests. `git diff --check` passed.

The first post-fix full HTTP run exposed a missing allowlist initialization in the newer cycle-259 pipelined test and aborted before cleanup; that test setup was corrected, and the focused and full HTTP suites were rerun successfully. It was a test-harness omission, not a product failure. No timeout, input narrowing, catch that hides a product failure, or broader suppression was used.

### Change and verdict

The source/test change spans `src/httpserver.cpp`, `src/httpserver.h`, `src/test/httpserver_tests.cpp`, `src/test/util/setup_common.cpp`, and the three functional-test Python modules. It does not alter allowed-client behavior, RPC authentication, protocol parsing, consensus, or wallet state. It closes the pre-parse authorization/resource boundary and makes the server's allowlist state explicit per `HTTPServer` instance.

**Confirmed and fixed.** The source/test/journal commit is recorded after this journal update as `http: reject disallowed RPC clients before parsing`, authored as `Lőrinc <pap.lorinc@gmail.com>`. Next cycles must perform a fresh gate and exact random draw, exclude this accept-time allowlist cell and the old #35772 duplicate, and select a new current evidence cell from recent history, coverage, tooling, or an unclosed risk-map boundary. The repository is not considered exhausted.

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
