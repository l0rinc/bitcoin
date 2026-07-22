# Fuzzing Findings

This is the local record of findings from the fuzz-contract investigation. Severity is
assessed against the clean `origin/master` baseline, not against an assertion or oracle
that was added by this branch. A finding is called a production defect only when a
clean-master reproducer or an independent race/sanitizer result demonstrated a source
bug. Contract assertions and better fuzzer construction are recorded separately.

The current baseline after the final fetch and rebase is
`efa1800a885c1ae605e18605ef73957ea13e575c`. The reorg campaign below ran immediately
before that fetch against `559d042ba2567a05e8d540c7d9d9a94c7d2973d2`; the four commits
between those tips touch only Qt, ZeroMQ dependencies, and include-lint tooling, so
they do not change the validation, compact-block, mempool, TxGraph, coins, or fuzz
code exercised here. Controls that explicitly name
`32eb52100296718f7c0469e3210ce1db73694793` are historical clean-master runs from an
earlier baseline; they remain valid evidence for the mutations they tested, but are
not claims about commits added since then.

## Confirmed production defects

### 1. Index publication race and restart state

* Current branch fix: `0bd90a32dc` (`index: synchronize chainstate publication during restart`).
* Related startup fix: `9ade168c8f` (`index: handle synced index before genesis activation`).
* Severity on the clean baseline: medium availability/correctness issue for an index
  restart or snapshot-completion workflow; no unauthenticated P2P trigger, consensus
  divergence, wallet loss, or persisted-state corruption was demonstrated.

`BaseIndex::Init()` obtained `m_chainstate` under `cs_main` but published the pointer
after releasing the lock. A reader in `BlockUntilSyncedToCurrentChain()`, `TxIndex`, or
`TxoSpenderIndex` could race that write while an index was stopped and reinitialized.
The clean tree reproduced the race under TSan with a reader thread performing lookups
while 1000 `Stop()`/`Init()` cycles ran. The fix publishes under `cs_main` and protects
the dependent lookups. The same investigation found a pre-genesis null state in which
an index could be marked synced before genesis notification was drained; the startup
test reproduced the old dereference and now covers the queued genesis transition.

### 2. Persistent coins cursor versus database resize

* Current branch fix: `5de77fca62` (`coins: serialize DB cursor lifetime with cache resize`).
* Severity on the clean baseline: medium local/authorized-workflow availability bug.

`CCoinsViewDB::ResizeCache()` could destroy and reopen the LevelDB database while a
cursor still owned an iterator. The existing resize test and coins fuzzer used an
in-memory database and missed this path. A clean-master ASan/UBSan reproducer created a
persistent cursor, resized the database from another thread while holding `cs_main`,
and then advanced the cursor; LevelDB aborted in `VersionSet::~VersionSet()`. TSan and
ASan/UBSan reproductions pass after the cursor holds the database mutex until iterator
destruction. The new deterministic test and `coins_view_db_resize_cursor` fuzzer
verify the complete UTXO set before and after resize.

### 3. V2 transport message-type buffer overflow at a direct boundary

* Current branch fix: `4e1f1a0f05` (`p2p: bound outbound transport message types`).
* Severity on the clean baseline: low to medium memory-safety hardening. No
  unauthenticated remote path to construct the overlong internal message was proven.

`V2Transport::SetMessageToSend()` copied an overlong direct/internal message type into
the fixed-size transport field. A deterministic unit that bypassed the normal capped
message generators produced an ASan heap-buffer-overflow. The normal RPC and fuzz
paths already cap message types, so the finding is at the transport API boundary, not
an established network attack. The production boundary now rejects the message and
preserves it for the caller.

### 4. RBF fee-diagram total overflow

* Current branch fix: `ce3fd38431` (`rbf: reject overflowing fee diagrams`).
* Severity on the clean baseline: medium policy robustness issue, conditional on
  extreme local transaction prioritisation; not consensus or wallet loss.

Two independent mempool entries prioritised to extreme modified fees caused
`CalculateChunksForRBF()` to produce aggregate fee/size coordinates outside the
representable-total precondition of `CompareChunks()`. A clean-master `tx_pool` ASan/
UBSan artifact reached the comparison, and the assertion-enabled path aborted. The
fix returns `UNCALCULABLE` before `CompareChunks()`, so the replacement fails closed.
The deterministic `rbf_tests/improves_feerate_diagram_rejects_overflowing_totals`
case catches the mutation while the pre-existing RBF tests did not.

The pre-rebase hashes `cf744ba8fc` and `228c637014` were duplicate local copies of
the same fail-closed production patch (identical patch IDs); the rebase retained one
current copy as `ce3fd38431`. The behavior trade-off is therefore part of that one
fix, not a separate later vulnerability or fix: replacements involving extreme local
priority deltas can be temporarily non-replaceable. It is a policy trade-off, not a
consensus vulnerability.

### 5. Mempool info fee-delta signed overflow

* Current branch fix: `dd3385cf0d` (`mempool: saturate TxMempoolInfo fee delta`).
* Severity on the clean baseline: medium for authenticated/local RPC robustness; no
  unauthenticated network trigger was demonstrated.

After adding a transaction with a positive base fee, two authenticated
`prioritisetransaction` calls could leave the modified fee at `INT64_MIN` while
`GetInfo()` evaluated `INT64_MIN - base_fee`. Clean-master ASan/UBSan aborted at the
`TxMempoolInfo` conversion. The fix uses a saturating subtraction and checks the
public view against that bounded result. The deterministic test reproduces the exact
two-step mutation, and the tx-pool sanitizer replay passed afterward.

### 6. Cache allocation percentage overflow

