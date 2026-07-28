# Historical Knowledge Recipe Synthesis

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
