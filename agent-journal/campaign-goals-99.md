# Reusable Continuous Agent Goals for Bitcoin Core and libsecp256k1

This set rewrites all **63 supplied goals**, separates overlapping campaigns by evidence source, and adds **36 new campaigns**. There are **99 standalone `/goal` prompts**. Every prompt contains its own run protocol, journal/handoff requirements, independent finding verification, commit discipline, and an explicit continuation loop.

The prompts intentionally do not define repository completion. They define finite, evidence-backed cycles and require priorities to be re-evaluated after each cycle. A session may stop at a real resource or external blocker, but it must leave the journal in a state another run can resume.

## External research incorporated

- Project Loupe: separated discovery/verification/fixing/reporting; regression-test PoCs; applicability checks; prior-finding search; semantic/hash dedup; independent verifier verdicts.
- Bitcoin Core fuzzing guide (doc/fuzzing.md): deterministic fuzz builds, qa-assets corpora, sanitized and high-throughput runs, libFuzzer/AFL++/Honggfuzz, coverage preservation.
- bitcoinfuzz: differential fuzzing across implementations and crypto libraries.
- libsecp256k1: field/scalar backend matrices, exhaustive tests, constant-time design, ctime/checkmem, VERIFY contracts, portable and assembly paths.
- Fuzz Introspector: static reachability vs dynamic coverage, complexity-ranked blockers.
- FuzzTest, Mull, CBMC, KLEE, FoundationDB simulation, Linux dm-flakey, Alive2, Csmith, YARPGen, Wycheproof, dudect, ctgrind, LLVM sanitizers, LLVM BOLT/SamplePGO.

## Prompt index

0. Continuous evidence-first bug mining — agent-journal/continuous-bug-mining.md
1. Source comment versus implementation contract audit — comment-code-contract
2. Assertion, Assume, and invariant reachability audit — assertion-invariant-audit
3. Current branch and PR leftover sweep — current-pr-leftovers
4. Public API, CLI, RPC, config, and help contract audit — public-interface-contracts
5. Boundary-condition and off-by-one audit — boundary-off-by-one
6. Serialization, deserialization, and untrusted-input sweep — serialization-untrusted-input
7. Untrusted-interface resource-exhaustion variant analysis — resource-exhaustion-variants
8. Locking, threading, and scheduler audit — locking-threading
9. Hit-frequency and suspicious-branch coverage audit — hit-frequency-coverage
10. Fuzz-target gap and harness-realism audit — fuzz-target-gaps
11. Sanitizer and Valgrind true-positive sweep — sanitizer-valgrind
12. Static-analysis true-positive campaign — static-analysis-true-positives
13. Secret-data lifetime and zeroization audit — secret-lifetime-zeroization
14. Secret-dependent control-flow and memory-access audit — secret-control-flow
15. Public object parsing and validation variant analysis — public-object-validation
16. Public API misuse-resistance audit — api-misuse-resistance
17. Build-matrix and module-configuration audit — build-matrix-modules
18. Exhaustive and algebraic-invariant audit — exhaustive-algebraic
19. Benchmark correctness and measurement-integrity audit — benchmark-integrity
20. Simple micro-optimization discovery and proof — micro-optimization
21. Long-running rebuild, recovery, and compaction profiling — rebuild-recovery-profile
22. Full sync, IBD, import, and end-to-end profiling — full-sync-ibd-profile
23. Perf and flamegraph investigation without forced commits — perf-flamegraph-investigation
24. Disk I/O, persistence growth, and write-amplification audit — disk-io-amplification
25. Recent performance-regression bisect — performance-regression-bisect
26. Bug fixed in one subsystem but present in another — cross-subsystem-bug-shapes
27. Error-path partial-state mutation audit — error-path-state
28. Weak-test oracle and mutation-survival audit — weak-test-oracles
29. Dead code, stale feature, and TODO archaeology — dead-stale-code
30. Security-sensitive and misleading logging audit — security-logging
31. Cross-layer docs, examples, tests, and implementation audit — cross-layer-contracts
32. Whole-history incomplete-fix and migration mining — whole-history-leftovers
33. External vulnerability and advisory variant analysis — external-vulnerability-variants
34. Uncovered-code classification and closure audit — uncovered-code-classification
35. Mutation-testing campaign — mutation-testing
36. Cross-tool sanitizer and static-analysis matrix — cross-tool-analysis-matrix
37. Build dead-zone and conditional-compilation audit — build-dead-zones
38. Failure cleanup and crash-safety audit — failure-cleanup-crash-safety
39. Generated-artifact and test-vector determinism audit — generated-artifact-determinism
40. Independent multi-agent disagreement and adjudication audit — multi-agent-adjudication
41. History archaeology from a seed topic — history-seed-archaeology
42. CI, coverage-bot, and review-bot follow-up audit — ci-review-bot-followup
43. Option and API lifecycle audit — option-api-lifecycle
44. Secret-copy and compiler-optimization audit — secret-copy-optimization
45. Constant-time boundary and declassification audit — constant-time-declassification
46. Public API output-on-failure audit — api-output-on-failure
47. Build-system and CI parity audit — build-ci-parity
48. Property, exhaustive, and algebraic oracle expansion — property-oracle-expansion
49. Critical whole-history must-fix sweep — critical-history-sweep
50. Fuzz Introspector blocker and complexity audit — fuzz-introspector-blockers
51. Invariant, differential, and metamorphic audit — differential-metamorphic
52. Integer overflow, narrowing, signedness, and division audit — integer-arithmetic-audit
53. Statistical timing-side-channel campaign — timing-side-channel
54. RAII, smart-pointer, and resource-leak audit — raii-resource-leaks
55. Alternative-implementation compatibility-difference audit — alternative-implementation-diff
56. Stale PR critical-fix resurrection audit — stale-pr-resurrection
57. Local-reasoning domain and relationship audit — local-reasoning-contracts
58. Exact helper reuse and minimal helper-extension audit — helper-reuse
59. C/C++ supply-chain and security-gate audit — supply-chain-security-gates
60. Historical reviewer-preference mining and reusable review skill — reviewer-preference-skill
61. Stateful contract-fuzzer expansion — stateful-contract-fuzzing
62. Rejected-finding resurrection and assumption attack — rejected-finding-resurrection
63. Loupe-style scout, verifier, fixer, and reporter pipeline — loupe-style-pipeline
64. Finding deduplication, recurrence, and semantic-fingerprint audit — finding-dedup-recurrence
65. Contributor-branch and work-in-progress radar — contributor-branch-radar
66. Cherry-pick, backport, and release-branch correctness audit — backport-correctness
67. Release-to-release behavioral and consensus differential — release-version-differential
68. Architecture, endianness, word-size, and ABI parity audit — architecture-abi-parity
69. SIMD, assembly, and portable-reference backend differential — backend-differential
70. Compiler, optimization, LTO, PGO, and BOLT differential — compiler-optimization-differential
71. Deterministic simulation and failure-schedule exploration — deterministic-simulation
72. Filesystem, power-loss, and crash-consistency injection — filesystem-crash-consistency
73. Network fragmentation, reordering, and partial-I/O state-machine audit — network-state-machine
74. Memory pressure, OOM, allocator, and fragmentation audit — memory-pressure-allocator
75. Build throughput, dependency graph, and container-cache audit — build-throughput-cacheability
76. Reproducible binaries, Guix, and toolchain-provenance audit — reproducible-builds
77. Symbolic execution and bounded-model-checking campaign — symbolic-model-checking
78. Compiler-transformation validation and miscompile isolation — translation-validation
79. Fuzz-corpus stewardship, minimization, and transfer audit — fuzz-corpus-stewardship
80. Fuzz-engine and property-framework differential — fuzz-engine-differential
81. Specification, test-vector, and formal-model drift audit — spec-vector-drift
82. secp256k1 field and scalar representation matrix — secp-field-scalar-matrix
83. secp256k1 group, ecmult, and formula-parity audit — secp-group-ecmult
84. secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit — secp-nonce-session
85. Bitcoin consensus mutation-score and kill-test audit — bitcoin-consensus-mutation
86. Bitcoin chainstate, reorg, prune, and index crash-symmetry audit — bitcoin-chainstate-symmetry
87. Bitcoin mempool, package, and eviction-accounting audit — bitcoin-mempool-accounting
88. Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit — bitcoin-wallet-recovery
89. Bitcoin P2P transport, permission, and peer-accounting audit — bitcoin-p2p-accounting
90. Whole-PR and commit knowledge-base recipe synthesis — historical-knowledge-recipes
91. Compiler and binary-hardening configuration audit — compiler-binary-hardening
92. ABI layout, alignment, aliasing, and object-lifetime audit — abi-alignment-aliasing
93. Allocation, syscall, clock, randomness, and callback fault injection — system-fault-injection
94. Bindings, FFI, and language-wrapper parity audit — bindings-ffi-parity
95. Database-engine and persistence-semantics differential — database-semantics-differential
96. TODO, FIXME, stub, and deferred-work challenge audit — todo-deferred-work
97. C and C++ defect-taxonomy sweep — cpp-defect-taxonomy
98. Floating-point edge values, sanitizer resurrection, and fuzz-exclusion audit — float-sanitizer-fuzz-exclusions

## Goals

