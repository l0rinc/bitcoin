# Review: fork PR 237 — psbt: update zero-input transaction outputs (2026-07-24)

Verdict: 🟢 correct and the right approach; two coverage gaps (since partially fixed).

## Correctness (all three guards audited)
- `CreateSig` ECDSA guard, `ComputeSchnorrSignatureHash` nullopt guard, `Checker()` reject-all: consistent, byte-identical for valid inputs. `static const BaseSignatureChecker reject_all` is thread-safe; its default-false CheckLockTime/CheckSequence correctly turns miniscript timelock evaluation into "unsatisfiable" instead of OOB.
- MuSig2 safe: `CreateMuSig2PartialSig` already has `if (!sighash.has_value()) return false;` before any deref (sign.cpp:170). Nonce/aggregate need no sighash.
- Does NOT weaken SignatureHash's assert — buggy direct callers still abort (Core keeps asserts in release); only wallet-reachable creator paths degrade gracefully. Guarding inside SignatureHash itself would have been the wrong place.

## Better than the early-return variant
This PR FILLS output metadata (hd_keypaths) and only fails signature creation — correct for the BIP370 Constructor flow where inputs are added after outputs. The audit-branch early-return (53506a51e9) skipped metadata entirely; this supersedes it.

## Coverage gaps
1. 🟡 No zero-input taproot PSBT test (the exact original crash vector). ✅ FIXED in branch update 36d80b3f8e: `for (bool has_input : {false, true})` on update_psbt_output_taproot.
2. 🟡 No functional/RPC-level test: bug was found via descriptorprocesspsbt; add zero-input createpsbt + key-holding wallet + walletprocesspsbt asserting no crash (cheap in rpc_psbt.py). STILL OPEN as of 2026-07-24.
3. Optional: MuSig2 zero-input (shared guard, low value); direct ECDSA-missing-input unit test (covered indirectly via without_input_legacy — adequate).

## Simplifications
None needed — three guards for three entry points is minimal; consolidating would require touching consensus-adjacent SignatureHash. Style nits only: "does not work" → "cannot work"; Checker() could document the reject-all return.

Ship it (taproot case already added); the functional test can follow.
