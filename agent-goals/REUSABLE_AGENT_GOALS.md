# Reusable Continuous Agent Goals for Bitcoin Core and libsecp256k1

This local catalog contains 124 standalone `/goal` prompts. Each fenced block is self-contained and can be selected independently. The catalog is generated from `goals.tsv`; keep the manifest and this file together when moving it.

## Goals

<a id="goal-0"></a>

### 0. Continuous evidence-first bug mining

<!-- slug: continuous-bug-mining; prompt-bytes: 3335 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/continuous-bug-mining.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Rotate current diff, history, tests, fuzzing, sanitizers, static analysis, coverage, performance, platform, and external evidence to find and prove distinct defects.
```

<a id="goal-1"></a>

### 1. Source comment versus implementation contract audit

<!-- slug: comment-code-contract; prompt-bytes: 3319 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/comment-code-contract.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare nontrivial comments with code, callers, tests, docs, blame, and historical rationale; correct only a proven stale contract or implementation.
```

<a id="goal-2"></a>

### 2. Assertion, Assume, and invariant reachability audit

<!-- slug: assertion-invariant-audit; prompt-bytes: 3335 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/assertion-invariant-audit.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Trace assertions and assumptions through release, fuzz, RPC, network, persisted-data, and optional-module paths; prove invalid assumptions or missing validation.
```

<a id="goal-3"></a>

### 3. Current branch and PR leftover sweep

<!-- slug: current-pr-leftovers; prompt-bytes: 3328 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/current-pr-leftovers.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Search each current commit for analogous sites, stale names, partial migrations, omitted tests, generated files, build lists, and unresolved review objections.
```

<a id="goal-4"></a>

### 4. Public API, CLI, RPC, config, and help contract audit

<!-- slug: public-interface-contracts; prompt-bytes: 3317 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/public-interface-contracts.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare registration, parsing, types, defaults, help, errors, runtime behavior, persistence, restart behavior, docs, tests, and compatibility.
```

<a id="goal-5"></a>

### 5. Boundary-condition and off-by-one audit

<!-- slug: boundary-off-by-one; prompt-bytes: 3312 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/boundary-off-by-one.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Formalize domains and test below, at, and above boundaries for heights, times, amounts, counts, offsets, limits, encodings, and iterator ranges.
```

<a id="goal-6"></a>

### 6. Serialization, deserialization, and untrusted-input sweep

<!-- slug: serialization-untrusted-input; prompt-bytes: 3311 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/serialization-untrusted-input.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Trace untrusted bytes through lengths, allocations, casts, loops, mutation, canonical encodings, output state, and later assumptions.
```

<a id="goal-7"></a>

### 7. Untrusted-interface resource-exhaustion variant analysis

<!-- slug: resource-exhaustion-variants; prompt-bytes: 3317 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/resource-exhaustion-variants.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Mine DoS shapes and calculate explicit CPU, memory, disk, network, descriptor, queue, retry, and retained-state bounds for realistic inputs.
```

<a id="goal-8"></a>

### 8. Locking, threading, and scheduler audit

<!-- slug: locking-threading; prompt-bytes: 3325 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/locking-threading.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Map locks, atomics, condition variables, callbacks, worker shutdown, lifetimes, lock order, and deterministic race schedules; prove first conflicting accesses.
```

<a id="goal-9"></a>

### 9. Hit-frequency and suspicious-branch coverage audit

<!-- slug: hit-frequency-coverage; prompt-bytes: 3318 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/hit-frequency-coverage.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Rank rarely executed branches by security, state mutation, complexity, error handling, persistence, secret handling, and untrusted-input proximity.
```

<a id="goal-10"></a>

### 10. Fuzz-target gap and harness-realism audit

<!-- slug: fuzz-target-gaps; prompt-bytes: 3336 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/fuzz-target-gaps.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare production entry points with fuzz reachability, dynamic coverage, state realism, error handling, and oracle quality; add deterministic high-value harness coverage.
```

<a id="goal-11"></a>

### 11. Sanitizer and Valgrind true-positive sweep

<!-- slug: sanitizer-valgrind; prompt-bytes: 3324 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/sanitizer-valgrind.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Run ASan, UBSan, TSan, MSan, LeakSanitizer, Valgrind, and sanitized fuzz/recovery workloads; minimize first invalid operations and reject broad suppressions.
```

<a id="goal-12"></a>

### 12. Static-analysis true-positive campaign

<!-- slug: static-analysis-true-positives; prompt-bytes: 3344 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/static-analysis-true-positives.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Use compiler warnings, clang analysis, clang-tidy, CodeQL or Semgrep, IWYU, and semantic queries to prove lifetime, overflow, result, lock, and control-flow defects.
```

<a id="goal-13"></a>

### 13. Secret-data lifetime and zeroization audit

<!-- slug: secret-lifetime-zeroization; prompt-bytes: 3347 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/secret-lifetime-zeroization.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Trace private keys, nonces, seeds, tweaks, passphrases, and secret-derived temporaries through copies, moves, errors, callbacks, logs, destructors, and optimized clearing.
```

<a id="goal-14"></a>

### 14. Secret-dependent control-flow and memory-access audit

<!-- slug: secret-control-flow; prompt-bytes: 3339 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/secret-control-flow.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Trace secret taint into branches, loop counts, indexes, addresses, helper choices, error paths, compiler output, ctgrind, dudect, and explicit declassification boundaries.
```

<a id="goal-15"></a>

### 15. Public object parsing and validation variant analysis

<!-- slug: public-object-validation; prompt-bytes: 3337 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/public-object-validation.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare all equivalent parsing paths for keys, signatures, scalars, scripts, descriptors, records, addresses, and wrappers across malformed and noncanonical inputs.
```

<a id="goal-16"></a>

### 16. Public API misuse-resistance audit

<!-- slug: api-misuse-resistance; prompt-bytes: 3347 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/api-misuse-resistance.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Read headers, examples, bindings, tests, and implementation as an adversarial caller; test ownership, lifetime, capability, callbacks, invalidation, status, and failure outputs.
```

<a id="goal-17"></a>

### 17. Build-matrix and module-configuration audit

<!-- slug: build-matrix-modules; prompt-bytes: 3328 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/build-matrix-modules.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Enumerate compilers, features, wallets, IPC, GUI, tools, fuzz, assembly, exhaustive, shared/static, cross, and sanitizer configurations and their interactions.
```

