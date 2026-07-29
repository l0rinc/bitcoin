# Secp256k1 Nonce and Signing State Machine Cycle 60

## Identity and Gate

- Cycle: `60`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `84`
- Goal: `secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit`
- Slug: `secp-nonce-session`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `a064c61c3e77cbf08e9a68cd466f12cd50654ec3`
- `origin/master...HEAD` at the gate: `2 889`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- `goals.tsv` validation: `validated_rows=99 total_lines=100`
- No relevant test, fuzz, sanitizer, daemon, or profiling process was running at the gate.

The prior cycles covered MuSig secret temporary zeroization, a ctime declassification boundary, and a MuSig counter public-key check. This cycle selects the separate state-machine/output contract: what each API initializes, consumes, invalidates, or preserves on invalid inputs, failed callbacks, duplicate use, malformed opaque objects, and wrong operation order.

## Hypotheses and Scope

1. A rejected MuSig operation leaves a public output object looking valid or leaves a caller-seeded output stale despite an established sibling contract.
2. A failed or wrong-order operation consumes a secret nonce inconsistently with the documented single-use rule, permitting reuse or silently destroying a retryable state.
3. A malformed public nonce, aggregate nonce, session, keypair, or callback failure reaches stateful arithmetic or publishes partial state before returning failure.

Primary surfaces: `musig_nonce_gen`, `musig_nonce_gen_counter`, `musig_nonce_process`, `musig_partial_sign`, public/aggregate nonce parse/serialize, keypair tweak/sign helpers, Schnorr signing, and ECDSA nonce callbacks. The cycle must distinguish a documented caller obligation from a production state transition defect and must not repeat the closed ctime/zeroization cells.

## Evidence Log

- `secp256k1_musig_nonce_gen` is declared with `SECP256K1_ARG_NONNULL(3)` for `pubnonce`. Its implementation first validates and clears `secnonce`, then checks `session_secrand32` for an all-zero value and returns through `secp256k1_musig_secnonce_invalidate` before calling `secp256k1_musig_nonce_gen_internal`. The internal helper validates `pubnonce`, but that validation is unreachable when the session randomness is all zero.
- The existing `CHECK_ILLEGAL` callback contract provides an independent oracle: a call with `pubnonce == NULL` must invoke the configured illegal-argument callback exactly once. The new regression combines that invalid output pointer with an all-zero session-random buffer, the early-return path, and otherwise valid inputs.
- Old-source control: temporarily removing only the new `ARG_CHECK(pubnonce != NULL)` and rebuilding `build_unit_wallet_clang19` produced `musig_api_tests` failure at `tests_impl.h:282`, `test condition failed: _calls_to_callback == 1`; the process aborted with exit status 134. The observed callback count was zero, proving the old ordering bypassed the required argument validation.
- Fixed-source control: restoring the precondition, rebuilding target `tests`, and running `build_unit_wallet_clang19/src/secp256k1/bin/tests -t=musig_api_tests -i=1 -log=1` passed with exit status 0. The full corrected run `build_unit_wallet_clang19/src/secp256k1/bin/tests -i=16` completed 16 iterations in 76.329 seconds with exit status 0. `git diff --check` passed.
- The fix is intentionally limited to validation ordering. `musig_pubnonce_parse` and `musig_aggnonce_parse` publish only after both points parse; `musig_partial_sig_parse` explicitly zeroes its output on failure; `musig_nonce_process` publishes the session only after complete success; and `musig_partial_sign` consumes the secret nonce before later validation as documented protection against nonce reuse. These are distinct contracts and supplied no independent source defect in this cycle.
- Verdict: confirmed and fixed. The defect is an API validation/diagnostic contract gap for a caller supplying two invalid arguments, not a dereference or consensus break on the exact early-return path. The minimal fix is the precondition in `session_impl.h` plus the focused `CHECK_ILLEGAL` regression in `tests_impl.h`.

## Verdict

- Confirmed and fixed in the source commit recorded below. No additional state-machine defect met the evidence threshold this cycle.

## Handoff

