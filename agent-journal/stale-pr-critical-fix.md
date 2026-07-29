# Stale PR Critical-Fix Resurrection

## Cycle 94 active investigation

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `56`
- Slug: `stale-pr-critical-fix`
- Branch: `uber-cycle-94-stale-pr-critical-fix-20260729`
- Start HEAD: `b72e2ade288714d66864e2b33048aa7297428bd2`
- `origin/master`: `9b38d077f8`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `29 977` (`origin/master...HEAD`)
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; no relevant Bitcoin Core process was running. Existing untracked artifacts, including the prompt catalog, were preserved and excluded from commits.

### Scope ledger

Mine abandoned or stale Bitcoin Core and libsecp256k1 pull requests for a technically reproducible security, correctness, testing, portability, or performance fix. Reconstruct original intent from the PR diff, review discussion, commits, linked issue, and current callers; then test the current tree rather than blindly rebasing old code.

Initial priority order:

1. Stale PRs with a regression test, sanitizer/fuzzer seed, or concrete reviewer-identified failure whose code path still exists.
2. PRs closed for inactivity or conflicts where the defect shape can be independently reproduced against current HEAD.
3. PRs whose proposed fix is obsolete but whose test, invariant, or negative case reveals an unprotected current boundary.
4. Performance and portability PRs only after correctness/security candidates are checked with stable measurements.

Exclude already mined review threads, already recorded goal journals, and proposals whose only evidence is an old style preference. A closed PR is a seed, not proof that its premise remains true. Do not resurrect a patch until current callers, history, and tests establish the contract.

### Working protocol

For every candidate, record PR number/title/status, source and base commits, linked issue, age and closure reason, touched contract, current-code applicability, review objections, exact reproducer or missing proof, and verdict. Search local history and journals for prior findings before opening a patch. Keep discovery and verification independent when practical. Preserve rejected candidates and their missing evidence so later cycles do not repeat them.

Potential finding classes are: stale security fix still reachable; incomplete follow-up to a merged fix; test-only regression oracle that masks a current defect; portability/build gap; or obsolete proposal correctly rejected by current design. The first experiment must be small and deterministic, with scratch data and a clean current build. No code change is justified until a current failing test, minimized seed, trace, matrix result, or rigorous contract proof exists.

Next actions: inventory closed PRs and local history by security/correctness keywords; select a distinct candidate with a preserved test or review objection; reproduce on current HEAD; then compare the old patch's intended contract with current implementation and coverage.