The full text of each of the 99 campaign prompts follows. Prompt texts are verbatim; each begins with `/goal`.

---

## Shared protocol boilerplate (verbatim, used by goals 0–97)

Every goal 0–97 is the concatenation of `/goal`, this shared protocol, and its own
"Campaign focus:" section (quoted verbatim per goal below). The shared protocol:

```text
Create/check out a new branch first. Treat this as a continuing investigation: after every cycle update `agent-journal/<journal-slug>.md`, re-rank unchecked surfaces from all accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal base/HEAD and dirty state; scope ledger; hypotheses; exact commands/key output; confirmed, dismissed, and inconclusive candidates; unrelated leads; source links/versions; review precedent; limitations; and next queue. Search the journal, issues, PRs, and history before reporting to avoid repeats. For every online PR, capture comment/commit style, stated priorities, accepted/rejected approaches, whether preferences are general/contextual/person-specific, and likely review objections.

Prefer few definitive findings. One independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build/test alone and be correct without later commits. Put strongest findings first; reorder only the local follow-up stack, never upstream history. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative/style-only cleanup, or needless helpers. Use scratch state, fixed seeds/tmpdirs, and stable stop conditions; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, or assumptions.

For each candidate: state hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code/test/doc/tool/dependency/other-project behavior; then lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External code/reports are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: failing-before/passing-after test, minimized fuzz seed/fixture, first-invalid-operation sanitizer/static trace, mutation/coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check PoCs/patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands/key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.
```

Goal 98 uses a slightly different protocol variant (already executed verbatim in this
workspace; see agent-journal/float-sanitizer-fuzz-exclusions.md for the full text used).

---

## Campaign focus sections (verbatim per goal)

### 0. Continuous evidence-first bug mining — journal: continuous-bug-mining
Mine the current repository for real security, correctness, performance, portability, testing, documentation, and build defects. Rotate evidence sources instead of repeatedly scanning the same files: current diff, recent merges, whole history, TODOs, coverage, mutations, sanitizer/static reports, benchmarks/profiles, platform matrices, external advisories, alternative implementations, and contributor branches.

Each cycle must expand a risk map of subsystems, trust boundaries, persistence formats, concurrency relationships, secret-bearing paths, and expensive operations. Pick the highest-value unexplored cell, form a falsifiable hypothesis, verify it, update the map, and immediately continue. Do not manufacture commits to show activity. Plausible but unproven candidates remain journal entries with the exact missing evidence.

### 1. Source comment versus implementation contract audit — comment-code-contract
Audit nontrivial source comments against implementation. Prioritize claims using must/never/always, lock or lifetime requirements, cache and ownership rules, serialization formats, compatibility/consensus behavior, secret handling, recovery, bounds, and performance assumptions.

For each claim, identify the exact code and callers it governs, classify the claim, and compare code, tests, docs, blame, and historical rationale. Decide whether code or comment is wrong. Change behavior only when the intended contract is independently supported; otherwise correct the stale claim. Ignore comments that are merely improvable. For behavior changes, prove the old test suite missed the discrepancy and add the narrowest regression oracle.

### 2. Assertion, Assume, and invariant reachability audit — assertion-invariant-audit
Search assertions, `Assume`, `Assert`, `CHECK_NONFATAL`, unreachable markers, VERIFY checks, and comments implying validation or impossible states. Extract the precise invariant and trace every caller, including release-only, fuzz, RPC/config, network, persisted-data, and optional-module paths.

Determine whether the invariant is enforced before use, whether an assertion is incorrectly serving as untrusted-input validation, and whether release builds preserve needed checks. Try to falsify the invariant with boundary inputs and state transitions. Fix only invalid assumptions, missing validation, or misleading contracts. Prove reachability with a focused test, fuzz seed, crafted fixture, or formal caller/dataflow proof.

### 3. Current branch and PR leftover sweep — current-pr-leftovers
For every commit on the current branch, state its intended behavior or migration rule, then search the entire tree for analogous sites that should have changed. Look for stale names/comments/tests, duplicate old logic, partial API conversions, forgotten help or release text, generated files, build lists, optional modules, cache/index formats, lock annotations, and cleanup paths.

Use history and review discussion to distinguish deliberate scope from accidental omission. Check each commit independently and the combined stack. Fix only true leftovers, one per follow-up commit, preserving the original intent. Also record unresolved review objections and predict which current changes would trigger the same objection.

### 4. Public API, CLI, RPC, config, and help contract audit — public-interface-contracts
Compare each public interface end to end: registration, parser, type/range/unit conversion, defaults, aliases, help text, errors, runtime observation, persistence, restart behavior, docs, tests, and release notes. Cover RPC/REST/CLI/config and library headers where present.

Find values that are parsed but ignored, documented but not accepted, stored in the wrong unit/type, applied only on one lifecycle path, or reported with stale fields/bounds. Exercise exact zero/one/max/negative/duplicate/unknown cases. Prefer behavioral assertions over string-only tests. Preserve compatibility unless the intended change is proven from policy, history, and callers.

### 5. Boundary-condition and off-by-one audit — boundary-off-by-one
Mine comparisons and arithmetic around heights, times, versions, sequence numbers, counts, amounts, offsets, file positions, cache sizes, resource limits, vector/span lengths, varints, scalar/field bounds, epochs, and iterator ranges. Review `<` versus `<=`, zero/one/max, empty/full, first/last, signed sentinels, and wraparound.

For each candidate, write the mathematical domain and expected boundary table before changing code. Test immediately below, at, and above the boundary on 32- and 64-bit-relevant widths. Separate compatibility/consensus rules from local policy. Require a focused failing-before test or executable arithmetic proof; do not change externally visible boundaries from intuition.

### 6. Serialization, deserialization, and untrusted-input sweep — serialization-untrusted-input
Audit network, RPC/config, block/transaction/script, key/signature/scalar, wallet/database/index, WAL/MANIFEST/table, and persisted-state parsers. Trace length and tag fields from bytes to allocations, loops, casts, object mutation, and later assumptions.

Look for missing allocation/CPU bounds, non-canonical encodings, truncation, signedness errors, duplicate encodings, partially initialized outputs, parse-then-assume paths, and failure after state mutation. Define valid and intentionally invalid domains. Add round-trip, canonicalization, negative, and output-on-failure oracles. Use malformed fixtures under ASan/UBSan/allocator limits and preserve minimized inputs.

### 7. Untrusted-interface resource-exhaustion variant analysis — resource-exhaustion-variants
Seed from historical DoS fixes, advisories, fuzz crashes, queue/cache/eviction changes, and comparable projects. Extract the bug shape: unbounded queue/allocation/log/disk growth, repeated expensive work, bad accounting, retry storm, cache bypass, timeout abuse, compaction amplification, or permission-dependent limit bypass.

Trace a realistic attacker or local-input path and calculate an explicit upper bound for CPU, memory, disk, network, descriptors, and retained state. Build a deterministic low-limit reproducer rather than a huge uncontrolled load. Verify release and restart cleanup. Commit only when the bound or accounting failure is demonstrated; report theoretical amplification separately.

### 8. Locking, threading, and scheduler audit — locking-threading
Map protected state, mutex ownership, lock order, atomics, condition variables, queues, callbacks, worker shutdown, and object lifetimes. Search annotations, lock assertions, callbacks under locks, read-modify-write splits, early returns, cancellation, and destruction while work remains.

Run TSan and lock-order instrumentation separately. Build deterministic schedules or barriers that amplify suspected races; do not rely on sleeps. Check start/stop/restart and zero/one/many worker configurations. For each issue, identify the first conflicting accesses or lock dependency cycle and prove the minimal fix preserves ordering, progress, and shutdown behavior.

### 9. Hit-frequency and suspicious-branch coverage audit — hit-frequency-coverage
Generate line, branch, function, and fuzz hit-frequency coverage, not just a percentage. Rank rarely hit branches by security impact, state mutation, complexity, error handling, persistence, secret handling, and proximity to untrusted inputs.

For each high-risk low-hit branch, explain why it is rare: legitimate platform/config path, hard precondition, harness blocker, dead code, or missing scenario. Add a behavior-checking test only when the branch matters; do not add execution-only coverage. Compare before/after hit counts and use a temporary branch inversion/removal to prove the new oracle detects wrong behavior.

### 10. Fuzz-target gap and harness-realism audit — fuzz-target-gaps
Inventory production entry points against existing fuzz targets. Find important parsers, state machines, recovery paths, validation branches, and optional modules that no target reaches, plus harnesses that use unrealistic state, discard errors too early, over-constrain inputs, or stop before the real validator.

Measure static reachability and dynamic coverage, then design the smallest deterministic harness using production-like construction. Add strong crash-independent oracles and recent regression seeds. Use Bitcoin Core `qa-assets` where applicable, preserve coverage-increasing inputs, and compare sanitized and high-throughput non-sanitized runs. Keep harness improvements separate from production fixes.

### 11. Sanitizer and Valgrind true-positive sweep — sanitizer-valgrind
Run separate ASan+UBSan, TSan, MSan where fully instrumentable, LeakSanitizer, Valgrind/Memcheck, and relevant fuzz configurations. Exercise unit, functional, benchmark, recovery, and narrow long-running workloads rather than only startup.