<a id="goal-18"></a>

### 18. Exhaustive and algebraic-invariant audit

<!-- slug: exhaustive-algebraic; prompt-bytes: 3346 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/exhaustive-algebraic.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
State identities for parsing, arithmetic, normalization, signing, storage, iteration, cache recomputation, and recovery; test exhaustive small domains and randomized properties.
```

<a id="goal-19"></a>

### 19. Benchmark correctness and measurement-integrity audit

<!-- slug: benchmark-integrity; prompt-bytes: 3324 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/benchmark-integrity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit benchmark setup, timed regions, units, inputs, caches, I/O, compiler-elision barriers, result validation, repetitions, noise, and profile attribution.
```

<a id="goal-20"></a>

### 20. Simple micro-optimization discovery and proof

<!-- slug: micro-optimization; prompt-bytes: 3344 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/micro-optimization.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Use profiles to select one narrow hot operation, then prove a simple allocation, copy, lookup, hashing, branch, serialization, or lock improvement with interleaved measurements.
```

<a id="goal-21"></a>

### 21. Long-running rebuild, recovery, and compaction profiling

<!-- slug: rebuild-recovery-profile; prompt-bytes: 3324 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/rebuild-recovery-profile.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Profile rebuild, reindex, rescan, recovery, snapshot, index, and compaction workloads with CPU, I/O, memory, fsync, progress, and correctness evidence.
```

<a id="goal-22"></a>

### 22. Full sync, IBD, import, and end-to-end profiling

<!-- slug: full-sync-ibd-profile; prompt-bytes: 3333 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/full-sync-ibd-profile.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Measure controlled sync/import/reindex phases while separating download, validation, scripts, crypto, chainstate, block I/O, compaction, logging, and network time.
```

<a id="goal-23"></a>

### 23. Perf and flamegraph investigation without forced commits

<!-- slug: perf-flamegraph-investigation; prompt-bytes: 3347 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/perf-flamegraph-investigation.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Capture perf, flamegraphs, scheduler, lock, CPU counter, memory, disk, network, and process data; leave measured hypotheses and raw artifacts even without a code change.
```

<a id="goal-24"></a>

### 24. Disk I/O, persistence growth, and write-amplification audit

<!-- slug: disk-io-amplification; prompt-bytes: 3333 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/disk-io-amplification.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Measure reads, writes, fsyncs, compactions, logs, temporary files, database growth, cache bypasses, repeated serialization, and crash-consistent storage placement.
```

<a id="goal-25"></a>

### 25. Recent performance-regression bisect

<!-- slug: performance-regression-bisect; prompt-bytes: 3339 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/performance-regression-bisect.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare stable workloads across a justified commit range, bisect a proven regression, explain changed work or resource behavior, and preserve correctness intent.
```

<a id="goal-26"></a>

### 26. Bug fixed in one subsystem but present in another

<!-- slug: cross-subsystem-bug-shapes; prompt-bytes: 3327 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/cross-subsystem-bug-shapes.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Mine historical fixes for structural bug shapes and search analogous parsers, caches, indexes, queues, state machines, and APIs with reachability proof.
```

<a id="goal-27"></a>

### 27. Error-path partial-state mutation audit

<!-- slug: error-path-state; prompt-bytes: 3312 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/error-path-state.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Enumerate all failure edges of status/result/exception APIs and compare complete pre/post object, cache, file, transaction, index, and retry state.
```

<a id="goal-28"></a>

### 28. Weak-test oracle and mutation-survival audit

<!-- slug: weak-test-oracles; prompt-bytes: 3330 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/weak-test-oracles.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Find no-op, success-only, log-only, duplicate-logic, sleep-based, and missing-negative tests; use targeted mutations and add the smallest behavior-sensitive oracle.
```

<a id="goal-29"></a>

### 29. Dead code, stale feature, and TODO archaeology

<!-- slug: dead-stale-code; prompt-bytes: 3331 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/dead-stale-code.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Inventory unreachable code, dormant flags, obsolete compatibility, duplicated implementations, stale tests/docs, and TODOs; prove intentional debt versus safe removal.
```

<a id="goal-30"></a>

### 30. Security-sensitive and misleading logging audit

<!-- slug: security-logging; prompt-bytes: 3338 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/security-logging.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Trace secrets, private metadata, peer identifiers, wallet details, paths, configuration, raw payloads, severity, escaping, repetition, and diagnostic truthfulness into logs.
```

<a id="goal-31"></a>

### 31. Cross-layer docs, examples, tests, and implementation audit

<!-- slug: cross-layer-contracts; prompt-bytes: 3324 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/cross-layer-contracts.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Build a contract table across source, public docs, examples, schemas, help, release notes, tests, and implementation; fix the smallest traceable mismatch.
```

<a id="goal-32"></a>

### 32. Whole-history incomplete-fix and migration mining

<!-- slug: history-incomplete-fixes; prompt-bytes: 3343 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/history-incomplete-fixes.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Mine historical partial fixes, follow-ups, reverted work, and migrations; locate omitted analogous sites, stale compatibility, and unresolved assumptions on current code.
```

<a id="goal-33"></a>

### 33. External vulnerability and advisory variant analysis

<!-- slug: external-vulnerability-variants; prompt-bytes: 3363 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/external-vulnerability-variants.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Use advisories and related-project fixes as seeds, map their preconditions to this tree, independently verify applicability, and produce report-ready evidence for remote-only defects.
```

<a id="goal-34"></a>

### 34. Uncovered-code classification and closure audit

<!-- slug: uncovered-code-closure; prompt-bytes: 3338 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/uncovered-code-closure.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Classify uncovered code as legitimate platform/config, harness gap, dead code, or risky behavior; close only high-value gaps with semantic tests and measured coverage.
```

<a id="goal-35"></a>

### 35. Mutation-testing campaign

<!-- slug: mutation-testing; prompt-bytes: 3329 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/mutation-testing.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Run focused source or LLVM-level mutations across security, consensus, persistence, crypto, networking, and recent fixes; kill realistic defects with minimal tests.
```

<a id="goal-36"></a>

### 36. Cross-tool sanitizer and static-analysis matrix

