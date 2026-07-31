# Historical Knowledge Recipe Synthesis: Cycle 120

## Cycle 176 Identity and Gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `60`
- Selected goal: `historical-knowledge-recipes` (Whole-PR and commit knowledge-base recipe synthesis)
- Branch: `uber-cycle-176-historical-knowledge-recipes-20260730`
- Start HEAD: `7809069a54de58c8e6277cf0e6ec866946d17a74`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence (`origin/master...HEAD`): `42 1135`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc`
- Tracked/index state was clean at the gate; known untracked agent artifacts were preserved.
- Preserved unrelated long-running tests: PIDs `777094` and `956381`; neither was modified.
- Exact selector result: `shuf -i 0-98 -n 1` -> `60`; no reroll was needed because the prior closed draw was `80`.

## Cycle 176 Scope and Exclusions

Mine a technically grounded reviewer recipe from an upstream whole-PR or commit sequence, including accepted and rejected approaches, then validate it against an independent held-out change and at least one negative control. This cycle opens the distinct fingerprint `resource-owning-move-operation-contract`: resource-owning move assignment must define release, transfer, aliasing, and destruction semantics before it is exposed; if no correct supported use exists, deleting the operation is preferable to shipping a partial RAII implementation. The held-out check will use `btck::Handle` self-move assignment, and the negative control will be a non-owning/value-only refactor or a copy-only API that has no resource-transfer contract.

Exclude the closed recipes `presence-vs-verification-before-assertion`, `reservation-conservation-after-deferred-eligibility`, `configurable-parallel-feature-lifecycle`, `provenance-aware-terminal-state-accounting`, `actionable-interface-minimal-schema-boundary-realism`, `resource-bound-backlog-duplicate-suppression-retention-telemetry`, and all previously harvested reviewer-preference cells. Also exclude prior local DynSock ownership, CNode refcount, error-path rollback, exact-helper-reuse, and generic lifetime findings as primary seeds; they may only serve as controls after the upstream evidence is independently established.

Initial queue:

1. Inspect upstream PR #35120 and its commit history/review text for the broken `scoped_connection` move-assignment contract, discarded implementation, and accepted deletion.
2. Inspect upstream PR #35143 and its tests as a held-out ownership/aliasing case; verify self-move leaves every public handle type valid and unchanged.
3. Search current resource-owning wrappers and move-assignment operators for an untested aliasing or release contract, then record any candidate as confirmed, dismissed, or inconclusive before considering a production change.
4. Validate the recipe on focused current tests and mutation-sensitive reasoning; keep this cycle journal-only unless a new defect has its own independent reproducer and self-contained source/test commit.

No Cycle 176 journal evidence or source/test changes have been made after this start record.

## Cycle 120 Identity and Gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `90`
- Selected goal: `historical-knowledge-recipes` (Whole-PR and commit knowledge-base recipe synthesis)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `uber-cycle-120-historical-knowledge-recipes-20260730`
- Start HEAD: `885645974b844071416088ef7d842808089777a3`
- Base: `origin/master` at `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- `origin/master...HEAD` at the gate: `40 1029`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Tracked and staged state at the gate: clean; persistent untracked artifacts and `test/cache/` were preserved.
- PID `777094` (`test_bitcoin --run_test=wallet_tests`) and parent PID `725042` were observed and will not be touched.

## Cycle 120 Scope and Exclusions

The existing recipes are closed: `presence-vs-verification-before-assertion`, `reservation-conservation-after-deferred-eligibility`, and `configurable-parallel-feature-lifecycle`. This cycle must select a different bug shape, derive a compact trigger/invariant/review recipe from a whole PR or commit sequence, and test it against a held-out change from another subsystem. A recipe-only result is acceptable when the current tree satisfies the extracted contract; any production finding requires an independent reproducer and its own self-contained commit.

## Cycle 120 Evidence Log

### Seed: compact-block slot provenance and counter conservation

The selected seed is Bitcoin Core PR `#35727`, merge `883ef1d85d74c928ae753e07116d3fc61de4e446`. Its source fix is `6aa5d8d9481f5e06b10095df7f46f0532f7ecdb7` (`blockencodings: fix extra transaction count`) and its characterization test is `be4e64d9e4051f7272c75c2819ceb075ed5452c7` (`test: characterize extra transaction miscount`). The merge history and review record show a focused two-stage correction rather than a broad refactor.

