# Critical whole-history must-fix sweep

## Cycle 281

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `49`
- Goal: `Critical whole-history must-fix sweep`
- Slug: `critical-history-must-fix`
- Branch: `uber-cycle-281-critical-whole-history-20260802`
- Start HEAD: `162354fe1400981e7ac90b2b450a00d7076df6a9`
- `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `45 1352` (`origin/master...HEAD`)
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- State SHA256 at gate: `56059a944a5b7eb6fe66342c883c9a46c0fe1257653a757e67592d56186d4029`
- Gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; all protected long-running test processes remained alive. Existing untracked artifacts are preserved and excluded from commits.

### Scope and historical seed

This cycle searched recent critical fixes, whole-history variants, backports, callers, and prior journals. The earlier Cycle 73 `SpawnProcess` finding (`9105c2db16`) concerned descriptor ownership when setup failed before or around `fork()`. That cell was explicitly excluded from reopening. The current candidate is a different phase: descriptor cleanup in the forked child after a successful `socketpair()` and before `execvp()`.

The seed was upstream merge `1a2523e901a6c7c876c8a0817601e77d83f394b9`, especially `4afbabdcef86c095b19b3b42b70a2483db8cab4a` and `8ab4b9fc856433ebcaaefb31524cb71ef8ff8089`. Those fixes establish that `RLIM_INFINITY` and values at or above `INT_MAX` must be clamped before converting an OS resource limit to `int`. The upstream PR description records the observed `-1` startup failure from `RLIM_INFINITY` and the analogous overflow for a large finite limit.

### Candidate: `SpawnProcess` closes neither the highest finite descriptor nor unrepresentable limits correctly

Trust boundary: `mp::SpawnProcess()` is used by the Bitcoin IPC child path (`src/ipc/process.cpp:36`) and receives the parent process's inherited descriptor table. After `fork()`, the child must close every descriptor from 3 through the OS soft `RLIMIT_NOFILE` boundary except the IPC socket retained as `fd0`, before executing the requested program.

Before this cycle, `src/ipc/libmultiprocess/src/mp/util.cpp` implemented:

```cpp
size_t MaxFd()
{
    struct rlimit nofile;
    if (getrlimit(RLIMIT_NOFILE, &nofile) == 0) {
        return nofile.rlim_cur - 1;
    } else {
        return 1023;
    }
}
```

The caller assigned that result to `int maxFd` and used `for (int fd = 3; fd < maxFd; ++fd)`. This has two independently demonstrated defects:

1. With a finite soft limit of 64, `MaxFd()` returned 63 and the exclusive loop stopped at 62, leaving inherited descriptor 63 open.
2. With `RLIM_INFINITY` or a value above `INT_MAX`, the `size_t` result can narrow to a negative `int` (or otherwise implementation-defined value), so the loop can close no inherited descriptors. This is the same conversion class already fixed in the main Bitcoin startup path, but the vendored IPC helper had no corresponding clamp.

The finite case is directly reachable without special privileges. A scratch process set its soft limit to 64, opened `/dev/null` until descriptor 63, freed two lower descriptors for `socketpair()`, and spawned `/bin/sh -c 'test -e /proc/self/fd/63'`. The unmodified current helper produced `child_exit=0 sentinel_fd=63`, proving that the inherited descriptor survived `execvp()`. This differs from the earlier setup-failure leak: the child was successfully created and the leak occurred during the normal cleanup loop.

### Fix and independent verification

`MaxFd()` now returns an exclusive upper bound as `int`. It clamps `RLIM_INFINITY` and values at or above `std::numeric_limits<int>::max()` using `std::cmp_greater_equal`, returns the finite soft limit without subtracting one, and uses 1024 rather than 1023 for the `getrlimit()` failure fallback. The new Linux `spawn_tests.cpp` regression creates the exact descriptor-63 boundary and asserts that the executed child cannot see `/proc/self/fd/63`.

Evidence:

- Original-source scratch command, linked against the pre-fix helper: `.../cycle281-spawn-fd-build/current/spawn-fd` -> `child_exit=0 sentinel_fd=63`.
- Fixed-source scratch command, linked against rebuilt `cycle246-mp-base/libmultiprocess.a`: `.../cycle281-spawn-fd-build/fixed/spawn-fd` -> `child_exit=1 sentinel_fd=63`.
- Rebuilt with `CCACHE_DIR=/data/my_storage/tmp/cycle281-ccache ninja -C /data/my_storage/tmp/cycle246-mp-base mptest -j2`.
- Focused command: `mkdir -p /data/my_storage/tmp/cycle281-mptest-final && TMPDIR=/data/my_storage/tmp/cycle281-mptest-final /data/my_storage/tmp/cycle246-mp-base/test/mptest --filter=spawn_tests.cpp --verbose` -> all 3 spawn tests passed, including the new regression.
- Full command: `mkdir -p /data/my_storage/tmp/cycle281-mptest-all && TMPDIR=/data/my_storage/tmp/cycle281-mptest-all /data/my_storage/tmp/cycle246-mp-base/test/mptest` -> all 18 libmultiprocess tests passed.
- `git diff --check` passed after the source and test changes.

The finite boundary is runtime-confirmed before and after with an independent standalone probe and the permanent test. The infinity and large-finite branches are proven by the exact source conversion path and the historical upstream clamp rationale; they were not forced in this host because the resulting cleanup loop would require scanning an impractically large descriptor range. No full Bitcoin build or daemon IPC startup test was required to establish the helper-level defect.

### Handoff

This cycle has one confirmed reachable finding and one self-contained source/test commit. Next queue: fresh gate and selector draw; do not reopen the earlier `SpawnProcess` setup-failure ownership cell unless new evidence shows a distinct phase or invariant.

## Cycle 96

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `49`
- Goal: `Critical whole-history must-fix sweep`
- Slug: `critical-history-must-fix`
- Branch: `uber-cycle-96-critical-whole-history-20260729`
- Start HEAD: `0fd60c504baa6dee79663866a06042b0a3bad996`
- `origin/master`: `9b38d077f894d27ea76413b1db1cb040e25dc296`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `29 983` (`origin/master...HEAD`)
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; no `bitcoind`, `test_bitcoin`, or libsecp test process was running. Existing untracked artifacts, including the previous cycle's probe, are preserved and excluded from commits.

### Scope and protocol

Search the full repository history for critical correctness, security, resource, persistence, cryptographic, wallet, and network fixes. For each historical seed, extract the exact invariant, trust boundary, missing guard/state transition, regression oracle, affected configurations, review rationale, and any prerequisite or follow-up. Search for current structural analogs rather than names alone, then verify only a reachable omission that is not already covered by the existing journal or test suite.

Prior cells excluded from this cycle include the Cycle 94 undecodable first cursor key fix, Cycle 95 MuSig parser output-state fix, the prior mempool relay-budget series, the earlier secp backend/vector campaigns, and historical fixes already used as direct evidence in those cycles. A historical bug is a seed, not proof that a similar-looking current site is wrong.

For every candidate record the seed commit and version, bug shape, source-to-sink path, current callers, configuration/module reachability, expected invariant, reproduction or proof, and verdict. Require a failing-before/passing-after regression, minimized fixture/fuzz seed, first-invalid sanitizer/static trace, mutation/coverage delta, or rigorous contract proof before changing code. Keep discovery and verification independent when practical. One self-contained finding per commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with its journal update; if no fix is justified, leave one journal-only close snapshot.

### Initial queue

1. Mine historical security/correctness commits with explicit regression tests and group them by invariant: bounds/overflow, failure cleanup/output state, persistence ordering, network parser/state, and accounting.
2. Search current analogs for the strongest shapes from `1a51aa96` (bounded PSBT keypath data), `1e009146` (oversized locator rejection), `c6c22f18` and `58b53774` (resource/fee arithmetic), and `20286797` (cryptographic boundary handling).
3. Search current analogs for `4691fb07`, `bb1070bb`, `192eab04`, `6132bf81`, `c4a73a8`, `55eaf087`, `f6c37d82`, and recent wallet write-failure fixes, focusing on rollback, output invalidation, and restart symmetry.
4. Search current analogs for `ddbb88ed`, `6fbcd164`, `4f867fc`, `ee9ad3c`, and the relay-budget series, focusing on ignored read failures, bounded queues, accounting, and state after disconnect.
5. Only after a local candidate is concrete, inspect reverts, backports, PR review, and whole-history variants for applicability and prior rejection rationale.

### Evidence ledger

### Historical seed review

The strongest historical bounds and fail-closed seeds were compared with the current tree and the existing journal before treating any analog as new:

| Seed | Invariant/shape | Current result |
| --- | --- | --- |
| `1e0091464c` | Reject a `getblocks`/`getheaders` locator before generic vector allocation | Already fixed on this branch by the Cycle 82 `ReadBlockLocator` change; excluded as a direct duplicate. |
| `1a51aa96` | Bound PSBT Taproot BIP32 keypath data before allocation | The current bounded parser and regression evidence are recorded in `serialization-untrusted-input.md`; no unbounded analogous field was found in this pass. |
| `192eab04`, `6132bf81` | Failed cryptographic decode must not publish partial output | Current AES-CBC and AEAD paths preserve the documented output contract; their regression evidence is recorded in `error-path-state.md`. |
| `55eaf087`, `f6c37d82` | Wallet persistence failure must not publish in-memory state | Current passphrase/address-book write paths and the later spent-state publication fix are covered by the error-path journals. |
| `ddbb88ed`, `6fbcd164`, `ee9ad3c5f9` | Read failure or per-peer request bound must not leave invalid network state | Current compact-block, `GETBLOCKTXN`, and direct-fetch paths contain the reviewed disconnect/bound checks; these are prior fixes, not current variants. |
| `4f867fc`, relay-budget follow-ups | Duplicate identities and filtered work must not inflate retained relay accounting | Current global WTXID queues and budget refunds were previously audited; no separate historical recurrence was found. |
| `dc3a2b9c3b`, `9fe5896a44`, `5aea3d0373` | Bound external record lengths, Tor control lines, and private-broadcast fanout | Current BDB migration, Tor line, and private-broadcast paths contain the corresponding limits and tests. |
| `e9ed898a0d`, `1ed799fb21`, `2189a6f5f2`, `359680b74d`, `794befd4b0`, `2fe34808fa`, `8313591715` | Durable-tip ordering, object lifetime, arithmetic saturation, lock ordering, atomic settings, signer failure, and unsolicited-block handling | Spot checks of the current implementations found the historical invariant or an explicit guard; no independently reachable omission was proven. |

The seed search was conducted by commit subject, `git log -S/-G`, current call/dataflow inspection, and searches of `agent-journal/` for prior findings. Historical fixes remain evidence sources, not proof that every similar-looking site is defective.

### Candidate: oversized `NOTFOUND` vector

Trust boundary: an unauthenticated peer controls the `notfound` payload. At `src/net_processing.cpp:5438-5450`, the handler currently deserializes a generic `std::vector<CInv>`, then converts entries only when `vInv.size() <= node::MAX_PEER_TX_ANNOUNCEMENTS + MAX_BLOCKS_IN_TRANSIT_PER_PEER`. The current constants are 5,000 and 16, so vectors above 5,016 are fully parsed and then produce an empty `tx_invs` result.

The generic `VectorFormatter` allocates in batches of at most `MAX_VECTOR_ALLOCATE=5,000,000` bytes (`src/serialize.h:36-39, 667-697`), while the transport rejects payloads above `MAX_PROTOCOL_MESSAGE_LENGTH=4,000,000` (`src/net.h:65`, `src/net.cpp:774-780`). A maximally shaped `NOTFOUND` payload therefore causes a bounded transient vector allocation and decode cost, roughly one `CInv` (36 bytes) per entry and at most about 111,000 entries before the message limit, while the already-existing receive payload is also bounded. The vector is not retained, and the downstream state transition receives no entries when the 5,016-entry condition fails. Deserialization failures, including `Vector length limit exceeded`, are caught by `ProcessMessages` as expected peer-message failures (`src/net_processing.cpp:207-225, 5567-5589`) without a retained partial result.

This is a real late-filtering optimization lead: `LIMITED_VECTOR` could avoid parsing entries that the handler deliberately discards. It is not a demonstrated critical current defect in this cycle. Cycle 57/82's resource campaign already traced `MAX_INV_SZ`, the 4 MiB message boundary, and related P2P queue cleanup and classified the bounded operation as no limit bypass or retained-state failure. No live memory-pressure reproducer or protocol contract requires `NOTFOUND` to be rejected at 5,016 rather than ignored after parsing, and the focused binary is absent from this checkout. Verdict: **dismissed as a new critical must-fix**; retain only if a later cycle supplies a distinct per-peer amplification, CPU-budget, or message-processing proof.

### Candidate: wallet fee-bumper arithmetic

`src/wallet/feebumper.cpp` combines the new feerate fee, the selected transaction's old fee, and `combined_bump_fee`; `src/wallet/spend.cpp` and `src/wallet/coinselection.cpp` aggregate ancestor bump fees and selected values. Current `CFeeRate::GetFee` uses saturating arithmetic, wallet amount paths enforce `MoneyRange`, and existing wallet/fuzz coverage exercises fee-bump accounting. The source inspection found no reachable wraparound, negative-value publication, or mismatch between the checked amount domain and the aggregate. Verdict: **dismissed**, with no source change justified.

### Commands and key output

- `shuf -i 0-98 -n 1` -> `49`; the TSV row is `critical-history-must-fix`.
- `git fetch origin master` passed. `origin/master` was `9b38d077f894d27ea76413b1db1cb040e25dc296`; merge-base was `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `29 983`.
- `git log --all -S'MAX_INV_SZ' --oneline -- src/net_processing.cpp src/protocol.h src/net_processing.h` found the prior locator and direct-fetch limit history; `git log --all -S'LimitedVectorFormatter' --oneline -- src/serialize.h src/test/serialize_tests.cpp` found the helper introduction in `94ed45427c` and its serialization test.
- `git show origin/master:src/net_processing.cpp` retains the same `NOTFOUND` generic-vector path. `git show origin/master:src/serialize.h` retains the 5 MiB batch formatter and the current vector semantics.
- `git diff --check` passed before close. No tracked worktree or index changes were present apart from the cycle journal start snapshot.
- `build_unit_clang19/bin/test_bitcoin` was not present, and `find . -maxdepth 3 -type f -name test_bitcoin -perm -111` found no test binary. Focused `serialize_tests`/`net_tests` execution was therefore unavailable; no test result is claimed.

### Review precedent and limitations

The historical fixes consistently use an explicit bound, fail-closed output/state contract, or a narrowly scoped regression test. The current `NOTFOUND` observation does not meet that bar for a critical fix because its work is bounded by the authenticated message framing and it does not create retained peer state. The absent build also prevented a runtime allocation measurement, sanitizer run, and a regression test for the optimization lead. No source or test change is committed.

### Handoff

Cycle 96 closes as a journal-only, no-finding cycle. Next queue: perform a fresh gate and selector draw; for the next critical-history cycle, prioritize a historical fix with an unreviewed persistence or cross-module state invariant, then inspect reverts/backports and recent upstream security commits not already represented in the ledgers. Do not reopen the oversized-locator, wallet output-state, relay-budget, or `NOTFOUND` cells without new evidence.