<!-- slug: sanitizer-analysis-matrix; prompt-bytes: 3353 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/sanitizer-analysis-matrix.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Run complementary sanitizer and analyzer combinations across modules and configurations, correlate reports, and separate root causes from instrumentation or suppression artifacts.
```

<a id="goal-37"></a>

### 37. Build dead-zone and conditional-compilation audit

<!-- slug: build-dead-zones; prompt-bytes: 3336 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/build-dead-zones.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Find code, tests, generated files, and security paths hidden by feature flags, platform guards, compiler checks, or conditional source lists; execute or justify each zone.
```

<a id="goal-38"></a>

### 38. Failure cleanup and crash-safety audit

<!-- slug: failure-cleanup; prompt-bytes: 3331 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/failure-cleanup.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Inject errors and abrupt exits through allocation, I/O, callbacks, locks, database, network, and parsing paths; prove cleanup, rollback, retry, and restart invariants.
```

<a id="goal-39"></a>

### 39. Generated-artifact and test-vector determinism audit

<!-- slug: deterministic-artifacts; prompt-bytes: 3330 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/deterministic-artifacts.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit generated code, fixtures, vectors, ordering, timestamps, locale, randomness, tool versions, and regeneration rules for reproducibility and stale output.
```

<a id="goal-40"></a>

### 40. Independent multi-agent disagreement and adjudication audit

<!-- slug: multi-agent-adjudication; prompt-bytes: 3351 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/multi-agent-adjudication.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Have independent investigators analyze the same candidate or surface, compare evidence and assumptions, adjudicate disagreement, and retain dissent with a final verifier verdict.
```

<a id="goal-41"></a>

### 41. History archaeology from a seed topic

<!-- slug: history-seed-archaeology; prompt-bytes: 3348 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/history-seed-archaeology.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Start from one bug shape, symbol, issue, or review topic and follow blame, merges, branches, reverts, tests, and later variants to generate evidence-backed current hypotheses.
```

<a id="goal-42"></a>

### 42. CI, coverage-bot, and review-bot follow-up audit

<!-- slug: ci-bot-followup; prompt-bytes: 3314 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/ci-bot-followup.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Mine CI failures, coverage comments, analyzer reports, review bots, and unresolved checks for real omissions, stale suppressions, and follow-up fixes.
```

<a id="goal-43"></a>

### 43. Option and API lifecycle audit

<!-- slug: option-api-lifecycle; prompt-bytes: 3322 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/option-api-lifecycle.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Trace creation, defaulting, mutation, runtime observation, reload, persistence, restart, deprecation, and removal of options and APIs across all clients.
```

<a id="goal-44"></a>

### 44. Secret-copy and compiler-optimization audit

<!-- slug: secret-copy-compiler; prompt-bytes: 3333 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/secret-copy-compiler.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Find avoidable secret copies and compiler transformations that retain or expose them; inspect optimized code, cleanse primitives, moves, buffers, and failure paths.
```

<a id="goal-45"></a>

### 45. Constant-time boundary and declassification audit

<!-- slug: constant-time-boundary; prompt-bytes: 3346 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/constant-time-boundary.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Define secret/public transitions around crypto and authentication APIs, then prove constant-time obligations, public-dependent exceptions, and declassification justifications.
```

<a id="goal-46"></a>

### 46. Public API output-on-failure audit

<!-- slug: public-output-failure; prompt-bytes: 3340 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/public-output-failure.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Exercise malformed and failing public calls to determine whether outputs are unchanged, zeroed, invalidated, or partially committed as documented and expected by callers.
```

<a id="goal-47"></a>

### 47. Build-system and CI parity audit

<!-- slug: build-ci-parity; prompt-bytes: 3328 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/build-ci-parity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare local build recipes, CI jobs, release builds, test matrices, generated manifests, cache keys, feature flags, and toolchain assumptions for missing coverage.
```

<a id="goal-48"></a>

### 48. Property, exhaustive, and algebraic oracle expansion

<!-- slug: property-oracle-expansion; prompt-bytes: 3351 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/property-oracle-expansion.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Expand weak example tests into properties, exhaustive small domains, differential references, metamorphic relations, failure-state postconditions, and minimized counterexamples.
```

<a id="goal-49"></a>

### 49. Critical whole-history must-fix sweep

<!-- slug: critical-history-must-fix; prompt-bytes: 3354 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/critical-history-must-fix.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Search all historical critical fixes and security-sensitive follow-ups for current variants, backports, reverts, and untested assumptions; verify only concrete reachable omissions.
```

<a id="goal-50"></a>

### 50. Fuzz Introspector blocker and complexity audit

<!-- slug: fuzz-introspector; prompt-bytes: 3336 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/fuzz-introspector.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Use static reachability, dynamic coverage, complexity ranking, call-depth and blocker data to explain why high-risk fuzz targets are ineffective and remove real blockers.
```

<a id="goal-51"></a>

### 51. Invariant, differential, and metamorphic audit

<!-- slug: invariant-differential; prompt-bytes: 3337 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/invariant-differential.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Define state and output invariants, compare alternative implementations and transformations, and use sequence-based metamorphic testing to find inconsistent behavior.
```

<a id="goal-52"></a>

### 52. Integer overflow, narrowing, signedness, and division audit

<!-- slug: integer-overflow; prompt-bytes: 3322 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/integer-overflow.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Trace widths and arithmetic around casts, shifts, division, sentinels, sizes, fees, amounts, counts, and serialization under 32- and 64-bit relevant domains.
```

<a id="goal-53"></a>

### 53. Statistical timing-side-channel campaign

<!-- slug: statistical-timing; prompt-bytes: 3354 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/statistical-timing.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Measure secret-class timing with controlled dudect-style experiments, compiler/backend variants, noise analysis, and dataflow review while treating passing statistics as limited evidence.
```

<a id="goal-54"></a>

### 54. RAII, smart-pointer, and resource-leak audit

<!-- slug: raai-resource-leaks; prompt-bytes: 3338 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/raai-resource-leaks.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit ownership, moves, exception/error paths, custom deleters, file descriptors, locks, threads, mappings, database handles, and callbacks for leaks and lifetime errors.
```

<a id="goal-55"></a>

### 55. Alternative-implementation compatibility-difference audit

<!-- slug: alternative-implementation; prompt-bytes: 3353 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/alternative-implementation.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare protocol, crypto, database, and wrapper behavior with related implementations, classify intentional policy differences, and prove local contract or compatibility defects.
```

<a id="goal-56"></a>

### 56. Stale PR critical-fix resurrection audit