The contract table is:

| State/source | Slot meaning | Counter effect | Can a later candidate refill it? |
| --- | --- | --- | --- |
| `NONE` | no candidate has claimed the short-ID slot | none | yes |
| `MEMPOOL` | a transaction came from the mempool | increment `mempool_count` | only if invalidated by the source-specific collision rule |
| `EXTRA` | a transaction came from `extra_txn` | increment both `mempool_count` and `extra_count` | only if invalidated by the source-specific collision rule |
| `COLLIDED` | the slot is ambiguous or invalidated | counters already decremented for its prior source | no |

The old `vector<bool> have_txn` retained presence but discarded provenance. Consequently, an unrelated `extra_txn` could first claim a slot, a later mempool short-ID collision could clear it, and the code could decrement the aggregate count as if the cleared entry had come from the mempool. The repair adds a parallel `TxSource` vector, decrements `extra_count` only when the invalidated source was `EXTRA`, and makes `COLLIDED` terminal so a later candidate cannot silently refill a slot that has already affected accounting.

The first test commit deliberately leaves one block transaction missing so scanning reaches the collision. It records the original mismatch (`extra_count == 0` where the contract requires `1`) rather than immediately choosing an implementation. The follow-up test expands the matrix: mempool-sourced collision, extra-sourced collision, and genuine transactions arriving after a collision. The final assertions cover both counters, availability, and the no-refill rule. This is the useful review pattern: characterize the state transition first, then mutate each provenance and terminal-state branch.

### Commit, review, and rejection evidence

The source commit states the mechanism and impact precisely: a short-ID collision can invalidate a mempool-sourced transaction after an unrelated transaction was found in `extra_txn`, causing the extra count to drift. It also records why retaining collision state matters for later candidates. The test commit is separate, which makes the regression oracle independently inspectable. The merge body records ACKs from `l0rinc` after retesting, `andrewtoth`, and `sedited`; no review evidence supports replacing the source enum with an aggregate-only adjustment.

The rejected designs are therefore concrete. Keeping only a boolean preserves occupancy but cannot answer which counter to decrement. Decrementing `extra_count` for every collision fixes one ordering but breaks the mempool-source case. Clearing the slot and allowing a later candidate to refill it hides the original collision and can double-count or resurrect a state that the compact-block reconstruction no longer knows how to classify. The minimal accepted design is explicit provenance plus a terminal collision state, with focused tests for each transition.

### Held-out validation: private-broadcast removal and disconnect state

The independent held-out change `13da611b396664bfc63587f5eeac3bdc1ce1b163` (`privatebroadcast: track disconnects for removal accounting`) matches the extracted recipe in another subsystem. Its old `Remove()` result exposed only confirmed receptions, although a picked but still unconfirmed connection had already consumed a counter slot. The repair preserves per-send provenance with `num_picked`, `num_confirmed`, and `num_unconfirmed_disconnected`, derives `NumUnstarted()` from consumed slots, and tombstones active node IDs until the in-flight removal is resolved. `MarkNodeDisconnected()` is idempotent and distinguishes a removed active node from a live pending send.

The analogy is behavioral, not lexical: both systems have a bounded aggregate, multiple source/status classes, a later invalidation event, and a terminal state that must not be re-used. The held-out unit mutations are sensitive to both halves of the recipe: changing `NumUnstarted()` back to confirmed-only fails the count assertions, while skipping tombstones fails the node-disconnect assertions. The current focused binary then passed both relevant suites:

```text
TMPDIR=/data/my_storage/tmp/cycle120-historical-recipe \
  /data/my_storage/tmp/cycle89-build/bin/test_bitcoin \
  --run_test=blockencodings_tests,private_broadcast_tests \
  --report_level=short --catch_system_errors=no

Test module "Bitcoin Core Test Suite" has passed with:
  40 test cases out of 1213 passed
  1173 test cases out of 1213 skipped
  20876 assertions out of 20876 passed
```

The binary is configured against `/data/my_storage/bitcoin`, and its timestamp is newer than the current private-broadcast test source. The current tree also contains the compact-block collision and no-refill assertions at the inspected source locations. No source change is justified by the held-out review.

