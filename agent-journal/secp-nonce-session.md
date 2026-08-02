# Journal: secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit (campaign 84)

Uber-goal rotation, severity-first: NONCE REUSE = KEY LOSS (highest secp class).
Branch: audit/secp-nonce-session from audit/resurrection @ 88ded8e1a2.
Vendored subtree (9caae50682). Host: aarch64; tests green on both backends (#82).
Prior: secp256k1 PR25 (opaque sig overflow) reviewed — hardening, not live.

## Scope ledger

| # | area | hypothesis seeds | verdict |
|---|------|------------------|---------|
| N1 | ECDSA nonce (RFC6979) | deterministic nonce derivable by caller misuse (NULL/aliased inputs, entropy XOR, counter) — API guards vs footguns | open |
| N2 | Schnorr nonce (BIP340 tagged hash) | aux-randomness optional path; nonce function input domain edges (zero key, n key, malformed) | open |
| N3 | signing output-on-failure | sig buffer state on failure paths; context capability enforcement | open |
| N4 | MuSig session state machine | single-use session enforcement, commitment binding, nonce reuse across sessions, partial-sig reuse | open |
| N5 | MuSig edge inputs | duplicate participant keys, zero/overflow scalars, cancellation paths | open |

## Verdicts

### N1/N2 (ECDSA RFC6979 + Schnorr BIP340 nonce derivation): DISMISSED by construction

Both nonce functions are deterministic-hedged derivations bound to
(msg, key, extra): RFC6979-HMAC-SHA256 (secp256k1.h:676-697) and the
BIP340 tagged hash. Same (msg,key,ndata) → same nonce → IDENTICAL sig
(no leak); different msg → different nonce. The only failure mode is a
caller-supplied custom noncefp — a documented caller-responsibility
boundary, not a library defect.

### N3 (signing output-on-failure + context capability): DISMISSED

Sign APIs return 0 on nonce-fn failure or invalid key with the signature
object unwritten (documented); context capability is checked via
ARG_CHECK + context-builtin asserts throughout the signing paths
(observed in partial_sign 641-657: VERIFY_CHECK/ARG_CHECK on every
entry).

### N4 (MuSig session state machine): DISMISSED — single-use enforced by zero-before-checks

partial_sign (session_impl.h:632-700): loads the secnonce scalars, then
IMMEDIATELY memzero_explicit's the caller's secnonce struct (648) BEFORE
any failure check — every subsequent call with that struct fails the
magic check (644-649). Nonce reuse through the API is impossible even on
error paths; the struct is destroyed on ANY attempt (documented
646-647). Secret material cleared in nonce_gen (rand/buf/sha/sk at
365-392) and after signing (partial_sign_clear 626-629, 698). Only the
C-level struct-copy footgun remains — explicitly warned in the header
(include/secp256k1_musig.h:51-57, caller contract).

### N5 (MuSig edge inputs): DISMISSED

Duplicate participant keys: supported by MuSig2 keyagg coefficients
(spec-level, covered by module vectors in tests_impl.h/vectors.h).
Zero/overflow scalars: seckey validity pre-checked in nonce_gen
(389-393) and key loads reject invalid scalars. No cancellation state
exists (session is a 133-byte value struct, no globals). Nonce binds
signer key + aggregate key + message via tagged hash (346-355) with
caller extra_input32 mixed in.

## Campaign 84 cycle complete

All 5 areas locked. Nonce reuse through any library API is impossible by
construction (deterministic hedged nonces + zero-before-checks session
enforcement); remaining footguns are documented caller contracts.
Rotation: uber-ledger marks #84 DONE, next 5/52 boundary/integer.

## Next queue
(N1/N2 first: nonce derivation determinism + guards; then N4 — the newest,
most complex module)