<!-- slug: stale-pr-critical-fix; prompt-bytes: 3349 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/stale-pr-critical-fix.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Mine abandoned or stale pull requests for reproducible security, correctness, testing, portability, or performance fixes, rebase intent carefully, and verify against current code.
```

<a id="goal-57"></a>

### 57. Local-reasoning domain and relationship audit

<!-- slug: local-reasoning-domain; prompt-bytes: 3349 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/local-reasoning-domain.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Challenge functions whose local assumptions omit callers, domains, relationships, ownership, ordering, or state transitions; prove the smallest missing precondition or invariant.
```

<a id="goal-58"></a>

### 58. Exact helper reuse and minimal helper-extension audit

<!-- slug: exact-helper-reuse; prompt-bytes: 3350 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/exact-helper-reuse.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Find duplicated near-equivalent helpers and determine whether exact reuse, a narrow extension, or intentional separation is required by contracts, performance, or constant-time rules.
```

<a id="goal-59"></a>

### 59. C/C++ supply-chain and security-gate audit

<!-- slug: cpp-supply-chain; prompt-bytes: 3344 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/cpp-supply-chain.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit dependencies, vendored code, build downloads, compiler tools, hashes, signatures, license gates, generated inputs, and security checks for practical supply-chain weaknesses.
```

<a id="goal-60"></a>

### 60. Historical reviewer-preference mining and reusable review skill

<!-- slug: reviewer-preference-mining; prompt-bytes: 3353 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/reviewer-preference-mining.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Extract technically grounded reviewer patterns, rejected approaches, test expectations, and architecture rules from history; validate reusable review recipes on held-out changes.
```

<a id="goal-61"></a>

### 61. Stateful contract-fuzzer expansion

<!-- slug: stateful-contract-fuzzer; prompt-bytes: 3348 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/stateful-contract-fuzzer.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Build production-backed operation-sequence fuzzers for stateful APIs, with model invariants, failure-state checks, deterministic shrinking, and minimized replayable sequences.
```

<a id="goal-62"></a>

### 62. Rejected-finding resurrection and assumption attack

<!-- slug: rejected-finding-resurrection; prompt-bytes: 3363 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/rejected-finding-resurrection.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Revisit dismissed findings and their assumptions, attack changed callers, configurations, versions, and input domains, and either confirm recurrence or preserve the dismissal rationale.
```

<a id="goal-63"></a>

### 63. Loupe-style scout, verifier, fixer, and reporter pipeline

<!-- slug: loupe-pipeline; prompt-bytes: 3323 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/loupe-pipeline.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Separate scouting, independent verification, fixing, and reporting with leases, regression PoCs, applicability checks, deduplication, and final review evidence.
```

<a id="goal-64"></a>

### 64. Finding deduplication, recurrence, and semantic-fingerprint audit

<!-- slug: finding-dedup-recurrence; prompt-bytes: 3347 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/finding-dedup-recurrence.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Index findings by path, trust boundary, shape, source-to-sink relation, versions, hashes, and semantics; detect duplicates, recurrences, incomplete variants, and regressions.
```

<a id="goal-65"></a>

### 65. Contributor-branch and work-in-progress radar

<!-- slug: contributor-branch-radar; prompt-bytes: 3365 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/contributor-branch-radar.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Inventory public contributor branches, PRs, bases, divergence, tests, review concerns, upcoming migrations, useful seeds, independent bugs, and conflict risks without copying unpublished work.
```

<a id="goal-66"></a>

### 66. Cherry-pick, backport, and release-branch correctness audit

<!-- slug: backport-correctness; prompt-bytes: 3335 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/backport-correctness.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare patch intent and behavior across maintenance branches, prerequisite chains, conflict resolutions, generated files, guards, tests, and upgrade/downgrade paths.
```

<a id="goal-67"></a>

### 67. Release-to-release behavioral and consensus differential

<!-- slug: release-version-differential; prompt-bytes: 3347 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/release-version-differential.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Feed identical deterministic vectors, blocks, transactions, scripts, wallets, databases, RPC cases, and transcripts to releases and current HEAD; classify expected drift.
```

<a id="goal-68"></a>

### 68. Architecture, endianness, word-size, and ABI parity audit

<!-- slug: architecture-abi-parity; prompt-bytes: 3339 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/architecture-abi-parity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare supported architectures, widths, alignment, endian conversions, atomics, filesystem and socket APIs, serialized outputs, assembly selection, and skipped tests.
```

<a id="goal-69"></a>

### 69. SIMD, assembly, and portable-reference backend differential

<!-- slug: backend-differential; prompt-bytes: 3341 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/backend-differential.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Force optimized and reference backends for hashing, crypto, field/scalar/group math, checksums, memory, and codecs; compare outputs, errors, aliasing, and timing contracts.
```

<a id="goal-70"></a>

### 70. Compiler, optimization, LTO, PGO, and BOLT differential

<!-- slug: compiler-optimization-differential; prompt-bytes: 3361 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/compiler-optimization-differential.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare supported compilers and optimization/profile modes for correctness, UB, miscompiles, barriers, constant-time behavior, size, startup, cache, and reproducible performance.
```

<a id="goal-71"></a>

### 71. Deterministic simulation and failure-schedule exploration

<!-- slug: deterministic-simulation; prompt-bytes: 3360 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/deterministic-simulation.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Explore seeded task ordering, time, randomness, network, disk, retries, and shutdown schedules through production logic with explicit state, progress, durability, and resource invariants.
```

<a id="goal-72"></a>

### 72. Filesystem, power-loss, and crash-consistency injection

<!-- slug: filesystem-crash-consistency; prompt-bytes: 3347 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/filesystem-crash-consistency.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Inject short writes, ENOSPC, EIO, drops, corruption, truncation, reorder assumptions, and process kills around durable boundaries; verify restart and idempotent recovery.
```

<a id="goal-73"></a>

### 73. Network fragmentation, reordering, and partial-I/O state-machine audit

<!-- slug: network-state-machine; prompt-bytes: 3349 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/network-state-machine.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Exercise fragmented and coalesced reads, short writes, EOF, reconnects, half-closes, stalls, backpressure, duplicate messages, and handshake state with deterministic socket shims.
```

<a id="goal-74"></a>

### 74. Memory pressure, OOM, allocator, and fragmentation audit