For every report, minimize the command and input, identify the first invalid operation, and classify project bug, test bug, dependency bug, unsupported instrumentation, known suppression, or false positive. Search suppressions for overly broad masks and prove any new suppression is unavoidable. Fix one root cause per commit and retain a focused regression test plus the raw trace.

### 12. Static-analysis true-positive campaign — static-analysis-true-positives
Run the repository's linters plus focused clang-tidy, clang static analyzer, CodeQL/Semgrep, IWYU, compiler warnings, and semantic queries. Prioritize lifetime, nullability, use-after-move, uninitialized state, narrowing, overflow, unchecked results, span/string_view lifetime, iterator invalidation, lock contracts, dead stores, and suspicious control flow.

Use text search only to route candidates; prove them through types, call/dataflow, and execution where possible. Build custom queries from proven historical bug shapes. Reject style-only output and document false-positive patterns so later cycles do not repeat them. Each fix must preserve project idioms and include the exact warning/query path.

### 13. Secret-data lifetime and zeroization audit — secret-lifetime-zeroization
Label private keys, nonces, seeds, tweaks, blinding values, passphrases, session material, authentication data, and secret-derived temporaries. Trace every copy, allocation, move, early return, exception/error path, callback capture, log, swap, and destructor.

Verify clearing on all exits and whether the compiler eliminates it; inspect optimized assembly or use project cleanse/checkmem utilities, Valgrind/MSan secret marking, and ctime tests. Distinguish secret, public, and intentionally declassified data. Avoid broad memset churn. Prove the value is sensitive, the old lifetime is avoidably longer, and the chosen mechanism survives optimization.

### 14. Secret-dependent control-flow and memory-access audit — secret-control-flow
Starting from signing, key generation, nonce/session handling, ECDH/MuSig, encryption/authentication, context randomization, and secret scalar multiplication, trace secret taint into branches, loop counts, array/table indexes, memory addresses, helper selection, and error exits.

Compare constant-time and variable-time helpers, alternative backends, debug/VERIFY modes, and compiler output. Mark explicit declassification boundaries and challenge each one. Use ctime tests, ctgrind/Valgrind or MSan secret marking, dudect-style statistics, and assembly traces. A timing test that passes is supporting evidence, not proof; retain the dataflow argument.

### 15. Public object parsing and validation variant analysis — public-object-validation
Compare every equivalent parse/validation path for public keys, x-only keys, signatures, scalars, scripts, descriptors, records, table/log entries, addresses, and API wrappers. Seed from historical malformed-input bugs in this and related projects.

Test truncated, oversized, out-of-range, non-canonical, infinity/impossible, duplicate, and mixed-format inputs. Verify failure is consistent, non-crashing, and leaves outputs in the documented safe state. Cross-check parse/serialize round trips and operations after parse. If another implementation diverges, identify which contract is wrong rather than treating majority behavior as truth.

### 16. Public API misuse-resistance audit — api-misuse-resistance
Read public headers, examples, bindings, tests, and implementation as an adversarial caller. Look for unclear ownership/lifetime, aliasing, context capability, thread-safety, secret/public status, callback obligations, invalidation, optional-module behavior, inconsistent return conventions, and outputs usable after failure.

Construct the smallest plausible misuse example and determine whether docs, examples, types, assertions, or implementation should change. Prefer clarifying and testing the existing contract over API redesign. Check that examples demonstrate validation and cleanup. Commit implementation changes only for a concrete unsafe or ambiguous behavior with a reproducer.

### 17. Build-matrix and module-configuration audit — build-matrix-modules
Enumerate supported compilers, build types, feature modules, wallet/IPC/GUI/tool/bench/fuzz toggles, assembly/SIMD backends, debug/VERIFY/exhaustive modes, static/shared libraries, cross builds, and sanitizer combinations. Compare that inventory with CI.

Cycle through uncovered pairwise and high-risk interactions, not only individual flags. Detect sources/tests omitted under one configuration, stale guards, examples that fail, behavior-changing defaults, and generated/install manifests that drift. Use separate build directories and record native versus cross/emulated evidence. Fix one proven configuration mismatch at a time.

### 18. Exhaustive and algebraic-invariant audit — exhaustive-algebraic
Identify operations with formal identities: parse/serialize, add/subtract, multiply/invert, negate twice, normalize/idempotence, tweak relations, sign/verify, connect/disconnect, write/read/recover, insert/delete, iterator forward/backward, and cache recomputation.

State each identity and its valid domain before testing. Exercise exhaustive small domains where available and deterministic randomized properties elsewhere, including invalid inputs and failure-state guarantees. Compare optimized and reference paths. Use a temporary mutation to prove the oracle is sensitive; if a property exposes a bug, minimize the counterexample and preserve it.

### 19. Benchmark correctness and measurement-integrity audit — benchmark-integrity
Audit benchmark names, setup, timed regions, batching, units, input realism, cache state, I/O, allocation, compiler-elision barriers, fixture reuse, and secret-path representativeness. Ensure the benchmark measures the claimed production operation and validates its result.

Run release-like builds, at least five comparable repetitions, and report raw samples, median, spread, outliers, environment, and profile attribution. Check debug/sanitizer runs only for correctness. Fix misleading benchmarks separately from production optimizations and use a temporary no-op or deliberate slowdown to prove the harness detects change.

### 20. Simple micro-optimization discovery and proof — micro-optimization
Use existing benchmarks and profiles to locate a narrow hot operation. Form one hypothesis involving avoidable allocation/copy/hash/lookup/branch/serialization/lock or better reuse of an existing helper. Prefer code that becomes no more complex.

Benchmark clean base and candidate with identical release flags and at least five interleaved runs; inspect assembly or perf counters when causality is unclear. Run correctness, sanitizer, and relevant fuzz/property tests. Reject wins within noise, workload-specific regressions, and changes that weaken invariants or readability. After each result, return to the next measured hot site.

### 21. Long-running rebuild, recovery, and compaction profiling — rebuild-recovery-profile
Select a reproducible rebuild, reindex, rescan, recovery, snapshot load, index build, or compaction workload on scratch data. Record hardware, OS, compiler, flags, filesystem/storage, cache settings, data preparation, commit, and stop condition.

Capture wall/CPU time, peak RSS, reads/writes, fsyncs, compactions, progress, and perf stacks over representative phases. Classify CPU, I/O, lock, allocator, logging, serialization, crypto, or database bottlenecks. Test one minimal hypothesis, rerun identically, and require the expected profile movement as well as a reproducible metric win and correctness/recovery validation.

### 22. Full sync, IBD, import, and end-to-end profiling — full-sync-ibd-profile
Run a controlled local IBD/full-sync/import/reindex replay using fixed peers or local block data and a scratch datadir. Pin stop height/range, validation shortcuts, cache/prune/index settings, parallelism, source data, compiler, CPU policy, and storage.

Collect wall/CPU/RSS/disk/network/progress series, logs, and sampled profiles. Separate download, validation, script/crypto, chainstate, block I/O, compaction, and logging time. Do not infer a CPU or disk win from network-bound wall time. Change one bottleneck at a time and require repeated before/after runs, matching final chainstate/results, and no privacy/DoS tradeoff.

### 23. Perf and flamegraph investigation without forced commits — perf-flamegraph-investigation
Choose one representative benchmark, functional test, daemon workload, build, or recovery phase. Record exact environment and capture perf data, flamegraphs, scheduler/lock views, CPU counters, RSS, disk, network, and process metrics.

Distinguish self versus child time, on-CPU versus I/O wait, lock contention, allocator overhead, logging, serialization, hashing/crypto, database/cache, and harness overhead. Rank fix hypotheses by expected impact and risk. Commit only a trivial, proven, measured fix; otherwise leave a detailed journal with raw artifact paths, commands, call stacks, and the next experiment.

### 24. Disk I/O, persistence growth, and write-amplification audit — disk-io-amplification
Measure a fixed storage-heavy workload with process and device counters, filesystem usage, database logs, fsync traces, temporary files, and persistent-state growth. Identify redundant reads, repeated serialization, cache bypass, extra flushes, compaction amplification, stale files, excessive logs, and tests leaking artifacts.

State where data should reside: memory/cache, WAL/log, table/SST, block/index file, or durable metadata. Stop processes before corrupting files so caches cannot mask behavior. A fix must preserve crash consistency and observable state while reducing measured bytes, syncs, or retained space across repeated runs.

### 25. Recent performance-regression bisect — performance-regression-bisect
Choose a stable benchmark or controlled workload and compare a justified recent commit range under identical conditions. If a regression exceeds noise, bisect to the first bad commit, then profile last-good and first-bad with matching symbols and data.

Explain the causal code path, including changed work counts, cache behavior, allocations, I/O, locking, or compiler output. Preserve the original commit's correctness intent with the smallest fix. Require an exact bisect log, interleaved repeated measurements, before/after profiles, and relevant correctness tests. If no regression is proven, record tested ranges and move to another workload.

### 26. Bug fixed in one subsystem but present in another — cross-subsystem-bug-shapes
Mine bug-fix commits, release notes, regression tests, and review comments for reusable defect shapes. Convert each seed into structural features: source, sink, missing guard, invalid state transition, cleanup omission, accounting rule, or stale invariant.

