# Reusable Continuous Agent Goals for Bitcoin Core, libsecp256k1, Sparrow Wallet, LevelDB, and Related Projects

---

# Uber-goal

<!-- prompt-chars: 3998; utf8-bytes: 3998 -->

Run continuing evidence-first investigations from the 161-goal catalog in `agent-journal/campaign-goals.md`. Treat each goal as a campaign, not repository completion. Continue until a tool/session limit or blocker, then leave an handoff.

Use `agent-journal/uber-goal-state.md` as ledger and `agent-journal/<slug>.md` per campaign. Before each cycle inspect repo profile, worktree/branch/base/HEAD/remotes/dirty state/jobs, catalog hash, `URGENT.md`, journals/artifacts, indexes, history, issues, PRs, and knowledge. Record revision, scope, tool/model, seed, budgets, and outputs. Never overwrite unrelated work or upstream refs.

For Bitcoin Core, first read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase. Search by PR, branch, commit, patch-id, symbol, bug shape, and reproducer. Classify every candidate as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown; recheck HEAD/upstream before acting. Do not recreate owned work or revive a refutation without new evidence. Rank consensus inflation/permanent split, remote memory safety or crash, funds/key/signing authorization, durable corruption or restart failure, and censorship/liveness above ordinary resource findings. Pure memory/disk/CPU amplification may occupy at most one of five Core cycles unless it demonstrates a practical default-node kill or exposes a deeper invariant.

Select pending, reopened, promoted, or highest-risk goals by a recorded draw; urgent candidates may preempt. Bind each draw to one symbol or entry path, input domain, oracle fields, perturbation, proof, and stop rule. Redraw if generic. Run 4-6 falsifiable iterations until confirmation, bounded exhaustion, or a blocker, then re-rank. Inspect callers/history, run the narrowest experiment, record commands/output, and end confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer when practical; verify before exposing a fix. No-new is evidence, not completion.

Track reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Match roots as new, persisting, reopened, resolved, or unknown. Preserve WIP tests/fuzzers/harnesses, branches, seeds, traces, failed attempts, false-positive reasons, and negative results with provenance. Reports, specs, sibling code, tools, scanners, and model opinions are leads, not proof.

Every cycle must return entropy: new bug shapes, invariants, oracles, suspicious sites, blocked paths, coverage gaps, tool limits, false-positive conditions, project priors, or analogies. Amend goals with concrete triggers, negative knowledge, and next experiments. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, lineage, and first runnable experiment. Promote only evidence-backed nonduplicates under 4,000 UTF-8 bytes; merge/retire overlap without deleting history. Regenerate index, count, character metadata, catalog hash, and lineage.

Keep at most ten live `URGENT.md` items ordered by severity, reachability, confidence, ownership, and next-proof value. Maintain append-only `agent/all-findings`; copy each fix, test, harness, experiment, evidence, and catalog-evolution commit with source mapping. Never squash/rewrite/force-push; repair or revert broken tips while retaining history.

Use scratch data and deterministic fault injection; leave no jobs running. Confirmed defects get the smallest standalone buildable commit with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. Update journal, ledger, urgent list, indexes, catalog/lineage, and `agent/all-findings`, then continue.

Adapt by project: Bitcoin Core - consensus/validation/UTXO/P2P/wallet/storage/tooling; libsecp256k1 - arithmetic/constant-time/API/backends; Sparrow - PSBT/signing/hardware-wallet/privacy/backups/releases; LevelDB - recovery/comparators/snapshots/iterators/compaction/corruption. Skip inapplicable surfaces.

---

This revision retains all 142 supplied campaigns, materially amends 18 existing campaigns, and adds 19 focused Bitcoin Core campaigns. There are **161 standalone `/goal` prompts**.

The Core changes are driven by two evidence sources:

- the supplied audit knowledgebase, including current ownership, supersession, refutation, fuzzer-boundary, and priority information;
- Bitcoin Core's official historical security advisories and BIP50, converted into reusable root-cause patterns and executable experiment recipes.

The new `bitcoin-core-security-profile.md` is mandatory context for Core campaigns. It prevents duplicate work by classifying candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown. It also changes Core selection pressure: consensus, remote crashes and memory safety, signing/funds authorization, durable recovery, and censorship/liveness outrank ordinary resource findings. Resource campaigns remain eligible when they demonstrate a distinct root, a practical default-node kill, or a deeper integrity failure.

The amended goals deepen assertion reachability, lifetime and callback ordering, historical variant mining, crash transactionality, fuzz-oracle realism, release backports, consensus mutation, chainstate/UTXO symmetry, mempool exact arithmetic, wallet signing intent, P2P state isolation, calibration, and block-ingress convergence.

Character counting includes the complete text beginning with `/goal` and excludes headings and code-fence markers. The longest prompt is goal **92**, at **3986 Unicode characters** and **3987 UTF-8 bytes**. The uber-goal is **3998 characters** and **3998 bytes**. Canonical goal-catalog SHA-256: `d00cc061c7dbc9911898a148386820ea40f2caa3550b247a5130297db71b986a`.

## Source snapshots

- Supplied goal catalog SHA-256: `27676efe7ebde63942dda9972156075ff58300cf283908a86a5520687ba67cfe`
- Supplied Core/secp knowledgebase SHA-256: `5ddd768e1577dde408dd8ab97dd5f54fb2bdf172c90e46232f6d927246a88bc4`

## External research incorporated