<!-- slug: memory-pressure-allocator; prompt-bytes: 3342 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/memory-pressure-allocator.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Measure heap, RSS, allocation lifetime, fragmentation, caches, arenas, and stack under normal and adversarial workloads; inject OOM and verify bounded graceful failure.
```

<a id="goal-75"></a>

### 75. Build throughput, dependency graph, and container-cache audit

<!-- slug: build-throughput-cacheability; prompt-bytes: 3347 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/build-throughput-cacheability.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Measure clean, incremental, no-op, parallel, and container builds for dependency fan-out, generated instability, header cost, serialized commands, and cache correctness.
```

<a id="goal-76"></a>

### 76. Reproducible binaries, Guix, and toolchain-provenance audit

<!-- slug: reproducible-builds; prompt-bytes: 3339 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/reproducible-builds.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Rebuild artifacts in pinned clean environments and compare hashes and binary differences; trace timestamps, paths, ordering, dependencies, signing, and host contamination.
```

<a id="goal-77"></a>

### 77. Symbolic execution and bounded-model-checking campaign

<!-- slug: symbolic-model-checking; prompt-bytes: 3350 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/symbolic-model-checking.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Apply CBMC or KLEE to bounded high-risk kernels with explicit production assumptions, memory/UB assertions, reference equivalence, postconditions, and replayable counterexamples.
```

<a id="goal-78"></a>

### 78. Compiler-transformation validation and miscompile isolation

<!-- slug: translation-validation; prompt-bytes: 3357 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/translation-validation.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare LLVM IR and optimized results with Alive2, compiler versions, UB-free reducers, and deterministic vectors to isolate source UB, compiler bugs, assembly contracts, or test errors.
```

<a id="goal-79"></a>

### 79. Fuzz-corpus stewardship, minimization, and transfer audit

<!-- slug: fuzz-corpus-stewardship; prompt-bytes: 3342 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/fuzz-corpus-stewardship.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Inventory corpora, coverage, size, runtime, provenance, and crashes; minimize and transfer structurally useful inputs across targets and engines without losing semantics.
```

<a id="goal-80"></a>

### 80. Fuzz-engine and property-framework differential

<!-- slug: fuzz-engine-differential; prompt-bytes: 3334 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/fuzz-engine-differential.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare libFuzzer, AFL++, Honggfuzz, and property-based fuzzing on shared targets, seeds, budgets, coverage, execution rate, crashes, memory, and corpus quality.
```

<a id="goal-81"></a>

### 81. Specification, test-vector, and formal-model drift audit

<!-- slug: spec-vector-drift; prompt-bytes: 3327 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/spec-vector-drift.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Map implementation and tests to authoritative BIPs, protocol documents, secp docs, formal models, and versioned vectors; resolve drift with traceable provenance.
```

<a id="goal-82"></a>

### 82. secp256k1 field and scalar representation matrix

<!-- slug: secp-field-scalar-matrix; prompt-bytes: 3346 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/secp-field-scalar-matrix.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare 5x52/10x26 fields, 4x64/8x32 scalars, VERIFY, exhaustive, inversion, compiler, and architecture paths with limb, carry, magnitude, aliasing, and boundary properties.
```

<a id="goal-83"></a>

### 83. secp256k1 group, ecmult, and formula-parity audit

<!-- slug: secp-group-ecmult; prompt-bytes: 3342 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/secp-group-ecmult.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare affine/Jacobian formulas, infinity, wNAF, windows, generator/arbitrary multiplication, endomorphism, batch inversion, optimized paths, and exhaustive/reference results.
```

<a id="goal-84"></a>

### 84. secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit

<!-- slug: secp-nonce-session; prompt-bytes: 3334 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/secp-nonce-session.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Model signing, nonce, tweak, ECDH, extrakeys, and MuSig transitions; test invalid order, reuse, duplicates, malformed values, callbacks, replay, and output-on-failure.
```

<a id="goal-85"></a>

### 85. Bitcoin consensus mutation-score and kill-test audit

<!-- slug: bitcoin-consensus-mutation; prompt-bytes: 3362 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/bitcoin-consensus-mutation.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Mutate consensus checks, activation boundaries, flags, script limits, canonical forms, cache keys, amount/sequence/time arithmetic, and prove tests distinguish each non-equivalent mutant.
```

<a id="goal-86"></a>

### 86. Bitcoin chainstate, reorg, prune, and index crash-symmetry audit

<!-- slug: bitcoin-chainstate-symmetry; prompt-bytes: 3333 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/bitcoin-chainstate-symmetry.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Model connect/disconnect, flush, reorg, restart, prune, snapshots, block/undo files, and indexes; inject crashes and assert durable-state and query symmetry.
```

<a id="goal-87"></a>

### 87. Bitcoin mempool, package, and eviction-accounting audit

<!-- slug: bitcoin-mempool-accounting; prompt-bytes: 3342 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/bitcoin-mempool-accounting.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Model acceptance, replacement, ancestor/descendant graphs, fees, clusters, expiry, trimming, conflicts, reorg removal, and memory accounting under operation sequences.
```

<a id="goal-88"></a>

### 88. Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit

<!-- slug: bitcoin-wallet-recovery; prompt-bytes: 3343 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/bitcoin-wallet-recovery.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Trace wallet encryption, passphrase changes, master keys, descriptors, keypool, reservation, migration, backup, restore, rescan, signers, failures, and restart durability.
```

<a id="goal-89"></a>

### 89. Bitcoin P2P transport, permission, and peer-accounting audit

<!-- slug: bitcoin-p2p-accounting; prompt-bytes: 3343 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/bitcoin-p2p-accounting.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Model peer lifecycle, v1/v2 transport, permissions, handshake, messages, quotas, discouragement, download state, queues, timeouts, and disconnect cleanup under partial I/O.
```

<a id="goal-90"></a>

### 90. Whole-PR and commit knowledge-base recipe synthesis

<!-- slug: historical-knowledge-recipes; prompt-bytes: 3343 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/historical-knowledge-recipes.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Extract reusable technical recipes from commits and PRs: invariants, bug shapes, rejected designs, benchmarks, fixtures, platform caveats, migrations, and follow-ups.
```

<a id="goal-91"></a>

### 91. Compiler and binary-hardening configuration audit

<!-- slug: compiler-binary-hardening; prompt-bytes: 3342 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/compiler-binary-hardening.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit warnings, stack protection, FORTIFY, PIE, RELRO, CFI, SafeStack, control-flow protections, assertions, sanitizers, and final binaries for concrete security value.
```

<a id="goal-92"></a>