### Negative controls and fingerprint boundary

`a39a6d7cafc54a5fba403a74710f141650d9a4f8` (`coins: check failed spend no-op contracts`) is not a match: it protects whole-operation rollback and has no deferred candidate source or aggregate reservation. `4ea3901a8834a85d0fb84d79f0a8398284ae013f` (`mempool: check dump failure preserves file`) likewise protects atomic file replacement, not provenance-aware counter ownership. These controls prevent the recipe from degenerating into “any state-accounting regression.”

The recipe fingerprint is `provenance-aware-terminal-state-accounting`. It is related to, but distinct from, the earlier `reservation-conservation-after-deferred-eligibility` recipe: that recipe starts with resources reserved before recipient eligibility is known, while this one starts with aggregate counts that cannot be interpreted correctly after a source-specific collision or lifecycle transition. A future candidate needs both explicit source/status classes and a proof that terminal states cannot be counted or selected again.

### Reusable recipe

1. List every per-item state that contributes to each aggregate counter. If two sources or lifecycle statuses have different decrement/refund rules, an occupancy bit is insufficient.
2. Write conservation equations before changing code. For each transition, state the prior source, the event that invalidates it, the exact counters to change, and whether the item becomes terminal.
3. Search for all candidate orderings: source A then source B, B then A, duplicate candidates, late candidates, missing candidates, removal, disconnect, retry, and node/object reuse.
4. Replace inferred provenance with an enum, tagged result, or equivalent status carried alongside the item. Keep terminal collision/removal/tombstone states explicit and non-refillable until their lifecycle is complete.
5. Add a characterization fixture for the first divergence, then a mutation-sensitive matrix that changes each source-specific decrement and terminal-state guard. Assert counters, availability, output selection, and post-event behavior together.
6. Validate the recipe against a held-out subsystem and at least one negative control. Preserve exact source commits, review rationale, commands, and limitations in the knowledge base.

### Cycle 120 verdict

**Recipe confirmed; no new production defect found on current HEAD.** PR `#35727` supplies the source/provenance invariant and the staged characterization-to-fix test sequence. `13da611b396664bfc63587f5eeac3bdc1ce1b163` is a positive held-out match, and the coins/dump-failure changes bound the fingerprint. This cycle requires a journal-only close and no implementation commit.

## Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `90`
- Selected goal: `historical-knowledge-recipes` (Whole-PR and commit knowledge-base recipe synthesis)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD: `1dcc2da988ee625fbc5d7d55eb6f894c1103ec52`
- Catalog: `agent-journal/reusable-continuous-agent-goals.md`

## Seed PR and Contract

The selected history seed was merge `5311b15727f2f282274472184185423e441abd85`, PR `#33014`, with top commit `7e19ce200b2e65770907a818b02e4ec3da9c5374`: `rpc: Fix descriptorprocesspsbt internal bug on invalid signatures`. The merge metadata records issue `#32849`, ACKs from `achow101` and `rkrux`, and a one-commit PR.

The falsifiable recipe hypothesis was: when an RPC decides that an artifact is complete and then calls a finalizer/extractor with a hard assertion, a presence-only completeness test is unsafe. A finalized-looking PSBT can contain non-empty final fields whose signatures do not verify. The contract must be “all final fields verify under the transaction's actual UTXOs and sighash rules,” not merely “some final fields are present.”

The trust boundary is a PSBT supplied through the public RPC. The failure path is an invalid final witness reaching `FinalizeAndExtractPSBT`; the expected public behavior is `complete: false`, no extracted `hex`, and no internal assertion/crash.

## Commit/PR Evidence

The merge diff was deliberately small and self-contained:

- `src/rpc/rawtransaction.cpp`: 8 changed lines. It replaces `PSBTInputSigned(input)` with one `PrecomputePSBTData(psbtx)` object and `PSBTInputSignedAndVerified(psbtx, i, &txdata)` for every input. The subsequent `FinalizeAndExtractPSBT` call remains gated by `if (complete)`.
- `test/functional/rpc_psbt.py`: 44 added lines. The regression first proves a valid descriptor PSBT completes, unloads the wallet, flips a bit only in the serialized taproot final signature, and asserts the mutated PSBT returns `complete == false` and has no `hex` field.
- `test/functional/test_framework/util.py`: a small reusable `bitflipper` helper. The framework PRNG seed makes the mutation replayable.

