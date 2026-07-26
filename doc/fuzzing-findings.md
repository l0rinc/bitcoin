# Fuzzing Findings

This is the local record of findings from the fuzz-contract investigation. Severity is
assessed against the clean `origin/master` baseline, not against an assertion or oracle
that was added by this branch. A finding is called a production defect only when a
clean-master reproducer or an independent race/sanitizer result demonstrated a source
bug. Contract assertions and better fuzzer construction are recorded separately.

The current baseline after the latest fetch and rebase is
`e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6`. The historical exact-master controls
below retain their named baselines; they are evidence for the mutations they
tested, not claims about an earlier branch tip. The latest rebase range from
`afa5e46bbc6dd750bd71920b659162a945abf0ae` to this tip includes network-capacity,
global transaction-rate limiting, and mempool-RPC presentation changes, but does
not change the compact-block,
cluster-mempool core, coins-cache, validation-index, descriptor-cache, or TxGraph
production paths covered by the controls below. Earlier master ranges recorded
in this file retain their own path-by-path qualifications.
Controls that explicitly name an older baseline, including
`32eb52100296718f7c0469e3210ce1db73694793` and `5311b15727f2f282274472184185423e441abd85`,
are historical clean-master runs; they remain valid evidence for the mutations they
tested, but are not claims that those exact controls include later master commits.

## Ledger summary (2026-07-26)

The findings are classified by what failed on an unmodified master baseline. The
branch's assertions, fuzzer-only checks, and deterministic tests are not counted as
new vulnerabilities unless the corresponding clean-master control reproduced a
production failure.

| Classification | Count or status | Current assessment |
| --- | --- | --- |
| Confirmed runtime defects | 42 | Fixed on this branch; the highest severity is medium, with triggers ranging from local/startup/direct-API workflows to malformed snapshots, P2P block ordering, cluster-mempool fee coordinates, malformed fee-estimator inputs, descriptor storage failures, and malformed HTTP/RPC input. No consensus, key-loss, unauthenticated memory-safety, or remote race bug was demonstrated. |
| Historical compact-block defect | 1 | Real short-ID accounting underflow on the pre-`6aa5d8d948` master; already fixed by current master and not counted among current defects. |
| Compact-block direct-API hardening | 4 families | Null/sparse inputs, oversized short-ID positions, and reusable `FillBlock()` state are now guarded and tested; no current P2P trigger was demonstrated. |
| Fuzzer/oracle and coverage omissions | Several | Remaining TxGraph saturation contracts, mempool/cluster assertions, network collision fallback, and parser exception classification were corrected without proving another clean-master production bug. The full-range TxGraph mutation also exposed the production defect recorded below. |
| CI supply-chain findings | 2 | Separate `audit/supply-chain` work: mutable executable and SDK downloads were pinned; these are CI risks, not runtime Bitcoin Core defects. |
| Confirmed consensus, key-loss, unauthenticated memory-safety, or remote race bugs | 0 | No clean-master campaign in this ledger demonstrated one. |

The forty-one confirmed runtime defects are: the index publication and restart race;
the persistent coins cursor versus database resize race; the V2 transport direct
boundary overflow; the RBF fee-diagram overflow; the equal-feerate TxGraph prefix
fee overflow; the saturated cluster chunk fee aggregation mismatch; the mempool info fee-delta
overflow; cache-allocation percentage overflow; coins-cache state capacity
arithmetic; `-maxmempool` byte-size multiplication overflow; stale Base58 output on
decode failure; the descriptor-cache partial merge; stale mining `submitSolution`
failure outputs; stale `TxIndex::FindTx` failure outputs; stale wallet transaction
detail outputs; the descriptor next-index persistence failure; stale
`ProcessNewBlock` `new_block` output after a block-file write failure; the
external-signer `n_signed` reporting omission; private-broadcast removal counter
drift; private-broadcast late-confirmation state drift; the nullable wallet
coin-control dereference; four malformed `bitcoin-cli` HTTP response acceptance
bugs (overlong status codes, status codes below 100, differing duplicate
`Content-Length` values, and invalid chunk terminators); reusable `ArgsManager`
command state; out-of-range compressed amounts; invalid Schnorr-key handling;
exhausted coins-DB cursor reads; and two HTTP request-parser atomicity defects
(partial chunked bodies and partial request fields); work added after an explicit
checkqueue completion; stale reusable sighash precomputation; duplicate serialized
assumeutxo coin records; failed block candidates retained across invalidation and
unlinked recovery; and pruned unlinked blocks no longer linked when their parent
arrives; invalid raw fee-estimator success thresholds; descriptor top-up cache
metadata inconsistency after a descriptor-row write failure; descriptor updates
that publish replacement state before a cache write failure can be rolled back; and
descriptor reservation returns that publish an in-memory index decrement before a
descriptor-row write failure; `MarkUnusedAddresses()` descriptor writes that
leave the live next index ahead of SQLite after a descriptor-row write failure;
descriptor encryption that reports success or publishes encrypted state after
an encrypted-key insert or plaintext-key erase failure.
Their exact reproducer,
clean-master evidence, branch fix, and residual reachability are recorded in the
corresponding sections below. The two race findings are correctness/availability
problems in local or startup workflows, not remotely reachable data races: the
sanitizer campaigns exercised independent fuzzer processes unless a test explicitly
created worker threads.

## Current conclusions

* The confirmed runtime defects below are bounded local, authenticated, startup,
  direct-API, client-parser, or malformed-HTTP workflow bugs. No clean-master run
  in this ledger demonstrated a consensus failure, wallet-key loss, unauthenticated
  remote memory-safety issue, or remotely exploitable race.
* The compact-block short-ID null, underflow, and direct-construction cases are
  explicitly separated by baseline: the underflow is fixed by current master
  `6aa5d8d948`, the normal P2P null-tail construction is avoided by current master
  `6f1c56f03a`, and the remaining branch fixes are direct-API contracts or test
  coverage. No additional compact-block collision defect was found after the rebase.
* The descriptor-cache partial-merge defect below is a real clean-master production
  defect. Its fuzzer construction was temporarily copied into an exact-master
  control, while `src/script/descriptor.cpp` remained unchanged; the fixed branch
  and deterministic unit test both pass the same input.
* The BnB attempt-limit investigation found a fuzzer construction omission, not a
  new production defect: the existing `bnb_exhaustion_with_solution_test` already
  catches the exact `GetAlgoCompleted()` mutant on current master, but no wallet
  fuzzer deliberately generated its 19 near-equal-UTXO, 100,000-attempt case.
* The descriptor top-up write-failure defect below is a clean-master storage
  consistency bug. The exact fuzzer seed fails on master and passes on this branch;
  the deterministic unit test verifies rollback of cache rows and preservation of
  the in-memory descriptor range.
* The descriptor-update write-failure defect below is a separate clean-master
  storage consistency bug. Updating an existing descriptor cleared and replaced
  live maps before `TopUpWithDB()` completed; a rejected cache write left the
  descriptor, script maps, and cached-index boundary disagreeing with one another
  and with the persisted descriptor. The branch now wraps the update in a database
  transaction and restores every affected in-memory map on failure.
* A second mutation of that same descriptor-update defect rejects the database
  insert for a newly supplied private descriptor key. On clean master, the
  plaintext and encrypted key maps were published before the insert completed;
  retrying after the injected failure then treated the key as already present and
  skipped the missing database row. `a2560d7ef1` already rolls back both maps, so
  this round adds the encrypted and plaintext deterministic controls plus the
  matching fuzzer oracle without adding another production change.
* The full-range TxGraph fee mutation was initially classified as fuzzer-only
  because three early failures were invalid arithmetic oracles. A later 384-byte
  input reached a different production path on clean master: `Updated()` summed
  equal-feerate chunk fees even though it only stored their cumulative size. The
  exact-master normal and ASan/UBSan controls reproduced signed overflow before
  the invariant failure; `32ed8c5596` fixes the source and adds a deterministic
  four-transaction regression.
* Current master PR #34672 added `reason` and `debug` output fields to the mining
  `submitSolution` interface. Its null-coinbase early return left caller-provided
  failure strings untouched instead of returning the empty outputs promised by the
  shared submission path. The clean-master C++ control failed with stale strings;
  the branch now clears and asserts both fields, and adds an IPC regression check.
* The coins-cache headroom calculation mixed unsigned cache capacities with signed
  intermediate values. A direct `SIZE_MAX` capacity on a 64-bit clean master made
  an empty cache report `CRITICAL`; this is a local direct-API/configuration
  correctness issue, not a network or consensus path. The same audit found that an
  extreme `-maxmempool` value could overflow the MB-to-byte multiplication: Debug
  with `-ftrapv` trapped and Release accepted the invalid result. Both paths now use
  width-appropriate checked or saturating arithmetic and deterministic tests.
* `Wallet::getWalletTxDetails()` returned an empty `WalletTx` for a missing
  transaction while leaving all caller-owned output parameters unchanged. The Qt
  transaction-description caller default-initializes `WalletTxStatus`,
  `inMempool`, and `numBlocks`, then formats them without a success return value;
  a wallet transaction disappearing between the model snapshot and this lookup
  could therefore read indeterminate status values. The clean-master control
  failed all 13 non-`WalletTx` output postconditions; the branch now clears and
  asserts the outputs and documents the contract.
* `DescriptorScriptPubKeyMan::GetNewDestination()` returned a newly derived
  address after its descriptor-index write failed, leaving memory ahead of the
  persisted wallet state. The exact clean-master SQLite control returned a
  destination and advanced `next_index`; the branch now commits the descriptor
  index to the database before publishing it in memory.
* `MarkUnusedAddresses()` had a separate clean-master descriptor-row failure:
  rejecting the metadata write left the live descriptor at `next_index == 2`
  while SQLite remained at `next_index == 1`, allowing a restart to reuse the
  keypool index. The branch now stages this caller's advance, forces its
  descriptor write, restores memory on failure, and preserves the ordinary
  `GetNewDestination()` write-failure return behavior.
* Descriptor encryption had two independent clean-master database-failure
  omissions. A failed `WALLETDESCRIPTORCKEY` insert was ignored by
  `DescriptorScriptPubKeyMan::Encrypt()`, which could publish encrypted
  in-memory state without an encrypted record on disk. A failed delete of the
  corresponding plaintext `WALLETDESCRIPTORKEY` was ignored by
  `WalletBatch::WriteCryptedDescriptorKey()`, which could commit both records.
  Both are local storage-failure paths; neither produced a remote or consensus
  attack, but either can make a wallet fail to load or leave key material and
  encryption state inconsistent across restart. The branch stages all encrypted
  keys until transaction commit, checks both database operations, and rolls back
  the wallet master-key map and transaction on every failure.
* `ExternalSignerScriptPubKeyMan::FillPSBT()` replaced a PSBT with the signer’s
  result without updating its `n_signed` output. The Qt PSBT operations dialog
  uses this count to report signing progress, so a partial PSBT with one newly
  finalized input was reported as `Could not sign any more inputs.` The exact
  clean-master control below reproduced the stale zero; the branch now counts
  unsigned-to-finalized input transitions after optional finalization.
* The private-broadcast state model had two clean-master accounting defects that
  were absent from the earlier network-fuzzer summary. Removing a transaction
  after multiple picked sends used the number of confirmations to cancel pending
  connections, even though unconfirmed picked sends had already consumed slots.
  Separately, a late PONG could confirm a send after `FinalizeNode()` had already
  scheduled its replacement. Exact-master temporary tests reproduced both cases
  with production sources unchanged; the branch fixes are `800a2aad69` and
  `449636bbe3`, and the fixed nine-case unit suite passes.
* `AvailableCoins()` explicitly accepts a null `CCoinControl`. The clean-master
  wallet path dereferenced it while evaluating unconfirmed TRUC outputs; the
  branch guard and the new unconfirmed-parent test close that API contract. The
  exact control segfaulted with exit 139, while the fixed wallet spend suite
  passes all four cases.
* The libevent-removal `bitcoin-cli` HTTP client had four independent malformed
  response acceptance paths. Exact clean-master fake-server controls accepted
  each malformed response; the fixed branch rejects them while retaining valid
  response behavior. These are low-severity client parser issues because the
  endpoint already controls the RPC body and no node or consensus state is
  affected.
* Five additional clean-master controls found reusable-parser and local-data
  boundary defects: stale `ArgsManager` command state, out-of-range compressed
  coin amounts, invalid Schnorr keys reaching libsecp, exhausted coins-DB cursor
  reads, and HTTP request parser state being committed before a complete parse.
  The HTTP parser work contains two distinct contracts: incomplete chunked bodies
  must not be committed or duplicated on retry, and control/header/body fields
  must remain unchanged on failed or incomplete parsing.
* Five more exact-master controls reproduced production defects already fixed in
  this stack: duplicate serialized assumeutxo records, three failed-block
  candidate transitions, pruned unlinked children that lose their chain
  transaction count, sequential checkqueue work added after `Complete()`, and
  stale reusable `PrecomputedTransactionData` witness hashes. The first three
  are medium validation/snapshot consistency issues; the latter two are low to
  medium direct-API contracts. The fixed branch passes each adapted regression.
* The fee-estimator control is a separate direct-API boundary: populated estimator
  data made NaN and negative success thresholds return a nonzero estimate and
  overwrite the caller's result object on clean master. The fixed branch rejects
  every non-finite or out-of-range threshold and preserves the output sentinel.
* The HTTP stale-`m_send_ready` candidate was explicitly excluded from the defect
  count: current `origin/master` already contains the cleanup in `73da2a8a52`
  (`http: prevent race condition between worker thread and I/O thread`). Replaying
  that branch mutation would therefore test an already-fixed master path rather
  than reveal a current-master bug.
* The pruned-candidate redownload claim from `78dc30bc4d40` is explicitly excluded:
  transplanting its deterministic test onto exact current master passed without
  the branch fix. That failure was introduced by the local validation stack, so
  it is not evidence of a current-master defect.
* Sanitizer workers in this ledger are independent processes unless explicitly stated
  otherwise. TSan results are evidence for the exercised interleavings and setup,
  not a proof that all wallet or node state is generally thread-safe.

## Post-rebase compact, cluster, and cache controls (2026-07-24)

The branch is rebased onto the current `origin/master`
`610dd320d1a80838fdf30ed1cb2e6ae1ec717f74`; `git merge-base HEAD origin/master` matches that tip. The
current-master range after the earlier compact and
cluster/coins controls was checked directly. The relevant production and fuzzer
paths (`blockencodings`, `net_processing`, `txgraph`, `txmempool`, `coins`,
`validation`, `partially_downloaded_block`, `tx_pool`, and `coinscache_sim`) are
unchanged between the earlier control baseline and this master tip.

The latest two-commit range only removes a testnet3 seed, so the exact-master
compact and cluster controls below remain applicable to the current master source.

The compact-block collision result is unchanged: the short-ID accounting
underflow is fixed by master `6aa5d8d948`, the normal null-tail construction is
avoided by master `6f1c56f03a`, and the remaining null, duplicate, position-bound,
and reusable-`FillBlock()` cases are direct-API contracts or network fallback
coverage. No additional clean-master collision crash, state corruption, or race
was found.

For a fresh current-branch cluster-mempool load, two independent normal
`tx_pool` workers replayed the complete 8,000-file corpus and completed 8,001
executions each. They reached coverage `20232` and `20232`; no assertion,
timeout, sanitizer report, race/deadlock report, or target artifact was written.
Two independent TSan workers then replayed disjoint 512-file spread slices in
the direct-file driver. Both exited zero (`5s` and `4s`) without a race report or
artifact. These workers own separate mempools, so this is evidence for the
exercised callback/index/cluster interleavings, not arbitrary shared live-node
schedules.

Two full ASan/UBSan corpus attempts reached the 4,096-input pulse without a
diagnostic, but stalled in the known high-cost tail and were stopped after more
than seven minutes. They are explicitly incomplete and are not counted as
passes. A bounded spread slice provided the completed sanitizer control instead:
two independent ASan/UBSan workers ran 514 executions each, reaching coverage
`62742` and `66519` with peak RSS of about 500 MB and 479 MB. Neither emitted an
assertion, sanitizer, race, timeout, or target artifact.

The existing post-rebase `clusterlin_*` and `coinscache_sim` gates recorded below
remain applicable because the inspected master range does not touch those paths.
The descriptor-update and equal-feerate TxGraph controls are the additional
clean-master production defects found in this round; the compact-block controls
found no new collision defect, vulnerability, race, or deterministic test omission.

## Confirmed production defects

### 1. Index publication race and restart state

* Current branch fix: `91727fcd94` (`index: synchronize chainstate publication during restart`).
* Related startup fix: `1da8dfbf77` (`index: handle synced index before genesis activation`).
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

* Current branch fix: `dc5e395ad5` (`coins: serialize DB cursor lifetime with cache resize`).
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

* Current branch fix: `33bf715b20` (`p2p: bound outbound transport message types`).
* Severity on the clean baseline: low to medium memory-safety hardening. No
  unauthenticated remote path to construct the overlong internal message was proven.

`V2Transport::SetMessageToSend()` copied an overlong direct/internal message type into
the fixed-size transport field. A deterministic unit that bypassed the normal capped
message generators produced an ASan heap-buffer-overflow. The normal RPC and fuzz
paths already cap message types, so the finding is at the transport API boundary, not
an established network attack. The production boundary now rejects the message and
preserves it for the caller.

### 4. RBF fee-diagram total overflow

* Current branch fix: `b46ee18a1e` (`rbf: reject overflowing fee diagrams`).
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
current copy as `b46ee18a1e`. The behavior trade-off is therefore part of that one
fix, not a separate later vulnerability or fix: replacements involving extreme local
priority deltas can be temporarily non-replaceable. It is a policy trade-off, not a
consensus vulnerability.

### 5. Mempool info fee-delta signed overflow

* Current branch fix: `19bd264620` (`mempool: saturate TxMempoolInfo fee delta`).
* Severity on the clean baseline: medium for authenticated/local RPC robustness; no
  unauthenticated network trigger was demonstrated.

After adding a transaction with a positive base fee, two authenticated
`prioritisetransaction` calls could leave the modified fee at `INT64_MIN` while
`GetInfo()` evaluated `INT64_MIN - base_fee`. Clean-master ASan/UBSan aborted at the
`TxMempoolInfo` conversion. The fix uses a saturating subtraction and checks the
public view against that bounded result. The deterministic test reproduces the exact
two-step mutation, and the tx-pool sanitizer replay passed afterward.

### 6. Cache allocation percentage overflow

* Current branch fix: `a8ac0bf83f` (`node: avoid cache allocation percentage overflow`).
* Severity on the clean baseline: low local-configuration correctness issue.

On a 64-bit host, `-dbcache=8796093022208` makes the byte total `2^63`. The old
`total_cache * 10 / 100` calculation wrapped before applying the index cap, giving a
zero txindex allocation instead of the one-GiB cap. The boundary unit and
`cache_sizes` fuzzer catch the old arithmetic; this is not P2P/RPC input and does not
affect consensus or persisted chain data.

### 7. Coins-cache state capacity arithmetic overflow

* Current branch fix: `cache: harden cache size arithmetic`.
* Severity on the clean master: low local configuration/direct-API correctness issue;
  no remote, consensus, wallet-loss, or memory-safety path was demonstrated.

The recent master coins-cache headroom test (`b36c2d78a3`) made the unused-mempool
space path worth auditing. The clean-master `Chainstate::GetCoinsCacheSizeState()`
implementation mixed `size_t` cache capacities with signed `int64_t` intermediates.
The deterministic mutation passed `max_coins_cache_size_bytes = SIZE_MAX` and
`max_mempool_size_bytes = 0` to an empty cache on a 64-bit host. Exact master
`7b6f9ba7ba` returned `CRITICAL` instead of `OK` in both Debug and Release builds:
the unsigned sum became `SIZE_MAX` and was then converted to a negative signed
total. This is a direct API boundary and is not a remotely supplied cache size.

The fix keeps cache capacities and usage in `size_t`, computes unused mempool space
with a checked subtraction, combines budgets with `SaturatingAdd`, and rewrites the
90-percent threshold without multiplying the total by nine.
The deterministic `validation_flush_tests/extreme_cache_capacity_is_not_critical`
case reproduces the clean-master result in both Debug and Release. The exact master
controls changed only the temporary test and left production code untouched, so
this is not a regression introduced by this branch or by `b36c2d78a3`.

The new `coins_cache_size_state` fuzzer contains an independent size-typed oracle
and always exercises `SIZE_MAX`/zero, zero/`SIZE_MAX`, and `SIZE_MAX`/`SIZE_MAX`
capacity combinations before consuming arbitrary values. Four normal workers
executed 862,306 mutations, four ASan/UBSan workers executed 140,240, and four
TSan workers executed 154,643; all exited zero without an assertion, sanitizer, or
race report. The focused validation suite passed all three cases after the fix.

### 8. `-maxmempool` byte-size multiplication overflow

* Current branch fix: `cache: harden cache size arithmetic`.
* Severity on the clean master: low startup/configuration correctness issue; no
  remote, consensus, wallet-loss, or memory-safety path was demonstrated.

The clean-master mutation set `-maxmempool=9223372036855` MB, which is
`INT64_MAX / 1'000'000 + 1`. The raw MB-to-byte multiplication overflowed: the
Debug exact-master control built with `-ftrapv` trapped with an illegal operand,
while the Release control accepted the option and produced the invalid wrapped
result. Accepting this value leaves the mempool budget outside its representable
range and should instead return a configuration error before constructing the
mempool.

The fix uses `CheckedMul` for the MB-to-byte conversion and rejects the option on
overflow. The deterministic
`validation_flush_tests/maxmempool_byte_size_overflow_is_rejected` case reproduces
the failure against exact master and passes after the fix. The fuzzer target above
is intentionally focused on cache-state arithmetic; the option parser has a direct
deterministic boundary test because invoking startup argument parsing per fuzz
iteration would add setup cost without improving this contract's signal.

### 9. Base58 decoder stale output on failure

* Base58Check fix: `0d191905b4`.
* Raw Base58 fix: `6bb8b66b34`.
* Severity on the clean baseline: low parser API correctness issue.

The public wrappers could return `false` while leaving caller-owned bytes from a
previous successful decode, including the embedded-NUL early return and max-length
failure paths. No current caller was shown to treat stale bytes as a successful
decode, but the boundary was ambiguous. Both wrappers now clear output on failure,
have production postconditions, deterministic tests, and corpus-backed normal and
ASan/UBSan fuzz coverage.

### 10. Descriptor-cache merge leaves partial state after a conflict

* Current branch fix: `4da71c90d7` (`descriptor: keep cache conflict merges atomic`).
* Severity on the exact clean-master control baseline
  `7b6f9ba7bad13b0c4169259000f7802854cdda0d` (the then-current master):
  medium local wallet consistency/availability issue. The exact production control ran
  at `b8844d3df759bfa070681327583427461d39105c`; subsequent master deltas, including
  PR #34672, do not change descriptor-cache production code or its tests. The trigger
  requires a conflicting descriptor cache, so no network or unauthenticated RPC attack
  was demonstrated; wallet-key loss and persisted database corruption were not
  demonstrated.

`DescriptorCache::MergeAndDiff()` on clean master merged the incoming parent-xpub
map before checking the incoming derived-xpub map. If the destination already held a
derived xpub at `(key expansion position, derivation index)` and the incoming cache
contained a new parent followed by a different derived xpub at that existing key,
the method inserted the parent and then threw the derived-conflict exception. The
wallet caller writes the returned diff only after `MergeAndDiff()` succeeds, so the
exception can leave the in-memory descriptor cache ahead of the database. A retry or
later wallet operation can then observe a cache/database mismatch until restart.

The branch fuzzer mutation and `descriptor_cache_merge_conflict_is_atomic` unit test
capture all three cache maps before the call and require the expected
`std::runtime_error` plus exact map equality afterward. For a clean-master proof, an
isolated worktree at the exact baseline kept the production implementation unchanged
and received only the branch fuzzer oracle as a temporary test fixture. Its
deterministic 88-byte input (SHA256
`92dd2f1c6e3c00eb0bb93f156802a1da4155db57476bf4459e4cc1fede715f65`) failed after
one execution with the parent-map assertion. The same input exited zero under the
branch ASan/UBSan fuzzer, and the rebuilt Debug unit test passed. The production
preflight now validates all conflict maps before mutating the destination. The
commit message records the intentional preflight-removal mutation used to prove the
unit test catches the old behavior.

### Additional conflict-ordering coverage (2026-07-24)

The fuzzer and deterministic tests now cover the three orderings that the original
derived-conflict construction did not reach:

* A destination parent at position 1, followed by an incoming new parent at position
  0 and a conflicting parent at position 1. This catches partial insertion within
  the parent map.
* A destination derived xpub at `(1, 1)`, followed by incoming derived index 0 and a
  conflicting index 1. This catches partial insertion within one derived-index map.
* A destination last-hardened xpub at position 1, followed by an incoming parent at
  position 0 and a conflicting last-hardened xpub at position 1. This catches partial
  insertion across the parent and last-hardened maps.

The helper catches only the expected `std::runtime_error` text and then requires all
three destination maps to equal their pre-call snapshots; other exceptions and
assertions remain fatal. From 440 of 506 QA inputs below 64 KiB (maximum 64,311
bytes), the edited branch fuzzer completed two normal 5,000-run workers at coverage
875, two ASan/UBSan workers at coverage 3,118, and two TSan workers at coverage 441.
All six workers exited zero without an assertion, sanitizer report, race/deadlock
report, timeout, or artifact. The deterministic descriptor suite passed all ten
cases, including the three new tests.

For the historical clean-master control, an exact
`5311b15727f2f282274472184185423e441abd85` worktree received only the edited fuzzer;
`src/script/descriptor.cpp` and the unit tests remained clean-master versions.
Replaying the 506 preserved QA seeds aborted
after 12 inputs when the last-hardened construction observed that clean master had
inserted the unrelated parent before throwing the conflict. The assertion was at
the oracle's parent-map postcondition, and the artifact SHA256 is
`0bf90c4d2f1eb93bf54dbf3a9605d229dec719f041d3385387c723857757c422`. Replaying that
exact artifact exited zero under the current normal, ASan/UBSan, and TSan branch
binaries. This confirms the three new mutations are additional reproductions of the
existing partial-merge defect, not a branch-introduced failure. No new production
fix or severity change is warranted because `4da71c90d7` preflights all three maps;
the new unit tests make each ordering deterministic.

### 11. Descriptor address index published after persistence failure

* Current branch fix: `e9d748a0c1` (`wallet: persist descriptor index before returning address`).
* Severity on clean master: low-to-medium local wallet consistency/availability issue;
  it requires a wallet database write failure and is not a consensus, key-loss, or
  unauthenticated remote vulnerability.

`DescriptorScriptPubKeyMan::GetNewDestination()` derived a valid destination, then
incremented `m_wallet_descriptor.next_index`, ignored the `false` result from
`WalletBatch::WriteDescriptor()`, and returned the destination. A database or
filesystem failure could therefore leave the caller with an address that was not
durable while the live descriptor manager had already advanced. After a restart the
old persisted index could be loaded and the same address generated again. The
failure is local and storage-dependent; it does not expose keys or alter consensus,
but it violates the wallet's persistence contract and can make address allocation
non-deterministic across restart.