Search analogous parsers, caches, indexes, queues, state machines, and APIs using call/dataflow or semantic queries rather than names alone. Prove each site is independently reachable and not protected differently. Mirror the seed's minimal reproducer or mutation. Keep seed provenance and explain why the omission is or is not intentional.

### 27. Error-path partial-state mutation audit — error-path-state
Find functions returning bool/status/result/optional, throwing, or using output parameters while mutating objects, caches, maps, counters, files, transactions, indexes, or caller-visible buffers. Enumerate every failure edge and state changed before it.

Infer the failure contract from docs, callers, nearby APIs, and tests: unchanged, zeroed, rolled back, invalidated, or explicitly partial. Inject each failure at the earliest and latest practical point. Add tests that compare complete pre/post state, not just the return value. Fix only proven contract violations and verify retry/restart behavior.

### 28. Weak-test oracle and mutation-survival audit — weak-test-oracles
Search tests that only assert success/no-crash/non-null/non-empty, ignore returns, catch broadly, check logs instead of state, duplicate implementation logic, rely on sleeps, or never test negative cases. Identify the exact behavior each test claims to protect.

Use Mull or temporary source mutations: invert/remove the key branch or call, alter a bound, skip a state update, or corrupt a result. If the test still passes, add the smallest property/postcondition that kills the mutant. Prioritize consensus, crypto, persistence, wallet, networking, and recent fixes. Record equivalent or unkillable mutants separately.

### 29. Dead code, stale feature, and TODO archaeology — dead-stale-code
Inventory uncalled functions, impossible branches, unused parameters/enums/options, obsolete compatibility paths, dormant macros, duplicated implementations, stale tests/docs, and TODO/FIXME/XXX comments. Check every supported build/module and distinguish production from test/fuzz/bench-only reachability.

Use history and linked PRs to decide whether code is intentionally staged, retained compatibility, or genuinely dead. Remove only with call-graph/build/coverage proof, or move harness-only helpers to test support. For TODOs, verify the premise still exists and search whether later work solved it elsewhere. Do not convert harmless defensive checks into cleanup commits.

### 30. Security-sensitive and misleading logging audit — security-logging
Trace secrets, private metadata, peer/network identifiers, wallet details, paths, auth/config values, raw payloads, and potentially attacker-controlled strings into logs and errors. Also audit severity/category, rate, repetition, truncation, escaping, and claims that misdescribe actual state.

Classify each value as secret/private/public/intentionally disclosed and each message as user-actionable, operational, debug, or unreachable. Reproduce exact output and volume. Fix concrete leaks, injection/confusion, amplification, or wrong diagnostics; do not merely rewrite prose. Validate redaction and that useful correlation remains.

### 31. Cross-layer docs, examples, tests, and implementation audit — cross-layer-contracts
Select one externally meaningful feature at a time and compare its complete contract across source comments, public docs, examples, tests, API schemas, help, release notes, and implementation. Unlike the source-comment campaign, focus on contradictions between layers and copied claims.

Build a contract table for inputs, outputs, defaults, failure behavior, compatibility, lifetime, security, and performance. Use blame/PR discussion to identify the authoritative layer. Fix the smallest proven mismatch and add a behavioral test where the contract was implicit. Record merely unclear wording without committing it.

### 32. Whole-history incomplete-fix and migration mining — whole-history-leftovers
Walk history in manageable ranges, prioritizing security, correctness, parser, persistence, crypto, locking, resource, API migration, and regression-test commits. For each seed, extract the pre-fix shape and intended repository-wide rule.

Search current HEAD for surviving old shapes, including tests, docs, examples, generated files, bindings, build manifests, and optional modules. Use blame to avoid applying obsolete rules. Prove reachability and equivalence before fixing. Journal the last inspected commit/range so later sessions resume rather than restart, eventually covering all history.

### 33. External vulnerability and advisory variant analysis — external-vulnerability-variants
Continuously collect relevant CVEs, advisories, security commits, OSS-Fuzz issues, compiler/sanitizer cases, and bugs in Bitcoin nodes, crypto libraries, databases, parsers, and bindings. Record exact affected version, patch, exploit shape, and trust boundary.

Translate each seed into a structural query and test local reachability from network, RPC/config, file/database, crypto input, module, or build/release boundary. Check existing mitigations before claiming impact. Build a minimal local vector. Fix only true local variants; for remote-only bugs produce a responsible, report-ready summary and keep local behavior unchanged.

### 34. Uncovered-code classification and closure audit — uncovered-code-classification
Generate current unit, functional, RPC, and fuzz coverage and process uncovered regions systematically. Classify each as platform/config-only, hard error path, missing scenario, harness artifact, genuinely dead, or unreachable because of a bug.

For important behavior, identify the production entry and add the narrowest assertion-rich test. For dead code, prove absence across supported builds and history. For harness artifacts, repair the harness rather than production. Maintain a line/range ledger so repeated runs advance through the tree. Reject tests whose only effect is increasing executed lines.

### 35. Mutation-testing campaign — mutation-testing
Run focused mutation testing on high-risk modules and recent changes. Include condition inversions, removed calls/state writes, arithmetic/operator changes, boundary shifts, return-value substitutions, and error-path omissions. Use Mull where practical and targeted temporary mutations elsewhere.

Classify survivors as weak oracle, equivalent mutant, unreachable code, wrong test selection, or potentially missing behavior. Kill valuable non-equivalent mutants with minimal property assertions or reveal a production bug. Track mutation score by subsystem but optimize for dangerous survivors, not percentage. Re-run mutations after each test change and preserve exact mutant identifiers/output.

### 36. Cross-tool sanitizer and static-analysis matrix — cross-tool-analysis-matrix
Build a matrix spanning GCC/Clang versions, ASan, UBSan subchecks, TSan, MSan, LSan, Valgrind, `_GLIBCXX_ASSERTIONS` or equivalent hardening, clang-tidy/static analyzer, and project lint jobs. Exercise representative unit, fuzz, functional, benchmark, and recovery paths.

Look for defects visible only under one optimizer/compiler/tool combination and for suppressions or disabled checks that create blind zones. Minimize and independently confirm every report. Fix project/test bugs only; document dependency/tool issues with versions. Continue filling matrix cells and prioritize untested high-risk configurations.

### 37. Build dead-zone and conditional-compilation audit — build-dead-zones
Map every `#if`, feature macro, platform/compiler guard, source-list condition, test skip, and CI exclusion to configurations that make it true and false. Identify code no supported build compiles, code compiled but never tested, and high-risk combinations absent from CI.

Use preprocessor output, compile databases, CMake traces, and cross/emulated builds. Check guard polarity, stale feature detection, declaration/definition parity, and release/package inclusion. Fix only demonstrated dead zones or unintended exclusions. Record unsupported combinations distinctly so they are not mistaken for project contracts.

### 38. Failure cleanup and crash-safety audit — failure-cleanup-crash-safety
Audit operations that acquire memory, locks, files, sockets, handles, transactions, temp files, background work, or durable state and can fail or be interrupted. Model authoritative state, commit point, rollback behavior, retry, startup recovery, and cleanup on all exits.

Inject failures before/after each meaningful mutation or durable write. Test clean failure, abrupt termination, and restart where relevant. Verify no leaked resource, mixed in-memory/on-disk state, stale marker, double action, or falsely advanced progress. Prefer exposing a narrow deterministic fault hook over sleeps or probabilistic timing.

### 39. Generated-artifact and test-vector determinism audit — generated-artifact-determinism
Inventory generated headers/tables, chain parameters, snapshots, test vectors, docs, schemas, source lists, and codegen outputs. Find the generator, pinned inputs/tool versions, and documented command for each.

Regenerate in a clean locale/timezone and, where practical, a second compiler/OS. Explain every diff: stale artifact, unstable ordering, timestamps, locale, randomness, dependency drift, or undocumented manual edits. Fix generator and artifact together only when inseparable. Require byte-identical repeat generation and a zero-diff verification command.

### 40. Independent multi-agent disagreement and adjudication audit — multi-agent-adjudication
Use separate roles: scout proposes a candidate and evidence without a fix; verifier reproduces it, searches prior findings, and locks confirmed/dismissed/inconclusive; fixer sees only confirmed evidence and produces the minimal change; reviewer attacks both proof and patch against project precedent.

Record disagreements, changed verdicts, and which missing instrument would resolve them. A second model or agent family is preferred for verification. Never let the existence of a patch influence whether the finding is real. Continue with rejected candidates as useful negative knowledge and periodically retest assumptions after relevant code changes.

### 41. History archaeology from a seed topic — history-seed-archaeology
Choose one seed topic per cycle, such as secret cleanup, constant-time behavior, lock annotations, resource limits, cache/recovery invariants, migrations, fuzz regressions, or API lifecycle. Use `git log --grep`, `-S`, `-G`, blame, PRs, and release notes to reconstruct its evolution.

Extract each historical rule, rejected alternative, and follow-up. Search current source/tests/docs/build files for old shapes and unimplemented implications. Name seed commits in findings and explain why the historical constraint still applies. Journal completed topic/time ranges and immediately choose the next highest-risk topic.

### 42. CI, coverage-bot, and review-bot follow-up audit — ci-review-bot-followup
Collect current branch CI logs, sanitizer/fuzz failures, static-analysis findings, coverage deltas, flaky-test evidence, and review-bot annotations. Map each result to the exact live line and commit, accounting for stale runs and rebases.