The implementation reuses the wallet path's verification helper instead of adding a second signature-validation implementation. `src/psbt.cpp:552-555` documents the old presence check; `src/psbt.cpp:557-600` performs actual UTXO/script verification; `src/psbt.cpp:626-645` constructs shared precomputed transaction data. The current extractor also protects its assertion path through `FinalizePSBT`, and the RPC caller only extracts when its verified `complete` flag is true.

The PR description identifies the rejected/unsafe design precisely: `PSBTInputSigned` only inspected non-empty final fields, which let invalid Schnorr bytes and unusual sighash combinations look complete. The fix does not broaden parsing or suppress the assertion; it restores the contract at the boundary before the assertion can be reached. No benchmark or platform-specific caveat was relevant to this correctness fix.

## Verification

The focused functional method was first invoked without the generated build config and failed before setup because `test/config.ini` was absent. A retry with `build_func_clang19/test/config.ini` correctly skipped because that build has wallet support disabled. Neither result is treated as a product failure.

The same method then ran against the wallet-enabled Clang 19 build with isolated scratch data, fixed ports, and fixed PRNG seed:

```text
python3 test/functional/rpc_psbt.py \
  --configfile build_unit_wallet_clang19/test/config.ini \
  --test_methods test_psbt_with_invalid_signature \
  --tmpdir /data/my_storage/tmp/rpc-psbt-recipe-wallet \
  --portseed 31991 --randomseed 12345 --loglevel INFO
```

Result: exit status 0, `Method 'test_psbt_with_invalid_signature' executed successfully`, `Tests successful`. The framework cleaned the scratch node directory; the raw log is preserved at `/data/my_storage/tmp/rpc-psbt-recipe-wallet.log`. No daemon or test process remains.

As a held-out dry review of the resulting contract, all current `FinalizeAndExtractPSBT` call sites were searched. The RPC call in `src/rpc/rawtransaction.cpp` is guarded by `complete`, while `FinalizePSBT` returns false before extraction when signing/verification cannot complete. This is the reusable call-site check the recipe requires for future APIs.

## Reusable Recipe

When reviewing an API that classifies an object as complete before a failure-prone finalizer, apply this sequence:

1. Write the postcondition the finalizer assumes, including cryptographic or semantic validity, not just field presence.
2. Find every completeness predicate and every extraction/assertion call; compare them with the strongest existing path, usually the wallet or verifier path.
3. Compute shared transaction/context data once and call the existing verification helper for every relevant input.
4. Construct a valid fixture through the public workflow, then mutate only the smallest serialized field that invalidates the semantic proof while preserving the apparent “finalized” shape.
5. Assert the public incomplete/error result, absence of extracted output, unchanged safe state, and process survival. Run the path with internal providers unloaded or bypassed when the interface is meant to be independently usable.
6. Keep the production diff narrow, preserve the existing assertion as a defensive invariant, and add the regression at the public boundary so future refactors cannot replace verification with presence checks.

This recipe generalizes to signed messages, descriptors, authentication tokens, persisted records, and any API where “non-empty” is weaker than “verified.”

## Verdict and Next Queue

**Recipe confirmed; no new production defect found on current HEAD.** The historical PR provides a complete invariant, mechanism, rejected approach, minimal fixture, reviewer evidence, and a passing focused regression. No implementation commit is warranted in this cycle.

Next queue: draw another eligible goal; later recipe cycles should extract a different bug shape, deduplicate this recipe by the `presence-vs-verification-before-assertion` fingerprint, and validate it against a held-out PR involving a different public boundary.

