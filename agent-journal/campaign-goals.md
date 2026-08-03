# Reusable Continuous Agent Goals for Bitcoin Core, libsecp256k1, Sparrow Wallet, LevelDB, and Related Projects

---
# Uber-goal

<!-- prompt-chars: 3920; utf8-bytes: 3936 -->

Run a continuing evidence-first investigation using the mutable 128-goal catalog in `agent-journal/campaign-goals.md`. This orchestrates campaigns, not repository completion. Continue until a real session/tool limit or external blocker, then leave a precise handoff.

Use `agent-journal/uber-goal-state.md` as ledger and `agent-journal/<slug>.md` per campaign. Before each cycle inspect repo profile, worktree/branch/base/HEAD/remotes/dirty state/jobs, catalog hash, `URGENT.md`, journals/artifacts, indexes, history, issues, and PRs. Record revision, repo/feature/path/diff/working-tree scope, knowledge inputs, tool/model/plugin versions, seed, worker/cost limits, and outputs. Never overwrite unrelated work or upstream refs.

Select pending, reopened, newly promoted, or highest-risk goals with a recorded random draw; an urgent candidate may preempt. Run 2-4 falsifiable iterations, stopping early only for confirmation, bounded exhaustion, or a blocker, then re-rank and rotate. Each inspects code/callers, runs the narrowest experiment, records commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before exposing a fix. Deep runs may use recorded no-new, total-run, worker, and cost limits. No-new is evidence, not completion.

Track reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Match root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/changed scope stays unknown. Preserve WIP tests/fuzzers/harnesses, branches, seeds, traces, failed attempts, false-positive reasons, and negative results with provenance/resume points. Search semantic/hash duplicates first. Reports, specs, sibling code, tools, scanners, and model opinions are leads, not proof.

Every cycle must return entropy. Extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, coverage gaps, tool limits, false-positive conditions, project priors, and cross-project analogies. Amend relevant goals with concrete triggers, negative knowledge, and next experiments. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, parent lineage, and first runnable experiment. Promote only evidence-backed nonduplicates under 4,000 characters; merge/retire overlap without deleting history. Regenerate index, count, character metadata, catalog hash, and lineage. New/amended goals are immediately eligible. Rewording is not entropy; negative results count only when they narrow scope, falsify an assumption, improve an oracle, or create a runnable next step.

Refresh `agent-journal/URGENT.md` after every cycle/verdict change. Keep at most ten live items ordered by severity, reachability, confidence, and next-proof value: 🚨 Critical; 🔴 High; 🟠 Medium/data-loss risk; 🟡 promising; ⚪ blocked; ✅ fixed/verified. State mechanism, trigger/missing proof, evidence, commits, and next action.

Maintain append-only `agent/all-findings` beside feature branches. Copy each fix, test, harness, experiment, evidence, and catalog-evolution commit with source mapping. Never squash/rewrite/force-push; repair/revert broken tips while retaining history.

Use scratch data and deterministic fault injection; leave no jobs running. Confirmed defects get the smallest standalone buildable commit with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. Update journal, ledger, `URGENT.md`, indexes, catalog/lineage, and `agent/all-findings`, then continue.

Adapt by project: Bitcoin Core - consensus/validation/UTXO/P2P/serialization/crypto/storage; libsecp256k1 - arithmetic/constant-time/API/backends; Sparrow - PSBT/signing/hardware-wallet/privacy/backups/releases; LevelDB - recovery/comparators/snapshots/iterators/compaction/corruption/client assumptions. Skip inapplicable surfaces.

---

This revision keeps all 110 prior campaigns, rewrites their shared run protocol around the combined Project Loupe and Codex Security lessons, and adds 18 distinct campaigns. There are **128 standalone `/goal` prompts**.

The catalog is now explicitly self-fueling. Every cycle must return grounded entropy by improving existing goals with new triggers, oracles, negative knowledge, and experiments, and by proposing materially different goals from suspicious code. New goals require provenance, a semantic fingerprint, and a runnable first experiment before promotion. The uber-goal regenerates the index, counts, character metadata, catalog hash, and lineage so promoted goals are selectable on the next draw.

The common protocol adds repository/path/diff/working-tree targeting, replayable scan recipes, preflight checks, knowledge-base inputs, explicit coverage and deferred work, root-cause continuity, conditional false-positive revalidation, risk-ranked deep scans with no-new and cost bounds, partial-result preservation, independent scout/verifier/fixer/reviewer roles, private artifact handling, and cross-scanner evidence. The catalog now adapts to Bitcoin Core, libsecp256k1, Sparrow Wallet, LevelDB, and other security-critical repositories.

Character counting includes the complete text beginning with `/goal` and excludes only headings and code-fence markers. The longest prompt is goal **109**, at **3,936 Unicode characters** and **3,937 UTF-8 bytes**, below the 4,000-byte limit. The uber-goal is **3,920 characters** and **3,936 bytes**. Catalog SHA-256: `d06ff0cdc24ab79e162aed0d93656663b01801abe7c521eb67642cbbc6c421b0`.

## External research incorporated