### 92. ABI layout, alignment, aliasing, and object-lifetime audit

<!-- slug: abi-alignment-aliasing; prompt-bytes: 3340 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/abi-alignment-aliasing.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Test packed layouts, unions, casts, placement new, raw storage, over-alignment, strict aliasing, pointer provenance, lifetimes, and C/C++ ABI across compilers and hosts.
```

<a id="goal-93"></a>

### 93. Allocation, syscall, clock, randomness, and callback fault injection

<!-- slug: system-fault-injection; prompt-bytes: 3346 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/system-fault-injection.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Fail allocations, opens, reads, writes, fsync, rename, sockets, threads, entropy, clocks, scheduler callbacks, databases, and user callbacks at deterministic operation counts.
```

<a id="goal-94"></a>

### 94. Bindings, FFI, and language-wrapper parity audit

<!-- slug: bindings-ffi-parity; prompt-bytes: 3337 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/bindings-ffi-parity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare C/C++ APIs with maintained wrappers for widths, ownership, lifetimes, nullability, callbacks, exceptions, buffers, secrets, threads, flags, and malformed inputs.
```

<a id="goal-95"></a>

### 95. Database-engine and persistence-semantics differential

<!-- slug: database-semantics-differential; prompt-bytes: 3350 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/database-semantics-differential.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare LevelDB assumptions with upstream and alternative engines for ordering, snapshots, iterators, batches, WAL/MANIFEST, checksums, filters, compaction, and recovery.
```

<a id="goal-96"></a>

### 96. TODO, FIXME, stub, and deferred-work challenge audit

<!-- slug: todo-deferred-work; prompt-bytes: 3336 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/todo-deferred-work.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Turn deferred-work markers, disabled tests, expected failures, stubs, compatibility code, and placeholders into current falsifiable questions with history and ownership.
```

<a id="goal-97"></a>

### 97. C and C++ defect-taxonomy sweep

<!-- slug: cpp-defect-taxonomy; prompt-bytes: 3342 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/cpp-defect-taxonomy.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Cycle through null, arithmetic, lifetime, bounds, race, deadlock, aliasing, cleanup, recursion, format, and unchecked-result defects using tools plus real reachability proof.
```

<a id="goal-98"></a>

### 98. Floating-point edge values, sanitizer resurrection, and fuzz-exclusion audit

<!-- slug: float-sanitizer-fuzz-exclusions; prompt-bytes: 3354 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/float-sanitizer-fuzz-exclusions.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Test exceptional floating values, revive suppressed sanitizer diagnostics, and challenge fuzzer exclusions while preserving legitimate preconditions and failure-state safety.
```

<a id="goal-99"></a>

### 99. PSBT serialized-key identity and round-trip preservation audit

<!-- slug: psbt-serialized-key-identity; prompt-bytes: 3421 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/psbt-serialized-key-identity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare PSBT map key identity with in-memory equality and ordering for every keydata-bearing field; mutate one serialized byte at a time, test parse/merge/RPC/re-encode preservation, and distinguish exact duplicates from distinct valid records.
```

<a id="goal-100"></a>

### 100. Append-only preallocation and physical/logical file-size audit

<!-- slug: append-only-file-allocation; prompt-bytes: 3463 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/append-only-file-allocation.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit preallocation, truncation, flush, rebuild, and recovery helpers when physical file size exceeds logical position; force unsupported-platform fallbacks, preserve existing bytes, test overflow and short writes, and compare Windows, macOS, POSIX, sparse, and crash-recovery semantics.
```

<a id="goal-101"></a>

### 101. Minisketch runtime implementation dispatch and availability audit

<!-- slug: minisketch-runtime-dispatch; prompt-bytes: 3472 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/minisketch-runtime-dispatch.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Exercise compile-time CLMUL inclusion and runtime CPU-feature selection across supported and unsupported CPUs; verify implementation_supported/create consistency, safe generic fallback, CPUID and OS assumptions, disabled-field matrices, and diagnostics without executing unsupported instructions.
```

<a id="goal-102"></a>

### 102. macOS preallocation and physical/logical file-size semantics audit

<!-- slug: macos-preallocation-semantics; prompt-bytes: 3424 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/macos-preallocation-semantics.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit the __APPLE__ F_PREALLOCATE and ftruncate path for existing-file shrinkage, fallback-result handling, off_t overflow, sparse and exFAT behavior, short writes, crashes, and restart/reindex preservation with platform tests or faithful models.
```

<a id="goal-103"></a>

### 103. X-only conversion precondition and invalid CPubKey boundary audit

<!-- slug: xonly-cpubkey-preconditions; prompt-bytes: 3488 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/xonly-cpubkey-preconditions.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit every XOnlyPubKey(CPubKey) caller for empty, short, syntactically valid, and off-curve inputs; prove safe preconditions across descriptors, signing, providers, wallet, fuzz, and serialization without changing raw script consensus semantics; add focused guards or contract tests only for confirmed failures.
```

<a id="goal-104"></a>

### 104. Integer option narrowing and resource-boundary audit

<!-- slug: integer-option-boundaries; prompt-bytes: 3459 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/integer-option-boundaries.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit int64_t-to-int, size_t, chrono, and signed/unsigned conversions in startup and resource options; model representability, negative values, downstream arithmetic, and OS caps for -par, duration options, wallet fee sizes, and index heights with sanitizer and clean-startup evidence.
```

<a id="goal-105"></a>

### 105. Minisketch API size and count arithmetic audit

<!-- slug: minisketch-api-size-arithmetic; prompt-bytes: 3437 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/minisketch-api-size-arithmetic.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit Minisketch serialized-size multiplication, capacity/max-elements computations, decode return counts, and C/C++ output bounds for size_t overflow, allocation-feasible extremes, and cross-backend consistency with boundary models and sanitized API probes.
```

<a id="goal-106"></a>

### 106. Generated-source escaping and provenance boundary audit

<!-- slug: generated-source-boundaries; prompt-bytes: 3495 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/generated-source-boundaries.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit generators that turn external or mutable text/data into C/C++, Rust, shell, manpage, build, or metadata artifacts for delimiter escaping, comment and literal closure, paths, command arguments, encoding, size limits, provenance, determinism, and fail-closed behavior with hostile fixtures and consumer compilation.
```

<a id="goal-107"></a>

### 107. BIP324 short-message ID and long-form interoperability audit

<!-- slug: bip324-short-id-parity; prompt-bytes: 3491 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/bip324-short-id-parity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Extract authoritative BIP324 and extension ID tables, compare Core and alternate implementations in both directions, require supported messages to accept short and long forms, reject unassigned IDs consistently, test feature negotiation and version gating, and preserve deterministic wire fixtures for every discrepancy.
```