The deterministic mutation set `m_keypool_size` to 1 and installed a SQLite
`BEFORE INSERT` trigger on the mock wallet database that raised `ABORT`. The exact
unmodified `origin/master` checkout at `7b6f9ba7ba` failed the focused test with
both postconditions: it returned a destination and reported `next_index == 1`
instead of `0` (`CONTROL_EXIT=201`). The pre-fix branch function was byte-for-byte
identical to this clean-master implementation, ruling out a regression introduced
by the local stack. The fixed implementation writes a copy of the descriptor with
the incremented index, returns `Error: Failed to write descriptor` when the write
fails, and updates the in-memory index only after a successful write.

The unit suite also covers the separate hardened-cache path: after one successful
address, the same SQLite mutation makes `TopUpWithDB` throw the expected
`std::runtime_error` with reason `TopUpWithDB: writing cache items failed`. The
scriptpubkeyman fuzzer can inject this mutation once per input and stops before
later actions that correctly encounter the failed database. It accepts only that
exact `std::runtime_error`; all other exception types and messages are rethrown.
Two independent normal workers completed 10,026 executions each, reaching coverage
12,943 with peak RSS of 575 MB. Two ASan/UBSan workers completed 10,028 executions
each, reaching coverage 27,481 and 27,487 with peak RSS of 996 and 1,014 MB. Two
TSan workers completed 10,026 executions each, reaching coverage 12,952 and 12,955
with peak RSS of 578 MB. All six workers exited without an assertion, sanitizer
report, unexpected exception, race/deadlock report, timeout, or artifact. The
focused four-case descriptor suite passed after the fix.

### 12. Mining `submitSolution` leaves stale failure outputs

* Current branch fix: this feature commit titled `mining: clear submitSolution failure outputs`.
* Severity on clean master: low API correctness issue for direct mining-interface
  callers and IPC clients; no consensus, node crash, or remote attack was
  demonstrated.

PR #34672 added `reason` and `debug` output parameters to `BlockTemplate::submitSolution`
and unified successful submissions with `SubmitBlock()`. The null-coinbase guard in
`BlockTemplateImpl::submitSolution()` returned `false` before reaching `SubmitBlock()`,
whose first operation clears both output strings. A caller that reused those strings
could therefore receive a failed submission result with stale text from an earlier
call. The interface documentation describes both parameters as output fields, and
the other `submitSolution` paths already clear them through `SubmitBlock()`.

An exact clean-master `7b6f9ba7bad13b0c4169259000f7802854cdda0d` worktree received
only a temporary `miner_tests/CreateNewBlock_validity` assertion. It initialized both
outputs to `"stale reason"` and `"stale debug"`, passed a null `CTransactionRef`, and
failed at the reason postcondition (`stale reason != `). The production implementation
and existing tests were otherwise unchanged. The branch fix clears both outputs on
that early return and asserts the empty postcondition; the same Debug test passes.
The IPC functional test now checks the serialized `result`, `reason`, and `debug`
fields for an empty coinbase. After installing Python `pycapnp` 2.2.4 under the
temporary storage area, `interface_ipc_mining.py` passed end to end in 24 seconds.
The clean-master C++ failure, fixed-branch unit pass, and IPC functional pass
therefore all cover this mutation.

### 13. `TxIndex::FindTx` leaves stale failure outputs

* Current branch fix: `txindex: clear failed FindTx outputs`.
* Severity on clean master: low direct-API correctness issue; current callers pass
  fresh locals, so no existing user-visible stale-data path or remote trigger was
  demonstrated.

`TxIndex::FindTx()` documents `block_hash` and `tx` as output parameters, but every
false return on clean master left caller-provided values unchanged. A deterministic
`txindex_tests/txindex_initial_sync` mutation initialized `tx` to a known coinbase
transaction and `block_hash` to `uint256::ONE`, then looked up the absent txid
`Txid::FromUint256(uint256::ZERO)`. The exact clean-master `7b6f9ba7ba` control
failed at `src/test/txindex_tests.cpp:29-30`: both `!tx_disk` and
`block_hash.IsNull()` were false. This is an API contract defect found during the
output-parameter audit prompted by the fuzzer stale-output findings, not a
sanitizer crash or a new fuzzer-only oracle.

The branch clears both outputs on entry and routes the database-miss, file-open,
deserialization, and txid-mismatch exits through a helper that asserts both outputs
remain empty. The same deterministic unit test passes with 116/116 assertions, and
the full three-case `txindex_tests` suite passes with 129/129 assertions. No caller
currently relies on stale values after a failed lookup; the fix makes the documented
failure contract explicit for direct and future callers.

### 14. `Wallet::getWalletTxDetails` leaves missing-transaction outputs stale

* Current branch fix: `wallet: clear missing getWalletTxDetails outputs`.
* Severity on clean master: low local GUI/API correctness issue with undefined
  status reads in the Qt caller; no remote, consensus, key-loss, or wallet-file
  corruption path was demonstrated.

`Wallet::getWalletTxDetails()` returns an empty `WalletTx` when the requested
transaction is absent, but clean master left `tx_status`, `messages`,
`payment_requests`, `in_mempool`, and `num_blocks` untouched. Unlike an API that
returns a boolean, the Qt transaction-description caller uses the returned
values immediately. Its `WalletTxStatus status`, `bool inMempool`, and `int
numBlocks` locals are default-initialized and therefore indeterminate when a
model transaction disappears before the details lookup; reused vector or scalar
outputs can also display stale data.

The deterministic control initialized every output to a non-default value,
looked up absent `Txid::FromUint256(uint256::ONE)`, and required an empty result
plus cleared outputs. In an exact clean-master `7b6f9ba7bad13b0c4169259000f7802854cdda0d`
Debug worktree, `wallet_tests/wallet_interface_missing_tx_outputs` failed 13 of
15 postconditions at `src/wallet/test/wallet_tests.cpp:74-86`; only the empty
`WalletTx` checks passed. Existing wallet tests used default-initialized outputs,
so they did not catch the contract omission.

The fix resets every output before the lookup and adds production `Assert`
postconditions on the missing-transaction path. The deterministic regression
test passes 15/15 assertions, and the full `wallet_tests` suite passes 15 cases
and 145/145 assertions in the Debug wallet build. This is a direct interface
contract fix, not evidence of a remotely reachable race: the wallet lock
serializes the lookup, while the stale model entry is the normal local trigger.

### 15. `ProcessNewBlock` reports a new block after a block-file write failure

* Current branch fix: `validation: clear new-block output on block write failure`.
* Severity on clean master: low local availability/bookkeeping issue. The trigger
  requires a block-file I/O failure, which is normally a local disk or filesystem
  problem; no consensus, memory-safety, wallet-loss, or remote-only attack was
  demonstrated.

`ChainstateManager::AcceptBlock()` set `*fNewBlock = true` before calling
`BlockManager::WriteBlock()` and `ReceivedBlockTransactions()`. If the block-file
write failed, `AcceptBlock()` returned `false` through its fatal I/O path, but the
caller-owned flag remained true even though the block index did not have
`BLOCK_HAVE_DATA`. `PeerManagerImpl::ProcessBlock()` ignores the boolean return
from `ProcessNewBlock()` and uses this flag to erase the in-flight request and
update the peer's last-block timestamp, so a failed write was reported as if the
block had been stored. A node with a failing filesystem is already degraded and
the fatal notification requests shutdown; this is therefore a consistency and
recovery issue, not a remotely exploitable block-validation bug.

The deterministic mutation renames the configured blocks directory after setup,
replaces it with a regular file, and submits a valid regtest block. The next
`FlatFileSeq` directory creation fails with `Not a directory`; the test keeps fatal
shutdown disabled long enough to require `ProcessNewBlock() == false`,
`new_block == false`, and cleanup of the original directory. The exact clean-master
`7b6f9ba7bad13b0c4169259000f7802854cdda0d` control received only this test and
failed at the `!new_block` postcondition (exit 201). The branch failed the same
way before the production edit and passes afterward. Existing block/reorg tests
covered invalid, duplicate, and successful storage paths, but
`TestChainstateManager::DisableNextWrite()` controls chainstate flushing rather
than `BlockManager::WriteBlock()`, so they did not reach this disk-failure branch.

The fix assigns `fNewBlock` only after both block storage and
`ReceivedBlockTransactions()` succeed. The focused Debug test passes on the fixed
branch; the existing `ProcessNewBlock` success/failure output assertions continue
to cover the ordinary paths. No fuzzer-only exception or race was involved, so no
additional fuzzer mutation was needed beyond the deterministic filesystem fault.

### 16. External signer omits signed-input count

* Current branch fix: `wallet: report inputs signed by external signer`.
* Severity on clean master: low direct-API and GUI correctness issue. The trigger
  requires a local external signer and a partial PSBT; it does not prevent signing,
  alter the transaction, expose keys, or provide a remote attack path.

`CWallet::FillPSBT()` documents `n_signed` as the number of inputs signed by the
wallet, and `PSBTOperationsDialog::signTransaction()` uses it to distinguish a
successful partial signing from the case where no more inputs could be signed.
`ExternalSignerScriptPubKeyMan::FillPSBT()` delegated the non-signing path to the
normal descriptor manager, but the signing path replaced the PSBT with the external
signer's response and never initialized or updated `n_signed`. A signer that
finalized one input in a still-incomplete PSBT therefore returned `n_signed == 0`.

The deterministic mutation uses a two-input PSBT with valid `witness_utxo` entries
whose scriptPubKeys are `OP_TRUE`. Input 0 carries master fingerprint `00000001`; the test-binary mock
external signer returns the same PSBT with a non-empty `final_script_sig` on input 0
and input 1 still unsigned. On clean master, `FillPSBT(..., &n_signed)` leaves the
count at zero even though `PSBTInputSigned(input 0)` is true. The focused test also
checks that the returned PSBT remains partial, so the stale count is not being hidden
by an already-complete transaction.

The exact `origin/master` `7b6f9ba7ba` Debug/Clang 19 control received only the
temporary mock subprocess branch in `src/test/system_tests.cpp` and the deterministic
unit test; `src/wallet/external_signer_scriptpubkeyman.cpp` remained unmodified. It
failed at `psbt_wallet_tests.cpp:101` with `0 != 1` and exited 201. The fixed branch
passes the same focused test, the full three-case `psbt_wallet_tests` suite, and the
existing `system_tests` suite. The production change initializes the output, records
which inputs were already finalized, and counts only unsigned-to-finalized
transitions after the signer and optional `FinalizePSBT()` call.

The existing `scriptpubkeyman` fuzzer did not discover this because it creates normal
descriptor managers and never invokes an external signer subprocess. Adding a child
process to every fuzz iteration would not improve the contract signal; the
deterministic subprocess mock is the stronger regression test for this boundary.

### 17. Private-broadcast removal counter drift

* Current branch fix: `800a2aad69` (`privatebroadcast: track disconnects for removal accounting`).
* Severity on clean master: medium within the optional private-broadcast feature;
  an availability/privacy degradation requiring a local private-broadcast request,
  selected peer timing, and an unconfirmed connection. No consensus, funds, wallet
  persistence, or unauthenticated memory-safety impact was demonstrated.

Clean master increments the global private-broadcast connection budget by three for
each new transaction. `PickTxForSend()` records a picked send, which means that
connection slot has already been consumed. The removal paths in
`PeerManagerImpl::AbortPrivateBroadcast()` and inbound transaction handling then
computed `3 - num_confirmed` and subtracted that many slots. With two picked sends,
one confirmed send, and one still-open unconfirmed send, the old code cancelled two
slots even though only one attempt was still unstarted. If the selected peer holds
the unconfirmed connection open, removal of that transaction can make the global
counter disagree with the actual open/unstarted work and delay fanout for another
private transaction. This is a feature-scoped liveness and privacy degradation,
not a remote crash: the attacker must be selected for a private-broadcast
connection and coordinate the transaction-removal timing.

The exact `origin/master` `7b6f9ba7ba` control added only
`clean_master_remove_cancellation_overcounts_unstarted_slots` to the temporary
`private_broadcast_tests.cpp`; all production sources remained unchanged. It added
one transaction, picked it for nodes 1 and 2, confirmed node 1, and compared the
clean-master `net_processing.cpp` formula with the number of unstarted attempts.
The unmodified master failed deterministically with `[2 != 1]` (exit 201). The
fixed branch's `disconnected_send_statuses_are_counted_for_removal` test checks the
same state using `RemoveResult::NumUnstarted(3)`, and all nine private-broadcast
unit tests pass after the fix.

The production fix returns picked, confirmed, and unconfirmed-disconnected counts,
uses `NumUnstarted()` in both removal paths, and keeps active node IDs as tombstones
until finalization so a late disconnect cannot allocate capacity to another
transaction. The clean-master control did not rely on an assertion introduced by
this branch; it compared the existing production cancellation calculation with the
state established by the existing `PrivateBroadcast` API.

### 18. Private-broadcast late confirmation after disconnect

* Current branch fix: `449636bbe3` (`privatebroadcast: ignore confirmations after disconnect`).
* Severity on clean master: low, conditional retry/staleness accounting issue in
  the private-broadcast state machine. It requires a queued PONG to be processed
  after the disconnect finalization check; no consensus, funds, wallet persistence,
  remote crash, or standalone TSan race was demonstrated.

On clean master, `FinalizeNode()` checks
`!DidNodeConfirmReception(nodeid) && HavePendingTransactions()` and schedules a
replacement for an unconfirmed private connection, but it does not mark that send
as disconnected. A matching PONG processed after that check is still accepted by
the unconditional `NodeConfirmedReception()`. The send then appears confirmed even
though the replacement decision has already been made; subsequent removal can
report the wrong confirmation count, and `GetStale()` can use the confirmed clock
instead of the original unconfirmed-send clock. The practical effect is a retry
budget or rebroadcast timing inconsistency, not a consensus or memory-safety bug.

The exact `origin/master` `7b6f9ba7ba` control added only
`clean_master_late_confirmation_changes_disconnected_send` to the temporary unit
file. It modeled the existing `FinalizeNode()` condition after one unconfirmed
send, advanced the fake clock, and then delivered the late confirmation through
the existing production API. With production code untouched, master failed
deterministically because `DidNodeConfirmReception()` became true. The fixed branch
marks the send disconnected in `MarkNodeDisconnected()` and ignores later
confirmations; `confirmation_after_disconnect_is_ignored` checks that the send
remains unconfirmed, remains disconnected for removal accounting, and retains the
full unstarted retry count. The fixed nine-case private-broadcast suite passes.

This is a state-ordering control, not a claim that every live peer teardown schedule
has been reproduced by TSan. The clean-master result proves that the production
state object accepts the invalid late transition; the residual reachability depends
on the asynchronous message and connection-finalization ordering.

### 19. Nullable coin control dereference for unconfirmed wallet outputs

* Current branch fix: `dfe7f9348f` (`wallet: handle unconfirmed coins without coin control`).
* Severity on clean master: low to medium local wallet availability/API correctness
  issue. A normal wallet caller can request unconfirmed coin selection without a
  `CCoinControl`; no key loss, consensus, funds, or remote-node attack was
  demonstrated.

`AvailableCoins()` explicitly accepts a null `CCoinControl`. On clean master, an
  unconfirmed wallet-owned output entered the `nDepth == 0` and
  `check_version_trucness` path in `src/wallet/spend.cpp`, which unconditionally
  read `coinControl->m_version`. The result was a null-pointer dereference before
  ordinary coin selection could return the output. Existing spend tests used only
  confirmed outputs and therefore never entered this branch.

The deterministic mutation creates a wallet-owned unconfirmed parent with two
outputs and calls `AvailableCoins(..., nullptr, ...)`. The exact
`origin/master` `7b6f9ba7ba` control received only that test; the production source
remained unchanged and the focused test terminated with SIGSEGV (exit 139) at the
null `coinControl` read. The fixed branch guards the TRUC version read with the
nullable pointer check and passes all four `wallet::spend_tests` cases, including
the unconfirmed-parent regression. The same commit extends `wallet_create_transaction`
to exercise shared ancestry and both outputs; its mempool reset and BTC-formatted
dust-fee setup are fuzzer-harness corrections, not additional production findings.

### 20-23. Malformed `bitcoin-cli` HTTP response acceptance

* Current branch fixes: `e516ea1d3d` (overlong status codes), `b500870cd3`
  (status codes below 100), `cb37575c97` (ambiguous duplicate content lengths),
  and `2e03ca53b1` (malformed chunked replies).
* Severity on clean master: low client-side parser robustness. A malicious or
  malformed RPC HTTP endpoint can choose these response fields, but the endpoint
  already controls the JSON-RPC body; no consensus, P2P, node, wallet-loss,
  persisted-state, or authority-escalation impact was demonstrated.

The simple Sock HTTP client introduced by the libevent-removal work had four
independent response-validation omissions:

* **20, overlong status code:** `HTTP/1.1 2000 Invalid` was parsed as status 200
  because the parser consumed only the first three digits and did not require a
  space afterward. The fixed parser rejects it as an invalid status-line format.
* **21, status below 100:** `HTTP/1.1 099 Invalid` was accepted as a successful
  response because the old parser checked only that the three characters were
  numeric and later treated every status below 400 as non-error. The fixed parser
  rejects status codes below 100.
* **22, differing duplicate `Content-Length`:** the old parser used only the first
  header, so `Content-Length: N` followed by `Content-Length: N+1` was accepted
  when the body had N bytes. The fixed parser requires all duplicate values to be
  identical while retaining the valid identical-duplicate case.
* **23, invalid chunk terminator:** after a nonzero chunk payload, the old parser
  discarded the next two bytes without checking for CRLF. It therefore accepted
  `XX` as a chunk terminator; the fixed parser rejects any terminator other than
  CRLF.

For clean-master proof, a temporary worktree at exact `origin/master` received
only the functional test additions; `src/bitcoin-cli.cpp` remained unchanged.
The exact Debug/Clang 19 `bitcoin-cli` and `bitcoind` accepted all four malformed
responses: the focused functional methods failed with `AssertionError: No
exception raised`, and a direct fake-server probe returned exit 0 and `ok` for
both `099` and `2000`. On the fixed branch, the chunked, duplicate-length, and
status focused methods each exited 0, covering both valid controls and all four
rejections. These tests remain functional rather than fuzzer tests because the
affected parser is private to the `bitcoin-cli` executable and is not linked into
an existing fuzz target; the fake-server mutation is the stronger deterministic
regression for this boundary.

### 24. Reusable `ArgsManager` retains command and error state

* Current branch fix: `6115b874ef` (`args: reset command parse state`).
* Severity on clean master: low reusable-API correctness. Normal `bitcoind` and
  `bitcoin-cli` startup parse arguments once on a fresh process-global object, so
  no normal startup, RPC, P2P, wallet, consensus, or persisted-state attack was
  demonstrated.

`ArgsManager::ParseParameters()` cleared command-line options but retained the
previous command, command arguments, and caller-provided error text. A successful
parse after a failed parse could therefore report stale failure state. `ClearArgs()`
also left the old command and command-mode latch active after registered commands
were removed, so a reused parser rejected free commands unexpectedly.

The exact `origin/master` control registered two commands, performed successful and
failed parses in sequence, then called `ClearArgs()` and parsed again. With
`src/common/args.cpp` unchanged, `argsman_tests/util_ParseParameters_replaces_command_state`
exited 201: 10 of 27 postconditions failed. The fixed branch passes the same case
with 29 assertions. The production fix clears parsed command/error state at parse
entry and resets command mode in `ClearArgs()`; the added system-fuzzer oracle
checks the same replacement sequence.

### 25. Compressed coin amounts accepted outside `MoneyRange()`

* Current branch fix: `970e8c997d` (`compressor: reject out-of-range compressed amounts`).
* Severity on clean master: medium local/persistent-data robustness. The affected
  decoder feeds coins, undo data, and assumeutxo-style serialized data; malformed
  local data or a malicious snapshot can otherwise produce an out-of-range
  `CAmount` before downstream validation. No P2P transaction or consensus wire
  path was demonstrated.

`AmountCompression::Unser()` decompressed arbitrary varints and assigned the result
  to a `CAmount` without enforcing the range precondition used by
  `CompressAmount()`. The exact clean-master control accepted both
  `CompressAmount(MAX_MONEY + 1)` and `uint64_t::max()` encodings: the focused
  `compress_tests/compress_amount_deserialize_range` case passed only its one valid
  assertion and failed its two expected exceptions (exit 201). The fixed branch
  passes all three assertions and rejects the invalid values at the shared
  deserialization boundary.

### 26. Invalid Schnorr key reaches libsecp256k1

* Current branch fix: `4639b59839` (`key: reject invalid Schnorr signing keys`).
* Severity on clean master: low to medium local wallet/key API robustness. The
  input requires a caller to construct an invalid `CKey`; no untrusted network path
  to create that key was demonstrated.

Schnorr signing did not apply the invalid-key guard used by the ECDSA path. A
  default-invalid `CKey` reached secp256k1 keypair creation, which invoked its
  illegal-argument callback instead of returning false and preserving the output
  buffer. The exact clean-master `key_tests/bip340_test_vectors` control aborted
  with `seckey32 != NULL` (exit 134). The fixed branch passes the same case with
  246 assertions and keeps the signature buffer unchanged on failure.

### 27. Coins DB cursor value read after exhaustion

* Current branch fix: `255555316f` (`coins: guard exhausted DB cursor values`).
* Severity on clean master: low to medium local coins-database API robustness. The
  direct cursor boundary is used by coins/snapshot inspection code; no remote,
  consensus, or ordinary P2P trigger was demonstrated.

`CCoinsViewDBCursor::GetKey()` treated an exhausted cursor as a miss, but
`GetValue()` unconditionally called LevelDB's iterator value accessor. The exact
  clean-master control walked a persistent cursor to exhaustion and then queried
  both outputs; `coins_tests/coins_db_cursor_exhaustion_outputs` aborted in
  LevelDB's `DBIter::value()` assertion (exit 134). The fixed branch's
  `coins_db_leveldb_layout` regression passes 28 assertions and leaves both
  caller-provided outputs unchanged on the exhausted path.

### 28. Incomplete chunked HTTP body is committed and duplicated

* Current branch fix: `d136d1bc34` (`http: commit chunked request bodies atomically`).
* Severity on clean master: low to medium malformed HTTP/RPC request robustness.
  An unauthenticated peer can choose chunk boundaries and connection timing, but
  the demonstrated effect is request-state corruption/connection handling, not
  memory safety, authentication bypass, consensus, wallet, or persisted-state
  corruption.

`HTTPRequest::LoadBody()` appended each decoded chunk directly to `m_body` before
the terminating chunk and trailers had arrived. On a retry with more bytes, the
same request body was appended again. The exact clean-master
`httpserver_tests/http_request_tests` control failed exactly two new assertions:
the body was visible after the incomplete parse and duplicated after the retry
(exit 201). The fixed implementation builds a local body and publishes it only
after the complete chunked message is valid; the fixed HTTP request suite passes
115 assertions.

### 29. HTTP request fields are published before parsing succeeds

* Current branch fix: `ee190cc079` (`http: parse request fields atomically`).
* Severity on clean master: low to medium malformed HTTP/RPC parser robustness.
  The parser is reachable from unauthenticated HTTP input, but the clean-master
  control demonstrated stale/partial request state rather than a remote memory
  safety or authentication failure.

On clean master, a bad request version left the parsed method and target visible, a
bad or incomplete header section published partial headers, and a no-body or short
body parse retained or appended stale body data. The exact control initialized
sentinel fields and ran `http_request_parse_failure_preserves_fields` with
`src/httpserver.cpp` unchanged; six of 37 postconditions failed (exit 201). The
fixed parser parses control fields, headers, and bodies in temporaries and commits
them only after success; the fixed branch passes all 37 assertions.

The fixed-branch fuzzer gates for these contracts were clean on 2026-07-23. Two
normal workers completed 14,475/14,623 `system`, 543,224/552,240
`txoutcompressor_deserialize`, 48,424/49,299 `key`, 4,897/4,897
`coins_view_db`, and 62,399/61,305 `http_request` executions. ASan/UBSan
replayed 1,598, 218, 2,161, 4,899, and 11,788 inputs respectively, with no
diagnostic or artifact. Two TSan workers completed 4,897 coins-DB inputs each
and 53,363/52,218 HTTP inputs, also without a report or artifact. These sanitizer
and TSan results validate the fixed branch's exercised paths; they do not upgrade
the local/API findings above into claims of general thread safety.

### 30. Checkqueue work added after explicit completion

* Current branch fix: `7e06037e8239` (`checkqueue: complete work added after control completion`).
* Severity on clean master: low to medium internal validation-work scheduling
  correctness. The demonstrated transition is sequential and requires a caller to
  reuse one `CCheckQueueControl` after calling `Complete()`; no remote trigger or
  consensus bypass was demonstrated.

`CCheckQueueControl::Complete()` marked the controller done, but a later non-empty
`Add()` left that flag set. The controller destructor then skipped its final
completion, so work added after the explicit completion could remain queued after
the controller scope ended. The deterministic mutation adds one check, completes
the control, adds two more checks, and observes the callback count after scope exit.

The exact `origin/master` control changed only the current-context unit test and
left `src/checkqueue.h` unchanged. It failed with `n_calls == 1` instead of `3`.
The fixed branch passes the same test and the full `checkqueue_tests` suite. The
fix clears the completed flag before accepting a non-empty post-complete batch;
empty batches and concurrent method calls retain their existing semantics.

### 31. Reused sighash precomputation retains stale witness hashes

* Current branch fix: `949c3916cc35` (`script: reset precomputed txdata before init`).
* Severity on clean master: low to medium direct script-signing API correctness.
  A caller must reuse one `PrecomputedTransactionData` object across incompatible
  transactions and precompute modes; no ordinary validation path or remote trigger
  was demonstrated. The stale cache can produce an incorrect witness-v0 signature
  hash for the second transaction.

The old `Init()` path left `m_bip143_segwit_ready` set when the first transaction
was force-initialized with empty spent outputs and the second transaction did not
request BIP143 precomputation. `SignatureHash()` then consumed hashes belonging to
the first transaction. The exact clean-master test first initialized `stale_tx`,
reinitialized the same object for `tx`, and compared four witness-v0 hash modes
with and without the cache. Master failed the readiness postcondition and all four
hash comparisons. The fixed branch passes the deterministic test and the sighash
suite; `Init()` now resets optional readiness before recomputation.

### 32. Duplicate serialized assumeutxo coin records are accepted

* Current branch fix: `e2b773c53d3f` (`validation: reject duplicate snapshot coin records`).
* Severity on clean master: medium local snapshot-integrity issue. A corrupt or
  malicious snapshot file can append a duplicate serialized UTXO record, inflate
  the metadata coin count, and still produce the same unique UTXO set and serialized
  UTXO hash because the cache ignores the duplicate. This is a local snapshot
  loading boundary, not a P2P or consensus-validation bypass.

The exact-master regression generated a valid snapshot, copied its first serialized
txid group to the end, incremented `m_coins_count`, and loaded it through
`ActivateSnapshot()`. With production `src/validation.cpp` unchanged, master
accepted the malformed snapshot, so the test's `!CreateAndActivate...` check
failed. The fixed branch rejects it by comparing recomputed unique coin statistics
with the metadata count and passes `chainstatemanager_activate_snapshot`. The same
mutation is now present in the snapshot fuzzers and the assumeutxo functional
coverage.

### 33. Failed blocks remain in validation candidate sets