Reproduce locally or in the closest documented environment. Classify true bug, missing test, infrastructure issue, dependency/tool defect, stale warning, or style noise. Search whether reviewers already resolved or rejected it. Fix only meaningful current issues, keeping author commits intact unless explicitly asked, and preserve links plus raw output in the journal and commit body.

### 43. Option and API lifecycle audit — option-api-lifecycle
For recently touched or suspicious options/APIs, follow creation through registration, parse, validation, storage, observation, scheduling, persistence, restart, migration, disablement, and removal. Cover startup/runtime, first-run, shutdown, reindex/import, offline retry, optional-feature, and non-primary modes.

For periodic or random triggers, expose deterministic due/force hooks and test both pure predicate arithmetic and lifecycle behavior. Check duplicate scheduling and edge-trigger consumption. Prefer no persisted bookkeeping when harmless replay is intended. Fix only an observable lifecycle mismatch with exact command/output proof.

### 44. Secret-copy and compiler-optimization audit — secret-copy-optimization
Find secret-bearing structs, arrays, scalars, and buffers copied by value, returned, inlined, captured, swapped, spilled, or retained in tests/benchmarks. Pay special attention to forced-inline macros, aggregate assignments, ABI copies, vectorization, and cleanup moved across optimization.

Compare `-O0/-O2/-O3`, GCC/Clang, LTO, and relevant architectures using optimized assembly and checkmem tools. Trace every physical and semantic copy and all exits. Reduce copies or clear them only when the secret lifetime and compiler behavior are proven. Re-check performance and constant-time properties after changes.

### 45. Constant-time boundary and declassification audit — constant-time-declassification
Map functions reachable from secret operations and mark every variable secret, public, or declassified. Audit where secret-derived values cross into variable-time helpers, logging, error reporting, table lookup, loop bounds, branches, or public outputs.

For each declassification, state why the value is already public or safe to reveal and whether failure/success itself leaks information. Compare backends, optional modules, VERIFY/exhaustive modes, and compiler output. Narrow overly broad declassification and add ctime/checkmem coverage. Preserve an explicit dataflow proof even when dynamic tools pass.

### 46. Public API output-on-failure audit — api-output-on-failure
For every exported function with output parameters or caller-visible object mutation, document return convention and failure-state contract: unchanged, zeroed, invalidated, partially specified, or guaranteed initialized. Compare header docs, examples, bindings, tests, and implementation.

Exercise malformed inputs, invalid contexts/capabilities, aliasing, invalid tweaks/keys/signatures, callback failure, and module-specific errors. Pre-fill outputs with sentinels and verify exact post-state. Fix implementation only when it violates the supported contract; otherwise make docs/tests explicit. Check that callers never consume unspecified output.

### 47. Build-system and CI parity audit — build-ci-parity
Compare CMake, remaining alternate build/package systems, presets, CI setup scripts, install/export manifests, examples, benches, fuzzers, exhaustive/ctime tests, optional modules, and cross-platform jobs. Label native, cross, emulator, and artifact-only evidence.

Find flags or files present in one path but absent in another, tests silently skipped, different defaults, stale generated lists, and package/install omissions. Validate each side in separate clean directories. Fix only parity defects where the project claims equivalent support; document intentional asymmetry and its review precedent.

### 48. Property, exhaustive, and algebraic oracle expansion — property-oracle-expansion
Review unit, randomized, exhaustive, fuzz, and integration tests for operations that check success but not the strongest relation. Add identities, inverse/replay, idempotence, canonicalization, monotonicity, ordering, state-recompute, and failure-no-mutation properties over the broadest cheap domain.

For secp256k1, use the exhaustive small group, module tests, Sage-derived relations, and backend comparisons. For Bitcoin, use serialization, script/sighash, coins cache, mempool/package, connect/disconnect, index, and wallet state models. Prove each oracle with a targeted mutation and avoid duplicating production logic.

### 49. Critical whole-history must-fix sweep — critical-history-sweep
Progress from initial commit to HEAD in recorded ranges, inspecting only reachable critical defects: UB/memory corruption, untrusted crash/DoS, consensus divergence, funds/key/privacy loss, data corruption, parser failure, race/deadlock, secret leakage, or omission of a critical check.

For each historical change, ask whether its old bug shape, partial migration, or review concern survives on current HEAD. Prove present reachability and severity from first principles. Skip cleanup, minor docs, and nice-to-have tests. Journal range checkpoints so repeated sessions eventually cover every commit without restarting.

### 50. Fuzz Introspector blocker and complexity audit — fuzz-introspector-blockers
Use Fuzz Introspector or equivalent call-tree analysis to compare static reachable complexity with dynamic coverage for every target. Rank blockers by the amount and risk of code hidden behind them, not merely branch count.

Trace each blocker to input structure, checksum/magic, global state, early validation, polymorphism, environment dependence, or harness construction. Decide whether to add a dictionary/custom mutator, structured input, realistic setup, new target, or no change because validation is the subject. Show before/after reachability, coverage, hit counts, and preserved determinism.

### 51. Invariant, differential, and metamorphic audit — differential-metamorphic
Enumerate pairs of equivalent implementations and inverse/state relations: fast/reference, old/new, scalar/SIMD, C/assembly, parser/serializer, incremental/recompute, batch/split, apply/revert, write/recover, iterator directions, and alternative libraries.

Define the shared domain and permitted differences before comparing. Feed identical deterministic vectors, normalize outputs, and isolate side effects. Use three-way or formal oracles when neither side is authoritative. Minimize divergences, identify which implementation or contract is wrong, and retain both the vector and a mutation proving oracle sensitivity.

### 52. Integer overflow, narrowing, signedness, and division audit — integer-arithmetic-audit
Trace externally derived and state-derived integers into additions, multiplications, shifts, divisions/modulo, negation, casts, allocations, indexes, offsets, time arithmetic, resource accounting, and serialization widths. Search zero divisors, `INT_MIN/-1`, signed overflow, unsigned wrap, truncation, and implementation-defined shifts.

Write the mathematical range and platform-width assumptions. Test 32/64-bit and Windows-relevant types, sanitizer integer subchecks, and exact boundary vectors. Prefer checked arithmetic or domain types only when they simplify a proven bug. Include null/empty and zero-count paths that can turn later arithmetic invalid.

### 53. Statistical timing-side-channel campaign — timing-side-channel
Select one secret-bearing primitive or API boundary per cycle. Construct two or more input classes differing only in a secret property, randomize execution order, control CPU noise, gather enough samples, and run dudect-style Welch tests plus checkmem/assembly analysis.

Investigate cache, branch predictor, variable-time arithmetic, error exits, allocator, and compiler/architecture effects. Repeat across optimizers and relevant CPUs. Treat a significant result as a lead requiring mechanism proof, and a non-significant result as non-proof. Fix only when secret dataflow and measurable behavior identify a concrete leak.

### 54. RAII, smart-pointer, and resource-leak audit — raii-resource-leaks
Map ownership of heap objects, file/socket handles, locks, transactions, threads, callbacks, scheduler work, iterators, snapshots, and external resources. Search raw-pointer escapes, shared_ptr cycles, reference captures, moved-from misuse, custom deleters, destruction-order assumptions, and early-return leaks.

Use LSan/Valgrind and deterministic lifecycle tests including construction failure, cancellation, shutdown, and restart. Prove the exact ownership cycle or dangling window; do not modernize pointers mechanically. Prefer the smallest RAII/lifetime correction and verify destruction order, callbacks, and thread joins.

### 55. Alternative-implementation compatibility-difference audit — alternative-implementation-diff
Mine Bitcoin Knots, btcd, libbitcoin, rust-bitcoin, bitcoinj, gocoin, libwally, OpenSSL/BoringSSL, noble/rust secp libraries, RocksDB/Pebble, and relevant forks for fixes and distinguishing tests. Define the local consensus, encoding, crypto, persistence, or recovery contract first.

Build minimal shared vectors and run each implementation/version. Policy, API, and tuning differences are not bugs unless they feed the shared contract. If local behavior is wrong, add a regression and minimal fix. If another project is wrong, preserve local code and produce a report-ready vector, logs, affected versions, and likely reporting channel.

### 56. Stale PR critical-fix resurrection audit — stale-pr-resurrection
Search closed, abandoned, draft, superseded, and stale open PRs/issues for claimed consensus, funds, remote DoS/crash, wallet/key, data-corruption, crypto, or recovery bugs. Record why each stalled, review objections, proposed tests, and later related work.

Reproduce the claim independently on current HEAD; never resurrect the old patch blindly. If still critical, design the smallest current-style fix and explain how it avoids prior objections. If fixed, noncritical, or under-proven, record decisive evidence. Continue through PR ranges with checkpoints and prioritize unmerged regression tests and credible reproducer discussions.

### 57. Local-reasoning domain and relationship audit — local-reasoning-contracts
For one function/class/module per cycle, write its legal input domain, preconditions, postconditions, ownership graph, lock requirements, invalidation rules, failure recovery, and persistence authority. Then ask whether a reviewer can verify those locally or must rely on hidden caller folklore.