- [Bitcoin Core security advisories](https://bitcoincore.org/en/security-advisories/): official severity policy and historical root causes.
- [BIP50](https://github.com/bitcoin/bips/blob/master/bip-0050.mediawiki): database-dependent consensus divergence and chain-fork post-mortem.
- [CVE-2018-17144](https://bitcoincore.org/en/2018/09/20/notice/): skipped duplicate-input validation, assertion failure, and inflation.
- [CVE-2024-52911](https://bitcoincore.org/en/2026/05/05/disclose-cve-2024-52911/): early-return script-check use-after-free.
- [CVE-2024-35202](https://bitcoincore.org/en/2024/10/08/disclose-blocktxn-crash/): compact-block one-shot state reuse.
- [CVE-2024-52912](https://bitcoincore.org/en/2024/07/03/disclose-timestamp-overflow/): signed overflow and adjusted-time netsplit.
- [CVE-2024-52913](https://bitcoincore.org/en/2024/07/03/disclose_already_asked_for/): bounded-map eviction enabling transaction censorship.
- [CVE-2024-52914](https://bitcoincore.org/en/2024/07/03/disclose-orphan-dos/): quadratic orphan resolution and expensive invalid work.
- [CVE-2024-52921](https://bitcoincore.org/en/2024/10/08/disclose-mutated-blocks-hindering-propagation/): one peer clearing another peer's download state.
- [CVE-2017-18350](https://bitcoincore.org/en/2019/11/08/CVE-2017-18350/): signedness and syscall-length memory corruption.
- [CVE-2015-3641](https://bitcoincore.org/en/2024/07/03/disclose_receive_buffer_oom/): per-connection receive-buffer allocation.
- [CVE-2025-46597](https://bitcoincore.org/en/2025/10/24/disclose-cve-2025-46597/): 32-bit overflow reachable through alternate block ingress.
- [Project Loupe](https://github.com/project-loupe/loupe): separate discovery and verification, prior-finding search, semantic deduplication, regression PoCs, and durable findings.
- [Codex Security](https://github.com/openai/codex-security): deep repeated discovery, validation, attack-path analysis, knowledge inputs, and coverage-aware completion.
- [Bitcoin Core fuzzing guide](https://github.com/bitcoin/bitcoin/blob/master/doc/fuzzing.md): deterministic corpora, sanitizer and high-throughput builds, multiple fuzz engines, and coverage preservation.
- [bitcoinfuzz](https://github.com/bitcoinfuzz/bitcoinfuzz): differential fuzzing across Bitcoin implementations.
- [libsecp256k1](https://github.com/bitcoin-core/secp256k1): exhaustive, ctime/checkmem, backend, and API assurance patterns.

## Prompt index
- [0. Continuous evidence-first bug mining](#goal-0) - 3610 characters, 3611 bytes
- [1. Source comment versus implementation contract audit](#goal-1) - 3931 characters, 3932 bytes
- [2. Assertion, Assume, and invariant reachability audit](#goal-2) - 3313 characters, 3314 bytes
- [3. Current branch and PR leftover sweep](#goal-3) - 3799 characters, 3800 bytes
- [4. Public API, CLI, RPC, config, and help contract audit](#goal-4) - 3743 characters, 3744 bytes
- [5. Boundary-condition and off-by-one audit](#goal-5) - 3781 characters, 3782 bytes
- [6. Serialization, deserialization, and untrusted-input sweep](#goal-6) - 3760 characters, 3761 bytes
- [7. Untrusted-interface resource-exhaustion variant analysis](#goal-7) - 3790 characters, 3791 bytes
- [8. Locking, threading, and scheduler audit](#goal-8) - 3792 characters, 3793 bytes
- [9. Hit-frequency and suspicious-branch coverage audit](#goal-9) - 3843 characters, 3844 bytes
- [10. Fuzz-target gap and harness-realism audit](#goal-10) - 3930 characters, 3931 bytes
- [11. Sanitizer and Valgrind true-positive sweep](#goal-11) - 3845 characters, 3846 bytes
- [12. Static-analysis true-positive campaign](#goal-12) - 3793 characters, 3794 bytes
- [13. Secret-data lifetime and zeroization audit](#goal-13) - 3738 characters, 3739 bytes
- [14. Secret-dependent control-flow and memory-access audit](#goal-14) - 3729 characters, 3730 bytes
- [15. Public object parsing and validation variant analysis](#goal-15) - 3722 characters, 3723 bytes
- [16. Public API misuse-resistance audit](#goal-16) - 3767 characters, 3768 bytes
- [17. Build-matrix and module-configuration audit](#goal-17) - 3710 characters, 3711 bytes
- [18. Exhaustive and algebraic-invariant audit](#goal-18) - 3728 characters, 3729 bytes
- [19. Benchmark correctness and measurement-integrity audit](#goal-19) - 3703 characters, 3704 bytes
- [20. Simple micro-optimization discovery and proof](#goal-20) - 3709 characters, 3710 bytes
- [21. Long-running rebuild, recovery, and compaction profiling](#goal-21) - 3706 characters, 3707 bytes
- [22. Full sync, IBD, import, and end-to-end profiling](#goal-22) - 3707 characters, 3708 bytes
- [23. Perf and flamegraph investigation without forced commits](#goal-23) - 3698 characters, 3699 bytes
- [24. Disk I/O, persistence growth, and write-amplification audit](#goal-24) - 3720 characters, 3721 bytes
- [25. Recent performance-regression bisect](#goal-25) - 3734 characters, 3735 bytes
- [26. Bug fixed in one subsystem but present in another](#goal-26) - 3925 characters, 3926 bytes
- [27. Error-path partial-state mutation audit](#goal-27) - 3840 characters, 3841 bytes
- [28. Weak-test oracle and mutation-survival audit](#goal-28) - 3850 characters, 3851 bytes
- [29. Dead code, stale feature, and TODO archaeology](#goal-29) - 3750 characters, 3751 bytes
- [30. Security-sensitive and misleading logging audit](#goal-30) - 3707 characters, 3708 bytes
- [31. Cross-layer docs, examples, tests, and implementation audit](#goal-31) - 3695 characters, 3696 bytes
- [32. Whole-history incomplete-fix and migration mining](#goal-32) - 3819 characters, 3820 bytes
- [33. External vulnerability and advisory variant analysis](#goal-33) - 3911 characters, 3912 bytes
- [34. Uncovered-code classification and closure audit](#goal-34) - 3807 characters, 3808 bytes
- [35. Mutation-testing campaign](#goal-35) - 3769 characters, 3770 bytes
- [36. Cross-tool sanitizer and static-analysis matrix](#goal-36) - 3700 characters, 3701 bytes
- [37. Build dead-zone and conditional-compilation audit](#goal-37) - 3690 characters, 3691 bytes
- [38. Failure cleanup and crash-safety audit](#goal-38) - 3797 characters, 3798 bytes
- [39. Generated-artifact and test-vector determinism audit](#goal-39) - 3653 characters, 3654 bytes
- [40. Independent multi-agent disagreement and adjudication audit](#goal-40) - 3870 characters, 3871 bytes
- [41. History archaeology from a seed topic](#goal-41) - 3825 characters, 3826 bytes
- [42. CI, coverage-bot, and review-bot follow-up audit](#goal-42) - 3704 characters, 3705 bytes
- [43. Option and API lifecycle audit](#goal-43) - 3724 characters, 3725 bytes
- [44. Secret-copy and compiler-optimization audit](#goal-44) - 3694 characters, 3695 bytes
- [45. Constant-time boundary and declassification audit](#goal-45) - 3696 characters, 3697 bytes
- [46. Public API output-on-failure audit](#goal-46) - 3728 characters, 3729 bytes
- [47. Build-system and CI parity audit](#goal-47) - 3674 characters, 3675 bytes
- [48. Property, exhaustive, and algebraic oracle expansion](#goal-48) - 3887 characters, 3888 bytes
- [49. Critical whole-history must-fix sweep](#goal-49) - 3843 characters, 3844 bytes
- [50. Fuzz Introspector blocker and complexity audit](#goal-50) - 3684 characters, 3685 bytes
- [51. Invariant, differential, and metamorphic audit](#goal-51) - 3831 characters, 3832 bytes
- [52. Integer overflow, narrowing, signedness, and division audit](#goal-52) - 3909 characters, 3910 bytes
- [53. Statistical timing-side-channel campaign](#goal-53) - 3700 characters, 3701 bytes
- [54. RAII, smart-pointer, and resource-leak audit](#goal-54) - 3706 characters, 3707 bytes
- [55. Alternative-implementation compatibility-difference audit](#goal-55) - 3875 characters, 3876 bytes
- [56. Stale PR critical-fix resurrection audit](#goal-56) - 3715 characters, 3716 bytes
- [57. Local-reasoning domain and relationship audit](#goal-57) - 3909 characters, 3910 bytes
- [58. Exact helper reuse and minimal helper-extension audit](#goal-58) - 3741 characters, 3742 bytes
- [59. C/C++ supply-chain and security-gate audit](#goal-59) - 3749 characters, 3750 bytes
- [60. Historical reviewer-preference mining and reusable review skill](#goal-60) - 3718 characters, 3719 bytes
- [61. Stateful contract-fuzzer expansion](#goal-61) - 3934 characters, 3935 bytes
- [62. Rejected-finding resurrection and assumption attack](#goal-62) - 3756 characters, 3757 bytes
- [63. Loupe and Codex Security scout, verifier, fixer, and reporter pipeline](#goal-63) - 3505 characters, 3506 bytes
- [64. Finding deduplication, recurrence, and semantic-fingerprint audit](#goal-64) - 3496 characters, 3497 bytes
- [65. Contributor-branch and work-in-progress radar](#goal-65) - 3780 characters, 3781 bytes
- [66. Cherry-pick, backport, and release-branch correctness audit](#goal-66) - 3849 characters, 3850 bytes
- [67. Release-to-release behavioral and consensus differential](#goal-67) - 3713 characters, 3714 bytes
- [68. Architecture, endianness, word-size, and ABI parity audit](#goal-68) - 3705 characters, 3706 bytes
- [69. SIMD, assembly, and portable-reference backend differential](#goal-69) - 3706 characters, 3707 bytes
- [70. Compiler, optimization, LTO, PGO, and BOLT differential](#goal-70) - 3740 characters, 3741 bytes
- [71. Deterministic simulation and failure-schedule exploration](#goal-71) - 3862 characters, 3863 bytes
- [72. Filesystem, power-loss, and crash-consistency injection](#goal-72) - 3834 characters, 3835 bytes
- [73. Network fragmentation, reordering, and partial-I/O state-machine audit](#goal-73) - 3789 characters, 3790 bytes
- [74. Memory pressure, OOM, allocator, and fragmentation audit](#goal-74) - 3730 characters, 3731 bytes
- [75. Build throughput, dependency graph, and container-cache audit](#goal-75) - 3756 characters, 3757 bytes
- [76. Reproducible binaries, Guix, and toolchain-provenance audit](#goal-76) - 3710 characters, 3711 bytes
- [77. Symbolic execution and bounded-model-checking campaign](#goal-77) - 3706 characters, 3707 bytes
- [78. Compiler-transformation validation and miscompile isolation](#goal-78) - 3758 characters, 3759 bytes
- [79. Fuzz-corpus stewardship, minimization, and transfer audit](#goal-79) - 3705 characters, 3706 bytes
- [80. Fuzz-engine and property-framework differential](#goal-80) - 3716 characters, 3717 bytes
- [81. Specification, test-vector, and formal-model drift audit](#goal-81) - 3885 characters, 3886 bytes
- [82. secp256k1 field and scalar representation matrix](#goal-82) - 3731 characters, 3732 bytes
- [83. secp256k1 group, ecmult, and formula-parity audit](#goal-83) - 3712 characters, 3713 bytes
- [84. secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit](#goal-84) - 3715 characters, 3716 bytes
- [85. Bitcoin consensus mutation-score and kill-test audit](#goal-85) - 3854 characters, 3855 bytes
- [86. Bitcoin chainstate, reorg, prune, and index crash-symmetry audit](#goal-86) - 3828 characters, 3829 bytes
- [87. Bitcoin mempool, package, and eviction-accounting audit](#goal-87) - 3826 characters, 3827 bytes
- [88. Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit](#goal-88) - 3859 characters, 3860 bytes
- [89. Bitcoin P2P transport, permission, and peer-accounting audit](#goal-89) - 3866 characters, 3867 bytes
- [90. Whole-PR, commit, and external knowledge-base recipe synthesis](#goal-90) - 3474 characters, 3475 bytes
- [91. Compiler and binary-hardening configuration audit](#goal-91) - 3768 characters, 3769 bytes
- [92. ABI layout, alignment, aliasing, and object-lifetime audit](#goal-92) - 3986 characters, 3987 bytes
- [93. Allocation, syscall, clock, randomness, and callback fault injection](#goal-93) - 3893 characters, 3894 bytes
- [94. Bindings, FFI, and language-wrapper parity audit](#goal-94) - 3706 characters, 3707 bytes
- [95. Database-engine and persistence-semantics differential](#goal-95) - 3775 characters, 3776 bytes
- [96. TODO, FIXME, stub, and deferred-work challenge audit](#goal-96) - 3863 characters, 3864 bytes
- [97. C and C++ defect-taxonomy sweep](#goal-97) - 3799 characters, 3800 bytes
- [98. Floating-point edge values, sanitizer resurrection, and fuzz-exclusion audit](#goal-98) - 3971 characters, 3972 bytes
- [99. Clean-room reimplementation and executable differential audit](#goal-99) - 3954 characters, 3955 bytes
- [100. Dangerous-sink reverse reachability and public attack synthesis](#goal-100) - 3291 characters, 3292 bytes
- [101. Public-boundary characterization and minimal-fix sequencing](#goal-101) - 3293 characters, 3294 bytes
- [102. Durable suspicion artifacts and cross-model replay](#goal-102) - 3260 characters, 3261 bytes
- [103. Finding composition and end-to-end exploit-chain synthesis](#goal-103) - 3261 characters, 3262 bytes
- [104. Analogical vulnerability translation and target-domain search](#goal-104) - 3918 characters, 3919 bytes
- [105. Project vulnerability autopsy and author-feature recurrence mining](#goal-105) - 3257 characters, 3258 bytes
- [106. Semantic-twin inconsistency and sloppiness-map audit](#goal-106) - 3911 characters, 3912 bytes
- [107. External conformance-suite and sibling-test transplantation](#goal-107) - 3927 characters, 3928 bytes
- [108. Adversarial peer, client, file, and environment artifact generation](#goal-108) - 3269 characters, 3270 bytes
- [109. Whole-feature cross-file public-path security audit](#goal-109) - 3931 characters, 3932 bytes
- [110. Self-fueling catalog evolution and entropy-quality audit](#goal-110) - 3884 characters, 3885 bytes
- [111. Coverage manifest, deferred-work, and incomplete-scan closure audit](#goal-111) - 3843 characters, 3844 bytes
- [112. Replayable scan recipes, finding continuity, and false-positive revalidation](#goal-112) - 3348 characters, 3349 bytes
- [113. Risk ranking, deep-scan stopping, and marginal-yield audit](#goal-113) - 3289 characters, 3290 bytes
- [114. Threat-model and knowledge-base conversion into executable oracles](#goal-114) - 3818 characters, 3819 bytes
- [115. Committed-diff, working-tree, and pre-commit security regression audit](#goal-115) - 3791 characters, 3792 bytes
- [116. Cross-scanner differential and disagreement audit](#goal-116) - 3787 characters, 3788 bytes
- [117. Security-agent calibration with historical bugs, mutants, and negative controls](#goal-117) - 3885 characters, 3886 bytes
- [118. Agent sandbox, credential, environment, and artifact-isolation audit](#goal-118) - 3860 characters, 3861 bytes
- [119. Bulk multi-repository and ecosystem recurrence mining](#goal-119) - 3838 characters, 3839 bytes
- [120. Sparrow PSBT, signing-intent, and hardware-wallet verification audit](#goal-120) - 3803 characters, 3804 bytes
- [121. Sparrow backend trust, privacy, and network-isolation audit](#goal-121) - 3816 characters, 3817 bytes
- [122. Sparrow wallet-file encryption, backup, import, and recovery audit](#goal-122) - 3859 characters, 3860 bytes
- [123. Sparrow Java and JavaFX lifecycle, concurrency, and secret-retention audit](#goal-123) - 3820 characters, 3821 bytes
- [124. Sparrow build, submodule, update, and release-integrity audit](#goal-124) - 3810 characters, 3811 bytes
- [125. LevelDB WAL, MANIFEST, VersionSet, and crash-recovery audit](#goal-125) - 3835 characters, 3836 bytes
- [126. LevelDB comparator, snapshot, iterator, filter, and compaction semantics audit](#goal-126) - 3817 characters, 3818 bytes
- [127. LevelDB corruption, checksums, background errors, and client-assumption audit](#goal-127) - 3886 characters, 3887 bytes
- [128. Bitcoin full, compact, RPC, and disk block-ingress convergence](#goal-128) - 3286 characters, 3286 bytes
- [129. Bitcoin wallet callback, rescan, reload, and restart convergence](#goal-129) - 2674 characters, 2674 bytes
- [130. Bitcoin validation-callback teardown linearization](#goal-130) - 2838 characters, 2838 bytes
- [131. Bitcoin fee-estimator checkpoint continuation fuzzing](#goal-131) - 2548 characters, 2548 bytes
- [132. Bitcoin mempool dump, import, and restart continuation equivalence](#goal-132) - 2430 characters, 2430 bytes
- [133. Bitcoin PSBT merge and finalization algebra](#goal-133) - 2727 characters, 2727 bytes
- [134. Bitcoin script-cache and verification-interface parity](#goal-134) - 2746 characters, 2746 bytes
- [135. Bitcoin wallet spend-construction contract matrix](#goal-135) - 2584 characters, 2584 bytes
- [136. Bitcoin AddrMan, asmap, and peers.dat state-machine audit](#goal-136) - 2609 characters, 2609 bytes
- [137. secp256k1 Silent Payments sender and receiver duality](#goal-137) - 2522 characters, 2522 bytes
- [138. secp256k1 SHA256 override dispatch closure](#goal-138) - 2626 characters, 2626 bytes
- [139. secp256k1 ecmult scratch rollback transactionality](#goal-139) - 2534 characters, 2534 bytes
- [140. secp256k1 EllSwift total-map and XDH differential](#goal-140) - 2483 characters, 2483 bytes
- [141. Bitcoin Core and secp256k1 BIP324 transcript parity](#goal-141) - 2549 characters, 2549 bytes
- [142. Bitcoin Core advisory-root-cause variant matrix](#goal-142) - 3131 characters, 3132 bytes
- [143. Bitcoin UTXO, coins cache, undo, and reorg conservation](#goal-143) - 3093 characters, 3094 bytes
- [144. Bitcoin invalid-block containment and rejection taxonomy](#goal-144) - 3071 characters, 3072 bytes
- [145. Bitcoin compact-block reconstruction ownership and one-shot lifecycle](#goal-145) - 3065 characters, 3066 bytes
- [146. Bitcoin block-index candidate, unlinked, and comparator-key state machine](#goal-146) - 2951 characters, 2952 bytes
- [147. Bitcoin durable batch retry and dirty-set transactionality](#goal-147) - 2988 characters, 2989 bytes
- [148. Bitcoin AssumeUTXO trust, background validation, and cleanup convergence](#goal-148) - 3037 characters, 3038 bytes
- [149. Bitcoin validation-cache provenance and mutable-object invalidation](#goal-149) - 3058 characters, 3059 bytes
- [150. Bitcoin asynchronous script-check and validation-work lifetime audit](#goal-150) - 2987 characters, 2988 bytes
- [151. Bitcoin headers-sync, adjusted-time, and IBD slot liveness](#goal-151) - 2954 characters, 2955 bytes
- [152. Bitcoin transaction-request, orphan, and censorship-resistance state machine](#goal-152) - 2996 characters, 2997 bytes
- [153. Bitcoin external-signer and signing-intent authorization](#goal-153) - 3019 characters, 3020 bytes
- [154. Bitcoin wallet database transactionality and fault-injection matrix](#goal-154) - 3027 characters, 3028 bytes
- [155. Bitcoin block and undo file cursor, seek, and format-width boundaries](#goal-155) - 2989 characters, 2990 bytes
- [156. Bitcoin release-branch security backport and disclosure parity](#goal-156) - 2928 characters, 2929 bytes
- [157. Bitcoin release verification, trusted-key quorum, and Git ancestry audit](#goal-157) - 2967 characters, 2968 bytes
- [158. Bitcoin critical RPC, REST, IPC, and C++ API boundary audit](#goal-158) - 2924 characters, 2925 bytes
- [159. Bitcoin valid-work adversarial block and miner-gated failure campaign](#goal-159) - 3035 characters, 3036 bytes
- [160. Bitcoin negative-control, supersession, and refutation replay](#goal-160) - 3040 characters, 3041 bytes

## Goals

<a id="goal-0"></a>

### 0. Continuous evidence-first bug mining

<!-- slug: continuous-bug-mining; prompt-chars: 3610; utf8-bytes: 3611 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/continuous-bug-mining.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Mine real defects while rotating current diffs, history, TODOs, coverage, mutations, sanitizer/static reports, benchmarks, platform matrices, advisories, sibling implementations, and contributor branches. For Bitcoin Core, load the Core security profile and supplied knowledgebase before selecting work. Apply its severity ladder, ownership/supersession map, known refutations, and current proof gaps. Do not spend repeated cycles rediscovering already-owned OOM families; prioritize consensus, crashes/memory safety, funds/signing intent, durable recovery, and censorship/liveness unless a resource path has a distinct practical kill or deeper root.

Maintain a risk map keyed by subsystem, public entry path, attacker prerequisite, durable state, concurrency, cache provenance, and oracle quality. Pick the highest-value unexplored cell, run a few evidence-producing passes, preserve artifacts, then switch methods. A no-new pass must redirect to another method, bug family, historical range, or untested state transition.

Use explicit scout, validation, attack-path, fix, and independent-review phases when useful. Record marginal yield and the exact missing proof for every retained candidate; never manufacture a commit.
```

<a id="goal-1"></a>

### 1. Source comment versus implementation contract audit

<!-- slug: comment-code-contract; prompt-chars: 3931; utf8-bytes: 3932 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/comment-code-contract.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Enumerate every source comment in sorted `path:start-line` order. Include production, test, fuzz, build, binding, and generated-source comments, recording explicit exclusions. Prioritize claims using must/never/always, lock/lifetime or cache/ownership rules, wire/consensus compatibility, secret handling, recovery, bounds, and performance assumptions.

For each comment, record `path:symbol:normalized-comment-hash`, verdict, and resume status in a coverage manifest. Locate governed code and callers, then compare tests, docs, blame, and history. Decide whether code or comment is wrong. Change behavior only when independent evidence supports the contract; otherwise fix the stale claim. Skip purely stylistic improvements. Behavior changes need a regression oracle proving prior tests missed the mismatch. Resume at the first missing manifest entry.
```

<a id="goal-2"></a>

### 2. Assertion, Assume, and invariant reachability audit

<!-- slug: assertion-invariant-audit; prompt-chars: 3313; utf8-bytes: 3314 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/assertion-invariant-audit.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Search assertions, `Assume`, `Assert`, `CHECK_NONFATAL`, unreachable markers, VERIFY checks, and impossible-state comments. Trace every caller in assertion-enabled, release, no-assert, fuzz, network, RPC, persisted-data, and optional-module builds. For Bitcoin Core seed from CVE-2018-17144's validation hidden behind sanity assertions, CVE-2024-35202's one-shot compact-block assertion, and current knowledgebase assertion families.

On an isolated branch add temporary guards at dereferences, divisions, indexes, casts, cache transitions, queue ownership, and durable-state publication. Fuzz public paths and compare Debug/NDEBUG outcomes. A legal untrusted input must fail through validation, not abort or become UB when checks compile out. Distinguish an internal contract violation from a missing boundary check, preserve the smallest violating input, and mutate/remove the upstream guard to prove the regression oracle.
```

<a id="goal-3"></a>

### 3. Current branch and PR leftover sweep

<!-- slug: current-pr-leftovers; prompt-chars: 3799; utf8-bytes: 3800 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/current-pr-leftovers.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For every commit on the current branch, state its intended behavior or migration rule, then search the entire tree for analogous sites that should have changed. Look for stale names/comments/tests, duplicate old logic, partial API conversions, forgotten help or release text, generated files, build lists, optional modules, cache/index formats, lock annotations, and cleanup paths.

Use history and review discussion to distinguish deliberate scope from accidental omission. Check each commit independently and the combined stack. Fix only true leftovers, one per follow-up commit, preserving the original intent. Also record unresolved review objections and predict which current changes would trigger the same objection.
```

<a id="goal-4"></a>

### 4. Public API, CLI, RPC, config, and help contract audit

<!-- slug: public-interface-contracts; prompt-chars: 3743; utf8-bytes: 3744 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/public-interface-contracts.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare each public interface end to end: registration, parser, type/range/unit conversion, defaults, aliases, help text, errors, runtime observation, persistence, restart behavior, docs, tests, and release notes. Cover RPC/REST/CLI/config and library headers where present.

Find values that are parsed but ignored, documented but not accepted, stored in the wrong unit/type, applied only on one lifecycle path, or reported with stale fields/bounds. Exercise exact zero/one/max/negative/duplicate/unknown cases. Prefer behavioral assertions over string-only tests. Preserve compatibility unless the intended change is proven from policy, history, and callers.
```

<a id="goal-5"></a>

### 5. Boundary-condition and off-by-one audit

<!-- slug: boundary-off-by-one; prompt-chars: 3781; utf8-bytes: 3782 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/boundary-off-by-one.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Mine comparisons and arithmetic around heights, times, versions, sequence numbers, counts, amounts, offsets, file positions, cache sizes, resource limits, vector/span lengths, varints, scalar/field bounds, epochs, and iterator ranges. Review `<` versus `<=`, zero/one/max, empty/full, first/last, signed sentinels, and wraparound.

For each candidate, write the mathematical domain and expected boundary table before changing code. Test immediately below, at, and above the boundary on 32- and 64-bit-relevant widths. Separate compatibility/consensus rules from local policy. Require a focused failing-before test or executable arithmetic proof; do not change externally visible boundaries from intuition.
```

<a id="goal-6"></a>

### 6. Serialization, deserialization, and untrusted-input sweep

<!-- slug: serialization-untrusted-input; prompt-chars: 3760; utf8-bytes: 3761 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/serialization-untrusted-input.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit network, RPC/config, block/transaction/script, key/signature/scalar, wallet/database/index, WAL/MANIFEST/table, and persisted-state parsers. Trace length and tag fields from bytes to allocations, loops, casts, object mutation, and later assumptions.

Look for missing allocation/CPU bounds, non-canonical encodings, truncation, signedness errors, duplicate encodings, partially initialized outputs, parse-then-assume paths, and failure after state mutation. Define valid and intentionally invalid domains. Add round-trip, canonicalization, negative, and output-on-failure oracles. Use malformed fixtures under ASan/UBSan/allocator limits and preserve minimized inputs.
```

<a id="goal-7"></a>

### 7. Untrusted-interface resource-exhaustion variant analysis

<!-- slug: resource-exhaustion-variants; prompt-chars: 3790; utf8-bytes: 3791 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/resource-exhaustion-variants.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Seed from historical DoS fixes, advisories, fuzz crashes, queue/cache/eviction changes, and comparable projects. Extract the bug shape: unbounded queue/allocation/log/disk growth, repeated expensive work, bad accounting, retry storm, cache bypass, timeout abuse, compaction amplification, or permission-dependent limit bypass.

Trace a realistic attacker or local-input path and calculate an explicit upper bound for CPU, memory, disk, network, descriptors, and retained state. Build a deterministic low-limit reproducer rather than a huge uncontrolled load. Verify release and restart cleanup. Commit only when the bound or accounting failure is demonstrated; report theoretical amplification separately.
```

<a id="goal-8"></a>

### 8. Locking, threading, and scheduler audit

<!-- slug: locking-threading; prompt-chars: 3792; utf8-bytes: 3793 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/locking-threading.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map protected state, lock order, queues, callbacks, worker ownership, and destruction. In Bitcoin Core prioritize script-check/precomputed-data lifetimes, validation callbacks, chainstate/index restart publication, DB cursor versus resize/compaction, wallet/GUI callbacks, and scheduler shutdown. Include early return, unregister during callback, callback-after-destruction, repeated init/teardown, and partial construction.

Use deterministic barriers and fault hooks, not sleeps. Run TSan and DEBUG_LOCKORDER separately, plus ASan for lifetime failures. Identify the first conflicting access or missing happens-before edge, and prove the fix preserves progress, lock order, callback completion, and restart behavior.
```

<a id="goal-9"></a>

### 9. Hit-frequency and suspicious-branch coverage audit

<!-- slug: hit-frequency-coverage; prompt-chars: 3843; utf8-bytes: 3844 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/hit-frequency-coverage.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Generate line, branch, function, and fuzz hit-frequency coverage, not just percentages. Rank rarely hit branches by security impact, state mutation, complexity, error handling, persistence, secret handling, and proximity to untrusted inputs.

For each high-risk low-hit branch, derive its exact precondition and construct a deterministic trigger. Prioritize collisions, wraparound, clock boundaries, restart states, unusual worker counts, and combinations random fuzzing is unlikely to reach. Explain whether rarity comes from platform/config, a hard guard, harness blockage, dead code, or a missing scenario. Prefer a public-path behavior test over execution-only coverage, compare hit counts, and mutate the branch to prove the new oracle notices wrong behavior.
```

<a id="goal-10"></a>

### 10. Fuzz-target gap and harness-realism audit

<!-- slug: fuzz-target-gaps; prompt-chars: 3930; utf8-bytes: 3931 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/fuzz-target-gaps.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory production entry points against fuzz targets. Find important parsers, state machines, recovery paths, validation branches, and optional modules no target reaches, plus harnesses that use unrealistic state, discard errors, over-constrain inputs, or stop before the real validator.

Upgrade the smallest useful harness through production-like and preferably public construction. Before increasing fuzz volume, add cheap preconditions, postconditions, independent recomputation, inverse relations, and external-reference checks so silent wrongness becomes a failure. Preserve magic values, exclusions, catches, and odd seeds as suspicious evidence and explore their neighbors. Measure static reachability and dynamic coverage, run sanitized and high-throughput builds, retain minimized inputs, and keep harness changes separate from production fixes.
```

<a id="goal-11"></a>

### 11. Sanitizer and Valgrind true-positive sweep

<!-- slug: sanitizer-valgrind; prompt-chars: 3845; utf8-bytes: 3846 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sanitizer-valgrind.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run separate ASan+UBSan, TSan, MSan where fully instrumentable, LeakSanitizer, Valgrind/Memcheck, and relevant fuzz configurations across unit, functional, recovery, benchmark, and narrow long-running workloads.

Treat each warning as a noisy sensor, not a request to silence the tool. Recover why suppressions, skips, or special cases exist; minimize the first invalid operation; then seek a production and preferably public trigger. Classify project bug, test bug, dependency bug, unsupported instrumentation, harmless diagnostic, or false positive, and state the possible security effect. Fix the root cause only after that classification, prove the warning disappears without a broader suppression, and retain the raw trace, reproducer, and nearby untested variants.
```

<a id="goal-12"></a>

### 12. Static-analysis true-positive campaign

<!-- slug: static-analysis-true-positives; prompt-chars: 3793; utf8-bytes: 3794 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/static-analysis-true-positives.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run the repository's linters plus focused clang-tidy, clang static analyzer, CodeQL/Semgrep, IWYU, compiler warnings, and semantic queries. Prioritize lifetime, nullability, use-after-move, uninitialized state, narrowing, overflow, unchecked results, span/string_view lifetime, iterator invalidation, lock contracts, dead stores, and suspicious control flow.

Use text search only to route candidates; prove them through types, call/dataflow, and execution where possible. Build custom queries from proven historical bug shapes. Reject style-only output and document false-positive patterns so later cycles do not repeat them. Each fix must preserve project idioms and include the exact warning/query path.
```

<a id="goal-13"></a>

### 13. Secret-data lifetime and zeroization audit

<!-- slug: secret-lifetime-zeroization; prompt-chars: 3738; utf8-bytes: 3739 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secret-lifetime-zeroization.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Label private keys, nonces, seeds, tweaks, blinding values, passphrases, session material, authentication data, and secret-derived temporaries. Trace every copy, allocation, move, early return, exception/error path, callback capture, log, swap, and destructor.

Verify clearing on all exits and whether the compiler eliminates it; inspect optimized assembly or use project cleanse/checkmem utilities, Valgrind/MSan secret marking, and ctime tests. Distinguish secret, public, and intentionally declassified data. Avoid broad memset churn. Prove the value is sensitive, the old lifetime is avoidably longer, and the chosen mechanism survives optimization.
```

<a id="goal-14"></a>

### 14. Secret-dependent control-flow and memory-access audit

<!-- slug: secret-control-flow; prompt-chars: 3729; utf8-bytes: 3730 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secret-control-flow.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Starting from signing, key generation, nonce/session handling, ECDH/MuSig, encryption/authentication, context randomization, and secret scalar multiplication, trace secret taint into branches, loop counts, array/table indexes, memory addresses, helper selection, and error exits.

Compare constant-time and variable-time helpers, alternative backends, debug/VERIFY modes, and compiler output. Mark explicit declassification boundaries and challenge each one. Use ctime tests, ctgrind/Valgrind or MSan secret marking, dudect-style statistics, and assembly traces. A timing test that passes is supporting evidence, not proof; retain the dataflow argument.
```

<a id="goal-15"></a>

### 15. Public object parsing and validation variant analysis

<!-- slug: public-object-validation; prompt-chars: 3722; utf8-bytes: 3723 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/public-object-validation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare every equivalent parse/validation path for public keys, x-only keys, signatures, scalars, scripts, descriptors, records, table/log entries, addresses, and API wrappers. Seed from historical malformed-input bugs in this and related projects.

Test truncated, oversized, out-of-range, non-canonical, infinity/impossible, duplicate, and mixed-format inputs. Verify failure is consistent, non-crashing, and leaves outputs in the documented safe state. Cross-check parse/serialize round trips and operations after parse. If another implementation diverges, identify which contract is wrong rather than treating majority behavior as truth.
```

<a id="goal-16"></a>

### 16. Public API misuse-resistance audit

<!-- slug: api-misuse-resistance; prompt-chars: 3767; utf8-bytes: 3768 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/api-misuse-resistance.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Read public headers, examples, bindings, tests, and implementation as an adversarial caller. Look for unclear ownership/lifetime, aliasing, context capability, thread-safety, secret/public status, callback obligations, invalidation, optional-module behavior, inconsistent return conventions, and outputs usable after failure.

Construct the smallest plausible misuse example and determine whether docs, examples, types, assertions, or implementation should change. Prefer clarifying and testing the existing contract over API redesign. Check that examples demonstrate validation and cleanup. Commit implementation changes only for a concrete unsafe or ambiguous behavior with a reproducer.
```

<a id="goal-17"></a>

### 17. Build-matrix and module-configuration audit

<!-- slug: build-matrix-modules; prompt-chars: 3710; utf8-bytes: 3711 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/build-matrix-modules.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Enumerate supported compilers, build types, feature modules, wallet/IPC/GUI/tool/bench/fuzz toggles, assembly/SIMD backends, debug/VERIFY/exhaustive modes, static/shared libraries, cross builds, and sanitizer combinations. Compare that inventory with CI.

Cycle through uncovered pairwise and high-risk interactions, not only individual flags. Detect sources/tests omitted under one configuration, stale guards, examples that fail, behavior-changing defaults, and generated/install manifests that drift. Use separate build directories and record native versus cross/emulated evidence. Fix one proven configuration mismatch at a time.
```

<a id="goal-18"></a>

### 18. Exhaustive and algebraic-invariant audit

<!-- slug: exhaustive-algebraic; prompt-chars: 3728; utf8-bytes: 3729 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/exhaustive-algebraic.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Identify operations with formal identities: parse/serialize, add/subtract, multiply/invert, negate twice, normalize/idempotence, tweak relations, sign/verify, connect/disconnect, write/read/recover, insert/delete, iterator forward/backward, and cache recomputation.

State each identity and its valid domain before testing. Exercise exhaustive small domains where available and deterministic randomized properties elsewhere, including invalid inputs and failure-state guarantees. Compare optimized and reference paths. Use a temporary mutation to prove the oracle is sensitive; if a property exposes a bug, minimize the counterexample and preserve it.
```

<a id="goal-19"></a>

### 19. Benchmark correctness and measurement-integrity audit

<!-- slug: benchmark-integrity; prompt-chars: 3703; utf8-bytes: 3704 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/benchmark-integrity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit benchmark names, setup, timed regions, batching, units, input realism, cache state, I/O, allocation, compiler-elision barriers, fixture reuse, and secret-path representativeness. Ensure the benchmark measures the claimed production operation and validates its result.

Run release-like builds, at least five comparable repetitions, and report raw samples, median, spread, outliers, environment, and profile attribution. Check debug/sanitizer runs only for correctness. Fix misleading benchmarks separately from production optimizations and use a temporary no-op or deliberate slowdown to prove the harness detects change.
```

<a id="goal-20"></a>

### 20. Simple micro-optimization discovery and proof

<!-- slug: micro-optimization; prompt-chars: 3709; utf8-bytes: 3710 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/micro-optimization.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Use existing benchmarks and profiles to locate a narrow hot operation. Form one hypothesis involving avoidable allocation/copy/hash/lookup/branch/serialization/lock or better reuse of an existing helper. Prefer code that becomes no more complex.

Benchmark clean base and candidate with identical release flags and at least five interleaved runs; inspect assembly or perf counters when causality is unclear. Run correctness, sanitizer, and relevant fuzz/property tests. Reject wins within noise, workload-specific regressions, and changes that weaken invariants or readability. After each result, return to the next measured hot site.
```

<a id="goal-21"></a>

### 21. Long-running rebuild, recovery, and compaction profiling

<!-- slug: rebuild-recovery-profile; prompt-chars: 3706; utf8-bytes: 3707 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/rebuild-recovery-profile.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select a reproducible rebuild, reindex, rescan, recovery, snapshot load, index build, or compaction workload on scratch data. Record hardware, OS, compiler, flags, filesystem/storage, cache settings, data preparation, commit, and stop condition.

Capture wall/CPU time, peak RSS, reads/writes, fsyncs, compactions, progress, and perf stacks over representative phases. Classify CPU, I/O, lock, allocator, logging, serialization, crypto, or database bottlenecks. Test one minimal hypothesis, rerun identically, and require the expected profile movement as well as a reproducible metric win and correctness/recovery validation.
```

<a id="goal-22"></a>

### 22. Full sync, IBD, import, and end-to-end profiling

<!-- slug: full-sync-ibd-profile; prompt-chars: 3707; utf8-bytes: 3708 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/full-sync-ibd-profile.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run a controlled local IBD/full-sync/import/reindex replay using fixed peers or local block data and a scratch datadir. Pin stop height/range, validation shortcuts, cache/prune/index settings, parallelism, source data, compiler, CPU policy, and storage.

Collect wall/CPU/RSS/disk/network/progress series, logs, and sampled profiles. Separate download, validation, script/crypto, chainstate, block I/O, compaction, and logging time. Do not infer a CPU or disk win from network-bound wall time. Change one bottleneck at a time and require repeated before/after runs, matching final chainstate/results, and no privacy/DoS tradeoff.
```

<a id="goal-23"></a>

### 23. Perf and flamegraph investigation without forced commits

<!-- slug: perf-flamegraph-investigation; prompt-chars: 3698; utf8-bytes: 3699 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/perf-flamegraph-investigation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Choose one representative benchmark, functional test, daemon workload, build, or recovery phase. Record exact environment and capture perf data, flamegraphs, scheduler/lock views, CPU counters, RSS, disk, network, and process metrics.

Distinguish self versus child time, on-CPU versus I/O wait, lock contention, allocator overhead, logging, serialization, hashing/crypto, database/cache, and harness overhead. Rank fix hypotheses by expected impact and risk. Commit only a trivial, proven, measured fix; otherwise leave a detailed journal with raw artifact paths, commands, call stacks, and the next experiment.
```

<a id="goal-24"></a>

### 24. Disk I/O, persistence growth, and write-amplification audit

<!-- slug: disk-io-amplification; prompt-chars: 3720; utf8-bytes: 3721 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/disk-io-amplification.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Measure a fixed storage-heavy workload with process and device counters, filesystem usage, database logs, fsync traces, temporary files, and persistent-state growth. Identify redundant reads, repeated serialization, cache bypass, extra flushes, compaction amplification, stale files, excessive logs, and tests leaking artifacts.

State where data should reside: memory/cache, WAL/log, table/SST, block/index file, or durable metadata. Stop processes before corrupting files so caches cannot mask behavior. A fix must preserve crash consistency and observable state while reducing measured bytes, syncs, or retained space across repeated runs.
```

<a id="goal-25"></a>

### 25. Recent performance-regression bisect

<!-- slug: performance-regression-bisect; prompt-chars: 3734; utf8-bytes: 3735 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/performance-regression-bisect.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Choose a stable benchmark or controlled workload and compare a justified recent commit range under identical conditions. If a regression exceeds noise, bisect to the first bad commit, then profile last-good and first-bad with matching symbols and data.

Explain the causal code path, including changed work counts, cache behavior, allocations, I/O, locking, or compiler output. Preserve the original commit's correctness intent with the smallest fix. Require an exact bisect log, interleaved repeated measurements, before/after profiles, and relevant correctness tests. If no regression is proven, record tested ranges and move to another workload.
```

<a id="goal-26"></a>

### 26. Bug fixed in one subsystem but present in another

<!-- slug: cross-subsystem-bug-shapes; prompt-chars: 3925; utf8-bytes: 3926 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cross-subsystem-bug-shapes.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Mine fixes and the supplied knowledgebase for reusable shapes, first checking ownership and supersession. For Bitcoin Core emphasize: ordered-container keys mutated in place; failed blocks left in candidates; dirty sets cleared before durable success; metadata published ahead of files; stale outputs after failure; one-shot objects reused; peer A mutating peer B state; fee aggregates leaving their representable domain; wallet memory advancing ahead of disk; and cached validity reused under a different context.

Translate each seed into source, sink, missing guard, state transition, and missing oracle. Search semantic twins across the same feature, author series, release branch, parser, cache, index, queue, wallet, and API. Require independent reachability and a mirrored reproducer; provenance ranks candidates but never proves them.
```

<a id="goal-27"></a>

### 27. Error-path partial-state mutation audit

<!-- slug: error-path-state; prompt-chars: 3840; utf8-bytes: 3841 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/error-path-state.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Find functions returning bool/status/result/optional, throwing, or using output parameters while mutating objects, caches, maps, counters, files, transactions, indexes, or caller-visible buffers. Enumerate every failure edge and state changed before it.

Infer whether failure promises unchanged, zeroed, rolled back, invalidated, or explicitly partial state. Inject failure at the earliest and latest practical points and compare complete pre/post state. Then walk the failure condition upward through callers, translating it at each layer until it reaches a public API, network input, file, config, or command, or until a proven guard blocks it. Prefer a functional reproduction, and verify retry/restart behavior before making the smallest contract-preserving fix.
```

<a id="goal-28"></a>

### 28. Weak-test oracle and mutation-survival audit

<!-- slug: weak-test-oracles; prompt-chars: 3850; utf8-bytes: 3851 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/weak-test-oracles.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Search tests that only assert success/no-crash/non-null/non-empty, ignore returns, catch broadly, check logs instead of state, duplicate implementation logic, rely on sleeps, or omit negative cases. Identify the exact behavior each test claims to protect.

Use Mull or temporary mutations: invert/remove a branch or call, alter a bound, skip a state update, or corrupt a result. If the test survives, add the smallest independent property or postcondition that kills the mutant. Prefer multiple oracles and a public/functional reproduction when the helper may be unused. For behavior changes, preserve a characterization of the old result and prove the corrected expectation fails on clean HEAD. Prioritize consensus, crypto, persistence, wallet, networking, and recent fixes.
```

<a id="goal-29"></a>

### 29. Dead code, stale feature, and TODO archaeology

<!-- slug: dead-stale-code; prompt-chars: 3750; utf8-bytes: 3751 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/dead-stale-code.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory uncalled functions, impossible branches, unused parameters/enums/options, obsolete compatibility paths, dormant macros, duplicated implementations, stale tests/docs, and TODO/FIXME/XXX comments. Check every supported build/module and distinguish production from test/fuzz/bench-only reachability.

Use history and linked PRs to decide whether code is intentionally staged, retained compatibility, or genuinely dead. Remove only with call-graph/build/coverage proof, or move harness-only helpers to test support. For TODOs, verify the premise still exists and search whether later work solved it elsewhere. Do not convert harmless defensive checks into cleanup commits.
```

<a id="goal-30"></a>

### 30. Security-sensitive and misleading logging audit

<!-- slug: security-logging; prompt-chars: 3707; utf8-bytes: 3708 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/security-logging.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Trace secrets, private metadata, peer/network identifiers, wallet details, paths, auth/config values, raw payloads, and potentially attacker-controlled strings into logs and errors. Also audit severity/category, rate, repetition, truncation, escaping, and claims that misdescribe actual state.

Classify each value as secret/private/public/intentionally disclosed and each message as user-actionable, operational, debug, or unreachable. Reproduce exact output and volume. Fix concrete leaks, injection/confusion, amplification, or wrong diagnostics; do not merely rewrite prose. Validate redaction and that useful correlation remains.
```

<a id="goal-31"></a>

### 31. Cross-layer docs, examples, tests, and implementation audit

<!-- slug: cross-layer-contracts; prompt-chars: 3695; utf8-bytes: 3696 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cross-layer-contracts.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select one externally meaningful feature at a time and compare its complete contract across source comments, public docs, examples, tests, API schemas, help, release notes, and implementation. Unlike the source-comment campaign, focus on contradictions between layers and copied claims.

Build a contract table for inputs, outputs, defaults, failure behavior, compatibility, lifetime, security, and performance. Use blame/PR discussion to identify the authoritative layer. Fix the smallest proven mismatch and add a behavioral test where the contract was implicit. Record merely unclear wording without committing it.
```

<a id="goal-32"></a>

### 32. Whole-history incomplete-fix and migration mining

<!-- slug: whole-history-leftovers; prompt-chars: 3819; utf8-bytes: 3820 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/whole-history-leftovers.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Walk history in recorded ranges, prioritizing security, consensus, memory safety, persistence, wallet, P2P state machines, and regression-test commits. For Bitcoin Core start from BIP50, official advisories, and the supplied knowledgebase. Reconstruct the introducing change, failed assumption, fix, covert/backport context, test that was missing, and later follow-ups.

Extract a repository-wide rule, then search current HEAD, release branches, related author series, tests, fuzzers, build variants, and sibling code for the pre-fix shape. Query the knowledgebase before acting so merged, owned, superseded, refuted, and assurance-only cases are not revived. Every reviewed range needs an explicit receipt and current reachability proof.
```

<a id="goal-33"></a>

### 33. External vulnerability and advisory variant analysis

<!-- slug: external-vulnerability-variants; prompt-chars: 3911; utf8-bytes: 3912 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/external-vulnerability-variants.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build a seed matrix from official advisories and comparable primary sources. For Bitcoin Core include: BIP50 database-limit consensus split; CVE-2018-17144 assertion-backed duplicate-input inflation; script-check early-return UAF; compact-block one-shot reuse; mutated/stalling peer state contamination; orphan quadratic work; transaction re-request censorship; adjusted-time overflow; signed-char/word-size bugs; aggregate versus per-peer accounting; and dependency trust boundaries.

Abstract each into authority, lifetime, arithmetic domain, state owner, commit point, and missing oracle. Generate current target analogies, then test the most reachable one through a real network/RPC/file/wallet/tool path. A historical similarity is only a lead; prove current mitigation, status/ownership, and a minimized local vector.
```

<a id="goal-34"></a>

### 34. Uncovered-code classification and closure audit

<!-- slug: uncovered-code-classification; prompt-chars: 3807; utf8-bytes: 3808 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/uncovered-code-classification.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Generate current unit, functional, RPC, and fuzz coverage and process uncovered regions systematically. Classify each as platform/config-only, hard error path, missing scenario, harness artifact, genuinely dead, or unreachable because of a bug.

For important uncovered code, derive the exact conditions needed and force them deterministically rather than waiting for probability. Trace from a production or public entry point, add temporary assertions around the rare state, and create the narrowest behavior-checking test. For dead code, prove absence across supported builds and history; for harness artifacts, repair the harness. Maintain a line/range ledger and reject tests whose only effect is executed-line count.
```

<a id="goal-35"></a>

### 35. Mutation-testing campaign

<!-- slug: mutation-testing; prompt-chars: 3769; utf8-bytes: 3770 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/mutation-testing.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run focused mutation testing on high-risk modules and recent changes. Include condition inversions, removed calls/state writes, arithmetic/operator changes, boundary shifts, return-value substitutions, and error-path omissions. Use Mull where practical and targeted temporary mutations elsewhere.

Classify survivors as weak oracle, equivalent mutant, unreachable code, wrong test selection, or potentially missing behavior. Kill valuable non-equivalent mutants with minimal property assertions or reveal a production bug. Track mutation score by subsystem but optimize for dangerous survivors, not percentage. Re-run mutations after each test change and preserve exact mutant identifiers/output.
```

<a id="goal-36"></a>

### 36. Cross-tool sanitizer and static-analysis matrix

<!-- slug: cross-tool-analysis-matrix; prompt-chars: 3700; utf8-bytes: 3701 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cross-tool-analysis-matrix.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build a matrix spanning GCC/Clang versions, ASan, UBSan subchecks, TSan, MSan, LSan, Valgrind, `_GLIBCXX_ASSERTIONS` or equivalent hardening, clang-tidy/static analyzer, and project lint jobs. Exercise representative unit, fuzz, functional, benchmark, and recovery paths.

Look for defects visible only under one optimizer/compiler/tool combination and for suppressions or disabled checks that create blind zones. Minimize and independently confirm every report. Fix project/test bugs only; document dependency/tool issues with versions. Continue filling matrix cells and prioritize untested high-risk configurations.
```

<a id="goal-37"></a>

### 37. Build dead-zone and conditional-compilation audit

<!-- slug: build-dead-zones; prompt-chars: 3690; utf8-bytes: 3691 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/build-dead-zones.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map every `#if`, feature macro, platform/compiler guard, source-list condition, test skip, and CI exclusion to configurations that make it true and false. Identify code no supported build compiles, code compiled but never tested, and high-risk combinations absent from CI.

Use preprocessor output, compile databases, CMake traces, and cross/emulated builds. Check guard polarity, stale feature detection, declaration/definition parity, and release/package inclusion. Fix only demonstrated dead zones or unintended exclusions. Record unsupported combinations distinctly so they are not mistaken for project contracts.
```

<a id="goal-38"></a>

### 38. Failure cleanup and crash-safety audit

<!-- slug: failure-cleanup-crash-safety; prompt-chars: 3797; utf8-bytes: 3798 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/failure-cleanup-crash-safety.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit operations with a commit point and rollback obligation. In Bitcoin Core prioritize wallet SQLite/BDB writes, block and undo flush ordering, block-index and coins dirty sets across retry, prune locks versus durable index state, AssumeUTXO rename/cleanup, settings, mempool/index dumps, and background work that outlives its owner.

Inject failure immediately before and after each mutation, write, flush, fsync, rename, metadata publication, and in-memory adoption. Test clean error, abrupt kill, retry, and restart. Assert the last good state remains usable, progress is not falsely advanced, retries do not forget work, and cleanup is idempotent. Preserve the exact failpoint schedule and on-disk fixture.
```

<a id="goal-39"></a>

### 39. Generated-artifact and test-vector determinism audit

<!-- slug: generated-artifact-determinism; prompt-chars: 3653; utf8-bytes: 3654 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/generated-artifact-determinism.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory generated headers/tables, chain parameters, snapshots, test vectors, docs, schemas, source lists, and codegen outputs. Find the generator, pinned inputs/tool versions, and documented command for each.

Regenerate in a clean locale/timezone and, where practical, a second compiler/OS. Explain every diff: stale artifact, unstable ordering, timestamps, locale, randomness, dependency drift, or undocumented manual edits. Fix generator and artifact together only when inseparable. Require byte-identical repeat generation and a zero-diff verification command.
```

<a id="goal-40"></a>

### 40. Independent multi-agent disagreement and adjudication audit

<!-- slug: multi-agent-adjudication; prompt-chars: 3870; utf8-bytes: 3871 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/multi-agent-adjudication.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Use separate roles: scout proposes a candidate and evidence without a fix; verifier reproduces it, searches prior findings, and locks confirmed/dismissed/inconclusive; fixer sees only confirmed evidence and makes the minimal change; reviewer attacks both proof and patch against project precedent.

Force independent work where useful: one agent writes a behavioral specification or harness, another model or family reimplements or checks it without inheriting the first verdict, and a third adjudicates divergences. Record artifacts, disagreements, changed verdicts, and the missing instrument that would resolve them. Never let an available patch decide whether the finding is real. Preserve rejected candidates as negative knowledge and replay them after relevant code or model changes.
```

<a id="goal-41"></a>

### 41. History archaeology from a seed topic

<!-- slug: history-seed-archaeology; prompt-chars: 3825; utf8-bytes: 3826 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/history-seed-archaeology.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Choose one seed topic or known project bug per cycle, such as cleanup, timing, locks, limits, cache/recovery invariants, migrations, fuzz regressions, or API lifecycle. Use `git log --grep`, `-S`, `-G`, blame, PRs, and release notes to reconstruct its evolution and how the defect or blind spot entered.

Extract the failed assumption, missing test or oracle, review conditions, author/series, affected feature, rejected alternatives, and follow-ups. Search current source, tests, docs, and build files for siblings and unimplemented implications. Name seed commits and explain why the old constraint still applies. Produce a concrete archaeology artifact for every range and immediately continue with the highest-risk project-specific pattern.
```

<a id="goal-42"></a>

### 42. CI, coverage-bot, and review-bot follow-up audit

<!-- slug: ci-review-bot-followup; prompt-chars: 3704; utf8-bytes: 3705 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/ci-review-bot-followup.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Collect current branch CI logs, sanitizer/fuzz failures, static-analysis findings, coverage deltas, flaky-test evidence, and review-bot annotations. Map each result to the exact live line and commit, accounting for stale runs and rebases.

Reproduce locally or in the closest documented environment. Classify true bug, missing test, infrastructure issue, dependency/tool defect, stale warning, or style noise. Search whether reviewers already resolved or rejected it. Fix only meaningful current issues, keeping author commits intact unless explicitly asked, and preserve links plus raw output in the journal and commit body.
```

<a id="goal-43"></a>

### 43. Option and API lifecycle audit

<!-- slug: option-api-lifecycle; prompt-chars: 3724; utf8-bytes: 3725 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/option-api-lifecycle.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For recently touched or suspicious options/APIs, follow creation through registration, parse, validation, storage, observation, scheduling, persistence, restart, migration, disablement, and removal. Cover startup/runtime, first-run, shutdown, reindex/import, offline retry, optional-feature, and non-primary modes.

For periodic or random triggers, expose deterministic due/force hooks and test both pure predicate arithmetic and lifecycle behavior. Check duplicate scheduling and edge-trigger consumption. Prefer no persisted bookkeeping when harmless replay is intended. Fix only an observable lifecycle mismatch with exact command/output proof.
```

<a id="goal-44"></a>

### 44. Secret-copy and compiler-optimization audit

<!-- slug: secret-copy-optimization; prompt-chars: 3694; utf8-bytes: 3695 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secret-copy-optimization.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Find secret-bearing structs, arrays, scalars, and buffers copied by value, returned, inlined, captured, swapped, spilled, or retained in tests/benchmarks. Pay special attention to forced-inline macros, aggregate assignments, ABI copies, vectorization, and cleanup moved across optimization.

Compare `-O0/-O2/-O3`, GCC/Clang, LTO, and relevant architectures using optimized assembly and checkmem tools. Trace every physical and semantic copy and all exits. Reduce copies or clear them only when the secret lifetime and compiler behavior are proven. Re-check performance and constant-time properties after changes.
```

<a id="goal-45"></a>

### 45. Constant-time boundary and declassification audit

<!-- slug: constant-time-declassification; prompt-chars: 3696; utf8-bytes: 3697 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/constant-time-declassification.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map functions reachable from secret operations and mark every variable secret, public, or declassified. Audit where secret-derived values cross into variable-time helpers, logging, error reporting, table lookup, loop bounds, branches, or public outputs.

For each declassification, state why the value is already public or safe to reveal and whether failure/success itself leaks information. Compare backends, optional modules, VERIFY/exhaustive modes, and compiler output. Narrow overly broad declassification and add ctime/checkmem coverage. Preserve an explicit dataflow proof even when dynamic tools pass.
```

<a id="goal-46"></a>

### 46. Public API output-on-failure audit

<!-- slug: api-output-on-failure; prompt-chars: 3728; utf8-bytes: 3729 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/api-output-on-failure.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For every exported function with output parameters or caller-visible object mutation, document return convention and failure-state contract: unchanged, zeroed, invalidated, partially specified, or guaranteed initialized. Compare header docs, examples, bindings, tests, and implementation.

Exercise malformed inputs, invalid contexts/capabilities, aliasing, invalid tweaks/keys/signatures, callback failure, and module-specific errors. Pre-fill outputs with sentinels and verify exact post-state. Fix implementation only when it violates the supported contract; otherwise make docs/tests explicit. Check that callers never consume unspecified output.
```

<a id="goal-47"></a>

### 47. Build-system and CI parity audit

<!-- slug: build-ci-parity; prompt-chars: 3674; utf8-bytes: 3675 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/build-ci-parity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare CMake, remaining alternate build/package systems, presets, CI setup scripts, install/export manifests, examples, benches, fuzzers, exhaustive/ctime tests, optional modules, and cross-platform jobs. Label native, cross, emulator, and artifact-only evidence.

Find flags or files present in one path but absent in another, tests silently skipped, different defaults, stale generated lists, and package/install omissions. Validate each side in separate clean directories. Fix only parity defects where the project claims equivalent support; document intentional asymmetry and its review precedent.
```

<a id="goal-48"></a>

### 48. Property, exhaustive, and algebraic oracle expansion

<!-- slug: property-oracle-expansion; prompt-chars: 3887; utf8-bytes: 3888 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/property-oracle-expansion.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Review unit, randomized, exhaustive, fuzz, and integration tests for operations that check success but not the strongest relation. Add identities, inverse/replay, idempotence, canonicalization, monotonicity, ordering, failure-no-mutation, and derived-state recomputation over the broadest cheap domain.

For caches, indexes, counters, summaries, and incremental state, build a slower independent truth path and compare after every operation, not only at the end; transient divergence may be the bug window. For secp256k1 use exhaustive groups, Sage-derived relations, modules, and backend comparisons. For Bitcoin use serialization, script/sighash, coins cache, mempool/package, connect/disconnect, indexes, and wallet models. Prove each oracle with a targeted mutation and avoid copying production logic.
```

<a id="goal-49"></a>

### 49. Critical whole-history must-fix sweep

<!-- slug: critical-history-sweep; prompt-chars: 3843; utf8-bytes: 3844 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/critical-history-sweep.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Walk initial commit to HEAD in checkpoints, but rank only reachable critical classes: consensus inflation or permanent split; remote memory corruption/crash; funds, keys, or signing authorization; durable corruption or restart denial; censorship/liveness; then practical default-node resource kills. Do not let already-owned memory-amplification variants dominate the campaign.

For Bitcoin Core use BIP50, official advisories, and the supplied knowledgebase as seed receipts. Ask whether each old failure shape, partial migration, covert fix, backport gap, or review concern survives on current HEAD. Query ownership and negative knowledge first, then prove current reachability and severity independently. Skip cleanup and latent APIs without an affected caller.
```

<a id="goal-50"></a>

### 50. Fuzz Introspector blocker and complexity audit

<!-- slug: fuzz-introspector-blockers; prompt-chars: 3684; utf8-bytes: 3685 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/fuzz-introspector-blockers.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Use Fuzz Introspector or equivalent call-tree analysis to compare static reachable complexity with dynamic coverage for every target. Rank blockers by the amount and risk of code hidden behind them, not merely branch count.

Trace each blocker to input structure, checksum/magic, global state, early validation, polymorphism, environment dependence, or harness construction. Decide whether to add a dictionary/custom mutator, structured input, realistic setup, new target, or no change because validation is the subject. Show before/after reachability, coverage, hit counts, and preserved determinism.
```

<a id="goal-51"></a>

### 51. Invariant, differential, and metamorphic audit

<!-- slug: differential-metamorphic; prompt-chars: 3831; utf8-bytes: 3832 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/differential-metamorphic.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Enumerate equivalent implementations and inverse/state relations: fast/reference, old/new, scalar/SIMD, C/assembly, parser/serializer, incremental/recompute, batch/split, apply/revert, write/recover, iterator directions, sibling libraries, and independently reimplemented references.

Define the shared domain and permitted differences before comparing. Feed identical valid, invalid, boundary, and stateful vectors; normalize outputs and isolate side effects. When neither side is authoritative, use a specification, third implementation, or algebraic oracle. Minimize divergences, trace them to a public caller where possible, identify which implementation or contract is wrong, and retain both the vector and a mutation proving oracle sensitivity.
```

<a id="goal-52"></a>

### 52. Integer overflow, narrowing, signedness, and division audit

<!-- slug: integer-arithmetic-audit; prompt-chars: 3909; utf8-bytes: 3910 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/integer-arithmetic-audit.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Trace externally and internally derived integers into additions, multiplications, shifts, divisions/modulo, negation, casts, allocations, indexes, offsets, time arithmetic, resource accounting, and serialization widths. Search zero divisors, `INT_MIN/-1`, signed overflow, unsigned wrap, truncation, and implementation-defined shifts.

Write the mathematical range and platform assumptions. In an isolated test branch, replace suspicious operations with checked versions and assertions so overflow, underflow, invalid subtraction, impossible time deltas, and narrowing become loud. Start from each dangerous sink, infer its required domain, and propagate it outward through callers to a public input. Test exact boundaries on relevant widths and sanitizers. Keep checked types only when they simplify a proven production defect.
```

<a id="goal-53"></a>

### 53. Statistical timing-side-channel campaign

<!-- slug: timing-side-channel; prompt-chars: 3700; utf8-bytes: 3701 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/timing-side-channel.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select one secret-bearing primitive or API boundary per cycle. Construct two or more input classes differing only in a secret property, randomize execution order, control CPU noise, gather enough samples, and run dudect-style Welch tests plus checkmem/assembly analysis.

Investigate cache, branch predictor, variable-time arithmetic, error exits, allocator, and compiler/architecture effects. Repeat across optimizers and relevant CPUs. Treat a significant result as a lead requiring mechanism proof, and a non-significant result as non-proof. Fix only when secret dataflow and measurable behavior identify a concrete leak.
```

<a id="goal-54"></a>

### 54. RAII, smart-pointer, and resource-leak audit

<!-- slug: raii-resource-leaks; prompt-chars: 3706; utf8-bytes: 3707 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/raii-resource-leaks.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map ownership of heap objects, file/socket handles, locks, transactions, threads, callbacks, scheduler work, iterators, snapshots, and external resources. Search raw-pointer escapes, shared_ptr cycles, reference captures, moved-from misuse, custom deleters, destruction-order assumptions, and early-return leaks.

Use LSan/Valgrind and deterministic lifecycle tests including construction failure, cancellation, shutdown, and restart. Prove the exact ownership cycle or dangling window; do not modernize pointers mechanically. Prefer the smallest RAII/lifetime correction and verify destruction order, callbacks, and thread joins.
```

<a id="goal-55"></a>

### 55. Alternative-implementation compatibility-difference audit

<!-- slug: alternative-implementation-diff; prompt-chars: 3875; utf8-bytes: 3876 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/alternative-implementation-diff.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Mine Bitcoin Knots, btcd, libbitcoin, rust-bitcoin, bitcoinj, gocoin, libwally, OpenSSL/BoringSSL, noble/rust secp libraries, RocksDB/Pebble, and relevant forks for fixes, tests, and reference behavior. Define the local consensus, encoding, crypto, persistence, or recovery contract first.

Do more than compare outputs: transplant distinguishing sibling tests, port a small reference implementation across languages when useful, and rebase relevant alternative patches or test branches onto current HEAD without copying conclusions. Keep adapters thin and provenance exact. Classify every divergence as local bug, sibling bug, intentional policy/API difference, obsolete test, or adapter error. Require a shared vector and public reachability before fixing or preparing a remote report.
```

<a id="goal-56"></a>

### 56. Stale PR critical-fix resurrection audit

<!-- slug: stale-pr-resurrection; prompt-chars: 3715; utf8-bytes: 3716 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/stale-pr-resurrection.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Search closed, abandoned, draft, superseded, and stale open PRs/issues for claimed consensus, funds, remote DoS/crash, wallet/key, data-corruption, crypto, or recovery bugs. Record why each stalled, review objections, proposed tests, and later related work.

Reproduce the claim independently on current HEAD; never resurrect the old patch blindly. If still critical, design the smallest current-style fix and explain how it avoids prior objections. If fixed, noncritical, or under-proven, record decisive evidence. Continue through PR ranges with checkpoints and prioritize unmerged regression tests and credible reproducer discussions.
```

<a id="goal-57"></a>

### 57. Local-reasoning domain and relationship audit

<!-- slug: local-reasoning-contracts; prompt-chars: 3909; utf8-bytes: 3910 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/local-reasoning-contracts.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For one function/class/module per cycle, write its legal input domain, preconditions, postconditions, ownership graph, lock requirements, invalidation rules, failure recovery, and persistence authority. Ask whether a reviewer can verify them locally or must rely on caller folklore.

Create a temporary shadow build that makes hidden assumptions explicit with checks, checked arithmetic, sentinel outputs, and independent recomputation. Target owner/observer, iterator/container, snapshot/database, cache/backend, context/object, callback/session, active-chain/index, and wallet/key relationships. Walk violations through callers to a public boundary and distinguish unreachable internal misuse from real exposure. Commit only concrete invalid-state, lifetime, or recovery defects; keep useful instrumentation as an experiment.
```

<a id="goal-58"></a>

### 58. Exact helper reuse and minimal helper-extension audit

<!-- slug: helper-reuse; prompt-chars: 3741; utf8-bytes: 3742 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/helper-reuse.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
First find duplicate code exactly covered by an existing helper: setup, parsing/serialization, result conversion, cleanup, locking, formatting, fixtures, builders, and assertions. Prove equivalence across inputs, errors, side effects, ownership, locks, diagnostics, and ordering by inlining the helper mentally or mechanically.

Only after exact reuse, consider one minimal parameter/overload/hook that immediately replaces multiple real duplicates or one high-risk block. Reject abstractions that hide case-specific meaning, weaken tests, cross layers, or exist only to reduce lines. Benchmark compile/runtime effects where relevant and remove newly dead code atomically.
```

<a id="goal-59"></a>

### 59. C/C++ supply-chain and security-gate audit

<!-- slug: supply-chain-security-gates; prompt-chars: 3749; utf8-bytes: 3750 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/supply-chain-security-gates.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit vendored/subtree/submodule code, depends manifests, hashes, patches, download URLs, toolchain/container pins, generated sources, CI actions/images, release signing, binary verification, SBOM/provenance, scanner configuration, and workflow permissions.

Trace each trusted artifact or gate from untrusted input to build, test, release, or install decision. Check cache poisoning, untrusted-fork execution, shell/path/env injection, stale allowlists, ignored vendor/generated paths, and vulnerable dependency reachability. Add no security theater: each change must block a demonstrated bad artifact, leaked secret, unsafe workflow, or false verification result.
```

<a id="goal-60"></a>

### 60. Historical reviewer-preference mining and reusable review skill

<!-- slug: reviewer-preference-skill; prompt-chars: 3718; utf8-bytes: 3719 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/reviewer-preference-skill.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Mine diverse merged, closed, abandoned, contentious, and high-impact PRs in each subsystem. Extract actual review comments, maintainer decisions, ACK/NACK rationale, requested evidence, commit-stack preferences, and post-merge follow-ups.

Classify every rule as general, subsystem-specific, reviewer/author-specific, contextual, stale, or one-off taste. Encode durable items as trigger + review question + evidence links + non-goals/counterexamples. Validate against held-out historical PRs and update the journal's reviewer map. Continue until PR ranges and major reviewers are covered, revisiting rules when project practice changes.
```

<a id="goal-61"></a>

### 61. Stateful contract-fuzzer expansion

<!-- slug: stateful-contract-fuzzing; prompt-chars: 3934; utf8-bytes: 3935 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/stateful-contract-fuzzing.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Upgrade one target at a time from no-crash execution to a stateful checker. For Bitcoin Core rotate `process_message(s)`, `deserialize`, `coinscache_sim`, `block_index_tree`, `txgraph`, `txorphan`, `utxo_snapshot`, script/signing, wallet, RPC, and IPC. Read the knowledgebase's restriction inventory before relaxing bounds: many exclusions are cost controls or invalid oracles, while some hide full-range fee, malformed JSON, snapshot, or transport domains.

Add independent recomputation after every operation, rollback/no-mutation checks, inverse/replay, peer-local ownership, exact arithmetic domains, and public-path differential oracles. Temporarily remove catches, `Assume` gates, clamps, and exceptional guards; minimize failures and determine whether the guard encodes a real production precondition. Keep harness and production fixes separate.
```

<a id="goal-62"></a>

### 62. Rejected-finding resurrection and assumption attack

<!-- slug: rejected-finding-resurrection; prompt-chars: 3756; utf8-bytes: 3757 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/rejected-finding-resurrection.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Collect candidates previously dismissed as unreachable, theoretical, test-only, root-only, mitigated, or irrelevant. Treat each dismissal as a falsifiable claim. List its assumptions about bounds, call order, state, configuration, platform, permissions, and trust boundary.

Attack one assumption at a time using widened fuzzing, crafted RPC/config/database/IPC/network fixtures, sanitizer runs, deterministic interleavings, corpus transfer, and historical states. Require a realistic boundary, not an artificial harness alone. Confirm rejection only after documented falsification attempts fail; otherwise fix the proven root cause and state severity without inflation.
```

<a id="goal-63"></a>

### 63. Loupe and Codex Security scout, verifier, fixer, and reporter pipeline

<!-- slug: loupe-style-pipeline; prompt-chars: 3505; utf8-bytes: 3506 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/loupe-style-pipeline.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Run a four-stage Project Loupe-style pipeline, but do not stop at per-file review. Scouts search prior findings, map a whole feature and public path, and produce a runnable regression test, fixture, harness, hostile artifact, or independent reference rather than a patch. A separate verifier must build and run it on clean HEAD, lock confirmed/dismissed/inconclusive, and record public reachability before seeing any fix.

Only confirmed findings reach a fixer; a final reviewer reruns failing-before/passing-after proof and project-precedent checks. Preserve unproven artifacts and negative results for later models, route candidates into specialized reimplementation, sink, conformance, or fault-injection campaigns, and run a composition pass over confirmed findings. Track leases, semantic/hash deduplication, status transitions, and exact PoCs in the journal.

Add repository/feature risk ranking before scouting, an explicit coverage manifest, root-cause continuity across reruns, and an attack-path pass after validation. Preserve a replayable scan recipe and treat incomplete coverage as unknown rather than clean.
```

<a id="goal-64"></a>

### 64. Finding deduplication, recurrence, and semantic-fingerprint audit

<!-- slug: finding-dedup-recurrence; prompt-chars: 3496; utf8-bytes: 3497 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/finding-dedup-recurrence.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Build a durable index of confirmed findings, rejected hypotheses, and unfinished suspicions using code path, trust boundary, bug shape, source-to-sink relation, affected versions, reproducer hash, semantic summary, artifact branch, tool/model version, and exact resume point. Index unfinished fuzzers, seeds, assertions, sanitizer traces, harnesses, and failed attempts, not only polished reports.

Before new work search hashes, symbols, commits, and semantic descriptions. Distinguish duplicate, recurrence, incomplete variant, changed reachability, or useful negative result. Periodically replay the highest-risk unresolved artifacts and old regression inputs with a different or newer model, initially hiding the old verdict where practical. Link rather than restate duplicates and never discard evidence merely because the first model judged it unimportant.

Treat each false-positive reason as a conditional guard or reachability claim. Revalidate it after code, scope, configuration, dependency, or threat-model changes, and track the same root cause as new, persisting, reopened, resolved, or unknown.
```

<a id="goal-65"></a>

### 65. Contributor-branch and work-in-progress radar

<!-- slug: contributor-branch-radar; prompt-chars: 3780; utf8-bytes: 3781 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/contributor-branch-radar.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Identify active contributors from recent PRs, reviews, commits, and subsystem ownership, then fetch their public GitHub branches/forks without altering upstream refs. Inventory branch purpose, base, divergence, touched contracts, tests, benchmark evidence, dependencies, and overlap with local work.

Read associated PRs/issues and record each contributor's style and unresolved review concerns. Look for upcoming migrations, fixes, optimizations, test ideas, and conflicting assumptions that should inform local plans. Never copy unpublished work without provenance. Flag branches containing independently reproducible bugs, useful seeds, or likely merge conflicts, and refresh the radar over time.
```

<a id="goal-66"></a>

### 66. Cherry-pick, backport, and release-branch correctness audit

<!-- slug: backport-correctness; prompt-chars: 3849; utf8-bytes: 3850 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/backport-correctness.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit security and correctness backports across maintained release branches and downstream forks. For Bitcoin Core build a ledger from official advisories, security-fix merges, follow-up fixes, and the supplied knowledgebase's known release gaps. Check prerequisites, covert-fix carriers, conflict resolution, generated files, version guards, test availability, and whether a later fix repaired the first fix.

Reproduce the original bug or a safe historical mutant on each claimed affected/fixed branch; compare behavior, not commit hashes. Use patch-id and range-diff only for routing. Verify upgrade/downgrade and old-format compatibility where persistence or wallet state is involved. Report missing or incorrect backports privately when disclosure timing requires it.
```

<a id="goal-67"></a>

### 67. Release-to-release behavioral and consensus differential

<!-- slug: release-version-differential; prompt-chars: 3713; utf8-bytes: 3714 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/release-version-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select adjacent and strategically distant released versions plus current HEAD. Feed identical blocks, transactions, scripts, RPC/config cases, wallets, databases, indexes, and network transcripts, normalizing intentionally changed output.

Use release notes, BIPs, and migration code to classify expected differences before judging them. Focus on undocumented consensus/validation drift, acceptance/rejection changes, state serialization, recovery, error semantics, and performance cliffs. Bisect unexpected divergence, test upgrade/downgrade/restart, and preserve portable fixtures. Continue across version pairs with a ledger.
```

<a id="goal-68"></a>

### 68. Architecture, endianness, word-size, and ABI parity audit

<!-- slug: architecture-abi-parity; prompt-chars: 3705; utf8-bytes: 3706 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/architecture-abi-parity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run or cross/emulate relevant x86_64, arm64, arm32, i686, big-endian, Windows, macOS, Linux, and FreeBSD configurations. Compare 32/64-bit widths, char signedness, alignment, packing, endian conversions, atomics, filesystem/socket APIs, time types, and serialized outputs.

Use QEMU or supported CI images where native hardware is unavailable and distinguish compile-only from executed evidence. Feed identical deterministic vectors and compare results, sanitizer traces, and performance where meaningful. Find architecture-specific UB, truncation, unaligned access, stale assembly selection, and platform-only skipped tests.
```

<a id="goal-69"></a>

### 69. SIMD, assembly, and portable-reference backend differential

<!-- slug: backend-differential; prompt-chars: 3706; utf8-bytes: 3707 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/backend-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory scalar, SIMD, assembly, hardware-accelerated, and reference implementations for hashes, crypto, field/scalar/group math, checksums, memory operations, and codecs. Force each backend independently and feed identical boundary, random, and malformed inputs.

Compare exact outputs, error behavior, aliasing support, state mutation, constant-time expectations, and architecture feature detection. Run sanitizers where possible and inspect fallback behavior on unsupported CPUs. Benchmark only after correctness. Minimize divergences and determine whether optimized code, reference code, dispatcher, or test oracle is wrong.
```

<a id="goal-70"></a>

### 70. Compiler, optimization, LTO, PGO, and BOLT differential

<!-- slug: compiler-optimization-differential; prompt-chars: 3740; utf8-bytes: 3741 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/compiler-optimization-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build with supported GCC/Clang versions at `-O0/-O1/-O2/-O3/-Os`, debug/release, LTO, PGO/SamplePGO, and BOLT where practical. Run identical deterministic correctness suites and representative workloads; compare outputs before treating speed as evidence.

Look for UB exposed only under optimization, miscompiles, missing barriers/cleanses, altered constant-time behavior, profile instability, and code-size/startup/cache tradeoffs. Use assembly/IR, perf counters, and compiler reducers to isolate anomalies. Adopt PGO/BOLT only with reproducible training workloads, held-out validation, measured gains, and no correctness or portability regression.
```

<a id="goal-71"></a>

### 71. Deterministic simulation and failure-schedule exploration

<!-- slug: deterministic-simulation; prompt-chars: 3862; utf8-bytes: 3863 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/deterministic-simulation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build or extend a deterministic scheduler/environment around one stateful subsystem: seeded wall and monotonic time, time jumps/drift, randomness, network/disk outcomes, task ordering, retries, shutdown, restart, and resource failures. Run production logic through interchangeable test interfaces where feasible.

Define invariants over intermediate and final state, progress, durability, and resource bounds. Generate aggressive reproducible schedules, including combinations rather than one fault at a time; record every choice, shrink failures, and replay seeds exactly. Recompute authoritative state after each step and seek a public scenario for failures. Avoid a fake parallel implementation that bypasses production logic. Each cycle adds a workload, fault class, or oracle.
```

<a id="goal-72"></a>

### 72. Filesystem, power-loss, and crash-consistency injection

<!-- slug: filesystem-crash-consistency; prompt-chars: 3834; utf8-bytes: 3835 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/filesystem-crash-consistency.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map durable boundaries for Core chainstate, block/undo files, block index, BaseIndex subclasses, wallets, settings, mempool/fee-estimator state, peers, and snapshots. Seed schedules from dirty undo files, metadata-after-flush failures, retryable batches that forget dirty entries, prune locks ahead of committed locators, wallet memory/disk divergence, and interrupted snapshot cleanup.

Inject short writes, EIO/ENOSPC, failed seek/truncate, dropped fsync, rename collision, corruption, and power loss before/after every commit point. Restart repeatedly and compare with an uninterrupted control. Require exact recovery or a controlled actionable failure, never a crash loop, silent stale data, false progress, or destruction of the last good copy.
```

<a id="goal-73"></a>

### 73. Network fragmentation, reordering, and partial-I/O state-machine audit

<!-- slug: network-state-machine; prompt-chars: 3789; utf8-bytes: 3790 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/network-state-machine.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Exercise transports and protocol parsers with fragmented/coalesced reads, short writes, EOF at every byte, delayed or reordered messages where permitted, duplicates, reconnects, half-close, backpressure, address-family differences, and zero/maximum payloads.

Force the agent to build a deterministic hostile peer or client that drives the real public protocol, rather than only describing malformed traffic. Model handshake and peer states explicitly; check memory/accounting, timeout resets, permissions, framing, cleanup, and no work after disconnect. Compare legacy/new transports and OS behavior, combine with assertions and sanitizers, and preserve minimized reusable transcripts and the adversarial tool.
```

<a id="goal-74"></a>

### 74. Memory pressure, OOM, allocator, and fragmentation audit

<!-- slug: memory-pressure-allocator; prompt-chars: 3730; utf8-bytes: 3731 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/memory-pressure-allocator.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Profile steady-state and peak heap, allocation counts/sizes/lifetimes, fragmentation, caches, arenas, stack use, and retained capacity under sync, reindex, mempool, wallet, RPC, fuzz, and adversarial inputs. Vary allocator, memory limit, and thread count.

Inject allocation failure where contracts permit, plus realistic cgroup/RLIMIT pressure. Check overflow before allocation, graceful failure, cleanup, cache accounting, retry storms, and whether memory returns after workload. Distinguish allocator behavior from leaks. Optimize only measured hot allocation patterns or proven excessive retention, with RSS/heap profiles and correctness tests.
```

<a id="goal-75"></a>

### 75. Build throughput, dependency graph, and container-cache audit

<!-- slug: build-throughput-cacheability; prompt-chars: 3756; utf8-bytes: 3757 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/build-throughput-cacheability.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Measure clean, incremental, no-op, and parallel builds with Ninja/CMake timing, compiler traces, header/include cost, generated steps, linker time, Docker layer reuse, and CI cache hit/miss behavior. Find unnecessary rebuild fan-out, unstable generated files, broad headers, serialized custom commands, poor job pools, and cache keys tied to irrelevant inputs.

Make the smallest dependency/build-script/container change. Prove no missing dependency with clean and randomized parallel builds, and no stale result after touching each true input. Report wall/CPU/RSS/cache-size effects over repeated runs. Do not trade correctness or developer clarity for tiny build wins.
```

<a id="goal-76"></a>

### 76. Reproducible binaries, Guix, and toolchain-provenance audit

<!-- slug: reproducible-builds; prompt-chars: 3710; utf8-bytes: 3711 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/reproducible-builds.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Rebuild release artifacts from clean environments, across supported hosts/architectures where documented, using pinned dependencies and Bitcoin Core Guix/depends or the project's equivalent. Compare hashes and use diffoscope-style analysis for differences.

Trace timestamps, paths, locale, ordering, archive metadata, toolchain drift, generated files, signing, and host contamination. Verify dependency hashes and source provenance, and distinguish reproducible unsigned payloads from signatures/packaging. Fix the narrow source of nondeterminism and rerun independently. Record exact toolchain/container commits and artifact hashes.
```

<a id="goal-77"></a>

### 77. Symbolic execution and bounded-model-checking campaign

<!-- slug: symbolic-model-checking; prompt-chars: 3706; utf8-bytes: 3707 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/symbolic-model-checking.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select small, high-risk pure or state-machine kernels with bounded inputs: arithmetic, parsers, encoders, cache transitions, queues, crypto helpers, and failure cleanup. Build focused CBMC or KLEE harnesses with explicit assumptions matching production domains.

Assert memory safety, no division/shift UB, postconditions, output-on-failure, algebraic identities, and equivalence to a reference. Treat bounds and environment stubs as part of the proof and attack them with concrete tests. Convert counterexamples into regression vectors. Never claim an unbounded proof; document unwind completeness and unsupported constructs.
```

<a id="goal-78"></a>

### 78. Compiler-transformation validation and miscompile isolation

<!-- slug: translation-validation; prompt-chars: 3758; utf8-bytes: 3759 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/translation-validation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For suspicious optimization-dependent behavior or critical arithmetic, capture pre/post LLVM IR and use Alive2 where supported to check refinement. Also compare GCC/Clang versions and optimization levels on deterministic vectors, especially code with overflow assumptions, aliasing, shifts, bit tricks, and constant-time masking.

If validation is inconclusive, reduce the function and use differential execution or generated UB-free cases. Determine whether source UB, compiler bug, inline assembly contract, or test error is responsible. Fix local UB rather than coding around a compiler unless project support requires it; produce a compiler-report reproducer for remote bugs.
```

<a id="goal-79"></a>

### 79. Fuzz-corpus stewardship, minimization, and transfer audit

<!-- slug: fuzz-corpus-stewardship; prompt-chars: 3705; utf8-bytes: 3706 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/fuzz-corpus-stewardship.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inventory corpora by target, source commit, coverage, size, runtime, flakiness, and sanitizer dependence. Merge and minimize with exact target/build versions; remove true duplicates without losing features, and identify oversized seeds that dominate execution.

Cross-seed structurally related targets and import public qa-assets/bitcoinfuzz corpora with provenance. Re-run old crashers and regression inputs on current HEAD. Preserve inputs that add stable coverage or encode important semantics, not random bulk. Track corpus coverage/time trends and submit project-appropriate improvements with deterministic reproduction.
```

<a id="goal-80"></a>

### 80. Fuzz-engine and property-framework differential

<!-- slug: fuzz-engine-differential; prompt-chars: 3716; utf8-bytes: 3717 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/fuzz-engine-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run selected targets under libFuzzer, AFL++, Honggfuzz, and a property framework such as FuzzTest where integration is practical. Keep target semantics and initial corpus comparable while using each engine's strengths: dictionaries, value profiles, CMP tracing, custom mutators, parallelism, and high-throughput modes.

Compare coverage growth, unique paths, crash classes, execution rate, memory, and corpus quality over fixed CPU budgets and repeated seeds. Transfer discoveries between engines and reproduce all failures in a sanitizer build. Change harnesses only for engine-neutral realism unless a documented adapter is required.
```

<a id="goal-81"></a>

### 81. Specification, test-vector, and formal-model drift audit

<!-- slug: spec-vector-drift; prompt-chars: 3885; utf8-bytes: 3886 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/spec-vector-drift.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map code and tests to BIPs, protocol documents, secp256k1 module docs, Sage scripts, Wycheproof vectors, official conformance suites, and other authoritative specifications. Pin versions and mark ambiguous, policy-specific, or intentionally divergent areas. Preserve primary sources resolved through any knowledge index.

Turn outside knowledge into executable oracles rather than prose context: derive tests, regenerate vectors, or implement a small independent reference. Exercise valid, invalid, edge, and historical cases; search for rules implemented but untested and tests copied from obsolete drafts. When code, tests, and implementations disagree, derive the result from the exact rule and minimize a public vector. Update vectors/tests or code only with traceable provenance and compatibility analysis.
```

<a id="goal-82"></a>

### 82. secp256k1 field and scalar representation matrix

<!-- slug: secp-field-scalar-matrix; prompt-chars: 3731; utf8-bytes: 3732 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secp-field-scalar-matrix.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
In libsecp256k1, compare 5x52 versus 10x26 field code, 4x64 versus 8x32 scalar code, normal versus VERIFY, exhaustive groups, inversion variants, and supported compiler/architecture paths. Extract magnitude, normalization, limb, carry, overflow, aliasing, and input-domain contracts from headers and verification code.

Generate boundary elements at every allowed magnitude and scalar edge; run add/mul/sqr/negate/inverse/normalize/serialize relations and cross-backend exact comparisons. Inspect 32-bit arithmetic carefully. Use exhaustive/random/property tests, UBSan, optimized assembly, and temporary bound violations to prove oracle sensitivity.
```

<a id="goal-83"></a>

### 83. secp256k1 group, ecmult, and formula-parity audit

<!-- slug: secp-group-ecmult; prompt-chars: 3712; utf8-bytes: 3713 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secp-group-ecmult.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit affine/Jacobian conversions, addition/doubling, infinity handling, wNAF/window tables, generator and arbitrary-point multiplication, endomorphism paths, batch inversion, and exhaustive-group formulas. State input domains and exceptional cases for each helper.

Compare optimized formulas with a simple reference and exhaustive small-group results across window sizes, backends, and VERIFY builds. Test aliasing and malformed internal states only where contracts define them. Search Sage derivations and historical formula fixes for variants. Benchmark changes only after algebraic parity and constant-time classification are proven.
```

<a id="goal-84"></a>

### 84. secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit

<!-- slug: secp-nonce-session; prompt-chars: 3715; utf8-bytes: 3716 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/secp-nonce-session.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model ECDSA/Schnorr signing, nonce generation, key tweaks, ECDH, extrakeys, and MuSig nonce/session/partial-signature transitions. Label secret/public inputs, single-use state, commitment binding, context capability, callback failure, and output-on-failure guarantees.

Test invalid order, reuse, duplicate participant/key, zero/overflow scalar, malformed serialization, cancellation, randomized context, and deterministic replay. Cross-check formal equations and other implementations without treating them as oracle. Run exhaustive/module/ctime/checkmem tests, preserve minimal sequences, and prioritize nonce reuse or partial-state bugs.
```

<a id="goal-85"></a>

### 85. Bitcoin consensus mutation-score and kill-test audit

<!-- slug: bitcoin-consensus-mutation; prompt-chars: 3854; utf8-bytes: 3855 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-consensus-mutation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Target consensus validation, contextual activation, serialization, script/sighash, UTXO transitions, and reusable validation caches. Seed mutations from BIP50's environment-dependent acceptance, CVE-2018-17144's skipped duplicate-input check, activation boundaries, amount/subsidy conservation, Signet authorization, and cached-validity provenance across mutable objects or network contexts.

Invert/remove one check or cache-key component at a time; perturb heights, time, flags, witness commitments, sigops, lock rules, and connect/disconnect state. Run unit, functional, fuzz, kernel, cross-release, architecture, and independent-node vectors. A surviving non-equivalent mutant is a critical oracle gap. Stop before committing any consensus-observable behavior change.
```

<a id="goal-86"></a>

### 86. Bitcoin chainstate, reorg, prune, and index crash-symmetry audit

<!-- slug: bitcoin-chainstate-symmetry; prompt-chars: 3828; utf8-bytes: 3829 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-chainstate-symmetry.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model UTXO and chainstate as a transaction: connect removes each spent prevout once, adds each spendable output once, and disconnect restores the exact prior state. Track `FRESH`/`DIRTY`, memory usage, best block, undo positions, candidate/unlinked sets, flush tips, prune locks, index locators, and snapshot/background chainstates.

Generate duplicate/same-block spends, failed `ConnectBlock`, same- and unequal-height reorgs, pure disconnects, prune/redownload, stale children, malformed/corrupt undo, retryable write failures, and crashes at every publication boundary. Compare incremental state with a slow reference and uninterrupted replay after each step. Invalid work must leave the parent view and unrelated peer/index state unchanged.
```

<a id="goal-87"></a>

### 87. Bitcoin mempool, package, and eviction-accounting audit

<!-- slug: bitcoin-mempool-accounting; prompt-chars: 3826; utf8-bytes: 3827 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-mempool-accounting.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model acceptance, orphan resolution, transaction requests, packages, replacement, clusters, fee deltas, expiry, trimming, block removal, and reorg reinsertion. Seed from historical orphan quadratic work and transaction-request censorship plus the knowledgebase's modified-fee, TxGraph, MiniMiner, relay-role, and stale-peer families.

After every operation recompute graph links, request ownership/timers, fee diagrams, memory/accounting, eviction protection, and accepted/rejected state from an independent model. Exercise extreme representable fees, overlapping packages, missing parents, peer disconnect/reconnect, `relay=0`, failures, and reorgs. Exact-ratio arithmetic must not be "fixed" by saturation without proving downstream algebra.
```

<a id="goal-88"></a>

### 88. Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit

<!-- slug: bitcoin-wallet-recovery; prompt-chars: 3859; utf8-bytes: 3860 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-wallet-recovery.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map wallet authority and durability across encryption/passphrase changes, descriptors/keypool, address reservation, PSBT/MuSig2, spend construction, external signers, rescans, migration, export, backup, and restore. Treat the displayed transaction and user-approved intent as authoritative: an external signer may add signatures but must not substitute destinations, amounts, fees, inputs, or change unnoticed.

Inject database begin/write/commit/rewrite failure and crashes before memory adoption. Test malformed/counterparty PSBT metadata, callback reentrancy, signer failure, unload/reload, and reorg. Assert no mixed encrypted/plaintext state, missing master key, memory-only success, key/address reuse, falsely complete PSBT, unauthorized transaction, or lost recovery path.
```

<a id="goal-89"></a>

### 89. Bitcoin P2P transport, permission, and peer-accounting audit

<!-- slug: bitcoin-p2p-accounting; prompt-chars: 3866; utf8-bytes: 3867 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bitcoin-p2p-accounting.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model peer-local and global state across v1/v2 transport, handshake, permissions, headers sync, block/tx download, compact blocks, orphan/request tracking, AddrMan, discouragement, proxies, and disconnect. Seed from incomplete-message buffers, headers spam, adjusted-time netsplit, addr overflow, GETDATA/INV loops, orphan stalls, tx-request censorship, compact-block assertion reuse, mutated/stalling blocks, and cross-peer state clearing.

Prioritize fatal assertions, lifetime bugs, censorship, and state corruption over already-owned amplification variants. Generate fragmented/reordered/duplicate messages, role changes, stalls, reconnects, and shutdown. Assert one peer cannot mutate another's ownership, timers, queues, or reconstruction state and honest progress remains bounded.
```

<a id="goal-90"></a>

### 90. Whole-PR, commit, and external knowledge-base recipe synthesis

<!-- slug: historical-knowledge-recipes; prompt-chars: 3474; utf8-bytes: 3475 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/historical-knowledge-recipes.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Progress through every commit and PR in recorded ranges and extract reusable technical knowledge, not reviewer taste alone. Every reviewed commit must leave proof of work: intended behavior, changed invariant, callers and files inspected, missing test/oracle, sibling sites, rejected design, benchmark or fixture, platform caveat, follow-up, and verdict. A bare summary or "looks fine" is incomplete.

Store concise recipes keyed by subsystem and trigger: when a future change touches X, inspect Y, run Z, and avoid W. Link primary evidence, mark stale/version-limited rules, and attach runnable artifacts where possible. Validate recipes on held-out PRs by checking whether they recover real historical comments or bugs. Record exact commit/range checkpoints, deduplicate, and revise rather than restarting or appending folklore.

Also ingest versioned architecture, policy, threat-model, and specification documents. Convert useful claims into executable tests, reference models, or review triggers, and preserve ambiguity as competing hypotheses rather than silently choosing one.
```

<a id="goal-91"></a>

### 91. Compiler and binary-hardening configuration audit

<!-- slug: compiler-binary-hardening; prompt-chars: 3768; utf8-bytes: 3769 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/compiler-binary-hardening.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit supported release and developer builds for warnings-as-errors policy, stack protection, FORTIFY, PIE/RELRO/NOW, CFI, SafeStack or platform equivalents, control-flow protections, `_GLIBCXX_ASSERTIONS`, hardened libc modes, integer/implicit-conversion sanitizers, and linker diagnostics.

For each missing or disabled mechanism, determine threat model, platform support, performance/size impact, dependency compatibility, and whether it catches a concrete project-relevant mutation or fixture. Inspect final binaries, not just flags. Add no checkbox hardening: require a demonstrated failure blocked or diagnostic gained, plus build/test/benchmark evidence across supported targets.
```

<a id="goal-92"></a>

### 92. ABI layout, alignment, aliasing, and object-lifetime audit

<!-- slug: abi-alignment-aliasing; prompt-chars: 3986; utf8-bytes: 3987 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/abi-alignment-aliasing.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Search packed structs, unions, reinterpret/static casts, placement new, memcpy of nontrivial objects, over-aligned types, custom allocators, spans over raw storage, strict-aliasing assumptions, pointer provenance, lifetime extension, and C/C++ ABI boundaries.

Compare sizes/offsets/alignment under compilers, architectures, optimization, sanitizers, and shared/static builds. Exercise unaligned buffers and aliasing permutations without invoking invalid inputs outside the contract. Use TypeSanitizer/UBSan, assembly, and small layout tests. Fix concrete UB or ABI mismatch, avoiding broad wrapper churn.

For secp256k1 preallocated contexts, guard exact-size buffers with canaries and count allocations. Exercise heap/preallocated clone chains and both valid destruction orders. Verify callbacks, SHA overrides, and randomization remain independent. Exclude wrong-destructor calls documented as undefined.
```

<a id="goal-93"></a>

### 93. Allocation, syscall, clock, randomness, and callback fault injection

<!-- slug: system-fault-injection; prompt-chars: 3893; utf8-bytes: 3894 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/system-fault-injection.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Create narrow deterministic hooks or wrappers to fail allocations, opens, reads/writes, fsync/rename, socket operations, thread creation, entropy, wall/monotonic clocks, scheduler callbacks, database writes, and user callbacks at the Nth operation. Include backward/forward time jumps, drift, repeated values, and short or partial success where APIs permit.

Use production and preferably public call paths with scratch resources. Sweep failure points and assert rollback, cleanup, retry bounds, diagnostics, intermediate state, and restart behavior. Minimize schedules, combine related failures, and determine whether the condition is cosmetic, locally recoverable, data-corrupting, or externally exploitable. Keep hooks test-only or aligned with existing infrastructure and turn exposed paths into durable tests.
```

<a id="goal-94"></a>

### 94. Bindings, FFI, and language-wrapper parity audit

<!-- slug: bindings-ffi-parity; prompt-chars: 3706; utf8-bytes: 3707 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bindings-ffi-parity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare C/C++ public APIs with maintained Rust, Python, Java, Go, C#, JNI, or other bindings used in the ecosystem. Audit widths, signedness, ownership, lifetimes, nullability, callbacks, exceptions/status mapping, thread safety, buffer lengths, secret cleanup, feature flags, and output-on-failure.

Build shared vectors and misuse cases, including 32-bit and malformed input. Determine whether divergence is in core, wrapper, generated bindings, or documentation. Do not change core to accommodate a broken wrapper unless the core contract is genuinely unsafe. Produce remote report-ready reproductions for binding-only defects.
```

<a id="goal-95"></a>

### 95. Database-engine and persistence-semantics differential

<!-- slug: database-semantics-differential; prompt-chars: 3775; utf8-bytes: 3776 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/database-semantics-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Compare the project's LevelDB usage and assumptions with upstream LevelDB history plus RocksDB, Pebble, or alternative backends as bug seeds. Focus on comparator ordering/stability, snapshots, iterators, batches, WAL/MANIFEST recovery, checksums, filters, compaction boundaries, deletes/overwrites, sync semantics, and corruption handling.

Build engine-neutral operation traces and crash/corruption fixtures, then state allowed implementation differences. Verify Bitcoin wrappers do not rely on undocumented backend behavior. If divergence proves a local wrapper/assumption bug, fix it; if another engine is wrong, document it separately. Measure performance only after semantic parity.
```

<a id="goal-96"></a>

### 96. TODO, FIXME, stub, and deferred-work challenge audit

<!-- slug: todo-deferred-work; prompt-chars: 3863; utf8-bytes: 3864 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/todo-deferred-work.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Enumerate TODO/FIXME/XXX, disabled tests, expected failures, unimplemented branches, temporary compatibility code, placeholder returns, sanitizer suppressions, skipped targets, magic fuzzer values, exceptional-case catches, and comments promising future cleanup. Link each to its origin and recover why the project treated it as unusual.

Turn every item into a falsifiable current question: hidden bug, missing coverage, obsolete workaround, blocked design, valid precondition, or safe intentional debt. Explore neighboring values and sibling sites, search whether later work solved it elsewhere, and preserve partial experiments even without a finding. Fix only concrete current defects or demonstrably stale exceptions; otherwise record exact blockers and the next runnable experiment.
```

<a id="goal-97"></a>

### 97. C and C++ defect-taxonomy sweep

<!-- slug: cpp-defect-taxonomy; prompt-chars: 3799; utf8-bytes: 3800 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cpp-defect-taxonomy.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Cycle systematically through well-known defect classes so attractive areas do not crowd out basic bugs: null dereference, division/modulo by zero, use-before-init, use-after-free/move, double free, invalid destruction order, dangling view/reference/iterator, out-of-bounds, signed/unsigned wrap, shift UB, strict aliasing, data race, deadlock, missed virtual destruction, exception/error leaks, recursion/stack exhaustion, format mismatch, and unchecked result.

For each class, combine semantic search, compiler/tool diagnostics, historical examples, and boundary tests. Trace real reachability and reject pattern-only matches. Maintain a class-by-subsystem coverage grid and continue with the highest-risk unchecked cell.
```

<a id="goal-98"></a>

### 98. Floating-point edge values, sanitizer resurrection, and fuzz-exclusion audit

<!-- slug: float-sanitizer-fuzz-exclusions; prompt-chars: 3971; utf8-bytes: 3972 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/float-sanitizer-fuzz-exclusions.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Cycle through three linked passes.

1. Floating-point edge values. Inventory float/double inputs and conversions in production APIs, RPC/JSON/config parsing, GUI, bindings, tests, benches, and tools. Call them with `+0`, `-0`, subnormal bounds, values adjacent to checked boundaries, max finite, infinities, quiet/signaling NaNs, varied NaN payloads, and decimal overflow/underflow. Check comparisons, integer casts, ordering/hashing, serialization, formatting, clamping, division, and containers across relevant compilers, rounding modes, and optimizations. Prove whether each value must be rejected, normalized, propagated, or ignored; never introduce floating point into consensus or secret crypto.

2. Sanitizer resurrection. Re-enable one `no_sanitize`, suppression, excluded target, skipped platform/CI job, recover mode, or omitted category and save its untouched log. On a throwaway branch let one agent patch every report. An independent verifier gets only the log/diff and keeps fixes with reproduced correctness evidence. Classify the rest and retain justified suppressions. Never broaden an allowlist.

3. Fuzzer exclusions. Inspect catches, ignored errors, early returns, clamps, `Assume` gates, magic values, and exceptional-value cases. Temporarily remove or invert one at a time and explore neighboring inputs. Preserve and minimize failures, then reproduce through the real public boundary. Classify valid precondition, harness bug, expected exception, or production defect, and assert safe output/state on legitimate failure. Never commit blind guard removal.
```

<a id="goal-99"></a>

### 99. Clean-room reimplementation and executable differential audit

<!-- slug: clean-room-reimplementation; prompt-chars: 3954; utf8-bytes: 3955 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/clean-room-reimplementation.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Choose one security-relevant feature with observable behavior and a bounded domain. One agent writes a precise behavioral specification from public docs, tests, history, callers, and black-box traces, then freezes the spec and vectors. Create a `.git`-free source export at the pinned revision, replace only the selected implementation body with a failing stub, and remove build outputs, patches, caches, and other copies of the old body. Give a second agent only that export, frozen spec/vectors, public interface, build/test commands, and permitted dependencies. Do not expose the original checkout, history, network search, old body, diff, blame, or prior verdicts; audit its commands for leakage. It builds a production-compatible replacement or a small independent reference when a full replacement is impractical.

Compile and run original and replacement against identical valid, invalid, boundary, stateful, restart, and failure vectors. Compare outputs, errors, side effects, state transitions, and resource use, using a third oracle or specification when neither side is authoritative. Good first slices have a narrow public seam and existing tests, such as compact-block reconstruction, snapshot metadata parsing, descriptor checksums, or an isolated secp256k1 module operation. The required result is runnable code and a differential harness, not a prose review. Minimize every divergence, trace it to a public caller, classify which contract or implementation is wrong, and preserve the spec, replacement, and vectors even when no defect is confirmed.
```

<a id="goal-100"></a>

### 100. Dangerous-sink reverse reachability and public attack synthesis

<!-- slug: sink-reverse-reachability; prompt-chars: 3291; utf8-bytes: 3292 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/sink-reverse-reachability.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Start from dangerous operations rather than public inputs: division/modulo, dereference, indexing, shifts, narrowing, allocation sizes, assertions, throws, parser assumptions, durable writes, and irreversible state changes. For each sink, state the exact values or state that make it fail or become unsafe and add a temporary assertion or checked wrapper.

Walk the caller graph upward. At each layer translate the sink condition into requirements on that caller until reaching network, RPC, config, file, wallet, library, or other public input, or a proven guard. Synthesize the smallest input and operation sequence satisfying every step, and record barriers where synthesis fails. Confirm through the real public path under assertions/sanitizers. Classify unreachable latent defects separately from exploitable or user-triggerable behavior; fix only after the complete source-to-sink chain is proven.
```

<a id="goal-101"></a>

### 101. Public-boundary characterization and minimal-fix sequencing

<!-- slug: public-characterization-fix; prompt-chars: 3293; utf8-bytes: 3294 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/public-characterization-fix.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
For a suspicious internal behavior, first determine whether real callers use it. Prefer multiple reproductions, including a functional or integration test through the public API, protocol, CLI/RPC, file format, or restart path. If no public route exists, prove the helper is unreachable or document the missing link rather than claiming impact.

Use a two-step stack where practical. The first commit changes no production code and adds a passing characterization test that records the current behavior, its public trigger, and why it is undesirable or risky; also preserve a command or alternate expected result showing the desired behavior fails on clean HEAD. The second commit makes the smallest production change and updates only the affected expectation. Build and test every commit independently, keep unrelated cleanup out, and retain lower-level tests only when they prove a distinct mechanism.
```

<a id="goal-102"></a>

### 102. Durable suspicion artifacts and cross-model replay

<!-- slug: durable-suspicion-replay; prompt-chars: 3260; utf8-bytes: 3261 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/durable-suspicion-replay.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Treat unfinished work as a first-class output. Preserve WIP branches, harnesses, fuzzers, corpora, minimized seeds, temporary assertions, sanitizer traces, coverage maps, failed hypotheses, odd observations, tool/model versions, exact commands, and the next experiment even when the current model thinks the lead is weak.

Maintain a machine-readable suspicion index linking every artifact to code revision, trust boundary, status, confidence, blockers, and resume point. Periodically give the highest-risk unresolved artifacts to a different or newer model, initially withholding the old verdict where practical, and require it to rerun rather than merely reread the conclusion. Compare outcomes, promote confirmed defects, retain useful negative results, and remove only artifacts proven redundant. Never let a model's refusal or low confidence erase executable evidence.
```

<a id="goal-103"></a>

### 103. Finding composition and end-to-end exploit-chain synthesis

<!-- slug: finding-composition; prompt-chars: 3261; utf8-bytes: 3262 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/finding-composition.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Convert every confirmed and strong inconclusive finding into a node describing preconditions, attacker control, privileges, persistence, outputs, side effects, and capabilities gained. Search for chains where one finding's effect satisfies another's precondition, including validation bypass, stale state, resource exhaustion, information disclosure, corruption, retry behavior, and recovery weaknesses.

Rank paths ending in consensus divergence, key/funds/privacy loss, durable corruption, privilege gain, or remote denial of service. Build one end-to-end public reproducer that demonstrates the chain and compare it with isolated controls for each component. Do not inflate severity from a hypothetical graph: record the exact broken edge when a chain cannot be realized. Preserve useful partial chains and identify the smallest independent fixes or mitigations that cut them.
```

<a id="goal-104"></a>

### 104. Analogical vulnerability translation and target-domain search

<!-- slug: analogical-vulnerability-translation; prompt-chars: 3918; utf8-bytes: 3919 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/analogical-vulnerability-translation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Take a proven vulnerability from this or another project and remove language- or product-specific details. Express the underlying failure as trust-boundary crossing, interpreter confusion, stale authority, unchecked size, partial commit, weak canonicalization, lifetime error, resource asymmetry, or another reusable shape.

Force the agent to generate several concrete analogies in the target domain across network, script, RPC/config, files/databases, wallet, crypto, build, and bindings. Rank them by public reachability and impact, then implement a harness or input for the best candidate. The analogy itself is never a finding: trace actual code, existing guards, and affected callers, minimize a runnable reproducer, and record why each rejected mapping fails. Add successful abstractions to the shared bug-shape index.
```

<a id="goal-105"></a>

### 105. Project vulnerability autopsy and author-feature recurrence mining

<!-- slug: project-bug-autopsy-recurrence; prompt-chars: 3257; utf8-bytes: 3258 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/project-bug-autopsy-recurrence.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
For each known bug or vulnerability in the project, reconstruct the introducing commit, intended change, failed assumption, missing validation or oracle, why tests/review missed it, how long it survived, and what finally exposed it. Produce an autopsy artifact before searching for variants.

Use the result to build project-specific priors. Inspect related commits and series by the introducing author, semantic siblings, and the surrounding feature or subsystem, because the same habits or weak assumptions may recur. Blame and feature ownership are deliberate ranking signals, not findings or reasons to soften the search. Each candidate still requires present-day reachability, a matched defect shape, and independent reproduction. Track which authors, features, and missing-oracle classes actually yield confirmed siblings so future ranking is evidence based.
```

<a id="goal-106"></a>

### 106. Semantic-twin inconsistency and sloppiness-map audit

<!-- slug: semantic-twin-inconsistency; prompt-chars: 3911; utf8-bytes: 3912 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/semantic-twin-inconsistency.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Cluster code that expresses the same concept even when names, containers, types, files, or layers differ. Build a contract matrix for empty/null values, duplicates, ordering, normalization, bounds, ownership, error behavior, output mutation, locking, persistence, and restart semantics.

Surface places where one twin validates, clamps, clears, rejects, or rolls back while another does not. Use history and public contracts to decide whether the difference is intentional, a migration artifact, or evidence that a safety rule was copied only partially. Create shared vectors or parallel public tests that exercise both paths. Treat inconsistency as a high-value lead rather than automatic proof, then trace real callers and fix one demonstrated contract mismatch at a time. Maintain a sloppiness map to prioritize nearby code.
```

<a id="goal-107"></a>

### 107. External conformance-suite and sibling-test transplantation

<!-- slug: conformance-test-transplant; prompt-chars: 3927; utf8-bytes: 3928 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/conformance-test-transplant.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Identify protocols, formats, cryptographic operations, databases, or APIs with an official conformance suite, reference vectors, or well-tested sibling implementation. Pin the exact source/version/license and transplant one case family through the target's public interface with the thinnest possible adapter. For Bitcoin Core, start with shared consensus, script, transaction, block, compact-block, or P2P cases from Bitcoin Knots, btcd, bcoin, rust-bitcoin, or bitcoinfuzz and route them through the kernel `btck_script_pubkey_verify` API, `testmempoolaccept`, `submitblock`, or the production P2P parser. Pin activation height, script flags, and consensus-versus-policy scope. For libsecp256k1, start with BIP vectors, Wycheproof, or a sibling binding/library and route them through the public C API while pinning strict versus lax DER, low-S policy, module set, and callback behavior. Also port distinguishing tests or relevant test-only branches from forks onto current HEAD.

Do not rewrite expected behavior to make tests pass. Classify every mismatch as target defect, sibling defect, intentional policy/API difference, obsolete specification, environment difference, or adapter bug. When useful, translate a small reference implementation from another language and compare it directly. Preserve provenance, original vectors, adapter limitations, and exact commands. A local fix requires a minimized shared test and contract proof, not majority behavior. Stop each cycle after one source and one case family are fully classified.
```

<a id="goal-108"></a>

### 108. Adversarial peer, client, file, and environment artifact generation

<!-- slug: adversarial-artifact-generation; prompt-chars: 3269; utf8-bytes: 3270 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/adversarial-artifact-generation.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Force the agent to build the adversary instead of describing one: a hostile protocol peer, malicious API client, malformed-but-plausible message stream, corrupt file/database generator, partial-I/O shim, clock source, callback, or restart driver. The artifact must be deterministic, reusable, and exercise the real production boundary.

Generate valid setup followed by invalid ordering, collisions, truncation, duplication, resource pressure, stale data, corruption, or environmental failure. Combine the artifact with assertions, sanitizers, coverage, and exact state snapshots. Verify cleanup, bounds, recovery, and externally visible impact, then shrink the scenario while keeping it public. Reject fake models that bypass production parsing or state transitions. Preserve useful tools and transcripts even when they expose only missing coverage or a blocked attack path.
```

<a id="goal-109"></a>

### 109. Whole-feature cross-file public-path security audit

<!-- slug: whole-feature-public-path; prompt-chars: 3931; utf8-bytes: 3932 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/whole-feature-public-path.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Select one externally meaningful feature, not one file. Map its complete path from public input through parsing, validation, conversion, caches, queues, threads, persistence, restart/recovery, and output. Include alternate entry points, optional modes, duplicated implementations, cleanup, and authority changes.

For a bounded cycle go two or three surfaces deep, list untouched edges, and produce a feature map plus at least one runnable end-to-end scenario. Compare assumptions at every boundary: which layer validates, owns, limits, persists, rolls back, and reports state. Search for gaps that no single-file review sees, especially valid components whose interaction is wrong. Use assertions, coverage, history, and sibling behavior to focus experiments, and prove any candidate through the public path before proposing the smallest local fix.
```

<a id="goal-110"></a>

### 110. Self-fueling catalog evolution and entropy-quality audit

<!-- slug: catalog-entropy-evolution; prompt-chars: 3884; utf8-bytes: 3885 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/catalog-entropy-evolution.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit the campaign catalog itself as a live security instrument. For each completed cycle, inspect what was learned, which prior assumptions were narrowed, which goals gained concrete triggers or oracles, and whether suspicious code created a genuinely different search direction.

Build a lineage graph from cycle and finding to amended or new goals. Reject generic rewrites, duplicates, and goals with no runnable first experiment. Merge overlapping goals, split goals whose evidence now reveals distinct mechanisms, and retire stale goals without erasing history. Replay a sample of amendments to prove they change target selection or experiments. Measure catalog growth, dedup rate, project coverage, and confirmed-finding yield so self-fueling produces useful entropy rather than prompt inflation.
```

<a id="goal-111"></a>

### 111. Coverage manifest, deferred-work, and incomplete-scan closure audit

<!-- slug: coverage-manifest-closure; prompt-chars: 3843; utf8-bytes: 3844 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/coverage-manifest-closure.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Create an explicit coverage manifest for the selected repository and target: files, functions, features, trust boundaries, public entry points, configurations, generated/vendor code, and ranking, review, validation, and attack-path phases. Record what was reviewed, partially reviewed, excluded, deferred, or unreachable and why.

Treat incomplete coverage as a first-class result, never as clean. When a prior finding disappears, determine whether its root cause was fixed, its location moved, or the later scan simply missed its scope. Rank high-risk unreviewed and deferred cells, construct the smallest experiment that closes one cell, and update both coverage and campaign queues. Use coverage deltas to select work, not to claim security from percentages.
```

<a id="goal-112"></a>

### 112. Replayable scan recipes, finding continuity, and false-positive revalidation

<!-- slug: scan-recipe-finding-continuity; prompt-chars: 3348; utf8-bytes: 3349 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/scan-recipe-finding-continuity.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Store an immutable, replayable recipe for each scan: repository and full revision, target scope/mode, knowledge inputs, tool/model/plugin versions, reasoning/worker settings, seeds, environment, cost limits, output schema, and artifact hashes. Re-run recipes against current code and exact old revisions to distinguish scanner drift from code drift.

Match findings by root cause rather than wording or line number and classify new, persisting, reopened, resolved, or unknown. Treat every false-positive reason as a conditional claim about guards, reachability, configuration, or trust boundaries; revalidate it whenever code, scope, dependencies, or threat model changes. Preserve sealed outputs and matching decisions so later agents can audit continuity rather than rediscover history.

Before an expensive run, preflight the target, output location, credential source, toolchain, plugin version, and conflicting overrides without starting the scanner.
```

<a id="goal-113"></a>

### 113. Risk ranking, deep-scan stopping, and marginal-yield audit

<!-- slug: risk-ranking-deep-scan-yield; prompt-chars: 3289; utf8-bytes: 3290 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until a real blocker; never claim completion. Before each cycle update `agent-journal/risk-ranking-deep-scan-yield.md` with repo/profile, revision/base/HEAD, state/jobs, scope, knowledge, prior findings/history, tool/model, seed/budget, commands/output, verdict, and resume point. Use scratch data; never touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 falsifiable iterations, then re-rank. Each inspects code/callers, runs the narrowest experiment, and ends confirmed, dismissed, or inconclusive. Separate scout/verifier/fixer/reviewer where practical; verify before seeing a fix. Search semantic/hash duplicates first. External reports, specs, sibling code, and scanners are leads, not proof.

Record trust boundary, contract, public path, versions, test gap, masking, limits, false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed, first-invalid trace, mutation/coverage delta, build matrix, or bounded proof. Prefer public proof and independent verification for high impact; do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; missing under incomplete/different scope stays unknown. Preserve WIP tests/harnesses, seeds, traces, failed attempts, and negative results. Confirmed defects get the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence; otherwise commit at most one focused artifact/journal snapshot. No churn or failure masking.

Mandatory entropy return: add new bug shapes, invariants, oracles, suspicious sites, blocked paths, false-positive conditions, and cross-project analogies as concrete triggers/experiments in existing goals. Put distinct ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, semantic fingerprint, and first runnable experiment; promote only evidence-backed nonduplicates under 4,000 bytes. Update catalog index/count/hash and lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, indexes, `URGENT.md`, and `agent/all-findings`, then continue.

Campaign focus:
Rank features and surfaces before scanning using public reachability, privilege, secrets, persistence, parser complexity, statefulness, historical defects, code churn, weak coverage, and unresolved suspicions. Allocate workers and budgets to the highest-value cells while retaining randomized exploration to avoid ranking lock-in.

Run repeated discovery under explicit total-run, no-new, worker, token, time, and cost bounds. Record unique root causes per run, duplicate rate, validation rate, coverage gained, and marginal cost. A no-new streak may stop that recipe, not declare the repository clean. Change method, model, knowledge, or target when yield stalls, and feed the measured strengths and blind spots back into future goal selection.

When a budget or cancellation stops a run, preserve and index partial findings, coverage, and artifacts rather than discarding the incomplete evidence.
```

<a id="goal-114"></a>

### 114. Threat-model and knowledge-base conversion into executable oracles

<!-- slug: knowledge-base-executable-oracles; prompt-chars: 3818; utf8-bytes: 3819 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/knowledge-base-executable-oracles.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Collect architecture documents, security policies, protocol specifications, threat models, incident reports, release notes, and maintainer notes with exact versions and provenance. Extract concrete assets, actors, trust boundaries, forbidden states, assumptions, and required behaviors, then map each claim to code, callers, and tests.

Do not leave outside knowledge as prompt context. Convert high-value claims into executable tests, assertions, reference models, attack scenarios, or coverage targets. Detect stale or contradictory knowledge by comparing it with current behavior and history. When a document is ambiguous, preserve competing interpretations and build a distinguishing vector instead of silently choosing one.
```

<a id="goal-115"></a>

### 115. Committed-diff, working-tree, and pre-commit security regression audit

<!-- slug: diff-working-tree-security; prompt-chars: 3791; utf8-bytes: 3792 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/diff-working-tree-security.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit a committed diff, branch range, or staged/unstaged working tree as a security change, not only as changed lines. Record the exact baseline and scope, then trace modified contracts through callers, tests, generated artifacts, configuration, persistence, and sibling implementations.

Compare root causes and coverage against the baseline: newly introduced, persisting, resolved, reopened, or unknown. Look for removed checks, weakened errors, stale tests, partial migrations, changed authority, and security fixes whose test is absent. Build the smallest pre-commit or CI reproducer for confirmed regressions. Never call a diff clean when surrounding impact or generated/dependent code was not reviewed.
```

<a id="goal-116"></a>

### 116. Cross-scanner differential and disagreement audit

<!-- slug: cross-scanner-differential; prompt-chars: 3787; utf8-bytes: 3788 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/cross-scanner-differential.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run the campaign goals, Project Loupe, Codex Security, and relevant static, sanitizer, or fuzz tools against the same pinned revision and equivalent scope. Preserve each tool's recipe, coverage, phases, raw findings, exclusions, cost, and model/tool versions, then normalize candidates by root cause.

Investigate every meaningful disagreement: one scanner found a bug another missed, one validated what another dismissed, or their coverage claims differ. Do not use majority vote as an oracle. Build a shared reproducer or prove which scanner assumption failed. Feed each confirmed strength, blind spot, false-positive pattern, and useful artifact back into the relevant goals and scanner configuration.
```

<a id="goal-117"></a>

### 117. Security-agent calibration with historical bugs, mutants, and negative controls

<!-- slug: agent-calibration-benchmark; prompt-chars: 3885; utf8-bytes: 3886 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/agent-calibration-benchmark.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build a hidden Core calibration set from BIP50, CVE-2018-17144, script-check UAF, compact-block one-shot crash, timestamp netsplit, orphan DoS, transaction censorship, signed-char SOCKS overflow, 32-bit block-size overflow, wallet write failures, block-index ordering, and cache-provenance bugs. Pair them with difficult negatives from the knowledgebase: resolved null compact entries, PSBT trailing-byte framing, intentional scientific-notation amounts, safe caller preconditions, duplicated oracle batches, and retracted UB/assert claims.

Measure root-cause recall, severity calibration, ownership/dedup accuracy, validation, and false positives without exposing labels. Turn misses into new executable oracles, but keep held-out cases and rotate revisions so the catalog does not memorize answers.
```

<a id="goal-118"></a>

### 118. Agent sandbox, credential, environment, and artifact-isolation audit

<!-- slug: agent-runtime-isolation; prompt-chars: 3860; utf8-bytes: 3861 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/agent-runtime-isolation.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Treat the scanned repository, its build scripts, tests, tools, and dependencies as potentially hostile to the scanning environment. Inventory filesystem, network, process, device, credential, keyring, SSH/Git, cloud-token, and environment-variable access available to agents and subprocesses.

Run canary credentials and controlled malicious fixtures to test unintended reads, writes, exfiltration, cross-scan contamination, and result tampering. Keep state and sensitive outputs outside the target worktree with restrictive permissions; pin immutable revisions for bulk work. Compare local, container, and sandboxed execution and verify cancellation/cleanup. Tighten the smallest boundary that blocks a demonstrated capability without making required builds or proofs impossible.
```

<a id="goal-119"></a>

### 119. Bulk multi-repository and ecosystem recurrence mining

<!-- slug: bulk-ecosystem-recurrence; prompt-chars: 3838; utf8-bytes: 3839 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/bulk-ecosystem-recurrence.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Run resumable campaigns over a pinned list of repositories and full commit hashes, with per-repository profile, scope, mode, budget, and result directory. Include related nodes, wallets, crypto libraries, storage engines, bindings, forks, and downstream users where authorization permits.

Normalize confirmed findings into reusable bug shapes and search for recurrence across projects, copied code, shared dependencies, test vectors, and API assumptions. Preserve project-specific contracts so differences are not treated as bugs merely because implementations disagree. Feed cross-project evidence back into local priorities, sibling-test transplantation, dependency upgrades, and new goals whose first experiment is defined for more than one repository.
```

<a id="goal-120"></a>

### 120. Sparrow PSBT, signing-intent, and hardware-wallet verification audit

<!-- slug: sparrow-psbt-signing-intent; prompt-chars: 3803; utf8-bytes: 3804 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-psbt-signing-intent.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
For Sparrow and related desktop wallets, map PSBT creation, import, merge, update, signing, finalization, broadcast, and save/reopen flows. Track transaction inputs/outputs, amounts, fees, change identification, sighash, derivation paths, master fingerprints, descriptors, multisig policy, labels, and network.

Build adversarial PSBTs and a fake or instrumented hardware wallet to compare host intent, device display, signed data, and final transaction. Test duplicate/conflicting fields, unknown/proprietary data, partial signatures, reordered outputs, malicious change claims, fee surprises, and mixed networks. Require an end-to-end proof of any signing mismatch; never infer safety from a correct UI summary alone.
```

<a id="goal-121"></a>

### 121. Sparrow backend trust, privacy, and network-isolation audit

<!-- slug: sparrow-backend-privacy; prompt-chars: 3816; utf8-bytes: 3817 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-backend-privacy.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model Sparrow's trust in Bitcoin Core, Electrum servers, public servers, Tor/proxy paths, fee sources, exchange-rate sources, and fallback logic. Identify which claims are verified locally and which can influence balances, history, UTXOs, confirmations, fees, addresses queried, or transaction broadcast.

Build a deterministic lying backend that omits, reorders, fabricates, delays, or equivocates about headers, transactions, UTXOs, mempool state, and fees. Test reorgs, reconnects, server switches, mainnet/testnet/signet confusion, proxy bypass, DNS/network leaks, and wallet-query correlation. Separate privacy loss, misleading display, signing risk, and recoverable availability failures, and preserve reusable server transcripts.
```

<a id="goal-122"></a>

### 122. Sparrow wallet-file encryption, backup, import, and recovery audit

<!-- slug: sparrow-wallet-recovery; prompt-chars: 3859; utf8-bytes: 3860 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-wallet-recovery.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Map Sparrow wallet creation, password/KDF use, encrypted persistence, keystores, seeds/xprvs, watch-only data, descriptors, labels, backups, imports/exports, migrations, autosave, and restore. Identify authoritative memory and disk state and every point where plaintext or partially updated data can remain.

Inject wrong passwords, malformed or truncated files, partial writes, rename/fsync failures, crashes, concurrent opens, old-version files, and interrupted migrations. Restart and verify no silent key loss, mixed encrypted/plaintext state, corrupted backup replacement, network mismatch, or watch-only wallet becoming sign-capable. Trace secrets through Java objects, clipboard/logging, temporary files, and cleanup, using only scratch wallets and deterministic fixtures.
```

<a id="goal-123"></a>

### 123. Sparrow Java and JavaFX lifecycle, concurrency, and secret-retention audit

<!-- slug: sparrow-javafx-lifecycle; prompt-chars: 3820; utf8-bytes: 3821 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-javafx-lifecycle.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit JavaFX application-thread rules, background services, futures, event handlers, cancellation, wallet/network switching, hardware-device callbacks, shutdown, and exception paths. Map ownership of windows, controllers, wallets, sessions, listeners, executors, and cached models.

Create deterministic barriers instead of sleeps to force close/reopen, rapid network changes, device disconnect, failed signing, repeated dialogs, and shutdown during persistence. Check stale UI state, double actions, races, leaked listeners/threads, use-after-close logic, and secrets retained in Strings, logs, clipboard, crash reports, or long-lived objects. Prefer public UI/service scenarios backed by state assertions rather than visual checks alone.
```

<a id="goal-124"></a>

### 124. Sparrow build, submodule, update, and release-integrity audit

<!-- slug: sparrow-release-integrity; prompt-chars: 3810; utf8-bytes: 3811 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/sparrow-release-integrity.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Audit Sparrow's Gradle build, pinned Java/toolchains, `drongo` and `lark` submodules, dependencies, generated resources, native packaging, reproducible pre-signing binaries, code signing, installers, release metadata, and update/download paths.

Rebuild pinned releases on independent environments and explain every binary difference. Test stale or substituted submodules, dependency confusion, untrusted repository/plugin input, update metadata tampering, downgrade/replay, signature or hash mismatch, and platform-specific packaging gaps. Separate reproducible payloads from nondeterministic signing/installer layers. A fix must block a demonstrated supply-chain or verification failure, not merely add another unchecked hash.
```

<a id="goal-125"></a>

### 125. LevelDB WAL, MANIFEST, VersionSet, and crash-recovery audit

<!-- slug: leveldb-crash-recovery; prompt-chars: 3835; utf8-bytes: 3836 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/leveldb-crash-recovery.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Model LevelDB's write path and durable authority across WriteBatch, log append/sync, memtable and immutable memtable, table building, VersionEdit, MANIFEST, CURRENT, file rename/delete, compaction, close, and reopen. Enumerate crash points before and after every state transition.

Use a deterministic Env or fault filesystem to inject short writes, sync/rename failures, dropped or reordered persistence, truncation, stale files, and process death. Reopen after each schedule and compare visible key/value state with the acknowledged-write contract, checking idempotence, orphan cleanup, sequence numbers, and background errors. Preserve minimized operation/fault traces and distinguish LevelDB defects from client misuse of `sync` or recovery guarantees.
```

<a id="goal-126"></a>

### 126. LevelDB comparator, snapshot, iterator, filter, and compaction semantics audit

<!-- slug: leveldb-semantic-matrix; prompt-chars: 3817; utf8-bytes: 3818 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/leveldb-semantic-matrix.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Build a semantic matrix for custom comparators and names, internal-key ordering, snapshots, forward/backward iterators, seeks, deletions/tombstones, WriteBatch order, Bloom/filter policies, block/table caches, and compaction across levels.

Generate operation traces with equal-prefix keys, unusual byte strings, comparator edge cases, snapshots spanning updates/deletes, iterator invalidation, filter false positives, and overlapping compactions. Compare incremental results with a simple ordered reference model after every step. Test reopen with changed or inconsistent comparator/filter configuration. Classify allowed probabilistic differences separately and require a minimized trace before changing stable API or on-disk behavior.
```

<a id="goal-127"></a>

### 127. LevelDB corruption, checksums, background errors, and client-assumption audit

<!-- slug: leveldb-corruption-client-contracts; prompt-chars: 3886; utf8-bytes: 3887 -->

```text
/goal
Create/check out a dedicated branch. Continue this evidence-first campaign until a real blocker; never claim repository completion. Before each cycle journal repo/profile, revision/base/HEAD, dirty state/jobs, target (repo/feature/path/diff/working tree), knowledge inputs, prior findings/issues/PRs/history, tool/model versions, seed/budgets, and resume point in `agent-journal/leveldb-corruption-client-contracts.md`. Use scratch data; do not touch unrelated work, upstream refs, default datadirs, live wallets/keys, or production databases.

Run 2-4 distinct falsifiable iterations, then re-rank. Each inspects code and callers, runs the narrowest useful experiment, records exact commands/output, and ends confirmed, dismissed, or inconclusive. Separate scout, verifier, fixer, and reviewer where practical; verify before revealing a fix. Search semantic/hash duplicates first. Reports, specs, sibling code, scanners, and model opinions are leads, not proof.

Record each candidate's trust boundary, contract, public source-to-sink path, history/spec evidence, affected versions, test gap, repair masking, limitations, conditional false-positive assumptions, and verdict. Require runnable evidence: fail-before/pass-after, minimized seed/fixture, first-invalid sanitizer/static trace, mutation/coverage delta, reproducible profile/build matrix, or bounded proof. Prefer public functional proof and two independent verifiers for consensus, keys/funds/privacy, crypto, persistence, or remote impact. Do not inflate latent issues.

Record reviewed/unreviewed/excluded/deferred scope; incomplete coverage is never clean. Track root causes as new, persisting, reopened, resolved, or unknown; absence under incomplete/different scope stays unknown. Preserve WIP harnesses/tests/fuzzers, branches, seeds, traces, failed attempts, and negative results. A confirmed defect gets the smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`, with deterministic regression evidence and complete mechanism, reachability, impact, commands, limits, and handoff. No churn, broad refactor, speculative cleanup, manufactured commit, or failure masking; otherwise commit at most one focused artifact/journal snapshot.

Mandatory entropy return: extract new bug shapes, invariants, oracles, suspicious sites, blocked paths, tool limits, false-positive conditions, and cross-project analogies. Add concrete triggers, negative knowledge, and experiments to existing goals. Put genuinely different ideas in `agent-journal/proposed-goals.md` with slug, profiles, provenance, risk, dedup fingerprint, and first runnable experiment; promote only evidence-backed semantic novelties into `agent-journal/campaign-goals.md`, each under 4,000 bytes. Update index/count/catalog hash and parent lineage. New/amended goals are immediately selectable; paraphrases do not count.

Update journal, ledger, coverage/finding indexes, `URGENT.md`, and `agent/all-findings` under the uber-goal, then continue. Never repeat a passing campaign without new evidence or stop at a plan.

Campaign focus:
Inject bit flips, truncation, malformed blocks, bad checksums, missing files, stale lock/CURRENT/MANIFEST state, permission errors, disk full, and background compaction failures through LevelDB's Env boundary. Compare normal and paranoid checking, cache-cold and cache-warm reads, repair, reopen, iteration, and compaction behavior.

Trace how corruption and background errors reach callers: returned Status, delayed failure, logging, read-only survival, or silent omission. Audit client assumptions about single-process access, atomic batches, snapshots, iterator lifetime, comparator stability, sync durability, and repair. For Bitcoin Core or other wrappers, build an engine-neutral public reproduction and determine whether the bug is in LevelDB, the wrapper, or an undocumented assumption.
```

<a id="goal-128"></a>

### 128. Bitcoin full, compact, RPC, and disk block-ingress convergence

<!-- slug: bitcoin-block-ingress-convergence; prompt-chars: 3286; utf8-bytes: 3286 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bitcoin-block-ingress-convergence.md` with revision/base/HEAD, dirty state/jobs, exact ingress pair, starting chain and mempool, fixture hash, permitted differences, oracle vector, seed/build, commands/output, verdict, and resume point. Use fresh regtest datadirs and preserve unrelated work.

Run 2-4 falsifiable iterations, one ingress pair and block class at a time. Inspect the complete path into `ChainstateManager::ProcessNewBlock`, including callers, transport reconstruction, validation signals, block storage, and restart. Separate fixture author and verifier where practical. Search existing tests, issues, and historical fixes before adding a harness. External implementations are leads, not proof.

For each divergence record the shared consensus contract, path-specific transport contract, first differing transition, public reachability, and whether fixture construction or normalization caused it. Require identical block bytes after successful reconstruction, exact commands, and before/after state snapshots. For representation-layer rejection require the expected status, no `ProcessNewBlock` call, and unchanged chainstate. A relay, request, disconnect, or timing difference is allowed only when declared before the run. Preserve minimized blocks, P2P transcripts, logs, and state vectors.

A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence. Do not change consensus behavior to force transport symmetry. Return the exact new equivalence relation, permitted difference, masking condition, and next matrix row to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Drive identical blocks through full P2P `block`, compact reconstruction (`InitData`/`FillBlock` and `blocktxn`), RPC `submitblock`, disk import/reindex, and direct kernel/mining interfaces where supported. Include valid, mutated, context-free-invalid, contextual-invalid, duplicate-known-invalid, failed-parent, pruned/redownloaded, and retry-after-system-error rows. Pin parent, chainstate, mempool, caches, peer ownership, and disk state.

For every row compare validation result/reason, active tip, block-index flags/candidate and unlinked membership, in-flight/download ownership, UTXO MuHash/totals, mempool removal, undo/block availability, index queries, notifications, and restart state. Failure must not mutate the parent coins view, publish trusted caches, clear another peer's reconstruction, poison later valid delivery, or misclassify a storage/system error as consensus invalidity.

Exercise short-ID collisions, malformed prefilled indexes, wrong/repeated/late `blocktxn`, full-block fallback, witness commitment mutation, same-block spends, duplicate inputs, missing parents, disconnect/reconnect, and concurrent announcers. Start with a four-path regtest matrix, then add fault injection and no-assert/sanitizer builds. Mutate each ingress-specific guard to prove all paths converge on one consensus decision and rollback contract.
```

<a id="goal-129"></a>

### 129. Bitcoin wallet callback, rescan, reload, and restart convergence

<!-- slug: bitcoin-wallet-state-convergence; prompt-chars: 2674; utf8-bytes: 2674 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bitcoin-wallet-state-convergence.md` with revision/base/HEAD, dirty state/jobs, wallet type, chain/mempool trace, state-construction path, normalization rules, seed/build, commands/output, verdict, and resume point. Use fresh regtest wallets and preserve unrelated work.

Run 2-4 falsifiable iterations, one event class and path pair at a time. Inspect wallet notification handlers, rescan code, database load, callers, and existing tests. Define which fields are reconstructible and which record path-specific history before comparison. Separate trace author and state verifier where practical. Search historical wallet fixes first.

For each divergence record the authoritative chain/mempool state, wallet contract, first differing update, public RPC effect, persistence, and whether the control wallets began from byte-equivalent databases. Require a minimized block/transaction trace plus normalized snapshots after every event. Do not require equality for intentionally local metadata unless it changes spending, balance, trust, conflict, or recovery behavior.

Preserve wallets, blocks, transactions, logs, and snapshots. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence. Return the exact convergence relation, allowed path difference, missing callback/rescan oracle, and next trace to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Compare wallet state reached through live `CWallet::transactionAddedToMempool`, `transactionRemovedFromMempool`, `blockConnected`, and `blockDisconnected` with an unloaded wallet followed by `ScanForWalletTransactions` or `RescanFromTime`, and with reload/restart from the same pre-event wallet database. Canonicalize `gettransaction` confirmation, block, conflict, trusted, and abandoned fields, `getbalances`, `listunspent` safety/spendability, and last processed block. In focused unit coverage also compare `mapTxSpends`, mempool conflicts, and `truc_child_in_mempool`.

Begin in `wallet_reorgsrestore.py` with parent, child, double-spend, and TRUC sibling transactions. Keep one wallet live, unload another across the reorg, then reload/rescan and restart both. Feed every wallet the same suffix and compare after each connect, disconnect, mempool add/remove, reload, rescan, and restart. Include watch-only and descriptor wallets only after the base matrix is stable.
```

<a id="goal-130"></a>

### 130. Bitcoin validation-callback teardown linearization

<!-- slug: bitcoin-validation-callback-lifetimes; prompt-chars: 2838; utf8-bytes: 2838 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the named schedule matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bitcoin-validation-callback-lifetimes.md` with revision/base/HEAD, dirty state/jobs, subscriber graph, queue state, schedule, lifetime oracle, checker build, commands/output, verdict, and resume point. Use test-only subscribers and preserve unrelated work.

Run 2-4 deterministic schedules, one unregister/destruction relation at a time. Inspect `ValidationSignalsImpl`, `NotificationsHandlerImpl`, `FlushBackgroundCallbacks`, task runners, scheduler stop, `Interrupt`, `Shutdown`, and every owning caller. Separate schedule author and verifier where practical. Run ASan and TSan in separate builds and distinguish a race from a contract misunderstanding.

For each failure record the exact happens-before contract, first callback or destruction that violates it, partial-initialization state, production caller, and checker trace. Require a replayable gate schedule rather than timing sleeps. Preserve subscriber traces, queues, logs, and failed schedules. Do not broaden ownership or shutdown code without a reachable lifetime violation.

A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence. Return the exact lifetime edge, missing stop/join/flush operation, false-positive condition, and next schedule to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Audit `ValidationSignalsImpl::Register`, `Unregister`, `Clear`, and `Iterate`, queued events, `NotificationsHandlerImpl::disconnect`, `FlushBackgroundCallbacks`, scheduler stop, and the explicit resets in `Shutdown()`. Enumerate unregister-before-start, unregister-during-call, unregister-all reentrancy, subscriber self-destruction, queue flush, scheduler stop, repeated `Interrupt`/`Shutdown`, and every partially initialized node cut point.

Assert unregistration prevents later events from selecting the subscriber, but allow a callback already selected by an in-flight dispatch to start or finish after return. Require the shared subscriber to stay alive through that callback, raw-pointer callers to provide their documented external lifetime, each subscriber to be destroyed exactly once, and queued work to drain or be discarded according to the documented phase without deadlock or use-after-free. First add a gated `util::TaskRunnerInterface` test double to `validationinterface_tests.cpp`, pause inside the first of two subscribers, race each unregister variant, and release deterministically. Then repeat the proven schedule through the real `NotificationsHandlerImpl` path in `src/node/interfaces.cpp`.
```

<a id="goal-131"></a>

### 131. Bitcoin fee-estimator checkpoint continuation fuzzing

<!-- slug: bitcoin-fee-estimator-checkpoint-fuzz; prompt-chars: 2548; utf8-bytes: 2548 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the operation/checkpoint matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bitcoin-fee-estimator-checkpoint-fuzz.md` with revision/base/HEAD, dirty state/jobs, deterministic clock, operation trace, checkpoint, serialization/fault mode, oracle vector, seed/build, commands/output, verdict, and resume point. Use scratch estimator files and preserve unrelated work.

Run 2-4 falsifiable traces, one checkpoint or persistence fault at a time. Inspect `CBlockPolicyEstimator`, `policy_estimator`, `policy_estimator_io`, RPC consumers, serialization, and existing fuzz/unit tests. Separate trace generator and oracle verifier where practical. Fix time and transaction ordering before comparing.

For each divergence record the estimator contract, first operation/target that differs, persisted fields involved, RPC reachability, and whether the fixture violated estimator preconditions. Require a minimized prefix/checkpoint/suffix trace and per-step output table. Preserve estimator files, logs, seeds, and temporary mutations. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence.

Return the exact continuation invariant, missing serialized field, corrupt-input contract, and next checkpoint to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Feed estimator A a deterministic prefix of `processTransaction`, `removeTx`, `processBlock`, and `FlushUnconfirmed` operations. Serialize A with `Write`, restore estimator B with `Read`, and feed both the identical suffix. After every operation compare `estimateFee`, `estimateRawFee` including every `EstimatorBucket`, `estimateSmartFee` including `FeeCalculation`, and `HighestTargetTracked` across confirmation targets, thresholds, horizons, and conservative modes. Add a replay-from-zero estimator C to distinguish persistence errors from incremental-update errors.

Checkpoint at every operation boundary around bucket edges, unconfirmed aging, replacements, block inclusion, and reorg-like removals. Separately inject truncation, stale timestamps, unknown versions, write interruption, and failed reads, then assert the documented fallback and unchanged live estimator state. Temporarily omit or perturb one serialized statistic to prove the continuation oracle detects drift before keeping any new test.
```

<a id="goal-132"></a>

### 132. Bitcoin mempool dump, import, and restart continuation equivalence

<!-- slug: bitcoin-mempool-checkpoint-equivalence; prompt-chars: 2430; utf8-bytes: 2430 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the checkpoint matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bitcoin-mempool-checkpoint-equivalence.md` with revision/base/HEAD, dirty state/jobs, transaction graph, metadata options, checkpoint path, suffix operations, oracle vector, seed/build, commands/output, verdict, and resume point. Use fresh regtest datadirs and scratch dump files.

Run 2-4 falsifiable traces, one persistence path and metadata option set at a time. Inspect `node::DumpMempool`, `LoadMempool`, `ImportMempoolOptions`, `importmempool`, restart wiring, and existing tests. Separate dump generator and verifier where practical. Fix chain tip, mock time, policy options, and transaction arrival order before comparing.

For each divergence record the persistence contract, first field or later operation that differs, public effect, and whether expiry or policy legitimately changed acceptance. Require a minimized prefix/checkpoint/suffix trace and normalized pool snapshots. Preserve dump files, logs, seeds, and rejected records. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence.

Return the exact continuation relation, intentionally omitted metadata, import-mode difference, and next path to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Compare an uninterrupted `CTxMemPool` with pools restored through `node::DumpMempool`/`LoadMempool`, normal restart, and RPC `importmempool` with explicit `ImportMempoolOptions`. Seed parent-child packages, RBF conflicts, fee deltas for present and absent txids, entry times around expiry, and unbroadcast entries. Canonicalize `infoAll()`, `GetPrioritisedTransactions()`, `GetUnbroadcastTxs()`, and verbose `getrawmempool` dependency, fee, ancestor, and descendant fields.

Feed every restored pool the same replacement, trim, expiry, block-removal, and reorg suffix and require continued equality for the selected metadata flags. Begin by strengthening the `validation_load_mempool` fuzzer with a structured dump/load round trip, then extend `mempool_persist.py` to compare automatic load with `importmempool` using `apply_fee_delta_priority`, `apply_unbroadcast_set`, and `use_current_time` combinations.
```

<a id="goal-133"></a>

### 133. Bitcoin PSBT merge and finalization algebra

<!-- slug: bitcoin-psbt-transform-algebra; prompt-chars: 2727; utf8-bytes: 2727 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the transform matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bitcoin-psbt-transform-algebra.md` with revision/base/HEAD, dirty state/jobs, PSBT version and fragment provenance, transform/order, conflict class, oracle vector, seed/build, commands/output, verdict, and resume point. Use synthetic keys and transactions only.

Run 2-4 falsifiable iterations, one field family and algebraic relation at a time. Inspect `PartiallySignedTransaction::Merge`, `PSBTInput::Merge`, `PSBTOutput::Merge`, `CombinePSBTs`, analysis/finalization callers, serialization, and existing tests. Separate fragment generator and verifier where practical. Define when field order is semantically irrelevant before canonicalizing.

For each divergence record the PSBT/BIP contract, first transform that differs, public RPC reachability, destination mutation on failure, and whether the fragments were legally combinable. Require minimized serialized fragments and results for every permutation. Preserve vectors, seeds, and temporary mutations. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence.

Return the exact algebraic relation, conflicting-field rule, missing field family, and next permutation to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
For PSBT v0 and v2 sharing the same unsigned transaction and containing disjoint or byte-identical fields, test merge/combination for permutation invariance, associativity, and idempotence. Canonicalize by parse plus reserialization. Every successful merge order must yield the same `AnalyzePSBT` role, `FinalizePSBT` result, and `FinalizeAndExtractPSBT` transaction bytes, txid, and wtxid. Treat conflicting values separately: derive each field's receiver-first or union rule from the BIPs and implementation, then assert the permitted order sensitivity. Never demand conflict rejection. Differing unique IDs or PSBT versions must return false before mutating the receiver.

Start in `src/test/fuzz/psbt.cpp`: split one decoded PSBT into three complementary fragments, combine every order plus duplicates, and cover legacy, segwit, Taproot key-path, Taproot script-path, unknown, and proprietary fields. Add malformed keys, duplicate semantic fields with distinct encodings, missing UTXO data, and finalized/unfinalized mixtures. Use a temporary merge mutation to prove each relation is sensitive, then confirm any defect through `combinepsbt`, `analyzepsbt`, or `finalizepsbt` in `rpc_psbt.py`.
```

<a id="goal-134"></a>

### 134. Bitcoin script-cache and verification-interface parity

<!-- slug: bitcoin-script-cache-verification-parity; prompt-chars: 2746; utf8-bytes: 2746 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the cache/interface matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bitcoin-script-cache-verification-parity.md` with revision/base/HEAD, dirty state/jobs, transaction/input, script flags, spent-output data, cold/warm and direct/queued path, cache action, oracle result, seed/build, commands/output, verdict, and resume point.

Run 2-4 falsifiable iterations, one flag transition or cache-key input at a time. Inspect `CheckInputScripts`, `CScriptCheck`, `ValidationCache`, `SignatureCache`, `VerifyScript`, kernel verification, callers, and existing cache tests. Separate vector author and verifier where practical. Define API-specific error representations before comparing.

For each divergence record the consensus/policy contract, exact cache key or interface boundary, first result/status difference, public reachability, and cache precondition. Require a minimized transaction/prevout/script vector and cold/warm trace. Preserve vectors, cache traces, and temporary key mutations. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence. Never disable caching to hide an incomplete key.

Return the exact parity relation, missing key input, permitted status difference, and next flag row to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Compare cold and warm `CheckInputScripts`, queued and direct `CScriptCheck`, direct `VerifyScript`, and `btck_script_pubkey_verify` over the supported script-flag lattice. Hold transaction, input index, amount, scriptPubKey, witness, annex, tapleaf/control data, and spent-output vector fixed. Compare success/failure and normalized script error while respecting interface-specific status contracts. Exercise cache store, hit-without-erase, hit-with-erase, miss, disabled cache, and concurrent lookup where supported.

Start from `txvalidationcache_tests.cpp` and the `script_sigcache` fuzzer. Temporarily omit or perturb each cache-key component: wtxid/flags for the full-script cache, and sighash/signature/pubkey plus ECDSA/Schnorr domain separation for the signature cache. Separately violate the documented spent-output/view precondition to prove the harness detects it without treating invalid caller state as a production bug. Include failing-then-valid and valid-then-failing signatures, Taproot versus legacy, and byte-identical transactions instantiated separately and reached through direct and queued paths. Confirm any surviving mutant through a public transaction- or block-validation path.
```

<a id="goal-135"></a>

### 135. Bitcoin wallet spend-construction contract matrix

<!-- slug: bitcoin-wallet-spend-contracts; prompt-chars: 2584; utf8-bytes: 2584 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the RPC/option matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bitcoin-wallet-spend-contracts.md` with revision/base/HEAD, dirty state/jobs, wallet snapshot, RPC/options, recipients and inputs, normalized output contract, seed/build, commands/output, verdict, and resume point. Use fresh regtest wallets and deterministic fee data.

Run 2-4 falsifiable iterations, one shared option or failure class at a time. Inspect the common spend/coin-selection code and each RPC adapter before comparing. Separate fixture author and verifier where practical. Define allowed random coin/change choices and compare contracts rather than demanding identical transactions.

For each divergence record the documented RPC contract, shared helper path, first option translation or mutation that differs, wallet-state effect, and whether watch-only/external-signer capability explains it. Require a minimized wallet snapshot and calls. Preserve wallets, PSBTs, transactions, logs, and seeds. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence.

Return the exact shared contract, intentional RPC difference, missing rollback assertion, and next option row to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Drive `fundrawtransaction`, `walletcreatefundedpsbt`, `send`, `sendall`, `bumpfee`, and `psbtbumpfee` from equivalent wallet snapshots. Cross only options that express the same intent: preset inputs, add-inputs, fee rate/mode/target, subtract-fee recipients, change address/type/position, replaceability, locktime, input locking, include-unsafe, solving data, and watch-only or external-signer mode.

Normalize recipient scripts/amounts, fee bounds, input eligibility, change ownership/type, replaceability/locktime, locked coins, PSBT completeness/finalization, and wallet mutation on success or failure. For randomized coin selection compare invariant bounds and ownership, then replay with fixed randomness when available. Inject insufficient funds, unavailable preset inputs, invalid change, fee-bound violations, signer failure, and database write failure. Start with one shared case in `wallet_fundrawtransaction.py` and `wallet_bumpfee.py`. Require every failing call to preserve reservations, locked coins, address reuse state, and transaction records unless the API documents otherwise.
```

<a id="goal-136"></a>

### 136. Bitcoin AddrMan, asmap, and peers.dat state-machine audit

<!-- slug: bitcoin-addrman-persistence-state; prompt-chars: 2609; utf8-bytes: 2609 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the operation/reload matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bitcoin-addrman-persistence-state.md` with revision/base/HEAD, dirty state/jobs, deterministic key/clock/asmap, address set, operation trace, reload/corruption mode, oracle vector, seed/build, commands/output, verdict, and resume point. Use scratch `peers.dat` files and no live peers.

Run 2-4 falsifiable traces, one bucket/collision or persistence boundary at a time. Inspect `AddrMan` add/good/attempt/select/collision logic, serialization, `peers.dat` load/save, asmap handling, callers, and existing fuzz/unit tests. Separate trace generator and model verifier where practical. Fix salts, clocks, services, and network grouping before comparing.

For each divergence record the table/bucket invariant, first operation that breaks it, public network effect, persistence phase, and whether randomized selection permits the result. Require a minimized operation trace and normalized table digest. Preserve address fixtures, dump files, logs, and seeds. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence.

Return the exact state invariant, allowed selection variance, corrupt-file contract, and next trace to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Use deterministic `AddrMan` keys and mock time to exercise `Add`, `Good`, `Attempt`, tried-collision resolution, `ResolveCollisions`, `Select`, `SelectTriedCollision`, serialization, and reload. Generate same-group addresses, multiple network types, duplicate endpoints, bucket collisions, terrible/stale entries, service changes, and source-address changes. After every operation recompute map/table membership, new/tried counts, bucket multiplicity, collision-set validity, and selectable-entry reachability from a slow model.

Compare uninterrupted state with save/reload under unchanged asmap, changed asmap checksum, empty asmap, and supported format versions. Inject truncation, bad checksum/magic, inconsistent counts, extra bytes, atomic-write interruption, and repeated restart. Classify fallback-to-empty versus startup failure from the actual contract. Begin in `addrman_tests.cpp` or the AddrMan fuzzer with a trace that crosses one tried-collision and reload boundary, then confirm user-visible effects through a scratch node only if the internal invariant diverges.
```

<a id="goal-137"></a>

### 137. secp256k1 Silent Payments sender and receiver duality

<!-- slug: secp-silentpayments-duality; prompt-chars: 2522; utf8-bytes: 2522 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the algebra/size matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/secp-silentpayments-duality.md` with revision/base/HEAD, dirty state/jobs, module/configuration, input and recipient vectors, permutation/boundary, oracle values, seed/build, commands/output, verdict, and resume point. Use deterministic test keys only.

Run 2-4 falsifiable iterations, one relation or boundary at a time. Inspect the public Silent Payments API, implementation, BIP352 vectors/contract, callers, and module tests. Separate vector author and algebra verifier where practical. Run ordinary and no-VERIFY tests where relevant and use exhaustive groups only for relations they preserve.

For each divergence record the exact sender/receiver equation, first public call that differs, valid-input domain, output mutation on failure, and whether ordering is contractual. Require minimized serialized inputs and intermediate public values. Preserve vectors, seeds, traces, and temporary equation/hash mutations. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence.

Return the exact duality relation, boundary, invalid-input contract, and next matrix row to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Cross-check `secp256k1_silentpayments_sender_create_outputs`, recipient prevout-summary creation, recipient output scanning, label create/parse/serialize, and labeled-spend creation. Permute mixed full and x-only inputs while holding the smallest outpoint fixed. The sender secret-key sum must match the recipient public-key sum and yield identical discovered output keys/tweaks. Permute recipients with duplicate scan/spend keys while tracking the stable output index.

Test recipient groups of 1, 2, 2323, and 2324, zero-sum or canceling input sets, duplicate keys/outpoints, invalid scan/spend keys, label values 0, 1, and `UINT32_MAX`, label serialize/parse, and `unlabeled + label` versus direct labeled derivation. Compare each specialized BIP352 tagged-hash midstate with generic tagged SHA256. First add one deterministic permutation property to the Silent Payments module tests, prove it with a temporary sign/index/hash mutation, then run the focused `silentpayments` target with a fixed 32-byte seed before widening the size matrix.
```

<a id="goal-138"></a>

### 138. secp256k1 SHA256 override dispatch closure

<!-- slug: secp-sha256-override-propagation; prompt-chars: 2626; utf8-bytes: 2626 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the public-path/lifecycle matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/secp-sha256-override-propagation.md` with revision/base/HEAD, dirty state/jobs, context type, override implementation, public API path, expected call count, clone/randomize/reset sequence, seed/build, commands/output, verdict, and resume point.

Run 2-4 falsifiable iterations, one API path and lifecycle transition at a time. Inspect `secp256k1_context_set_sha256_compression`, hash-context ownership, every default hash/nonce consumer, custom callback bypass, clones, randomization, and existing probe tests. Separate compressor author and call-trace verifier where practical. Compare semantic output and exact override invocation.

For each divergence record the documented override scope, first missed or extra dispatch, context mutation, output/failure contract, and configuration. Require a minimized call sequence and compressor trace. Preserve seeds, traces, and temporary bad compressors. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence. Never change cryptographic output merely to make call counts uniform.

Return the exact propagation edge, intentional bypass, rollback rule, and next API row to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Trace a forwarding SHA256 compression override through default ECDSA nonce generation, default ECDH hashing, Schnorr, MuSig, EllSwift, Silent Payments, and `tagged_sha256`. Compare the default compressor with a forwarding wrapper and an equivalent one-block-loop implementation. Outputs must remain byte-identical while every expected compression reaches the override. Caller-supplied custom nonce or hash callbacks must bypass the context override when the API contract says they own hashing.

Test heap and preallocated contexts, clone in both directions, source destruction before/after clone, randomize with seed and NULL, reset to the default compressor, and rejected/bad override installation. A rejected override must leave the prior one active. Clones must preserve then independently mutate the hook. Specialized tagged-hash midstates must equal generic tagged hashing under the same override. Start from `plug_sha256_compression_tests`, `sha256_compression_smoke_test_tests`, and `ecdsa_ctx_sha256`. Add one table row at a time and assert both output and observed compression count.
```

<a id="goal-139"></a>

### 139. secp256k1 ecmult scratch rollback transactionality

<!-- slug: secp-ecmult-scratch-rollback; prompt-chars: 2534; utf8-bytes: 2534 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the size/callback matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/secp-ecmult-scratch-rollback.md` with revision/base/HEAD, dirty state/jobs, backend/configuration, scratch prefix/size, point count, algorithm/batches, callback fault, checkpoint oracle, seed/build, commands/output, verdict, and resume point.

Run 2-4 falsifiable iterations, one dispatch threshold or callback failure at a time. Inspect scratch checkpoint/allocation code, `secp256k1_ecmult_multi_var`, simple/Strauss/Pippenger paths, batching, callers, and tests. Separate vector/callback author and scratch-state verifier where practical. Run VERIFY, checkmem/Valgrind, ASan/UBSan, exhaustive, and 32-bit configurations where each adds evidence.

For each divergence record the allocation/rollback contract, first skipped/repeated callback or leaked allocation, output-on-failure behavior, retry result, and exact configuration. Require a minimized point set, scratch size, failing callback index, and allocation trace. Preserve seeds, traces, and temporary checkpoint mutations. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence.

Return the exact transactionality relation, alignment/threshold boundary, callback contract, and next row to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Place a known prefix allocation in scratch space, save its incoming checkpoint, and call multi-scalar multiplication with scratch sizes 0, required minus 1, required, required plus alignment, and values around each batch/algorithm threshold including `ECMULT_PIPPENGER_THRESHOLD`. Fail the point callback at every index and in each later batch. Include zero, duplicate, inverse, and infinity-result points without violating the callback contract.

After every success or failure require `alloc_size` to equal the incoming checkpoint, prefix bytes to remain intact, callback indices to have no skips or repeats, and an immediate retry to equal `secp256k1_ecmult_multi_simple_var`. Poison callback outputs on failure and use checkmem/ASan to expose reads after failure. Begin by parameterizing the existing false callback in `scratch_tests`, `invalid_scratch_space_tests`, and `ecmult_multi_tests`, then temporarily remove one checkpoint application to prove the oracle fails.
```

<a id="goal-140"></a>

### 140. secp256k1 EllSwift total-map and XDH differential

<!-- slug: secp-ellswift-total-map-xdh; prompt-chars: 2483; utf8-bytes: 2483 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the encoding/backend matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/secp-ellswift-total-map-xdh.md` with revision/base/HEAD, dirty state/jobs, backend/configuration, keys and 64-byte encodings, relation/hash callback/party, boundary, seed/build, commands/output, verdict, and resume point. Use deterministic test keys only.

Run 2-4 falsifiable iterations, one relation and backend pair at a time. Inspect EllSwift public APIs, field/group helpers, BIP324 hash callbacks, callers, exhaustive tests, and vectors. Separate encoding generator and algebra verifier where practical. Define when equality is of full points, x-coordinates, or transcript-bound hashes.

For each divergence record the exact total-map/XDH contract, first public call that differs, valid-input domain, callback/output-on-failure behavior, and backend. Require minimized keys/encodings and public outputs. Preserve vectors, seeds, traces, and temporary branch/hash mutations. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence.

Return the exact relation, boundary encoding, transcript distinction, and next backend row to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Assert that arbitrary 64-byte inputs to `secp256k1_ellswift_decode` produce valid public keys, encode/decode preserves the full point, and create/decode equals `secp256k1_ec_pubkey_create` for the same secret. For two keys require bilateral `secp256k1_ellswift_xdh` symmetry. With a raw-x callback, require equality with ordinary ECDH against the decoded peer. Distinct encodings of one point must yield the same raw shared x-coordinate while the BIP324 callback binds the ordered encodings and therefore may differ.

Exercise u/t halves equal to 0, p, p+1, and all-ones, zero/overflow secret keys, party 0/1, equal encodings, custom hash failure, and same-point/different-encoding pairs. Repeat under int128/5x52 and int64/10x26 field backends and an exhaustive group. Begin by adding the same-point/different-encoding relation to the EllSwift module tests, run the focused `ellswift` target, then run exhaustive tests in both wide-multiply configurations. Use a temporary transcript-order mutation to prove the hash oracle is sensitive.
```

<a id="goal-141"></a>

### 141. Bitcoin Core and secp256k1 BIP324 transcript parity

<!-- slug: bip324-ellswift-transcript-parity; prompt-chars: 2549; utf8-bytes: 2549 -->

```text
/goal
Create/check out a dedicated branch. Continue this bounded evidence-first campaign until the cross-layer matrix is exhausted, a defect is confirmed, or a real blocker/session limit is reached. Before each cycle update `agent-journal/bip324-ellswift-transcript-parity.md` with Core/secp revisions, dirty state/jobs, fixed keys/entropy/encodings, initiator role, fragmentation schedule, implementation pair, oracle vector, seed/build, commands/output, verdict, and resume point. Use local test peers only.

Run 2-4 falsifiable iterations, one transcript or transport boundary at a time. Inspect secp256k1 EllSwift APIs, Core key wrappers, `BIP324Cipher`, v2 transport, the Python reference, vectors, and callers. Separate vector author and cross-implementation verifier where practical. Pin both revisions and normalize only framing details declared before comparison.

For each divergence record the BIP324 transcript equation, first layer that differs, initiator/responder ordering, public handshake effect, and whether the Python or C++ adapter is wrong. Require minimized encodings plus a fragmented byte transcript. Preserve vectors, packet traces, logs, and temporary ordering mutations. A confirmed defect gets the smallest standalone buildable commit with deterministic regression evidence.

Return the exact cross-layer relation, permitted framing difference, role-ordering rule, and next schedule to the catalog. Update the journal, ledger, `URGENT.md`, indexes, and `agent/all-findings`, then continue.

Campaign focus:
Cross-check `secp256k1_ellswift_create`/`decode`/`xdh`, Core's `EllSwiftPubKey`, `CKey::ComputeBIP324ECDHSecret`, `BIP324Cipher::Initialize`, and the Python `v2_p2p.py`/`crypto/ellswift.py` reference. Fix both secret keys and entropy. Swap initiator/responder roles and transcript ordering. For every row compare EllSwift encodings, raw ECDH x-coordinate where exposed, BIP324 transcript hash, send/receive keys, session ID, packet length masks, and first encrypted packet round trip.

Vary arbitrary valid 64-byte encodings, two encodings of the same point, equal public keys, role reversal, garbage lengths 0 and 4095, and delivery fragmented at every byte around the 64-byte key and garbage terminator. A malformed or incomplete handshake must not initialize or reuse cipher state. Start by feeding one `bip324_tests.cpp` vector through the Python reference and `CKey::ComputeBIP324ECDHSecret`, then extend the existing v2 functional peer with deterministic fragmentation only after byte-level parity is proven.
```

<a id="goal-142"></a>

### 142. Bitcoin Core advisory-root-cause variant matrix

<!-- slug: bitcoin-core-advisory-variants; prompt-chars: 3131; utf8-bytes: 3132 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-core-advisory-variants.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Build an executable matrix from BIP50 and every official Bitcoin Core security advisory. For each seed record the introducing/fixing commits, failed assumption, attacker capability, affected configuration, why tests/review missed it, and the minimal historical vector. At minimum cover duplicate-input inflation, database-dependent consensus, async script-check UAF, compact-block one-shot reuse, mutated/stalling block ownership, orphan quadratic work, transaction-request censorship, adjusted-time overflow, headers/addr/INV/GETDATA memory or CPU failures, signed-char SOCKS overflow, dependency RCE, 32-bit overflow, and log/disk filling.

Abstract each into authority, lifetime, arithmetic domain, state owner, commit/rollback point, and missing oracle. Search current code and related author/feature history for semantic variants, not names. Replay safe historical vectors or equivalent mutants against current HEAD and prove the current mitigation is exercised. Keep a coverage ledger by advisory and bug shape; "no variant" requires named surfaces and protection evidence, not a grep result.
```

<a id="goal-143"></a>

### 143. Bitcoin UTXO, coins cache, undo, and reorg conservation

<!-- slug: bitcoin-utxo-undo-conservation; prompt-chars: 3093; utf8-bytes: 3094 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-utxo-undo-conservation.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Build an independent UTXO model. For each block, new state equals old state minus every uniquely spent prevout plus every spendable output; fees equal inputs minus outputs; coinbase is at most subsidy plus fees. Compare this after each `CheckBlock`/contextual check, `ConnectBlock`, `UpdateCoins`, cache flush, `DisconnectBlock`, and replay. Cover same-block spends, duplicate inputs/txids and BIP30, maturity, unspendable outputs, amount bounds, script failure, activation boundaries, and system/storage errors.

Track `FRESH`/`DIRTY`, spent markers, memory usage, best-block hash, parent purity, batch writes, overlay prefetch, and `CBlockUndo` ordering/metadata. A failed or invalid connect must leave the parent view byte-for-byte equivalent; connect then disconnect must recover the start; two paths to one tip must converge. Exercise malformed/missing/truncated undo, pruning, AssumeUTXO, `VerifyDB` levels, crash/restart, and temporary mutations of every critical check. Strengthen `coinscache_sim` with the reference map and transient-state assertions.
```

<a id="goal-144"></a>

### 144. Bitcoin invalid-block containment and rejection taxonomy

<!-- slug: bitcoin-invalid-block-containment; prompt-chars: 3071; utf8-bytes: 3072 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-invalid-block-containment.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Treat block rejection as a transaction with an explicit taxonomy: malformed transport, mutated block, context-free consensus invalid, contextual invalid, known invalid, failed parent, duplicate, policy-only, and local system/storage error. Map which flags, caches, candidate sets, peer penalties, in-flight owners, files, notifications, and descendants each result may change.

Deliver the same invalid bytes through full block, compact reconstruction, `submitblock`, disk import, reconsideration, and direct kernel/mining interfaces. Repeated invalid delivery must be deterministic; later valid delivery of the same header/parent must remain possible where the contract allows. Prove an invalid or unsolicited block cannot mutate UTXO state, publish trusted cache bits, enter candidates, poison descendants, erase another peer's download/reconstruction state, mark storage failure as consensus invalidity, or leave callbacks using destroyed data. Compare release/no-assert/sanitizer behavior and mutate each early-return cleanup edge.
```

<a id="goal-145"></a>

### 145. Bitcoin compact-block reconstruction ownership and one-shot lifecycle

<!-- slug: bitcoin-compact-block-lifecycle; prompt-chars: 3065; utf8-bytes: 3066 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-compact-block-lifecycle.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Model `PartiallyDownloadedBlock` and surrounding net-processing state as a peer-owned, one-shot state machine. Cover `cmpctblock`, prefilled indexes, short-ID mapping/collisions, mempool and extra-tx sources, `getblocktxn`, `blocktxn`, full-block fallback, optimistic reconstruction, cached/disk fallback, and final validation.

Generate wrong/reordered/duplicate/late/exact-cardinality responses, repeated `FillBlock`, concurrent announcers, owner disconnect, mutated/rejected blocks, request recreation, collision fallback, and three occupied download slots. Assert counters and output reset transactionally, only the owning peer can clear/replace its state, suppression/rate limits survive failure, and reconstructed bytes follow the same validation path as full blocks. Seed from CVE-2024-35202, mutated/stalling block disclosures, 32-bit block-size overflow, and the knowledgebase's repeated-scan/replay families. Measure work under `cs_main`, but report amplification only after a practical progress or crash effect is shown.
```

<a id="goal-146"></a>

### 146. Bitcoin block-index candidate, unlinked, and comparator-key state machine

<!-- slug: bitcoin-block-index-containers; prompt-chars: 2951; utf8-bytes: 2952 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-block-index-containers.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Audit every ordered or keyed block-index container: active chain, candidate sets per chainstate, `m_blocks_unlinked`, failed descendants, precious-block ordering, sequence IDs, chainwork ties, prune/redownload state, and index locators. List every field participating in comparison/hash and every mutation site.

Generate reorg, failed branch, pruned child before parent, redownload, duplicate `ReceivedBlockTransactions`, reconsider/invalidate, `preciousblock`, snapshot chainstate creation/deletion, and restart sequences. Before changing a comparator key, require erase-mutate-reinsert or proof that membership is absent. Recompute eligibility and ordering from a slow model after every transition; run `CheckBlockIndex()` and mutation tests that omit each erase/reinsert. No failed block may remain selectable, no linkable child may be lost, and equal-work ordering must stay strict, monotonic, and restart-stable.
```

<a id="goal-147"></a>

### 147. Bitcoin durable batch retry and dirty-set transactionality

<!-- slug: bitcoin-durable-batch-retry; prompt-chars: 2988; utf8-bytes: 2989 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-durable-batch-retry.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Find write paths that stage dirty maps/sets, serialize a batch, clear staging, then call LevelDB/SQLite/file operations or otherwise permit retry. Define the durable commit point and which in-memory work must survive a retryable failure.

Prioritize block-index and coins metadata, BaseIndex commits, wallet batches, settings, fee estimator, mempool/peer dumps, and snapshot publication. Inject one-shot EIO/ENOSPC/short write/commit failure before and after serialization, append, sync, rename, and metadata update. Retry in-process and after restart. Assert a failed attempt cannot forget dirty entries, publish a newer dependent state, duplicate side effects, or report success; the next successful retry must persist exactly the intended union. Seed from the knowledgebase's forgotten block-index dirty sets, metadata-after-flush, wallet write failures, and partial descriptor-cache merges. Use a reference durable-state journal to compare each schedule.
```

<a id="goal-148"></a>

### 148. Bitcoin AssumeUTXO trust, background validation, and cleanup convergence

<!-- slug: bitcoin-assumeutxo-lifecycle; prompt-chars: 3037; utf8-bytes: 3038 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-assumeutxo-lifecycle.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Model snapshot parsing/loading, snapshot chainstate activation, background from-genesis validation, wallet/index consumers, snapshot invalidation, chainstate takeover, deletion, and restart. Track base hash/height, metadata counts, UTXO hash, chainstate roles, usable/disconnectable range, directories, prune locks, and every durable marker.

Test zero/duplicate/out-of-order coin records, valid-hash wrong metadata, truncated/corrupt files, competing forks below/near the base, reorg during background validation, invalidation discovered only later, crash at each rename/delete/takeover step, and repeated cleanup after an orphan directory exists. Compare final state with a full sync to the same tip. A failed load or invalidated snapshot must leave a usable node, must not select an undisconnectable fork, expose inconsistent wallet/index state, or enter a permanent startup-failure loop. Include the knowledgebase's missing semantically-valid-later-invalid test as an assurance row, not a presumed bug.
```

<a id="goal-149"></a>

### 149. Bitcoin validation-cache provenance and mutable-object invalidation

<!-- slug: bitcoin-validation-cache-provenance; prompt-chars: 3058; utf8-bytes: 3059 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-validation-cache-provenance.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Inventory cached validity and derived data in `CBlock`, transactions, script/signature caches, `PrecomputedTransactionData`, kernel block handles, mining interfaces, Signet checks, and contextual validation. For each cache key, state every byte, flag, network/context, spent-output vector, height/time, and mutability assumption on which the result depends.

Validate an object, then change one omitted dimension or reuse it through another network/context/interface; compare against a fresh object and uncached reference. Include mutation after trusted block checks, Signet authorization across contexts, cold/warm script checks, queued/direct verification, cache erase modes, and copied/moved handles. A cache hit must never upgrade partial/context-free validity into contextual validity. Temporarily remove each key component or invalidation step and require a public or direct-API oracle to fail. Distinguish stock `bitcoind` reachability from reusable kernel/mining API hazards without understating either contract.
```

<a id="goal-150"></a>

### 150. Bitcoin asynchronous script-check and validation-work lifetime audit

<!-- slug: bitcoin-scriptcheck-lifetimes; prompt-chars: 2987; utf8-bytes: 2988 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-scriptcheck-lifetimes.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Map ownership and destruction order for `CCheckQueueControl`, `CScriptCheck`, `PrecomputedTransactionData`, spent-output/script storage, worker queues, cancellation, and all block-validity callers. Enumerate every early return after work submission, synchronous/queued configuration, zero/one/many workers, exception/error path, shutdown, and object move/reallocation.

Use deterministic barriers so a worker pauses on each borrowed object while the caller takes the target exit. Run ASan and TSan separately and identify the first invalid access. Recreate the root pattern of CVE-2024-52911, then search analogous queue controls, callbacks, futures, and prefetch owners. Prove the owner outlives completion on success and every failure path; RAII construction order, explicit wait/drain, and cancellation semantics must agree. Also mutate lifetime ordering to show the regression test detects the historical failure rather than merely completing quickly.
```

<a id="goal-151"></a>

### 151. Bitcoin headers-sync, adjusted-time, and IBD slot liveness

<!-- slug: bitcoin-headers-time-ibd-liveness; prompt-chars: 2954; utf8-bytes: 2955 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-headers-time-ibd-liveness.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Model adjusted time, peer time samples, header timestamp/MTP rules, low-work PRESYNC/REDOWNLOAD state, initial sync-slot selection, commitment-memory bounds, header download timeouts, and peer replacement. Track which peers influence each value and which clock may jump.

Seed from the timestamp-adjustment netsplit, low-difficulty headers memory advisories, empty-headers slot stalls, future-MTP/lagging-clock arithmetic, and concurrent REDOWNLOAD retention. Exercise `INT64_MIN`, forward/backward wall-clock jumps, stale/future forks, valid empty headers, repeated low-work chains, inbound-only and mixed peer roles, disconnect/reconnect, and restart. Prove one peer cannot indefinitely own the sole sync slot, bypass a memory/commitment bound through signed/unsigned conversion, or make valid new blocks appear too far in the future. Use mock time and locally generated headers, not sleeps or public-network timing.
```

<a id="goal-152"></a>

### 152. Bitcoin transaction-request, orphan, and censorship-resistance state machine

<!-- slug: bitcoin-txrequest-orphan-censorship; prompt-chars: 2996; utf8-bytes: 2997 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-txrequest-orphan-censorship.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Build a slow model for announcements, request scheduling, timeouts, peer eligibility/relay role, orphan insertion/resolution, parent dependencies, accepted-package paths, rejection, disconnect, and re-request. Track every `(peer, txid/wtxid/parent)` owner, deadline, candidate peer, orphan memory/latency score, and activity/eviction protection.

Replay the historical `g_already_asked_for` censorship shape and orphan quadratic stall, then test current `TxRequestTracker`, orphanage, package handling, `relay=0`, unregistered/disconnected peers, duplicate INV/TX, withheld responses, bogus-announcement churn, child-before-parent, and restart boundaries. A peer that withholds data must not monopolize a transaction; unrelated peers must make bounded progress; invalid orphans must not trigger repeated global work; accepted paths must update activity consistently. Use deterministic mock time and mutation of eviction/request selection to prove the oracle.
```

<a id="goal-153"></a>

### 153. Bitcoin external-signer and signing-intent authorization

<!-- slug: bitcoin-external-signer-intent; prompt-chars: 3019; utf8-bytes: 3020 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-external-signer-intent.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Define the exact user-approved signing intent before invoking any external signer: recipients/scripts, amounts, fee bounds, selected inputs, change ownership/script, locktime, sequences, sighash, derivations, network, and allowed metadata additions. Treat the signer and returned PSBT/transaction as hostile except for its signatures.

Exercise replacement of outputs, fee, inputs, change, locktime, sighash, transaction version, unknown fields, key origins, and final scripts; partial or malformed signer replies; command/IPC failure; replay; and wallet unload. Compare the returned object with the approved intent before commit, wallet insertion, or relay. Test hardware/external-signer RPC and GUI flows plus PSBT combine/finalize. A signer may sign but cannot silently redefine the transaction. Seed from the knowledgebase's transaction-substitution finding and distinguish what upstream #35358 covers from remaining callers. Preserve a complete before/after intent digest in tests.
```

<a id="goal-154"></a>

### 154. Bitcoin wallet database transactionality and fault-injection matrix

<!-- slug: bitcoin-wallet-db-transactionality; prompt-chars: 3027; utf8-bytes: 3028 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-wallet-db-transactionality.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Create a fault-injection matrix for wallet creation, encryption, passphrase change, descriptor/key writes, keypool/top-up, address reservation, transaction insertion, label/metadata changes, migration, import/export, backup, and rewrite. State the authoritative memory/disk state and atomic group for each operation.

Fail begin/write/erase/commit/rewrite at every operation index, then retry, unload/reload, restart, rescan, and restore. Assert success is never reported before durability; memory cannot advance beyond disk; partial encrypted/plaintext or descriptor/keypool states cannot become loadable-but-wrong; failed spend/import/export does not consume reservations or lose recovery data; and rollback errors are surfaced. Seed from the wallet encryption/passphrase fixes, descriptor-cache partial merges, keypool commit gaps, `importprunedfunds`, `createfromdump`, and plaintext-slack warning. Keep privacy hygiene separate from funds-loss transactionality when their fixes differ.
```

<a id="goal-155"></a>

### 155. Bitcoin block and undo file cursor, seek, and format-width boundaries

<!-- slug: bitcoin-block-undo-file-widths; prompt-chars: 2989; utf8-bytes: 2990 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-block-undo-file-widths.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Inventory every block/undo file number, size, offset, `FlatFilePos`, serialization width, seek/truncate helper, rollover threshold, checksum scope, and compatibility assumption. Compute exact first-overflow positions on 32- and 64-bit hosts and for legacy file APIs.

Test boundaries around 2 GiB, 4 GiB, configured block-file rollover, `uint32_t` wrap, sparse files, large undo records, negative/corrupt metadata, failed seek followed by truncate, append after reorg to an older rev file, pruning, reload, and old-reader behavior. Use sparse scratch fixtures where consensus-valid construction is prohibitively costly, then separately document the valid-work path. A wrapped cursor must never alias an earlier record; failed seek must not truncate at a stale position; new formats need explicit compatibility/fail-closed tests. Seed from the knowledgebase's dirty undo flush, Windows truncation, corrupt block-tree metadata, and 4-GiB undo cursor finding.
```

<a id="goal-156"></a>

### 156. Bitcoin release-branch security backport and disclosure parity

<!-- slug: bitcoin-security-backport-parity; prompt-chars: 2928; utf8-bytes: 2929 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-security-backport-parity.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Build a version/branch matrix from official advisories, security-fix merges, release tags, and the supplied knowledgebase. For each root cause identify first vulnerable, first fixed, maintained branches, covert carrier commits, prerequisite/follow-up fixes, tests, and disclosure status.

Check release branches and downstream packages semantically: replay a safe historical vector or mutant, inspect conflict resolution and feature differences, and verify that test absence did not make a partial backport appear complete. Search for fixes present on master but missing from a maintained release, backports missing a later fix-the-fix, or behavior changed by surrounding refactors. Respect embargo/disclosure rules and report privately where required. Do not infer vulnerability solely from a missing commit hash; prove the vulnerable shape survives and the target configuration is supported.
```

<a id="goal-157"></a>

### 157. Bitcoin release verification, trusted-key quorum, and Git ancestry audit

<!-- slug: bitcoin-release-verification-trust; prompt-chars: 2967; utf8-bytes: 2968 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-release-verification-trust.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Audit Bitcoin Core release and maintainer verification tools as authorization boundaries. Define which Git commit/tree, binary hashes, signer identities, key status, threshold, trusted history, and release metadata must be proven before acceptance.

Test `verify-binaries`, `verify-commits`, tag/commit verification, downloaded manifests, and related maintainer tools with valid signatures from untrusted, expired, or revoked keys; duplicate identities; key rotation; divergent ancestry; shallow/missing history; replaced artifacts; stale manifests; and partial network failure. A cryptographically valid signature is insufficient unless its key is currently trusted for that role, and a verified commit must belong to the intended trusted ancestry. Use disposable repositories and keys. Seed from the knowledgebase's inactive/untrusted quorum and divergent-history findings, while checking current public ownership before fixing.
```

<a id="goal-158"></a>

### 158. Bitcoin critical RPC, REST, IPC, and C++ API boundary audit

<!-- slug: bitcoin-critical-api-boundaries; prompt-chars: 2924; utf8-bytes: 2925 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-critical-api-boundaries.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Inventory public and semi-public entry points that turn caller values into indexes, iterator ranges, sizes, lifetimes, callbacks, or trusted state: wallet RPC pagination, PSBT/descriptor operations, REST, IPC/libmultiprocess, libbitcoinkernel, mining interfaces, and direct C++ component APIs.

Exercise negative/zero/one/`INT_MAX`/`UINT_MAX`, overflowed sums, mismatched lengths, null documented arguments, temporary ranges/spans/views, malformed JSON/PSBT, wrong enums/contexts, callback reentrancy, partial initialization, and output reuse after failure. Compare Debug, NDEBUG, sanitizer, 32-bit, and FFI callers. Require validation before iterator arithmetic or dereference, no exception/abort across C/IPC boundaries, and explicit output-on-failure contracts. Prioritize normally reachable crashes, funds/signing errors, or trusted-cache misuse; label latent component misuse honestly.
```

<a id="goal-159"></a>

### 159. Bitcoin valid-work adversarial block and miner-gated failure campaign

<!-- slug: bitcoin-valid-work-adversarial-blocks; prompt-chars: 3035; utf8-bytes: 3036 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-valid-work-adversarial-blocks.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Search for failures reachable only after valid proof of work or consensus-valid chain preparation. Build blocks that maximize legal transaction count, witness vector count, scripts/prevouts, undo size, sigchecks, coinbase size, compact-reconstruction dependence, same-block dependencies, file rollover, and reorg depth while staying within consensus.

Separate setup cost, trigger work, victim configuration, and affected platforms. Route each vector through full and compact relay, validation with inline/queued checks, disk write, undo creation, restart, and disconnect/reorg. Look for word-size overflow, lifetime overlap, redundant owner copies, file-position wrap, cache provenance, fatal errors, and divergence across architectures. Seed from the script-check UAF, 32-bit pathological-block crash, valid-block script-copy pressure, and undo cursor overflow. A synthetic fixture may prove arithmetic/lifetime, but severity requires a credible consensus-valid construction and cost analysis.
```

<a id="goal-160"></a>

### 160. Bitcoin negative-control, supersession, and refutation replay

<!-- slug: bitcoin-negative-control-replay; prompt-chars: 3040; utf8-bytes: 3041 -->

```text
/goal
Target repository: https://github.com/bitcoin/bitcoin

Verify the checkout/remotes correspond to `bitcoin/bitcoin`; record base/HEAD, branch, dirty state, build, and scope. Create/check out a dedicated branch; never alter or push upstream refs. Continue this bounded evidence-first campaign until the named matrix is exhausted, a defect is confirmed, or a real tool/session blocker is reached. Before each cycle update `agent-journal/bitcoin-negative-control-replay.md` and `agent-journal/index.md`.

Read `agent-journal/bitcoin-core-security-profile.md` and the supplied Core/secp knowledgebase first. Search by symbol, PR, branch, commit, patch-id, bug shape, and reproducer; classify candidates as live-unowned, owned, merged, superseded, refuted, assurance-only, or unknown, then verify current HEAD/upstream. Do not duplicate owned work or revive negative knowledge without a new trigger.

Run 2-4 falsifiable iterations over 2-3 surfaces, then re-rank. For each candidate state the invariant, trust boundary, attacker/user prerequisites, public source-to-sink path, affected versions/configurations, first invalid operation or state transition, impact, existing protections, and missing oracle. Require a failing-before/passing-after test, minimized seed/fixture, sanitizer trace, deterministic schedule, differential vector, crash-recovery replay, or bounded proof. Consensus, funds/signing, persistence, privacy, and remote findings require two independent verifiers. Stop for sign-off before any consensus-observable change.

Use scratch datadirs, wallets, keys, peers, repositories, and fault devices. Preserve commands/output, seeds, traces, fixtures, false-positive conditions, and exact resume points. One proven root cause per smallest standalone buildable commit, authored as `Lőrinc <pap.lorinc@gmail.com>`; otherwise commit at most one focused journal/harness artifact. No churn, broad refactor, speculative hardening, failure masking, or severity inflation. Keep undisclosed findings private.

Campaign focus:
Turn the supplied knowledgebase's refutations, supersessions, and assurance-only results into executable negative controls. Include resolved null compact extra slots after #35670, PSBT trailing bytes rejected by outer decoders, intentional scientific-notation amounts, safe caller preconditions, retracted `rotr`/UniValue claims, duplicate oracle batches, local-only corruption classifications, and patches already owned upstream or on the fork.

For each control pin the revision, original claim, decisive guard/caller/history evidence, and a mutation that would make the feared behavior real. Re-run controls after relevant code, compiler, model, or harness changes. The agent must distinguish "not reproduced", "unreachable on supported public paths", "already fixed", "intentional contract", and "unknown due incomplete scope". Promote only when new evidence falsifies the stored reason; otherwise record the saved investigation and prevent duplicate findings from entering `URGENT.md` or a new branch.
```
