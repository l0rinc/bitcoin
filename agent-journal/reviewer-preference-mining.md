# Historical reviewer-preference mining and reusable review skill

## Cycle 25: technical review rule extraction

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `60`
- Slug: `reviewer-preference-mining`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at the cycle gate: `8b5df2d6f0140eae014c14d1f3a291a1aab6e98d`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Gate: `git fetch origin master --quiet` passed; tracked source was clean; only agent-owned journals/probes and existing `test/cache/` were untracked; no source, test, fuzz, sanitizer, daemon, or profiling process was running.

### Method and remote evidence

Local history and merged diffs were the primary evidence. The GitHub CLI was present but unauthenticated, so review discussion was collected from the public API with commands of this form:

```text
curl -fsSL 'https://api.github.com/repos/bitcoin/bitcoin/pulls/35692/reviews?per_page=100'
curl -fsSL 'https://api.github.com/repos/bitcoin/bitcoin/issues/35692/comments?per_page=100'
```

The same review and issue-comment endpoints were queried for PRs `#35320`, `#35639`, `#35090`, `#35681`, `#35215`, `#35616`, and `#35767`. The remote evidence is limited by unauthenticated API visibility and should not be treated as a complete record of every review exchange.

### Evidence clusters

| PR / source | Accepted technical signal | Rejected or limited approach | Classification |
|---|---|---|---|
| [#35692](https://github.com/bitcoin/bitcoin/pull/35692), `bc7d9050467fb387c01a73d59cdb914c839964fc` | Replace the unreachable tried-collision branch with `Assume(vvTried[...] != -1)` after considering the operational impact of internal corruption. | Do not mechanically replace every assertion with `Assume`. | Contextual: availability versus fail-fast severity for this invariant. |
| [#35320](https://github.com/bitcoin/bitcoin/pull/35320), `2cf9d79d84cb485e31e2d78a5744c1e4dc5f5f44` | Assert the BIP32 seed range at the internal `CExtKey::SetSeed` boundary so failure cannot leave a partially usable object; keep vector coverage in `src/test/bip32_tests.cpp`. | Returning a status without an observable rejection would leave an invalid or ambiguous object. | General internal-invariant and partial-state rule; exact range and `Assert` choice remain contract-specific. |
| [#35639](https://github.com/bitcoin/bitcoin/pull/35639), `4c9de7d5b329f8ffca7222d8f53c2f5a5616dfc9` | Validate the external-signer fingerprint against the documented eight-hex-character contract at the public response boundary and add a functional test. | Do not trust a field merely because the signer normally emits it. | General public/untrusted-input validation rule. |
| [#35090](https://github.com/bitcoin/bitcoin/pull/35090), `d24d3cbad0109adc1a8ca4b7cba16aa920fa2799` | Reset connection and node state per fuzz iteration, use current context code, preserve malformed and multi-peer paths, use readable `CallOneOf` choices, and retain behavioral assertions. | A random or happy-path-only harness, stale context pointers, broad warning suppression, or an input-space restriction without coverage evidence. | General fuzz-harness realism/reset/oracle rules; dedicated versus general targets is contextual. |
| [#35681](https://github.com/bitcoin/bitcoin/pull/35681), `1fc9277a1c13c13f2ff1825f703a77ebcc9791be` | Put the asynchronous trigger inside `assert_debug_log`, then wait for disconnect and assert the exact message count. | Triggering first and observing logs later leaves an avoidable race window. | General asynchronous-test determinism rule. |
| [#35215](https://github.com/bitcoin/bitcoin/pull/35215), merge `32eb521...` | Make Python fixtures portable by copying JSON through CMake instead of relying on Linux-only `__file__` links; preserve fixed-width vector paths and branch coverage. | Do not change every hasher by analogy: user-provided, unvalidated inputs can change the security model. | General fixture/coverage portability rule; hasher applicability is contextual. |
| [#35616](https://github.com/bitcoin/bitcoin/pull/35616), source in the cache-size migration | Respect prerequisite ordering, add the required `<cstdint>` include, and keep `max/min` where the ordering contract is clearer than a generic clamp. | Do not use `std::clamp` or type changes as automatic style transformations. | General dependency/include hygiene; expression style is contextual. |
| [#35767](https://github.com/bitcoin/bitcoin/pull/35767), source around `faada35...` | Record the external OSS-Fuzz failure context and choose a project-side thread-lifetime fix when the proposed environment variable was broad or insufficiently supported. | Treat an external integration suggestion as a proof or universally applicable fix. | General external-report applicability rule; integration remediation is contextual. |

The recurring signal is technical: reviewers ask whether a proposed change preserves the actual invariant, lifecycle, trust boundary, test oracle, and supported build path. Commit acceptance alone is not evidence of a universal reviewer preference.

### Reusable review skill

General rules supported by multiple clusters:

1. Trace the contract through callers and lifecycle state. Use the actual durable or authoritative state, not a nearby convenient value.
2. Validate public or externally supplied values at the boundary using the documented or specified domain, then assert the behavioral consequence at the natural functional test owner.
3. For internal invariants, choose fail-fast versus recoverable handling from the impact of corruption and the caller contract. Never mechanically replace `Assert` with `Assume`, or return a status that permits partial state.
4. Tests should exercise the behavior that matters: exact state, output, count, transition, or rejection. Existing vectors can cover an assertion's domain without making the assertion itself a separately testable runtime branch.
5. Fuzz harnesses must reset state and lifetimes per iteration, use current production context, retain valid and malformed paths, show coverage-oriented realism, and keep a strong oracle. Avoid broad suppressions.
6. Asynchronous tests should narrow the trigger-to-observation window and wait for the exact observable event rather than rely on timing luck.
7. Fixture and build changes must work on supported operating systems and preserve every meaningful fixed-width or branch-sensitive case.
8. Keep prerequisite ordering and include dependencies explicit; do not treat a local style preference as a correctness rule.

Context-bound rules:

- `Assume` versus `Assert` depends on whether the invariant violation is recoverable, availability-critical, or evidence of unrecoverable corruption.
- A dedicated fuzz target can be useful when it reduces the input space, while a general target can expose interactions; coverage and reachability decide.
- `max/min` versus `std::clamp` depends on the range contract and readability of the ordering proof.
- A faster or collision-resistant hasher needs an input-trust and security-model analysis at each use site.
- Naming, helper placement, and commit sequencing suggestions are optional unless they protect a contract, dependency, or project gate.

### Held-out validation

After extracting the rules, four commits from other clusters were checked as held-out examples:

- `e9ed898a0da064715c8ef66a71ee72c3a35e008b` changes `Chainstate::FlushStateToDisk` to signal `GetLocator(m_last_flushed_block)` rather than `GetLocator(m_chain.Tip())`. This applies the lifecycle/durable-state rule: a flush notification must describe the state actually flushed.
- `6ee05c4b188c0da8cefb7f361c3ba6866c5710b5` adds a wallet coin-selection test with a known exhaustion fixture, exact valid output, selection count, and `GetAlgoCompleted()==false`. This applies the natural-owner and exact-postcondition oracle rules.
- `6aa5d8d9481f5e06b10095df7f46f0532f7ecdb7` adds explicit transaction-source state for block-encoding collisions and tests mempool, extra, collision, decrement, and terminal no-refill behavior. This applies the state-machine and multi-transition oracle rule.
- `122691124f` was checked in the local history as the cache-fixture follow-up; its `CCoinsMap` construction tracks `SaltedCoinsCacheHasher`, applying the fixture/type-contract rule. This same-feature control is not independent evidence for a broad reviewer preference.

The first three held-out examples are independent of the primary review clusters and recover the extracted rules. The fourth is a same-feature follow-up and is useful as a fixture-specific control, not independent evidence for a broad reviewer preference. No held-out commit exposed a new repository defect.

### Verdict and handoff

Verdict: reusable technical review recipe confirmed; no production source change justified. The evidence supports contract-, lifecycle-, trust-boundary-, oracle-, and portability-driven review rules, with explicit contextual exceptions. The unauthenticated public API may omit comments or review edits, and merged history cannot establish what reviewers rejected. No test run was needed for this history-only campaign; exact source tips, API endpoints, local diffs, and held-out checks are recorded above. No process remains running.

Next work should draw a distinct eligible goal and use this recipe when evaluating its review and history evidence. Do not reopen this campaign unless new review data, a different subsystem cluster, or a concrete recurrence changes the evidence.

## Cycle 49: current-master contract and review-surface mining

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `60`
- Slug: `reviewer-preference-mining`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at the cycle gate: `bf28e1aa0e1aef9a9ae7ef05ef51c7aaa1e1600a`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` was `2 866`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Corrected actual-tab TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: `git fetch origin master` passed; tracked and staged state was clean; only agent-owned journals/probes, `agent-goals/`, and existing `test/cache/` were untracked; no source, test, fuzz, sanitizer, daemon, or profiling process was running.

The cycle-25 sample of this goal is closed and was not repeated. This cycle reopened only a distinct current-master evidence cell: post-cycle-25 merged PRs, current review comments, and held-out validation of a revised technical recipe. Review acceptance was treated as evidence of what survived review, not proof that every reviewer shares the same preference. Public unauthenticated API responses can omit hidden or line-level review context.

### Fresh review evidence

The primary sample was the set of changes merged after the old cycle-25 sample and their public review discussions:

- [PR #35690](https://github.com/bitcoin/bitcoin/pull/35690), source `a8223bb4e62c8facd7e99eb05221c67cdcf0b52c`, merge `fe1cb6e40d7bae535c73e23c3bb1c7e6f14d644d`: `WalletError` deliberately has a generic display-only code and a specific `UnlockNeeded` code. The discussion rejected speculative `INVALID_DESCRIPTOR`, `INVALID_PARAMETER`, and `MISC_ERROR` expansion unless a caller can act differently. Reviewers also discussed validating parameters without duplicating the wallet implementation. This supports an actionable-code rule, not a preference for any particular enum names.
- [PR #35736](https://github.com/bitcoin/bitcoin/pull/35736), source `7298281ba8dfb58e07121c74e64f07861ec21f5c`, merge `559d042ba2567a05e8d540c7d9d9a94c7d2973d2`: the `netmagic` command became the structured `getchainparams` command. Review discussion explicitly weighed removing fields with no current use against retaining a self-documenting, extensible object. The final output uses named nested objects and versioned fixture files, while the command rejects extra arguments.
- [PR #35746](https://github.com/bitcoin/bitcoin/pull/35746), sources `a7e980af31b92f5a21be6b91bed353e65c5cf770` and `f3f302150b5fc115e0561dd639b9b389822999e9`, merge `a31c30290d4c86000601de665866f9015635f48f`: CI had overridden `BASE_BUILD_DIR`, bypassing the existing space/UTF-8 path test. The fix quotes every NSIS `File` input and forces the externally supplied build path to contain a space and non-ASCII symbols. A reviewer reproduced the old installer failure on Debian with the actual CI path, confirmed the fix, and recorded a range-diff after rebase.
- [PR #35076](https://github.com/bitcoin/bitcoin/pull/35076), source `51ee8ca1683ce1ba9997d0ccf014a1986afceb93`, merge `afa5e46bbc6dd750bd71920b659162a945abf0ae`: the pruning warning was made actionable and consistent between CLI help and the Qt tooltip by naming both wallets and indexes and the required reindex consequence.
- [PR #35775](https://github.com/bitcoin/bitcoin/pull/35775), source `2cb3bfa8df7cab0635be221af1c8754dbbaff335`, merge `290cb2f17ef6ba9198934bbaec53fa962a1dfa18`: a review caught that “everywhere” overstated a change scoped to Guix scripts and asked whether the linter should change only for a codebase-wide policy. The final wording narrowed the claim to the actual affected directory.

The new evidence repeats the old contract/lifecycle/oracle emphasis but adds three separable rules: stable machine-readable interfaces should be driven by caller action; structured output should be minimal enough to defend while using an extensible shape for proven future-facing data; and build/CI tests must validate the externally injected paths and variables actually consumed by every downstream tool. Scope-accurate documentation is a cross-layer contract, not merely prose polish.

### Reusable current-master recipe

When reviewing a public interface, build or test change, apply this sequence:

1. State the durable consumer contract before judging the diff. For an error code, name the caller action it enables; for structured output, identify each field's consumer or specification basis; for a build variable, trace its value from CI injection through scripts, generators, installers, and artifacts.
2. Keep machine-readable taxonomies small. Use one generic code when the caller only displays the message, add a specific code only when a caller can recover, retry, unlock, choose another path, or otherwise behave differently, and preserve the detailed translated message separately. Avoid duplicating validation solely to manufacture finer categories.
3. Prefer a stable, self-documenting output object over positional or opaque output, but do not add speculative fields without a current contract. Add per-variant fixtures for valid, conditional, and rejected forms, and test the negative argument/error contract.
4. Test the real boundary that failed. If a build path comes from an environment override, make CI set that override to the adversarial value; exercise spaces, non-ASCII, quoting, globs, and rebase/merge changes through the final installer or artifact consumer. A test of the default variable is insufficient when CI replaces it.
5. Keep user-facing claims aligned across every surface and scoped to the affected component. Check CLI help, GUI text, docs, release notes, and linter/policy rules together, and state the operational consequence rather than only the setting's mechanism.
6. Re-run the recipe on an independent held-out change and classify each recovered rule as general, contextual, or not supported. Do not elevate a preferred spelling, helper, or reviewer identity into a project-wide rule.

### Held-out validation

The current tree supplied executable and static controls for the recipe:

- `cmake -S . -B '/data/my_storage/tmp/cycle49-build-space_ ₿🧪_' -G Ninja -DBUILD_UTIL=ON -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_ZMQ=OFF` configured successfully. The isolated `cmake --build ... --target bitcoin-util -j2` completed all 132 actions despite the space and non-ASCII path.
- The resulting binary's `getchainparams` output matched `test/functional/data/util/getchainparams-mainnet.json` and `getchainparams-regtest.json` byte-for-byte. `getchainparams extra_arg` returned status 1 and `getchainparams does not take arguments`. This validates a self-documenting structured output, conditional fields, exact fixtures, and a negative interface contract.
- `git grep` on `origin/master` found only the two deliberately documented `WalletErrorCode` values, `GenericError` and `UnlockNeeded`; their comments state the display-only versus unlock-and-retry behavior. No speculative enum expansion or caller duplication was introduced by the scaffold commit.
- `origin/master` contains the paired pruning strings in `src/init.cpp` and `src/qt/forms/optionsdialog.ui`, both naming wallets, indexes, synchronization, and reindexing. Existing `feature_index_prune.py`, `feature_pruning.py`, `wallet_assumeutxo.py`, and `wallet_migration.py` cover the operational consequences rather than only checking text.
- `origin/master` contains the paired CI path override and quoted NSIS inputs. The local `makensis` executable is unavailable, so the final Windows installer invocation could not be rerun here; the CMake/path build and the public PR reproduction remain the available independent controls.
- The `getchainparams` fixture, pruning documentation, and CI path checks are independent of the historical cycle-25 held-out commits. The extra-transaction-count state-machine change from `6aa5d8d948` remains excluded as a prior-cycle control, not counted as new evidence.

### Verdict and handoff

Verdict: **reusable technical review recipe confirmed; no new repository defect found on current HEAD**. The evidence is stronger than a reviewer-style collection because each rule maps to a caller contract, output schema, build boundary, or user-visible operational consequence, and the held-out controls reproduce those obligations. No production source change is warranted in this history/review cycle. The precise limitation is that unauthenticated API data cannot establish rejected comments exhaustively, and the NSIS deploy target could not run because `makensis` is unavailable. No process remains running.

Next queue: draw another distinct eligible goal. Preserve this recipe under the fingerprint `actionable-interface-minimal-schema-boundary-realism` and do not reopen it unless new review evidence or a concrete recurrence changes the rule.
