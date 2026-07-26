# PR 32575 — your-review-comments vs pushed head 270af71e1e (2026-07-25)

Mapping each of your 2026-03 review comments to the current head. Verdict at the end.

## ✅ Applied correctly

1. **Cache encapsulation with AssertLockHeld on BOTH contains and insert** — current `ValidationCache::IsScriptValidated` and `CacheScriptValidation` both carry `AssertLockHeld(cs_main)` + `EXCLUSIVE_LOCKS_REQUIRED` (commit 79e0bd7edf). Matches your suggested patch shape almost exactly.
2. **Return the cache key instead of bool / no hash recomputation** — the criticized `PreCheckInputScripts` was replaced by `GetScriptCacheEntry()` returning `std::optional<uint256>` (nullopt = already validated/coinbase; the hash computed once and returned for later insert). Better than your suggestion.
3. **Test updates in a separate commit** — `f14e5d5be8` (txvalidationcache_tests) is standalone.
4. **Smaller steps, isolated behavior change** — the series is now encapsulate → extract txdata → extract cache entry → test → behavior change (60bc921f97) → pvChecks removal. The "dead code always returns true" path you flagged in 7726a6a is gone with the special case itself.

## ⚠️ Partially applied / remaining discussion

5. **EnsureTxData** (your "extract only the spent-outputs calculation" comment) — current form still bundles gather + `txdata.Init` + assert rather than extracting only the calculation. The gather uses reserve + emplace_back (no extra copies, as you required). Not a correctness issue; cosmetic vs your ideal shape.
6. **Your CollectScriptChecks split (the Concept NACK) was NOT adopted** — the author did not split collection into a separate function; collection is inlined in ConnectBlock, with `CheckInputScripts` kept for mempool. The end state is equivalent-ish to your 34875 but shaped differently. The NACK's core risk (refactor+behavior entangled) is mitigated: the behavior change is isolated in commit 60bc921f97, and my line-by-line review found the semantics identical (cache hash, erase gating, execution order with 0 workers, error identity = the MT path all tests already assert).

## Safe to ACK?

Yes for the code as-is — the applied suggestions are correct, my deep review found no leftover bugs (see reviews/2026-07-24-pr-32575-script-check-unification.md), and the remaining deltas are shape preferences, not defects. The one decision left for you: PR 34875 (your narrow alternative) — close it or rework it, since 32575 supersedes it; worth saying so in a comment before they collide.