- Source finding: `secp256k1_musig_nonce_gen` now validates the required public-nonce output before the all-zero session-random early return. The regression uses a null `pubnonce`, an all-zero 32-byte session-random buffer, valid key/session inputs, and requires one illegal callback.
- Validation: old-source mutation failed at `_calls_to_callback == 1` with exit 134; fixed focused test passed; fixed full libsecp test binary passed 16 iterations/76.329 seconds; `git diff --check` passed.
- Negative controls: no stale-public-output, nonce-consumption, parse, or post-failure publication defect was independently proven. Next run must recheck the gate, search this journal and history, and select a distinct catalog cell.

## Cycle 72: Failure, Retry, and Binding State Cell

### Identity and Gate

- Cycle: `72`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `84`
- Goal: `secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit`
- Slug: `secp-nonce-session`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goal TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Branch: `uber-cycle-72-secp-nonce-session-20260728`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `480832aabf6e86a9750dd0a788cccc7f74d0b4ec`
- `origin/master...HEAD` at the gate: `919 2`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- No relevant daemon, test, fuzz, sanitizer, Valgrind, or profiling process was running at the gate.

### Prior cells excluded

Cycle 60 already fixed MuSig invalid-argument ordering for `musig_nonce_gen` when both `pubnonce` and all-zero session randomness were invalid. Earlier cells covered MuSig secret zeroization and ctime declassification. This cycle excludes those findings and instead targets the state left behind by failures after a valid object has already been created.

### Hypotheses and scope

1. `musig_nonce_gen`, `musig_nonce_agg`, or `musig_nonce_process` may leave a caller-seeded output object looking valid after a failure, allowing a retry path or ignored return value to reuse stale public/session state contrary to a documented output contract.
2. `musig_partial_sign` may consume or retain its secret nonce inconsistently across wrong-order calls, malformed sessions, key mismatches, and invalid output arguments. The header explicitly promises nonce invalidation, so distinguish intentional security consumption from an accidental partial publication.
3. `musig_partial_sig_agg` and `schnorrsig_sign_custom` may publish stale or nonzero output after a callback, parse, or session failure, or may produce a valid-looking result that is accepted by a later verification operation.
4. Duplicate public keys, duplicate public nonces, infinity aggregate nonces, zero/overflow scalars, and mismatched keyagg/session/message objects may create a state transition that succeeds when the documented MuSig binding should reject it.

Evidence must include exact pre/post bytes and return values, a valid end-to-end signing control, minimized malformed inputs, and an old-source or temporary mutation control for any claimed defect. A stale output without a stated API contract remains an inconclusive misuse observation, not a production finding.

### Initial execution plan

Build the standalone libsecp256k1 tests with Clang 19 and run the focused MuSig/Schnorr suites. Add a disposable C++ state-machine probe under `agent-journal/` using initialized sentinels and serialized round trips. Exercise valid two-party signing, failed nonce generation, malformed aggregate/session objects, wrong key/message bindings, repeated partial signing, invalid custom nonce callbacks, duplicate participants, and infinity nonce sums. Compare output bytes before and after every failure, verify that successful controls still produce BIP340-valid signatures, and apply temporary source mutations only to prove an oracle can detect a suspected contract violation.

### Cycle 72 Evidence and Verdict

