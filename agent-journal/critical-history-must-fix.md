# Critical whole-history must-fix sweep

## Cycle 96

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `49`
- Goal: `Critical whole-history must-fix sweep`
- Slug: `critical-history-must-fix`
- Branch: `uber-cycle-96-critical-whole-history-20260729`
- Start HEAD: `0fd60c504baa6dee79663866a06042b0a3bad996`
- `origin/master`: `9b38d077f894d27ea76413b1db1cb040e25dc296`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `29 983` (`origin/master...HEAD`)
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; no `bitcoind`, `test_bitcoin`, or libsecp test process was running. Existing untracked artifacts, including the previous cycle's probe, are preserved and excluded from commits.

### Scope and protocol

Search the full repository history for critical correctness, security, resource, persistence, cryptographic, wallet, and network fixes. For each historical seed, extract the exact invariant, trust boundary, missing guard/state transition, regression oracle, affected configurations, review rationale, and any prerequisite or follow-up. Search for current structural analogs rather than names alone, then verify only a reachable omission that is not already covered by the existing journal or test suite.

Prior cells excluded from this cycle include the Cycle 94 undecodable first cursor key fix, Cycle 95 MuSig parser output-state fix, the prior mempool relay-budget series, the earlier secp backend/vector campaigns, and historical fixes already used as direct evidence in those cycles. A historical bug is a seed, not proof that a similar-looking current site is wrong.

For every candidate record the seed commit and version, bug shape, source-to-sink path, current callers, configuration/module reachability, expected invariant, reproduction or proof, and verdict. Require a failing-before/passing-after regression, minimized fixture/fuzz seed, first-invalid sanitizer/static trace, mutation/coverage delta, or rigorous contract proof before changing code. Keep discovery and verification independent when practical. One self-contained finding per commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with its journal update; if no fix is justified, leave one journal-only close snapshot.

### Initial queue

1. Mine historical security/correctness commits with explicit regression tests and group them by invariant: bounds/overflow, failure cleanup/output state, persistence ordering, network parser/state, and accounting.
2. Search current analogs for the strongest shapes from `1a51aa96` (bounded PSBT keypath data), `1e009146` (oversized locator rejection), `c6c22f18` and `58b53774` (resource/fee arithmetic), and `20286797` (cryptographic boundary handling).
3. Search current analogs for `4691fb07`, `bb1070bb`, `192eab04`, `6132bf81`, `c4a73a8`, `55eaf087`, `f6c37d82`, and recent wallet write-failure fixes, focusing on rollback, output invalidation, and restart symmetry.
4. Search current analogs for `ddbb88ed`, `6fbcd164`, `4f867fc`, `ee9ad3c`, and the relay-budget series, focusing on ignored read failures, bounded queues, accounting, and state after disconnect.
5. Only after a local candidate is concrete, inspect reverts, backports, PR review, and whole-history variants for applicability and prior rejection rationale.

### Evidence ledger

Pending. Each cycle update must append exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, review precedent, limitations, and the next distinct queue.