## Cycle 19: Reservation-Conservation and Deferred-Delivery Accounting

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `90`
- Selected goal: `historical-knowledge-recipes` (reopened only because new history supplied a distinct bug-shape cluster)
- Gate: fresh `origin/master` fetch succeeded; base `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; HEAD before this journal cycle `ec4401b816f132f2f35c1f2e64cf51e2046e8e32`; tracked source clean; `git diff --check` passed; no active build, daemon, fuzz, or test process.
- Prior-recipe exclusion: the cycle-4 `presence-vs-verification-before-assertion` recipe was not reused. The new cluster concerns reservations made before a destination is known to accept or receive work.

### Seed cluster and evidence

The current history contains a consecutive relay-accounting campaign whose exact clean-master controls expose the same conservation failure at different deferred-delivery exits:

- `68b745e2084dc553debcee59f43f6318e5c63978`, `net: preserve relay budget for known transactions`: `ProcessInvBacklog()` reserved count and serialized-size tokens before known-inventory filtering. Two peers already knowing 30 transactions consumed the budget without receiving INV messages; the following transaction remained delayed.
- `a13956d2a83c9370e74e9f133c1bbaca6b643f7f`, `net,fuzz,test: refund relay budget without eligible peers`: a no-peer case consumed both reservations even though the selected transaction had no recipient.
- `81518c1dc174a5850ec7611660af8dd9569cfb0b`, `net,fuzz,test,doc: refund relay budget for filtered peers`: a BIP37 filter rejected delivery after reservation, consuming capacity without an INV.
- `dccab6b25a1a4e7622bdd162be6177d5d4072f2c`, `net,fuzz,test,doc: refund relay budget for fee-filtered peers`: a BIP133 fee filter produced the same count/size drift; the fix also preserved selection-time payment for a stale entry removed before extraction, which is a distinct intentional-consumption case.
- `ade589fd6a98437c9d847f84b20b9072c815ee31`, `net,fuzz,test,doc: exclude non-relaying peers from relay budget`: a handshake-complete `relay=0` peer had a relay object but could not receive queued announcements, so both global buckets were consumed without an eligible recipient.
- `ce7258e1c9d02ff595e1f07d00a4bb111703e1da`, `test: cover outbound transaction relay rate bucket`: held-out coverage showed that arithmetic tests and inbound coverage did not exercise the live outbound multi-peer scheduling path.

The common mechanism is not merely “a counter was wrong.” It is a lifecycle mismatch: a resource reservation is made at selection time, while eligibility, delivery, disconnect, or extraction is resolved later. The correct accounting depends on whether the reservation paid for a selected attempt, was rejected before any eligible attempt, or was already consumed by an in-flight operation.

### Reusable recipe

When reviewing a resource-limited state machine with deferred destinations, apply this sequence:

1. Identify every reservation point and name each resource dimension separately, such as count, bytes, descriptors, queue slots, or connection attempts.
2. Trace the reservation through selection, recipient construction, filtering, enqueue, send, acknowledgement, disconnect, removal, retry, and cleanup. Search for a global bucket plus per-destination queues rather than auditing either layer alone.
3. Enumerate every post-reservation exit: known destination, empty destination set, disabled relay/permission, policy filter, stale or missing object, disconnect, shutdown, cancellation, and retry. For each exit, state whether the reservation is refunded, transferred, or intentionally retained.
4. Define conservation equations for each resource and assert both directions. For example, rejected-before-delivery must restore count and size, while a selected stale entry can intentionally retain a selection-time reservation. Assert backlog, per-peer queues, in-flight state, and retry/tombstone state together.
5. Exercise inbound and outbound paths, multiple recipients, zero/one/many destinations, policy changes after selection, node-id reuse, and disconnect/removal ordering. Use a deterministic wire or stateful-fuzz trace, not only isolated token arithmetic.
6. Build the exact clean-master control and record the first observable divergence, expected severity, and whether the issue is local availability, privacy, policy, consensus, wallet, or memory safety. Do not inflate a local relay starvation issue into a remote DoS claim.
7. Fix the ownership boundary with one shared predicate or result type, preserve lock ordering, and add a mutation-sensitive regression for both the refund and intentional-consumption cases. Re-run the original control, focused unit/fuzz tests, and a sanitizer or race configuration where the state machine is concurrent.

### Held-out dry review

`13da611b396664bfc63587f5eeac3bdc1ce1b163`, `privatebroadcast: track disconnects for removal accounting`, independently matches the recipe. Its old `Remove()` path used confirmed receptions to cancel connection-counter slots even when a selected connection was still open and unconfirmed. The fix tracks picked, confirmed, and unconfirmed-disconnected statuses, returns `NumUnstarted()`, tombstones active nodes removed with a transaction, and makes later disconnect handling idempotent. The deterministic unit mutation catches both the wrong counter formula and missing tombstones; the guided fuzzer adds the same result/count oracle. This is a confirmed positive held-out match with a different subsystem and a disconnect/removal exit.

`a39a6d7cafc54a5fba403a74710f141650d9a4f8`, `coins: check failed spend no-op contracts`, is a useful negative control. It verifies that a failed coin spend preserves cache size, usage, dirty count, and a caller-provided output object. There is no deferred reservation or destination eligibility decision, so the new recipe does not claim it as a match. It belongs to the separate `failed-operation-must-be-local-no-op` fingerprint.

`4ea3901a8834a85d0fb84d79f0a8398284ae013f`, `mempool: check dump failure preserves file`, is another boundary control: the `.new` file and rename sequence already implements atomic replacement, and the commit adds a preservation oracle without a production change. It shares rollback/conservation vocabulary but not deferred destination accounting, so it remains outside this recipe.

### Verdict

**Recipe confirmed; no new production defect found on current HEAD.** The relay sequence supplies five independently demonstrated post-reservation exits, while private-broadcast removal/disconnect provides an independent held-out match and the coin-spend/dump-failure commits prevent overgeneralization. No implementation commit is warranted in this cycle. The recipe is indexed by `reservation-conservation-after-deferred-eligibility` and is distinct from `presence-vs-verification-before-assertion`, `failed-operation-must-be-local-no-op`, and wallet rollback.

Next queue: draw another eligible goal. If goal 90 is drawn again, select a different history cluster and validate it against held-out commits rather than appending another relay-accounting variant.

## Cycle 31: Configurable Parallel-Feature Lifecycle and Fallback

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `90`
- Selected goal: `historical-knowledge-recipes` (reopened on a third, distinct history cluster)
- Gate: fetched `origin/master`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` was `2 834`; HEAD before this handoff was `0b265d3668853de9d679c2a8e1532ef7087e3b6`; tracked and staged state was clean; no source, build, test, daemon, or fuzz process remained active.
- Prior-recipe exclusions: this is neither the cycle-4 `presence-vs-verification-before-assertion` recipe nor the cycle-19 `reservation-conservation-after-deferred-eligibility` recipe. It targets configuration-driven concurrency and fallback semantics.