* Current branch fix: `7971db2096` (`node: avoid cache allocation percentage overflow`).
* Severity on the clean baseline: low local-configuration correctness issue.

On a 64-bit host, `-dbcache=8796093022208` makes the byte total `2^63`. The old
`total_cache * 10 / 100` calculation wrapped before applying the index cap, giving a
zero txindex allocation instead of the one-GiB cap. The boundary unit and
`cache_sizes` fuzzer catch the old arithmetic; this is not P2P/RPC input and does not
affect consensus or persisted chain data.

### 7. Base58 decoder stale output on failure

* Base58Check fix: `c8808363c4`.
* Raw Base58 fix: `4796db0ef3`.
* Severity on the clean baseline: low parser API correctness issue.

The public wrappers could return `false` while leaving caller-owned bytes from a
previous successful decode, including the embedded-NUL early return and max-length
failure paths. No current caller was shown to treat stale bytes as a successful
decode, but the boundary was ambiguous. Both wrappers now clear output on failure,
have production postconditions, deterministic tests, and corpus-backed normal and
ASan/UBSan fuzz coverage.

## Compact-block short-ID investigation

### Historical real defect, fixed on current master

* Branch accounting work: `31026b969f` and the collision-focused fuzz commits.
* Upstream fix already in current master: `6aa5d8d948` (PR #35727).
* Severity on the pre-PR master: low to medium internal reconstruction accounting;
  no consensus failure or uninstrumented release crash was demonstrated.

The reproducer fills a short-ID slot from the mempool, then processes a distinct
extra transaction with the same short ID. The old collision path unconditionally
decremented `extra_count`, even though the slot was mempool-sourced, underflowing the
`size_t` counter from zero. The source-state model and deterministic collision tests
proved the transition. Current master decrements the extra count only for an
extra-sourced slot, so this particular bug is no longer present on the baseline.

### Direct-API hardening, not a current P2P vulnerability

The following cases were reproduced against clean master by constructing
`PartiallyDownloadedBlock` inputs directly, then fixed or covered on this branch:

* A valid extra transaction followed by a null extra entry with the same short ID
  reached a null dereference. The guard is in `b99ada5789`; current production extra
  cache construction was changed by #35670 (`1a3cbf1bd2`) to avoid null tail entries.
  `e7408f4f5f` adds the distinct extra-source/null ordering to both the unit oracle and
  fuzzer. This remains direct-API hardening on current master.
* More than `uint16_t` short-ID positions wrapped the internal position map and could
  overwrite the wrong slot. `c2bff602b0` rejects the oversized direct input. Wire
  deserialization already rejects a transaction count above `uint16_t::max`, so no
  remote compact-block path was demonstrated.
* `FillBlock()` left partial header/counter state after too-short input and left
  derived counters after a successful fill. `bc7d8eef54`, `b9468a012e`, and
  `2236cbd59c` reset or preserve state at the reusable-object boundary. The production
  caller discards failed requests, so no remotely reachable state corruption was
  shown.
* Constructing `CBlockHeaderAndShortTxIDs` from an empty or sparse in-memory `CBlock`
  could underflow the short-ID vector size or dereference a null transaction while
  deriving the IDs. `46cb5a5f64` adds the production preconditions and deterministic
  `blockencodings_tests/HeaderAndShortIDsRejectsInvalidBlockTxRefs` coverage. This is
  direct-API contract hardening: no clean-master P2P caller was found that constructs a
  compact block from an empty or sparse source block, so its severity on the baseline is
  none confirmed rather than a remotely reachable crash.

The collision fuzzer now constructs valid `<wtxid, transaction>` pairs, exercises
mempool-first, extra-first, duplicate, null, prefilled, early-exit, and terminal
collision orderings, and checks source/counter/slot invariants. A source review also
checked the exact `uint16_t` position boundary: 65,535 transactions is representable,
while 65,536 is rejected before allocation or index mapping. No additional wraparound
candidate was found. No compact-block race in the extra vector was found; its access
remains serialized by the message-processing mutex.

The focused mutation comparison used the same 3,646-byte seed directory on both
implementations. The branch collision harness completed 1,000 mutations in each of
two ASan/UBSan workers and each of two TSan workers, with no report or artifact. A
fresh ASan/UBSan build of exact master `32eb52100296718f7c0469e3210ce1db73694793`,
using the unmodified upstream `partially_downloaded_block` harness, completed 1,000
mutations in each of two workers in 10 and 11 seconds, also without a report or
artifact. The master comparison rules out a sanitizer-visible defect in the
production path under natural mutations; it does not claim that upstream's original
harness reaches every branch-specific forced collision mode added on this branch.

### Network fallback oracle gap closed

The wire `cmpctblock` fuzzer already had a mutation that made two distinct
transaction positions carry the same short ID, but it did not establish that the
network handler took the required fallback. The new deterministic sequence first
announces the valid header so the block is the first in-flight request, then sends
the duplicate-short-ID compact block. `PartiallyDownloadedBlock::InitData()` must
return `READ_STATUS_FAILED`; the peer must remain connected, receive a full-block
`GETDATA` rather than `GETBLOCKTXN`, and successfully complete when the full block
is delivered. The functional test records this exact mutation and also restores the
consumed UTXO so it does not perturb later compact-block cases.

This was a coverage omission, not a new master defect. The first version of the
network oracle deliberately asserted `GETDATA` after sending a random block header,
but sanitizer replay found four fuzzer-fixture assumptions before collision handling:
the peer could be pre-`VERACK`, the random header version could be rejected, a
coinbase-only block could have zero short IDs, or the header could be ignored by
`CanDirectFetch`/low-work anti-DoS policy. None produced a production report. The
corrected fixture requires a connected, non-private peer with block and witness
service plus `NoBan` permission for the one-header trusted-peer setup, sets mock node
time relative to the active tip, forces a valid version-4 header with unique nonce/time,
and builds a coinbase plus two valid witness spends before recomputing the commitment,
merkle root, and header hash. The minimized sanitizer artifact from the false oracle
now passes in both sanitizer builds.

The final corrected branch replay used the existing 1,970-input `cmpctblock` corpus
with two independent workers per sanitizer. TSan completed 1,971 executions per
worker in 84 seconds; ASan/UBSan completed 1,976 executions per worker in 544 seconds.
Every job exited 0 with no sanitizer report or artifact. An isolated exact-master
`32eb52100296718f7c0469e3210ce1db73694793` build was previously given the same
temporary test-only collision hook and deterministic first-in-flight network oracle
while leaving production code at the master revision; its two ASan/UBSan workers
completed 1,000 mutations each without a report or artifact. The direct unit
duplicate-ID case therefore has a network-level regression test and a matching fuzzer
postcondition, with no production fix warranted on current master.

### Later parallel peer collision

The second wire mutation creates a valid first request from peer A, then sends the
same duplicate-short-ID compact block from a later high-bandwidth peer B. The required
postconditions are that peer B stays connected without `GETDATA` or `GETBLOCKTXN`, the
peer-A request remains in flight, and a valid compact block from peer A still produces
`GETBLOCKTXN`, accepts the two returned transactions in `BLOCKTXN`, and advances the
tip. The deterministic functional test `test_duplicate_short_id_parallel_peer`
exercises this exact sequence and passed as part of the complete
`p2p_compactblocks.py` run.

This is also a coverage omission, not a production bug: the relevant peer-ordering
logic is already present on master, and no clean-master crash, disconnect, request
loss, or sanitizer report was found. The branch changes involved here are fuzzer
construction, production `PartiallyDownloadedBlock` contract hardening from the
earlier compact investigation, and deterministic test coverage; no new production
behavioral fix was required for the parallel case.

For an independent clean-master check, an ASan/UBSan build at
`32eb52100296718f7c0469e3210ce1db73694793` replayed all 2,015 existing
`partially_downloaded_block` inputs with two jobs and two workers: both workers ran
2,016 executions and exited 0 without a report or artifact. The clean-master
`cmpctblock` replay reached 256 executions per worker before being interrupted in a
high-cost generated case; it produced no report or artifact and is explicitly an
incomplete gate, not a pass.

## Findings that were not production bugs

### Full-range TxGraph fee mutation (2026-07-22)

The `txgraph` fuzzer previously selected the alternate `AddTransaction` and
`SetTransactionFee` fees from `[-0x8000000000000, 0x7ffffffffffff]`, approximately
`+/-2^47`. The mutation was widened to the complete signed `int64_t` range while
leaving transaction sizes, cluster limits, and graph topology limits unchanged. The
existing 5,120-input TxGraph corpus, run by two independent normal workers, exposed
three fuzzer-contract omissions. All three were reproducible with the same artifact in
both workers and replayed cleanly after correction:

* A 66-byte input (minimized to 28 bytes, SHA256
  `01381d33ed4df56d7b4a87b998c566cb623e0739c115dbbd8007ec5e639bab49`) reached
  `CompareChunks()` with cumulative `FeeFrac` coordinates that overflowed. The
  production function documents that callers must establish this precondition; the
  fuzzer now checks `CanCompareChunks()` before applying that ordering oracle at both
  call sites. The corresponding unminimized input has SHA256
  `10983e359fba7ea9872b1713b8f7efd1e16f2f4138524807af3ee8469b57a5ca`.
* A 177-byte input (SHA256
  `955f0a272b3138c794179c470cdccc85c8f02c74c0535e0178f69d950dcffbbb`) made the
  staging-gain oracle compare saturated diagram sums and a saturated difference. The
  fuzzer now uses `CheckedFeePerWeightSum` and checked signed subtraction, asserting
  equality only when both sides are representable. This preserves useful full-range
  graph execution without treating saturation as exact arithmetic.
* A 661-byte input (SHA256
  `bcc22d7da7483cd13df351d47ff4fa4a51d6d7bea594a3734963475469ef5d59`) exposed a
  stale fuzzer oracle in `GetWorstMainChunk`: it modeled raw `ChunkLinearizationInfo`
  while branch production commit `9ceae5b32f` intentionally uses `GetChunking()` to
  collapse a connected cluster when `FeeFrac` saturation makes raw chunks appear
  disconnected. The oracle now uses the same `chunk_linearization_info_fn` fallback.
  This was a branch-local fuzzer omission, not a new production defect.

For a clean-master control, an isolated worktree at
`32eb52100296718f7c0469e3210ce1db73694793` received only the two fee-range widenings.
Its ASan/UBSan build reached the first `CompareChunks` assertion in the fuzzer
(`src/test/fuzz/txgraph.cpp:1264`) and emitted no production stack frame or sanitizer
diagnostic. This proves the first failure is an invalid fuzzer oracle on master, not a
master production failure. No production patch or new deterministic production test was
added for these three cases: the production saturation fallback is already covered by
`txgraph_saturated_chunking_keeps_block_builder_connected`, and the corrected fuzzer
oracles now avoid asserting non-representable arithmetic.

The corrected branch fuzzer completed two normal workers at 10,000 executions each,
two ASan/UBSan workers at 6,123 executions each, and two TSan workers at 6,120
executions each. All jobs exited zero without a report or artifact. The sanitizer
replays of the 28-, 177-, and 661-byte inputs also exited zero. This is coverage and
oracle hardening, with severity `n/a` for production security: no new clean-master
crash, race, consensus issue, or remotely reachable vulnerability was found.

* Coins-cache contracts, cluster-mempool topology/fee diagrams, validation signal
  payloads, block-filter equivalence, parser atomicity, and many cache/index oracles
  caught mutations in newly added assertions or invalid fuzzer inputs. They did not
  reproduce an unmodified clean-master defect unless listed above.
* The coins money-range work (`6f88cd9668`) caught fuzz-generated invalid coins being
  sent directly to amount compression. Consensus-valid transaction and snapshot
  callers already satisfy `MoneyRange()`; the fix documents that precondition and
  makes the fuzzer reject invalid construction.
* Several fuzz targets originally caught broad exceptions. The exception-narrowing
  commits classify expected parse/policy failures and allow unexpected exceptions to
  escape. This improves discovery signal; no additional production bug was proven by
  those changes.
* Release builds compile `Assume()` out. The validation-interface tests were adjusted
  to expect `NonFatalCheckError` only in Assume-aborting builds; this was a definite
  test configuration issue, not a shipped-node vulnerability.
* The CI supply-chain audit is maintained on the separate `audit/supply-chain` line:
  `48d6460315` pins shellcheck/mlc assets and `09a2c19d8e` pins SDK downloads. Before
  those fixes, mutable executable/SDK assets lacked hash verification; the worst case
  included token exposure and cache persistence. These are repository/CI risks, not
  fuzz-discovered runtime Bitcoin Core bugs.

## Verification status

Before the latest rebase, the compact-block unit suite passed 27 cases in the
Assume-aborting Debug build. Current-master compact and coins corpus controls were
run with multiple workers and existing QA corpora under Clang ASan/UBSan without
reports. Most of the following detailed gates were run on the preceding clean
baseline `32eb521002`; the branch is now rebased onto
`efa1800a88`. The later `validation_block_reorg` gate ran on the intermediate
`559d042ba2` baseline and is called out separately below.

* `blockencodings_tests` passed all 27 cases; the selected hash, Base58, cache,
  mempool, RBF, HTTP, network, and index suites passed 81 cases.
* The corrected `cmpctblock` fixture replayed its 1,970 existing QA inputs with two
  independent workers under both Clang ASan/UBSan and Clang TSan. ASan/UBSan completed
  1,976 executions per worker in 544 seconds; TSan completed 1,971 per worker in 84
  seconds. Every job exited 0 with no report or target artifact. The separate
  `partially_downloaded_block` replay also completed with two workers and no report or
  artifact.
* `coins_view` replayed all 21,873 QA inputs in each of two sanitizer workers with
  exit code 0 and no artifacts.
* A current Clang TSan/libFuzzer build (distinct from the single-input TSan driver)
  replayed `tx_pool`'s 8,000 inputs, `cmpctblock`'s 1,970 inputs, and
  `partially_downloaded_block`'s 2,015 inputs with two jobs and two workers. Every
  job exited 0; no TSan report or artifact was produced. The two compact targets
  therefore have both ASan/UBSan and TSan corpus evidence on the current stack.
* The current Clang ASan/UBSan wallet build replayed `coincontrol`'s 497 inputs with
  two jobs and two workers for 1,000 runs per worker. All jobs exited 0; no sanitizer
  report or artifact was produced. The same build replayed `coinselection_bnb`'s 575
  inputs and `coinselection_srd`'s 393 inputs with two jobs and two workers for 2,000
  runs per worker. All four jobs exited 0; no sanitizer report or artifact was
  produced.
* The current Clang TSan/libFuzzer build also replayed `coincontrol`'s 497 inputs and
  `coinselection_bnb`'s 483 inputs with two jobs and two workers for 1,000 runs per
  worker. Every job exited 0; no TSan report or artifact was produced. These are
  race-signal gates only, not evidence that wallet code is generally thread-safe.
* The same TSan build replayed `coinscache_sim`'s 1,515 inputs with two jobs and two
  workers. Both jobs exited 0 with no TSan report or artifact; this exercised the
  cache overlay fetches and their persistent thread pool, but the workers still use
  independent cache instances.
* The transaction-state TSan gates replayed `txdownloadman_impl`'s 1,560 inputs,
  `txorphan_protected`'s 646 inputs, and `txorphanage_sim`'s 1,346 inputs with two
  jobs and two workers. Every job exited 0; no TSan report or artifact was produced.
* `checkqueue` replayed its 2,003-input QA corpus with two jobs and two workers under
  both Clang ASan/UBSan and Clang TSan. Every job exited 0, including inputs that
  selected zero through two internal queue workers; no sanitizer report or artifact
  was produced. The ASan workers completed 3,008 executions each with peak RSS of
  about 694 MB, while the TSan workers completed in 3 seconds.
* `dbwrapper_concurrent_reads` replayed its 1,116-input QA corpus under Clang TSan
  with two jobs and two workers; each worker also created eight concurrent readers,
  and every job exited 0 without a TSan report or artifact. The parallel ASan run
  reached 512 mutations per worker before remaining in one expensive read workload
  until the five-minute cutoff; it produced no sanitizer report or artifact and is
  not counted as a completed ASan gate.
* `dbwrapper_threaded` replayed its 2,528-input QA corpus and `threadpool` replayed
  its 896-input QA corpus under both Clang TSan and Clang ASan/UBSan, with two jobs
  and two workers per target. Every job exited 0 without a sanitizer report or
  artifact. The ASan `dbwrapper_threaded` workers completed 2,532 executions each
  in about 257 seconds (peak RSS about 744 MB); the TSan workers completed in about
  20 seconds. The ASan `threadpool` workers completed 1,000 and 1,003 executions
  respectively, and its TSan workers completed in about 9 seconds.
* The deterministic `dbwrapper` target replayed its 1,835-input QA corpus under the
  same two-job/two-worker ASan/UBSan and TSan setup. Every job exited 0 without a
  sanitizer report or artifact; ASan workers completed in about 163 seconds with
  peak RSS around 711 MB, and TSan workers completed in about 22 seconds.
* `banman` replayed its 1,825-input QA corpus under both Clang ASan/UBSan and Clang
  TSan with two jobs and two workers. Every job exited 0 without a sanitizer report
  or artifact; the ASan workers completed in about 171 seconds with peak RSS around
  771 MB, while the TSan workers completed in about 44 seconds. The corpus included
  corrupted persistence, clear/reload transitions, and extreme signed ban-time
  offsets.
* `connman` replayed its 3,405-input QA corpus under both sanitizers with two jobs
  and two workers. Every job exited 0 without a sanitizer report; ASan workers
  completed 3,440 and 3,441 executions in about 179 seconds with peak RSS around
  1,001 MB, while TSan workers completed in about 32 seconds. The ASan run emitted
  one 235 KB slow-unit input at 21 seconds; the non-sanitized fuzzer replayed it in
  3,401 ms with exit code 0, so it is sanitizer/mock-socket overhead rather than a
  production performance defect.
* `addrman` replayed its 2,191-input QA corpus with two jobs and two workers under
  Clang TSan. Both jobs completed all 2,194 seed and generated inputs, taking 352
  and 354 seconds with peak RSS of about 325 MB, and neither produced a TSan report
  or artifact. The corresponding ASan/UBSan jobs reached 1,115 and 1,077 executions
  before the bounded run was stopped at the expensive state-building region; neither
  produced a sanitizer report or target artifact. `addrman_serdeser` was similarly
  exercised with two jobs and two workers under both sanitizers. The ASan workers
  reached 191 executions each and the TSan workers 817 and 823 executions before
  the same bounded cutoff, without diagnostics. Its three slow-unit artifacts
  replayed in the normal fuzzer in 4,984, 5,954, and 6,242 ms with exit code 0.
  These are incomplete sanitizer gates and performance observations, not evidence
  of an addrman production defect.
* `headers_sync_state` ran 1,000 inputs in each of two ASan/UBSan jobs and two TSan
  jobs over its 887-input corpus. All jobs exited 0 without a sanitizer report or
  artifact. ASan workers completed in 8 seconds with peak RSS of 460 and 462 MB;
  TSan workers completed in 1 and 2 seconds with peak RSS of 138 and 140 MB. This
  exercised continuous-header construction, commitment transitions, redownload,
  proof-of-work output ordering, and locator invariants.
* `validation_block_reorg` was run against the then-current
  `origin/master` tip `559d042ba2`, immediately before the final fetch and rebase.
  Two independent normal workers each completed 5,000
  executions; two independent TSan workers each completed 5,000 executions in
  about 584--585 seconds with peak RSS of 310--311 MB; and two independent
  ASan/UBSan workers each completed 5,000 executions in 693 and 696 seconds with
  peak RSS of 644--646 MB. Every job exited 0 without a sanitizer report or
  artifact. The corpus and mutations exercise genesis reprocessing, empty
  competing branches, same-height forks, longer competing forks and reorgs,
  invalid blocks with descendants, invalidation, failure-flag reset, precious
  block no-ops, flush-before/after transitions, and chainstate notification
  assertions. The later master delta is unrelated to this target. This follow-up
  found no new production defect or race.
* `validation_load_mempool` completed its 1,799-input corpus under TSan with two
  jobs and two workers: both workers executed 1,802 inputs in 274 seconds, with
  peak RSS of 397 and 398 MB, and no report or artifact. A follow-up ASan/UBSan
  campaign from two fresh copies completed 5,000 executions per worker in 949 and
  927 seconds, at peak RSS of 827 and 829 MB, also without a diagnostic or
  artifact. The corpus exercised current and legacy dump formats, metadata option
  combinations, unbroadcast state, repeated pool replacement, and atomic dump
  failure paths, including the previously expensive round-trip region.
* `tx_pool` was replayed from two fresh copies of its 8,000-input QA corpus under
  ASan/UBSan. Two independently captured workers completed 10,000 executions in
  982 and 884 seconds, with peak RSS of 667 and 672 MB; neither produced a
  sanitizer report or artifact. This covered cluster topology and fee diagrams,
  ancestor/descendant repair, randomized and witness indexes, RBF/TRUC policy,
  block-builder chunk iteration, prioritisation, and mempool information views.
  No new production defect or race was found.
* `process_messages` completed its 4,431-input corpus under TSan with two jobs and
  two workers: 4,433 and 4,434 executions in 157 seconds, peak RSS about 341 MB,
  no report or artifact. `process_message` likewise completed 3,699 and 3,698
  inputs under TSan in 86 and 89 seconds, with peak RSS of 361 and 364 MB and no
  report or artifact. The ASan/UBSan `process_messages` jobs reached 5,179 and
  5,187 executions before the bounded run was stopped at the expensive region;
  peak RSS was 724 and 725 MB, with no diagnostic or artifact. These network
  gates are evidence against a race in the exercised message/peer lifecycle, not
  a proof that all live connection interleavings are race-free.
* `txrequest` completed 1,000 inputs per TSan worker with two jobs and two workers,
  reaching peak RSS of 148 MB and producing no report or artifact. Its ASan/UBSan
  workers reached the 512 pulse in the same high-cost mutation region at about
  717 and 718 MB RSS before being stopped; no diagnostic or artifact was produced,
  so this is an incomplete ASan gate. Before the full-range fee mutation was
  corrected, `txgraph` completed all 5,123 inputs in each TSan worker in 96 and 97
  seconds at about 139 MB RSS, with no report or artifact. Its then-current ASan
  workers reached the 4,096 pulse at about 681 and 682 MB RSS before the bounded
  cutoff, again without a diagnostic or artifact. The final full-range gate and its
  exact counts are recorded above; together these runs directly exercised
  cluster-mempool topology mutations and transaction request bookkeeping without
  revealing a race.
* The standard mempool target was replayed from 2,826 existing inputs below 64 KiB
  with two jobs and two workers. TSan completed 2,827 executions per worker in 102 and
  103 seconds at about 345 MB RSS, with no report or artifact. ASan/UBSan reached 512
  executions per worker in the bounded window at about 545 and 550 MB RSS, then was
  stopped in an expensive mutation region without a diagnostic or artifact; it is an
  incomplete ASan gate, not evidence of a mempool defect.
* A current `tx_pool_standard` ASan/UBSan run from two fresh copies of its 2,857-input
  corpus reached 2,851 executions per worker before the 1,200-second outer cap, at
  peak RSS of 744 and 755 MB, without a sanitizer or crash artifact. Two slow units
  were identified (`962e3f76102eff4245e9a42eb4197ffc8978a7a9`, SHA256
  `22710aabce82d919a26683b30133b6257649a208ab1f20d4839ee2177aa49485`, and
  `f5e2f1eada3a748963a1cb4cadaa2bf224eac617`, SHA256
  `7a307c1df10352957d291594b758fbe8d8f3ba98f7ce5f9c0aa083286c8d8a26`). Their exact
  replays exited 0 under ASan/UBSan in 10.838 and 16.991 seconds and under TSan in
  1.176 and 2.142 seconds, with no report. This is an incomplete performance-limited
  gate, not a standard-mempool defect.
* `clusterlin_linearize` completed the 672-input QA corpus under TSan with two jobs
  and two workers: 1,000 executions per worker in 31 and 33 seconds at peak RSS of
  126 MB, with no report or artifact. The full ASan/UBSan run reached 436 and 437
  executions before two 15-18 second exhaustive-model inputs held both workers;
  it produced only slow-unit artifacts, no sanitizer diagnostic. A supplemental
  run over the same 672 inputs with `-max_len=64` completed 1,000 executions per
  worker in 9 and 10 seconds at 279 and 281 MB, also clean. The slow-unit input
  replayed with the normal fuzzer in 3,184 ms and exited 0, so this is model
  complexity and sanitizer overhead, not a production performance finding.
* `package_rbf` was replayed from a 761-file subset of its 1,111-input corpus with
  inputs below 64 KiB. TSan completed 1,000 executions per worker in 26 and 27
  seconds at peak RSS of 154 and 157 MB, with no report or artifact. ASan/UBSan
  reached 674 and 675 executions at peak RSS of 665 and 667 MB before the same
  high-cost region was stopped; no diagnostic or artifact was produced, so that
  earlier ASan run was incomplete. A current follow-up from two fresh copies of
  the same 761-file filtered corpus completed 1,000 executions per worker in 261
  and 262 seconds at peak RSS of 688 and 690 MB, with no sanitizer report or
  artifact. No package-RBF production defect or race was found.
* `p2p_private_broadcast` had no saved QA corpus, so it was run from an empty seed
  directory. Its ASan/UBSan workers completed 1,000 executions each in 59 and 52
  seconds at peak RSS of 684 and 686 MB; its TSan workers completed 1,000 and 1,001
  executions in 11 and 10 seconds at 320 and 316 MB. Every job exited 0 without a
  sanitizer report or artifact. The run covered IBD and post-IBD broadcast aborts,
  private-broadcast handshake/relay combinations, duplicate and pending
  transactions, GETDATA/PONG confirmation, mixed peer types, and malformed inbound
  messages. It is discovery evidence, not a corpus-backed regression gate.
* `http_request` completed its 11,681-input corpus with two jobs and two workers
  under ASan/UBSan and TSan. ASan/UBSan completed 11,684 executions per worker in
  53 and 54 seconds, with peak RSS of 710 and 712 MB; TSan completed 11,684 per
  worker in 16 seconds, with peak RSS of 135 and 136 MB. Every job exited 0 without
  a sanitizer report or artifact. This covered the libevent-replacement
  `HTTPRequest`, `HTTPHeaders`, `HTTPRemoteClient` parser, response, receive-buffer,
  send-buffer, connection-state, query, and body-size contracts. It does not by
  itself cover a live `HTTPServer` accept-loop and shutdown integration.
  The existing eight-case `httpserver_tests` suite then passed under the Clang TSan
  unit build in 1.74 seconds, including the live socket loop, malformed-request
  disconnect, send errors, and shutdown/join paths, with no TSan report.
* The package-acceptance targets were also exercised from filtered corpora. The
  `tx_package_eval` subset contained 2,440 of 2,871 inputs below 64 KiB; ASan/UBSan
  reached 729 and 731 executions, and TSan reached 1,976 and 1,977 executions,
  before the workers were stopped in the expensive package-building region. The
  `ephemeral_package_eval` corpus had 2,098 inputs, all below 64 KiB; ASan/UBSan
  reached 578 and 579 executions and TSan reached 1,011 per worker. No ASan/UBSan
  diagnostic or actual TSan data-race report occurred. The interrupted TSan logs
  contain only the runtime's signal-unsafe warning from libFuzzer's interrupt
  handler after the deliberate stop, so these are incomplete gates rather than
  production findings.
* A current `tx_package_eval` ASan/UBSan campaign from two fresh copies of the
  2,440-input filtered corpus reached 2,001 and 2,000 executions before deliberate
  interruption at the same 12-second slow unit
  (`e383bdbfe9aaf3d4104e7441e29eeb05596de952`, SHA256
  `b05e7e360c48be5c11dc76f870763957c17810c6c7da9e89ff597c7ee9f62b8d`). Neither
  worker produced a sanitizer or crash artifact. Replaying that exact input once
  took 10.675 seconds under ASan/UBSan and 857 ms under TSan, both with exit code
  0 and no report. This is sanitizer/model complexity, not a package-evaluation
  defect; the corpus gate remains incomplete.
* A follow-up `ephemeral_package_eval` replay used the 2,057 corpus inputs below
  16 KiB to reduce input-size confounding. With two workers per sanitizer, the
  ASan/UBSan workers reached `#512` each and the TSan workers reached `#1024`
  each before the deliberately bounded run was interrupted; no sanitizer
  diagnostic, race report, timeout artifact, or crash artifact was produced.
  The workers were CPU-bound in libFuzzer's seed-corpus replay, not deadlocked:
  a perf sample was in `ReadAndExecuteSeedCorpora`, and the target's bounded
  package-building loop was the hot path. A separate 32-seed ASan/UBSan probe
  completed normally (`#33 DONE`, 67 seconds) with no diagnostic. This is still
  an incomplete corpus gate, not evidence of a production defect; the TSan
  signal-unsafe warning from the deliberate interrupt is a sanitizer-runtime
  artifact caused by stopping while the target was unwinding.
* `p2p_transport_serialization` completed 1,000 inputs in each ASan/UBSan worker
  (136 and 143 seconds, peak RSS about 770 MB) and each TSan worker (11 seconds,
  peak RSS about 157 MB). `p2p_transport_bidirectional_v1v2` likewise completed
  1,000 inputs per worker under ASan/UBSan in 176 seconds at 747 and 749 MB RSS,
  and under TSan in 54 and 55 seconds at 148 and 152 MB RSS. All jobs exited 0
  without a sanitizer report or artifact. The V2 bidirectional TSan jobs completed
  1,629 inputs each in 96 and 97 seconds at 153 and 154 MB RSS, also clean. Its
  ASan/UBSan jobs reached pulse 1,024 at roughly 724–729 MB RSS before the bounded
  cutoff, with no diagnostic or artifact; that ASan gate is incomplete.
* `p2p_handshake` completed its 2,858-input corpus under TSan with two jobs and
  two workers (2,860 and 2,861 executions in 23 and 25 seconds, peak RSS 318 and
  322 MB) and under ASan/UBSan (3,351 and 3,335 executions in 91 seconds, peak
  RSS about 698 MB). `bip324_cipher_roundtrip` completed 1,524/1,523 inputs under
  ASan/UBSan in 186/187 seconds at 669/673 MB RSS and 1,524 inputs per TSan worker
  in 50/52 seconds at 129/126 MB RSS. `bip324_ecdh` completed 1,543/1,542 ASan
  inputs in 14/15 seconds at 262/263 MB RSS and 1,543 inputs per TSan worker in
  7 seconds at 136/129 MB RSS. Every completed job exited 0 without a report or
  artifact; these exercised handshake state transitions, late-feature disconnects,
  key agreement, session/garbage-terminator matching, damaged ciphertext, and
  decrypt-output preservation.
* `p2p_headers_presync` completed its 1,031-input QA corpus with two jobs and two
  workers under both sanitizers. The ASan/UBSan workers completed 1,422 and 1,427
  executions in 313 and 312 seconds, with peak RSS of about 733 MB; the TSan
  workers completed 1,034 executions each in 52 and 53 seconds, with peak RSS of
  about 319 and 323 MB. Every job exited 0 without a sanitizer report or artifact.
  The target exercised max-header replies, presync header/compact-block/block
  messages, stale-height and inflight tracking, insufficient-work headers, and
  invariants that the best header and active tip remain unchanged.
* `node_eviction` completed its 609-input QA corpus with two jobs and two workers
  under ASan/UBSan and TSan. ASan/UBSan completed 1,000 executions per worker in
  103 and 104 seconds, with peak RSS of about 505 and 514 MB; TSan completed 1,000
  executions per worker in 26 and 27 seconds, with peak RSS of about 165 and 160
  MB. Every job exited 0 without a sanitizer report or artifact. The corpus
  exercised protected peers, noisy candidates, eligibility changes, and selection
  invariants after peer movement.
* `net` completed its 2,083-input QA corpus with two jobs and two workers under
  both sanitizers. ASan/UBSan completed 2,086 executions per worker in 102 and 103
  seconds, with peak RSS of about 644 and 647 MB; TSan completed 2,086 executions
  per worker in 30 seconds, with peak RSS of about 180 and 182 MB. Every job exited
  0 without a sanitizer report or artifact. This exercised node-reference
  accounting, connection statistics, local-address bookkeeping, and routability
  transitions.
* `local_address` completed its 1,419-input QA corpus with two jobs and two workers
  under both sanitizers. ASan/UBSan completed 1,701 and 1,692 executions in 18
  seconds, with peak RSS of about 311 and 313 MB; TSan completed 1,422 executions
  per worker in 4 seconds, with peak RSS of about 127 and 131 MB. Every job exited
  0 without a sanitizer report or artifact. The `p2p_private_broadcast` target was
  inventoried but had no existing QA corpus, so it is not represented as a completed
  gate here.
* An ASan/UBSan `txorphan` run over 686 inputs was stopped after both workers reached
  the same expensive region. One worker reported a libFuzzer timeout at 39 seconds
  while `TxOrphanageImpl::EraseTx()` computed transaction weight for a large
  synthetic transaction; no ASan or UBSan diagnostic was present. The
  resulting 68,345-byte slow input and 105,144-byte timeout input replayed in the
  non-sanitized fuzzer in 3,977 ms and 7,398 ms respectively, both with exit code 0.
  This is sanitizer overhead on an artificial construction, not a production bug.
  The parallel ASan/UBSan `txdownloadman` run was also stopped at 512 executions per
  worker without a sanitizer report or artifact and is not counted as complete.
* An ASan/UBSan `scriptpubkeyman` run was started with two jobs and two workers over
  10,025 existing inputs, but both workers remained CPU-bound in a single expensive
  case past the five-minute wall-clock budget. It produced no sanitizer marker or
  artifact before being stopped and is not counted as a completed gate.
* A follow-up ASan/UBSan and TSan attempt used hardlinks to the 9,817 existing
  `scriptpubkeyman` inputs below 64 KiB (36 MB total), again with two jobs and two
  workers per sanitizer. Both reached the 4,096 pulse before the five-minute budget
  without a sanitizer report or artifact, then were stopped. The size filter changed
  throughput but did not complete the corpus gate; no wallet-manager defect was
  proven.
* `coinscache_sim` reached 1,416 inputs before the optional run was stopped because
  both workers remained on a 10-second slow corpus unit. That unit replayed once in
  18.6 seconds with exit code 0 and no sanitizer report. This is a fuzzing throughput
  observation, not a confirmed production defect.
* The coins-cache follow-up used the current master baseline
  `32eb52100296718f7c0469e3210ce1db73694793` and two jobs with two workers per
  sanitizer. `coins_view_overlay` completed its 12,723-input corpus after filtering
  inputs above 64 KiB: ASan/UBSan workers completed 12,724 executions each in about
  112 and 113 seconds at peak RSS of 379 and 380 MB; TSan workers completed 12,724
  executions each in 47 and 48 seconds at about 150 MB. No report or artifact was
  produced. This exercised `StartFetching`, asynchronous overlay lookups, cache
  mutation guards, flush/restore transitions, and the backend equivalence checks.
* `coinscache_sim`'s filtered 1,502-input corpus completed under TSan with 1,503
  executions per worker in 188 and 189 seconds at peak RSS of 143 and 144 MB, with
  no TSan report or artifact. Its ASan/UBSan workers reached 1,024 executions each
  and then remained in the same expensive corpus region for more than four minutes;
  no ASan/UBSan diagnostic or artifact was produced, so that run is incomplete. The
  previously recorded full-corpus TSan gate remains the stronger corpus result.
* On the current rebased stack, the same filtered `coinscache_sim` corpus was run
  under ASan/UBSan with two workers. The workers were deliberately interrupted at
  1,401 and 1,400 executions after both repeatedly reached the same 10-second slow
  unit (`8bdf52da47cc919025ba1b2a544f001f69780649`, SHA256
  `b9d7bbc50d256d14256cb38cec34bd25d2501d91a6b466bda2c0b3a0033e2871`); neither
  produced a sanitizer report or artifact. Replaying that exact input once took
  18.6 seconds under ASan/UBSan and 819 ms under TSan, both with exit code 0 and no
  report. This confirms model complexity and sanitizer overhead, not a cache race
  or memory defect; it is not counted as a complete corpus gate.
* `coins_view_db_resize_cursor` was seeded with 256 existing `coins_view_db` inputs
  and completed 1,000 mutated executions per worker under both ASan/UBSan (19
  seconds) and TSan (9 seconds), without a report or artifact. This directly ran the
  concurrent cursor `GetKey`/`GetValue`/`Next` loop across `ResizeCache`, joined the
  resize thread, and checked both pre- and post-resize snapshots for exact equality.
* `coins_view_stacked` had no native QA corpus. A fixed empty-input smoke run repeated
  the stacked setup 1,000 times per worker under both sanitizers, but libFuzzer
  correctly reported that it performed no mutation. A mutation run seeded from
  21,408 existing `coins_view` inputs completed under TSan with 21,874 executions per
  worker in 87 seconds and no report or artifact. The corresponding ASan/UBSan run
  reached pulse 16,384 in the expensive corpus region without a report or artifact;
  a second run over 21,388 inputs below 64 KiB reached the same pulse and was stopped.
  These are incomplete ASan gates, not evidence of a stacked-cache defect.
* The UTXO snapshot follow-up used filtered existing corpora below 64 KiB. Valid
  `utxo_snapshot` ran 1,092 and 1,095 executions under ASan/UBSan and 1,027 per worker
  under TSan, with all jobs exiting 0 and no report or artifact. Invalid
  `utxo_snapshot_invalid` ran 1,359 ASan/UBSan and 1,287 TSan executions per worker,
  also clean. `utxo_total_supply` reached 256 executions per worker under each
  sanitizer without a report or artifact before the intentionally bounded stop; its
  TSan logs contain only the runtime's signal-unsafe warning from that SIGTERM, so this
  is an incomplete gate.
* No production code mistake, race report, or deterministic assertion failure was
  found in this coins-cache round. The resize/cursor target is now covered by a
  multi-threaded sanitizer gate, while the overlay and stacked targets have explicit
  corpus-backed TSan evidence and their ASan limitations are recorded above.

The additional `package_rbf` and `clusterlin_linearize` ASan workers were stopped
after more than five minutes on expensive corpus cases; they emitted no sanitizer
marker or target artifact, but are not counted as completed corpus gates.

Scratch data from this round was removed after the artifact and sanitizer scans; the
shared QA corpora and ccache were preserved. This file should be amended if a later
master rebase changes the status of any item above.
