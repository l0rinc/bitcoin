# Loupe-Style Scout, Verifier, Fixer, and Reporter Pipeline

## Cycle 85 start

- Selected by the uber loop: exact `shuf -i 0-98 -n 1` -> `63` (`loupe-pipeline`).
- Branch: `uber-cycle-85-loupe-pipeline-20260729`.
- Cycle-start HEAD: `eb9a35d13935d67f56d0a967eca598dfe648bae9` (`journal: close deterministic simulation cycle 84`).
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` is `2 956`.
- Dirty state: only preserved untracked agent/user artifacts are present; no tracked edits at initialization. No relevant process is running.
- Catalog/protocol/manifest hashes: catalog `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Scope: separate scouting, independent verification, fixing, and reporting with leases, regression PoCs, applicability checks, semantic/hash deduplication, and final review evidence.
- Exclusions: do not repeat the closed cycles' individual source cells, and do not treat the pre-existing Codex Security documentation artifacts as proof of a current repository defect. Their candidate ledgers remain provenance to inspect, not an already verified pipeline.

## Campaign contract

Every candidate must have a trust boundary, contract/invariant, source and history evidence, a reproducible discovery artifact, an independent verifier verdict before fixing, a deduplication result, and a final report/commit trail. A scout may produce a PoC or exact fixture but must not silently patch the candidate. A verifier must be able to reject the candidate and must record applicability, reachability, expected behavior, and the first invalid operation or failed oracle. A fixer may act only on a confirmed local finding. A reporter must preserve the raw evidence, limitations, and review rationale.

## Initial hypotheses

1. Existing agent/security ledgers may mix discovery and verification statuses, allowing a candidate to be treated as confirmed without an independent reproducible verdict.
2. Hash/semantic deduplication may collapse distinct recurrence or changed-reachability findings, or fail to connect prior finding fingerprints to current candidates.
3. A report/commit may omit the PoC, verifier receipt, applicability result, or exact source/HEAD, making the final finding non-reproducible even when the code fix is correct.
4. Leases or ownership markers may be absent or advisory, allowing parallel scouts to duplicate work or a fixer to act before verification.

## Initial queue and protocol

- Inventory `agent-journal` finding ledgers, `doc/security` scan artifacts, candidate/review receipts, source inventories, regression PoCs, commit messages, and any lease/ownership mechanism.
- Trace a small number of completed and rejected findings end to end from discovery through independent verification, source fix or dismissal, final report, and journal/state update.
- Search prior findings by exact hash, symbol/path, semantic summary, and commit/PR provenance before selecting a new pipeline cell.
- Choose the highest-risk unproven pipeline contract, create or reuse the smallest deterministic fixture, and record whether the defect is in repository code, test/harness, documentation, tool, dependency, or process evidence.
