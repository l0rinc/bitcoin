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