* Current branch fix: `ee0db855a418` (`validation: keep failed blocks out of candidates`).
* Severity on clean master: medium validation block-index consistency and
  availability issue. The transitions are reachable through invalid block
  reception, reorg/invalidation, and candidate recovery; no consensus bypass,
  wallet impact, or persisted-format corruption was demonstrated.

Three deterministic clean-master controls exposed the same invariant through
different state transitions. Receiving an unlinked child, invalidating it, and
then receiving its parent left the failed child in `setBlockIndexCandidates`.
Invalidating an out-of-chain candidate parent left its failed child there as well.
Finally, invalidating an active-chain block while a higher-work side descendant was
cached left that descendant as a failed candidate during disconnect. Each exact
master control failed with candidate membership `1` where `0` was required. The
fixed branch passes all three tests, removes failed blocks from every chainstate
candidate set, skips failed recovery entries, and avoids reinserting invalidated
unlinked descendants while preserving their chain transaction counts.

### 34. Pruning an unlinked child loses its chain transaction count

* Current branch fix: `43f0581be847` (`validation: keep pruned unlinked blocks linkable`).
* Severity on clean master: medium pruned-node validation/index consistency issue.
  A node can receive a child before its parent, prune the child's block data, and
  later receive the parent. No consensus bypass or persisted-format corruption was
  demonstrated, but the resulting block-index state aborts the assertion-enabled
  consistency check and can strand the child from chain-transaction accounting.

The exact-master regression received an off-chain child, pruned its block file,
verified that the child remained processed but lacked chain transaction counts, and
then received the parent. Master had already erased the child from
`m_blocks_unlinked`; the child stayed at `m_chain_tx_count == 0`, and
`CheckBlockIndex()` aborted after the parent arrived. The same result occurred in
the exact Debug ASan/UBSan control without a memory-safety report. The fixed branch
keeps pruned entries that still need ancestor chain counts in `m_blocks_unlinked`
and passes the deterministic regression, `blockmanager_tests`, and the block-index
fuzzer gate.

### 35. Invalid raw fee-estimator thresholds fail open

* Current branch fix: `f7f0ba94a24d` (`fees: reject invalid raw fee thresholds`).
* Severity on clean master: low to medium direct fee-estimator API robustness.
  RPC validation already rejects these values, but direct estimator callers can
  pass NaN, infinities, or values outside `[0, 1]`. No wallet-loss, consensus, P2P,
  or persisted-state attack was demonstrated.

`CBlockPolicyEstimator::estimateRawFee()` rejected only thresholds greater than
one. With populated estimator buckets, a NaN or negative threshold made the
success comparisons fail open and produced a nonzero estimate while also replacing
the caller's `EstimationResult` sentinel. The exact current-master control at
`22a03ca694` wrote a sane estimator file, then exercised `-inf`, NaN, `-1`,
`1 + epsilon`, and `+inf`; 15 postconditions failed, including the estimate and
the four sentinel fields for the three fail-open values.

The fixed branch passes the deterministic unit regression. Its `policy_estimator`
fuzzer now injects the same invalid values and checks both the zero-fee result and
output preservation. Two normal workers completed 2,000 executions each from a
private copy of the 1,254-input QA corpus; two ASan/UBSan workers completed 1,290
replays each after corpus growth. All exited successfully without a diagnostic or
artifact.

### 36. Descriptor top-up leaves cache state inconsistent after metadata write failure

* Current branch fix: `e8f820f41d` (`wallet: make descriptor topups consistent on write failure`).
* Severity on clean master: low to medium local wallet-storage consistency and
  recovery issue. The trigger requires a descriptor metadata row write to fail
  after cache-row writes succeed; no remote, consensus, key-loss, or normal
  storage-failure exploit was demonstrated.

`DescriptorScriptPubKeyMan::TopUpWithDB()` wrote new descriptor cache rows and
mutated the in-memory descriptor, script maps, and maximum cached index before
ignoring the result of `WriteDescriptor()`. The exact `origin/master`
`22a03ca69443deb66b98deb37d8e84a33968ac10` control installed a SQLite trigger that
rejected only the 49-byte descriptor metadata key while allowing the 62-byte
descriptor-cache keys, then called `TopUp(2)` on a valid ranged `wpkh` descriptor.
Clean master returned success, advanced the in-memory range to 2, persisted range
end 1, and left two cache rows where one existed before. The deterministic unit
test reproduces the mutation and fails on the old code.

The same metadata-write mutation was transplanted into a fresh exact-master
fuzzer control. Seed
`00a76e59d7b467c470ddf35b051073c01900f0ad` failed master at the new
`assert(!topup_succeeded)`; the same seed passed on the fixed branch. The fix
stages the descriptor, cache index, script map, and pubkey map locally, checks the
descriptor write, and publishes them only after all writes succeed. The public
`TopUp()` transaction then rolls back cache rows when the descriptor write fails.
The new `topup_descriptor_write_failure` unit test checks the exception, memory
state, persisted descriptor range, and cache-row count.

The direct scriptpubkeyman harness replayed all 10,025 QA inputs using two
independent workers without a failure. A full sanitized corpus attempt reached
7,637 executions per worker before a controlled stop in the large-input slow
region; it produced no Bitcoin diagnostic or target artifact. A completed capped
sanitized gate used two ASan/UBSan workers, 256 seeds, and 1,000 mutations per
worker, adding 85 and 89 coverage units with no diagnostic or artifact. The
metadata fuzzer catches only the exact expected `TopUpWithDB` runtime error; the
existing all-write mutation was also extended to accept its separately expected
descriptor-row failure when no cache row needs writing.

This proof covers the public `TopUp()` path and its explicit transaction. The
internal `TopUpWithDB()` helper is also used by callers that supply their own
batch context; this commit does not claim to make every larger import/update
workflow atomic, and no clean-master failure was demonstrated for those callers.

### 37. Descriptor update publishes state before a cache write failure

* Current branch fix: the grouped wallet descriptor-update rollback commit.
* Severity on clean master: medium local wallet-storage consistency and availability
  issue. The trigger requires a descriptor update to encounter a database write
  failure; no remote, consensus, key-loss, or unauthenticated memory-safety path was
  demonstrated.

`DescriptorScriptPubKeyMan::UpdateWalletDescriptor()` cleared the live script and
pubkey maps, reset `m_max_cached_index`, and assigned the replacement descriptor
before calling `UpdateWithSigningProvider()`. On a valid `wpkh(xpub/*)` update with
an expanded range, a SQLite trigger rejecting descriptor writes made clean master
throw `TopUpWithDB: writing cache items failed` after rebuilding part of the live
map. The object then reported a replacement `range_end` with a zero cached-index
boundary, while the database still contained the old descriptor range. A caller
continuing after the import error could therefore miss or mis-handle wallet
scripts until restart.

The exact clean-master control was run against `origin/master`
`26b730cdbf2c77c12f684fd50bd376212b725394` in a separate worktree. It passed only
when asserting the bad state: one live script, `GetEndRange() == 0`, an in-memory
replacement range of 2, and a persisted range of 1. The production source was
unchanged in that control. The fixed deterministic unit test uses the same
descriptor, replacement range, and all-write SQLite trigger; it requires the
exception, preserves the original live map and range, and verifies that both the
persisted descriptor and the pre-existing cache-row count are unchanged.

The production fix begins a wallet database transaction before publishing the
replacement, snapshots the descriptor, key maps, script maps, and cached-index
boundary, and restores them if any update write or commit fails. The new
`scriptpubkeyman` fuzzer mutation performs the same valid range extension with an
empty replacement cache while all descriptor writes are rejected, catches only the
two expected `TopUpWithDB` write errors, and asserts the original live postconditions.
The normal wallet fuzzer completed 2,000 bounded corpus/mutation executions
(`11,572` to `11,711` coverage); the ASan/UBSan wallet fuzzer completed 705
executions over the 702-file slice with no diagnostic or artifact.

### 38. Equal-feerate TxGraph prefix accumulates overflowing fees

* Current branch fix: `32ed8c5596` (`txgraph: avoid fee overflow in equal-feerate prefixes`).
* Severity on clean master: medium local cluster-mempool/block-builder consistency
  issue. The trigger uses the full signed fee-coordinate API or a local/direct fee
  modification path; no consensus, wallet, unauthenticated remote, or persisted-state
  impact was demonstrated.

The full-range alternate `AddTransaction` and `SetTransactionFee` mutation from
`2de843c875` exposed a production path that the earlier TxGraph oracle failures did
not. `GenericClusterImpl::Updated()` needed only the cumulative size of the prefix
of equal-feerate chunks for `m_main_equal_feerate_chunk_prefix_size`, but it also
summed the chunks' `FeeFrac` fees. Two equal chunks with fee
`4882961666570175427` overflowed the signed 64-bit fee coordinate. The same
overflowed prefix then caused `SanityCheck()` to reject its ratio invariant.

The clean-master control used baseline `afa5e46bbc6d`, with only the branch
`src/test/fuzz/txgraph.cpp` mutation transplanted and all production sources left
unchanged. The original 384-byte artifact has SHA256
`8b6f9c911745a6db52fd35bb9b59c383d22a92ad91a1b6873047978b1575b83c`. A normal
Debug replay trapped in `FeeFrac::operator+=`; an exact-master Clang 19
ASan/UBSan replay reported signed integer overflow at `src/util/feefrac.h:109`
before the production `SanityCheck()` assertion. The only later master range,
`afa5e46bbc6d..610dd320d1`, removes a testnet3 seed and does not touch this path.

The deterministic `txgraph_equal_feerate_prefix_does_not_overflow` regression
creates three equal-feerate parents, each with fee `INT64_MAX / 2 + 1` and size 1,
all depending on one zero-fee child. Before the fix, the existing saturating
`FeeFrac` arithmetic made the stored prefix ratio disagree after the third equal
chunk; after the fix, the shared helper tracks the reference ratio and cumulative
size directly and asserts the equal-rate precondition in production code. The
`txgraph_tests` suite (21 cases), `cluster_linearize_tests` suite (18 cases), and
the same artifact under normal and ASan/UBSan branch fuzz binaries all pass.

This is a follow-up source/test commit to the existing fuzzer mutation and the
related saturated-chunking fix in `eb5a249ff2`; no duplicate fuzzer mutation was
needed. It is counted as a confirmed clean-master defect, unlike the three earlier
full-range TxGraph oracle omissions.

The same round also replayed the separate 31-byte `INT64_MIN` reverse-fee control
(SHA256 `67d5dc06605771d92342ee080a49e2950a42e15d8c3d829d0d1b12d368f323b`). Exact
master still reports undefined negation at `cluster_linearize.h:1981`; the current
branch passes it through existing fix `09ad70a0a4`. This is revalidation of that
already counted and fixed clean-master defect, not an additional finding from this
round.

## Compact-block short-ID investigation

### Rebase assessment (2026-07-22)