### Seed history and evidence

The seed cluster is Bitcoin Core PR `#35295`, merge `c0e91efdb31fa930593f61cc87464e94c9f1ac72`, `validation: fetch block input prevouts in parallel during ConnectBlock`. Its ten-commit history supplies a complete migration trail:

- `5bf1c32008173db2080286b2690f4951299d1619` registers `-prevoutfetchthreads`, stores the typed option, rejects negative values, and clamps larger values to `MAX_PREVOUTFETCH_THREADS`.
- `f82043af507a2f2caacdae1af6bcacddc8c4876b`, `ede11b83141d0d0998cd95ebabf8d1656c6f7765`, `fdf283036a1e16f546f96ca9c2d6d33f3a4fea56`, and `ab2a3792372c6b99b9d6749a1841dbb363264573` carry the option into the overlay, prefetch state, ready-state synchronization, and concurrent fetch implementation.
- `d69a3b20deca56ff8f925d93471d4caeacaa4d21` updates the ownership/lifecycle documentation; `760fb22dc370b0882bd345ff913f9337a9b6e4c1` adds `StartFetching` unit coverage; `ce610a6ff445bb8a812e650c91f501a1ecf0b19c` and `0e10937184438f8d81940336b183267e59f959b7` expand fuzz reachability; and `dc1c17c0856e3455a0d62f9ffd807c0d14feff62` records the migration in release notes.

The PR description and review record establish the intended contract: the feature only parallelizes existing UTXO reads, does not change validation or consensus, defaults to eight workers, permits at most sixteen, and uses zero to restore serial behavior. The safety argument depends on concurrent LevelDB reads, `ConnectBlock` remaining under `cs_main`, only `ProcessInput` running concurrently, and `Reset`/`Flush`/`StopFetching` joining workers before state is reused. ACKs from `l0rinc`, `willcl-ark`, `theStack`, and `ryanofsky` include the final `StopFetching` and `AllInputsConsumed` lifecycle changes, which are evidence that review focused on shutdown and completion semantics rather than only the option parser.

