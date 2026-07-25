# Review: l0rinc/secp256k1 PR 25 — ecdsa: reject overflowed opaque signatures (2026-07-24)

Verdict: 🟢 correct, 🟡 low severity — good hardening, not urgent.

## What it is
An API-robustness gap, not a vulnerability (PR body says so honestly). Wire parsers (parse_compact/DER) already reject r/s ≥ group order n. The gap is the opaque object itself: if the host mutates it after parsing (memory bug, type confusion, deliberate misuse), the loader fed out-of-range scalars into code assuming the invariant:
- VERIFY builds: abort on the scalar invariant (scalar_impl.h:43) — loud crash.
- no-VERIFY builds: silently continue with the invalid scalar → wrong verify/serialize results.
- 64-bit memcpy path specifically: direct memcpy into the scalar with no check at all — worst of the three loaders.

## Why NOT serious for Bitcoin Core
- Not remotely triggerable: every external-input path goes through the bounded parsers.
- Core's internal sigs come from signing or parsing — corrupted-opaque state can't arise from network/RPC/wallet/PSBT input. Requires a host-side memory bug or API misuse by the embedding application.

## Fix correctness (verified line-by-line)
- All four signature_load consumers guarded (serialize_der, serialize_compact, normalize, verify at secp256k1.c:450/463/478/502) — the complete set; signing uses save, unaffected.
- `(void)ctx;` removal correct: ARG_CHECK expands to `secp256k1_callback_call(&ctx->illegal_callback, ...)` — ctx genuinely used now, no unused-parameter warning.
- 32-byte-scalar path: memcpy + check_overflow (no reduction, correct); other path: set_b32 with overflow pointer — reduced value never observable (returns 0 first).
- Contract matches library convention: illegal-argument callback + return 0, per-component (test mutates r and s independently with 0xFF, checks all four consumers).
- Zero behavior change for valid objects; cost is two comparisons per load.

## Nits / optional follow-ups
1. secp256k1_ecdsa_recoverable_signature has the same opaque-object class with its own loader — same treatment as a follow-up (out of scope here, fine).
2. normalize returning 0 conflates "unchanged" with "error" — no error channel exists, illegal callback carries the signal; acceptable.
3. serialize_der's 0-return leaves outputlen untouched — worth a doc line that failure means no output produced.

Merge it as hardening; nobody needs to rush it into a release.
