# Translation Validation Journal

## Cycle 55: compiler-transformation validation and miscompile isolation

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `78`
- Slug: `translation-validation`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- HEAD at the gate: `98150b59231b0e5229a229da2c3a29abd824e5fa`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` was `2 877`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Corrected actual-tab TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate status: tracked and staged state clean; only recorded agent-owned untracked paths present; no relevant process running.

The existing branch is the dedicated investigation branch. This cycle excludes prior compiler-adjacent cells: the cycle-26 sanitizer/compiler matrix, cycle-44 GCC timing screen, cycle-47 SHA256 sanitizer guard, and cycle-53 allocator/OOM policy. It will focus on a new deterministic transformation-validation cell.

### Scope ledger and hypotheses

| Surface | Expected contract | Evidence status |
|---|---|---|
| Optimized C/C++ arithmetic and bit manipulation | Defined source behavior must agree across supported optimization levels and compilers | Unchecked |
| Inline assembly and compiler barriers | Declared inputs, outputs, clobbers, aliasing, and sanitizer attributes must match generated code | Unchecked |
| Crypto and serialization hot kernels | Identical deterministic vectors and valid-domain results across IR/compiler variants | Unchecked |
| Undefined or implementation-defined source behavior | Must be identified before labeling a compiler result a miscompile | Unchecked |
| LLVM transformation validation | Alive2 or a reduced equivalent proof should distinguish refinement failure from source-domain mismatch | Tool availability pending |

Initial hypotheses are deliberately falsifiable:

1. A supported optimization transformation changes a defined result in a small arithmetic, serialization, or crypto helper.
2. A compiler-dependent result is instead explained by source UB, an invalid test input, or an inline-assembly contract violation.
3. A sanitizer/compiler exclusion hides a real optimization-sensitive defect in a path that has a deterministic production oracle.

### Required evidence

For every candidate, preserve compiler versions, flags, target CPU, pre/post LLVM IR or assembly, deterministic input, output bytes/status, sanitizer result, and the exact reduced command. A source fix requires a failing-before/passing-after test or equivalent executable counterexample. A compiler report requires a UB-free minimized reproducer and independent compiler/version evidence. An inconclusive tool run remains a journal entry rather than a source change.

### Next experiment

Inventory installed GCC/Clang/LLVM/Alive2/reducer tooling and recent optimization-sensitive history. Then choose one small, pure, high-risk helper with an existing exact oracle and compare `-O0`, `-O2`, and `-O3` outputs across available compilers before attempting broad builds. Do not infer a miscompile from a benchmark-only difference.

### Cycle 55 results

- Tool inventory: supported Clang 19.1.7 at `/usr/bin/clang++-19`, GCC 12.2 at `/usr/bin/g++-12`, Clang 14.0.6, LLVM `opt`/`llc`/`llvm-reduce` 14 and 19. Alive2, C-Reduce, Csmith, and YARPGen were unavailable. The repository documents Clang 17 and GCC 12.1 as the minimum supported versions in `doc/dependencies.md:13`; Clang 14 was therefore retained only as an expected compatibility control.
- Surface selected: `SipHasher13UJ::Hash(const uint256&)` and `Hash(const uint256&, uint64_t)` in `src/crypto/siphash.h`, compared against the independently sequenced `WriteJumbo`/`Write`/`Finalize` implementation in `src/crypto/siphash.cpp`. The probe exercised 100,004 deterministic cases, including zero, one, all-ones, alternating-byte, pseudo-random 256-bit inputs, random keys, random extra words, and a normal-block prefix before the fixed-width operation.
- Cross-compiler and optimization result: Clang 19 and GCC 12 at `-O0`, `-O1`, `-O2`, `-O3`, and `-Os`, plus Clang ThinLTO and GCC LTO, each returned `cases=100004 checksum=2796535566890446307`. No fixed-versus-generic mismatch occurred.
- LLVM evidence: Clang 19 emitted IR for the probe and `src/crypto/siphash.cpp` at `-O0` and `-O3`; `opt-19 -passes=verify -disable-output` accepted all four modules. This validates IR structure but is not an Alive2 refinement proof.
- Unsupported compiler control: Clang 14 failed during header instantiation in `src/uint256.h` at the `consteval` hex-digit constructor using the system C++12 reverse iterator implementation. This is consistent with the documented Clang minimum and is not a current-tree miscompile finding.
- Project validation: `build_unit_clang19/bin/test_bitcoin --run_test=hash_tests --log_level=test_suite` passed 4 cases with no errors. The ASan/UBSan probe returned the same checksum with no report. The existing `integer` fuzzer completed 10,000 normal runs (`cov 1546`, `ft 2135`, `corp 50/2823b`) and 2,000 Clang ASan/UBSan runs (`cov 182`, `ft 183`) with no artifact or diagnostic.
- Setup correction: the first fuzzer commands correctly failed because their explicitly requested artifact-prefix directories did not exist. The directories were created and the exact runs were repeated successfully; the setup failure is not product evidence.
- Verdict: dismissed for a new source defect in this cell. No source or production-test change is justified. The scratch probe was removed after preserving its commands and aggregate output here.
- Limitations and next queue: no Alive2 proof, no alternate architecture execution, and no new inline-assembly reducer were available. Reopen this cell only with a new compiler/backend, architecture, transformation, or minimized counterexample; otherwise draw a distinct goal from the full catalog.
