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