- [Project Loupe](https://github.com/project-loupe/loupe): separate discovery and verification jobs, prior-finding search, semantic and deterministic deduplication, regression-test PoCs, explicit verdicts, optional independent fixes, worker isolation, and durable findings.
- [Codex Security](https://github.com/openai/codex-security): repository, path, committed-diff, and working-tree targets; standard and deep modes; risk ranking, file review, validation, and attack-path phases; replayable scan history; root-cause matching; coverage-aware status; conditional false-positive reasons; knowledge-base inputs; bulk scans; preflight, cost, worker, and no-new stopping controls.
- [Bitcoin Core fuzzing guide](https://github.com/bitcoin/bitcoin/blob/master/doc/fuzzing.md): deterministic fuzz builds, qa-assets corpora, sanitized and high-throughput runs, multiple fuzz engines, and coverage preservation.
- [bitcoinfuzz](https://github.com/bitcoinfuzz/bitcoinfuzz): differential fuzzing across Bitcoin implementations and cryptographic libraries with shared vectors and corpora.
- [libsecp256k1](https://github.com/bitcoin-core/secp256k1): field/scalar backend matrices, exhaustive tests, constant-time design, ctime/checkmem tests, VERIFY contracts, and portable/assembly paths.
- [Sparrow Wallet](https://github.com/sparrowwallet/sparrow): a Java desktop Bitcoin wallet using PSBTs and hardware wallets, with `drongo` and `lark` submodules and reproducible pre-signing release binaries.
- [LevelDB](https://github.com/google/leveldb): ordered key/value storage with custom comparators, atomic batches, snapshots, iterators, Env-injected filesystem behavior, and a maintenance focus on critical data-loss or memory-corruption defects.
- [Fuzz Introspector](https://google.github.io/oss-fuzz/advanced-topics/fuzz-introspector/), [FuzzTest](https://github.com/google/fuzztest), [Mull](https://github.com/mull-project/mull), [CBMC](https://github.com/diffblue/cbmc), [KLEE](https://klee-se.org/), [FoundationDB simulation](https://apple.github.io/foundationdb/testing.html), [dm-flakey](https://docs.kernel.org/admin-guide/device-mapper/dm-flakey.html), [Alive2](https://github.com/AliveToolkit/alive2), [Csmith](https://github.com/csmith-project/csmith), [YARPGen](https://github.com/intel/yarpgen), [Project Wycheproof](https://github.com/C2SP/wycheproof), [dudect](https://github.com/oreparaz/dudect), [ctgrind](https://github.com/agl/ctgrind), and [LLVM sanitizers](https://clang.llvm.org/docs/UsersManual.html): coverage, property, mutation, bounded-proof, fault-injection, compiler-differential, vector, timing, and runtime-analysis techniques retained from the prior catalog.

## Prompt index

- [0. Continuous evidence-first bug mining](#goal-0) - 3,572 characters, 3,573 bytes
- [1. Source comment versus implementation contract audit](#goal-1) - 3,817 characters, 3,818 bytes
- [2. Assertion, Assume, and invariant reachability audit](#goal-2) - 3,274 characters, 3,275 bytes
- [3. Current branch and PR leftover sweep](#goal-3) - 3,804 characters, 3,805 bytes
- [4. Public API, CLI, RPC, config, and help contract audit](#goal-4) - 3,748 characters, 3,749 bytes
- [5. Boundary-condition and off-by-one audit](#goal-5) - 3,786 characters, 3,787 bytes
- [6. Serialization, deserialization, and untrusted-input sweep](#goal-6) - 3,765 characters, 3,766 bytes
- [7. Untrusted-interface resource-exhaustion variant analysis](#goal-7) - 3,795 characters, 3,796 bytes
- [8. Locking, threading, and scheduler audit](#goal-8) - 3,741 characters, 3,742 bytes
- [9. Hit-frequency and suspicious-branch coverage audit](#goal-9) - 3,848 characters, 3,849 bytes
- [10. Fuzz-target gap and harness-realism audit](#goal-10) - 3,935 characters, 3,936 bytes
- [11. Sanitizer and Valgrind true-positive sweep](#goal-11) - 3,850 characters, 3,851 bytes
- [12. Static-analysis true-positive campaign](#goal-12) - 3,798 characters, 3,799 bytes
- [13. Secret-data lifetime and zeroization audit](#goal-13) - 3,743 characters, 3,744 bytes
- [14. Secret-dependent control-flow and memory-access audit](#goal-14) - 3,734 characters, 3,735 bytes
- [15. Public object parsing and validation variant analysis](#goal-15) - 3,727 characters, 3,728 bytes
- [16. Public API misuse-resistance audit](#goal-16) - 3,772 characters, 3,773 bytes
- [17. Build-matrix and module-configuration audit](#goal-17) - 3,715 characters, 3,716 bytes
- [18. Exhaustive and algebraic-invariant audit](#goal-18) - 3,733 characters, 3,734 bytes
- [19. Benchmark correctness and measurement-integrity audit](#goal-19) - 3,708 characters, 3,709 bytes
- [20. Simple micro-optimization discovery and proof](#goal-20) - 3,714 characters, 3,715 bytes
- [21. Long-running rebuild, recovery, and compaction profiling](#goal-21) - 3,711 characters, 3,712 bytes
- [22. Full sync, IBD, import, and end-to-end profiling](#goal-22) - 3,712 characters, 3,713 bytes
- [23. Perf and flamegraph investigation without forced commits](#goal-23) - 3,703 characters, 3,704 bytes
- [24. Disk I/O, persistence growth, and write-amplification audit](#goal-24) - 3,725 characters, 3,726 bytes
- [25. Recent performance-regression bisect](#goal-25) - 3,739 characters, 3,740 bytes
- [26. Bug fixed in one subsystem but present in another](#goal-26) - 3,842 characters, 3,843 bytes
- [27. Error-path partial-state mutation audit](#goal-27) - 3,845 characters, 3,846 bytes
- [28. Weak-test oracle and mutation-survival audit](#goal-28) - 3,855 characters, 3,856 bytes
- [29. Dead code, stale feature, and TODO archaeology](#goal-29) - 3,755 characters, 3,756 bytes
- [30. Security-sensitive and misleading logging audit](#goal-30) - 3,712 characters, 3,713 bytes
- [31. Cross-layer docs, examples, tests, and implementation audit](#goal-31) - 3,700 characters, 3,701 bytes
- [32. Whole-history incomplete-fix and migration mining](#goal-32) - 3,872 characters, 3,873 bytes
- [33. External vulnerability and advisory variant analysis](#goal-33) - 3,851 characters, 3,852 bytes
- [34. Uncovered-code classification and closure audit](#goal-34) - 3,812 characters, 3,813 bytes
- [35. Mutation-testing campaign](#goal-35) - 3,774 characters, 3,775 bytes
- [36. Cross-tool sanitizer and static-analysis matrix](#goal-36) - 3,705 characters, 3,706 bytes
- [37. Build dead-zone and conditional-compilation audit](#goal-37) - 3,695 characters, 3,696 bytes
- [38. Failure cleanup and crash-safety audit](#goal-38) - 3,707 characters, 3,708 bytes
- [39. Generated-artifact and test-vector determinism audit](#goal-39) - 3,658 characters, 3,659 bytes
- [40. Independent multi-agent disagreement and adjudication audit](#goal-40) - 3,875 characters, 3,876 bytes
- [41. History archaeology from a seed topic](#goal-41) - 3,830 characters, 3,831 bytes
- [42. CI, coverage-bot, and review-bot follow-up audit](#goal-42) - 3,709 characters, 3,710 bytes
- [43. Option and API lifecycle audit](#goal-43) - 3,729 characters, 3,730 bytes
- [44. Secret-copy and compiler-optimization audit](#goal-44) - 3,699 characters, 3,700 bytes
- [45. Constant-time boundary and declassification audit](#goal-45) - 3,701 characters, 3,702 bytes
- [46. Public API output-on-failure audit](#goal-46) - 3,733 characters, 3,734 bytes
- [47. Build-system and CI parity audit](#goal-47) - 3,679 characters, 3,680 bytes
- [48. Property, exhaustive, and algebraic oracle expansion](#goal-48) - 3,892 characters, 3,893 bytes
- [49. Critical whole-history must-fix sweep](#goal-49) - 3,703 characters, 3,704 bytes
- [50. Fuzz Introspector blocker and complexity audit](#goal-50) - 3,689 characters, 3,690 bytes
- [51. Invariant, differential, and metamorphic audit](#goal-51) - 3,836 characters, 3,837 bytes
- [52. Integer overflow, narrowing, signedness, and division audit](#goal-52) - 3,914 characters, 3,915 bytes
- [53. Statistical timing-side-channel campaign](#goal-53) - 3,705 characters, 3,706 bytes
- [54. RAII, smart-pointer, and resource-leak audit](#goal-54) - 3,711 characters, 3,712 bytes
- [55. Alternative-implementation compatibility-difference audit](#goal-55) - 3,880 characters, 3,881 bytes
- [56. Stale PR critical-fix resurrection audit](#goal-56) - 3,720 characters, 3,721 bytes
- [57. Local-reasoning domain and relationship audit](#goal-57) - 3,914 characters, 3,915 bytes
- [58. Exact helper reuse and minimal helper-extension audit](#goal-58) - 3,746 characters, 3,747 bytes
- [59. C/C++ supply-chain and security-gate audit](#goal-59) - 3,754 characters, 3,755 bytes
- [60. Historical reviewer-preference mining and reusable review skill](#goal-60) - 3,723 characters, 3,724 bytes
- [61. Stateful contract-fuzzer expansion](#goal-61) - 3,890 characters, 3,891 bytes
- [62. Rejected-finding resurrection and assumption attack](#goal-62) - 3,761 characters, 3,762 bytes
- [63. Loupe and Codex Security scout, verifier, fixer, and reporter pipeline](#goal-63) - 3,510 characters, 3,511 bytes
- [64. Finding deduplication, recurrence, and semantic-fingerprint audit](#goal-64) - 3,501 characters, 3,502 bytes
- [65. Contributor-branch and work-in-progress radar](#goal-65) - 3,785 characters, 3,786 bytes
- [66. Cherry-pick, backport, and release-branch correctness audit](#goal-66) - 3,751 characters, 3,752 bytes
- [67. Release-to-release behavioral and consensus differential](#goal-67) - 3,718 characters, 3,719 bytes
- [68. Architecture, endianness, word-size, and ABI parity audit](#goal-68) - 3,710 characters, 3,711 bytes
- [69. SIMD, assembly, and portable-reference backend differential](#goal-69) - 3,711 characters, 3,712 bytes
- [70. Compiler, optimization, LTO, PGO, and BOLT differential](#goal-70) - 3,745 characters, 3,746 bytes
- [71. Deterministic simulation and failure-schedule exploration](#goal-71) - 3,867 characters, 3,868 bytes
- [72. Filesystem, power-loss, and crash-consistency injection](#goal-72) - 3,836 characters, 3,837 bytes
- [73. Network fragmentation, reordering, and partial-I/O state-machine audit](#goal-73) - 3,794 characters, 3,795 bytes
- [74. Memory pressure, OOM, allocator, and fragmentation audit](#goal-74) - 3,735 characters, 3,736 bytes
- [75. Build throughput, dependency graph, and container-cache audit](#goal-75) - 3,761 characters, 3,762 bytes
- [76. Reproducible binaries, Guix, and toolchain-provenance audit](#goal-76) - 3,715 characters, 3,716 bytes
- [77. Symbolic execution and bounded-model-checking campaign](#goal-77) - 3,711 characters, 3,712 bytes
- [78. Compiler-transformation validation and miscompile isolation](#goal-78) - 3,763 characters, 3,764 bytes
- [79. Fuzz-corpus stewardship, minimization, and transfer audit](#goal-79) - 3,710 characters, 3,711 bytes
- [80. Fuzz-engine and property-framework differential](#goal-80) - 3,721 characters, 3,722 bytes
- [81. Specification, test-vector, and formal-model drift audit](#goal-81) - 3,890 characters, 3,891 bytes
- [82. secp256k1 field and scalar representation matrix](#goal-82) - 3,736 characters, 3,737 bytes
- [83. secp256k1 group, ecmult, and formula-parity audit](#goal-83) - 3,717 characters, 3,718 bytes
- [84. secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit](#goal-84) - 3,720 characters, 3,721 bytes
- [85. Bitcoin consensus mutation-score and kill-test audit](#goal-85) - 3,731 characters, 3,732 bytes
- [86. Bitcoin chainstate, reorg, prune, and index crash-symmetry audit](#goal-86) - 3,724 characters, 3,725 bytes
- [87. Bitcoin mempool, package, and eviction-accounting audit](#goal-87) - 3,730 characters, 3,731 bytes
- [88. Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit](#goal-88) - 3,739 characters, 3,740 bytes
- [89. Bitcoin P2P transport, permission, and peer-accounting audit](#goal-89) - 3,758 characters, 3,759 bytes
- [90. Whole-PR, commit, and external knowledge-base recipe synthesis](#goal-90) - 3,479 characters, 3,480 bytes
- [91. Compiler and binary-hardening configuration audit](#goal-91) - 3,773 characters, 3,774 bytes
- [92. ABI layout, alignment, aliasing, and object-lifetime audit](#goal-92) - 3,689 characters, 3,690 bytes
- [93. Allocation, syscall, clock, randomness, and callback fault injection](#goal-93) - 3,898 characters, 3,899 bytes
- [94. Bindings, FFI, and language-wrapper parity audit](#goal-94) - 3,711 characters, 3,712 bytes
- [95. Database-engine and persistence-semantics differential](#goal-95) - 3,780 characters, 3,781 bytes
- [96. TODO, FIXME, stub, and deferred-work challenge audit](#goal-96) - 3,868 characters, 3,869 bytes
- [97. C and C++ defect-taxonomy sweep](#goal-97) - 3,804 characters, 3,805 bytes
- [98. Floating-point edge values, sanitizer resurrection, and fuzz-exclusion audit](#goal-98) - 3,926 characters, 3,927 bytes
- [99. Clean-room reimplementation and executable differential audit](#goal-99) - 3,270 characters, 3,271 bytes
- [100. Dangerous-sink reverse reachability and public attack synthesis](#goal-100) - 3,296 characters, 3,297 bytes
- [101. Public-boundary characterization and minimal-fix sequencing](#goal-101) - 3,298 characters, 3,299 bytes
- [102. Durable suspicion artifacts and cross-model replay](#goal-102) - 3,265 characters, 3,266 bytes
- [103. Finding composition and end-to-end exploit-chain synthesis](#goal-103) - 3,266 characters, 3,267 bytes
- [104. Analogical vulnerability translation and target-domain search](#goal-104) - 3,923 characters, 3,924 bytes
- [105. Project vulnerability autopsy and author-feature recurrence mining](#goal-105) - 3,262 characters, 3,263 bytes
- [106. Semantic-twin inconsistency and sloppiness-map audit](#goal-106) - 3,916 characters, 3,917 bytes
- [107. External conformance-suite and sibling-test transplantation](#goal-107) - 3,272 characters, 3,273 bytes
- [108. Adversarial peer, client, file, and environment artifact generation](#goal-108) - 3,274 characters, 3,275 bytes
- [109. Whole-feature cross-file public-path security audit](#goal-109) - 3,936 characters, 3,937 bytes
- [110. Self-fueling catalog evolution and entropy-quality audit](#goal-110) - 3,889 characters, 3,890 bytes
- [111. Coverage manifest, deferred-work, and incomplete-scan closure audit](#goal-111) - 3,848 characters, 3,849 bytes
- [112. Replayable scan recipes, finding continuity, and false-positive revalidation](#goal-112) - 3,353 characters, 3,354 bytes
- [113. Risk ranking, deep-scan stopping, and marginal-yield audit](#goal-113) - 3,294 characters, 3,295 bytes
- [114. Threat-model and knowledge-base conversion into executable oracles](#goal-114) - 3,823 characters, 3,824 bytes
- [115. Committed-diff, working-tree, and pre-commit security regression audit](#goal-115) - 3,796 characters, 3,797 bytes
- [116. Cross-scanner differential and disagreement audit](#goal-116) - 3,792 characters, 3,793 bytes
- [117. Security-agent calibration with historical bugs, mutants, and negative controls](#goal-117) - 3,831 characters, 3,832 bytes
- [118. Agent sandbox, credential, environment, and artifact-isolation audit](#goal-118) - 3,865 characters, 3,866 bytes
- [119. Bulk multi-repository and ecosystem recurrence mining](#goal-119) - 3,843 characters, 3,844 bytes
- [120. Sparrow PSBT, signing-intent, and hardware-wallet verification audit](#goal-120) - 3,808 characters, 3,809 bytes
- [121. Sparrow backend trust, privacy, and network-isolation audit](#goal-121) - 3,821 characters, 3,822 bytes
- [122. Sparrow wallet-file encryption, backup, import, and recovery audit](#goal-122) - 3,864 characters, 3,865 bytes
- [123. Sparrow Java and JavaFX lifecycle, concurrency, and secret-retention audit](#goal-123) - 3,825 characters, 3,826 bytes
- [124. Sparrow build, submodule, update, and release-integrity audit](#goal-124) - 3,815 characters, 3,816 bytes
- [125. LevelDB WAL, MANIFEST, VersionSet, and crash-recovery audit](#goal-125) - 3,840 characters, 3,841 bytes
- [126. LevelDB comparator, snapshot, iterator, filter, and compaction semantics audit](#goal-126) - 3,822 characters, 3,823 bytes
- [127. LevelDB corruption, checksums, background errors, and client-assumption audit](#goal-127) - 3,891 characters, 3,892 bytes

## Goals

<a id="goal-0"></a>

### 0. Continuous evidence-first bug mining

<!-- slug: continuous-bug-mining; prompt-chars: 3572; utf8-bytes: 3573 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/continuous-bug-mining.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Mine the current repository for real security, correctness, performance, portability, testing, documentation, and build defects. Rotate evidence sources and methods instead of rescanning the same files: current diffs, history, TODOs, maintainer-marked sketchy areas, coverage, mutations, sanitizer/static reports, benchmarks, platform matrices, external advisories, sibling implementations, and contributor branches.

Work breadth-first. Run a few shallow evidence-producing passes, preserve every useful artifact, then switch methods so later campaigns inherit seeds, failed assumptions, harnesses, and partial results. Expand a risk map of subsystems, trust boundaries, persistence, concurrency, secrets, and expensive operations. Pick the highest-value unexplored cell, form a falsifiable hypothesis, verify it, update the map, and continue. Do not manufacture commits; keep unproven candidates with the exact missing evidence.

Use explicit ranking, review, validation, attack-path, fix, and independent-review phases when they add value. Record phase-specific coverage and marginal yield so a repeated no-new pass redirects the next campaign instead of ending the investigation.
```

<a id="goal-1"></a>

### 1. Source comment versus implementation contract audit

<!-- slug: comment-code-contract; prompt-chars: 3817; utf8-bytes: 3818 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/comment-code-contract.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit nontrivial source comments against implementation. Prioritize claims using must/never/always, lock or lifetime requirements, cache and ownership rules, serialization formats, compatibility/consensus behavior, secret handling, recovery, bounds, and performance assumptions.

For each claim, identify the exact code and callers it governs, classify the claim, and compare code, tests, docs, blame, and historical rationale. Decide whether code or comment is wrong. Change behavior only when the intended contract is independently supported; otherwise correct the stale claim. Ignore comments that are merely improvable. For behavior changes, prove the old test suite missed the discrepancy and add the narrowest regression oracle.
```

<a id="goal-2"></a>

### 2. Assertion, Assume, and invariant reachability audit

<!-- slug: assertion-invariant-audit; prompt-chars: 3274; utf8-bytes: 3275 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/assertion-invariant-audit.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Search assertions, `Assume`, `Assert`, `CHECK_NONFATAL`, unreachable markers, VERIFY checks, and comments implying validation or impossible states. Extract each invariant and trace every caller, including release, fuzz, RPC/config, network, persisted-data, and optional-module paths.

Also create an intentionally assertion-saturated test branch. Add temporary range, nullability, ownership, state, cache, monotonicity, and checked-arithmetic guards even when they would be too noisy for production. Start at divisions, dereferences, indexes, shifts, casts, and allocations; infer what must be true, then propagate that requirement outward through callers. Fuzz and run public scenarios against the instrumented build. Distinguish internal contract failures from missing untrusted-input validation, and retain minimized violations before deciding which checks belong in production.
```

<a id="goal-3"></a>

### 3. Current branch and PR leftover sweep

<!-- slug: current-pr-leftovers; prompt-chars: 3804; utf8-bytes: 3805 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/current-pr-leftovers.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For every commit on the current branch, state its intended behavior or migration rule, then search the entire tree for analogous sites that should have changed. Look for stale names/comments/tests, duplicate old logic, partial API conversions, forgotten help or release text, generated files, build lists, optional modules, cache/index formats, lock annotations, and cleanup paths.

Use history and review discussion to distinguish deliberate scope from accidental omission. Check each commit independently and the combined stack. Fix only true leftovers, one per follow-up commit, preserving the original intent. Also record unresolved review objections and predict which current changes would trigger the same objection.
```

<a id="goal-4"></a>

### 4. Public API, CLI, RPC, config, and help contract audit

<!-- slug: public-interface-contracts; prompt-chars: 3748; utf8-bytes: 3749 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/public-interface-contracts.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare each public interface end to end: registration, parser, type/range/unit conversion, defaults, aliases, help text, errors, runtime observation, persistence, restart behavior, docs, tests, and release notes. Cover RPC/REST/CLI/config and library headers where present.

Find values that are parsed but ignored, documented but not accepted, stored in the wrong unit/type, applied only on one lifecycle path, or reported with stale fields/bounds. Exercise exact zero/one/max/negative/duplicate/unknown cases. Prefer behavioral assertions over string-only tests. Preserve compatibility unless the intended change is proven from policy, history, and callers.
```

<a id="goal-5"></a>

### 5. Boundary-condition and off-by-one audit

<!-- slug: boundary-off-by-one; prompt-chars: 3786; utf8-bytes: 3787 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/boundary-off-by-one.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Mine comparisons and arithmetic around heights, times, versions, sequence numbers, counts, amounts, offsets, file positions, cache sizes, resource limits, vector/span lengths, varints, scalar/field bounds, epochs, and iterator ranges. Review `<` versus `<=`, zero/one/max, empty/full, first/last, signed sentinels, and wraparound.

For each candidate, write the mathematical domain and expected boundary table before changing code. Test immediately below, at, and above the boundary on 32- and 64-bit-relevant widths. Separate compatibility/consensus rules from local policy. Require a focused failing-before test or executable arithmetic proof; do not change externally visible boundaries from intuition.
```

<a id="goal-6"></a>

### 6. Serialization, deserialization, and untrusted-input sweep

<!-- slug: serialization-untrusted-input; prompt-chars: 3765; utf8-bytes: 3766 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/serialization-untrusted-input.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit network, RPC/config, block/transaction/script, key/signature/scalar, wallet/database/index, WAL/MANIFEST/table, and persisted-state parsers. Trace length and tag fields from bytes to allocations, loops, casts, object mutation, and later assumptions.

Look for missing allocation/CPU bounds, non-canonical encodings, truncation, signedness errors, duplicate encodings, partially initialized outputs, parse-then-assume paths, and failure after state mutation. Define valid and intentionally invalid domains. Add round-trip, canonicalization, negative, and output-on-failure oracles. Use malformed fixtures under ASan/UBSan/allocator limits and preserve minimized inputs.
```

<a id="goal-7"></a>

### 7. Untrusted-interface resource-exhaustion variant analysis

<!-- slug: resource-exhaustion-variants; prompt-chars: 3795; utf8-bytes: 3796 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/resource-exhaustion-variants.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Seed from historical DoS fixes, advisories, fuzz crashes, queue/cache/eviction changes, and comparable projects. Extract the bug shape: unbounded queue/allocation/log/disk growth, repeated expensive work, bad accounting, retry storm, cache bypass, timeout abuse, compaction amplification, or permission-dependent limit bypass.

Trace a realistic attacker or local-input path and calculate an explicit upper bound for CPU, memory, disk, network, descriptors, and retained state. Build a deterministic low-limit reproducer rather than a huge uncontrolled load. Verify release and restart cleanup. Commit only when the bound or accounting failure is demonstrated; report theoretical amplification separately.
```

<a id="goal-8"></a>

### 8. Locking, threading, and scheduler audit

<!-- slug: locking-threading; prompt-chars: 3741; utf8-bytes: 3742 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/locking-threading.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map protected state, mutex ownership, lock order, atomics, condition variables, queues, callbacks, worker shutdown, and object lifetimes. Search annotations, lock assertions, callbacks under locks, read-modify-write splits, early returns, cancellation, and destruction while work remains.

Run TSan and lock-order instrumentation separately. Build deterministic schedules or barriers that amplify suspected races; do not rely on sleeps. Check start/stop/restart and zero/one/many worker configurations. For each issue, identify the first conflicting accesses or lock dependency cycle and prove the minimal fix preserves ordering, progress, and shutdown behavior.
```

<a id="goal-9"></a>

### 9. Hit-frequency and suspicious-branch coverage audit

<!-- slug: hit-frequency-coverage; prompt-chars: 3848; utf8-bytes: 3849 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/hit-frequency-coverage.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Generate line, branch, function, and fuzz hit-frequency coverage, not just percentages. Rank rarely hit branches by security impact, state mutation, complexity, error handling, persistence, secret handling, and proximity to untrusted inputs.

For each high-risk low-hit branch, derive its exact precondition and construct a deterministic trigger. Prioritize collisions, wraparound, clock boundaries, restart states, unusual worker counts, and combinations random fuzzing is unlikely to reach. Explain whether rarity comes from platform/config, a hard guard, harness blockage, dead code, or a missing scenario. Prefer a public-path behavior test over execution-only coverage, compare hit counts, and mutate the branch to prove the new oracle notices wrong behavior.
```

<a id="goal-10"></a>

### 10. Fuzz-target gap and harness-realism audit

<!-- slug: fuzz-target-gaps; prompt-chars: 3935; utf8-bytes: 3936 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/fuzz-target-gaps.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory production entry points against fuzz targets. Find important parsers, state machines, recovery paths, validation branches, and optional modules no target reaches, plus harnesses that use unrealistic state, discard errors, over-constrain inputs, or stop before the real validator.

Upgrade the smallest useful harness through production-like and preferably public construction. Before increasing fuzz volume, add cheap preconditions, postconditions, independent recomputation, inverse relations, and external-reference checks so silent wrongness becomes a failure. Preserve magic values, exclusions, catches, and odd seeds as suspicious evidence and explore their neighbors. Measure static reachability and dynamic coverage, run sanitized and high-throughput builds, retain minimized inputs, and keep harness changes separate from production fixes.
```

<a id="goal-11"></a>

### 11. Sanitizer and Valgrind true-positive sweep

<!-- slug: sanitizer-valgrind; prompt-chars: 3850; utf8-bytes: 3851 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sanitizer-valgrind.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run separate ASan+UBSan, TSan, MSan where fully instrumentable, LeakSanitizer, Valgrind/Memcheck, and relevant fuzz configurations across unit, functional, recovery, benchmark, and narrow long-running workloads.

Treat each warning as a noisy sensor, not a request to silence the tool. Recover why suppressions, skips, or special cases exist; minimize the first invalid operation; then seek a production and preferably public trigger. Classify project bug, test bug, dependency bug, unsupported instrumentation, harmless diagnostic, or false positive, and state the possible security effect. Fix the root cause only after that classification, prove the warning disappears without a broader suppression, and retain the raw trace, reproducer, and nearby untested variants.
```

<a id="goal-12"></a>

### 12. Static-analysis true-positive campaign

<!-- slug: static-analysis-true-positives; prompt-chars: 3798; utf8-bytes: 3799 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/static-analysis-true-positives.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run the repository's linters plus focused clang-tidy, clang static analyzer, CodeQL/Semgrep, IWYU, compiler warnings, and semantic queries. Prioritize lifetime, nullability, use-after-move, uninitialized state, narrowing, overflow, unchecked results, span/string_view lifetime, iterator invalidation, lock contracts, dead stores, and suspicious control flow.

Use text search only to route candidates; prove them through types, call/dataflow, and execution where possible. Build custom queries from proven historical bug shapes. Reject style-only output and document false-positive patterns so later cycles do not repeat them. Each fix must preserve project idioms and include the exact warning/query path.
```

<a id="goal-13"></a>

### 13. Secret-data lifetime and zeroization audit

<!-- slug: secret-lifetime-zeroization; prompt-chars: 3743; utf8-bytes: 3744 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secret-lifetime-zeroization.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Label private keys, nonces, seeds, tweaks, blinding values, passphrases, session material, authentication data, and secret-derived temporaries. Trace every copy, allocation, move, early return, exception/error path, callback capture, log, swap, and destructor.

Verify clearing on all exits and whether the compiler eliminates it; inspect optimized assembly or use project cleanse/checkmem utilities, Valgrind/MSan secret marking, and ctime tests. Distinguish secret, public, and intentionally declassified data. Avoid broad memset churn. Prove the value is sensitive, the old lifetime is avoidably longer, and the chosen mechanism survives optimization.
```

<a id="goal-14"></a>

### 14. Secret-dependent control-flow and memory-access audit

<!-- slug: secret-control-flow; prompt-chars: 3734; utf8-bytes: 3735 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secret-control-flow.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Starting from signing, key generation, nonce/session handling, ECDH/MuSig, encryption/authentication, context randomization, and secret scalar multiplication, trace secret taint into branches, loop counts, array/table indexes, memory addresses, helper selection, and error exits.

Compare constant-time and variable-time helpers, alternative backends, debug/VERIFY modes, and compiler output. Mark explicit declassification boundaries and challenge each one. Use ctime tests, ctgrind/Valgrind or MSan secret marking, dudect-style statistics, and assembly traces. A timing test that passes is supporting evidence, not proof; retain the dataflow argument.
```

<a id="goal-15"></a>

### 15. Public object parsing and validation variant analysis

<!-- slug: public-object-validation; prompt-chars: 3727; utf8-bytes: 3728 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/public-object-validation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare every equivalent parse/validation path for public keys, x-only keys, signatures, scalars, scripts, descriptors, records, table/log entries, addresses, and API wrappers. Seed from historical malformed-input bugs in this and related projects.

Test truncated, oversized, out-of-range, non-canonical, infinity/impossible, duplicate, and mixed-format inputs. Verify failure is consistent, non-crashing, and leaves outputs in the documented safe state. Cross-check parse/serialize round trips and operations after parse. If another implementation diverges, identify which contract is wrong rather than treating majority behavior as truth.
```

<a id="goal-16"></a>

### 16. Public API misuse-resistance audit

<!-- slug: api-misuse-resistance; prompt-chars: 3772; utf8-bytes: 3773 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/api-misuse-resistance.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Read public headers, examples, bindings, tests, and implementation as an adversarial caller. Look for unclear ownership/lifetime, aliasing, context capability, thread-safety, secret/public status, callback obligations, invalidation, optional-module behavior, inconsistent return conventions, and outputs usable after failure.

Construct the smallest plausible misuse example and determine whether docs, examples, types, assertions, or implementation should change. Prefer clarifying and testing the existing contract over API redesign. Check that examples demonstrate validation and cleanup. Commit implementation changes only for a concrete unsafe or ambiguous behavior with a reproducer.
```

<a id="goal-17"></a>

### 17. Build-matrix and module-configuration audit

<!-- slug: build-matrix-modules; prompt-chars: 3715; utf8-bytes: 3716 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/build-matrix-modules.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Enumerate supported compilers, build types, feature modules, wallet/IPC/GUI/tool/bench/fuzz toggles, assembly/SIMD backends, debug/VERIFY/exhaustive modes, static/shared libraries, cross builds, and sanitizer combinations. Compare that inventory with CI.

Cycle through uncovered pairwise and high-risk interactions, not only individual flags. Detect sources/tests omitted under one configuration, stale guards, examples that fail, behavior-changing defaults, and generated/install manifests that drift. Use separate build directories and record native versus cross/emulated evidence. Fix one proven configuration mismatch at a time.
```

<a id="goal-18"></a>

### 18. Exhaustive and algebraic-invariant audit

<!-- slug: exhaustive-algebraic; prompt-chars: 3733; utf8-bytes: 3734 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/exhaustive-algebraic.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Identify operations with formal identities: parse/serialize, add/subtract, multiply/invert, negate twice, normalize/idempotence, tweak relations, sign/verify, connect/disconnect, write/read/recover, insert/delete, iterator forward/backward, and cache recomputation.

State each identity and its valid domain before testing. Exercise exhaustive small domains where available and deterministic randomized properties elsewhere, including invalid inputs and failure-state guarantees. Compare optimized and reference paths. Use a temporary mutation to prove the oracle is sensitive; if a property exposes a bug, minimize the counterexample and preserve it.
```

<a id="goal-19"></a>

### 19. Benchmark correctness and measurement-integrity audit

<!-- slug: benchmark-integrity; prompt-chars: 3708; utf8-bytes: 3709 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/benchmark-integrity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit benchmark names, setup, timed regions, batching, units, input realism, cache state, I/O, allocation, compiler-elision barriers, fixture reuse, and secret-path representativeness. Ensure the benchmark measures the claimed production operation and validates its result.

Run release-like builds, at least five comparable repetitions, and report raw samples, median, spread, outliers, environment, and profile attribution. Check debug/sanitizer runs only for correctness. Fix misleading benchmarks separately from production optimizations and use a temporary no-op or deliberate slowdown to prove the harness detects change.
```

<a id="goal-20"></a>

### 20. Simple micro-optimization discovery and proof

<!-- slug: micro-optimization; prompt-chars: 3714; utf8-bytes: 3715 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/micro-optimization.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Use existing benchmarks and profiles to locate a narrow hot operation. Form one hypothesis involving avoidable allocation/copy/hash/lookup/branch/serialization/lock or better reuse of an existing helper. Prefer code that becomes no more complex.

Benchmark clean base and candidate with identical release flags and at least five interleaved runs; inspect assembly or perf counters when causality is unclear. Run correctness, sanitizer, and relevant fuzz/property tests. Reject wins within noise, workload-specific regressions, and changes that weaken invariants or readability. After each result, return to the next measured hot site.
```

<a id="goal-21"></a>

### 21. Long-running rebuild, recovery, and compaction profiling

<!-- slug: rebuild-recovery-profile; prompt-chars: 3711; utf8-bytes: 3712 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/rebuild-recovery-profile.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select a reproducible rebuild, reindex, rescan, recovery, snapshot load, index build, or compaction workload on scratch data. Record hardware, OS, compiler, flags, filesystem/storage, cache settings, data preparation, commit, and stop condition.

Capture wall/CPU time, peak RSS, reads/writes, fsyncs, compactions, progress, and perf stacks over representative phases. Classify CPU, I/O, lock, allocator, logging, serialization, crypto, or database bottlenecks. Test one minimal hypothesis, rerun identically, and require the expected profile movement as well as a reproducible metric win and correctness/recovery validation.
```

<a id="goal-22"></a>

### 22. Full sync, IBD, import, and end-to-end profiling

<!-- slug: full-sync-ibd-profile; prompt-chars: 3712; utf8-bytes: 3713 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/full-sync-ibd-profile.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run a controlled local IBD/full-sync/import/reindex replay using fixed peers or local block data and a scratch datadir. Pin stop height/range, validation shortcuts, cache/prune/index settings, parallelism, source data, compiler, CPU policy, and storage.

Collect wall/CPU/RSS/disk/network/progress series, logs, and sampled profiles. Separate download, validation, script/crypto, chainstate, block I/O, compaction, and logging time. Do not infer a CPU or disk win from network-bound wall time. Change one bottleneck at a time and require repeated before/after runs, matching final chainstate/results, and no privacy/DoS tradeoff.
```

<a id="goal-23"></a>

### 23. Perf and flamegraph investigation without forced commits

<!-- slug: perf-flamegraph-investigation; prompt-chars: 3703; utf8-bytes: 3704 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/perf-flamegraph-investigation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Choose one representative benchmark, functional test, daemon workload, build, or recovery phase. Record exact environment and capture perf data, flamegraphs, scheduler/lock views, CPU counters, RSS, disk, network, and process metrics.

Distinguish self versus child time, on-CPU versus I/O wait, lock contention, allocator overhead, logging, serialization, hashing/crypto, database/cache, and harness overhead. Rank fix hypotheses by expected impact and risk. Commit only a trivial, proven, measured fix; otherwise leave a detailed journal with raw artifact paths, commands, call stacks, and the next experiment.
```

<a id="goal-24"></a>

### 24. Disk I/O, persistence growth, and write-amplification audit

<!-- slug: disk-io-amplification; prompt-chars: 3725; utf8-bytes: 3726 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/disk-io-amplification.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Measure a fixed storage-heavy workload with process and device counters, filesystem usage, database logs, fsync traces, temporary files, and persistent-state growth. Identify redundant reads, repeated serialization, cache bypass, extra flushes, compaction amplification, stale files, excessive logs, and tests leaking artifacts.

State where data should reside: memory/cache, WAL/log, table/SST, block/index file, or durable metadata. Stop processes before corrupting files so caches cannot mask behavior. A fix must preserve crash consistency and observable state while reducing measured bytes, syncs, or retained space across repeated runs.
```

<a id="goal-25"></a>

### 25. Recent performance-regression bisect

<!-- slug: performance-regression-bisect; prompt-chars: 3739; utf8-bytes: 3740 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/performance-regression-bisect.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Choose a stable benchmark or controlled workload and compare a justified recent commit range under identical conditions. If a regression exceeds noise, bisect to the first bad commit, then profile last-good and first-bad with matching symbols and data.

Explain the causal code path, including changed work counts, cache behavior, allocations, I/O, locking, or compiler output. Preserve the original commit's correctness intent with the smallest fix. Require an exact bisect log, interleaved repeated measurements, before/after profiles, and relevant correctness tests. If no regression is proven, record tested ranges and move to another workload.
```

<a id="goal-26"></a>

### 26. Bug fixed in one subsystem but present in another

<!-- slug: cross-subsystem-bug-shapes; prompt-chars: 3842; utf8-bytes: 3843 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cross-subsystem-bug-shapes.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Mine bug-fix commits, release notes, regression tests, and review comments for reusable defect shapes. Convert each seed into source, sink, missing guard, invalid transition, cleanup omission, accounting rule, stale invariant, and missing oracle.

After a confirmed bug, widen the search around the same feature and subsystem, then use blame and the introducing author's related commits or series as strong ranking signals. Search semantic siblings rather than names alone across parsers, caches, indexes, queues, and APIs. Author or feature provenance is not proof: each candidate needs independent reachability, protection analysis, and a mirrored reproducer or mutation. Record why each omission is intentional or another member of the defect cluster.
```

<a id="goal-27"></a>

### 27. Error-path partial-state mutation audit

<!-- slug: error-path-state; prompt-chars: 3845; utf8-bytes: 3846 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/error-path-state.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Find functions returning bool/status/result/optional, throwing, or using output parameters while mutating objects, caches, maps, counters, files, transactions, indexes, or caller-visible buffers. Enumerate every failure edge and state changed before it.

Infer whether failure promises unchanged, zeroed, rolled back, invalidated, or explicitly partial state. Inject failure at the earliest and latest practical points and compare complete pre/post state. Then walk the failure condition upward through callers, translating it at each layer until it reaches a public API, network input, file, config, or command, or until a proven guard blocks it. Prefer a functional reproduction, and verify retry/restart behavior before making the smallest contract-preserving fix.
```

<a id="goal-28"></a>

### 28. Weak-test oracle and mutation-survival audit

<!-- slug: weak-test-oracles; prompt-chars: 3855; utf8-bytes: 3856 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/weak-test-oracles.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Search tests that only assert success/no-crash/non-null/non-empty, ignore returns, catch broadly, check logs instead of state, duplicate implementation logic, rely on sleeps, or omit negative cases. Identify the exact behavior each test claims to protect.

Use Mull or temporary mutations: invert/remove a branch or call, alter a bound, skip a state update, or corrupt a result. If the test survives, add the smallest independent property or postcondition that kills the mutant. Prefer multiple oracles and a public/functional reproduction when the helper may be unused. For behavior changes, preserve a characterization of the old result and prove the corrected expectation fails on clean HEAD. Prioritize consensus, crypto, persistence, wallet, networking, and recent fixes.
```

<a id="goal-29"></a>

### 29. Dead code, stale feature, and TODO archaeology

<!-- slug: dead-stale-code; prompt-chars: 3755; utf8-bytes: 3756 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/dead-stale-code.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory uncalled functions, impossible branches, unused parameters/enums/options, obsolete compatibility paths, dormant macros, duplicated implementations, stale tests/docs, and TODO/FIXME/XXX comments. Check every supported build/module and distinguish production from test/fuzz/bench-only reachability.

Use history and linked PRs to decide whether code is intentionally staged, retained compatibility, or genuinely dead. Remove only with call-graph/build/coverage proof, or move harness-only helpers to test support. For TODOs, verify the premise still exists and search whether later work solved it elsewhere. Do not convert harmless defensive checks into cleanup commits.
```

<a id="goal-30"></a>

### 30. Security-sensitive and misleading logging audit

<!-- slug: security-logging; prompt-chars: 3712; utf8-bytes: 3713 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/security-logging.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Trace secrets, private metadata, peer/network identifiers, wallet details, paths, auth/config values, raw payloads, and potentially attacker-controlled strings into logs and errors. Also audit severity/category, rate, repetition, truncation, escaping, and claims that misdescribe actual state.

Classify each value as secret/private/public/intentionally disclosed and each message as user-actionable, operational, debug, or unreachable. Reproduce exact output and volume. Fix concrete leaks, injection/confusion, amplification, or wrong diagnostics; do not merely rewrite prose. Validate redaction and that useful correlation remains.
```

<a id="goal-31"></a>

### 31. Cross-layer docs, examples, tests, and implementation audit

<!-- slug: cross-layer-contracts; prompt-chars: 3700; utf8-bytes: 3701 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cross-layer-contracts.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select one externally meaningful feature at a time and compare its complete contract across source comments, public docs, examples, tests, API schemas, help, release notes, and implementation. Unlike the source-comment campaign, focus on contradictions between layers and copied claims.

Build a contract table for inputs, outputs, defaults, failure behavior, compatibility, lifetime, security, and performance. Use blame/PR discussion to identify the authoritative layer. Fix the smallest proven mismatch and add a behavioral test where the contract was implicit. Record merely unclear wording without committing it.
```

<a id="goal-32"></a>

### 32. Whole-history incomplete-fix and migration mining

<!-- slug: whole-history-leftovers; prompt-chars: 3872; utf8-bytes: 3873 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/whole-history-leftovers.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Walk history in manageable ranges, prioritizing security, correctness, parsers, persistence, crypto, locking, resources, API migrations, and regression-test commits. Require proof of work for every reviewed commit or bounded range: intent, changed invariant, affected callers, missing oracle, sibling sites, and an explicit verdict; never accept a bare "looks fine."

For each seed, extract the pre-fix shape and intended repository-wide rule. Search current HEAD for surviving shapes in code, tests, docs, examples, generated files, bindings, build manifests, related author series, and the same feature. Use blame to avoid applying obsolete rules. Prove equivalence and present reachability before fixing, and journal exact range checkpoints so later models resume rather than restart.
```

<a id="goal-33"></a>

### 33. External vulnerability and advisory variant analysis

<!-- slug: external-vulnerability-variants; prompt-chars: 3851; utf8-bytes: 3852 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/external-vulnerability-variants.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Collect relevant CVEs, advisories, security commits, OSS-Fuzz issues, compiler/sanitizer cases, and bugs in Bitcoin nodes, crypto libraries, databases, parsers, and bindings. Record affected version, patch, exploit shape, trust boundary, and why the bug escaped.

Do not search only for literal equivalents. Abstract each seed into its underlying failure, such as untrusted data crossing an interpreter boundary, stale authority, partial commit, unchecked size, or confused ownership. Generate target-specific analogies across network, RPC/config, script, file/database, wallet, build, and crypto surfaces, then test the most reachable one. An analogy is only a lead: require a minimal local vector and current mitigation analysis before reporting or fixing.
```

<a id="goal-34"></a>

### 34. Uncovered-code classification and closure audit

<!-- slug: uncovered-code-classification; prompt-chars: 3812; utf8-bytes: 3813 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/uncovered-code-classification.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Generate current unit, functional, RPC, and fuzz coverage and process uncovered regions systematically. Classify each as platform/config-only, hard error path, missing scenario, harness artifact, genuinely dead, or unreachable because of a bug.

For important uncovered code, derive the exact conditions needed and force them deterministically rather than waiting for probability. Trace from a production or public entry point, add temporary assertions around the rare state, and create the narrowest behavior-checking test. For dead code, prove absence across supported builds and history; for harness artifacts, repair the harness. Maintain a line/range ledger and reject tests whose only effect is executed-line count.
```

<a id="goal-35"></a>

### 35. Mutation-testing campaign

<!-- slug: mutation-testing; prompt-chars: 3774; utf8-bytes: 3775 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/mutation-testing.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run focused mutation testing on high-risk modules and recent changes. Include condition inversions, removed calls/state writes, arithmetic/operator changes, boundary shifts, return-value substitutions, and error-path omissions. Use Mull where practical and targeted temporary mutations elsewhere.

Classify survivors as weak oracle, equivalent mutant, unreachable code, wrong test selection, or potentially missing behavior. Kill valuable non-equivalent mutants with minimal property assertions or reveal a production bug. Track mutation score by subsystem but optimize for dangerous survivors, not percentage. Re-run mutations after each test change and preserve exact mutant identifiers/output.
```

<a id="goal-36"></a>

### 36. Cross-tool sanitizer and static-analysis matrix

<!-- slug: cross-tool-analysis-matrix; prompt-chars: 3705; utf8-bytes: 3706 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cross-tool-analysis-matrix.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build a matrix spanning GCC/Clang versions, ASan, UBSan subchecks, TSan, MSan, LSan, Valgrind, `_GLIBCXX_ASSERTIONS` or equivalent hardening, clang-tidy/static analyzer, and project lint jobs. Exercise representative unit, fuzz, functional, benchmark, and recovery paths.

Look for defects visible only under one optimizer/compiler/tool combination and for suppressions or disabled checks that create blind zones. Minimize and independently confirm every report. Fix project/test bugs only; document dependency/tool issues with versions. Continue filling matrix cells and prioritize untested high-risk configurations.
```

<a id="goal-37"></a>

### 37. Build dead-zone and conditional-compilation audit

<!-- slug: build-dead-zones; prompt-chars: 3695; utf8-bytes: 3696 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/build-dead-zones.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map every `#if`, feature macro, platform/compiler guard, source-list condition, test skip, and CI exclusion to configurations that make it true and false. Identify code no supported build compiles, code compiled but never tested, and high-risk combinations absent from CI.

Use preprocessor output, compile databases, CMake traces, and cross/emulated builds. Check guard polarity, stale feature detection, declaration/definition parity, and release/package inclusion. Fix only demonstrated dead zones or unintended exclusions. Record unsupported combinations distinctly so they are not mistaken for project contracts.
```

<a id="goal-38"></a>

### 38. Failure cleanup and crash-safety audit

<!-- slug: failure-cleanup-crash-safety; prompt-chars: 3707; utf8-bytes: 3708 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/failure-cleanup-crash-safety.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit operations that acquire memory, locks, files, sockets, handles, transactions, temp files, background work, or durable state and can fail or be interrupted. Model authoritative state, commit point, rollback behavior, retry, startup recovery, and cleanup on all exits.

Inject failures before/after each meaningful mutation or durable write. Test clean failure, abrupt termination, and restart where relevant. Verify no leaked resource, mixed in-memory/on-disk state, stale marker, double action, or falsely advanced progress. Prefer exposing a narrow deterministic fault hook over sleeps or probabilistic timing.
```

<a id="goal-39"></a>

### 39. Generated-artifact and test-vector determinism audit

<!-- slug: generated-artifact-determinism; prompt-chars: 3658; utf8-bytes: 3659 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/generated-artifact-determinism.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory generated headers/tables, chain parameters, snapshots, test vectors, docs, schemas, source lists, and codegen outputs. Find the generator, pinned inputs/tool versions, and documented command for each.

Regenerate in a clean locale/timezone and, where practical, a second compiler/OS. Explain every diff: stale artifact, unstable ordering, timestamps, locale, randomness, dependency drift, or undocumented manual edits. Fix generator and artifact together only when inseparable. Require byte-identical repeat generation and a zero-diff verification command.
```

<a id="goal-40"></a>

### 40. Independent multi-agent disagreement and adjudication audit

<!-- slug: multi-agent-adjudication; prompt-chars: 3875; utf8-bytes: 3876 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/multi-agent-adjudication.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Use separate roles: scout proposes a candidate and evidence without a fix; verifier reproduces it, searches prior findings, and locks confirmed/dismissed/inconclusive; fixer sees only confirmed evidence and makes the minimal change; reviewer attacks both proof and patch against project precedent.

Force independent work where useful: one agent writes a behavioral specification or harness, another model or family reimplements or checks it without inheriting the first verdict, and a third adjudicates divergences. Record artifacts, disagreements, changed verdicts, and the missing instrument that would resolve them. Never let an available patch decide whether the finding is real. Preserve rejected candidates as negative knowledge and replay them after relevant code or model changes.
```

<a id="goal-41"></a>

### 41. History archaeology from a seed topic

<!-- slug: history-seed-archaeology; prompt-chars: 3830; utf8-bytes: 3831 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/history-seed-archaeology.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Choose one seed topic or known project bug per cycle, such as cleanup, timing, locks, limits, cache/recovery invariants, migrations, fuzz regressions, or API lifecycle. Use `git log --grep`, `-S`, `-G`, blame, PRs, and release notes to reconstruct its evolution and how the defect or blind spot entered.

Extract the failed assumption, missing test or oracle, review conditions, author/series, affected feature, rejected alternatives, and follow-ups. Search current source, tests, docs, and build files for siblings and unimplemented implications. Name seed commits and explain why the old constraint still applies. Produce a concrete archaeology artifact for every range and immediately continue with the highest-risk project-specific pattern.
```

<a id="goal-42"></a>

### 42. CI, coverage-bot, and review-bot follow-up audit

<!-- slug: ci-review-bot-followup; prompt-chars: 3709; utf8-bytes: 3710 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/ci-review-bot-followup.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Collect current branch CI logs, sanitizer/fuzz failures, static-analysis findings, coverage deltas, flaky-test evidence, and review-bot annotations. Map each result to the exact live line and commit, accounting for stale runs and rebases.

Reproduce locally or in the closest documented environment. Classify true bug, missing test, infrastructure issue, dependency/tool defect, stale warning, or style noise. Search whether reviewers already resolved or rejected it. Fix only meaningful current issues, keeping author commits intact unless explicitly asked, and preserve links plus raw output in the journal and commit body.
```

<a id="goal-43"></a>

### 43. Option and API lifecycle audit

<!-- slug: option-api-lifecycle; prompt-chars: 3729; utf8-bytes: 3730 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/option-api-lifecycle.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For recently touched or suspicious options/APIs, follow creation through registration, parse, validation, storage, observation, scheduling, persistence, restart, migration, disablement, and removal. Cover startup/runtime, first-run, shutdown, reindex/import, offline retry, optional-feature, and non-primary modes.

For periodic or random triggers, expose deterministic due/force hooks and test both pure predicate arithmetic and lifecycle behavior. Check duplicate scheduling and edge-trigger consumption. Prefer no persisted bookkeeping when harmless replay is intended. Fix only an observable lifecycle mismatch with exact command/output proof.
```

<a id="goal-44"></a>

### 44. Secret-copy and compiler-optimization audit

<!-- slug: secret-copy-optimization; prompt-chars: 3699; utf8-bytes: 3700 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secret-copy-optimization.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Find secret-bearing structs, arrays, scalars, and buffers copied by value, returned, inlined, captured, swapped, spilled, or retained in tests/benchmarks. Pay special attention to forced-inline macros, aggregate assignments, ABI copies, vectorization, and cleanup moved across optimization.

Compare `-O0/-O2/-O3`, GCC/Clang, LTO, and relevant architectures using optimized assembly and checkmem tools. Trace every physical and semantic copy and all exits. Reduce copies or clear them only when the secret lifetime and compiler behavior are proven. Re-check performance and constant-time properties after changes.
```

<a id="goal-45"></a>

### 45. Constant-time boundary and declassification audit

<!-- slug: constant-time-declassification; prompt-chars: 3701; utf8-bytes: 3702 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/constant-time-declassification.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map functions reachable from secret operations and mark every variable secret, public, or declassified. Audit where secret-derived values cross into variable-time helpers, logging, error reporting, table lookup, loop bounds, branches, or public outputs.

For each declassification, state why the value is already public or safe to reveal and whether failure/success itself leaks information. Compare backends, optional modules, VERIFY/exhaustive modes, and compiler output. Narrow overly broad declassification and add ctime/checkmem coverage. Preserve an explicit dataflow proof even when dynamic tools pass.
```

<a id="goal-46"></a>

### 46. Public API output-on-failure audit

<!-- slug: api-output-on-failure; prompt-chars: 3733; utf8-bytes: 3734 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/api-output-on-failure.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For every exported function with output parameters or caller-visible object mutation, document return convention and failure-state contract: unchanged, zeroed, invalidated, partially specified, or guaranteed initialized. Compare header docs, examples, bindings, tests, and implementation.

Exercise malformed inputs, invalid contexts/capabilities, aliasing, invalid tweaks/keys/signatures, callback failure, and module-specific errors. Pre-fill outputs with sentinels and verify exact post-state. Fix implementation only when it violates the supported contract; otherwise make docs/tests explicit. Check that callers never consume unspecified output.
```

<a id="goal-47"></a>

### 47. Build-system and CI parity audit

<!-- slug: build-ci-parity; prompt-chars: 3679; utf8-bytes: 3680 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/build-ci-parity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare CMake, remaining alternate build/package systems, presets, CI setup scripts, install/export manifests, examples, benches, fuzzers, exhaustive/ctime tests, optional modules, and cross-platform jobs. Label native, cross, emulator, and artifact-only evidence.

Find flags or files present in one path but absent in another, tests silently skipped, different defaults, stale generated lists, and package/install omissions. Validate each side in separate clean directories. Fix only parity defects where the project claims equivalent support; document intentional asymmetry and its review precedent.
```

<a id="goal-48"></a>

### 48. Property, exhaustive, and algebraic oracle expansion

<!-- slug: property-oracle-expansion; prompt-chars: 3892; utf8-bytes: 3893 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/property-oracle-expansion.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Review unit, randomized, exhaustive, fuzz, and integration tests for operations that check success but not the strongest relation. Add identities, inverse/replay, idempotence, canonicalization, monotonicity, ordering, failure-no-mutation, and derived-state recomputation over the broadest cheap domain.

For caches, indexes, counters, summaries, and incremental state, build a slower independent truth path and compare after every operation, not only at the end; transient divergence may be the bug window. For secp256k1 use exhaustive groups, Sage-derived relations, modules, and backend comparisons. For Bitcoin use serialization, script/sighash, coins cache, mempool/package, connect/disconnect, indexes, and wallet models. Prove each oracle with a targeted mutation and avoid copying production logic.
```

<a id="goal-49"></a>

### 49. Critical whole-history must-fix sweep

<!-- slug: critical-history-sweep; prompt-chars: 3703; utf8-bytes: 3704 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/critical-history-sweep.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Progress from initial commit to HEAD in recorded ranges, inspecting only reachable critical defects: UB/memory corruption, untrusted crash/DoS, consensus divergence, funds/key/privacy loss, data corruption, parser failure, race/deadlock, secret leakage, or omission of a critical check.

For each historical change, ask whether its old bug shape, partial migration, or review concern survives on current HEAD. Prove present reachability and severity from first principles. Skip cleanup, minor docs, and nice-to-have tests. Journal range checkpoints so repeated sessions eventually cover every commit without restarting.
```

<a id="goal-50"></a>

### 50. Fuzz Introspector blocker and complexity audit

<!-- slug: fuzz-introspector-blockers; prompt-chars: 3689; utf8-bytes: 3690 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/fuzz-introspector-blockers.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Use Fuzz Introspector or equivalent call-tree analysis to compare static reachable complexity with dynamic coverage for every target. Rank blockers by the amount and risk of code hidden behind them, not merely branch count.

Trace each blocker to input structure, checksum/magic, global state, early validation, polymorphism, environment dependence, or harness construction. Decide whether to add a dictionary/custom mutator, structured input, realistic setup, new target, or no change because validation is the subject. Show before/after reachability, coverage, hit counts, and preserved determinism.
```

<a id="goal-51"></a>

### 51. Invariant, differential, and metamorphic audit

<!-- slug: differential-metamorphic; prompt-chars: 3836; utf8-bytes: 3837 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/differential-metamorphic.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Enumerate equivalent implementations and inverse/state relations: fast/reference, old/new, scalar/SIMD, C/assembly, parser/serializer, incremental/recompute, batch/split, apply/revert, write/recover, iterator directions, sibling libraries, and independently reimplemented references.

Define the shared domain and permitted differences before comparing. Feed identical valid, invalid, boundary, and stateful vectors; normalize outputs and isolate side effects. When neither side is authoritative, use a specification, third implementation, or algebraic oracle. Minimize divergences, trace them to a public caller where possible, identify which implementation or contract is wrong, and retain both the vector and a mutation proving oracle sensitivity.
```

<a id="goal-52"></a>

### 52. Integer overflow, narrowing, signedness, and division audit

<!-- slug: integer-arithmetic-audit; prompt-chars: 3914; utf8-bytes: 3915 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/integer-arithmetic-audit.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Trace externally and internally derived integers into additions, multiplications, shifts, divisions/modulo, negation, casts, allocations, indexes, offsets, time arithmetic, resource accounting, and serialization widths. Search zero divisors, `INT_MIN/-1`, signed overflow, unsigned wrap, truncation, and implementation-defined shifts.

Write the mathematical range and platform assumptions. In an isolated test branch, replace suspicious operations with checked versions and assertions so overflow, underflow, invalid subtraction, impossible time deltas, and narrowing become loud. Start from each dangerous sink, infer its required domain, and propagate it outward through callers to a public input. Test exact boundaries on relevant widths and sanitizers. Keep checked types only when they simplify a proven production defect.
```

<a id="goal-53"></a>

### 53. Statistical timing-side-channel campaign

<!-- slug: timing-side-channel; prompt-chars: 3705; utf8-bytes: 3706 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/timing-side-channel.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select one secret-bearing primitive or API boundary per cycle. Construct two or more input classes differing only in a secret property, randomize execution order, control CPU noise, gather enough samples, and run dudect-style Welch tests plus checkmem/assembly analysis.

Investigate cache, branch predictor, variable-time arithmetic, error exits, allocator, and compiler/architecture effects. Repeat across optimizers and relevant CPUs. Treat a significant result as a lead requiring mechanism proof, and a non-significant result as non-proof. Fix only when secret dataflow and measurable behavior identify a concrete leak.
```

<a id="goal-54"></a>

### 54. RAII, smart-pointer, and resource-leak audit

<!-- slug: raii-resource-leaks; prompt-chars: 3711; utf8-bytes: 3712 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/raii-resource-leaks.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map ownership of heap objects, file/socket handles, locks, transactions, threads, callbacks, scheduler work, iterators, snapshots, and external resources. Search raw-pointer escapes, shared_ptr cycles, reference captures, moved-from misuse, custom deleters, destruction-order assumptions, and early-return leaks.

Use LSan/Valgrind and deterministic lifecycle tests including construction failure, cancellation, shutdown, and restart. Prove the exact ownership cycle or dangling window; do not modernize pointers mechanically. Prefer the smallest RAII/lifetime correction and verify destruction order, callbacks, and thread joins.
```

<a id="goal-55"></a>

### 55. Alternative-implementation compatibility-difference audit

<!-- slug: alternative-implementation-diff; prompt-chars: 3880; utf8-bytes: 3881 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/alternative-implementation-diff.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Mine Bitcoin Knots, btcd, libbitcoin, rust-bitcoin, bitcoinj, gocoin, libwally, OpenSSL/BoringSSL, noble/rust secp libraries, RocksDB/Pebble, and relevant forks for fixes, tests, and reference behavior. Define the local consensus, encoding, crypto, persistence, or recovery contract first.

Do more than compare outputs: transplant distinguishing sibling tests, port a small reference implementation across languages when useful, and rebase relevant alternative patches or test branches onto current HEAD without copying conclusions. Keep adapters thin and provenance exact. Classify every divergence as local bug, sibling bug, intentional policy/API difference, obsolete test, or adapter error. Require a shared vector and public reachability before fixing or preparing a remote report.
```

<a id="goal-56"></a>

### 56. Stale PR critical-fix resurrection audit

<!-- slug: stale-pr-resurrection; prompt-chars: 3720; utf8-bytes: 3721 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/stale-pr-resurrection.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Search closed, abandoned, draft, superseded, and stale open PRs/issues for claimed consensus, funds, remote DoS/crash, wallet/key, data-corruption, crypto, or recovery bugs. Record why each stalled, review objections, proposed tests, and later related work.

Reproduce the claim independently on current HEAD; never resurrect the old patch blindly. If still critical, design the smallest current-style fix and explain how it avoids prior objections. If fixed, noncritical, or under-proven, record decisive evidence. Continue through PR ranges with checkpoints and prioritize unmerged regression tests and credible reproducer discussions.
```

<a id="goal-57"></a>

### 57. Local-reasoning domain and relationship audit

<!-- slug: local-reasoning-contracts; prompt-chars: 3914; utf8-bytes: 3915 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/local-reasoning-contracts.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For one function/class/module per cycle, write its legal input domain, preconditions, postconditions, ownership graph, lock requirements, invalidation rules, failure recovery, and persistence authority. Ask whether a reviewer can verify them locally or must rely on caller folklore.

Create a temporary shadow build that makes hidden assumptions explicit with checks, checked arithmetic, sentinel outputs, and independent recomputation. Target owner/observer, iterator/container, snapshot/database, cache/backend, context/object, callback/session, active-chain/index, and wallet/key relationships. Walk violations through callers to a public boundary and distinguish unreachable internal misuse from real exposure. Commit only concrete invalid-state, lifetime, or recovery defects; keep useful instrumentation as an experiment.
```

<a id="goal-58"></a>

### 58. Exact helper reuse and minimal helper-extension audit

<!-- slug: helper-reuse; prompt-chars: 3746; utf8-bytes: 3747 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/helper-reuse.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
First find duplicate code exactly covered by an existing helper: setup, parsing/serialization, result conversion, cleanup, locking, formatting, fixtures, builders, and assertions. Prove equivalence across inputs, errors, side effects, ownership, locks, diagnostics, and ordering by inlining the helper mentally or mechanically.

Only after exact reuse, consider one minimal parameter/overload/hook that immediately replaces multiple real duplicates or one high-risk block. Reject abstractions that hide case-specific meaning, weaken tests, cross layers, or exist only to reduce lines. Benchmark compile/runtime effects where relevant and remove newly dead code atomically.
```

<a id="goal-59"></a>

### 59. C/C++ supply-chain and security-gate audit

<!-- slug: supply-chain-security-gates; prompt-chars: 3754; utf8-bytes: 3755 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/supply-chain-security-gates.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit vendored/subtree/submodule code, depends manifests, hashes, patches, download URLs, toolchain/container pins, generated sources, CI actions/images, release signing, binary verification, SBOM/provenance, scanner configuration, and workflow permissions.

Trace each trusted artifact or gate from untrusted input to build, test, release, or install decision. Check cache poisoning, untrusted-fork execution, shell/path/env injection, stale allowlists, ignored vendor/generated paths, and vulnerable dependency reachability. Add no security theater: each change must block a demonstrated bad artifact, leaked secret, unsafe workflow, or false verification result.
```

<a id="goal-60"></a>

### 60. Historical reviewer-preference mining and reusable review skill

<!-- slug: reviewer-preference-skill; prompt-chars: 3723; utf8-bytes: 3724 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/reviewer-preference-skill.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Mine diverse merged, closed, abandoned, contentious, and high-impact PRs in each subsystem. Extract actual review comments, maintainer decisions, ACK/NACK rationale, requested evidence, commit-stack preferences, and post-merge follow-ups.

Classify every rule as general, subsystem-specific, reviewer/author-specific, contextual, stale, or one-off taste. Encode durable items as trigger + review question + evidence links + non-goals/counterexamples. Validate against held-out historical PRs and update the journal's reviewer map. Continue until PR ranges and major reviewers are covered, revisiting rules when project practice changes.
```

<a id="goal-61"></a>

### 61. Stateful contract-fuzzer expansion

<!-- slug: stateful-contract-fuzzing; prompt-chars: 3890; utf8-bytes: 3891 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/stateful-contract-fuzzing.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Upgrade one fuzz target at a time from no-crash execution to a deterministic state-machine checker. Read public production entry points, tests, comments, callers, history, suppressions, and special seed values; define valid states, malformed inputs that must fail safely, and legal transitions.

Densify the oracle before adding cycles: pre/postconditions, independent recomputation after every step, inverse/replay, idempotence, ordering, accounting, output-on-failure, and differential checks. Extend operation sequences without unrealistic constraints and prefer a public functional reproducer for discoveries. Run sanitizer and high-throughput builds with qa-assets and external corpora, preserve minimized seeds and coverage gains, and separate production, harness, oracle, and nondeterminism bugs.
```

<a id="goal-62"></a>

### 62. Rejected-finding resurrection and assumption attack

<!-- slug: rejected-finding-resurrection; prompt-chars: 3761; utf8-bytes: 3762 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/rejected-finding-resurrection.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Collect candidates previously dismissed as unreachable, theoretical, test-only, root-only, mitigated, or irrelevant. Treat each dismissal as a falsifiable claim. List its assumptions about bounds, call order, state, configuration, platform, permissions, and trust boundary.

Attack one assumption at a time using widened fuzzing, crafted RPC/config/database/IPC/network fixtures, sanitizer runs, deterministic interleavings, corpus transfer, and historical states. Require a realistic boundary, not an artificial harness alone. Confirm rejection only after documented falsification attempts fail; otherwise fix the proven root cause and state severity without inflation.
```

<a id="goal-63"></a>

### 63. Loupe and Codex Security scout, verifier, fixer, and reporter pipeline

<!-- slug: loupe-style-pipeline; prompt-chars: 3510; utf8-bytes: 3511 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/loupe-style-pipeline.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Run a four-stage Project Loupe-style pipeline, but do not stop at per-file review. Scouts search prior findings, map a whole feature and public path, and produce a runnable regression test, fixture, harness, hostile artifact, or independent reference rather than a patch. A separate verifier must build and run it on clean HEAD, lock confirmed/dismissed/inconclusive, and record public reachability before seeing any fix.

Only confirmed findings reach a fixer; a final reviewer reruns failing-before/passing-after proof and project-precedent checks. Preserve unproven artifacts and negative results for later models, route candidates into specialized reimplementation, sink, conformance, or fault-injection campaigns, and run a composition pass over confirmed findings. Track leases, semantic/hash deduplication, status transitions, and exact PoCs in the journal.

Add repository/feature risk ranking before scouting, an explicit coverage manifest, root-cause continuity across reruns, and an attack-path pass after validation. Preserve a replayable scan recipe and treat incomplete coverage as unknown rather than clean.
```

<a id="goal-64"></a>

### 64. Finding deduplication, recurrence, and semantic-fingerprint audit

<!-- slug: finding-dedup-recurrence; prompt-chars: 3501; utf8-bytes: 3502 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/finding-dedup-recurrence.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Build a durable index of confirmed findings, rejected hypotheses, and unfinished suspicions using code path, trust boundary, bug shape, source-to-sink relation, affected versions, reproducer hash, semantic summary, artifact branch, tool/model version, and exact resume point. Index unfinished fuzzers, seeds, assertions, sanitizer traces, harnesses, and failed attempts, not only polished reports.

Before new work search hashes, symbols, commits, and semantic descriptions. Distinguish duplicate, recurrence, incomplete variant, changed reachability, or useful negative result. Periodically replay the highest-risk unresolved artifacts and old regression inputs with a different or newer model, initially hiding the old verdict where practical. Link rather than restate duplicates and never discard evidence merely because the first model judged it unimportant.

Treat each false-positive reason as a conditional guard or reachability claim. Revalidate it after code, scope, configuration, dependency, or threat-model changes, and track the same root cause as new, persisting, reopened, resolved, or unknown.
```

<a id="goal-65"></a>

### 65. Contributor-branch and work-in-progress radar

<!-- slug: contributor-branch-radar; prompt-chars: 3785; utf8-bytes: 3786 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/contributor-branch-radar.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Identify active contributors from recent PRs, reviews, commits, and subsystem ownership, then fetch their public GitHub branches/forks without altering upstream refs. Inventory branch purpose, base, divergence, touched contracts, tests, benchmark evidence, dependencies, and overlap with local work.

Read associated PRs/issues and record each contributor's style and unresolved review concerns. Look for upcoming migrations, fixes, optimizations, test ideas, and conflicting assumptions that should inform local plans. Never copy unpublished work without provenance. Flag branches containing independently reproducible bugs, useful seeds, or likely merge conflicts, and refresh the radar over time.
```

<a id="goal-66"></a>

### 66. Cherry-pick, backport, and release-branch correctness audit

<!-- slug: backport-correctness; prompt-chars: 3751; utf8-bytes: 3752 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/backport-correctness.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit maintenance/release branches and downstream backports for missing prerequisite/follow-up commits, conflict-resolution mistakes, reordered dependencies, stale generated files, wrong version guards, and tests that passed only on master. Compare patch-id, range-diff, blame, and semantic behavior rather than hash alone.

Prioritize consensus, serialization, wallet, database, network, and security fixes. Reproduce the original bug and expected fix on every supported target branch; compare binaries/tests and upgrade/downgrade paths. Identify incorrect cherry-picks in either local or downstream projects and provide minimal corrective commits with exact ancestry.
```

<a id="goal-67"></a>

### 67. Release-to-release behavioral and consensus differential

<!-- slug: release-version-differential; prompt-chars: 3718; utf8-bytes: 3719 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/release-version-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select adjacent and strategically distant released versions plus current HEAD. Feed identical blocks, transactions, scripts, RPC/config cases, wallets, databases, indexes, and network transcripts, normalizing intentionally changed output.

Use release notes, BIPs, and migration code to classify expected differences before judging them. Focus on undocumented consensus/validation drift, acceptance/rejection changes, state serialization, recovery, error semantics, and performance cliffs. Bisect unexpected divergence, test upgrade/downgrade/restart, and preserve portable fixtures. Continue across version pairs with a ledger.
```

<a id="goal-68"></a>

### 68. Architecture, endianness, word-size, and ABI parity audit

<!-- slug: architecture-abi-parity; prompt-chars: 3710; utf8-bytes: 3711 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/architecture-abi-parity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run or cross/emulate relevant x86_64, arm64, arm32, i686, big-endian, Windows, macOS, Linux, and FreeBSD configurations. Compare 32/64-bit widths, char signedness, alignment, packing, endian conversions, atomics, filesystem/socket APIs, time types, and serialized outputs.

Use QEMU or supported CI images where native hardware is unavailable and distinguish compile-only from executed evidence. Feed identical deterministic vectors and compare results, sanitizer traces, and performance where meaningful. Find architecture-specific UB, truncation, unaligned access, stale assembly selection, and platform-only skipped tests.
```

<a id="goal-69"></a>

### 69. SIMD, assembly, and portable-reference backend differential

<!-- slug: backend-differential; prompt-chars: 3711; utf8-bytes: 3712 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/backend-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory scalar, SIMD, assembly, hardware-accelerated, and reference implementations for hashes, crypto, field/scalar/group math, checksums, memory operations, and codecs. Force each backend independently and feed identical boundary, random, and malformed inputs.

Compare exact outputs, error behavior, aliasing support, state mutation, constant-time expectations, and architecture feature detection. Run sanitizers where possible and inspect fallback behavior on unsupported CPUs. Benchmark only after correctness. Minimize divergences and determine whether optimized code, reference code, dispatcher, or test oracle is wrong.
```

<a id="goal-70"></a>

### 70. Compiler, optimization, LTO, PGO, and BOLT differential

<!-- slug: compiler-optimization-differential; prompt-chars: 3745; utf8-bytes: 3746 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/compiler-optimization-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build with supported GCC/Clang versions at `-O0/-O1/-O2/-O3/-Os`, debug/release, LTO, PGO/SamplePGO, and BOLT where practical. Run identical deterministic correctness suites and representative workloads; compare outputs before treating speed as evidence.

Look for UB exposed only under optimization, miscompiles, missing barriers/cleanses, altered constant-time behavior, profile instability, and code-size/startup/cache tradeoffs. Use assembly/IR, perf counters, and compiler reducers to isolate anomalies. Adopt PGO/BOLT only with reproducible training workloads, held-out validation, measured gains, and no correctness or portability regression.
```

<a id="goal-71"></a>

### 71. Deterministic simulation and failure-schedule exploration

<!-- slug: deterministic-simulation; prompt-chars: 3867; utf8-bytes: 3868 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/deterministic-simulation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build or extend a deterministic scheduler/environment around one stateful subsystem: seeded wall and monotonic time, time jumps/drift, randomness, network/disk outcomes, task ordering, retries, shutdown, restart, and resource failures. Run production logic through interchangeable test interfaces where feasible.

Define invariants over intermediate and final state, progress, durability, and resource bounds. Generate aggressive reproducible schedules, including combinations rather than one fault at a time; record every choice, shrink failures, and replay seeds exactly. Recompute authoritative state after each step and seek a public scenario for failures. Avoid a fake parallel implementation that bypasses production logic. Each cycle adds a workload, fault class, or oracle.
```

<a id="goal-72"></a>

### 72. Filesystem, power-loss, and crash-consistency injection

<!-- slug: filesystem-crash-consistency; prompt-chars: 3836; utf8-bytes: 3837 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/filesystem-crash-consistency.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map durable boundaries for chainstate, wallets, indexes, block files, settings, logs, manifests, snapshots, and generated state. Use controlled kills, short writes, ENOSPC/EIO, dropped or corrupted writes, truncation, reorder assumptions, permission changes, and tools such as dm-flakey on scratch devices.

Enumerate crash points before and after write, flush, rename, fsync, metadata update, and in-memory commit. Include attacker- or user-inducible corrupt files and malformed persisted state, then trace whether failure remains local or reaches a public security effect. Restart and verify authoritative state, idempotent recovery, no false progress, and bounded repair. Preserve the exact fault schedule and fixture; never corrupt live data.
```

<a id="goal-73"></a>

### 73. Network fragmentation, reordering, and partial-I/O state-machine audit

<!-- slug: network-state-machine; prompt-chars: 3794; utf8-bytes: 3795 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/network-state-machine.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Exercise transports and protocol parsers with fragmented/coalesced reads, short writes, EOF at every byte, delayed or reordered messages where permitted, duplicates, reconnects, half-close, backpressure, address-family differences, and zero/maximum payloads.

Force the agent to build a deterministic hostile peer or client that drives the real public protocol, rather than only describing malformed traffic. Model handshake and peer states explicitly; check memory/accounting, timeout resets, permissions, framing, cleanup, and no work after disconnect. Compare legacy/new transports and OS behavior, combine with assertions and sanitizers, and preserve minimized reusable transcripts and the adversarial tool.
```

<a id="goal-74"></a>

### 74. Memory pressure, OOM, allocator, and fragmentation audit

<!-- slug: memory-pressure-allocator; prompt-chars: 3735; utf8-bytes: 3736 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/memory-pressure-allocator.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Profile steady-state and peak heap, allocation counts/sizes/lifetimes, fragmentation, caches, arenas, stack use, and retained capacity under sync, reindex, mempool, wallet, RPC, fuzz, and adversarial inputs. Vary allocator, memory limit, and thread count.

Inject allocation failure where contracts permit, plus realistic cgroup/RLIMIT pressure. Check overflow before allocation, graceful failure, cleanup, cache accounting, retry storms, and whether memory returns after workload. Distinguish allocator behavior from leaks. Optimize only measured hot allocation patterns or proven excessive retention, with RSS/heap profiles and correctness tests.
```

<a id="goal-75"></a>

### 75. Build throughput, dependency graph, and container-cache audit

<!-- slug: build-throughput-cacheability; prompt-chars: 3761; utf8-bytes: 3762 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/build-throughput-cacheability.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Measure clean, incremental, no-op, and parallel builds with Ninja/CMake timing, compiler traces, header/include cost, generated steps, linker time, Docker layer reuse, and CI cache hit/miss behavior. Find unnecessary rebuild fan-out, unstable generated files, broad headers, serialized custom commands, poor job pools, and cache keys tied to irrelevant inputs.

Make the smallest dependency/build-script/container change. Prove no missing dependency with clean and randomized parallel builds, and no stale result after touching each true input. Report wall/CPU/RSS/cache-size effects over repeated runs. Do not trade correctness or developer clarity for tiny build wins.
```

<a id="goal-76"></a>

### 76. Reproducible binaries, Guix, and toolchain-provenance audit

<!-- slug: reproducible-builds; prompt-chars: 3715; utf8-bytes: 3716 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/reproducible-builds.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Rebuild release artifacts from clean environments, across supported hosts/architectures where documented, using pinned dependencies and Bitcoin Core Guix/depends or the project's equivalent. Compare hashes and use diffoscope-style analysis for differences.

Trace timestamps, paths, locale, ordering, archive metadata, toolchain drift, generated files, signing, and host contamination. Verify dependency hashes and source provenance, and distinguish reproducible unsigned payloads from signatures/packaging. Fix the narrow source of nondeterminism and rerun independently. Record exact toolchain/container commits and artifact hashes.
```

<a id="goal-77"></a>

### 77. Symbolic execution and bounded-model-checking campaign

<!-- slug: symbolic-model-checking; prompt-chars: 3711; utf8-bytes: 3712 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/symbolic-model-checking.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select small, high-risk pure or state-machine kernels with bounded inputs: arithmetic, parsers, encoders, cache transitions, queues, crypto helpers, and failure cleanup. Build focused CBMC or KLEE harnesses with explicit assumptions matching production domains.

Assert memory safety, no division/shift UB, postconditions, output-on-failure, algebraic identities, and equivalence to a reference. Treat bounds and environment stubs as part of the proof and attack them with concrete tests. Convert counterexamples into regression vectors. Never claim an unbounded proof; document unwind completeness and unsupported constructs.
```

<a id="goal-78"></a>

### 78. Compiler-transformation validation and miscompile isolation

<!-- slug: translation-validation; prompt-chars: 3763; utf8-bytes: 3764 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/translation-validation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For suspicious optimization-dependent behavior or critical arithmetic, capture pre/post LLVM IR and use Alive2 where supported to check refinement. Also compare GCC/Clang versions and optimization levels on deterministic vectors, especially code with overflow assumptions, aliasing, shifts, bit tricks, and constant-time masking.

If validation is inconclusive, reduce the function and use differential execution or generated UB-free cases. Determine whether source UB, compiler bug, inline assembly contract, or test error is responsible. Fix local UB rather than coding around a compiler unless project support requires it; produce a compiler-report reproducer for remote bugs.
```

<a id="goal-79"></a>

### 79. Fuzz-corpus stewardship, minimization, and transfer audit

<!-- slug: fuzz-corpus-stewardship; prompt-chars: 3710; utf8-bytes: 3711 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/fuzz-corpus-stewardship.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory corpora by target, source commit, coverage, size, runtime, flakiness, and sanitizer dependence. Merge and minimize with exact target/build versions; remove true duplicates without losing features, and identify oversized seeds that dominate execution.

Cross-seed structurally related targets and import public qa-assets/bitcoinfuzz corpora with provenance. Re-run old crashers and regression inputs on current HEAD. Preserve inputs that add stable coverage or encode important semantics, not random bulk. Track corpus coverage/time trends and submit project-appropriate improvements with deterministic reproduction.
```

<a id="goal-80"></a>

### 80. Fuzz-engine and property-framework differential

<!-- slug: fuzz-engine-differential; prompt-chars: 3721; utf8-bytes: 3722 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/fuzz-engine-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run selected targets under libFuzzer, AFL++, Honggfuzz, and a property framework such as FuzzTest where integration is practical. Keep target semantics and initial corpus comparable while using each engine's strengths: dictionaries, value profiles, CMP tracing, custom mutators, parallelism, and high-throughput modes.

Compare coverage growth, unique paths, crash classes, execution rate, memory, and corpus quality over fixed CPU budgets and repeated seeds. Transfer discoveries between engines and reproduce all failures in a sanitizer build. Change harnesses only for engine-neutral realism unless a documented adapter is required.
```

<a id="goal-81"></a>

### 81. Specification, test-vector, and formal-model drift audit

<!-- slug: spec-vector-drift; prompt-chars: 3890; utf8-bytes: 3891 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/spec-vector-drift.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map code and tests to BIPs, protocol documents, secp256k1 module docs, Sage scripts, Wycheproof vectors, official conformance suites, and other authoritative specifications. Pin versions and mark ambiguous, policy-specific, or intentionally divergent areas. Preserve primary sources resolved through any knowledge index.

Turn outside knowledge into executable oracles rather than prose context: derive tests, regenerate vectors, or implement a small independent reference. Exercise valid, invalid, edge, and historical cases; search for rules implemented but untested and tests copied from obsolete drafts. When code, tests, and implementations disagree, derive the result from the exact rule and minimize a public vector. Update vectors/tests or code only with traceable provenance and compatibility analysis.
```

<a id="goal-82"></a>

### 82. secp256k1 field and scalar representation matrix

<!-- slug: secp-field-scalar-matrix; prompt-chars: 3736; utf8-bytes: 3737 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secp-field-scalar-matrix.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
In libsecp256k1, compare 5x52 versus 10x26 field code, 4x64 versus 8x32 scalar code, normal versus VERIFY, exhaustive groups, inversion variants, and supported compiler/architecture paths. Extract magnitude, normalization, limb, carry, overflow, aliasing, and input-domain contracts from headers and verification code.

Generate boundary elements at every allowed magnitude and scalar edge; run add/mul/sqr/negate/inverse/normalize/serialize relations and cross-backend exact comparisons. Inspect 32-bit arithmetic carefully. Use exhaustive/random/property tests, UBSan, optimized assembly, and temporary bound violations to prove oracle sensitivity.
```

<a id="goal-83"></a>

### 83. secp256k1 group, ecmult, and formula-parity audit

<!-- slug: secp-group-ecmult; prompt-chars: 3717; utf8-bytes: 3718 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secp-group-ecmult.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit affine/Jacobian conversions, addition/doubling, infinity handling, wNAF/window tables, generator and arbitrary-point multiplication, endomorphism paths, batch inversion, and exhaustive-group formulas. State input domains and exceptional cases for each helper.

Compare optimized formulas with a simple reference and exhaustive small-group results across window sizes, backends, and VERIFY builds. Test aliasing and malformed internal states only where contracts define them. Search Sage derivations and historical formula fixes for variants. Benchmark changes only after algebraic parity and constant-time classification are proven.
```

<a id="goal-84"></a>

### 84. secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit

<!-- slug: secp-nonce-session; prompt-chars: 3720; utf8-bytes: 3721 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secp-nonce-session.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model ECDSA/Schnorr signing, nonce generation, key tweaks, ECDH, extrakeys, and MuSig nonce/session/partial-signature transitions. Label secret/public inputs, single-use state, commitment binding, context capability, callback failure, and output-on-failure guarantees.

Test invalid order, reuse, duplicate participant/key, zero/overflow scalar, malformed serialization, cancellation, randomized context, and deterministic replay. Cross-check formal equations and other implementations without treating them as oracle. Run exhaustive/module/ctime/checkmem tests, preserve minimal sequences, and prioritize nonce reuse or partial-state bugs.
```

<a id="goal-85"></a>

### 85. Bitcoin consensus mutation-score and kill-test audit

<!-- slug: bitcoin-consensus-mutation; prompt-chars: 3731; utf8-bytes: 3732 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-consensus-mutation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Target consensus and script-validation code plus serialization feeding it. Apply focused temporary mutants: invert checks, shift activation boundaries, skip flags, alter sighash/script limits, accept non-canonical forms, remove cache-key inputs, or perturb amount/sequence/time arithmetic.

Run unit, functional, fuzz, and available consensus vectors. Any surviving non-equivalent mutant is a critical oracle gap: add the smallest test that distinguishes behavior without duplicating implementation. Prove the mutant is reachable and the expected outcome from consensus rules/history. Never commit mutants or casually alter consensus behavior.
```

<a id="goal-86"></a>

### 86. Bitcoin chainstate, reorg, prune, and index crash-symmetry audit

<!-- slug: bitcoin-chainstate-symmetry; prompt-chars: 3724; utf8-bytes: 3725 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-chainstate-symmetry.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model block connect/disconnect, flush, reorg, restart, prune, snapshot/background chainstate, block/undo files, and every BaseIndex-derived index. Track active tip, flushed chainstate, locators, best blocks, file positions, cache flags, and durable commit points.

Generate same-height and unequal-height reorgs, pure disconnects, interrupted writes/flushes, pruning races, stale children, index lag/ahead states, and restart at each transition. Assert connect/disconnect and replay symmetry, no locator advancement past durability, recoverable indexes, and matching queried results. Use scratch datadirs and deterministic fault hooks.
```

<a id="goal-87"></a>

### 87. Bitcoin mempool, package, and eviction-accounting audit

<!-- slug: bitcoin-mempool-accounting; prompt-chars: 3730; utf8-bytes: 3731 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-mempool-accounting.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build a state-machine model for transaction/package acceptance, replacement, ancestor/descendant tracking, fee deltas, clusters, expiry, trimming, conflicts, and removal for block/reorg. Compare incremental indexes/counters with full recomputation after every operation.

Fuzz operation sequences around limits, overlapping packages, rejected transactions, RBF, orphan-like states, reorg reinsertion, and memory pressure. Assert graph symmetry, no stale links, exact resource accounting, deterministic ordering where promised, and unchanged state on failure. Seed from historical mempool DoS/accounting fixes and alternative-node divergences.
```

<a id="goal-88"></a>

### 88. Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit

<!-- slug: bitcoin-wallet-recovery; prompt-chars: 3739; utf8-bytes: 3740 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-wallet-recovery.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map wallet transactions and durable boundaries for creation, encryption/passphrase change, master keys, descriptor/key encryption, keypool/top-up, address reservation, migration, backup/restore, rescans, and external signers. Identify which memory and database state is authoritative.

Inject database write/erase/commit failures and crashes at every step, then restart and verify no mixed plaintext/encrypted state, missing master key, unusable descriptor, silent key loss, duplicate address reservation, or memory-only success. Test legacy/descriptor and SQLite/BDB-supported paths as applicable, with scratch wallets and deterministic KDF/test clocks.
```

<a id="goal-89"></a>

### 89. Bitcoin P2P transport, permission, and peer-accounting audit

<!-- slug: bitcoin-p2p-accounting; prompt-chars: 3758; utf8-bytes: 3759 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-p2p-accounting.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model connection lifecycle across inbound/outbound/manual/feeler/block-relay, v1/v2 transports, permissions, handshake, message processing, discouragement/ban, quotas, download state, and disconnect. Track bytes, queues, in-flight blocks, timeouts, service flags, and peer-manager/net state ownership.

Generate fragmented messages, invalid order, duplicate handshakes, permission changes, partial sends, stalls, reconnects, address-family variants, and shutdown races. Assert bounded queues/work, consistent accounting, no stale permissions/state after disconnect, and no assertion from untrusted input. Compare versions and alternative nodes only on shared protocol rules.
```

<a id="goal-90"></a>

### 90. Whole-PR, commit, and external knowledge-base recipe synthesis

<!-- slug: historical-knowledge-recipes; prompt-chars: 3479; utf8-bytes: 3480 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/historical-knowledge-recipes.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Progress through every commit and PR in recorded ranges and extract reusable technical knowledge, not reviewer taste alone. Every reviewed commit must leave proof of work: intended behavior, changed invariant, callers and files inspected, missing test/oracle, sibling sites, rejected design, benchmark or fixture, platform caveat, follow-up, and verdict. A bare summary or "looks fine" is incomplete.

Store concise recipes keyed by subsystem and trigger: when a future change touches X, inspect Y, run Z, and avoid W. Link primary evidence, mark stale/version-limited rules, and attach runnable artifacts where possible. Validate recipes on held-out PRs by checking whether they recover real historical comments or bugs. Record exact commit/range checkpoints, deduplicate, and revise rather than restarting or appending folklore.

Also ingest versioned architecture, policy, threat-model, and specification documents. Convert useful claims into executable tests, reference models, or review triggers, and preserve ambiguity as competing hypotheses rather than silently choosing one.
```

<a id="goal-91"></a>

### 91. Compiler and binary-hardening configuration audit

<!-- slug: compiler-binary-hardening; prompt-chars: 3773; utf8-bytes: 3774 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/compiler-binary-hardening.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit supported release and developer builds for warnings-as-errors policy, stack protection, FORTIFY, PIE/RELRO/NOW, CFI, SafeStack or platform equivalents, control-flow protections, `_GLIBCXX_ASSERTIONS`, hardened libc modes, integer/implicit-conversion sanitizers, and linker diagnostics.

For each missing or disabled mechanism, determine threat model, platform support, performance/size impact, dependency compatibility, and whether it catches a concrete project-relevant mutation or fixture. Inspect final binaries, not just flags. Add no checkbox hardening: require a demonstrated failure blocked or diagnostic gained, plus build/test/benchmark evidence across supported targets.
```

<a id="goal-92"></a>

### 92. ABI layout, alignment, aliasing, and object-lifetime audit

<!-- slug: abi-alignment-aliasing; prompt-chars: 3689; utf8-bytes: 3690 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/abi-alignment-aliasing.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Search packed structs, unions, reinterpret/static casts, placement new, memcpy of nontrivial objects, over-aligned types, custom allocators, spans over raw storage, strict-aliasing assumptions, pointer provenance, lifetime extension, and C/C++ ABI boundaries.

Compare sizes/offsets/alignment under compilers, architectures, optimization, sanitizers, and shared/static builds. Exercise unaligned buffers and aliasing permutations without invoking invalid inputs outside the contract. Use TypeSanitizer/UBSan, assembly, and small layout tests. Fix concrete UB or ABI mismatch, avoiding broad wrapper churn.
```

<a id="goal-93"></a>

### 93. Allocation, syscall, clock, randomness, and callback fault injection

<!-- slug: system-fault-injection; prompt-chars: 3898; utf8-bytes: 3899 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/system-fault-injection.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Create narrow deterministic hooks or wrappers to fail allocations, opens, reads/writes, fsync/rename, socket operations, thread creation, entropy, wall/monotonic clocks, scheduler callbacks, database writes, and user callbacks at the Nth operation. Include backward/forward time jumps, drift, repeated values, and short or partial success where APIs permit.

Use production and preferably public call paths with scratch resources. Sweep failure points and assert rollback, cleanup, retry bounds, diagnostics, intermediate state, and restart behavior. Minimize schedules, combine related failures, and determine whether the condition is cosmetic, locally recoverable, data-corrupting, or externally exploitable. Keep hooks test-only or aligned with existing infrastructure and turn exposed paths into durable tests.
```

<a id="goal-94"></a>

### 94. Bindings, FFI, and language-wrapper parity audit

<!-- slug: bindings-ffi-parity; prompt-chars: 3711; utf8-bytes: 3712 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bindings-ffi-parity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare C/C++ public APIs with maintained Rust, Python, Java, Go, C#, JNI, or other bindings used in the ecosystem. Audit widths, signedness, ownership, lifetimes, nullability, callbacks, exceptions/status mapping, thread safety, buffer lengths, secret cleanup, feature flags, and output-on-failure.

Build shared vectors and misuse cases, including 32-bit and malformed input. Determine whether divergence is in core, wrapper, generated bindings, or documentation. Do not change core to accommodate a broken wrapper unless the core contract is genuinely unsafe. Produce remote report-ready reproductions for binding-only defects.
```

<a id="goal-95"></a>

### 95. Database-engine and persistence-semantics differential

<!-- slug: database-semantics-differential; prompt-chars: 3780; utf8-bytes: 3781 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/database-semantics-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare the project's LevelDB usage and assumptions with upstream LevelDB history plus RocksDB, Pebble, or alternative backends as bug seeds. Focus on comparator ordering/stability, snapshots, iterators, batches, WAL/MANIFEST recovery, checksums, filters, compaction boundaries, deletes/overwrites, sync semantics, and corruption handling.

Build engine-neutral operation traces and crash/corruption fixtures, then state allowed implementation differences. Verify Bitcoin wrappers do not rely on undocumented backend behavior. If divergence proves a local wrapper/assumption bug, fix it; if another engine is wrong, document it separately. Measure performance only after semantic parity.
```

<a id="goal-96"></a>

### 96. TODO, FIXME, stub, and deferred-work challenge audit

<!-- slug: todo-deferred-work; prompt-chars: 3868; utf8-bytes: 3869 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/todo-deferred-work.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Enumerate TODO/FIXME/XXX, disabled tests, expected failures, unimplemented branches, temporary compatibility code, placeholder returns, sanitizer suppressions, skipped targets, magic fuzzer values, exceptional-case catches, and comments promising future cleanup. Link each to its origin and recover why the project treated it as unusual.

Turn every item into a falsifiable current question: hidden bug, missing coverage, obsolete workaround, blocked design, valid precondition, or safe intentional debt. Explore neighboring values and sibling sites, search whether later work solved it elsewhere, and preserve partial experiments even without a finding. Fix only concrete current defects or demonstrably stale exceptions; otherwise record exact blockers and the next runnable experiment.
```

<a id="goal-97"></a>

### 97. C and C++ defect-taxonomy sweep

<!-- slug: cpp-defect-taxonomy; prompt-chars: 3804; utf8-bytes: 3805 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cpp-defect-taxonomy.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Cycle systematically through well-known defect classes so attractive areas do not crowd out basic bugs: null dereference, division/modulo by zero, use-before-init, use-after-free/move, double free, invalid destruction order, dangling view/reference/iterator, out-of-bounds, signed/unsigned wrap, shift UB, strict aliasing, data race, deadlock, missed virtual destruction, exception/error leaks, recursion/stack exhaustion, format mismatch, and unchecked result.

For each class, combine semantic search, compiler/tool diagnostics, historical examples, and boundary tests. Trace real reachability and reject pattern-only matches. Maintain a class-by-subsystem coverage grid and continue with the highest-risk unchecked cell.
```

<a id="goal-98"></a>

### 98. Floating-point edge values, sanitizer resurrection, and fuzz-exclusion audit

<!-- slug: float-sanitizer-fuzz-exclusions; prompt-chars: 3926; utf8-bytes: 3927 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/float-sanitizer-fuzz-exclusions.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Cycle through three linked passes.

1. Floating-point edge values. Inventory float/double inputs and conversions in production APIs, RPC/JSON/config parsing, GUI, bindings, tests, benches, and tools. Call them with `+0`, `-0`, subnormal bounds, values adjacent to checked boundaries, max finite, infinities, quiet/signaling NaNs, varied NaN payloads, and decimal overflow/underflow. Check comparisons, integer casts, ordering/hashing, serialization, formatting, clamping, division, and containers across relevant compilers, rounding modes, and optimizations. Prove whether each value must be rejected, normalized, propagated, or ignored; never introduce floating point into consensus or secret crypto.

2. Sanitizer resurrection. Find `no_sanitize`, suppressions, excluded targets, compiler/platform skips, disabled CI jobs, recover modes, and omitted categories. Recover the original reason, re-enable one diagnostic at a time, and minimize the first true positive. Remove a suppression only after its cause is fixed or proven absent; never broaden an allowlist.

3. Fuzzer exclusions. Inspect catches, ignored errors, early returns, clamps, `Assume` gates, magic values, and exceptional-value cases. Temporarily remove or invert one at a time and explore neighboring inputs. Preserve and minimize failures, then reproduce through the real public boundary. Classify valid precondition, harness bug, expected exception, or production defect, and assert safe output/state on legitimate failure. Never commit blind guard removal.
```

<a id="goal-99"></a>

### 99. Clean-room reimplementation and executable differential audit

<!-- slug: clean-room-reimplementation; prompt-chars: 3270; utf8-bytes: 3271 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/clean-room-reimplementation.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Choose one security-relevant feature with observable behavior and a bounded domain. One agent first writes a precise behavioral specification from public docs, tests, history, and callers. A separate agent, isolated from implementation details and prior verdicts where practical, builds a small independent implementation or reference model, possibly in another language.

Compile and run both against identical valid, invalid, boundary, and stateful vectors. Compare outputs, errors, side effects, state transitions, and resource use, using a third oracle or specification when neither side is authoritative. The required result is runnable code and a differential harness, not a prose review. Minimize every divergence, trace it to a public caller, classify which contract or implementation is wrong, and preserve the reference and vectors even when no defect is confirmed.
```

<a id="goal-100"></a>

### 100. Dangerous-sink reverse reachability and public attack synthesis

<!-- slug: sink-reverse-reachability; prompt-chars: 3296; utf8-bytes: 3297 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/sink-reverse-reachability.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Start from dangerous operations rather than public inputs: division/modulo, dereference, indexing, shifts, narrowing, allocation sizes, assertions, throws, parser assumptions, durable writes, and irreversible state changes. For each sink, state the exact values or state that make it fail or become unsafe and add a temporary assertion or checked wrapper.

Walk the caller graph upward. At each layer translate the sink condition into requirements on that caller until reaching network, RPC, config, file, wallet, library, or other public input, or a proven guard. Synthesize the smallest input and operation sequence satisfying every step, and record barriers where synthesis fails. Confirm through the real public path under assertions/sanitizers. Classify unreachable latent defects separately from exploitable or user-triggerable behavior; fix only after the complete source-to-sink chain is proven.
```

<a id="goal-101"></a>

### 101. Public-boundary characterization and minimal-fix sequencing

<!-- slug: public-characterization-fix; prompt-chars: 3298; utf8-bytes: 3299 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/public-characterization-fix.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
For a suspicious internal behavior, first determine whether real callers use it. Prefer multiple reproductions, including a functional or integration test through the public API, protocol, CLI/RPC, file format, or restart path. If no public route exists, prove the helper is unreachable or document the missing link rather than claiming impact.

Use a two-step stack where practical. The first commit changes no production code and adds a passing characterization test that records the current behavior, its public trigger, and why it is undesirable or risky; also preserve a command or alternate expected result showing the desired behavior fails on clean HEAD. The second commit makes the smallest production change and updates only the affected expectation. Build and test every commit independently, keep unrelated cleanup out, and retain lower-level tests only when they prove a distinct mechanism.
```

<a id="goal-102"></a>

### 102. Durable suspicion artifacts and cross-model replay

<!-- slug: durable-suspicion-replay; prompt-chars: 3265; utf8-bytes: 3266 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/durable-suspicion-replay.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Treat unfinished work as a first-class output. Preserve WIP branches, harnesses, fuzzers, corpora, minimized seeds, temporary assertions, sanitizer traces, coverage maps, failed hypotheses, odd observations, tool/model versions, exact commands, and the next experiment even when the current model thinks the lead is weak.

Maintain a machine-readable suspicion index linking every artifact to code revision, trust boundary, status, confidence, blockers, and resume point. Periodically give the highest-risk unresolved artifacts to a different or newer model, initially withholding the old verdict where practical, and require it to rerun rather than merely reread the conclusion. Compare outcomes, promote confirmed defects, retain useful negative results, and remove only artifacts proven redundant. Never let a model's refusal or low confidence erase executable evidence.
```

<a id="goal-103"></a>

### 103. Finding composition and end-to-end exploit-chain synthesis

<!-- slug: finding-composition; prompt-chars: 3266; utf8-bytes: 3267 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/finding-composition.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Convert every confirmed and strong inconclusive finding into a node describing preconditions, attacker control, privileges, persistence, outputs, side effects, and capabilities gained. Search for chains where one finding's effect satisfies another's precondition, including validation bypass, stale state, resource exhaustion, information disclosure, corruption, retry behavior, and recovery weaknesses.

Rank paths ending in consensus divergence, key/funds/privacy loss, durable corruption, privilege gain, or remote denial of service. Build one end-to-end public reproducer that demonstrates the chain and compare it with isolated controls for each component. Do not inflate severity from a hypothetical graph: record the exact broken edge when a chain cannot be realized. Preserve useful partial chains and identify the smallest independent fixes or mitigations that cut them.
```

<a id="goal-104"></a>

### 104. Analogical vulnerability translation and target-domain search

<!-- slug: analogical-vulnerability-translation; prompt-chars: 3923; utf8-bytes: 3924 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/analogical-vulnerability-translation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Take a proven vulnerability from this or another project and remove language- or product-specific details. Express the underlying failure as trust-boundary crossing, interpreter confusion, stale authority, unchecked size, partial commit, weak canonicalization, lifetime error, resource asymmetry, or another reusable shape.

Force the agent to generate several concrete analogies in the target domain across network, script, RPC/config, files/databases, wallet, crypto, build, and bindings. Rank them by public reachability and impact, then implement a harness or input for the best candidate. The analogy itself is never a finding: trace actual code, existing guards, and affected callers, minimize a runnable reproducer, and record why each rejected mapping fails. Add successful abstractions to the shared bug-shape index.
```

<a id="goal-105"></a>

### 105. Project vulnerability autopsy and author-feature recurrence mining

<!-- slug: project-bug-autopsy-recurrence; prompt-chars: 3262; utf8-bytes: 3263 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/project-bug-autopsy-recurrence.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
For each known bug or vulnerability in the project, reconstruct the introducing commit, intended change, failed assumption, missing validation or oracle, why tests/review missed it, how long it survived, and what finally exposed it. Produce an autopsy artifact before searching for variants.

Use the result to build project-specific priors. Inspect related commits and series by the introducing author, semantic siblings, and the surrounding feature or subsystem, because the same habits or weak assumptions may recur. Blame and feature ownership are deliberate ranking signals, not findings or reasons to soften the search. Each candidate still requires present-day reachability, a matched defect shape, and independent reproduction. Track which authors, features, and missing-oracle classes actually yield confirmed siblings so future ranking is evidence based.
```

<a id="goal-106"></a>

### 106. Semantic-twin inconsistency and sloppiness-map audit

<!-- slug: semantic-twin-inconsistency; prompt-chars: 3916; utf8-bytes: 3917 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/semantic-twin-inconsistency.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Cluster code that expresses the same concept even when names, containers, types, files, or layers differ. Build a contract matrix for empty/null values, duplicates, ordering, normalization, bounds, ownership, error behavior, output mutation, locking, persistence, and restart semantics.

Surface places where one twin validates, clamps, clears, rejects, or rolls back while another does not. Use history and public contracts to decide whether the difference is intentional, a migration artifact, or evidence that a safety rule was copied only partially. Create shared vectors or parallel public tests that exercise both paths. Treat inconsistency as a high-value lead rather than automatic proof, then trace real callers and fix one demonstrated contract mismatch at a time. Maintain a sloppiness map to prioritize nearby code.
```

<a id="goal-107"></a>

### 107. External conformance-suite and sibling-test transplantation

<!-- slug: conformance-test-transplant; prompt-chars: 3272; utf8-bytes: 3273 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/conformance-test-transplant.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Identify protocols, formats, cryptographic operations, databases, or APIs with an official conformance suite, reference vectors, or well-tested sibling implementation. Pin the exact source/version/license and transplant the tests through the target's public interface with the thinnest possible adapter. Also port distinguishing tests or relevant test-only branches from forks onto current HEAD.

Do not rewrite expected behavior to make tests pass. Classify every mismatch as target defect, sibling defect, intentional policy/API difference, obsolete specification, environment difference, or adapter bug. When useful, translate a small reference implementation from another language and compare it directly. Preserve provenance, original vectors, adapter limitations, and exact commands. A local fix requires a minimized shared test and contract proof, not majority behavior.
```

<a id="goal-108"></a>

### 108. Adversarial peer, client, file, and environment artifact generation

<!-- slug: adversarial-artifact-generation; prompt-chars: 3274; utf8-bytes: 3275 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/adversarial-artifact-generation.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Force the agent to build the adversary instead of describing one: a hostile protocol peer, malicious API client, malformed-but-plausible message stream, corrupt file/database generator, partial-I/O shim, clock source, callback, or restart driver. The artifact must be deterministic, reusable, and exercise the real production boundary.

Generate valid setup followed by invalid ordering, collisions, truncation, duplication, resource pressure, stale data, corruption, or environmental failure. Combine the artifact with assertions, sanitizers, coverage, and exact state snapshots. Verify cleanup, bounds, recovery, and externally visible impact, then shrink the scenario while keeping it public. Reject fake models that bypass production parsing or state transitions. Preserve useful tools and transcripts even when they expose only missing coverage or a blocked attack path.
```

<a id="goal-109"></a>

### 109. Whole-feature cross-file public-path security audit

<!-- slug: whole-feature-public-path; prompt-chars: 3936; utf8-bytes: 3937 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/whole-feature-public-path.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select one externally meaningful feature, not one file. Map its complete path from public input through parsing, validation, conversion, caches, queues, threads, persistence, restart/recovery, and output. Include alternate entry points, optional modes, duplicated implementations, cleanup, and authority changes.

For a bounded cycle go two or three surfaces deep, list untouched edges, and produce a feature map plus at least one runnable end-to-end scenario. Compare assumptions at every boundary: which layer validates, owns, limits, persists, rolls back, and reports state. Search for gaps that no single-file review sees, especially valid components whose interaction is wrong. Use assertions, coverage, history, and sibling behavior to focus experiments, and prove any candidate through the public path before proposing the smallest local fix.
```

<a id="goal-110"></a>

### 110. Self-fueling catalog evolution and entropy-quality audit

<!-- slug: catalog-entropy-evolution; prompt-chars: 3889; utf8-bytes: 3890 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/catalog-entropy-evolution.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit the campaign catalog itself as a live security instrument. For each completed cycle, inspect what was learned, which prior assumptions were narrowed, which goals gained concrete triggers or oracles, and whether suspicious code created a genuinely different search direction.

Build a lineage graph from cycle and finding to amended or new goals. Reject generic rewrites, duplicates, and goals with no runnable first experiment. Merge overlapping goals, split goals whose evidence now reveals distinct mechanisms, and retire stale goals without erasing history. Replay a sample of amendments to prove they change target selection or experiments. Measure catalog growth, dedup rate, project coverage, and confirmed-finding yield so self-fueling produces useful entropy rather than prompt inflation.
```

<a id="goal-111"></a>

### 111. Coverage manifest, deferred-work, and incomplete-scan closure audit

<!-- slug: coverage-manifest-closure; prompt-chars: 3848; utf8-bytes: 3849 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/coverage-manifest-closure.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Create an explicit coverage manifest for the selected repository and target: files, functions, features, trust boundaries, public entry points, configurations, generated/vendor code, and ranking, review, validation, and attack-path phases. Record what was reviewed, partially reviewed, excluded, deferred, or unreachable and why.

Treat incomplete coverage as a first-class result, never as clean. When a prior finding disappears, determine whether its root cause was fixed, its location moved, or the later scan simply missed its scope. Rank high-risk unreviewed and deferred cells, construct the smallest experiment that closes one cell, and update both coverage and campaign queues. Use coverage deltas to select work, not to claim security from percentages.
```

<a id="goal-112"></a>

### 112. Replayable scan recipes, finding continuity, and false-positive revalidation

<!-- slug: scan-recipe-finding-continuity; prompt-chars: 3353; utf8-bytes: 3354 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/scan-recipe-finding-continuity.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Store an immutable, replayable recipe for each scan: repository and full revision, target scope/mode, knowledge inputs, tool/model/plugin versions, reasoning/worker settings, seeds, environment, cost limits, output schema, and artifact hashes. Re-run recipes against current code and exact old revisions to distinguish scanner drift from code drift.

Match findings by root cause rather than wording or line number and classify new, persisting, reopened, resolved, or unknown. Treat every false-positive reason as a conditional claim about guards, reachability, configuration, or trust boundaries; revalidate it whenever code, scope, dependencies, or threat model changes. Preserve sealed outputs and matching decisions so later agents can audit continuity rather than rediscover history.

Before an expensive run, preflight the target, output location, credential source, toolchain, plugin version, and conflicting overrides without starting the scanner.
```

<a id="goal-113"></a>

### 113. Risk ranking, deep-scan stopping, and marginal-yield audit

<!-- slug: risk-ranking-deep-scan-yield; prompt-chars: 3294; utf8-bytes: 3295 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/risk-ranking-deep-scan-yield.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 characters. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Rank features and surfaces before scanning using public reachability, privilege, secrets, persistence, parser complexity, statefulness, historical defects, code churn, weak coverage, and unresolved suspicions. Allocate workers and budgets to the highest-value cells while retaining randomized exploration to avoid ranking lock-in.

Run repeated discovery under explicit total-run, no-new, worker, token, time, and cost bounds. Record unique root causes per run, duplicate rate, validation rate, coverage gained, and marginal cost. A no-new streak may stop that recipe, not declare the repository clean. Change method, model, knowledge, or target when yield stalls, and feed the measured strengths and blind spots back into future goal selection.

When a budget or cancellation stops a run, preserve and index partial findings, coverage, and artifacts rather than discarding the incomplete evidence.
```

<a id="goal-114"></a>

### 114. Threat-model and knowledge-base conversion into executable oracles

<!-- slug: knowledge-base-executable-oracles; prompt-chars: 3823; utf8-bytes: 3824 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/knowledge-base-executable-oracles.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Collect architecture documents, security policies, protocol specifications, threat models, incident reports, release notes, and maintainer notes with exact versions and provenance. Extract concrete assets, actors, trust boundaries, forbidden states, assumptions, and required behaviors, then map each claim to code, callers, and tests.

Do not leave outside knowledge as prompt context. Convert high-value claims into executable tests, assertions, reference models, attack scenarios, or coverage targets. Detect stale or contradictory knowledge by comparing it with current behavior and history. When a document is ambiguous, preserve competing interpretations and build a distinguishing vector instead of silently choosing one.
```

<a id="goal-115"></a>

### 115. Committed-diff, working-tree, and pre-commit security regression audit

<!-- slug: diff-working-tree-security; prompt-chars: 3796; utf8-bytes: 3797 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/diff-working-tree-security.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit a committed diff, branch range, or staged/unstaged working tree as a security change, not only as changed lines. Record the exact baseline and scope, then trace modified contracts through callers, tests, generated artifacts, configuration, persistence, and sibling implementations.

Compare root causes and coverage against the baseline: newly introduced, persisting, resolved, reopened, or unknown. Look for removed checks, weakened errors, stale tests, partial migrations, changed authority, and security fixes whose test is absent. Build the smallest pre-commit or CI reproducer for confirmed regressions. Never call a diff clean when surrounding impact or generated/dependent code was not reviewed.
```

<a id="goal-116"></a>

### 116. Cross-scanner differential and disagreement audit

<!-- slug: cross-scanner-differential; prompt-chars: 3792; utf8-bytes: 3793 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cross-scanner-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run the campaign goals, Project Loupe, Codex Security, and relevant static, sanitizer, or fuzz tools against the same pinned revision and equivalent scope. Preserve each tool's recipe, coverage, phases, raw findings, exclusions, cost, and model/tool versions, then normalize candidates by root cause.

Investigate every meaningful disagreement: one scanner found a bug another missed, one validated what another dismissed, or their coverage claims differ. Do not use majority vote as an oracle. Build a shared reproducer or prove which scanner assumption failed. Feed each confirmed strength, blind spot, false-positive pattern, and useful artifact back into the relevant goals and scanner configuration.
```

<a id="goal-117"></a>

### 117. Security-agent calibration with historical bugs, mutants, and negative controls

<!-- slug: agent-calibration-benchmark; prompt-chars: 3831; utf8-bytes: 3832 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/agent-calibration-benchmark.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Construct a hidden calibration set from historical project bugs, reverted fixes, realistic security mutants, difficult nonbugs, and clean controls. Preserve exact revisions and public proofs. Run agents without exposing labels and measure root-cause recall, false positives, validation accuracy, time/cost, coverage, and which phase found or rejected each case.

Analyze misses by missing knowledge, wrong scope, lazy review, weak oracle, unreachable harness, model refusal, or verification failure. Turn failure classes into concrete goal improvements and rerun held-out cases. Avoid training directly on the test set or optimizing for headline recall; rotate unseen cases and require that calibration gains transfer to live repository work.
```

<a id="goal-118"></a>

### 118. Agent sandbox, credential, environment, and artifact-isolation audit

<!-- slug: agent-runtime-isolation; prompt-chars: 3865; utf8-bytes: 3866 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/agent-runtime-isolation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Treat the scanned repository, its build scripts, tests, tools, and dependencies as potentially hostile to the scanning environment. Inventory filesystem, network, process, device, credential, keyring, SSH/Git, cloud-token, and environment-variable access available to agents and subprocesses.

Run canary credentials and controlled malicious fixtures to test unintended reads, writes, exfiltration, cross-scan contamination, and result tampering. Keep state and sensitive outputs outside the target worktree with restrictive permissions; pin immutable revisions for bulk work. Compare local, container, and sandboxed execution and verify cancellation/cleanup. Tighten the smallest boundary that blocks a demonstrated capability without making required builds or proofs impossible.
```

<a id="goal-119"></a>

### 119. Bulk multi-repository and ecosystem recurrence mining

<!-- slug: bulk-ecosystem-recurrence; prompt-chars: 3843; utf8-bytes: 3844 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bulk-ecosystem-recurrence.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run resumable campaigns over a pinned list of repositories and full commit hashes, with per-repository profile, scope, mode, budget, and result directory. Include related nodes, wallets, crypto libraries, storage engines, bindings, forks, and downstream users where authorization permits.

Normalize confirmed findings into reusable bug shapes and search for recurrence across projects, copied code, shared dependencies, test vectors, and API assumptions. Preserve project-specific contracts so differences are not treated as bugs merely because implementations disagree. Feed cross-project evidence back into local priorities, sibling-test transplantation, dependency upgrades, and new goals whose first experiment is defined for more than one repository.
```

<a id="goal-120"></a>

### 120. Sparrow PSBT, signing-intent, and hardware-wallet verification audit

<!-- slug: sparrow-psbt-signing-intent; prompt-chars: 3808; utf8-bytes: 3809 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-psbt-signing-intent.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For Sparrow and related desktop wallets, map PSBT creation, import, merge, update, signing, finalization, broadcast, and save/reopen flows. Track transaction inputs/outputs, amounts, fees, change identification, sighash, derivation paths, master fingerprints, descriptors, multisig policy, labels, and network.

Build adversarial PSBTs and a fake or instrumented hardware wallet to compare host intent, device display, signed data, and final transaction. Test duplicate/conflicting fields, unknown/proprietary data, partial signatures, reordered outputs, malicious change claims, fee surprises, and mixed networks. Require an end-to-end proof of any signing mismatch; never infer safety from a correct UI summary alone.
```

<a id="goal-121"></a>

### 121. Sparrow backend trust, privacy, and network-isolation audit

<!-- slug: sparrow-backend-privacy; prompt-chars: 3821; utf8-bytes: 3822 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-backend-privacy.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model Sparrow's trust in Bitcoin Core, Electrum servers, public servers, Tor/proxy paths, fee sources, exchange-rate sources, and fallback logic. Identify which claims are verified locally and which can influence balances, history, UTXOs, confirmations, fees, addresses queried, or transaction broadcast.

Build a deterministic lying backend that omits, reorders, fabricates, delays, or equivocates about headers, transactions, UTXOs, mempool state, and fees. Test reorgs, reconnects, server switches, mainnet/testnet/signet confusion, proxy bypass, DNS/network leaks, and wallet-query correlation. Separate privacy loss, misleading display, signing risk, and recoverable availability failures, and preserve reusable server transcripts.
```

<a id="goal-122"></a>

### 122. Sparrow wallet-file encryption, backup, import, and recovery audit

<!-- slug: sparrow-wallet-recovery; prompt-chars: 3864; utf8-bytes: 3865 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-wallet-recovery.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map Sparrow wallet creation, password/KDF use, encrypted persistence, keystores, seeds/xprvs, watch-only data, descriptors, labels, backups, imports/exports, migrations, autosave, and restore. Identify authoritative memory and disk state and every point where plaintext or partially updated data can remain.

Inject wrong passwords, malformed or truncated files, partial writes, rename/fsync failures, crashes, concurrent opens, old-version files, and interrupted migrations. Restart and verify no silent key loss, mixed encrypted/plaintext state, corrupted backup replacement, network mismatch, or watch-only wallet becoming sign-capable. Trace secrets through Java objects, clipboard/logging, temporary files, and cleanup, using only scratch wallets and deterministic fixtures.
```

<a id="goal-123"></a>

### 123. Sparrow Java and JavaFX lifecycle, concurrency, and secret-retention audit

<!-- slug: sparrow-javafx-lifecycle; prompt-chars: 3825; utf8-bytes: 3826 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-javafx-lifecycle.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit JavaFX application-thread rules, background services, futures, event handlers, cancellation, wallet/network switching, hardware-device callbacks, shutdown, and exception paths. Map ownership of windows, controllers, wallets, sessions, listeners, executors, and cached models.

Create deterministic barriers instead of sleeps to force close/reopen, rapid network changes, device disconnect, failed signing, repeated dialogs, and shutdown during persistence. Check stale UI state, double actions, races, leaked listeners/threads, use-after-close logic, and secrets retained in Strings, logs, clipboard, crash reports, or long-lived objects. Prefer public UI/service scenarios backed by state assertions rather than visual checks alone.
```

<a id="goal-124"></a>

### 124. Sparrow build, submodule, update, and release-integrity audit

<!-- slug: sparrow-release-integrity; prompt-chars: 3815; utf8-bytes: 3816 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-release-integrity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit Sparrow's Gradle build, pinned Java/toolchains, `drongo` and `lark` submodules, dependencies, generated resources, native packaging, reproducible pre-signing binaries, code signing, installers, release metadata, and update/download paths.

Rebuild pinned releases on independent environments and explain every binary difference. Test stale or substituted submodules, dependency confusion, untrusted repository/plugin input, update metadata tampering, downgrade/replay, signature or hash mismatch, and platform-specific packaging gaps. Separate reproducible payloads from nondeterministic signing/installer layers. A fix must block a demonstrated supply-chain or verification failure, not merely add another unchecked hash.
```

<a id="goal-125"></a>

### 125. LevelDB WAL, MANIFEST, VersionSet, and crash-recovery audit

<!-- slug: leveldb-crash-recovery; prompt-chars: 3840; utf8-bytes: 3841 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/leveldb-crash-recovery.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model LevelDB's write path and durable authority across WriteBatch, log append/sync, memtable and immutable memtable, table building, VersionEdit, MANIFEST, CURRENT, file rename/delete, compaction, close, and reopen. Enumerate crash points before and after every state transition.

Use a deterministic Env or fault filesystem to inject short writes, sync/rename failures, dropped or reordered persistence, truncation, stale files, and process death. Reopen after each schedule and compare visible key/value state with the acknowledged-write contract, checking idempotence, orphan cleanup, sequence numbers, and background errors. Preserve minimized operation/fault traces and distinguish LevelDB defects from client misuse of `sync` or recovery guarantees.
```

<a id="goal-126"></a>

### 126. LevelDB comparator, snapshot, iterator, filter, and compaction semantics audit

<!-- slug: leveldb-semantic-matrix; prompt-chars: 3822; utf8-bytes: 3823 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/leveldb-semantic-matrix.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build a semantic matrix for custom comparators and names, internal-key ordering, snapshots, forward/backward iterators, seeks, deletions/tombstones, WriteBatch order, Bloom/filter policies, block/table caches, and compaction across levels.

Generate operation traces with equal-prefix keys, unusual byte strings, comparator edge cases, snapshots spanning updates/deletes, iterator invalidation, filter false positives, and overlapping compactions. Compare incremental results with a simple ordered reference model after every step. Test reopen with changed or inconsistent comparator/filter configuration. Classify allowed probabilistic differences separately and require a minimized trace before changing stable API or on-disk behavior.
```

<a id="goal-127"></a>

### 127. LevelDB corruption, checksums, background errors, and client-assumption audit

<!-- slug: leveldb-corruption-client-contracts; prompt-chars: 3891; utf8-bytes: 3892 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/leveldb-corruption-client-contracts.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 characters. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inject bit flips, truncation, malformed blocks, bad checksums, missing files, stale lock/CURRENT/MANIFEST state, permission errors, disk full, and background compaction failures through LevelDB's Env boundary. Compare normal and paranoid checking, cache-cold and cache-warm reads, repair, reopen, iteration, and compaction behavior.

Trace how corruption and background errors reach callers: returned Status, delayed failure, logging, read-only survival, or silent omission. Audit client assumptions about single-process access, atomic batches, snapshots, iterator lifetime, comparator stability, sync durability, and repair. For Bitcoin Core or other wrappers, build an engine-neutral public reproduction and determine whether the bug is in LevelDB, the wrapper, or an undocumented assumption.
```