Target owner/observer, iterator/container, snapshot/database, cache/backend, context/object, callback/session, registry/member, active-chain/index, and wallet/key relationships. Find callers that violate hidden assumptions or docs/tests that promise more. Prefer explicit checks/tests/comments within existing design; commit only concrete invalid-state, lifetime, or recovery defects.

### 58. Exact helper reuse and minimal helper-extension audit — helper-reuse
First find duplicate code exactly covered by an existing helper: setup, parsing/serialization, result conversion, cleanup, locking, formatting, fixtures, builders, and assertions. Prove equivalence across inputs, errors, side effects, ownership, locks, diagnostics, and ordering by inlining the helper mentally or mechanically.

Only after exact reuse, consider one minimal parameter/overload/hook that immediately replaces multiple real duplicates or one high-risk block. Reject abstractions that hide case-specific meaning, weaken tests, cross layers, or exist only to reduce lines. Benchmark compile/runtime effects where relevant and remove newly dead code atomically.

### 59. C/C++ supply-chain and security-gate audit — supply-chain-security-gates
Audit vendored/subtree/submodule code, depends manifests, hashes, patches, download URLs, toolchain/container pins, generated sources, CI actions/images, release signing, binary verification, SBOM/provenance, scanner configuration, and workflow permissions.

Trace each trusted artifact or gate from untrusted input to build, test, release, or install decision. Check cache poisoning, untrusted-fork execution, shell/path/env injection, stale allowlists, ignored vendor/generated paths, and vulnerable dependency reachability. Add no security theater: each change must block a demonstrated bad artifact, leaked secret, unsafe workflow, or false verification result.

### 60. Historical reviewer-preference mining and reusable review skill — reviewer-preference-skill
Mine diverse merged, closed, abandoned, contentious, and high-impact PRs in each subsystem. Extract actual review comments, maintainer decisions, ACK/NACK rationale, requested evidence, commit-stack preferences, and post-merge follow-ups.

Classify every rule as general, subsystem-specific, reviewer/author-specific, contextual, stale, or one-off taste. Encode durable items as trigger + review question + evidence links + non-goals/counterexamples. Validate against held-out historical PRs and update the journal's reviewer map. Continue until PR ranges and major reviewers are covered, revisiting rules when project practice changes.

### 61. Stateful contract-fuzzer expansion — stateful-contract-fuzzing
Upgrade one existing fuzz target at a time from no-crash to a deterministic state-machine checker. Read production entry points, tests, comments, callers, and history; define valid states, malformed inputs that must fail safely, and legal transitions.

Add cheap pre/postconditions, recomputation, inverse/replay, idempotence, ordering, accounting, output-on-failure, and differential oracles. Extend operation sequences and ordering without unrealistic constraints. Run sanitizer and high-throughput builds using qa-assets and external corpora; preserve minimized seeds and coverage gains. Distinguish production, harness, oracle, and nondeterminism bugs.

### 62. Rejected-finding resurrection and assumption attack — rejected-finding-resurrection
Collect candidates previously dismissed as unreachable, theoretical, test-only, root-only, mitigated, or irrelevant. Treat each dismissal as a falsifiable claim. List its assumptions about bounds, call order, state, configuration, platform, permissions, and trust boundary.

Attack one assumption at a time using widened fuzzing, crafted RPC/config/database/IPC/network fixtures, sanitizer runs, deterministic interleavings, corpus transfer, and historical states. Require a realistic boundary, not an artificial harness alone. Confirm rejection only after documented falsification attempts fail; otherwise fix the proven root cause and state severity without inflation.

### 63. Loupe-style scout, verifier, fixer, and reporter pipeline — loupe-style-pipeline
Run a four-stage pipeline inspired by Project Loupe. Scout agents enumerate severity-ordered candidates and must search prior findings first. For each candidate they produce a regression-test PoC or exact fixture, not a patch. A separate verifier checks applicability and reproduction, then irrevocably records confirmed/dismissed/inconclusive before seeing a fix.

Only confirmed findings reach a fixer, which creates the smallest patch. A final reviewer reruns proof, checks regressions and project precedent, and prepares a report/commit. Track leases or ownership so parallel agents do not duplicate files. Give Bitcoin-focused scouts access to a spec/history index such as bkb-mcp when available. Require semantic and hash deduplication, preserved PoCs, and status transitions in the journal.

### 64. Finding deduplication, recurrence, and semantic-fingerprint audit — finding-dedup-recurrence
Build a durable index of findings and rejected hypotheses using code path, trust boundary, bug shape, source-to-sink relation, affected versions, reproducer hash, and semantic summary. Before new work, search exact hashes, symbols, nearby commits, and semantic descriptions.

When a candidate matches an old finding, determine whether it is duplicate, recurrence after refactor, incomplete variant, or changed reachability. Link rather than restate duplicates. Periodically replay old regression inputs and queries on new HEADs to detect recurrence. Use this index to select genuinely new surfaces and to prevent agents from repeatedly rediscovering attractive false positives.

### 65. Contributor-branch and work-in-progress radar — contributor-branch-radar
Identify active contributors from recent PRs, reviews, commits, and subsystem ownership, then fetch their public GitHub branches/forks without altering upstream refs. Inventory branch purpose, base, divergence, touched contracts, tests, benchmark evidence, dependencies, and overlap with local work.

Read associated PRs/issues and record each contributor's style and unresolved review concerns. Look for upcoming migrations, fixes, optimizations, test ideas, and conflicting assumptions that should inform local plans. Never copy unpublished work without provenance. Flag branches containing independently reproducible bugs, useful seeds, or likely merge conflicts, and refresh the radar over time.

### 66. Cherry-pick, backport, and release-branch correctness audit — backport-correctness
Audit maintenance/release branches and downstream backports for missing prerequisite/follow-up commits, conflict-resolution mistakes, reordered dependencies, stale generated files, wrong version guards, and tests that passed only on master. Compare patch-id, range-diff, blame, and semantic behavior rather than hash alone.

Prioritize consensus, serialization, wallet, database, network, and security fixes. Reproduce the original bug and expected fix on every supported target branch; compare binaries/tests and upgrade/downgrade paths. Identify incorrect cherry-picks in either local or downstream projects and provide minimal corrective commits with exact ancestry.

### 67. Release-to-release behavioral and consensus differential — release-version-differential
Select adjacent and strategically distant released versions plus current HEAD. Feed identical blocks, transactions, scripts, RPC/config cases, wallets, databases, indexes, and network transcripts, normalizing intentionally changed output.

Use release notes, BIPs, and migration code to classify expected differences before judging them. Focus on undocumented consensus/validation drift, acceptance/rejection changes, state serialization, recovery, error semantics, and performance cliffs. Bisect unexpected divergence, test upgrade/downgrade/restart, and preserve portable fixtures. Continue across version pairs with a ledger.

### 68. Architecture, endianness, word-size, and ABI parity audit — architecture-abi-parity
Run or cross/emulate relevant x86_64, arm64, arm32, i686, big-endian, Windows, macOS, Linux, and FreeBSD configurations. Compare 32/64-bit widths, char signedness, alignment, packing, endian conversions, atomics, filesystem/socket APIs, time types, and serialized outputs.

Use QEMU or supported CI images where native hardware is unavailable and distinguish compile-only from executed evidence. Feed identical deterministic vectors and compare results, sanitizer traces, and performance where meaningful. Find architecture-specific UB, truncation, unaligned access, stale assembly selection, and platform-only skipped tests.

### 69. SIMD, assembly, and portable-reference backend differential — backend-differential
Inventory scalar, SIMD, assembly, hardware-accelerated, and reference implementations for hashes, crypto, field/scalar/group math, checksums, memory operations, and codecs. Force each backend independently and feed identical boundary, random, and malformed inputs.

Compare exact outputs, error behavior, aliasing support, state mutation, constant-time expectations, and architecture feature detection. Run sanitizers where possible and inspect fallback behavior on unsupported CPUs. Benchmark only after correctness. Minimize divergences and determine whether optimized code, reference code, dispatcher, or test oracle is wrong.

### 70. Compiler, optimization, LTO, PGO, and BOLT differential — compiler-optimization-differential
Build with supported GCC/Clang versions at `-O0/-O1/-O2/-O3/-Os`, debug/release, LTO, PGO/SamplePGO, and BOLT where practical. Run identical deterministic correctness suites and representative workloads; compare outputs before treating speed as evidence.

Look for UB exposed only under optimization, miscompiles, missing barriers/cleanses, altered constant-time behavior, profile instability, and code-size/startup/cache tradeoffs. Use assembly/IR, perf counters, and compiler reducers to isolate anomalies. Adopt PGO/BOLT only with reproducible training workloads, held-out validation, measured gains, and no correctness or portability regression.

### 71. Deterministic simulation and failure-schedule exploration — deterministic-simulation
Build or extend a deterministic scheduler/environment around one stateful subsystem, inspired by FoundationDB simulation: seeded time, randomness, network/disk outcomes, task ordering, retries, and shutdown events. Run production logic through interchangeable test interfaces where feasible.

Define invariants over final state, progress, durability, and resource bounds. Generate aggressive but reproducible schedules, record every choice, shrink failures, and replay seeds exactly. Combine failures rather than testing one in isolation. Avoid a parallel fake implementation that stops exercising production code. Each cycle adds a new workload, fault class, or invariant.

