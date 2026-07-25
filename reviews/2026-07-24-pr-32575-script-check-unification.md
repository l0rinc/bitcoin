# Review: PR 32575 — consensus: remove special treatment for single-threaded script checking (2026-07-24)

Verdict: NOT DANGEROUS as currently written; no leftover bugs found.

The "new" unified path is literally the existing multithreaded path every node with workers already runs. The PR deletes the alternate single-threaded path and lets the queue (which already supports 0 workers by executing inline, in order, on the calling thread via `CCheckQueueControl::Complete()`) handle both.

## Line-by-line semantics comparison

| aspect | old single-threaded | old multithreaded | new unified |
|---|---|---|---|
| cache hash | CSHA256(wtxid‖flags) | same | same (GetScriptCacheEntry, verbatim) |
| cache hit erase | erase iff !fCacheResults | same | same |
| check execution | inline per tx, before UpdateCoins, break at first failure | queued, run at Complete() after all UpdateCoins | = old MT |
| checks per input | all, vin order | all (any order across workers) | all; 0 workers: inline, vin order |
| cache insert (TestBlockValidity) | per tx after its checks pass | never | per block, only after Complete() succeeds AND state.IsValid() — strictly safer |
| reject reason | mandatory-script-verify-flag-failed | block-script-verify-flag-failed | = old MT (what all functional tests already assert) |
| failure world-state | overlay discarded by ResetGuard | same | same (coins.h:581 `~ResetGuard → m_cache.Reset()`) |

The surviving difference — scripts for a failing tx now execute after UpdateCoins for the whole block — is consensus-inert: script verification reads only `txdata.m_spent_outputs` (prepared beforehand), never the live view; failed blocks' view mutations are discarded by the existing RAII in every configuration.

## Hunted and cleared
- UTXO pollution from invalid blocks: impossible — CoinsViewOverlay is Reset() on ConnectTip scope exit unless Flush() ran (success only). Old-ST and new paths leave identical post-failure state.
- Cache semantics: hash, contains-with-erase, fCacheResults gating byte-identical; new insert is stricter (old ST cached entries for TestBlockValidity blocks that later failed — now fixed).
- Complete() with 0 workers: inline, queue order (= tx order), deterministic.
- Error identity: matches MT path that all functional tests pin (feature_cltv/csv_activation/dersig/nulldummy). No test pins the old ST string.
- Mempool path: untouched (inline, no pvChecks, no insert).
- BIP68/CSV and flag computation: outside the diff. The BIP110 bug class cited in the user's NACK was flag selection — not engaged here.

## Flags for the user
1. OVERLAP with your PR 34875 (same split: PrepareScriptChecks/CollectScriptChecks). If 32575 merges, 34875 needs rework or should be closed in favor — comment before they collide.
2. Your NACK's substance was adopted: behavior change isolated in one commit (60bc921f97); your lock-assertion concern fixed (IsScriptValidated/CacheScriptValidation both carry AssertLockHeld); Complete() returning first script error already existed on master. ACKing the rest would be defensible on the merits.

## Historical precedent
Pre-2015 all inline; CheckQueue (2015-16) added the parallel path but kept inline → the dual-use CheckInputScripts pvChecks asymmetry (incl. the TestBlockValidity-only-caches-in-ST quirk mzumsande/theuni flagged). Steady unification since; this PR is the endpoint — it deletes the less-tested path in favor of the one the fleet already runs.