<a id="goal-108"></a>

### 108. Persisted GCS filter payload validation and recovery audit

<!-- slug: persisted-gcs-filter-validation; prompt-bytes: 3581 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/persisted-gcs-filter-validation.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit block-filter index flat-file and database coupling, semantic validation skipped on local reads, malformed N/truncated/excess Golomb-Rice payloads, hash/position mismatches, startup and query recovery, and corruption classification; use coupled scratch fixtures, deterministic fault injection, bounded-work proofs, and independent restart/query oracles without duplicating network filter parsing.
```

<a id="goal-109"></a>

### 109. libbitcoinkernel C ABI nullability, callback, and width matrix

<!-- slug: kernel-c-abi-boundary-matrix; prompt-bytes: 3619 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/kernel-c-abi-boundary-matrix.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare the raw libbitcoinkernel C API with its C++ btck wrapper and header contracts for nullable arrays and output handles, borrowed lifetimes, callback ownership and reentrancy, status versus exception translation, length-delimited buffers, enum domains, and size_t or fixed-width conversions; use deterministic raw-C consumers, wrapper side-by-side tests, fault injection, and 32-bit or compile-time evidence to prove distinct mismatches.
```

<a id="goal-110"></a>

### 110. Extended public-key identity across descriptor and wallet containers

<!-- slug: extpubkey-identity-matrix; prompt-bytes: 3576 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/extpubkey-identity-matrix.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit CExtPubKey identity across descriptor extraction, gethdkeys, createwalletdescriptor, backup, migration, and PSBT. Compare cryptographic derivation equivalence with serialized metadata (version, depth, fingerprint, child, chain code, and public key), prove intentional deduplication, and test xpub/xprv and origin output under metadata collisions with explicit comparators and recurrence fixtures.
```

<a id="goal-111"></a>

### 111. Kernel library and chainstate configuration parity audit

<!-- slug: kernel-chainstate-config-parity; prompt-bytes: 3704 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/kernel-chainstate-config-parity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit the CMake relationship among BUILD_KERNEL_LIB, BUILD_UTIL_CHAINSTATE, BUILD_KERNEL_TEST, functional-test capability flags, install components, pkg-config/header exports, Windows presets, cross builds, reduced exports, and fuzz overrides. For every supported cell prove target membership, generated metadata, dependency closure, and truthful enablement; use configure-only matrices when compilation is too expensive, and fix only mismatches that can be reproduced with exact cache values and target or install evidence.
```

<a id="goal-112"></a>

### 112. Differential and varint formatter input-boundary audit

<!-- slug: difference-formatter-input-bounds; prompt-bytes: 3819 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/difference-formatter-input-bounds.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit custom DifferenceFormatter, CompactSize, varint, and related vector formatters at public P2P, RPC, file, and persisted-state boundaries. Compare declared counts with representable element domains, allocation timing, arithmetic overflow, partial output, retry or penalty behavior, valid maximum cardinality, and malformed-input accounting across every caller. Use count-only malformed fixtures, independent old/new probes, boundary round trips, sanitizer or allocation evidence, history and recurrence searches, and exact caller contracts; avoid repeating repaired GETBLOCKTXN unless a different formatter or failure mode is proven.
```

<a id="goal-113"></a>

### 113. BIP32 seed-length contract and caller parity audit

<!-- slug: bip32-seed-contract-parity; prompt-bytes: 3767 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/bip32-seed-contract-parity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit CExtKey::SetSeed and every wallet, descriptor, fuzz, binding, and release-branch caller against the BIP32 128--512-bit seed contract. Exercise empty, 15, 16, 32, 64, and 65-byte inputs; compare valid-key creation, assertion or error translation, output state, entropy assumptions, zeroization, and wrapper behavior across current and maintained releases. Use source-matched old/new tests, caller/dataflow traces, 32-bit or binding evidence where available, and recurrence fixtures; do not repeat the repaired SetSeed production change without a distinct caller or failure-mode mismatch.
```

<a id="goal-114"></a>

### 114. Taproot satisfaction and wallet fee-bound differential

<!-- slug: taproot-satisfaction-fee-boundaries; prompt-bytes: 3883 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/taproot-satisfaction-fee-boundaries.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Differentially audit tr() and rawtr() satisfaction-size bounds from descriptor parsing through wallet input/vsize/fee and PSBT or external signing. Enumerate keypath versus every scriptpath, leaf script and control-block compact-size prefixes, Merkle depths, Miniscript and multi_a stack counts, maximum-signature policy, multipath expansion, unknown tree metadata, and actual serialized witnesses. Compare descriptor formulas with independent signed-witness and full-transaction size oracles under boundary trees, nested braces, large scripts, duplicate leaves, and unavailable keys; reproduce historical workarounds, require no underestimation, and preserve unknown contracts rather than guessing.
```

<a id="goal-115"></a>

### 115. Compressed script replacement and malformed-UTXO state audit

<!-- slug: compressed-script-replacement-invariant; prompt-bytes: 3990 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/compressed-script-replacement-invariant.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit ScriptCompression::Unser and every TxOutCompression, Coin, undo, database-cursor, and snapshot caller for output-state independence on oversized, truncated, special, and noncanonical compressed-script encodings. Compare fresh and reused destinations, CDBWrapper deserialize-target copy and commit behavior, cursor loops, snapshot export, block undo recovery, and fuzz harnesses. Prove bounded memory and byte consumption at MAX_SCRIPT_SIZE, MAX_SCRIPT_SIZE+1, CompactSize boundaries, truncated payloads, and repeated entries. Use independent round-trip, metamorphic, and malformed-fixture oracles, preserve the historical OOM protection, verify output-on-failure contracts, and add recurrence seeds without repeating the clear-before-replace fix unless a distinct caller or failure mode is shown.
```

<a id="goal-116"></a>

### 116. Generated C++ annotation identifier and type contract audit