### 72. Filesystem, power-loss, and crash-consistency injection — filesystem-crash-consistency
Map durable boundaries for chainstate, wallets, indexes, block files, settings, logs, manifests, snapshots, and generated state. Use controlled process kills, short writes, ENOSPC/EIO, dropped/corrupted writes, truncation, reorder assumptions, permission changes, and tools such as dm-flakey on scratch devices.

Enumerate crash points before/after write, flush, rename, fsync, metadata update, and in-memory commit. Restart and verify authoritative state, idempotent recovery, no false progress, and bounded repair. Test multiple filesystems where semantics matter. Never corrupt live data; preserve exact fault schedule and disk image/fixture.

### 73. Network fragmentation, reordering, and partial-I/O state-machine audit — network-state-machine
Exercise transports and protocol parsers with fragmented/coalesced reads, short writes, EOF at every byte, delayed or reordered messages where transport permits, duplicate messages, reconnects, half-close, backpressure, address-family differences, and zero/maximum payloads.

Model handshake and peer states explicitly. Check memory/accounting, timeout resets, permission transitions, message framing, cleanup, and no processing after disconnect. Use deterministic socket shims or packet transcripts instead of sleeps/public networks. Compare legacy and new transports plus OS socket behavior, preserving minimized sequences.

### 74. Memory pressure, OOM, allocator, and fragmentation audit — memory-pressure-allocator
Profile steady-state and peak heap, allocation counts/sizes/lifetimes, fragmentation, caches, arenas, stack use, and retained capacity under sync, reindex, mempool, wallet, RPC, fuzz, and adversarial inputs. Vary allocator, memory limit, and thread count.

Inject allocation failure where contracts permit, plus realistic cgroup/RLIMIT pressure. Check overflow before allocation, graceful failure, cleanup, cache accounting, retry storms, and whether memory returns after workload. Distinguish allocator behavior from leaks. Optimize only measured hot allocation patterns or proven excessive retention, with RSS/heap profiles and correctness tests.

### 75. Build throughput, dependency graph, and container-cache audit — build-throughput-cacheability
Measure clean, incremental, no-op, and parallel builds with Ninja/CMake timing, compiler traces, header/include cost, generated steps, linker time, Docker layer reuse, and CI cache hit/miss behavior. Find unnecessary rebuild fan-out, unstable generated files, broad headers, serialized custom commands, poor job pools, and cache keys tied to irrelevant inputs.

Make the smallest dependency/build-script/container change. Prove no missing dependency with clean and randomized parallel builds, and no stale result after touching each true input. Report wall/CPU/RSS/cache-size effects over repeated runs. Do not trade correctness or developer clarity for tiny build wins.

### 76. Reproducible binaries, Guix, and toolchain-provenance audit — reproducible-builds
Rebuild release artifacts from clean environments, across supported hosts/architectures where documented, using pinned dependencies and Bitcoin Core Guix/depends or the project's equivalent. Compare hashes and use diffoscope-style analysis for differences.

Trace timestamps, paths, locale, ordering, archive metadata, toolchain drift, generated files, signing, and host contamination. Verify dependency hashes and source provenance, and distinguish reproducible unsigned payloads from signatures/packaging. Fix the narrow source of nondeterminism and rerun independently. Record exact toolchain/container commits and artifact hashes.

### 77. Symbolic execution and bounded-model-checking campaign — symbolic-model-checking
Select small, high-risk pure or state-machine kernels with bounded inputs: arithmetic, parsers, encoders, cache transitions, queues, crypto helpers, and failure cleanup. Build focused CBMC or KLEE harnesses with explicit assumptions matching production domains.

Assert memory safety, no division/shift UB, postconditions, output-on-failure, algebraic identities, and equivalence to a reference. Treat bounds and environment stubs as part of the proof and attack them with concrete tests. Convert counterexamples into regression vectors. Never claim an unbounded proof; document unwind completeness and unsupported constructs.

### 78. Compiler-transformation validation and miscompile isolation — translation-validation
For suspicious optimization-dependent behavior or critical arithmetic, capture pre/post LLVM IR and use Alive2 where supported to check refinement. Also compare GCC/Clang versions and optimization levels on deterministic vectors, especially code with overflow assumptions, aliasing, shifts, bit tricks, and constant-time masking.

If validation is inconclusive, reduce the function and use differential execution or generated UB-free cases. Determine whether source UB, compiler bug, inline assembly contract, or test error is responsible. Fix local UB rather than coding around a compiler unless project support requires it; produce a compiler-report reproducer for remote bugs.

### 79. Fuzz-corpus stewardship, minimization, and transfer audit — fuzz-corpus-stewardship
Inventory corpora by target, source commit, coverage, size, runtime, flakiness, and sanitizer dependence. Merge and minimize with exact target/build versions; remove true duplicates without losing features, and identify oversized seeds that dominate execution.

Cross-seed structurally related targets and import public qa-assets/bitcoinfuzz corpora with provenance. Re-run old crashers and regression inputs on current HEAD. Preserve inputs that add stable coverage or encode important semantics, not random bulk. Track corpus coverage/time trends and submit project-appropriate improvements with deterministic reproduction.

### 80. Fuzz-engine and property-framework differential — fuzz-engine-differential
Run selected targets under libFuzzer, AFL++, Honggfuzz, and a property framework such as FuzzTest where integration is practical. Keep target semantics and initial corpus comparable while using each engine's strengths: dictionaries, value profiles, CMP tracing, custom mutators, parallelism, and high-throughput modes.

Compare coverage growth, unique paths, crash classes, execution rate, memory, and corpus quality over fixed CPU budgets and repeated seeds. Transfer discoveries between engines and reproduce all failures in a sanitizer build. Change harnesses only for engine-neutral realism unless a documented adapter is required.

### 81. Specification, test-vector, and formal-model drift audit — spec-vector-drift
Map code and tests to BIPs, protocol documents, secp256k1 module docs, Sage scripts, Wycheproof vectors, and other authoritative specifications. Pin document/vector versions and record ambiguous or intentionally policy-specific areas. Use bkb-mcp or another Bitcoin knowledge index when available, but preserve the primary document/commit links it resolves.

Regenerate or import relevant vectors, then test valid, invalid, edge, and historical cases. Search for rules implemented but untested, tests copied from obsolete drafts, and comments that overstate a spec. When implementations disagree, derive the expected result from the exact rule. Update vectors/tests or code only with traceable provenance and compatibility analysis.

### 82. secp256k1 field and scalar representation matrix — secp-field-scalar-matrix
In libsecp256k1, compare 5x52 versus 10x26 field code, 4x64 versus 8x32 scalar code, normal versus VERIFY, exhaustive groups, inversion variants, and supported compiler/architecture paths. Extract magnitude, normalization, limb, carry, overflow, aliasing, and input-domain contracts from headers and verification code.

Generate boundary elements at every allowed magnitude and scalar edge; run add/mul/sqr/negate/inverse/normalize/serialize relations and cross-backend exact comparisons. Inspect 32-bit arithmetic carefully. Use exhaustive/random/property tests, UBSan, optimized assembly, and temporary bound violations to prove oracle sensitivity.

### 83. secp256k1 group, ecmult, and formula-parity audit — secp-group-ecmult
Audit affine/Jacobian conversions, addition/doubling, infinity handling, wNAF/window tables, generator and arbitrary-point multiplication, endomorphism paths, batch inversion, and exhaustive-group formulas. State input domains and exceptional cases for each helper.

Compare optimized formulas with a simple reference and exhaustive small-group results across window sizes, backends, and VERIFY builds. Test aliasing and malformed internal states only where contracts define them. Search Sage derivations and historical formula fixes for variants. Benchmark changes only after algebraic parity and constant-time classification are proven.

### 84. secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit — secp-nonce-session
Model ECDSA/Schnorr signing, nonce generation, key tweaks, ECDH, extrakeys, and MuSig nonce/session/partial-signature transitions. Label secret/public inputs, single-use state, commitment binding, context capability, callback failure, and output-on-failure guarantees.

Test invalid order, reuse, duplicate participant/key, zero/overflow scalar, malformed serialization, cancellation, randomized context, and deterministic replay. Cross-check formal equations and other implementations without treating them as oracle. Run exhaustive/module/ctime/checkmem tests, preserve minimal sequences, and prioritize nonce reuse or partial-state bugs.

### 85. Bitcoin consensus mutation-score and kill-test audit — bitcoin-consensus-mutation
Target consensus and script-validation code plus serialization feeding it. Apply focused temporary mutants: invert checks, shift activation boundaries, skip flags, alter sighash/script limits, accept non-canonical forms, remove cache-key inputs, or perturb amount/sequence/time arithmetic.

Run unit, functional, fuzz, and available consensus vectors. Any surviving non-equivalent mutant is a critical oracle gap: add the smallest test that distinguishes behavior without duplicating implementation. Prove the mutant is reachable and the expected outcome from consensus rules/history. Never commit mutants or casually alter consensus behavior.

### 86. Bitcoin chainstate, reorg, prune, and index crash-symmetry audit — bitcoin-chainstate-symmetry
Model block connect/disconnect, flush, reorg, restart, prune, snapshot/background chainstate, block/undo files, and every BaseIndex-derived index. Track active tip, flushed chainstate, locators, best blocks, file positions, cache flags, and durable commit points.