The reusable contract cells are therefore: public registration/help and release documentation; typed option storage; default, zero, negative, and above-maximum parsing; runtime wiring; serial fallback; worker start/ready/stop/reset/flush behavior; lock and object-lifetime assumptions; normal tests; fuzz state coverage; and representative performance evidence. The defect shape to mine in future migrations is a split contract where an option is accepted but one of those cells still assumes the old fixed behavior.

### Held-out validation and controls

`c7af7477b106e321d161f97c4893373426fc152b`, `validation: add -inputfetchthreads configuration option`, is a useful negative control. Its standalone four-file diff adds help text, typed storage, and a parser that rejects negative values and clamps to sixteen, but it adds no runtime consumer and no test. It is not evidence of a current production bug because it is a staged historical option commit on a detached branch; it demonstrates why a recipe must not treat parser coverage as feature completion. Its later branch context was not used as a production oracle.

The later local follow-up `3873d90f06df3cbec47277b5c48778bdbcd03219`, `fuzz, test: cover prevout-fetch worker count boundaries`, is a second control. It adds a shared fuzz input provider for zero/one/default/maximum workers, a direct overlay boundary test, and bounded stacked-coins/cache simulations. The recorded ASan/UBSan and TSan slices found no production defect, but the follow-up proves that the initial unit coverage did not exercise every worker-count state. It is retained as evidence for the coverage cell, not treated as upstream review evidence.

### Verification

The current chainstate option tests were rerun independently after the cycle gate:

```text
build_unit_clang19/bin/test_bitcoin --run_test=validation_chainstatemanager_tests --report_level=short

Test module "Bitcoin Core Test Suite" has passed with:
  22 test cases out of 1189 passed
  1167 test cases out of 1189 skipped
  2068 assertions out of 2068 passed
```

The current tree uses the later `-prevoutfetchthreads` option rather than the staged `-inputfetchthreads` name. History and source searches found the option registration, typed storage, parser, runtime worker lifecycle, tests, fuzz harnesses, and release documentation in the PR cluster; the branch's existing boundary follow-up covers zero, one, default, and maximum worker selections. No current source change is justified by this recipe cycle. Raw command output and the catalog/protocol hashes are represented in the uber-goal state handoff.

### Reusable recipe

When reviewing a configurable parallel or asynchronous feature, apply this sequence:

1. Build a lifecycle matrix before reading the diff: registration/help, typed option, parser domain, default/zero/max/negative behavior, runtime consumer, worker ownership, completion, shutdown, reset/flush, documentation, tests, fuzz reachability, and held-out performance evidence.
2. Prove the fallback contract first. The disabling value must reproduce the old serial behavior, while default and maximum values must exercise the new path without changing externally visible correctness or consensus semantics.
3. Trace every boundary where the option becomes a thread pool or scheduler: start, ready, concurrent work, cancellation, error propagation, join, object reuse, and destruction. Treat comments and ACKs about stop ordering as contract evidence.
4. Test parser values at zero, one, default, maximum, maximum plus one, negative, and the largest representable input. Distinguish deliberate clamping from rejection and verify the typed conversion cannot overflow before the clamp.
5. Add behavior-sensitive unit tests plus stateful fuzz inputs that actually select each worker mode. A test that only constructs the pool or reaches a branch is insufficient; assert output, error, state, and shutdown behavior.
6. Separate performance claims from correctness. Record storage/CPU environment, compare serial and parallel paths on held-out workloads, and reject a feature if its speedup depends on an unbounded queue, altered validation rule, or unverified lifetime assumption.
7. Use staged option-only commits as negative controls during review. If a change has help and parsing but no consumer, tests, or fallback proof, keep it in the migration queue rather than declaring the feature complete.

### Verdict and next queue

**Recipe confirmed; no new production defect found on current HEAD.** PR `#35295` supplies the complete lifecycle and independent review evidence; `c7af7477` demonstrates the incomplete-parser-only control; and `3873d90` demonstrates why worker-count boundary coverage must be added separately. The focused chainstate tests passed all 2068 assertions. No implementation commit is warranted in this history-only cycle.

Next queue: draw another eligible goal. If goal 90 repeats, select a fourth history cluster unrelated to PSBT verification, relay reservations, and configurable parallel features, and preserve this recipe under the fingerprint `configurable-parallel-feature-lifecycle`.