- The disposable public-API probe is `agent-journal/secp_nonce_cycle72_probe.cpp`. It first establishes a valid two-party MuSig flow and BIP340 verification, then checks all-zero session randomness, invalid secret keys, null output through the library's illegal-argument path, wrong-key and invalid-session binding, single-use nonce consumption, failed and zero custom Schnorr nonces, malformed cache/nonce/signature pointer arrays, infinity aggregate nonce processing, duplicate public keys, duplicate-key signing, and exact caller-sentinel preservation.
- The normal Clang 19 `-O2` probe passed all checks. The first infinity assertion was corrected as a harness mistake: the initial version passed the signer's own nonce while describing it as unrelated; the final version uses the other signer's nonce and passes the rejection check.
- The probe was linked once against the existing Clang 19 ASan/UBSan libsecp256k1 archive and passed under `ASAN_OPTIONS=halt_on_error=1:detect_leaks=0:allocator_may_return_null=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. The deliberate null-output call initially triggered UBSan at the annotated probe call site; routing that negative test through an unannotated function pointer reached the library's intended `ARG_CHECK` path and produced no sanitizer diagnostic.
- `build_unit_clang19/src/secp256k1/bin/tests -t=musig_api_tests -i=1 -log=1` and `-t=test_schnorrsig_api` passed. The combined `-t=musig -t=schnorrsig -i=4 -j=2 -log=1` matrix passed all enabled tests. The full `build_unit_clang19/src/secp256k1/bin/tests -i=16 -j=2 -log=1` run passed with exit status 0 in 44.534 seconds. `git diff --check` passed.
- Static review confirms the observed transitions are intentional: `musig_partial_sign` zeroes the secret nonce before later validation as documented; early `nonce_gen` failure leaves the public output unspecified but does clear the secret nonce; parsers publish only after successful parse or explicitly zero on failure; `nonce_process` publishes only after complete validation; partial-signature aggregation writes only after all terms load; and Schnorr custom-nonce failure clears its signature output. Duplicate keys and encoded infinity aggregate nonces are accepted by the documented/internal multiset and exceptional-point rules, while unrelated signer nonces fail verification.
- Verdict: dismissed for a new source defect. No stale-output, nonce-reuse, malformed-binding, duplicate-key, infinity, or custom-callback contract violation met the evidence threshold. No production source change is justified. Raw logs are under `/data/my_storage/tmp/secp-nonce-cycle72-*.log`; the probe source is retained for the handoff.

### Cycle 72 Handoff

- Cycle complete on branch `uber-cycle-72-secp-nonce-session-20260728`; no relevant process remains running.
- Next run must re-check the gate, search this journal and prior cycle 60 evidence, and draw a distinct goal from the full catalog rather than reopening these closed state cells.

## Cycle 95: Current nonce/session state cell

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `84`
- Goal: `secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit`
- Slug: `secp-nonce-session`
- Branch: `uber-cycle-95-secp-nonce-session-20260729`
- Start HEAD: `beedbcc175bd90e749a7ee9d444687ec7b6167b4`
- `origin/master`: `9b38d077f894d27ea76413b1db1cb040e25dc296`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `29 980` (`origin/master...HEAD`)
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; no relevant Bitcoin Core or libsecp test process was running. Existing untracked artifacts were preserved and are excluded from commits.

### Scope ledger

Audit nonce, signing, key-tweak, ECDH, extrakeys, Schnorr, and MuSig state machines in the vendored libsecp256k1 subtree. Prioritize secret reuse, state reuse, invalid ordering, duplicate participant/key handling, malformed or boundary scalars, callback failure, randomized-context behavior, and output-on-failure contracts.

Exclude the completed secp field/scalar backend matrix, group/ecmult formula audit, BIP324 vector refresh, and already recorded secret-lifetime or timing cells unless a new state-machine path supplies independent evidence. Do not infer a defect from a different API's convention. Establish each module's documented contract from headers, tests, callers, history, and upstream review before changing behavior.

### Working protocol

For every candidate, record the exact API and module, state variables and valid transitions, secret/public classification, caller and trust boundary, relevant history or external report, and expected output/state after success and failure. Use production APIs through existing module tests or a minimal harness; avoid hand-built parallel models that do not exercise the implementation.

Test valid sequences plus invalid order, reuse, duplicate participant/key, zero and overflow scalar, malformed serialization, cancellation, callback failure, randomized-context replay, and output-on-failure cases. Preserve minimal sequences and deterministic seeds. For a confirmed issue require a failing-before/passing-after test, minimized state sequence, first-invalid trace, or a rigorous contract proof; use two independent verifier forms for nonce, key, wallet, or remotely reachable impact when practical.

Prefer one self-contained commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, with the journal update. Keep diffs minimal, test every commit alone, and do not weaken checks or hide failures with catches, narrower inputs, or assumptions. If no fix is justified, record the exact negative evidence and next distinct queue rather than manufacturing a change.

### Initial queue

1. Compare ECDSA and Schnorr signing output/error paths, including nonce callback failure and zero/invalid nonce handling.
2. Trace MuSig nonce generation, commitment, aggregation, session initialization, partial signing, and repeated-use transitions.
3. Audit extrakeys/tweak and ECDH state/output behavior after invalid keys, overflow tweaks, or callback failures.
4. Compare module tests, `VERIFY`/checkmem behavior, and public headers for output-on-failure and single-use state guarantees.
5. Search recent upstream history and PRs only after local state transitions identify a concrete contract gap.