<!-- slug: mpgen-cpp-annotation-contract; prompt-bytes: 3976 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/mpgen-cpp-annotation-contract.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit mpgen namespace, wrap, exception, name, and related Text annotations that flow into C++ namespaces, types, method or field identifiers, declarations, and templates. Enumerate valid Cap'n Proto identifier and C++ token domains, then use hostile quotes, backslashes, comments, newlines, punctuation, Unicode, keywords, duplicate scopes, and empty values in isolated schemas. Compare parser acceptance, generated parse trees, compiler diagnostics, downstream include and type availability, and intended compatibility. Validate or reject only the proven unsafe domain before output creation, preserve valid existing schemas, and add independent generation and consumer tests plus history review; keep include-path rejection from Cycle 316 as prior evidence and seek distinct annotation contracts.
```

<a id="goal-117"></a>

### 117. BIP324 alternate-form fixture and peer-harness audit

<!-- slug: bip324-alt-form-harness; prompt-bytes: 3922 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/bip324-alt-form-harness.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit Core transport tests, the Python v2 peer, bitcoinfuzz adapters, and alternate implementations against BIP324 IDs 1-28, BIP434 FEATURE 37, and undefined IDs. For every supported long command, force one-byte and 13-byte encodings in both directions with minimal valid and boundary payloads; compare type, payload, errors, connection state, and version gating. Find helpers that always canonicalize to short form, tests that receive but never emit alternate forms, registry drift, and oracles unable to kill a wrong decoder. Use isolated source-matched fixtures and mutation-sensitive assertions; keep the known rust-bitcoin gap and Core reserved-slot policy as prior evidence, and change production code only for a separately proven local defect.
```

<a id="goal-118"></a>

### 118. Windows native and cross-build artifact-check parity audit

<!-- slug: windows-artifact-check-parity; prompt-bytes: 3761 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/windows-artifact-check-parity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Compare CMake-produced Windows executables, manifests, install/deploy outputs, generated test configuration, and native versus cross CI checkers. Reconcile every target guard and exception list against current source attachments and historical target additions; use AST or mocked checker tests plus Windows-independent graph evidence to catch stale skips, unchecked artifacts, and platform-specific false capability claims. Preserve the repaired bitcoin-chainstate manifest exclusion as prior evidence, and investigate only a changed target, generator, checker, or artifact contract.
```

<a id="goal-119"></a>

### 119. Optimization-mode flag and generated-rule parity audit

<!-- slug: optimization-mode-flag-parity; prompt-bytes: 3887 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/optimization-mode-flag-parity.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit Debug, Release, RelWithDebInfo, Coverage, sanitizer, IPO/LTO, PGO, and explicit optimization append flags across top-level CMake and nested libsecp256k1 projects. Compare cache values, configure summaries, generated compile and link commands, target properties, and final artifacts; detect stale reused-build flags, duplicate or missing flags, C/C++ asymmetry, and policies that rewrite a requested mode. Use isolated configure-only or small-target probes, exact source-matched command checks, and one behavioral test where practical. Treat documented normalization as intentional only when generated rules and tests agree; fix a proven build-contract mismatch without reopening prior compiler matrices.
```

<a id="goal-120"></a>

### 120. Secret-input failure-path ctime coverage audit

<!-- slug: secret-failure-ctime-coverage; prompt-bytes: 3989 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/secret-failure-ctime-coverage.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit secret-bearing crypto entry points for failure paths lacking a ctime or equivalent constant-time regression oracle. Inventory zero or overflow scalars, invalid session randomness, malformed secret-state objects, failed callbacks, output cleanup, and status declassification across ECDSA, ECDH, Schnorr, MuSig, ElligatorSwift, Silent Payments, wallet keys, and bindings. Mark only secret bytes undefined, define documented public outputs and statuses, and use minimized MSan/Valgrind cases plus temporary secret-branch mutations to prove oracle sensitivity. Separate branches on public or explicitly declassified invalid results from secret-dependent control flow; add the smallest focused test for a confirmed gap, preserve raw traces and contracts, and do not claim timing proof from a passing ctime run.
```

<a id="goal-121"></a>

### 121. Loadblock multi-file ordering and deferred-parent recovery audit

<!-- slug: loadblock-order-recovery; prompt-bytes: 3988 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/loadblock-order-recovery.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit -loadblock's repeated-file and within-file ordering contract from argument parsing through ImportBlocks, LoadExternalBlockFile, block-index insertion, activation, restart, and logs. Generate parent-first, child-first, interleaved, split, duplicate, malformed, and missing-parent files and compare one-file versus many-file imports. Establish whether order is documented or inherited from bootstrap linearization; check whether skipped children recover on restart or are silently lost, whether repeated options preserve CLI/config order, and whether failures are observable. Use process-boundary and source-matched regression tests. Change code or help only when contract/history supports it; otherwise preserve the sharp edge as an explicit test/goal and record the smallest design needed for deferred replay.
```

<a id="goal-122"></a>

### 122. Wallet TXO cache lifetime and replacement audit

<!-- slug: wallet-txo-cache-lifetime; prompt-bytes: 3810 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/wallet-txo-cache-lifetime.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit WalletTXO and other caches that store references into replaceable CWalletTx, CTransaction, CTxOut, descriptor, or key-manager objects. Trace witness replacement, imports, migration, reload, reorg, deletion, and descriptor changes. Build same-txid stripped/witness fixtures with pointer-identity and ASan/UBSan controls; search every refresh/invalidation caller and prove no stale reference survives map or shared_ptr replacement. Distinguish immutable aliases from replaceable objects, preserve locks, and fix only confirmed lifetime or stale-state defects. Re-run prior regression seeds and retain minimized transition sequences.
```

<a id="goal-123"></a>

### 123. Taproot fee-estimation test-oracle and workaround audit

<!-- slug: taproot-fee-test-oracle; prompt-bytes: 3903 -->

```text
/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/taproot-fee-test-oracle.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
Audit Taproot descriptor and wallet tests for stale fee-rate overrides, comments, hard-coded margins, fixture choices, and assertions that mask script-path or key-path fee estimation. Trace sendtoaddress, walletcreatefundedpsbt, signing, external signing, PSBT finalization, and serialized witness sizes. Compare automatic estimation with explicit rates across NUMS, nested, multi_a, unknown-tree, maximum-signature, and unavailable-key cases; require fee and vsize assertions that distinguish underestimation rather than mere mempool acceptance. Search history for workarounds, preserve compatibility tests intentionally targeting old releases, and remove only proven stale bypasses with deterministic functional or unit evidence.
```