Generate same-height and unequal-height reorgs, pure disconnects, interrupted writes/flushes, pruning races, stale children, index lag/ahead states, and restart at each transition. Assert connect/disconnect and replay symmetry, no locator advancement past durability, recoverable indexes, and matching queried results. Use scratch datadirs and deterministic fault hooks.

### 87. Bitcoin mempool, package, and eviction-accounting audit — bitcoin-mempool-accounting
Build a state-machine model for transaction/package acceptance, replacement, ancestor/descendant tracking, fee deltas, clusters, expiry, trimming, conflicts, and removal for block/reorg. Compare incremental indexes/counters with full recomputation after every operation.

Fuzz operation sequences around limits, overlapping packages, rejected transactions, RBF, orphan-like states, reorg reinsertion, and memory pressure. Assert graph symmetry, no stale links, exact resource accounting, deterministic ordering where promised, and unchanged state on failure. Seed from historical mempool DoS/accounting fixes and alternative-node divergences.

### 88. Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit — bitcoin-wallet-recovery
Map wallet transactions and durable boundaries for creation, encryption/passphrase change, master keys, descriptor/key encryption, keypool/top-up, address reservation, migration, backup/restore, rescans, and external signers. Identify which memory and database state is authoritative.

Inject database write/erase/commit failures and crashes at every step, then restart and verify no mixed plaintext/encrypted state, missing master key, unusable descriptor, silent key loss, duplicate address reservation, or memory-only success. Test legacy/descriptor and SQLite/BDB-supported paths as applicable, with scratch wallets and deterministic KDF/test clocks.

### 89. Bitcoin P2P transport, permission, and peer-accounting audit — bitcoin-p2p-accounting
Model connection lifecycle across inbound/outbound/manual/feeler/block-relay, v1/v2 transports, permissions, handshake, message processing, discouragement/ban, quotas, download state, and disconnect. Track bytes, queues, in-flight blocks, timeouts, service flags, and peer-manager/net state ownership.

Generate fragmented messages, invalid order, duplicate handshakes, permission changes, partial sends, stalls, reconnects, address-family variants, and shutdown races. Assert bounded queues/work, consistent accounting, no stale permissions/state after disconnect, and no assertion from untrusted input. Compare versions and alternative nodes only on shared protocol rules.

### 90. Whole-PR and commit knowledge-base recipe synthesis — historical-knowledge-recipes
Progress through every commit and PR in recorded ranges and extract reusable technical knowledge, not only reviewer taste: invariant introduced, bug shape, rejected design, benchmark method, test fixture, platform caveat, migration rule, follow-up, and final rationale.

Store concise recipes keyed by subsystem and trigger: when a future change touches X, inspect Y, run Z, and avoid W. Link primary evidence and mark stale/version-limited rules. Validate recipes on held-out PRs by performing dry reviews and checking whether they recover real historical comments or bugs. Deduplicate and revise rather than endlessly appending folklore.

### 91. Compiler and binary-hardening configuration audit — compiler-binary-hardening
Audit supported release and developer builds for warnings-as-errors policy, stack protection, FORTIFY, PIE/RELRO/NOW, CFI, SafeStack or platform equivalents, control-flow protections, `_GLIBCXX_ASSERTIONS`, hardened libc modes, integer/implicit-conversion sanitizers, and linker diagnostics.

For each missing or disabled mechanism, determine threat model, platform support, performance/size impact, dependency compatibility, and whether it catches a concrete project-relevant mutation or fixture. Inspect final binaries, not just flags. Add no checkbox hardening: require a demonstrated failure blocked or diagnostic gained, plus build/test/benchmark evidence across supported targets.

### 92. ABI layout, alignment, aliasing, and object-lifetime audit — abi-alignment-aliasing
Search packed structs, unions, reinterpret/static casts, placement new, memcpy of nontrivial objects, over-aligned types, custom allocators, spans over raw storage, strict-aliasing assumptions, pointer provenance, lifetime extension, and C/C++ ABI boundaries.

Compare sizes/offsets/alignment under compilers, architectures, optimization, sanitizers, and shared/static builds. Exercise unaligned buffers and aliasing permutations without invoking invalid inputs outside the contract. Use TypeSanitizer/UBSan, assembly, and small layout tests. Fix concrete UB or ABI mismatch, avoiding broad wrapper churn.

### 93. Allocation, syscall, clock, randomness, and callback fault injection — system-fault-injection
Create narrow deterministic hooks or wrappers to fail allocations, opens, reads/writes, fsync/rename, socket operations, thread creation, entropy, clocks, scheduler callbacks, database writes, and user callbacks at the Nth operation. Use production call paths and scratch resources.

Sweep failure points and assert rollback, cleanup, retry bounds, diagnostics, and restart state. Include short/partial success where APIs permit, not only hard failure. Minimize failing schedules and ensure hooks are test-only or match existing infrastructure. Continue across subsystems and turn uncovered error paths into durable regression tests.

### 94. Bindings, FFI, and language-wrapper parity audit — bindings-ffi-parity
Compare C/C++ public APIs with maintained Rust, Python, Java, Go, C#, JNI, or other bindings used in the ecosystem. Audit widths, signedness, ownership, lifetimes, nullability, callbacks, exceptions/status mapping, thread safety, buffer lengths, secret cleanup, feature flags, and output-on-failure.

Build shared vectors and misuse cases, including 32-bit and malformed input. Determine whether divergence is in core, wrapper, generated bindings, or documentation. Do not change core to accommodate a broken wrapper unless the core contract is genuinely unsafe. Produce remote report-ready reproductions for binding-only defects.

### 95. Database-engine and persistence-semantics differential — database-semantics-differential
Compare the project's LevelDB usage and assumptions with upstream LevelDB history plus RocksDB, Pebble, or alternative backends as bug seeds. Focus on comparator ordering/stability, snapshots, iterators, batches, WAL/MANIFEST recovery, checksums, filters, compaction boundaries, deletes/overwrites, sync semantics, and corruption handling.

Build engine-neutral operation traces and crash/corruption fixtures, then state allowed implementation differences. Verify Bitcoin wrappers do not rely on undocumented backend behavior. If divergence proves a local wrapper/assumption bug, fix it; if another engine is wrong, document it separately. Measure performance only after semantic parity.

### 96. TODO, FIXME, stub, and deferred-work challenge audit — todo-deferred-work
Enumerate TODO/FIXME/XXX, disabled tests, expected failures, unimplemented branches, temporary compatibility code, placeholder returns, and comments promising future cleanup. Link each to origin commit/PR/issue and determine whether its premise, risk, and owner still exist.

Try to turn every item into a falsifiable current question: hidden bug, missing coverage, obsolete workaround, blocked design, or safe intentional debt. Search for later commits that solved it elsewhere. Fix only concrete current defects or remove demonstrably stale markers; otherwise enrich the journal with exact blockers and a review-ready next experiment.

### 97. C and C++ defect-taxonomy sweep — cpp-defect-taxonomy
Cycle systematically through well-known defect classes so attractive areas do not crowd out basic bugs: null dereference, division/modulo by zero, use-before-init, use-after-free/move, double free, invalid destruction order, dangling view/reference/iterator, out-of-bounds, signed/unsigned wrap, shift UB, strict aliasing, data race, deadlock, missed virtual destruction, exception/error leaks, recursion/stack exhaustion, format mismatch, and unchecked result.

For each class, combine semantic search, compiler/tool diagnostics, historical examples, and boundary tests. Trace real reachability and reject pattern-only matches. Maintain a class-by-subsystem coverage grid and continue with the highest-risk unchecked cell.

### 98. Floating-point edge values, sanitizer resurrection, and fuzz-exclusion audit — float-sanitizer-fuzz-exclusions
Cycle through three linked passes.

1. Floating-point edge values. Inventory float/double inputs and conversions in production APIs, RPC/JSON/config parsing, GUI, bindings, tests, benches, and tools. Call them with `+0`, `-0`, smallest/largest subnormals, values adjacent to every checked boundary, max finite, `+inf`, `-inf`, quiet/signaling NaNs, varied NaN payloads, and decimal overflow/underflow. Check comparisons, integer casts, ordering/hashing, serialization, formatting, clamping, division, and containers under relevant compilers, rounding modes, and optimization flags. Prove whether each value must be rejected, normalized, propagated, or ignored; never introduce floating point into consensus or secret-dependent crypto.

2. Sanitizer resurrection. Find `no_sanitize`, suppressions, excluded targets, platform/compiler skips, disabled CI jobs, recover modes, and omitted sanitizer categories. Recover the original reason, then re-enable one diagnostic at a time in an isolated clean build. Minimize the first true positive. Remove a suppression only after its cause is fixed or proven absent; never replace it with a broader allowlist.

3. Fuzzer exclusions. Inspect catches, ignored errors, early returns, clamps, `Assume` gates, and exceptional-value special cases. Temporarily remove or invert one at a time. If this crashes, preserve/minimize the input and reproduce through the real production boundary. Classify it as a valid precondition, harness bug, unchecked expected exception, or production defect. For legitimate exceptions, assert outputs and state are unchanged, rolled back, zeroed, or otherwise safe. Never commit blind removal of a necessary guard.
