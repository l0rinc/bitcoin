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

- Pending: source/history contract inventory, targeted failure-state controls, and focused test execution.

## Verdict

- Pending.

## Handoff

- Pending completion. Record exact operation sequences, pre/post object bytes, callback behavior, minimized reproducer, and the next distinct state-machine cell.