The branch was fetched and rebased onto clean `origin/master`
`7b6f9ba7bad13b0c4169259000f7802854cdda0d`. Since the earlier compact-block
comparison at `bc49bd154a31`, master added Qt, ZeroMQ, include-lint, seed address
BIP324-service-flag, descriptorprocesspsbt invalid-signature handling, and the
PR #34672 mining IPC changes; none alter compact-block construction, reconstruction,
validation, or the fuzzer code covered below. The historical short-ID accounting
defect remains fixed by master commit `6aa5d8d948` (PR #35727), and the normal
production path still has the #35670 optimization (`6f1c56f03a`) that avoids null tail
entries. Rechecking after this rebase changed commit identifiers and the baseline, but
produced no new clean-master compact-block defect or race and justified no additional
production fix.

After the latest rebase onto `7b6f9ba7ba`, the rebuilt normal wallet fuzzer replayed
1,971 inputs from the preserved `cmpctblock` corpus and 2,016 inputs from the
preserved `partially_downloaded_block` corpus. Both collision-heavy targets completed
without an assertion, sanitizer report, race/deadlock report, timeout, or artifact.

A final targeted collision replay on 2026-07-23 used a private 512-input slice of
the existing `coins_view_db` corpus for `coins_view_db_resize_cursor` and private
copies of the compact-block corpora. The normal resize target completed 5,000
mutations; its resulting 735-file input set was replayed under ASan/UBSan (736
executions) and the direct-file TSan driver (735 files), with no diagnostic or
artifact. The collision-heavy `cmpctblock` target completed 5,000 normal
mutations, and the generated additions were replayed under ASan/UBSan (184
executions) and direct-file TSan (179 files), also clean. These are additional
current-branch gates, not new production fixes; the exact-master comparison
recorded below remains the clean-baseline control, and compact-block production
sources did not change between that control and `7b6f9ba7ba`.

The historical `b8844d3df7..5311b157` master delta was also checked; its only relevant
new runtime change is in `src/rpc/rawtransaction.cpp` for invalid PSBT signatures.
The subsequent `5311b15727..7b6f9ba7ba` delta is PR #34672's mining IPC work and does
not invalidate the compact-block control results.

### Historical real defect, fixed on current master

* Branch accounting work: `f6fdbb11a2` and the collision-focused fuzz commits.
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
  reached a null dereference. The guard is in `e5887a465f`; current production extra
  cache construction was changed by #35670 (`6f1c56f03a`) to avoid null tail entries.
  `ebe7771a92` adds the distinct extra-source/null ordering to both the unit oracle and
  fuzzer. This remains direct-API hardening on current master.
* More than `uint16_t` short-ID positions wrapped the internal position map and could
  overwrite the wrong slot. `6452baca8b` rejects the oversized direct input. Wire
  deserialization already rejects a transaction count above `uint16_t::max`, so no
  remote compact-block path was demonstrated.
* `FillBlock()` left partial header/counter state after too-short input and left
  derived counters after a successful fill. `677b49c96a`, `25de1a0975`, and
  `50e082f637` reset or preserve state at the reusable-object boundary. The production
  caller discards failed requests, so no remotely reachable state corruption was
  shown.
* Constructing `CBlockHeaderAndShortTxIDs` from an empty or sparse in-memory `CBlock`
  could underflow the short-ID vector size or dereference a null transaction while
  deriving the IDs. `c179ef3dd1` adds the production preconditions and deterministic
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

### BnB attempt-limit contract (2026-07-22)

The existing wallet fuzzer's `bnb_finds_min_waste` target intentionally limits its
pool to 16 groups so it can brute-force every subset. That means it cannot reach the
`TOTAL_TRIES` boundary. The current master unit test
`bnb_exhaustion_with_solution_test` uses 19 effective values `100000..100018`, a
target of `800000`, and the default `cost_of_change` of 359 sats to reach exactly
100,000 evaluated selections while still returning a valid, explicitly incomplete
result. Before this change, no fuzzer target constructed that boundary or checked the
corresponding result contract.

The new `bnb_attempt_limit` target uses that fixture for empty input and lets fuzz
bytes perturb the target, change cost, effective values, and input weights while
keeping all amounts positive and bounded. For every returned result it checks the
target window, maximum weight, the 100,000-attempt ceiling, and the rule that an
incomplete result reports exactly 100,000 attempts. Production `SelectCoinsBnB()` now
has matching `Assert()` postconditions for the same two attempt invariants. The
existing deterministic unit test remains the fixed canonical regression test; no new
production bug was inferred from the added mutation.

The branch's canonical empty-input run passed in the normal, ASan/UBSan, and TSan
drivers. The normal target then completed 1,000 corpus executions; ASan/UBSan
completed 1,000; and the TSan single-input driver replayed 484 files including the
483 preserved `coinselection_bnb` seeds. No assertion, sanitizer, race/deadlock
report, timeout, or artifact was produced.

After rebasing onto current master `7b6f9ba7ba`, the wallet fuzzer was rebuilt and the
canonical input plus a private copy of the 483-seed corpus completed successfully
again; the Debug `bnb_exhaustion_with_solution_test` also passed with
`ABORT_ON_FAILED_ASSUME` enabled.

For the historical clean-master control, exact
`5311b15727f2f282274472184185423e441abd85` received only the new fuzzer target. Its
canonical run and a 1,000-execution replay of the same 483 preserved seeds passed with
the production source unchanged. The later `5311b15727..7b6f9ba7ba` master range does
not touch wallet coin selection. To prove
the oracle is mutation-sensitive, a temporary clean-master source mutation changed
only BnB's `result.SetAlgoCompleted(false)` to `true`; after rebuilding, the one-byte
zero seed failed deterministically at the new `!GetAlgoCompleted()` assertion (exit
77). The mutation was removed, the clean-master binary rebuilt, and the canonical plus
1,000-run control passed again. This is a coverage and production-contract hardening
commit with severity `n/a`; it does not increase the confirmed-defect count.

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
  while branch production commit `398aad7290` intentionally uses `GetChunking()` to
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
* The coins money-range work (`81536edc94`) caught fuzz-generated invalid coins being
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
baseline `32eb521002`; later controls explicitly name the intermediate
`bc49bd154a` baseline or the current `a2e074d66a` baseline. The later
`validation_block_reorg` gate ran on the intermediate `559d042ba2` baseline and is
called out separately below.

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
* The post-rebase AddrMan mutation gate replayed the same 2,191-input corpus with
  two independent normal workers for 3,000 mutations each. They completed in 448
  and 458 seconds, reached the same coverage plateau of 5,536, added 3 and 7 fresh
  units, and used 163 and 165 MB peak RSS without an assertion or artifact. The
  ten fresh units were replayed with two ASan/UBSan workers (27 and 28 executions
  in 12 and 13 seconds, 292 and 295 MB peak RSS) and two TSan/libFuzzer workers
  (20 executions each, 137 and 138 MB peak RSS); all four replays were clean.
  This is independent multi-worker evidence for the `GetEntries`, `GetAddr`,
  `Select`, collision-resolution, and service-update mutations, not a new
  production defect or race. The current master tip remains an ancestor and its
  later changes do not touch AddrMan.
* `addrman_serdeser` was also started from its 1,437-input corpus with two normal
  workers. Both reached 443 executions at about one execution per second before
  the bounded run was stopped after roughly 5.5 minutes (77 and 79 MB peak RSS),
  without a diagnostic or artifact. That full gate is incomplete because the
  serializer's generated address state becomes expensive, rather than because of
  a failure. A completed smoke gate over the 32 smallest seeds (152 bytes total)
  ran 33 executions in each of two ASan/UBSan workers (3 seconds, 311 and 312 MB)
  and two TSan/libFuzzer workers (under 1 second, 129 and 126 MB); all exited 0
  without an artifact. No clean-master AddrMan production bug or race was proved.
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
* A post-rebase `validation_block_reorg` gate ran from the preserved 2,257-input
  corpus on the current branch. Two independent normal workers completed 5,000
  executions each, reaching coverage 13,864 and 13,865; the expanded private
  corpora were then replayed by two ASan/UBSan workers and two TSan workers for
  5,000 executions each. ASan/UBSan reached coverage 42,907 and 42,915 in 691
  and 683 seconds; TSan reached coverage 5,279 in 584 and 581 seconds. All six
  jobs exited zero without an assertion, sanitizer report, race/deadlock report,
  timeout, or artifact. The inputs exercised genesis reprocessing, empty and
  same-height competing branches, longer reorgs, invalid descendants,
  invalidation/failure-flag reset, precious-block no-ops, flush transitions, and
  validation notifications. This extends the prior older-baseline gate; no new
  production defect or race was found, so no clean-master control was needed.
* The RPC fuzzer was replayed with `LIMIT_TO_RPC_COMMAND=descriptorprocesspsbt`
  against all 13,390 preserved inputs in two independent normal workers. Each
  completed 13,391 executions at coverage 15,825. The two ASan/UBSan workers
  completed 13,393 executions at coverage 53,070 and 53,076; the two TSan workers
  completed 13,391 executions at coverage 6,791. Every job exited zero without an
  assertion, sanitizer report, race/deadlock report, timeout, or artifact. This
  exercised the current-master invalid-signature handling and malformed PSBT
  combinations in PR #33014; no production defect or race was found.
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
* A current normal `txrequest` run replayed the original 727-input corpus with two
  independent workers for 5,000 mutations each. They completed in 500 and 504
  seconds, at peak RSS of 66 and 67 MB, with no assertion, sanitizer, or crash
  artifact. The workers added 82 unique units to the shared QA corpus without
  increasing the 2,415-coverage plateau. Replaying that fresh 82-unit slice under
  two ASan/UBSan workers completed 103 and 104 executions in 101 and 105 seconds
  at about 732 MB RSS; two TSan/libFuzzer workers completed 110 and 114 executions
  in 11 seconds at 129 and 130 MB RSS. All four replays were clean. This is
  additional state-space and sanitizer evidence for duplicate announcements,
  expiry, time reversal, re-selection, and disconnect cleanup, not a new
  production defect or race.
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
* A post-rebase `tx_package_eval` replay used two private copies of 2,440 corpus
  inputs below 64 KiB. Two normal workers completed 3,000 executions each in 344
  seconds, reaching coverage 19,068 and 19,069 and expanding their corpora to
  1,595 and 1,594 units. The two ASan/UBSan workers reached the 1,024 pulse and
  were deliberately interrupted in the known slow package-building input
  `e383bdbfe9aaf3d4104e7441e29eeb05596de952`; neither produced a sanitizer or
  crash artifact. The two TSan workers completed 2,452 and 2,461 total executions
  in 152 and 153 seconds at coverage 8,537 and 8,553, without a race report or
  artifact. This remains an incomplete ASan gate, not a production finding; no
  package-state inconsistency or race was found in the completed runs.
* A post-rebase `ephemeral_package_eval` campaign used two independent copies of
  the 2,098-input QA corpus. The normal workers reached 1,240 and 1,241 total
  executions before being interrupted in the known slow package-construction
  region (exit 72); neither added a unit or produced an artifact. Two ASan/UBSan
  workers replayed a frozen 256-input slice and were stopped at 137 total
  executions after the 128 pulse, at about 1 execution/sec, with coverage 58,820
  and 58,833 and no diagnostic or artifact. Two TSan/libFuzzer workers completed
  the same-sized independent slices, 257 total executions each in 70 and 71
  seconds, at coverage 7,681, without a race report or artifact. The fuzzer's
  package-result, duplicate-rejection, mempool-index, and unchanged-state
  assertions therefore remained clean in the completed paths; this is an
  incomplete ASan/normal corpus gate, not evidence of a package defect.
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
* After the master rebase, the full 2,098-input `ephemeral_package_eval` corpus was
  rerun with two fresh ASan/UBSan workers. Both reached 788 executions in about
  five and a half minutes, at peak RSS of 481 and 476 MB, before the same
  CPU-bound package-building region was deliberately interrupted. The corrected
  TSan/libFuzzer binary reached 1,520 executions per worker at about 317 MB RSS
  before the corresponding bounded interruption. Neither sanitizer produced a
  diagnostic, race report, timeout artifact, or crash artifact. A completed
  32-seed smoke slice then ran 33 executions per worker: ASan/UBSan finished in
  74 seconds at 477 and 475 MB RSS, and TSan finished in 8 seconds at 315 and
  317 MB RSS, all clean. An earlier invocation of the standalone non-libFuzzer
  TSan driver failed in its file-reading assertion before target execution and
  is excluded from the gate and findings. This remains a performance-limited
  ephemeral-package gate, not a production defect.
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
* A completed current-branch wallet campaign then used the same 9,817 filtered
  `scriptpubkeyman` inputs. Two independent normal workers ran 13,000 executions
  each; two ASan/UBSan workers and two TSan/libFuzzer workers ran 1,024 each. All
  six jobs exited 0 without an assertion, sanitizer report, race report, timeout,
  or target artifact. The coverage plateau and resident memory remained bounded;
  this is evidence for the exercised descriptor-manager action sequences, not a
  general wallet thread-safety proof.
* `spkm_migration` used all 778 existing inputs. Two normal workers ran 10,000
  executions each, followed by two ASan/UBSan and two TSan/libFuzzer workers at
  1,024 executions each. Every job exited 0 without a diagnostic or artifact. The
  migration fuzzer's legacy/descriptor key-chain, watch-only, multisig, and script
  invariants therefore have current sanitizer coverage; no additional migration
  defect was found.
* `script_descriptor_cache` used 440 existing inputs below 64 KiB. Two normal
  workers ran 20,000 executions each, then two ASan/UBSan and two TSan/libFuzzer
  workers ran 1,024 each. All jobs exited 0 without an exception outside the
  fuzzer's expected `std::runtime_error`, sanitizer report, race report, or
  artifact. This current-branch gate is separate from the exact-master control
  that reproduced the partial-merge defect above.
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
* A post-rebase `coins_view_db_resize_cursor` campaign used 512 private inputs from
  the existing `coins_view_db` corpus. Two normal workers completed 3,000 mutations
  each, reaching coverage 6,044 and 6,042 without an assertion or artifact. The
  resulting expanded slice was replayed by two ASan/UBSan workers for 3,000
  executions each (coverage 19,456 and 19,458) and by two TSan/libFuzzer workers for
  3,000 executions each (coverage 2,555 and 2,557); all four sanitizer workers
  exited 0 without a diagnostic or artifact. Each sanitizer worker used its own
  frozen input copy, so corpus-file replacement could not be mistaken for a target
  race. The campaign exercised the `m_db_mutex` handoff while a cursor performed
  `GetKey`/`GetValue`/`Next`, the resize-thread join, and exact pre/post-resize
  snapshots. No coins-cache inconsistency, memory defect, or race was found. Two
  earlier TSan attempts using the direct multi-file driver were excluded because
  that binary interpreted libFuzzer flags as input paths; they did not execute the
  target.
* A final single-worker resize replay then completed 5,000 normal mutations from
  the same private seed slice. The resulting 735 files completed ASan/UBSan replay
  (736 executions) and direct-file TSan replay (735 files) without a report or
  artifact. This adds current-branch stress evidence but does not change the
  clean-master severity assessment or identify a new coins-cache defect.
* `coins_view_stacked` had no native QA corpus. A fixed empty-input smoke run repeated
  the stacked setup 1,000 times per worker under both sanitizers, but libFuzzer
  correctly reported that it performed no mutation. A mutation run seeded from
  21,408 existing `coins_view` inputs completed under TSan with 21,874 executions per
  worker in 87 seconds and no report or artifact. The corresponding ASan/UBSan run
  reached pulse 16,384 in the expensive corpus region without a report or artifact;
  a second run over 21,388 inputs below 64 KiB reached the same pulse and was stopped.
  These are incomplete ASan gates, not evidence of a stacked-cache defect.
* The current post-rebase `coins_view_stacked` mutation gate seeded two independent
  normal workers from 21,388 `coins_view` inputs below 64 KiB and ran 25,000
  executions per worker. They completed in 60 and 61 seconds, added 80 and 90
  units, reached coverage 10,491 and 10,493, and used 83 and 81 MB peak RSS,
  without an assertion or artifact. The original 163 fresh units were replayed
  under branch ASan/UBSan (256 executions per worker, 11 and 12 seconds, 419 MB
  peak RSS) and TSan/libFuzzer (256 per worker, 2 seconds, 150 MB peak RSS), with
  no diagnostic, race report, or artifact.
* The same fresh-unit slice was then replayed against unmodified clean master
  `bc49bd154a31` in an isolated Clang 19 build. The ASan/UBSan workers completed
  256 executions each over an expanded 238-seed slice in 4 seconds at 394 and
  395 MB peak RSS, without a report or artifact. A frozen 249-seed superset was
  replayed by two clean-master TSan/libFuzzer workers for 256 executions each in
  2 seconds at 141 and 142 MB peak RSS, also without a report, race diagnostic, or
  artifact. This closes the former stacked ASan gap and proves no clean-master
  overlay production defect or race in the exercised reset, out-of-order lookup,
  concurrent-read, and cache-publication combinations.
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

## Post-rebase fee and knapsack gates (2026-07-22)

Two additional wallet targets were exercised on the rebased branch. These runs
produced no production finding and did not change the severity of any item above.

* `wallet_fees` started from all 134 QA inputs. Two independent normal workers
  completed 3,000 executions each in about two seconds, reaching coverage 1383 and
  adding 11 and 9 units. The resulting corpora were run under two ASan/UBSan
  workers for 5,000 executions each (15 seconds, peak RSS 565 and 567 MB) and two
  TSan workers for 5,000 executions each (19 seconds, peak RSS 557 MB). All six
  jobs exited zero without an assertion, sanitizer report, race report, timeout, or
  artifact. This exercised the existing `GetMinimumFeeRate` floor contracts across
  fallback, discard, minimum, mempool minimum, coin-control feerate, confirmation
  target, and conservative/economical fee-estimation combinations.
* `coinselection_knapsack` started from 517 of 575 QA inputs below 64 KiB. Two
  normal workers completed 3,000 executions each in 9 and 8 seconds, reaching
  coverage 1078 and adding 2 and 10 units. Two ASan/UBSan workers then completed
  5,000 executions each in 116 and 123 seconds, with 558 MB peak RSS. Two TSan
  workers completed 5,000 executions each in 46 and 52 seconds, with 557 MB peak
  RSS. Every job exited zero without an assertion, sanitizer report, race report,
  timeout, or artifact. The mutations covered extreme fee rates, subtract-fee
  outputs, empty and non-positive output groups, eligibility filters, selection
  weight limits, change targets, bump-fee discounts, and post-selection input
  merging. No coin-selection inconsistency or race was found.

The sanitizer workers used separate wallet/coin-selection instances, so these runs
are evidence for the exercised target state and race instrumentation, not proof of
thread safety for shared live wallet objects.

## Post-rebase BDB parser gate (2026-07-22)

`wallet_bdb_parser` was replayed from its 125-input QA corpus with two independent
normal workers. Each completed 5,000 executions, reached coverage 320, and added
two reduced units without rethrowing an exception outside the fuzzer's explicit
Berkeley DB parse-error allowlist. The expanded corpora were replayed with two
ASan/UBSan workers and two TSan/libFuzzer workers, 5,000 executions per worker.
All four sanitizer jobs exited zero in about two seconds at 558 MB peak RSS; none
produced a sanitizer report, race/deadlock report, timeout, or artifact.

This target writes each input to a wallet file, opens it through the read-only BDB
adapter, and dumps successfully parsed databases. The result strengthens the
exception-narrowing coverage for malformed wallet files, but it does not establish
that every BDB error string is classified correctly or that live wallet migration
callers are race-free. No production parser defect was found in this campaign.

## Post-rebase CoinGrinder gates (2026-07-22)

The `coin_grinder` target was seeded from 668 of its 698 QA inputs below 64 KiB.
Two independent normal workers completed 2,000 executions each in 13 and 11
seconds, reaching coverage 1038 and adding 1 and 2 units. Two ASan/UBSan workers
completed 3,000 executions each in 56 and 61 seconds at 559 MB peak RSS. Two TSan
workers completed 3,000 executions each in 45 and 47 seconds at 559 MB peak RSS.
All six jobs exited zero without an assertion, sanitizer report, race/deadlock
report, timeout, or artifact.

`coin_grinder_is_optimal` used all 265 QA inputs. Its normal workers completed
2,000 executions each in about two seconds, reaching coverage 362; its ASan/UBSan
workers completed 3,000 each in eight seconds at 559 MB peak RSS; and its TSan
workers completed 3,000 each in seven and eight seconds at the same RSS. Those six
jobs also exited zero without diagnostics or artifacts. The targets exercised
fee-rate and long-term-fee extremes, change-cost thresholds, maximum selection
weights, empty/small output-group sets, cross-algorithm comparisons, and exhaustive
subset optimality. No selection inconsistency or race was found.

The brute-force assertions are valuable evidence for the tested small-group model,
but each worker owns independent selection state; they do not prove that shared
wallet coin-selection callers are race-free.

### Cross-seeded `bnb_finds_min_waste`

This related target had no native QA corpus, so it was cross-seeded from the 668
bounded `coin_grinder` inputs. Two normal workers completed 3,000 executions each
in five and six seconds, reaching coverage 488 and 491 and adding 36 and 37 units.
Two ASan/UBSan workers completed 3,000 executions each in 26 and 25 seconds, with
coverage 1042 and 1041 and 559 MB peak RSS. Two TSan workers completed 3,000 each
in 18 and 15 seconds, with coverage 495 and 559 MB peak RSS. Every worker exited
zero without an assertion, sanitizer report, race/deadlock report, timeout, or
artifact. The cross-seed added useful BnB waste, dust, change-cost, and subset
boundary states, but it did not expose a production inconsistency or race; no
clean-master source comparison or production change was warranted.

## Post-rebase fee arithmetic gates (2026-07-22)

Three lower-level fee targets were run with two independent workers in each build
configuration. `fees` replayed all 420 QA inputs: normal workers completed 5,000
executions each and reached coverage 98, ASan/UBSan workers completed 5,000 each
with coverage 211, and TSan workers completed 5,000 each with coverage 98. The
normal workers added 6 and 8 units; all sanitizer workers exited zero in two or
three seconds at 559 MB peak RSS, without an assertion, sanitizer report, race or
deadlock report, timeout, or artifact.

`fee_rate` replayed all 32 inputs. Its normal workers reached coverage 266 and
added 40 and 39 units; ASan/UBSan reached coverage 468 and 467; and TSan reached
coverage 266 in both workers. Each build ran 5,000 executions per worker and
exited zero without diagnostics or artifacts. `feefrac` likewise replayed all 70
inputs, with normal coverage 123, ASan/UBSan coverage 217, and TSan coverage 123;
each of its six workers completed 5,000 executions cleanly.

Together these runs exercised signed and saturating fee addition/multiplication,
round-up and round-down division, zero and negative rates, maximum-size inputs,
FeeFilterRounder bounds, ratio ordering, and fee-reason serialization. No arithmetic
inconsistency, memory error, or race was found, and no production change or
clean-master comparison was warranted.

## Post-rebase MiniMiner cluster gate (2026-07-22)

`mini_miner` was seeded from 1,641 of its 1,650 QA inputs below 64 KiB. Two normal
workers completed 3,000 executions each in 78 and 76 seconds, reaching coverage
3791 and 3790 and adding 77 and 76 units. The expanded corpora were replayed by
two ASan/UBSan workers for 3,000 executions each in 264 and 265 seconds, reaching
coverage 8929 and 8927 with 711 and 716 MB peak RSS. Two TSan workers completed
3,000 executions each in 205 and 203 seconds, reaching coverage 3793 and 3792
with 560 MB peak RSS. Every worker exited zero without an assertion, sanitizer
report, race/deadlock report, timeout, or artifact.

This gate exercised clustered parent/child transactions, `GatherClusters` query
deduplication, manual-versus-pool MiniMiner linearization, duplicate and unknown
outpoints, topological order, per-outpoint bump-fee monotonicity, saturated total
bump fees, target-feerate ordering, and mock-template membership. The comments in
the fuzzer intentionally avoid asserting aggregate monotonicity when signed fee
prioritisation saturates; no additional production inconsistency or race was found
and no clean-master comparison or production change was warranted.

## Post-rebase AddrMan and wallet gates (2026-07-22)

The latest fetch still points `origin/master` at
`a2e074d66ac17ca7907909bbbb563e77185a45e5`. The only delta since the exact
descriptor-cache control is the AddrMan equality simplification and Qt test changes;
neither changes descriptor-cache, compact-block, or wallet transaction production
logic. The following gates therefore ran on the current rebased branch and are
additional evidence for the existing findings, not new fixes.

### AddrMan serialization and equality

`addrman_serdeser` was seeded from 1,295 of the 1,437 existing QA inputs (the
remaining inputs exceeded the bounded seed-size limit). Two independent normal
workers completed 1,296 executions each, with 1891 coverage and 558 MB peak RSS.
Two ASan/UBSan workers completed 1,296 executions each in about 692 seconds, with
641 and 645 MB peak RSS. Two independent TSan/libFuzzer workers completed 1,296
executions each in 559 and 580 seconds, with 559 MB peak RSS. Every worker exited
zero, and all six artifact directories were empty. The fuzzer's serialize/deserialize
equality contract, including the current master's defaulted `AddressPosition`
comparison, exposed no assertion, memory error, or race.

This is still a per-process gate: the two workers do not share an AddrMan instance.
It is evidence against races or state corruption in the exercised serialization and
mutation setup, not a proof that all live AddrMan callers are thread-safe.

### Wallet transaction construction

`wallet_create_transaction` started from two independent copies of its 1,357-input
QA corpus. Normal workers completed 2,000 executions each in 125 and 129 seconds,
adding 16 and 25 corpus units, respectively, without an assertion or artifact.
Those expanded corpora were replayed under two ASan/UBSan workers (1,577 and 1,586
executions; 590 and 592 MB peak RSS; 357 and 364 seconds) and two TSan/libFuzzer
workers (1,373 and 1,382 executions; 558 MB peak RSS; 294 and 299 seconds). All
four sanitizer workers exited zero with no sanitizer report, race report, timeout,
or artifact.

The target exercised synthetic confirmed and unconfirmed ancestry, shared-wallet
outputs, `AvailableCoins`, coin control, change, locktime, fee-rate, recipient, and
mempool combinations. The normal corpus mutations also learned serialized values
for zero/one/three transaction counts and `timesmart`; none exposed a production
inconsistency. The fuzzer constructs independent wallet state in each process, so
these results do not rule out races between a live wallet, validation callbacks, and
the mempool. They do strengthen the conclusion that no new wallet transaction
defect was found in this campaign.

## Post-rebase fee-estimator state and persistence gates (2026-07-22)

The `policy_estimator` target was seeded from 1,055 of 1,254 QA inputs below 64 KiB;
`policy_estimator_io` was seeded from 1,809 of 1,916. The first target mutates
transaction arrival/removal, block processing, fee estimates, thresholds, horizons,
and `FlushUnconfirmed`, then checks fee-estimate contracts and a final write/read
round trip. The I/O target reuses a static estimator across inputs and checks that a
failed read leaves every captured estimator field unchanged, while a successful read
keeps targets and fee values in range before writing it back.

`policy_estimator` normal workers completed 3,000 executions each in about two
seconds, reached coverage 1270 and 1271, and added 42 and 48 units at 560 MB peak
RSS. Their expanded corpora were replayed by ASan/UBSan workers for 3,000 executions
each: coverage was 2331 and 2335, runtime about 11 seconds, peak RSS 627 and 632 MB,
and 33 and 32 units were added. TSan workers likewise completed 3,000 executions
each in 14 and 15 seconds, reached coverage 1276 and 1280, and added 30 and 39 units
at 559 MB peak RSS.

`policy_estimator_io` normal workers completed 3,000 executions each in about six
seconds, reached coverage 511, and added 3 and 4 units at 560 MB peak RSS. Its
ASan/UBSan workers completed 3,000 each in about 69 seconds, reached coverage 1072,
and added 3 and 1 units at 558 MB peak RSS. Its TSan workers completed 3,000 each in
36 and 37 seconds, reached coverage 511, and added 2 units each at 559 MB peak RSS.
All twelve jobs exited zero without an assertion, sanitizer report, TSan race or
deadlock report, timeout, or target artifact.

The persistence target's static estimator state is intentionally exercised across
sequential libFuzzer inputs within each process. That tests reset, failed-read, and
round-trip contamination contracts, but it is not a concurrent production race
proof; no live estimator instance is shared between these workers. No policy
estimator production defect, persistence inconsistency, or race was found, so this
gate added coverage and evidence only and did not change the severity ledger.

## Post-rebase package and mempool completion gates (2026-07-22)

The earlier `tx_pool_standard`, `tx_package_eval`, and `ephemeral_package_eval`
sanitizer runs were limited by large or expensive corpus cases. A follow-up used
two independent normal workers to generate fresh units, then replayed each expanded
corpus in separate ASan/UBSan and TSan workers. The standard and ordinary package
targets used inputs below 64 KiB; the ephemeral target's 2,098-input corpus was
already below 36 KiB. These are completed bounded gates and supersede neither the
earlier incomplete high-size observations nor the distinction between current-branch
fuzzer contracts and clean-master production behavior.

`tx_pool_standard` started from 2,826 filtered inputs. Its normal workers completed
3,000 executions each in 20 seconds, reached coverage 8931, and added 14 and 13
units. ASan/UBSan completed 3,000 each in 85 seconds at coverage 19830 and 19823,
adding 6 and 5 units. TSan completed 3,000 each in 84 seconds at coverage 8944 and
8942, adding 4 and 10 units. All six workers used 557 MB peak RSS and exited zero
without an assertion, sanitizer report, race/deadlock report, timeout, or artifact.
The target exercised standard and non-standard acceptance, RBF prioritisation,
TRUC limits, randomized and witness indexes, ancestry/cluster invariants, fee and
size accounting, block-template removal, and validation-signal callbacks.

`tx_package_eval` started from 2,440 filtered inputs. Its normal workers completed
3,000 executions each in 38 and 37 seconds, reached coverage 8544 and 8543, and
added 20 and 23 units. ASan/UBSan completed 3,000 each in 178 and 175 seconds at
coverage 19208 and 19203, adding 26 and 21 units. TSan completed 3,000 each in 178
and 176 seconds at coverage 8553 in both workers, adding 21 and 24 units. All six
workers used 557 MB peak RSS and exited zero without an assertion, sanitizer report,
race/deadlock report, timeout, or artifact. The package test-accept versus submit
paths, duplicate-package rejection, partial result maps, client feerate limits,
TRUC/package policy, outpoint indexes, and unchanged-state snapshots remained
consistent.

`ephemeral_package_eval` replayed all 2,098 inputs. Normal workers completed 3,000
executions each in 141 and 145 seconds, reached coverage 7802 and 7822, and added
48 and 45 units. ASan/UBSan completed 3,000 each in 728 and 726 seconds at coverage
17813 and 17805, adding 23 and 29 units. TSan completed 3,000 each in 680 and 682
seconds at coverage 7813 and 7830, adding 38 and 32 units. All six workers used
557 MB peak RSS and exited zero without an assertion, sanitizer report,
race/deadlock report, timeout, or artifact. This specifically exercised dust-child
eviction, child double-spends that leave parents childless, ephemeral policy, and
package state cleanup.

These completed gates found no new production defect or race and require no source
fix. The workers use independent mempool and validation state, so the TSan results
cover the fuzzer-created callback and thread interactions but do not prove that
arbitrary live node instances are race-free. No clean-master comparison was
warranted because no production assertion, sanitizer failure, or behavioral
inconsistency was found.

## Post-rebase transaction-request gate (2026-07-22)

`txrequest` was replayed from 762 bounded QA inputs with two independent normal
workers. The target compares `TxRequestTracker` with a reference state machine while
mutating txid and wtxid announcements, preferred-peer ordering, request expiry,
duplicate announcements, responses, forgotten hashes, peer disconnects, and
single-peer and all-peer request queries.

The normal workers completed 3,000 executions each in 42 and 40 seconds, reached
coverage 1389 and 1388, and added 66 and 71 units at 559 MB peak RSS. Their expanded
corpora were replayed by ASan/UBSan workers for 1,000 executions each in 62 and 59
seconds, reaching coverage 2879 in both workers at 616 and 624 MB peak RSS and
adding 7 and 10 units. TSan workers completed 1,000 executions each in 49 and 50
seconds, reaching coverage 1389 and 1388 at 558 MB peak RSS and adding 2 and 5
units. All six jobs exited zero without an assertion, sanitizer report, race or
deadlock report, timeout, or artifact.

This closes the earlier full-seed ASan gap for the bounded corpus. The fuzzer's
reference model and the sanitizer workers operate sequentially inside each process,
so the result checks request-state consistency and sanitizer-visible lifecycle
behavior but is not proof that every live peer interleaving is race-free. No
production defect or clean-master failure was found, so no source fix was warranted.

## Post-rebase transaction-download manager gate (2026-07-22)

`txdownloadman_impl` started from 1,553 bounded QA inputs. Its normal workers
completed 3,000 executions each in 86 and 84 seconds, reached coverage 3441 in both
workers, and added 13 and 10 units at 560 MB peak RSS. The expanded corpora were
replayed under ASan/UBSan: the workers completed 1,567 and 1,564 seed-plus-mutation
executions in 227 seconds, reached coverage 7739 and 7738, and used 619 and 629 MB
peak RSS. TSan completed the same 1,567 and 1,564 executions in 209 seconds per
worker, reached coverage 3445 in both, and used 560 MB peak RSS. All six jobs exited
zero without an assertion, sanitizer report, race/deadlock report, timeout, or
artifact.

The sequence covered peer connect/disconnect and feature changes, active-tip and
block connect/disconnect notifications, mempool acceptance/rejection, txid and
wtxid announcements, request selection and completion, `NOTFOUND`, orphanage
ownership and usage, rejection/reconsideration filters, package validation work,
and final empty-state checks across all peers. The fuzzer is sequential within each
process and the workers use independent manager instances, so TSan evidence covers
the exercised callbacks and state transitions but does not prove arbitrary live
peer interleavings are race-free. No production inconsistency or clean-master
failure was found, so no source fix was warranted.

## Post-rebase message-processing lifecycle gate (2026-07-23)

The complementary `process_message` and `process_messages` targets were replayed
from their full bounded QA corpora. The single-message target resets chainman and
peer-manager state around one fuzzed message; the multi-message target keeps one to
three peers alive while processing repeated messages and checking send-queue memory
and peer relay state.

`process_message` normal workers completed 3,299 executions each in four seconds,
reaching coverage 8121 and 8127. `process_messages` normal workers completed 4,178
each in 14 seconds, reaching coverage 8725 in both. ASan/UBSan completed 4,037 and
4,025 executions for `process_message` in 19 seconds, reaching coverage 17123 and
17124 at 643 and 640 MB peak RSS. It completed 5,493 and 5,488 executions for
`process_messages` in 66 and 67 seconds, reaching coverage 18029 and 18035 at 558
MB peak RSS. TSan completed 3,299 `process_message` executions per worker in 18
seconds at coverage 8144 and 558 MB peak RSS, and 4,178 `process_messages` executions
per worker in 67 and 68 seconds at coverage 8750 and 558 MB peak RSS.

All twelve target workers exited zero without an assertion, sanitizer report,
race/deadlock report, timeout, or artifact. The gate covered malformed and valid
message types, peer feature combinations, message queues, repeated processing,
chainman reset and dirty-chain cleanup, AddrMan/PeerManager replacement, validation
callbacks, special-peer address relay, and send-queue accounting. One initial TSan
launcher used a misspelled scratch path and exited before target execution; it is
excluded from these counts and was replaced by the corrected worker.

No message-processing production defect or race was found, and no clean-master
comparison or source fix was warranted. Each fuzzer invocation owns its node and
peer state, so TSan covers the exercised lifecycle operations but does not prove all
live connection interleavings are race-free.

## Post-rebase concurrent DB wrapper gate (2026-07-23)

`dbwrapper_concurrent_reads` was replayed from 1,116 bounded QA inputs (maximum
input size 738 bytes) with two independent workers in each build mode. The target
uses a deterministic environment that defers compaction, seeds 100--5,000
entries, schedules up to eight persistent `dbfuzz` reader workers, and overlaps
read, existence, iterator-seek/range, erase/write, and reopen checks with the
main thread draining deferred compaction work. Each input also verifies the
final database state after all futures join.

Normal workers completed 1,117 executions each in about six seconds, reached
coverage 2457, and used 558 MB peak RSS. ASan/UBSan workers completed 1,133 and
1,127 executions in 60 seconds each, reached coverage 5930 and 5939, and used
832 and 890 MB peak RSS. TSan workers completed 1,117 executions each in 39
seconds, reached coverage 2454, and used 558 MB peak RSS. All six workers exited
zero without an assertion, sanitizer report, race/deadlock report, timeout, or
artifact; no generated corpus unit was needed to reproduce a failure.

The TSan result is meaningful for the target's persistent reader pool and the
deferred-compaction overlap, but each fuzzer process owns an independent
`DBWrapper` and its deterministic environment. It therefore does not prove
that arbitrary live node instances, external database users, or all LevelDB
thread schedules are race-free. No production inconsistency or clean-master
failure was found, so no source fix or deterministic unit-test addition was
warranted.

## Post-rebase bounded orphanage gate (2026-07-23)

The main `txorphan` state-machine target was replayed from 404 of its 686 QA
inputs after filtering out inputs at or above 64 KiB. The filter was deliberate:
the earlier full-corpus ASan attempt reached an artificial transaction-weight
workload whose megabyte-scale synthetic transaction caused sanitizer timeouts,
without producing a sanitizer diagnostic. This gate therefore adds sanitizer
coverage for the bounded state combinations but does not claim the full
unfiltered corpus is complete.

Two normal workers completed 3,000 executions each in 101 and 110 seconds,
reached coverage 3736 and 3737, added 151 and 150 units, and used 561 MB peak
RSS. Their expanded corpora were replayed by ASan/UBSan workers for 1,000
executions each, reaching coverage 3951 and 3946 at 560 MB peak RSS and adding
9 and 11 units. TSan then replayed the sanitizer-expanded corpora for 1,000
executions each, reaching coverage 1518 and 1519 at 560 MB peak RSS and adding
9 and 7 units. All six workers exited zero without an assertion, sanitizer
report, race/deadlock report, timeout, or artifact.

The target exercised orphan insertion and duplicate announcements, peer usage
and eviction, protected/reconsiderable children, parent work sets, peer and
block erasure, transaction-weight limits, and final `SanityCheck()` state
validation. Each fuzzer process owns an independent orphanage and executes its
state machine sequentially; the TSan result is not proof that arbitrary live
peer interleavings are race-free. No production inconsistency or clean-master
failure was found, so no source fix or deterministic unit-test addition was
warranted.

## Post-rebase cluster post-linearization gates (2026-07-23)

The `clusterlin_postlinearize` target replayed all 1,720 bounded QA inputs
(maximum input size 447 bytes). Its normal workers completed 3,000 executions
each in about one second at coverage 1021 and 560 MB peak RSS. ASan/UBSan
workers replayed 1,721 executions each in five seconds at coverage 3195 and
561 MB peak RSS. TSan workers replayed 1,721 executions each at coverage 428
and 561 MB peak RSS. The target checked topology, diagram monotonicity across
repeated `PostLinearize()` calls, and connectedness of representable chunks.

The distinct `clusterlin_postlinearize_moved_leaf` target replayed all 1,082
bounded QA inputs (maximum input size 478 bytes). Its normal workers completed
3,000 executions each in about one second at coverage 1021 and 558 MB peak
RSS. ASan/UBSan workers replayed 1,083 executions each in three seconds at
coverage 3164 and 558 MB peak RSS. TSan workers replayed 1,083 executions
each at coverage 424 and 558 MB peak RSS. This target checked the RBF-style
operation of moving a leaf to the back, increasing its fee, post-linearizing,
and preserving or improving the prior chunk diagram.

All twelve workers across both targets exited zero without an assertion,
sanitizer report, race/deadlock report, timeout, or artifact. Each process
owns an independent dependency graph and executes sequentially, so TSan does
not prove arbitrary shared cluster-mempool interleavings are race-free. No
production inconsistency or clean-master failure was found, so no source fix
or deterministic unit-test addition was warranted.

## Current-master bounded cluster linearization gate (2026-07-23)

The `clusterlin_linearize` target was rerun against the current rebased source,
including master's trained-cost-model implementation, from all 672 QA inputs
(maximum input size 521 bytes). A `-max_len=64` bound was used to keep mutation
work in the finite-cost region; this is a current-master coverage gate and not
a replacement for the separate unbounded TSan result or a full unbounded ASan
exhaustive-model proof recorded earlier.

Two normal workers completed 1,000 executions each in about one second, reached
coverage 1949, and added 11 and 12 units at 558 MB peak RSS. The expanded
corpora were replayed by ASan/UBSan workers for 1,000 executions each in four
and five seconds, reaching coverage 6190 and 6190 at 558 MB peak RSS and adding
5 and 7 units. TSan replayed the sanitizer-expanded corpora for 1,000
executions each, reaching coverage 1112 at 558 MB peak RSS and adding 9 and 8
units. All six workers exited zero without an assertion, sanitizer report,
race/deadlock report, timeout, or artifact.

The target exercised connected and disconnected dependency graphs, supplied
and absent old linearizations, topological and non-topological input claims,
bounded and optimal work budgets, fallback ordering, deterministic reruns, and
the trained cost model's optimality and chunk-order contracts. Each process
owns an independent graph and runs sequentially, so TSan does not prove
arbitrary live cluster-mempool interleavings are race-free. No production
inconsistency or clean-master failure was found, so no source fix or
deterministic unit-test addition was warranted.

## Post-rebase bounded coins-cache simulation gate (2026-07-23)

`coinscache_sim` was rerun from 1,501 of its 1,515 QA inputs below 64 KiB. The
only excluded input was the previously identified slow workload
`8bdf52da47cc919025ba1b2a544f001f69780649` (13,298 bytes, SHA256
`b9d7bbc50d256d14256cb38cec34bd25d2501d91a6b466bda2c0b3a0033e2871`), whose
individual ASan/UBSan and TSan replays were already clean. Excluding that seed
allowed the remaining cache state space to complete without conflating
sanitizer overhead with a production timeout.

Normal workers completed 3,000 executions each in 427 and 437 seconds, reached
coverage 4149 and 4150, added 73 and 70 units, and used 558 MB peak RSS. The
expanded corpora were replayed by ASan/UBSan workers for 1,573 and 1,576
executions, reaching coverage 4228 and 4226 at 561 MB peak RSS. TSan replayed
the same expanded corpora for 1,572 and 1,575 executions, reaching coverage
1808 and 1809 at 558 MB peak RSS. All six workers exited zero without an
assertion, sanitizer report, race/deadlock report, timeout, or artifact.

The simulation exercised cache-stack creation and destruction, coin addition
and spending, overwrite rules, unspendable outputs, flush and reset guards,
best-block inheritance, `CoinsViewOverlay::StartFetching`, asynchronous
prevout fetching through the persistent thread pool, and final cache/memory
sanity checks. Each process owns an independent cache hierarchy; TSan covers
the target's asynchronous fetch work but does not prove arbitrary shared live
node cache interleavings are race-free. No production inconsistency or
clean-master failure was found, so no source fix or deterministic unit-test
addition was warranted.

## Post-rebase IPC round-trip gate (2026-07-23)

The new `ipc` fuzzer was built with Clang 19, `BUILD_FOR_FUZZING=ON`, and
`ENABLE_IPC=ON` using the repository-pinned Cap'n Proto 1.5.0 package. It starts
a real multiprocess-proxy event-loop thread and exercises integer addition,
`COutPoint`, byte-vector reversal, script transport, typed and raw `UniValue`
JSON transport, and transaction round trips. Invalid raw JSON is required to
return the expected `Invalid UniValue JSON received over IPC` error. The
harness catches only its local `RawIpcError` for that expected failure; event
loop errors and all other exceptions remain fatal, so this gate does not turn
unexpected IPC failures into passing fuzz cases.

With empty initial seeds, two normal workers completed 5,000 executions each
in about three seconds, reached coverage 2299 and 1792, added 146 and 153
units, and used 558 MB peak RSS. Their corpora were replayed and mutated by
two ASan/UBSan workers for 2,000 executions each; those workers reached
coverage 6107 and 4919, added 75 and 37 units, and used 556 MB peak RSS. The
ASan-expanded corpora were then replayed and mutated by two TSan workers for
2,000 executions each; those workers reached coverage 2392 and 1815, added
29 and 23 units, and used 559 MB peak RSS. All 18,000 fuzzer executions
completed without an assertion, sanitizer report, race/deadlock report,
timeout, or artifact.

The event-loop thread is joined during teardown, and each fuzzer process owns
an independent IPC graph and proxy pair. The normal, ASan/UBSan, and TSan
results therefore cover the exercised request/response lifecycle and teardown
interleavings, but do not prove arbitrary live multiprocess IPC schedules are
race-free. No production inconsistency or clean-master failure was found, so
no source fix or deterministic unit-test addition was warranted.

## Post-rebase SOCKS5 transcript gate (2026-07-24)

The branch-only `socks5` harness was expanded to exercise the complete scripted
SOCKS5 client transcript: no-auth and username/password method selection,
method-selection failures, authentication failures with unread sentinel bytes,
connection failures for bad versions/reply codes/reserved bytes/address types,
successful connections, overlong destinations, and overlong credentials. The
optional `FUZZED_SOCKET_FAKE_LATENCY=1` mode also drives the receive-timeout
path. Assertions compare every request byte, require the expected boolean result,
and verify that failure replies leave the sentinel unread. The target does not
catch arbitrary exceptions; unexpected assertions or exceptions remain fatal.

On the branch, two normal workers ran 5,000 executions each from all 1,268 QA
inputs (5.1 MB total, maximum input 3,608 bytes), reaching coverage 1,481.
ASan/UBSan then ran two workers for 5,000 executions each over the expanded
corpora, reaching coverage 3,841. TSan ran two workers for 5,000 executions
each, reaching coverage 577. The branch fake-latency mode ran two workers for
2,000 executions each and reached coverage 1,501. All eight workers exited zero
without an assertion, sanitizer report, race/deadlock report, timeout, or
artifact.

Because the branch changes `netbase.cpp` as well as the fuzzer, those results
were not treated as production evidence. A disposable worktree at the exact
clean-master baseline `a2e074d66ac17ca7907909bbbb563e77185a45e5` received only
the branch `src/test/fuzz/socks5.cpp`; the sole adjustment removed the
branch-only `ResetFuzzedSockMockedFds()` helper call, which does not exist on
that baseline. The clean production sources and tests were otherwise untouched.
The unmodified master fuzzer first passed two 5,000-run workers. The
transplanted branch fuzzer then passed two normal 5,000-run workers (coverage
549/550), two ASan/UBSan 5,000-run workers (coverage 1,078/1,078), two TSan
5,000-run workers (coverage 552/552), and two fake-latency 2,000-run workers
(coverage 553/554). The clean controls used the branch-expanded 1,299/1,305
input corpora and produced no artifact or diagnostic.

This is strong evidence that the new transcript mutations do not expose a
current-master SOCKS5 production failure or race. It is not evidence that all
live socket schedules are race-free: each fuzzer process owns an independent
scripted socket. No production fix, deterministic unit-test change, or severity
change was warranted.

## Post-rebase cluster-mempool corpus matrix (2026-07-23)

The current `tx_pool` and `tx_pool_standard` harnesses were replayed from fresh
copies of the existing QA corpora with two independent workers per flavor. These
targets exercise cluster topology and fee diagrams, ancestor/descendant repair,
RBF/TRUC policy, block-builder chunk iteration, prioritisation, mempool views,
standardness gating, validation callbacks, and randomized/witness indexes. The
fuzzer processes own independent mempools; TSan therefore covers the target's
exercised internal worker/callback interleavings, not arbitrary live-node sharing.

The unrestricted `tx_pool` corpus contained 8,000 inputs. Normal workers completed
8,001 executions each (coverage 20,284 and 20,285); TSan workers completed 8,001
each (coverage 10,039 in both runs). All four completed jobs exited zero without
an assertion, sanitizer report, race/deadlock report, timeout, or target artifact.

The `tx_pool_standard` corpus contained 2,857 inputs. Normal workers completed
2,858 executions each (coverage 19,748 and 19,750); TSan workers completed 2,858
each (coverage 8,943 and 8,942). Both standard-policy TSan jobs and both normal
jobs exited zero without a report or artifact.

Full ASan/UBSan replays were attempted from two independent copies of each corpus.
The unrestricted workers reached the 4,096-input pulse before a controlled stop;
the standard-policy workers reached 1,024 before the same large-input region held
both workers. Neither attempt emitted a target sanitizer diagnostic or crash
artifact, but both are explicitly incomplete gates. A manually interrupted
multi-worker libFuzzer run also printed a stack in libFuzzer's own
`InterruptExitCode()`; it had no Bitcoin frame and is excluded from production
evidence.

To obtain completed sanitizer evidence, a deterministic spread slice of 512 QA
inputs was replayed under two independent ASan/UBSan workers for each target. The
unrestricted slice completed 515 executions per worker, with coverage 66,760 and
66,754. The standard-policy slice completed 514 executions per worker, with
coverage 64,419 and 64,427. No assertion, sanitizer report, race report, or target
artifact occurred. The standard slice's only slow-unit marker was a 207,014-byte
input (SHA256
`e4cb432a1c9a8edad4159b07ec746bc34737821d911a42bb8ffa07335e4d1ad1`); its exact
normal-binary replay exited zero in about four seconds, so it is sanitizer/model
overhead rather than a demonstrated production performance issue.

This matrix found no new production inconsistency, race, memory-safety failure, or
policy defect. At the time of this matrix the confirmed-runtime-defect count was
twelve; the later descriptor-index persistence defect is recorded above.

### Re-evaluation after the compact-block collision work

No severity changes are warranted on the clean baseline. The compact-block
short-ID underflow is already fixed by master `6aa5d8d948` (PR #35727), and the
normal null-tail construction is avoided by master `6f1c56f03a` (PR #35670). The
remaining null, oversized-position, sparse-block, and reusable-`FillBlock` cases
are direct-API contracts covered by branch assertions/tests; no clean-master P2P
caller or new collision race was found. The thirty-nine confirmed production defects
listed above remain the only confirmed runtime defects in this ledger, ordered by
the severity assigned against current master. The post-collision exact-master
controls subsequently added five confirmed defects: duplicate snapshot records,
failed candidate-set transitions, pruned unlinked-chain accounting, post-complete
checkqueue work, and stale reusable sighash state. The later TxGraph control added
the equal-feerate prefix fee overflow and saturated cluster chunk fee aggregation
mismatch described above. The AddrMan and wallet
sanitizer gates above found no additional production mistake, race, consensus issue,
or remotely reachable memory-safety defect.

The additional `package_rbf` and `clusterlin_linearize` ASan workers were stopped
after more than five minutes on expensive corpus cases; they emitted no sanitizer
marker or target artifact, but are not counted as completed corpus gates.

Scratch data from this round was removed after the artifact and sanitizer scans; the
shared QA corpora and ccache were preserved. This file should be amended if a later
master rebase changes the status of any item above.

## Current-master cluster linearization backend control (2026-07-24)

The current master production delta includes merge `22a03ca694` (`clusterlin:
minor SFL optimizations`). The audit checked its cached dependency-count arrays,
reserved suboptimal-chunk storage, and fixed-size ready/dependency queues against
the `DepGraph` size invariants. Production `TxGraph` uses `BitSet<64>` while the
ordinary cluster fuzzer helpers use `TestBitSet = BitSet<32>`. The separate
`clusterlin_backend_equivalence` oracle compares the production-sized graph
against the other bitset backends, but had no preserved QA corpus; that absence
was a coverage omission, not a production failure.

The ordinary targets were replayed from private copies of the existing QA
corpora. On the branch, `clusterlin_sfl` completed 3,000 normal executions and
the direct-file TSan replay covered all 895 inputs; its ASan/UBSan run reached
2,668 executions before the 180-second bound without a diagnostic and is
explicitly incomplete. `clusterlin_postlinearize` completed 3,000 normal and
2,000 ASan/UBSan executions, with a clean 1,720-file TSan replay. The expensive
`clusterlin_linearize` target reached 2,497 normal and 508 ASan/UBSan executions
before their 180-second bounds, with no diagnostic; its 672-file TSan replay
completed. The incomplete runs are not counted as completed corpus gates.

The backend oracle was exercised with a generated 273-byte seed containing 64
transactions in one parent-child chain, filling the production bitset and
driving the cached dependency counts through the full graph. The branch completed
2,000 normal and 1,000 ASan/UBSan executions, plus a TSan replay; the final
coverage values were 4,750, 15,392, and a clean one-file TSan result. A disposable
worktree at the prior exact master `afa5e46bbc6d` received only the fuzzer oracle
change `e7f651f1b5` so that the production source stayed at master. The later
`afa5e46bbc6d..610dd320d1` range only removes a testnet3 seed. Its normal
control completed 2,000 executions (coverage 4,402), and a separately configured
ASan/UBSan build completed 1,000 executions (coverage 14,333). No run produced
an assertion failure, sanitizer report, race/deadlock report, backend mismatch,
timeout, or artifact.

An earlier control invocation used a libFuzzer-only master build with ASan
environment variables set; because that binary was not instrumented, it is not
counted as sanitizer evidence. The real exact-master sanitizer run above closes
that bookkeeping gap. No clean-master production inconsistency, vulnerability,
race, or deterministic test omission was demonstrated, so no production fix or
unit-test change was warranted.

## Current-master validation-load-mempool replay (2026-07-23)

The rebased `validation_load_mempool` target was replayed from a private copy of
all 1,799 QA inputs (132,748,569 bytes total; maximum input 1,048,229 bytes).
The corpus covers current and v1 dump formats, malformed records, metadata option
combinations, disabled metadata import, trailing parse failures, and atomic dump
failure paths. The fuzzer treats only the target's expected `std::ios_base::failure`
as a load error; unexpected exceptions, assertions, and state-contract failures
remain fatal.

Two normal workers completed 5,000 executions each, reaching coverage 8,346 with
peak RSS of 211 and 209 MB. TSan workers replayed all 1,799 seeds and completed
1,800 executions each, reaching coverage 2,983 and 2,996 with peak RSS of 389 MB.
For memory instrumentation, two ASan/UBSan workers replayed an evenly selected
449-input slice and completed 512 executions each, reaching coverage 24,551 and
24,554 with peak RSS of 827 and 796 MB. All six workers exited zero without an
assertion, sanitizer report, race/deadlock report, timeout, or artifact.

Each fuzzer process owns its own mempool and dump files. TSan therefore covers the
target's exercised thread interactions, but does not prove arbitrary concurrent
live-node persistence or mempool schedules race-free. This gate found no new
production inconsistency, exception-classification error, race, or clean-master
failure; no source or deterministic test change was warranted.

## Post-fix validation-block-reorg sanitizer gate (2026-07-23)

After the `ProcessNewBlock` `new_block` output fix in `0b1bbf527c`, the
`validation_block_reorg` target was replayed from a private copy of all 2,257 QA
inputs (62,359 bytes total; input sizes from 1 to 74 bytes). The replay was run
again against the rebased branch so that the new postcondition did not merely
exist in a deterministic test.

Two normal workers completed 5,000 executions each, both reaching coverage
13,866, with peak RSS of 110 and 112 MB. Two TSan workers completed 5,000 each,
reaching coverage 5,279, with peak RSS of 310 and 315 MB. Two ASan/UBSan workers
also completed 5,000 each, reaching coverage 42,908 and 42,927, with peak RSS of
661 and 654 MB. All six workers exited zero without an assertion, sanitizer or
race report, timeout, or target artifact.

This is post-fix evidence for the stale `new_block` output contract described
above, not an independent discovery of a new reorg defect. The workers used
independent fuzzer processes and therefore do not establish that arbitrary live
node reorg schedules are race-free. At the time of this gate the confirmed-runtime-
defect count was twelve; the later descriptor-index persistence defect is recorded
above, and no further change was warranted by this gate.

## Current-master block-index/reorg state gate (2026-07-23)

The `block_index_tree` target was replayed from two independent copies of all
2,381 QA inputs (2,019,276 bytes total; maximum input 26,372 bytes). Its model
deliberately covers invalidation and reconsideration of genesis and same-height
forks, invalid descendants, candidate-set maintenance, arbitrary pruning and
redownload bookkeeping, and partial reorg progress.

Two normal workers completed 5,000 executions each, reaching coverage 2,900,
with peak RSS of 101 and 105 MB. Two TSan workers completed 5,000 each,
reaching coverage 1,111 and 1,112, with peak RSS of 315 and 311 MB. Two
ASan/UBSan workers completed 5,000 each, reaching coverage 8,983 and 8,984,
with peak RSS of 746 and 748 MB. All six workers exited zero without an
assertion, sanitizer or race report, timeout, or target artifact.

The model explicitly stops a run when a simulated reorg would need missing undo
data; this mirrors the production `ActivateBestChain` failure path rather than
exercising it through a real pruned node. Consequently this gate is evidence for
the exercised block-index invariants only, not proof that arbitrary deep pruned
reorgs are recoverable or that live-node reorg schedules are race-free. No new
clean-master production inconsistency was demonstrated, so no source or
deterministic test change was warranted.

## Current-master block-filter/index gate (2026-07-23)

The current `blockfilter` target and the stateful `blockfilter_index_tests` suite
were checked after the rebase onto `7b6f9ba7bad13b0c4169259000f7802854cdda0d`.
The parser corpus contained 734 inputs totaling 46,256,258 bytes, with a maximum
input size of 1,007,040 bytes. Fuzzer workers used independent private copies of
the corpus so libFuzzer corpus updates could not race another worker or alter the
preserved QA corpus.

Two normal libFuzzer workers completed 5,000 executions each, reaching coverage
3,199 with peak RSS of 577 MB; the workers took 184 and 155 seconds. Two TSan
file-driver workers replayed all 734 inputs each and completed in 60 seconds,
with no TSan report. Two ASan/UBSan libFuzzer workers completed 5,000 executions
each, reaching coverage 9,369 and 9,375, with peak RSS of 819 and 795 MB; they
took 667 and 886 seconds. All six valid workers exited zero without an assertion,
sanitizer report, race report, timeout, or production artifact. The ASan/UBSan
run wrote only the existing slow-unit timing artifacts for large filter inputs;
they were removed after the replay and did not identify a correctness failure.

The normal and TSan `blockfilter_index_tests/*` runs each passed all five cases:
initial sync, initialization/destruction, null block-transaction-reference
rejection, index reinitialization concurrent with a reader, and the deliberately
blocked reorg schedule. The normal suite took 1.35 seconds and the TSan suite
4.52 seconds. This covers the index worker's tested persistence and reorg
interleavings in addition to the parser fuzzer, but the parser target itself is
single-process and the index tests exercise only their explicit schedules; this
does not prove arbitrary live-node index or reorg concurrency race-free.

The first attempted TSan commands were excluded from the gate because the TSan
build uses Bitcoin Core's file-list driver: unlike the libFuzzer build, it treats
`-runs=5000` as an input path and correctly failed its harness assertion at
`src/test/fuzz/fuzz.cpp:268`. The corrected TSan replay passed with corpus
directories only. No clean-master production inconsistency or new block-filter
bug was demonstrated, so no source or deterministic test change was warranted.

## Resumed stacked coins-cache sanitizer gate (2026-07-23)

The `coins_view_stacked` target was replayed from two private copies of 256
existing `coins_view` inputs below 64 KiB. This specifically exercises a
`CCoinsViewDB` backend beneath a cache and a parallel `CoinsViewOverlay`,
including reset, flush, backend changes, dirty/fresh transitions, and the
thread-pool prevout fetch path.

Two independent ASan/UBSan workers completed 1,000 mutations each in 21 and 18
seconds, reaching coverage 30,495 and 30,395 with peak RSS of 428 and 432 MB.
The private corpora grew to 397 and 387 files. Replaying those expanded copies
with the direct-file TSan driver completed all 397 and 387 inputs in one second
per worker. Every worker exited zero without an assertion, sanitizer report,
race report, timeout, or target artifact.

This is a completed bounded sanitizer gate for the stacked cache and overlay
worker transitions, not a replay of the full `coins_view` corpus. The workers
own independent cache/database instances, so TSan covers the target's worker
interleavings rather than arbitrary shared live-node schedules. No clean-master
production inconsistency or race was demonstrated; no source or deterministic
test change was warranted.

## Resumed ephemeral package-evaluation sanitizer gate (2026-07-23)

The `ephemeral_package_eval` target was replayed from two private copies of 128
existing inputs below 16 KiB. The target builds ephemeral parent/child packages,
exercises package test-accept and single-transaction submission, double-spend
eviction, prioritisation, validation callbacks, and the mempool input/index
invariants.

Two independent ASan/UBSan workers completed 256 mutations each in 580 and 625
seconds, reaching coverage 58,823 and 58,849 with peak RSS of 521 and 525 MB.
Their private corpora expanded to 238 and 227 files. The rebuilt direct-file
TSan driver replayed those expanded corpora in 55 seconds per worker. All four
workers exited zero without an assertion, sanitizer report, race report,
timeout, or target artifact.

This is a completed bounded sanitizer gate, not a full replay of the 2,098-input
corpus. Each process owns an independent mempool and validation state, so TSan
covers the target's callback and worker interactions rather than arbitrary
shared live-node schedules. No clean-master production inconsistency, policy
failure, or race was demonstrated; no source or deterministic test change was
warranted.

## Resumed UTXO total-supply sanitizer gate (2026-07-23)

The `utxo_total_supply` target was replayed from two independent private copies
of 64 existing inputs below 64 KiB. The target constructs a regtest chainstate
with an early BIP34 activation, mutates block/transaction encodings including
duplicate-coinbase candidates, and checks circulation, total supply, output
counts, invalid-block non-mutation, and cache wipe/reload invariants. The
initial seed corpus was 64 files, 16,779 bytes total, with a maximum input size
of 665 bytes.

Two ASan/UBSan libFuzzer workers completed 256 mutations each in 821 and 880
seconds, reaching coverage 72,315 and 70,428 with peak RSS of 686 and 650 MB.
Their private corpora expanded to 209 and 219 files. The first worker wrote
only a `slow-unit-*` file, which is libFuzzer timing metadata for a slow input,
not a crash or correctness artifact; neither worker produced an error report.
The rebuilt direct-file TSan driver replayed all 209 and 219 expanded inputs in
512 and 526 seconds. All four workers exited zero without an assertion,
sanitizer report, race report, timeout, or production artifact.

This is a completed bounded sanitizer gate, not a replay of the full
`utxo_total_supply` corpus. Each process owns an independent chainstate and
database, so TSan covers the target's local cache/database transitions rather
than arbitrary shared live-node schedules. No clean-master production
inconsistency, supply-calculation failure, or race was demonstrated; no source
or deterministic test change was warranted.

## Resumed transaction-download manager wrapper gate (2026-07-23)

The `txdownloadman` wrapper target was replayed from two independent private
copies of 92 existing inputs below 16 KiB. This complements the existing
`txdownloadman_impl` reference-model gate by exercising the public wrapper over
precomputed conflicting transactions and parent/child chains. The mutations
cover peer connect/disconnect, active-tip and block connect/disconnect events,
mempool acceptance/rejection, tx announcements and requests, received and
missing transactions, package validation, reconsideration work, backward and
forward time jumps, and final per-peer/global emptiness checks.

Two normal workers completed 512 mutations each in 59 and 60 seconds, expanding
their private corpora to 281 and 264 files without an assertion or artifact.
Two ASan/UBSan workers then completed 512 mutations each in 264 and 311
seconds. Both reached coverage 23,033; peak RSS was 696 and 715 MB, and their
corpora grew to 354 and 339 files. The direct-file TSan driver replayed all 354
and 339 expanded inputs in 18 and 19 seconds. All six workers exited zero
without an assertion, sanitizer report, race report, timeout, or target
artifact.

This is a completed bounded wrapper gate, not a full replay of the 1,500-input
`txdownloadman` corpus. Each process owns an independent download manager,
mempool, and peer state, so TSan covers the target's exercised lifecycle
transitions rather than arbitrary shared live-node schedules. No clean-master
production inconsistency, request-state failure, or race was demonstrated; no
source or deterministic test change was warranted.

## Resumed simple cluster-linearization and tree sanitizer gates (2026-07-23)

The `clusterlin_simple_finder`, `clusterlin_simple_linearize`, and
`clusterlin_postlinearize_tree` targets were replayed from private copies of
their existing QA corpora. The simple finder compares the candidate search
against exhaustive results on small graphs and checks topological validity,
connectedness, iteration bounds, and fee-rate ordering. The simple linearizer
checks its claimed optimality against exhaustive linearizations and arbitrary
valid linearizations. The tree target checks that post-linearization improves
or preserves the fee diagram, is idempotent, and agrees with a high-budget
`Linearize` call on upright and reverse trees.

The simple finder normal worker completed 3,000 mutations in 241 seconds at
coverage 723 and 56 MB peak RSS. Its libFuzzer live reduced corpus ended at
201 inputs; the on-disk private corpus contained 263 inputs after newly found
cases were retained. The simple linearizer normal worker completed 3,000
mutations in 44 seconds at coverage 1,047 and 57 MB peak RSS. Its live reduced
corpus ended at 301 inputs. The ASan/UBSan linearizer worker completed 3,000
mutations in 221 seconds at coverage 3,121 and 671 MB peak RSS, with a live
corpus of 305 inputs.

The initial simple-finder ASan/UBSan run used the usual 30-second per-input
bound and stopped on a timeout, not a sanitizer finding. The 110-byte seed
`db14ffffe86f4bf4ca4a60b8432653e31aa61263` took 47.884 seconds in the branch
ASan build and its stack was in `DepGraph::FeeRate` called by the fuzzer's
`SimpleCandidateFinder` oracle. The same seed took 12.362 seconds in the
normal branch build and 16.411 seconds in an independently built ASan/UBSan
fuzzer at the then-current master `51143291a6`; neither run reported an error.
The branch-only slowdown comes from the additional assertion and oracle work
on this branch, so this is not a production-master performance or memory bug.
An extended 120-second-bound replay was stopped with SIGUSR1 after 264
executions because the retained finder seeds made corpus startup excessively
slow. It produced only three `slow-unit-*` timing files and no sanitizer
report. This means the finder has a completed normal and direct-file TSan
gate, but not a claimed full 3,000-input ASan gate; the timeout is recorded as
an instrumentation limitation rather than silently treated as a pass.

Direct-file TSan replay succeeded for the exercised private corpora: the
finder replayed 263 files in 9 seconds and the simple linearizer replayed 439
files in 2 seconds. The tree target's normal worker completed 3,000 mutations
in one second at coverage 1,809 and 58 MB peak RSS. Its ASan/UBSan worker
completed 3,000 mutations in 9 seconds at coverage 5,522 and 363 MB peak RSS;
the direct-file TSan driver replayed 885 files in one second. All completed
jobs exited without an assertion, sanitizer report, race report, or target
artifact. No clean-master production inconsistency, optimality failure,
linearization failure, or race was demonstrated; no source or deterministic
test change was warranted.

## Resumed public orphanage sanitizer gate (2026-07-23)

The `txorphan` target was replayed from two independent private copies of 64
existing inputs below 16 KiB. The selected seeds totaled 98,155 bytes with a
maximum input size of 12,231 bytes. This bounded slice preserves the target's
transaction construction, duplicate txid/wtxid, peer-announcement, protected
orphan, eviction, and reconsideration paths while excluding the oversized
synthetic inputs that previously limited the full ASan run.

Two normal workers completed 512 mutations each in 5 and 4 seconds, expanding
their private corpora to 174 and 161 files. Two ASan/UBSan workers then
completed 512 mutations each in 33 and 26 seconds, reaching coverage 11,400
and 11,402 with peak RSS of 313 and 310 MB; their corpora expanded to 230 and
210 files. The direct-file TSan driver replayed all 230 and 210 expanded inputs
in one second per worker. All six workers exited zero without an assertion,
sanitizer report, race report, timeout, or target artifact.

This is a completed bounded public-orphanage gate, not a full replay of the
686-input `txorphan` corpus. Each process owns an independent orphanage and
peer state, so TSan covers the target's exercised transitions rather than
arbitrary shared live-node schedules. No clean-master production
inconsistency, eviction failure, or race was demonstrated; no source or
deterministic test change was warranted.

## Resumed transaction-request sanitizer gate (2026-07-23)

The `txrequest` target was replayed from two independent private copies of 62
existing inputs below 16 KiB. The selected seeds totaled 158,365 bytes with a
maximum input size of 15,165 bytes. Its reference model and implementation
were exercised across txid/wtxid announcements, preferred-peer ordering,
request expiry, duplicate announcements, responses, forgotten hashes, peer
disconnects, and single-peer/all-peer request queries, including forward and
backward time jumps.

Two normal workers completed 512 mutations each in 41 and 45 seconds,
expanding their private corpora to 312 and 316 files. Two ASan/UBSan workers
then completed 512 mutations each in 189 and 205 seconds, reaching coverage
8,056 in both runs with peak RSS of 721 and 723 MB; their corpora expanded to
374 and 396 files. The direct-file TSan driver replayed all 374 and 396 inputs
in 10 and 11 seconds. All six workers exited zero without an assertion,
sanitizer report, race report, timeout, or target artifact.

This is a completed bounded request-state gate, not a full replay of the
809-input `txrequest` corpus. Each process owns an independent tracker, so
TSan covers the target's exercised state transitions rather than arbitrary
shared live-node schedules. No clean-master production inconsistency, request
selection failure, or race was demonstrated; no source or deterministic test
change was warranted.

## Resumed DepGraph simulation sanitizer gate (2026-07-23)

The `clusterlin_depgraph_sim` target was replayed from two independent private
copies of all 1,666 existing inputs. Its reference model exercises dependency
insertion, arbitrary dependency subsets, transaction removal, topological
append, compaction, ancestor propagation, dependency counts, and the final
serialized graph sanity check beneath cluster linearization.

Two normal workers completed 3,000 and 3,375 total executions in 14 and 20
seconds, reaching coverage 1,166 with peak RSS of 58 MB in both runs. Their
private corpora expanded to 1,671 and 1,668 files. Two ASan/UBSan workers then
completed 3,000 mutations each in 65 and 68 seconds, reaching coverage 3,249
with peak RSS of 510 and 523 MB; their corpora expanded to 1,674 and 1,672
files. The direct-file TSan driver replayed all 1,674 and 1,672 inputs in one
second per worker. All six workers exited zero without an assertion, sanitizer
report, race report, timeout, or target artifact.

This is a completed full-seed graph simulation gate. Each process owns an
independent `DepGraph`, so TSan covers the target's exercised operations rather
than arbitrary shared cluster-mempool schedules. No clean-master production
inconsistency, graph-model failure, or race was demonstrated; no source or
deterministic test change was warranted.

## Post-rebase cluster chunking and SFL sanitizer gates (2026-07-23)

The `clusterlin_chunking` and `clusterlin_sfl` targets were replayed from two
independent private copies of their existing QA corpora. `clusterlin_chunking`
started from 572 inputs and checks fee-diagram partition consistency,
monotonic chunk ordering, contiguous coverage, equal-rate boundaries, and the
checked comparable-chunk overflow paths. `clusterlin_sfl` started from 895
inputs and exercises spanning-forest state transitions, connected and
disconnected graphs, loaded linearizations, optimal/minimal steps, and
deterministic reruns.

Normal workers completed 2,000 mutations per target and worker. Chunking
finished in under one second per worker at coverage 935; SFL finished in 34
and 37 seconds at coverage 2,058. The normal corpora expanded to 575/572 and
897/898 files. ASan/UBSan workers completed 2,000 mutations per target and
worker: chunking finished in 3 seconds at coverage 2,885 with peak RSS of
276/275 MB, while SFL finished in 144/160 seconds at coverage 6,114 with peak
RSS of 606/604 MB. The sanitizer corpora expanded to chunking 578/574 and SFL
902/900 files.

Direct-file TSan replay succeeded for all four expanded corpora: chunking
completed 578/574 inputs in one second per worker and SFL completed 902/900 in
four seconds per worker. All twelve normal and sanitizer jobs exited zero
without an assertion, sanitizer report, race report, timeout, or target
artifact.

These are completed corpus-backed gates for the two algorithm targets. Each
process owns an independent graph and runs sequentially, so TSan covers the
target's exercised operations rather than arbitrary shared cluster-mempool
schedules. No clean-master production inconsistency, fee-diagram failure, or
race was demonstrated; no source or deterministic test change was warranted.

## Resumed DepGraph serialization sanitizer gate (2026-07-23)

The `clusterlin_depgraph_serialization` target was replayed from two
independent private copies of all 387 existing inputs. It deserializes
dependency graphs with the target's expected parse-failure classification,
checks graph sanity and acyclicity, round-trips the graph representation, and
then deliberately introduces a cycle to verify rejection.

Two normal workers completed 3,000 mutations each in one second, reaching
coverage 843 with peak RSS of 56 and 57 MB; their private corpora expanded to
389 files each. Two ASan/UBSan workers completed 3,000 mutations each in five
seconds, reaching coverage 2,432 with peak RSS of 286 MB in both runs; their
corpora expanded to 392 and 391 files. The direct-file TSan driver replayed all
392 and 391 inputs in under one second per worker. All six workers exited zero
without an assertion, sanitizer report, race report, timeout, or target
artifact.

This is a completed full-seed serialization-boundary gate. Each process owns an
independent graph, so TSan covers the target's local serialization and cycle
checks rather than arbitrary shared cluster-mempool schedules. No clean-master
production inconsistency, parser classification failure, or race was
demonstrated; no source or deterministic test change was warranted.

## Resumed cluster component and graph-repair sanitizer gates (2026-07-23)

The `clusterlin_components` and `clusterlin_make_connected` targets were
replayed from two independent private copies of their existing QA corpora.
Components started from 326 inputs and checks connected-component discovery,
subset containment, ancestor/descendant closure, and reachability. MakeConnected
started from 360 inputs and checks that graph repair produces a sane connected
dependency graph.

Normal workers completed 3,000 mutations per target and worker in about one
second. Components reached coverage 829 with peak RSS of 56/55 MB and expanded
to 330/331 files; MakeConnected reached coverage 835 with peak RSS of 56/58 MB
and expanded to 362/360 files. ASan/UBSan workers completed 3,000 mutations
per target and worker: components reached coverage 2,443 in 9 seconds at
286/284 MB RSS and expanded to 337/341 files; MakeConnected reached coverage
2,418 in 8 seconds at 287 MB RSS and expanded to 364/363 files.

The direct-file TSan driver replayed all four expanded corpora successfully:
components completed 337/341 inputs in one second per worker, and MakeConnected
completed 364/363 inputs in one second or less. All twelve normal and sanitizer
jobs exited zero without an assertion, sanitizer report, race report, timeout,
or target artifact.

These are completed corpus-backed graph-structure gates. Each process owns an
independent graph, so TSan covers the exercised component and repair operations
rather than arbitrary shared cluster-mempool schedules. No clean-master
production inconsistency, connectivity failure, or race was demonstrated; no
source or deterministic test change was warranted.

## Resumed orphanage simulation sanitizer gate (2026-07-23)

The `txorphanage_sim` target was replayed from two independent private copies
of all 1,346 existing QA inputs. It constructs up to 16 transactions with
topologies, duplicate txids and distinct wtxids, and up to 16 peers. The real
`TxOrphanage` is compared with a naive announcement model across transaction
and peer erasure, block erasure, child-workset reconsideration, peer eviction,
announcement ordering, and final usage/count/latency inspectors.

Two normal workers completed 3,000 mutations each in 50 seconds, reaching
coverage 4,893 with peak RSS of 67 and 63 MB; their live reduced corpora ended
at 983 and 982 inputs. Two ASan/UBSan workers completed 3,000 mutations each in
331 and 327 seconds, reaching coverage 15,560 and 15,567 with peak RSS of 671
and 674 MB; their live reduced corpora ended at 999 and 1,006 inputs. The
direct-file TSan driver replayed the expanded private corpora of 1,352 and
1,359 files in four seconds per worker. All six jobs exited zero without an
assertion, sanitizer report, race/deadlock report, timeout, exception, or
target artifact.

Each fuzzer process owns an independent orphanage and executes its state model
sequentially. The TSan result therefore covers the target's exercised
internal operations and sanitizer-instrumented setup, not arbitrary shared
live-node peer schedules. No clean-master candidate, production inconsistency,
or race was demonstrated; no source or deterministic test change was
warranted.

## Resumed sighash and script-interpreter oracle gates (2026-07-23)

The `sighash_cache` target was replayed from two independent private copies of
all 585 existing inputs, with a maximum input size of 621,249 bytes. The
additional mutations exercise cache aliases, script-code mismatches, witness
mode mismatches, transaction reuse, and all 100 hash-type combinations. The
target also checks the witness-v0 hash after reusing precomputed transaction
data across transactions.

Two normal workers completed 5,000 mutations each in 86 and 126 seconds,
reaching coverage 1,377 with peak RSS of 112 and 110 MB; their live reduced
corpora ended at 416 and 400 inputs. Two ASan/UBSan workers completed 3,000
mutations each in 254 and 451 seconds, reaching coverage 3,832 in both runs
with peak RSS of 651 and 681 MB; their live reduced corpora ended at 408 and
417 inputs. The direct-file TSan driver replayed the expanded private corpora
of 733 and 755 files in 15 and 17 seconds. The only log records were two
slow-unit timing markers (13 and 21 seconds); there was no assertion,
sanitizer report, race report, timeout, exception, or target artifact.

The stale-precomputation defect already recorded in finding 31 remains the
known production issue covered by this target; it was fixed separately and no
additional stale-state or cache-alias inconsistency was found. Because this
round produced no candidate failure, an exact clean-master replay was not
needed to classify a new defect. The independent-process TSan replay covers
the target's exercised cache operations, not arbitrary concurrent live-node
schedules. No source or deterministic test change was warranted.

The sibling `script_interpreter` target was replayed from two independent
private copies of a bounded 473-input slice of its 622-input corpus. The
slice contains inputs below 64 KiB (maximum 59,962 bytes) to avoid spending
the gate on oversized synthetic inputs while retaining the script, signature,
transaction, and `CastToBool` oracle paths. Two normal workers completed 5,000
mutations each in three seconds, reaching coverage 1,481 with peak RSS of 66
MB and live reduced corpora of 380 and 376 inputs. Two ASan/UBSan workers
completed 5,000 mutations each in 17 and 16 seconds, reaching coverage 4,277
with peak RSS of 507 and 520 MB and live reduced corpora of 375 and 382
inputs. The direct-file TSan driver replayed the expanded corpora of 534 and
523 files in one second per worker. All eight runs exited zero without a
`CastToBool` mismatch, assertion, sanitizer report, race report, timeout,
exception, or target artifact. No production or deterministic test change
was warranted.

Both targets use independent fuzzer processes, so these results establish
local oracle and sanitizer coverage rather than a proof about shared-node
concurrency. No new clean-master production inconsistency, vulnerability, or
race was demonstrated.

## Resumed standalone PSBT oracle gate (2026-07-23)

The standalone `psbt` target was replayed from two independent private copies
of all 7,773 existing QA inputs (37,114,823 bytes; maximum input 835,312
bytes). Its existing and branch-added contracts cover PSBT decoding and
serialization round trips, version handling, analysis-result consistency,
signature-data conversion, finalization and extraction, merge versus combine
equivalence, self-merge stability, unsigned-input accounting, and removal of
unnecessary transactions.

Two normal workers each completed 15,000 executions in 26 seconds, including
the full seed corpus followed by thousands of mutations. They reached
coverage 14,700 and 14,701 with peak RSS of 123 and 125 MB, added 23 and 28
units, and ended with private corpora of 7,795 and 7,801 files. Two
ASan/UBSan workers completed 11,000 executions each in 178 and 179 seconds
from those expanded corpora, reaching coverage 50,909 and 50,912 with peak
RSS of 790 and 778 MB and adding 5 and 7 units. The direct-file TSan driver
then replayed 7,795 and 7,801 files in 12 seconds per worker. All six runs
exited zero without an assertion, sanitizer report, race report, timeout,
unexpected exception, or target artifact.

The first seed-only launch was discarded because the 7,773-file corpus
consumed the nominal 5,000-run budget before mutation; the corrected 15,000
execution campaign is the gate recorded here. Each process owns an
independent PSBT state, so TSan covers the target's exercised operations and
setup rather than arbitrary concurrent RPC or wallet callers. No clean-master
candidate, production inconsistency, vulnerability, race, or deterministic
test gap was demonstrated; no source or test change was warranted.

## Resumed PSBT base64 decoder gate (2026-07-23)

The `psbt_base64_decode` target was replayed from two independent private
copies of all 2,102 existing QA inputs (46,847,634 bytes; maximum input
970,624 bytes). It accepts only valid base64 PSBTs, checks that decoding and
re-encoding preserve the trimmed input, serializes the decoded PSBT, and
checks that a second base64 decode round-trips the serialized PSBT exactly.

Two normal workers completed 8,000 executions each in 22 and 23 seconds,
reaching coverage 5,873 with peak RSS of 162 MB in both runs. They added 14
and 15 units and ended with 2,116-file private corpora. Two ASan/UBSan workers
completed 6,000 executions each in 74 and 76 seconds, reaching coverage
19,217 with peak RSS of 630 and 627 MB and adding 9 and 10 units. The
direct-file TSan driver replayed the resulting 2,125 and 2,126 files in 7
and 8 seconds. All six jobs exited zero without an assertion, sanitizer
report, race report, timeout, unexpected exception, or target artifact.

Each process owns independent decoded PSBT state, so the TSan replay covers
the decoder and round-trip operations exercised by the target rather than
concurrent callers sharing a PSBT. No clean-master candidate, production
inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed script-flag matrix gate (2026-07-23)

The `script_flags` target was replayed from two independent private copies of
its 2,579-input QA corpus (46,641,664 bytes, maximum input size 100,001
bytes). It deserializes transactions and spent outputs, exercises valid
script-flag combinations, compares `VerifyScript` with and without a
`ScriptError` output, and checks that removing flags from a passing execution
or adding flags to a failing execution preserves the result. Its only
exception catch remains the deserialization-specific `std::ios_base::failure`.

Two normal workers completed 10,000 executions each in 92 seconds, reaching
coverage 3,123 with peak RSS of 105 MB and adding 13 and 20 units; their
corpora expanded to 2,592 and 2,598 files. A full expanded ASan/UBSan replay
and a second replay restricted to inputs below 64 KiB were deliberately
stopped at the first 1,024-execution pulse after both workers encountered a
high-cost seed transition. Neither attempt emitted a sanitizer report,
assertion, timeout, or artifact; these are incomplete gates, not sanitizer
passes or failures.

For a completed bounded sanitizer gate, two workers replayed the complete
normal corpora restricted to 1,336 and 1,340 inputs below 4 KiB. ASan/UBSan
completed 5,000 executions each in 14 seconds, reaching coverage 11,740 and
11,744 with peak RSS of 426 and 418 MB and adding 51 and 42 units. The
direct-file TSan driver replayed the resulting 1,387 and 1,381 files in one
second per worker. All completed jobs exited zero without an assertion,
sanitizer report, race report, timeout, unexpected exception, or artifact.

The TSan workers own independent transactions, spent outputs, and checkers,
so the result covers the exercised verification paths rather than shared
concurrent node validation. No clean-master candidate, production
inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed eval-script contract gate (2026-07-23)

The `eval_script` target was replayed from two independent private copies of
its 1,876-input QA corpus (1,098,895 bytes in total, maximum input size
10,010 bytes). The target checks the `EvalScript` result and error contract
with and without an error pointer, compares the resulting stacks, and runs
the check for both `SigVersion::BASE` and `SigVersion::WITNESS_V0`.

Two normal workers completed 10,000 executions each in 10 and 11 seconds,
adding 72 and 80 units with peak RSS of 59 and 57 MB; their corpora expanded
to 1,944 and 1,956 files. Two ASan/UBSan workers completed 6,000 executions
each in 28 and 25 seconds, reaching coverage 3,761 with peak RSS of 640 and
619 MB and adding 42 and 27 units. The direct-file TSan driver replayed the
resulting 1,986 and 1,983 files in one second per worker. All six jobs exited
zero without an assertion, sanitizer report, race report, timeout,
unexpected exception, or target artifact.

Each process owns its script and execution-data state, so the TSan replay
covers the target's exercised interpreter calls rather than shared concurrent
validation schedules. No clean-master candidate, production inconsistency,
vulnerability, race, or deterministic test gap was demonstrated; no source
or test change was warranted.

## Resumed private-broadcast model gate (2026-07-23)

The model-level `private_broadcast` target has no native QA corpus. It was
seeded from 760 existing `txrequest` inputs below 64 KiB (3,701,794 bytes;
maximum 61,906 bytes), so the seed bytes are a useful mutation source but do
not represent a corpus collected from this target. The model covers queue
capacity, duplicate txids with distinct wtxids, best-peer selection, node
confirmation, disconnect and retry behavior, removal accounting,
unconfirmed-disconnected slots, stale expiry, and forward clock jumps.

Two normal workers completed 10,000 executions each in 634 and 644 seconds,
reaching coverage 3,009 with peak RSS of 65 MB in both runs. They added 177
and 174 units and ended with 929 and 923 private corpus files. A full
ASan/UBSan replay of those expanded corpora was deliberately stopped at the
512-input pulse after both workers became CPU-bound near 719 MB RSS; neither
produced a sanitizer report or artifact. A second ASan/UBSan attempt over
fresh units was likewise stopped in the same model-cost region. These are
incomplete full-corpus sanitizer attempts, not sanitizer failures.

For a bounded sanitizer gate, the fresh units were restricted to 138 and 147
inputs below 4 KiB. Two ASan/UBSan workers completed 512 executions each in
189 and 211 seconds, reaching coverage 9,180 with peak RSS of 733 and 735 MB;
they added 148 and 139 units and produced no assertion, sanitizer report,
timeout, or artifact. The direct-file TSan driver replayed the full 929 and
923 expanded normal corpora in 20 seconds per worker, also without a race
report or artifact.

The TSan workers use independent `PrivateBroadcast` instances, so this is
evidence for the model's exercised transitions and target setup rather than
arbitrary shared live-node scheduling. No clean-master candidate, new
production inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed script-parsing span gate (2026-07-23)

The `script_parsing` target was replayed from two independent private copies
of its 83-input QA corpus (407,692 bytes, maximum input size 137,885 bytes).
It checks `Const`, `Func`, and `Expr` span advancement, non-consuming failure
behavior, balanced-expression boundaries, and delimiter handling against
independent reference calculations.

Two normal workers completed 10,000 executions each, reaching coverage 384
with peak RSS of 57 MB and adding 34 and 49 units; their corpora expanded to
76 and 75 files. Two ASan/UBSan workers completed 5,000 each in 17 and 16
seconds, reaching coverage 936 with peak RSS of 516 and 512 MB and adding 10
and 9 units. The direct-file TSan driver replayed the resulting 121 and 131
files in under one second per worker. All six jobs exited zero without an
assertion, sanitizer report, race report, timeout, unexpected exception, or
target artifact.

The TSan workers own independent spans and strings, so this covers parser
state transitions rather than shared concurrent callers. No clean-master
candidate, production inconsistency, vulnerability, race, or deterministic
test gap was demonstrated; no source or test change was warranted.

## Resumed script text round-trip gate (2026-07-23)

The `parse_script` target was replayed from two independent private copies of
its 314-input QA corpus (12,174,139 bytes, maximum input size 723,013 bytes).
It parses textual scripts, formats the parsed bytecode, reparses the
formatted result, and checks bytecode and formatted-string idempotence while
catching only `ScriptParseError` for invalid source text.

Two normal workers completed 8,000 executions each in 58 seconds, reaching
coverage 1,256 with peak RSS of 80 and 78 MB and adding 113 and 98 units; their
corpora expanded to 414 and 400 files. Two ASan/UBSan workers completed 5,000
each in 173 and 195 seconds, reaching coverage 3,333 with peak RSS of 571 and
582 MB and adding 26 and 39 units. The direct-file TSan driver replayed the
resulting 439 and 437 files in three seconds per worker. All six jobs exited
zero without an assertion, sanitizer report, race report, timeout, unexpected
exception, or target artifact.

The TSan workers own independent parser state, so this covers textual
round-trip calls rather than shared concurrent callers. No clean-master
candidate, production inconsistency, vulnerability, race, or deterministic
test gap was demonstrated; no source or test change was warranted.

## Resumed Base58 contract gates (2026-07-23)

The `base58_encode_decode` and `base58check_encode_decode` targets were
replayed from two independent private copies of their existing QA corpora:
151 inputs for Base58 and 196 for Base58Check. The guided mutations exercise
encode/decode round trips, surrounding and internal whitespace, invalid
characters, maximum-length boundaries, failure-output clearing, Base58Check
checksum rejection, and the corresponding round-trip contracts.

Two normal workers per target completed 5,000 executions. Base58 added 126
and 113 units in five seconds with peak RSS of 58 and 56 MB; Base58Check
added 119 and 114 units in six seconds with peak RSS of 54 MB in both runs.
Two ASan/UBSan workers per target completed 3,000 executions: Base58 took 18
seconds at 251 and 252 MB RSS and added 26 and 31 units; Base58Check took 27
and 26 seconds at 259 and 258 MB RSS and added 13 and 25 units. The direct-
file TSan workers replayed expanded corpora of 273/275 Base58 files and
316/318 Base58Check files in under one second per worker. All twelve jobs
exited zero without an assertion, sanitizer report, race report, timeout,
unexpected exception, or artifact.

Each process owns independent decoder state, so TSan covers the exercised
encoding operations rather than arbitrary concurrent callers. No clean-master
candidate, production inconsistency, vulnerability, race, or deterministic
test gap was demonstrated; no source or test change was warranted.

## Resumed signature-cache contract gate (2026-07-23)

The `script_sigcache` target was replayed from two independent private copies
of a bounded 627-input slice of its 669-input QA corpus. The selected inputs
totaled 1,226,125 bytes with a maximum size of 56,663 bytes. The target varies
zero through the default signature-cache size, transaction and witness data,
ECDSA and Schnorr key material, cache-entry hashes, input indexes, amounts,
and store flags. It checks deterministic key construction and the
set/get-without-erase, get-with-erase, and post-erase miss sequence through
the caching signature checker.

Two normal workers completed 8,000 executions each in 35 and 39 seconds,
reaching coverage 2,360 with peak RSS of 107 and 108 MB; their corpora
expanded to 687 and 686 files. Two ASan/UBSan workers completed 5,000 each in
91 and 94 seconds, reaching coverage 10,368 with peak RSS of 598 and 597 MB
and adding 32 and 24 units. The direct-file TSan driver replayed the expanded
719 and 710 files in 20 seconds per worker. All six jobs exited zero without
an assertion, sanitizer report, race report, timeout, unexpected exception,
or target artifact.

Each fuzzer process owns an independent `SignatureCache`; the TSan result
therefore covers cache operations and target setup rather than shared live
node cache schedules. No clean-master candidate, production inconsistency,
vulnerability, race, or deterministic test gap was demonstrated; no source
or test change was warranted.

## Resumed signature-checker replay gate (2026-07-23)

The `signature_checker` target was replayed from two independent private
copies of a bounded 1,785-input slice of its 1,796-input QA corpus. The
selected inputs totaled 1,088,685 bytes with a maximum input size of 49,349
bytes. It compares `EvalScript` with explicit and implicit
`ScriptExecutionData`, repeats the explicit path, and checks deterministic
ECDSA/Schnorr, locktime, sequence, and code-separator callback behavior under
valid and invalid script-flag combinations.

Two normal workers completed 8,000 executions each in four seconds, adding
235 and 218 units with peak RSS of 61 and 62 MB; their corpora expanded to
2,009 and 1,998 files. Two ASan/UBSan workers completed 5,000 each, adding
99 and 93 units at peak RSS of 604 and 565 MB. The direct-file TSan driver
replayed the resulting 2,106 and 2,090 files in one second per worker. All
six jobs exited zero without an assertion, sanitizer report, race report,
timeout, unexpected exception, or target artifact.

Each process owns its deterministic checker and script state, so the TSan
replay covers the target's exercised interpreter calls rather than shared
concurrent script-validation schedules. No clean-master candidate,
production inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed script-operations gate (2026-07-23)

The `script_ops` target was replayed from two independent private copies of
its 320-input QA corpus. The normal workers were allowed to retain the
one-megabyte boundary input; the expanded corpora contained 407 and 406
files. The target mutates scripts and checks legacy and accurate sigop
counts, P2SH sigop extraction, valid-op and push-only classification,
P2A/P2W/P2TR recognition, witness-program output clearing, unspendability,
and failed `GetOp` output/iterator contracts.

Two normal workers completed 8,000 executions each in 81 and 63 seconds,
reaching coverage 814 and 815 with peak RSS of 132 and 128 MB and adding 96
and 91 units. A full expanded ASan/UBSan replay was deliberately stopped
after both workers spent about 90 seconds on the one-megabyte mutation loop
without producing a diagnostic; this was a harness-cost limit, not a target
failure. Two bounded ASan/UBSan workers then replayed the complete normal
corpora restricted to the 319 inputs below 64 KiB, completing 5,000
executions each in 50 and 26 seconds. They reached coverage 2,089 and 2,095,
peaked at 577 and 579 MB RSS, and added 31 and 33 units. The direct-file TSan
driver replayed the resulting 349 and 352 files in under one second per
worker. No assertion, sanitizer report, race report, timeout, unexpected
exception, or target artifact was produced by any completed gate.

The TSan workers own independent scripts and have no shared mutable node
state, so this is evidence for the exercised script operations rather than
arbitrary concurrent validation schedules. No clean-master candidate,
production inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed miniscript-string gate (2026-07-23)

The `miniscript_string` target was replayed from two independent private
copies of its 1,169-input QA corpus. The inputs totaled 4,627,825 bytes with
a maximum size of 307,120 bytes. The target mutates textual Miniscript,
selects the parser context, and checks successful parse context preservation,
canonical `ToString` output, and a second parse/equality round trip. Inputs
rejected by the target's existing expense guard are intentionally skipped.

Two normal workers completed 8,000 executions each in 41 and 42 seconds,
reaching coverage 2,992 with peak RSS of 93 and 91 MB; their corpora expanded
to 1,203 and 1,202 files. Two ASan/UBSan workers completed 5,000 executions
each in 176 seconds, reaching coverage 9,633 with peak RSS of 736 and 738 MB
and adding 14 and 12 units. The direct-file TSan driver replayed the
resulting 1,217 and 1,214 files in 10 seconds per worker. All six jobs exited
zero without an assertion, sanitizer report, race report, timeout,
unexpected exception, or target artifact.

Each fuzzer process owns independent parser objects, so the TSan replay covers
the exercised string parser and round-trip operations rather than shared
concurrent Miniscript callers. No clean-master candidate, production
inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed miniscript-script gate (2026-07-23)

The `miniscript_script` target was replayed from two independent private
copies of its 1,042-input QA corpus. The inputs totaled 14,046,535 bytes with
a maximum size of 331,252 bytes. The target deserializes arbitrary scripts,
selects the P2WSH or Tapscript parser context, and checks successful
`FromScript` context preservation and exact `ToScript` round-tripping.

Two normal workers completed 8,000 executions each in 58 and 75 seconds,
reaching coverage 3,157 with peak RSS of 217 and 222 MB and adding 23 and 22
units; their corpora expanded to 1,065 and 1,064 files. Two ASan/UBSan
workers completed 5,000 executions each in 321 and 301 seconds, reaching
coverage 10,124 with peak RSS of 710 and 717 MB and adding 9 and 8 units. The
direct-file TSan driver replayed the resulting 1,074 and 1,072 files in 21
and 20 seconds. All six completed jobs exited zero without an assertion,
sanitizer report, race report, timeout, unexpected exception, or target
artifact. A preliminary one-run ASan smoke command was stopped because a
maximum-size seed was still executing; it was not counted as a gate result.

Each fuzzer process owns independent script and parser state, so the TSan
replay covers the exercised script parser and round-trip operations rather
than shared concurrent Miniscript callers. No clean-master candidate,
production inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed miniscript-stable gate (2026-07-23)

The `miniscript_stable` target was replayed from two independent private
copies of its 2,234-input QA corpus. The inputs totaled 316,311 bytes with a
maximum size of 110,592 bytes. The target generates stable Miniscript nodes
under both P2WSH and Tapscript contexts and checks textual and script
round-trips, resource estimates, malleable and non-malleable satisfactions,
script verification, and satisfiability invariants.

Two normal workers completed 8,000 executions each in 32 and 36 seconds,
reaching coverage 7,359 with peak RSS of 68 MB and adding 23 and 18 units;
their corpora expanded to 2,257 and 2,252 files. Two ASan/UBSan workers
completed 5,000 executions each in 232 and 234 seconds, reaching coverage
26,503 with peak RSS of 1,111 and 1,047 MB and adding 8 and 9 units. Both
sanitizer workers independently emitted the same libFuzzer `slow-unit`
artifact (SHA-256
`4038d83879f4cd69a55428c632893603b5eeb8d099000dc55ea3d7155eca6686`),
whose deterministic single-input replay took about 2.4 seconds in the normal
build and 12.1 seconds under ASan/UBSan. It produced no assertion,
sanitizer, or crash report. The direct-file TSan driver replayed the expanded
corpora including that slow input, 2,266 and 2,262 files, in 17 seconds per
worker without a race report.

The slow input is a performance observation about the fuzz target's generated
node and satisfaction checks, not a demonstrated production vulnerability:
the input is provider data rather than a serialized network or descriptor
script, and the normal non-sanitized fuzzer replay did not fail. Each fuzzer
process owns independent node and script state, so the TSan result covers the
exercised operations rather than shared concurrent Miniscript callers. No
clean-master candidate, production inconsistency, vulnerability, race, or
deterministic test gap was demonstrated; no source or test change was
warranted.

## Resumed miniscript-smart gate (2026-07-23)

The `miniscript_smart` target was replayed from two independent private copies
of its 2,259-input QA corpus. The inputs totaled 263,019 bytes with a maximum
size of 4,181 bytes. The target constructs nodes for the four base types under
the selected P2WSH or Tapscript context and runs the same textual and script
round-trips, resource estimates, satisfaction, verification, and
satisfiability invariants used by the semantic checks.

Two normal workers completed 8,000 executions each in 51 and 52 seconds,
reaching coverage 7,320 with peak RSS of 71 and 73 MB and adding 48 and 43
units; their corpora expanded to 2,307 and 2,302 files. Two ASan/UBSan
workers completed 5,000 executions each in 292 and 296 seconds, reaching
coverage 26,507 and 26,508 with peak RSS of 927 and 917 MB and adding 6 and
13 units. The direct-file TSan driver replayed the resulting 2,313 and 2,315
files in 15 seconds per worker. All six jobs exited zero without an
assertion, sanitizer report, race report, timeout, unexpected exception, or
target artifact.

Each fuzzer process owns independent node and script state, so the TSan replay
covers the exercised smart construction and semantic operations rather than
shared concurrent Miniscript callers. No clean-master candidate, production
inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed script-sign gate (2026-07-23)

The `script_sign` target was replayed from two independent private copies of
its 6,177-input QA corpus. The inputs totaled 70,174,825 bytes with a maximum
size of 938,133 bytes. The target exercises HD key-path serialization and
deserialization, signature-data merging, legacy and witness signing, script
signature creation, transaction signing, and the resulting per-input error
map contracts. Its malformed stream paths catch only `std::ios_base::failure`;
other exceptions are allowed to escape.

Two normal workers reached coverage 7,991 and 7,990 with peak RSS of 120 and
178 MB. Because libFuzzer reloaded and retained the expanding corpus during
the run, it reported 8,067 and 10,459 total executed units rather than the
requested 8,000; the resulting corpora contained 6,201 and 6,226 files. The
full boundary-inclusive ASan/UBSan replays completed 6,203 and 6,228 total
units, reaching coverage 27,822 and 27,819 with peak RSS of 681 and 676 MB.
The direct-file TSan driver replayed the same 6,201 and 6,226 files in 10
seconds per worker. All completed jobs exited zero without an assertion,
sanitizer report, race report, timeout, unexpected exception, or target
artifact.

No clean-master candidate, production inconsistency, vulnerability, race, or
deterministic test gap was demonstrated; no source or test change was
warranted.

## Resumed valid UTXO-snapshot activation gate (2026-07-23)

The `utxo_snapshot` target was replayed from two independent private copies of
its 1,013-input QA corpus. The inputs totaled 11,525,417 bytes with a maximum
size of 1,027,881 bytes. This target constructs valid and malformed metadata,
activates snapshots, checks UTXO statistics and chain-index invariants, rejects
second activation, and resets the chainstate manager after successful
activation so later inputs do not inherit dirty state.

Two normal workers completed 3,000 executions each in 89 and 86 seconds,
reaching coverage 13,127 with peak RSS of 87 MB and adding 49 and 55 units;
their corpora expanded to 1,060 and 1,067 files. Two ASan/UBSan workers
completed the planned 2,000 executions each in 262 and 266 seconds, reaching
coverage 41,839 and 41,834 with peak RSS of 529 and 533 MB and adding 10 and
9 units. The direct-file TSan driver replayed the resulting 1,070 and 1,075
files in 21 seconds per worker. All six jobs exited zero without an assertion,
sanitizer report, race report, timeout, unexpected exception, or target
artifact.

No clean-master candidate, production inconsistency, vulnerability, race, or
deterministic test gap was demonstrated; no source or test change was
warranted.

## Resumed invalid UTXO-snapshot activation gate (2026-07-23)

The `utxo_snapshot_invalid` target was replayed from two independent private
copies of its 1,562-input QA corpus. The inputs totaled 83,147,971 bytes with
a maximum size of 983,050 bytes. It constructs structured and randomized
snapshot metadata, optionally duplicates coin records, appends a deliberately
invalid late coin record, and asserts that failed activation preserves the
current chainstate, snapshot metadata, cache sizes, and active chainstate
identity.

Two normal workers completed 8,000 executions each in 217 and 222 seconds,
reaching coverage 8,029 and 8,404 with peak RSS of 108 and 177 MB and adding
55 and 51 units; their corpora expanded to 1,614 and 1,612 files. Two
ASan/UBSan workers completed 5,000 each in 252 and 238 seconds, reaching
coverage 25,886 and 25,890 with peak RSS of 711 and 701 MB and adding 14 and
12 units. The direct-file TSan driver replayed the resulting 1,626 and 1,623
files in 36 seconds per worker. All six jobs exited zero without an assertion,
sanitizer report, race report, timeout, unexpected exception, or target
artifact.

This completes the sanitizer follow-up that was previously bounded before
the full invalid-snapshot corpus. No clean-master candidate, production
inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed general proof-of-work gate (2026-07-23)

The `pow` target was replayed from two independent private copies of its
443-input QA corpus. The inputs totaled 32,588,237 bytes with a maximum size
of 1,038,232 bytes. The target mutates compact-target validity, proof-work
calculation, difficulty transitions, block-index relationships, and
proof-equivalent-time overflow and sign contracts.

Two normal workers completed 8,000 executions each in 352 and 395 seconds,
reaching coverage 829 with peak RSS of 103 and 104 MB and adding 14 and 21
units; their corpora expanded to 457 and 464 files. A full boundary-inclusive
ASan/UBSan attempt was deliberately stopped after 512 executions per worker,
when maximum-size inputs reduced throughput to about two executions per
second at 537 and 540 MB RSS. It produced no diagnostic or artifact, but it
is incomplete evidence and is not counted as a sanitizer pass.

The completed bounded ASan/UBSan replay used all normal-corpus inputs below
64 KiB: 353 and 362 files, totaling 6,936,035 bytes combined, with a maximum
size of 60,840 bytes. The two workers completed 5,000 executions each in 550
and 491 seconds, reaching coverage 2,072 with peak RSS of 547 and 539 MB and
adding 51 and 49 units. The direct-file TSan driver replayed the resulting
401 and 410 files in one second per worker. All completed jobs exited zero
without an assertion, sanitizer report, race report, timeout, unexpected
exception, or target artifact.

No clean-master candidate, production inconsistency, vulnerability, race, or
deterministic test gap was demonstrated; no source or test change was
warranted.

## Resumed proof-of-work transition gate (2026-07-23)

The `pow_transition` target was replayed from two independent private copies
of its 235-input QA corpus. The inputs totaled 3,477 bytes with a maximum
size of 41 bytes. The target constructs a full difficulty-adjustment period
with mutated old/new timestamps, versions, and compact targets, then checks
that `GetNextWorkRequired` satisfies `PermittedDifficultyTransition`.

Two normal workers completed 8,000 executions each in 23 seconds, reaching
coverage 386 with peak RSS of 56 and 54 MB and adding 8 and 7 units; their
corpora expanded to 241 and 240 files. Two ASan/UBSan workers completed 5,000
each in 66 and 64 seconds, reaching coverage 1,039 with peak RSS of 656 and
654 MB and adding 0 and 2 units. The direct-file TSan driver replayed the
resulting 241 and 242 files in one second per worker. All six jobs exited zero
without an assertion, sanitizer report, race report, timeout, unexpected
exception, or target artifact.

No clean-master candidate, production inconsistency, vulnerability, race, or
deterministic test gap was demonstrated; no source or test change was
warranted.

## Resumed cluster backend-equivalence gate (2026-07-23)

The `clusterlin_backend_equivalence` target had no native QA corpus, so it was
seeded from the existing cluster-linearization, dependency-graph
serialization, chunking, simple-linearization, and post-linearization corpora.
After deduplicating identical content, each worker used 9,174 inputs totaling
3,103,373 bytes with a maximum size of 8,472 bytes. The target parses an
arbitrary dependency graph and optional linearization, then compares
linearization, chunking, optimality, cost, and input handling across the four
bitset backends used for 64-position clusters.

Two normal workers replayed the full corpus in 23 seconds, reaching coverage
4,883 with peak RSS of 58 and 56 MB. The ASan/UBSan workers replayed the same
corpus in 114 seconds, reaching coverage 15,671 with peak RSS of 636 MB per
worker. The direct-file TSan driver replayed 9,174 files in seven seconds per
worker. All six jobs exited zero without an assertion, sanitizer report, race
report, timeout, unexpected exception, or target artifact.

No clean-master candidate, production inconsistency, vulnerability, race, or
deterministic test gap was demonstrated; no source or test change was
warranted.

## Resumed pool-resource invariant gate (2026-07-23)

The `pool_resource` target was replayed from two independent private copies of
its 1,708-input QA corpus. The inputs totaled 8,144,838 bytes with a maximum
size of 520,066 bytes. The target exercises pool-resource block sizes and
alignment configurations, allocation and deallocation orderings, free-list
and chunk accounting, alignment guarantees, live-span disjointness, and
preservation of randomized contents until deallocation.

Two normal workers completed 8,000 executions each in 9 and 8 seconds,
reaching coverage 2,277 with peak RSS of 64 and 65 MB and adding 45 and 37
units; their corpora expanded to 1,753 and 1,745 files. Two ASan/UBSan
workers completed 5,000 each in 31 and 27 seconds, reaching coverage 7,431
with peak RSS of 618 and 619 MB and adding 15 and 13 units. The direct-file
TSan driver replayed the resulting 1,768 and 1,758 files in one second per
worker. All six jobs exited zero without an assertion, sanitizer report, race
report, timeout, unexpected exception, or target artifact.

Each fuzzer process owns an independent pool resource, so the TSan replay
covers allocator operations and invariants rather than shared concurrent
allocator callers. No clean-master candidate, production inconsistency,
vulnerability, race, or deterministic test gap was demonstrated; no source
or test change was warranted.

## Resumed CuckooCache collision gate (2026-07-23)

The `cuckoocache` target was replayed from two independent private copies of
its 674-input QA corpus. The inputs totaled 8,276,114 bytes with a maximum
size of 352,499 bytes. The target mutates packed atomic flag arrays across
resizes, randomized and deterministic CuckooCache insert/contains/erase
operations, and explicitly checks that inserting an erased colliding entry
refreshes it without losing a second colliding entry.

Two normal workers completed 8,000 executions each in 40 and 41 seconds,
reaching coverage 760 with peak RSS of 95 and 97 MB and adding 58 and 53
units; their corpora expanded to 731 and 727 files. Two ASan/UBSan workers
completed 5,000 each in 128 and 138 seconds, reaching coverage 2,147 and
2,146 with peak RSS of 582 and 583 MB and adding 37 and 28 units. The
direct-file TSan driver replayed the resulting 768 and 755 files in 24 and
23 seconds. All six jobs exited zero without an assertion, sanitizer report,
race report, timeout, unexpected exception, or target artifact.

Each fuzzer process owns its cache and provider state, so the TSan replay
covers the exercised resize and collision operations rather than shared
concurrent cache callers. No clean-master candidate, production
inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed transaction-download manager lifecycle gate (2026-07-23)

The `txdownloadman_impl` target was replayed from a private bounded copy of
1,536 of its 1,560 QA inputs. The selected inputs totaled 5,453,608 bytes and
had a maximum size of 14,335 bytes. The target exercises peer connect and
disconnect, wtxid-relay accounting, transaction announcements and requests,
not-found responses, orphan resolution, 1-parent-1-child package formation,
mempool acceptance and rejection callbacks, block connect/disconnect filters,
reconsideration work, time reversal, and final empty-state checks.

The full bounded seed corpus was too expensive for useful mutation: two normal
workers were interrupted after 1,373 executions each while replaying the same
slow state, without an assertion, diagnostic, or artifact. A deterministic
spread slice of 128 inputs (447,806 bytes, maximum 12,157 bytes) was therefore
used for discovery. Its two normal workers were stopped after 591 and 650
executions, adding 211 and 224 units and expanding their independent corpora to
336 and 343 files. Neither worker produced an assertion, crash, or target
artifact.

Two ASan/UBSan workers completed a separate 64-input seed slice (69,805 bytes,
maximum 2,755 bytes) for 128 executions each in 91 and 85 seconds. They reached
coverage 26,003 and 26,004, added 41 and 39 units, and used peak RSS of 669 and
670 MB. The direct-file TSan driver replayed the resulting 336- and 343-file
corpora in three batches per worker, with every invocation exiting zero and no
race report. All completed gates were free of assertions, sanitizer reports,
race reports, unexpected exceptions, and target artifacts.

The interrupted larger ASan replay timed out only on the known 7,449-byte
input `e36b72c63cbf8f8f2a6704866be08d6bbda26062`; its stack was in
`TxOrphanageImpl::SanityCheck()` repeatedly computing transaction weight. The
same input exited zero in 8.2 seconds with the normal binary and 33.2 seconds
under ASan/UBSan, with no memory or undefined-behavior report. This is a
sanitizer/model performance observation, not a production defect or a completed
full-corpus gate. Each fuzzer process owns an independent manager, so TSan
covers the exercised lifecycle operations rather than arbitrary live-node
peer scheduling. No clean-master candidate, production inconsistency,
vulnerability, race, or deterministic test gap was demonstrated; no source or
test change was warranted.

## Resumed ephemeral-package evaluation gate (2026-07-23)

The `ephemeral_package_eval` target was replayed from a deterministic spread
slice of 64 existing inputs below 16 KiB. The selected inputs totaled 356,177
bytes with a maximum size of 13,472 bytes. This target exercises ephemeral-dust
and TRUC package construction, duplicate-package rejection, package test-accept
atomicity, package versus single-transaction submission, replacement and
standardness combinations, validation callbacks, mempool input indexes, and
the ephemeral-package invariants.

Two normal workers completed 256 executions each without an assertion, crash,
or target artifact. Their independent corpora expanded to 217 and 220 files;
the higher-coverage worker reached coverage 18,028, added 159 units, and used
614 MB peak RSS. Two ASan/UBSan workers completed 66 total executions each in
127 seconds, reaching coverage 58,672 and 58,684 at 614 MB peak RSS, without
adding a unit or producing a sanitizer report. The direct-file TSan driver
replayed the 217- and 220-file expanded corpora in two batches per worker; all
invocations exited zero without a race report or artifact.

This is a bounded sanitizer completion slice; the previously recorded full
corpus attempts remain performance-limited. Each process owns an independent
mempool and validation state, so TSan covers the exercised package operations
rather than arbitrary live-node schedules. No clean-master candidate,
production inconsistency, vulnerability, race, or deterministic test gap was
demonstrated; no source or test change was warranted.

## Resumed package test-accept cache-contract gate (2026-07-23)

Commit `d4fe20c8e8` adds a cache-membership oracle to both `tx_package_eval`
and `ephemeral_package_eval`. For every package input, the fuzzer records
`CCoinsViewCache::HaveCoinInCache` before `ProcessNewPackage` test-accept and
requires the complete membership map to be unchanged afterward, including
the second test-accept path. The production cleanup loops in both
`AcceptToMemoryPool` and `ProcessNewPackage` now assert that every outpoint
listed in `coins_to_uncache` is absent after `Uncache`.

The deterministic `txpackage_tests` case flushes the fixture cache, warms one
clean input, leaves a second clean input uncached, and submits one valid
transaction spending both. It verifies that test-accept preserves the first
entry and removes the second. Existing package tests covered result fields,
mempool state, and callbacks, but did not inspect chainstate cache membership.

Two normal `tx_package_eval` workers started with 387 and 362 corpus files,
completed 388 and 363 executions, reached coverage 18,824 and 18,839, and
used 619 MB peak RSS. Two normal `ephemeral_package_eval` workers started
with independent 32-input slices below 16 KiB, completed 128 executions each,
reached coverage 18,032 and 18,079, and used 617 MB peak RSS. The ephemeral
slices expanded to 113 and 114 files.

The ASan/UBSan package replays completed 389 and 364 executions, reached
coverage 61,295 and 61,316, and used 617 MB peak RSS. ASan/UBSan ephemeral
replays covered the expanded 113- and 114-file corpora and completed 114 and
116 executions, reaching coverage 58,958 and 58,900 at 618 MB peak RSS. The
Debug and TSan deterministic test both passed, with 5/5 assertions. Direct
file TSan replay covered the 387-, 362-, 113-, and 114-file corpora in
parallel batches. Every gate exited zero without an assertion, sanitizer or
race report, timeout, unexpected exception, or artifact.

Severity: n/a. This is a coverage and assertion-hardening change, not a
confirmed master-branch bug. The cleanup behavior is unchanged in release
builds because the new production checks use `Assume`; no clean-master
production inconsistency, vulnerability, race, or additional deterministic
test gap was demonstrated.

## Post-rebase cache and private-broadcast controls (2026-07-23)

After rebasing on exact `origin/master` `526673487ceef18d1df41d77b4783b9b3908ced5`,
the new cache-boundary targets were seeded explicitly because neither had a
native QA corpus. The seeds included empty input, one-byte `00`, `01`, `7f`,
`80`, and `ff` values, and a 64-byte zero input. `cache_sizes` completed two
independent 10,000-execution normal workers, reaching coverage 1,128, followed
by two ASan/UBSan workers with 5,000 executions each, reaching coverage 3,046.
`coins_cache_size_state` completed the same normal and sanitizer budgets,
reaching normal coverage 868/869 and sanitizer coverage 2,526 in both workers.
Direct-file TSan replay covered the resulting 43/45 `cache_sizes` files and
20/20 `coins_cache_size_state` files. No assertion, sanitizer, race, timeout,
or artifact occurred.

The cache-size target checks the `uint64_t` allocation arithmetic at negative,
minimum, percentage, index-count, and saturated `-dbcache` boundaries. The
cache-state target checks the mixed signed/unsigned headroom calculation at
`SIZE_MAX` and random capacities. These are arithmetic contracts; they do not
populate a live cache. The stateful `coinscache_sim`, `coins_view_stacked`, and
DB-resize controls above remain the evidence for cache mutation and asynchronous
prevout-fetch transitions. Each fuzzer process owns independent state, so TSan
does not prove arbitrary shared live-node cache schedules race-free.

The `p2p_private_broadcast` harness was then seeded from 256 existing
`txrequest` inputs below 64 KiB. Two normal workers completed 2,000 mutations
each, expanding to 425 and 434 files and reaching coverage 8,038/8,053. Two
ASan/UBSan workers completed 1,500 mutations each, reaching coverage
24,374/24,372 at 662/657 MB peak RSS. Direct-file TSan replay covered 490 and
497 files. The run includes IBD and post-IBD paths, zero-to-three pending
transactions, relay-true and relay-false private handshakes, extra peer types,
GETDATA/PONG confirmation, repeated broadcasts, and malformed inbound message
types. Every job exited zero without an assertion, sanitizer, race/deadlock
report, timeout, unexpected exception, or target artifact.

The private-broadcast production state machine differs from clean master on
this branch because earlier commits fixed two confirmed accounting defects.
Therefore the same expanded inputs were replayed against an exact-master
ASan/UBSan build. The unmodified master fuzzer completed 1,000 mutations in
each of two workers. The branch's 12-line relay-false fuzzer delta was then
grafted into the disposable checkout while all production sources remained at
`526673487c`; two workers, each with an explicit zero-byte relay-false seed,
completed 1,023 and 1,075 executions. Neither control produced an assertion,
sanitizer report, or artifact. This rules out a current-master production
failure for the new handshake mutation, while not claiming that arbitrary
live peer teardown schedules are race-free.

A typed-catch audit of the reviewed cache and private-broadcast paths found no
catch-all or `std::exception` handler. The private-broadcast loop catches only
`std::ios_base::failure`, the expected malformed-message parser exception;
assertions and all other exception types remain fatal. No production bug,
deterministic test gap, or source change was warranted by this campaign.

## OpenRPC alias and document-shape gate (2026-07-23)

The current master OpenRPC feature is represented by `672dd42d14`
(`getopenrpcinfo`) and `ca9ffb8e12` (`rpc.discover`), merged in
`526673487ceef18d1df41d77b4783b9b3908ced5`. The RPC fuzzer already listed both
commands as safe, but it only checked the shape of exceptions after executing
one command. It did not compare the two public entry points or inspect the
generated document beyond whatever path the selected command happened to take.

The fuzzer now checks, after either command executes, that
`getopenrpcinfo(false)` and `rpc.discover` serialize identically. It also checks
the generated document's OpenRPC version, object/array types, strictly sorted
unique method names, parameter arrays, and result name/schema objects. The
`rpc_openrpc_alias_matches` unit test provides deterministic coverage of the
same cross-entrypoint contract. No production assertion was added: both
production handlers already call the same `CRPCTable::buildOpenRPCDoc(false)`
implementation, so a production-side duplicate call would add RPC work without
checking an independent invariant. The existing functional
`rpc_openrpc.py` test also compares the aliases.

Seed construction was verified against `FuzzedDataProvider`: `ConsumeTime`
consumes integral bytes from the end of the input. Therefore valid reachability
seeds place `getopenrpcinfo\\X` or `rpc.discover\\X` first and append eight time
bytes. The hidden-path seed additionally encodes one positional `true`
argument before the time suffix. Earlier files that put the time bytes first
were discarded because they did not reliably reach the selected method.

With the corrected seeds, two independent normal workers per path completed
1,500 executions each for public `getopenrpcinfo`, hidden-argument
`getopenrpcinfo`, and `rpc.discover` (9,000 total). Two independent
ASan/UBSan workers per path completed 1,000 executions each (6,000 total).
The expanded ASan corpora contained 30, 27, 44, 33, 26, and 19 files; the
direct-file TSan driver replayed all 179 files. Every completed job exited zero
without an assertion, sanitizer report, race report, unexpected exception,
timeout, or artifact. The two initial discovery attempts that hit the full
root filesystem were rerun with `TMPDIR=/mnt/my_storage/tmp` after removing
stale generated test datadirs; they are not counted as fuzz executions.

Mutation proof: `src/rpc/server.cpp` was temporarily changed so the
`rpc.discover` actor called `buildOpenRPCDoc(/*include_hidden=*/true)` instead
of `false`, with no other production change. The deterministic unit test
failed (exit 201), and correctly laid-out public, hidden, and discovery seeds
each aborted at the fuzzer alias assertion (libFuzzer exit 77). The production
mutation was restored before the clean gates. This is a coverage and oracle
change, not a confirmed master-branch bug; no clean-master production
inconsistency, vulnerability, race, or additional deterministic test gap was
found.

## Script failure-error oracle after master rebase (2026-07-23)

The investigation initially ran against `origin/master`
`774d11c58f221294950f05ac4d249e19583e4b7b`, which merged three
script-vector commits: `a86a96d17b` for
`CHECKSIGVERIFY`/`CHECKMULTISIGVERIFY`, `37edf0e233` for CLTV, and
`c4068cf37b` for negative-zero CSV. The vectors already gave deterministic
coverage for their expected `ScriptError` values, but `eval_script` only
checked that the result, error-pointer result, and final stack agreed. A
production path returning the wrong non-OK error consistently would therefore
pass the fuzzer contract.

The fuzzer now checks the exact failure errors for the new empty-stack,
negative, negative-zero, non-minimal, zero, and five-byte CLTV/CSV cases, plus
the failing CHECKSIGVERIFY and CHECKMULTISIGVERIFY cases, under both BASE and
WITNESS_V0. The existing master JSON vectors remain the deterministic unit
coverage; no separate unit case or production assertion was needed.

Mutation proof: `src/script/interpreter.cpp` was temporarily changed only at
the CHECKSIGVERIFY failure return, replacing `SCRIPT_ERR_CHECKSIGVERIFY` with
`SCRIPT_ERR_EVAL_FALSE`. After rebuilding the mutated production library, the
pre-change `eval_script` contract replayed 37 boundary/corpus inputs and
exited zero, while the deterministic `script_json_test` failed with exit 201.
With the new oracle and the mutation still present, the first replayed input
aborted at the exact-error assertion with libFuzzer exit 77. The production
line was restored before every clean build and replay.

Controls: the rebased branch's `script_tests` suite passed all 27 cases. A
normal campaign completed 5,000 executions and an ASan/UBSan campaign
completed 5,000 executions, all without assertions, sanitizer reports, or
artifacts. The direct-file TSan driver replayed 37
independent inputs without a race report. An isolated build from the exact
`774d11c58f221294950f05ac4d249e19583e4b7b` control tree passed all 21
script-suite cases; the relevant interpreter paths differ from this branch only
by unrelated earlier assertion/reset work.
During finalization, `origin/master` advanced to
`26b730cdbf2c77c12f684fd50bd376212b725394`; the branch was rebased onto it
before this commit. Those five intervening commits add BIP32 seed-length
validation and a secp256k1 subtree update, but do not touch the script
interpreter, script vectors, or this oracle. The rebased branch's refreshed
script/fuzzer gates were rerun after the rebase: the branch `script_tests`
suite passed 27 cases, normal and ASan/UBSan fuzzers each completed 1,000
executions, and direct-file TSan replay covered all eight intended boundary
inputs without a report.

Severity: n/a. This is an error-code oracle and fuzzer-coverage improvement,
not a confirmed master-branch production bug, vulnerability, race, or
behavior change. The temporary mutation was deliberately non-consensus and was
never committed.

## Current-master FeeFrac aggregation follow-up (2026-07-24)

* Current branch fix: `f1d3c0f450` (`txgraph: canonicalize saturated chunk fee aggregation`).

The full-range fee mutation from `2de843c875` exposed a separate production defect
in the cluster chunking path. The deterministic witness has three locally modified
transactions with fees `FeeFrac{INT64_MIN, 1}`, `FeeFrac{INT64_MIN, 1}`, and
`FeeFrac{100, 1}`, dependencies `0 -> 1 -> 2`, and linearization `{0, 1, 2}`.
The values are outside ordinary transaction policy, but the TxGraph fee-modification
API accepts the full signed fee coordinate and the cluster backend must remain
internally consistent for such local state.

An exact clean-master worktree at `6b059d9dbd` was built with Clang 19 Debug and
`-ftrapv`. Calling `ChunkLinearizationInfo()` with the witness trapped in signed
`FeeFrac` addition (exit 132). A second direct test built a real TxGraph with the
same transactions and dependencies: `SanityCheck()` passed, but
`GetBlockBuilder()` reached the same production aggregation and trapped. This
distinguishes the finding from a fuzzer-only assertion or an invalid simulator
model. On the branch before this follow-up, the earlier saturation fallback
(`5fea0b29f4`, originally `eb5a249ff2`) avoided the trap but returned a cached
chunk fee of `INT64_MIN` while summing the returned chunk's individual members
gave `INT64_MIN + 100`; the new deterministic TxGraph test catches that mismatch.

The same clean-master mutation also fails earlier in the graph lifecycle. A
temporary `txgraph_full_range_probe` used three `FeeFrac{INT64_MIN, 1}` transactions
of size 1 with star dependencies `0 -> 1` and `0 -> 2`, then called `DoWork(300000)`.
The normal Clang 19 Debug `-ftrapv` build trapped in `FeeFrac::operator+=` while
`SpanningForestState::Activate` loaded the linearization, before a block-builder
query. An exact sanitized build reported signed overflow at `feefrac.h:109` and
invalid negation of `INT64_MIN` at `cluster_linearize.h:1981`, then reached the
staging-diagram invariant. This is the same aggregate-fee defect, not a second
finding; `txgraph_do_work_saturated_fee_merge_is_stable` now covers the activation
path deterministically on the fixed branch.

The production fix makes the transaction set the source of truth for aggregate
fees. `SetInfo::Set`, merge, and subtract operations now recompute with
`DepGraph::FeeRate()` in canonical position order. `ChunkLinearization()` projects
the same `ChunkLinearizationInfo()` result, and all spanning-forest activation and
deactivation paths use the canonical merge/subtract operations. When saturating
arithmetic makes a checked comparison diagram depend on merge order,
`ComparableChunkLinearization()` declines comparison rather than asserting a false
equivalence. The deterministic tests cover both the standalone chunking result and
the real block-builder cache/member-fee contract, as well as the earlier `DoWork`
activation path.

The fuzzer changes are grouped with the production fix because they define the
discovery and regression contract. `clusterlin_chunking` now constructs the
full-range three-transaction case separately from the serialized depgraph format,
whose fee encoding masks values to 52 bits. The TxGraph simulator uses the same
canonical set aggregation, and its cached-fee and staging-diagram oracles skip
only comparisons whose saturated sums are not representable. During development,
two corpus inputs triggered those newly stricter fuzzer oracles: one exposed a
simulation-versus-cluster position-order difference, and one exposed a staging
diagram difference under saturation. Both replayed cleanly on the pre-change
branch baseline, so they are recorded as oracle corrections, not additional
production defects. A first comparator assertion was likewise caused by the new
production change and was corrected by making the comparator reject that
non-comparable case.

Verification after the fix used the existing QA corpora with independent workers:
`clusterlin_chunking` completed 16,000 normal and 8,000 ASan/UBSan executions;
the TxGraph normal corpus completed two 5,121-input replays; the saved oracle
inputs and two 258-input TxGraph ASan/UBSan slices completed without a report; and
two 64-input TSan slices each for `txgraph` and `clusterlin_chunking` exited zero.
The three new Debug unit tests also pass, including the `DoWork` regression. No
sanitizer race, consensus effect,
unauthenticated remote trigger, wallet impact, or persisted-state corruption was
demonstrated. Severity against clean master is medium: a local/direct fee
modification can make cluster-mempool or block-builder fee metadata trap or become
inconsistent, affecting local policy and construction, but it is not a consensus
or remote memory-safety vulnerability.

## Latest-master global transaction-rate-limit controls (2026-07-25)

The latest baseline for this pass was the exact `origin/master` tip
`e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6`, including the global transaction
rate-limit and TokenBucket changes. The existing functional
`p2p_tx_relay_rate_limit.py` scenario passed on the rebased branch: an 80-
transaction burst at `-txsendrate=2`, a backlogged RBF replacement, and complete
drainage all produced the expected 80 surviving announcements.

The broad P2P controls were replayed from the preserved QA corpora under the
Clang 19 TSan build: `process_messages` completed 4,432 inputs,
`cmpctblock` 1,971, `txdownloadman` 1,501, `txdownloadman_impl` 1,561, and
`txrequest` 810. Every run exited zero without a ThreadSanitizer report,
sanitizer diagnostic, assertion, or artifact. Earlier normal and ASan/UBSan
multi-worker controls for `process_messages` and `cmpctblock` were also clean.

A deterministic delayed-verack probe checked a boundary where
`ProcessInvBacklog()` runs before a peer's first post-verack `SendMessages()`
initializes `m_next_inv_send_time`. A P2P transaction accepted while the only
new peer was still completing its handshake left the global backlog at zero and
the peer received no later `INV`. This is expected best-effort relay behavior,
not a newly introduced defect: the pre-global-rate-limit clean-master source
also skipped peers while `m_next_inv_send_time == 0`. The probe passed against
both the rebased branch and an exact clean `origin/master` build; no production
change or permanent test was warranted.

Result: no new global-rate-limit, transaction-download race, compact-block
collision, consensus, wallet, or unauthenticated memory-safety defect was
demonstrated in this pass.

## Full coins-cache view sanitizer follow-up (2026-07-25)

The preserved full corpora were replayed against the rebased branch to close
the remaining sanitizer coverage gap around cache/view composition. The TSan
libFuzzer build completed 21,874 `coins_view` inputs and 13,049
`coins_view_overlay` inputs. A full ASan/UBSan replay of
`coins_view_overlay` completed 14,072 executions at 577 MB peak RSS. All runs
exited zero without an assertion, sanitizer diagnostic, race report, timeout,
or artifact.

These controls exercised cache reads and writes, overlay fetches, flush and
restore transitions, backend replacement, reset/destruction, and asynchronous
prevout-fetch work. Each fuzzer process owns its own cache and backend, so TSan
does not establish that arbitrary live-node cache callers are race-free. The
branch includes the previously recorded clean-master coins-cache fixes; no new
clean-master failure, production inconsistency, or deterministic test gap was
found, and no source change was warranted.

## Live HTTP and CLI TSan integration (2026-07-25)

A disposable Clang 19 `RelWithDebInfo` TSan node build was configured with
`SANITIZERS=thread`, `BUILD_FOR_FUZZING=OFF`, `BUILD_DAEMON=ON`,
`BUILD_TESTS=OFF`, `ENABLE_WALLET=OFF`, `ENABLE_IPC=OFF`, and `WITH_ZMQ=OFF`.
This was necessary because the existing TSan libFuzzer build intentionally does
not expose the `bitcoind` target. The daemon and CLI were then exercised through
the functional framework using the build-generated `config.ini`.

`interface_http.py` passed, covering HTTP/1.1 persistence and keep-alive,
connection close, HEAD responses, request-size limits, pipelining, chunked
requests, RPC timeouts, authentication failures, unsafe methods, path
traversal, request smuggling, duplicate `Content-Length`, null bytes, invalid
HTTP versions, and invalid header whitespace. `interface_bitcoin_cli.py` also
passed after the omitted `bitcoin-cli` target was built; it covered malformed
chunked responses, conflicting duplicate lengths, invalid status codes, named
arguments, RPC wait timeouts, cookie/password handling, request IDs, CLI
argument conflicts, and the no-IPC error path. Wallet-specific CLI cases were
skipped because this integration build deliberately disabled the wallet.

Both tests exited zero with `TSAN_OPTIONS=halt_on_error=1` and emitted no
ThreadSanitizer race, sanitizer, assertion, timeout, or daemon-log diagnostic.
The initial CLI attempt failed only before test execution with
`FileNotFoundError` for the unbuilt `bitcoin-cli`; building that target and
rerunning produced the passing result. This closes the live HTTP/RPC lifecycle
gate for the configured components, but it is not a claim that wallet or IPC
code was tested, and no production defect or permanent test gap was found.

## Compact-block collision recheck on current master (2026-07-25)

The collision-focused partial-block state machine was replayed again after the
latest master rebase. The branch's `partially_downloaded_block` target completed
all 2,016 inputs in the preserved corpus under the normal Clang 19 fuzzer. A
selected 503-file spread containing inputs up to 939,095 bytes then completed
504 ASan/UBSan executions and 504 TSan executions. All exited without an
assertion, sanitizer diagnostic, race report, timeout, or artifact. The
branch's `blockencodings_tests` suite also passed all 27 cases.

To rule out a previous branch change masking a production failure, the same
partial-block spread was run against an unmodified exact `origin/master`
worktree at `e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6`. Its original
`partially_downloaded_block` fuzzer completed 504 ASan/UBSan executions without
a report. The exact-master original `cmpctblock` target also completed a
492-file collision-heavy spread in 498 ASan/UBSan executions without a report,
assertion, or artifact. These clean-master controls cover production behavior
without the branch's collision hooks or state assertions.

The remaining position-mapping review found no new boundary failure. A
repeated prefilled absolute position is not representable on the wire: each
encoded prefilled offset advances the position by `index + 1`, so positions are
strictly increasing. The 65,535-transaction boundary and the no-coinbase,
prefilled, duplicate-source, null-source, early-exit, terminal-collision, and
reusable-`FillBlock` transitions are covered by the deterministic tests and
fuzzer controls already recorded above. No current-master compact-block
collision defect, race, or deterministic test gap was demonstrated, so no
production or fuzzer source change was warranted in this recheck.

## Inbound transaction-relay capacity and eviction (2026-07-25)

The latest master feature was exercised with a new `connman_inbound_relay_capacity`
target. Each input chooses `m_max_automatic_connections` across the complete
valid `0..200` range and `m_full_relay_inbound_percent` across `0..100`, then
constructs 1..64 registered `CNode` objects. The first node is always inbound
and tx-relaying so zero-capacity configurations are reachable. The remaining
nodes vary connection type, relay state, disconnect state, NoBan permission,
eviction preference, service/traffic timestamps, ping time, and network
metadata. The target also mutates an optional protected peer and may invoke the
capacity check twice, covering the post-eviction state rather than only a fresh
node list.

The oracle counts live inbound tx-relay peers before `EvictTxPeerIfFull()`. It
requires no new disconnect while at or below capacity; above capacity it
requires either one newly disconnected inbound tx-relay candidate that is not
protected or a false return with no disconnect when every candidate is
protected. The production path now also asserts this same inbound/tx-relay
postcondition after the candidate snapshot is reacquired and before marking a
node disconnected. This is invariant hardening at the snapshot/relock
boundary, not a behavior change and not a discovered clean-master defect.

The exact clean-master worktree at `e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6`
contained only the target and required headers; its production sources were
otherwise unchanged. Two independent ASan/UBSan workers on the branch and two
on exact master each completed 3,000 generated executions. Exact master also
replayed the two branch corpora (60 and 63 inputs) without a diagnostic. After
adding the production assertion, branch ASan/UBSan replays completed 60 and 63
inputs, and branch TSan replays completed the same 60 and 63 inputs, all with
zero exit status and no assertion, sanitizer, race, timeout, or artifact.
The focused `net_peer_eviction_tests,net_tests` unit selection passed all 34
cases, and the rebuilt `p2p_connection_limits.py` functional test passed in
7 seconds. The existing functional test covers the live handshake, bloom-filter
relay transition, zero/100 percent settings, and tx-only eviction behavior, so
no additional deterministic regression test was needed.

Result against clean master: no new production bug, remote denial of service,
state inconsistency, or race was demonstrated. The fuzzer target and the
production invariant are retained as coverage and hardening for the new
capacity/eviction feature; the finding severity is therefore none.

## Descriptor self-expansion at an exhausted keypool (2026-07-26)

The recent master change `1e996640e6` makes `CanGetAddresses()` return true
for ranged descriptors whose keys can self-expand, even when
`next_index == range_end`. The existing `scriptpubkeyman` target could parse
unhardened xpub descriptors, but it did not deliberately consume the final
cached address and then verify the self-expansion transition. The new action
does exactly that: it consumes the current cache, asserts that availability
remains true for a self-expanding single-type descriptor, requests the next
destination, and checks that both `next_index` and `range_end` advance.

The deterministic `get_new_destination_self_expanding_xpub` unit case uses a
one-entry `wpkh(xpub/*)` keypool and makes the same transition in 12 assertions.
The direct fuzzer seed is `wpkh(%04/*)` followed by the `FuzzedDataProvider`
terminator and action-selection bytes; `%04` expands to a generated xpub.

The branch unit test passed 12/12 assertions. The branch wallet ASan/UBSan
target ran with two workers over the 10,026-input preserved corpus and a
13,000-run budget per worker (about 2,974 mutations after corpus loading); both
workers exited zero with no assertion, sanitizer, timeout, or artifact, adding
43 and 25 units respectively. The direct seed also completed cleanly under the
same sanitizer build. An exact detached `origin/master` worktree at
`e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6`, with only the deterministic unit
case injected, passed the same 12/12 assertions under the Debug
`ABORT_ON_FAILED_ASSUME` configuration. This control is important because the
branch contains earlier descriptor write/rollback changes
(`059365c88b`, `5f9d7b084f`, and `a2560d7ef1`), while the self-expansion feature
itself is already on master.

Result against clean master: the state transition is correct and no production
defect was demonstrated. This is a coverage omission only; severity none. No
production assertion or behavior change is warranted.

## Descriptor reservation return after a descriptor write failure (2026-07-26)

The existing reservation lifecycle calls `GetReservedDestination()` to consume
an address and `ReturnDestination()` when the caller cancels the reservation.
The clean-master mutation used a one-entry `wpkh(xpub/*)` descriptor, successfully
reserved index 0, and then installed a deterministic SQLite `BEFORE INSERT`
trigger that rejects the descriptor-row write. On the unmodified
`origin/master` baseline `e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6`,
`ReturnDestination()` decremented `m_wallet_descriptor.next_index` in memory
before checking the failed write. The database row remained at `next_index == 1`,
while the live descriptor became `next_index == 0`; the deterministic test failed
at the live-state assertion with `0 != 1` and exit code 201.

The production fix stages the decremented `WalletDescriptor`, writes that copy,
and publishes it to `m_wallet_descriptor` only after `WriteDescriptor()` returns
true. A failed write therefore leaves both representations at the consumed index
and suppresses the address-availability notification. This is distinct from the
earlier branch-only `GetNewDestination()` write-failure fix (`059365c88b`): the
exact-master control contained neither production change when this return-path
failure was reproduced. Applying only the new fix to that control made the same
test pass all 9/9 assertions.

The branch's complete `scriptpubkeyman_tests` suite passed 8 cases and 64
assertions. A direct `wpkh(%04/*)` fuzzer seed reached the new action and passed
under ASan/UBSan. Two ASan/UBSan workers then loaded 10,026 corpus inputs and
completed 13,000 executions each, exiting zero without an assertion, sanitizer
diagnostic, timeout, or artifact; they added 33 and 31 units and peaked at 933
and 977 MB RSS. The failure requires a local wallet database/storage failure or
equivalent fault, not a remote or consensus input. It is a low-to-medium
severity wallet state-consistency and availability defect, with no demonstrated
key loss or remote attack path.

## MarkUnusedAddresses after a descriptor write failure (2026-07-26)

This is a separate clean-master wallet-state defect from the reservation return
case above. The mutation used a one-entry `wpkh(xpub/*)` descriptor, consumed
index 0, topped the cache through index 10, selected the cached script at index
1, and called `MarkUnusedAddresses()` while a SQLite `BEFORE INSERT` trigger
rejected only the 49-byte descriptor metadata row. On unmodified
`origin/master` `e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6`, the call left the live
descriptor at `next_index == 2` while SQLite remained at `next_index == 1`.
The exact deterministic control failed at the persisted-state assertion with
`1 != 2`, 12/13 assertions passing, and exit code 201. A restart after this
failure could reuse the already-used keypool index, causing wallet address
reuse and inconsistent accounting. The trigger is local storage failure or an
equivalent database fault, so severity is low to medium rather than remote or
consensus-level.

The first attempted repair restored the historical unconditional descriptor
write. That fixed the successful next-index mutation, but changed the existing
`get_new_destination_write_failure` contract from a returned error to a
`TopUpWithDB: writing descriptor failed` exception. The final fix retains the
staged top-up state from `5f9d7b084f`, adds a private `TopUpInternal()` force-write
path for `MarkUnusedAddresses()`, and restores the pre-call descriptor if the
transaction throws or returns false. Ordinary top-ups still write descriptor
metadata only when their staged range/cache state changed.

The branch regression introduced by `5f9d7b084f` is recorded separately from
the clean-master finding: its conditional write skipped a successful
`next_index`-only `MarkUnusedAddresses()` update, and the guided success seed
`371c53b218e5c793e7f2c066809ab7345182b86cb1f24053f96908d5eb01880b` failed the
new persisted oracle on that pre-repair branch. The exact clean master passed
the same successful mutation, so that success-path discrepancy was not counted
as a master defect. The clean-master write-failure control above is what
establishes the confirmed production bug.

The deterministic branch regression test now covers both successful persistence
and the forced-write failure: the complete `scriptpubkeyman_tests` suite passed
10 cases and 92 assertions, including 13 assertions for the failure case. The
fuzzer has matching success and failure actions. The failure action catches only
the exact expected `std::runtime_error` and asserts that both live and persisted
`next_index` remain unchanged; the guided failure seed is
`58055d264cf970c55e77782107e57f3fab082577906a57eeee1a20d9a537277d`.
Removing the force-write condition made that seed abort at `assert(expected_failure)`
(exit 77); restoring it passed the same seed under ASan/UBSan. Both guided
seeds and a private 10,027-file corpus copy then passed two ASan/UBSan workers
for 13,000 executions each, adding 32 and 31 units with peak RSS of about
1,018 and 1,036 MB. One 10-second slow-unit artifact was replayed separately
in 18.9 seconds without a diagnostic; it was a performance outlier, not a
failure artifact.

## Descriptor encryption after a descriptor-key database failure (2026-07-26)

This is a new clean-master wallet-storage defect, distinct from the descriptor
index and cache-row failures above. The exact baseline was
`origin/master` `e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6`. The deterministic
mutation created a private `wpkh(xprv/*)` descriptor, opened an active
`WalletBatch`, and injected SQLite failures in the descriptor-key transition.

Two independent omissions were reproduced. With a `BEFORE INSERT` failure for
the encrypted `WALLETDESCRIPTORCKEY` record, master ignored the false return
from `WriteCryptedDescriptorKey()`. The direct control therefore returned true
from `DescriptorScriptPubKeyMan::Encrypt()` and reported a valid decryption
check even though the encrypted record was not written. With a `BEFORE DELETE`
failure for the plaintext `WALLETDESCRIPTORKEY`, master also returned true and
`TxnCommit()` succeeded: `WriteCryptedDescriptorKey()` ignored the failed
`EraseIC()` call, leaving both plaintext and encrypted records in the database.
The two exact-master controls each failed with exit code 201 at the expected
`!encrypted` assertion; the second control specifically completed the commit.

The production fix has three parts. `DescriptorScriptPubKeyMan::Encrypt()` now
encrypts into a temporary `CryptedKeyMap`, checks every database result, and
registers a transaction listener that publishes the map and clears plaintext
memory only after commit. `WalletBatch::WriteCryptedDescriptorKey()` now returns
the plaintext erase result instead of unconditionally returning true.
`CWallet::EncryptWallet()` uses a stack-owned batch, checks transaction start,
master-key write, manager encryption, and commit, and restores
`mapMasterKeys`/`nMasterKeyMaxID` after any failed or exceptional transaction.

The branch regression coverage is deterministic:
`encrypt_descriptor_write_failure_preserves_state` covers the encrypted-record
insert failure, `encrypt_descriptor_erase_failure_preserves_state` covers the
plaintext-record delete failure and verifies that the database still contains
only the plaintext record after rollback,
`encrypt_wallet_descriptor_write_failure_preserves_state` covers the caller's
descriptor-manager failure path, and
`encrypt_wallet_master_key_write_failure_preserves_state` covers failure of the
master-key record itself. The focused `scriptpubkeyman_tests` suite passed all
14 cases after these additions.

The `scriptpubkeyman` fuzzer now has matching insert-failure and delete-failure
actions. The delete mutation was reached by both ASan/UBSan workers from a
private 124-file corpus on their initial corpus pass. After removing the
temporary reachability assertion, both generated inputs replayed cleanly. A
real campaign then ran two independent sanitizer workers with `-jobs=2
-workers=2`, each loading all 124 files and completing 5,000 executions in
about 12 seconds; both exited zero without assertions, sanitizer diagnostics,
timeouts, or artifacts, reaching coverage 12,238 and 12,256.

Severity is low to medium against master: a local wallet database or storage
failure is required, and no remote, consensus, or demonstrated key-loss path
was found. The potential impact is wallet availability and inconsistent
encryption state across restart, with the delete-failure path also leaving
plaintext key material in the database until the fault is repaired.

## Descriptor update after a descriptor-key write failure (2026-07-26)

This is an additional failure path through the descriptor-update transaction
defect already fixed by `a2560d7ef1`; it is not a new production regression on
this branch. An exact detached `origin/master` control at
`e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6` created an existing private
`wpkh(xprv/*)` descriptor, encrypted and unlocked the wallet, supplied a fresh
private key through `UpdateWalletDescriptor()`, and injected a SQLite
`BEFORE INSERT` failure for that key's `WALLETDESCRIPTORCKEY` row. The first
update threw the expected `UpdateWithSigningProvider: writing descriptor
private key failed` error, but the live encrypted-key map retained the new key.
After removing the trigger, retrying therefore skipped the database insert and
the final database-key assertion failed. The same pre-fix ordering affected
the plaintext `WALLETDESCRIPTORKEY` branch.

The existing production transaction snapshots and restores both descriptor-key
maps, the descriptor, script maps, and cached-index boundary around the update.
This round adds deterministic plaintext and encrypted unit tests that require
the failed call to leave both memory and SQLite unchanged, then require a
retry to persist the key. The fuzzer adds the corresponding exact-key SQLite
failure mutation, catches only the expected runtime error, checks the rollback
oracle, drops the trigger, and checks that the retry writes the key. A temporary
assertion proved the guided 20-byte seed selected this new action; after removing
the probe, the seed passed under ASan/UBSan.

The focused `scriptpubkeyman_tests` suite passed all 16 cases. Two independent
ASan/UBSan workers loaded the preserved 10,025-file corpus plus the guided seed,
executed 10,029 units each, and exited zero with no assertion, sanitizer,
timeout, or artifact. The clean-master failure requires a local wallet database
or storage failure and can leave the wallet's live and persisted key state
divergent until restart or repair; no remote, consensus, or demonstrated key
loss path was found. Severity against clean master is low to medium. The fix is
already present in `a2560d7ef1`; this commit supplies the missing encrypted-path
proof and fuzzer coverage.

## Resumed RPC and relay corpus gates (2026-07-26)

The latest-master RPC vsize fields were rechecked with the existing `rpc` target
and its 13,390-input QA corpus. Two independent Clang 19 ASan/UBSan workers
loaded the complete corpus and executed 13,534 and 13,531 units respectively;
both exited zero, added no corpus units, and produced no assertion, sanitizer,
timeout, or artifact. The corpus includes the mempool-related RPC commands
`getmempoolentry`, `getrawtransaction`, `submitpackage`, and
`testmempoolaccept`. The deterministic functional tests
`mempool_accept.py`, `mempool_sigoplimit.py`, and `rpc_packages.py` also all
passed, covering the distinction between sigop-adjusted vsize, BIP141 vsize,
and the deprecated `vsize` field.

The cluster-mempool transaction-pool target then replayed all 8,000 preserved
inputs under two ASan/UBSan workers (8,002 and 8,003 units) and two TSan workers
(8,001 units each). The global relay/message-processing target replayed all
4,431 inputs under two ASan/UBSan workers (5,807 and 5,824 units) and two TSan
workers (4,432 units each). Every worker exited zero without an assertion,
sanitizer diagnostic, race report, timeout, or artifact, and none added a new
unit.

These runs exercise the latest mempool RPC presentation, cluster ancestry and
fee metadata, global transaction-rate limiting, peer state transitions, and
the narrowed malformed-message exception path. The RPC production sources are
unchanged from clean master; the transaction-pool and message-processing
assertions are branch hardening, and the existing exact-master controls cover
the corresponding production paths. No new production mistake, race, or
deterministic test omission was demonstrated; severity is none.

## Resumed concurrent database-reader ASan gate (2026-07-26)

The existing `dbwrapper_concurrent_reads` QA corpus has 1,116 small inputs.
The earlier full ASan/UBSan attempt was performance-limited in large seeded
databases, while the corresponding full TSan gate had already completed. To
separate sanitizer cost from a diagnostic, two private 128-input slices were
used: the 128 smallest seeds and the 128 largest seeds.

The smallest-seed ASan/UBSan worker completed 512 executions, added 151 units,
and exited zero without an assertion, sanitizer report, timeout, or artifact.
The largest-seed worker was deliberately interrupted after 168 executions when
it repeatedly reached the same 12-second slow unit; it produced no diagnostic
or crash artifact. Replaying the two slow units individually exited zero in
1.295 and 1.767 seconds with the normal fuzzer and in 154 and 160 milliseconds
under TSan. This confirms database-model and sanitizer overhead rather than a
runtime defect or race. The full ASan corpus gate remains incomplete and is
not counted as a pass; no clean-master control or source change was warranted.

## Resumed transaction-message relay gate (2026-07-26)

The `process_message` QA corpus contains 29 existing inputs whose first
message selector is `tx`. They were replayed as fixed inputs in independent
directories under the current branch's Clang 19 ASan/UBSan and TSan
libFuzzer builds with `LIMIT_TO_MESSAGE_TYPE=tx`. The ASan/UBSan replay
completed 37 total executions and the TSan replay completed 30; both exited
zero without an assertion, sanitizer report, race report, timeout, or
artifact.

The current functional `p2p_tx_relay_rate_limit.py` scenario also passed in
four seconds. It covers the configured count-bucket burst, backlog drainage,
an RBF replacement while the original is queued, and final announcement
accounting. These checks found no global relay-rate inconsistency, memory
defect, or race; no production or fuzzer source change was warranted. The
fixed-input replay covers the selected transaction-message path and the
functional test covers the live bucket workflow, not arbitrary peer
scheduling.

## Resumed coins-cache simulation ASan gate (2026-07-26)

The stateful `coinscache_sim` target was exercised from two independent
64-input slices of its existing QA corpus. The 64 smallest seeds completed
512 ASan/UBSan executions, added 109 units, and exited zero without an
assertion, sanitizer report, timeout, or artifact. The 64 largest seeds were
deliberately stopped after 13 executions when the same 12-second cache-model
construction recurred; no diagnostic or crash artifact was produced.

The resulting slow input replayed individually in 2.639 seconds with the
normal fuzzer and 1.085 seconds under TSan, both with exit code zero and no
report. This confirms model and sanitizer overhead rather than a coins-cache
memory defect or race. The full ASan corpus gate remains incomplete; the
existing full TSan gate and these bounded ASan controls found no new
production inconsistency, so no clean-master control or source change was
warranted.

## Permanent transaction relay size-bucket coverage (2026-07-26)

The original `p2p_tx_relay_rate_limit.py` test exhausted only the count
bucket with small transactions. The new
`p2p_tx_relay_rate_limit_size.py` scenario sets `-txsendrate=1000`, submits
140 standard 100,000-vbyte transactions to one inbound peer, and therefore
exercises the 12 MB serialized-size bucket before the count bucket can limit
the burst. It asserts that a backlog is created, advances mock time through
size-bucket refills, and requires the backlog and peer announcement set to
contain exactly the 140 submitted wtxids.

The test passed through `test_runner.py` in 29 seconds on the rebased branch.
This mutation was not covered by the existing rate-limit scenario or the
TokenBucket unit cases. The production behavior remained consistent; no
production fix, sanitizer finding, race, or clean-master comparison was
warranted.

## Rebased index concurrency gate (2026-07-26)

After rebasing onto `origin/master` at
`e34b8d5a7dcd45e4faa3bb5fdeae64bf049037f6`, the `baseindex_tests`,
`blockfilter_index_tests`, `txindex_tests`, `txospenderindex_tests`, and
`coinstatsindex_tests` suites were each run under the Clang 19 TSan unit
binary. All five suites passed without a race report. This includes the
explicit index reader schedule that calls `BlockUntilSyncedToCurrentChain()`
while repeatedly stopping and reinitializing the index, and the blocked
background-sync reorg schedule.

The two highest-risk schedules were then stress-repeated in four independent
TSan processes. Each process ran ten repetitions of both
`index_reinit_reader_race` and `index_reorg_crash`, for 40 repetitions of each
case overall. All 80 invocations passed with no TSan diagnostic, assertion,
timeout, or test failure. The run found no new index lifetime, notification
ordering, or reorg race on the rebased branch; no production or deterministic
test change was warranted.
