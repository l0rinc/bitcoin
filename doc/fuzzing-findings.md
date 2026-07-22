# Fuzzing Findings

This is the local record of findings from the fuzz-contract investigation. Severity is
assessed against the clean `origin/master` baseline, not against an assertion or oracle
that was added by this branch. A finding is called a production defect only when a
clean-master reproducer or an independent race/sanitizer result demonstrated a source
bug. Contract assertions and better fuzzer construction are recorded separately.

The baseline when this file was written was `32eb52100296718f7c0469e3210ce1db73694793`.
The branch was rebased onto that baseline before this file was added. The baseline must
be refreshed after every later rebase.

## Confirmed production defects

### 1. Index publication race and restart state

* Current branch fix: `2e88f807c8` (`index: synchronize chainstate publication during restart`).
* Related startup fix: `fd40472deff` (`index: handle synced index before genesis activation`).
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

* Current branch fix: `3a298d4117` (`coins: serialize DB cursor lifetime with cache resize`).
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

* Current branch fix: `5675fb4a05` (`p2p: bound outbound transport message types`).
* Severity on the clean baseline: low to medium memory-safety hardening. No
  unauthenticated remote path to construct the overlong internal message was proven.

`V2Transport::SetMessageToSend()` copied an overlong direct/internal message type into
the fixed-size transport field. A deterministic unit that bypassed the normal capped
message generators produced an ASan heap-buffer-overflow. The normal RPC and fuzz
paths already cap message types, so the finding is at the transport API boundary, not
an established network attack. The production boundary now rejects the message and
preserves it for the caller.

### 4. RBF fee-diagram total overflow

* Current branch fix: `cf744ba8fc` (`rbf: reject overflowing fee diagrams`).
* Severity on the clean baseline: medium policy robustness issue, conditional on
  extreme local transaction prioritisation; not consensus or wallet loss.

Two independent mempool entries prioritised to extreme modified fees caused
`CalculateChunksForRBF()` to produce aggregate fee/size coordinates outside the
representable-total precondition of `CompareChunks()`. A clean-master `tx_pool` ASan/
UBSan artifact reached the comparison, and the assertion-enabled path aborted. The
fix returns `UNCALCULABLE` before `CompareChunks()`, so the replacement fails closed.
The deterministic `rbf_tests/improves_feerate_diagram_rejects_overflowing_totals`
case catches the mutation while the pre-existing RBF tests did not.

The later `228c637014` fail-closed policy change is a separate behavior decision: it
can make replacements involving extreme local priority deltas temporarily
non-replaceable. It is a policy trade-off, not a consensus vulnerability.

### 5. Mempool info fee-delta signed overflow

* Current branch fix: `2e017e848a` (`mempool: saturate TxMempoolInfo fee delta`).
* Severity on the clean baseline: medium for authenticated/local RPC robustness; no
  unauthenticated network trigger was demonstrated.

After adding a transaction with a positive base fee, two authenticated
`prioritisetransaction` calls could leave the modified fee at `INT64_MIN` while
`GetInfo()` evaluated `INT64_MIN - base_fee`. Clean-master ASan/UBSan aborted at the
`TxMempoolInfo` conversion. The fix uses a saturating subtraction and checks the
public view against that bounded result. The deterministic test reproduces the exact
two-step mutation, and the tx-pool sanitizer replay passed afterward.

### 6. Cache allocation percentage overflow

* Current branch fix: `0d01df4fcc` (`node: avoid cache allocation percentage overflow`).
* Severity on the clean baseline: low local-configuration correctness issue.

On a 64-bit host, `-dbcache=8796093022208` makes the byte total `2^63`. The old
`total_cache * 10 / 100` calculation wrapped before applying the index cap, giving a
zero txindex allocation instead of the one-GiB cap. The boundary unit and
`cache_sizes` fuzzer catch the old arithmetic; this is not P2P/RPC input and does not
affect consensus or persisted chain data.

### 7. Base58 decoder stale output on failure

* Base58Check fix: `f95539c3b0`.
* Raw Base58 fix: `390bde6d88`.
* Severity on the clean baseline: low parser API correctness issue.

The public wrappers could return `false` while leaving caller-owned bytes from a
previous successful decode, including the embedded-NUL early return and max-length
failure paths. No current caller was shown to treat stale bytes as a successful
decode, but the boundary was ambiguous. Both wrappers now clear output on failure,
have production postconditions, deterministic tests, and corpus-backed normal and
ASan/UBSan fuzz coverage.

## Compact-block short-ID investigation

### Historical real defect, fixed on current master

* Branch accounting work: `f50dfe996b` and the collision-focused fuzz commits.
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
  reached a null dereference. The guard is in `4b6e6fd35d`; current production extra
  cache construction was changed by #35670 (`1a3cbf1bd2`) to avoid null tail entries.
  `c7258b01d8` adds the distinct extra-source/null ordering to both the unit oracle and
  fuzzer. This remains direct-API hardening on current master.
* More than `uint16_t` short-ID positions wrapped the internal position map and could
  overwrite the wrong slot. `7c6cc2387d` rejects the oversized direct input. Wire
  deserialization already rejects a transaction count above `uint16_t::max`, so no
  remote compact-block path was demonstrated.
* `FillBlock()` left partial header/counter state after too-short input and left
  derived counters after a successful fill. `078238ed7d`, `d34751cfa5`, and
  `e5ae33f338` reset or preserve state at the reusable-object boundary. The production
  caller discards failed requests, so no remotely reachable state corruption was
  shown.

The collision fuzzer now constructs valid `<wtxid, transaction>` pairs, exercises
mempool-first, extra-first, duplicate, null, prefilled, early-exit, and terminal
collision orderings, and checks source/counter/slot invariants. Two-worker normal and
ASan/UBSan corpus replays completed without new artifacts after the current-master
recheck. A source review also checked the exact `uint16_t` position boundary: 65,535
transactions is representable, while 65,536 is rejected before allocation or index
mapping. No additional wraparound candidate was found. No compact-block race in the
extra vector was found; its access remains serialized by the message-processing mutex.

## Findings that were not production bugs

* Coins-cache contracts, cluster-mempool topology/fee diagrams, validation signal
  payloads, block-filter equivalence, parser atomicity, and many cache/index oracles
  caught mutations in newly added assertions or invalid fuzzer inputs. They did not
  reproduce an unmodified clean-master defect unless listed above.
* The coins money-range work (`576f3519dd`) caught fuzz-generated invalid coins being
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

Before this requested rebase, the compact-block unit suite passed 27 cases in the
Assume-aborting Debug build. Current-master compact and coins corpus controls were
run with multiple workers and existing QA corpora under Clang ASan/UBSan without
reports. After the explicit rebase onto the fetched `32eb521002` tip:

* `blockencodings_tests` passed all 27 cases; the selected hash, Base58, cache,
  mempool, RBF, HTTP, network, and index suites passed 81 cases.
* `cmpctblock` and `partially_downloaded_block` each ran with two sanitizer workers
  over the existing QA corpus and completed with exit code 0 and no target artifacts.
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

The additional `package_rbf` and `clusterlin_linearize` ASan workers were stopped
after more than five minutes on expensive corpus cases; they emitted no sanitizer
marker or target artifact, but are not counted as completed corpus gates.

Scratch data from this round was removed after the artifact and sanitizer scans; the
shared QA corpora and ccache were preserved. This file should be amended if a later
master rebase changes the status of any item above.
