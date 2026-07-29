# Translation Validation Journal

## Cycle 108: optimized obfuscation transformation validation

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `78`
- Slug: `translation-validation`
- Branch: `uber-cycle-108-translation-validation-20260729`
- Cycle start HEAD: `190301f93e36503fbfed4a21bcefa38ec7be09a1`
- `origin/master` after the fresh gate: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` was `40 1005`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Fresh gate: `git fetch origin master` succeeded; tracked and index state were clean; `git diff --check` passed; the catalog hashes matched; PID `777094` and its Codex parent `725042` were present and preserved.

### Scope and exclusions

Goal 78 requires comparison of a bounded transformation across compiler/optimization variants, with source-domain and tool limitations kept explicit. Cycle 55 already covered SipHash across GCC/Clang, optimization levels, LTO, IR verification, and a fuzzer control. Cycle 105 covered broad GCC/Clang Release/IPO, manual Clang profile-use, deterministic utility outputs, and the full unit suite. Cycles 26, 44, 47, 69, 90, and 100 covered sanitizer/compiler matrices, timing/assembly, SHA256 dispatch, libsecp ECDH, and backend output parity. Those cells are excluded unless a new counterexample appears.

The new cell is the optimized `Obfuscation::operator()` in `src/util/obfuscation.h`: it peels an unaligned prefix, promises alignment with `std::assume_aligned<8>`, applies an unrolled 8-word loop, and uses fixed-width `memcpy` for a bytewise XOR transform. The production contract is exact byte-for-byte equivalence to a simple reference for every key, starting address alignment, key offset, and target size; a second application with the same key/offset must restore the original bytes.

### Hypotheses

1. The optimized alignment peel or unrolled transformation diverges from the reference at a boundary involving zero/one/eight-byte sizes, offsets modulo eight, or buffers with nonzero base alignment.
2. A compiler or LTO transformation changes the defined output or creates an alignment/aliasing diagnostic in a valid caller.
3. Any apparent divergence is instead caused by an invalid `assume_aligned` precondition, a test oracle issue, or unsupported compiler behavior; no source change is made without a minimized, reproducible valid-domain counterexample.

### Required evidence

Preserve compiler versions, flags, target CPU, source and IR/assembly snippets, deterministic input digest, output digest, reference comparison count, sanitizer diagnostics, and exact commands. Alive2 is expected to be unavailable; LLVM `opt -passes=verify` will be treated only as structural IR validation. A source fix requires a failing-before/passing-after regression or an independent UB-free counterexample. A benchmark-only difference is not a finding.

### Cycle 108 results

Tool inventory found Clang 19.1.7, GCC 12.2.0, `opt-19`, `llvm-reduce-19`, and `llvm-mca-19`. Alive2 (`alive-tv`/`alive2`) was unavailable. The selected helper's optimization history includes the alignment-peel/unrolled-body change `248b6a27c3` and later fixed-width-key refactors; this is a new helper-level translation cell, not a repeat of the Cycle 55 SipHash probe.

The direct reference probe was `/data/my_storage/tmp/cycle108-obfuscation-probe.cpp`. It generated four key classes (zero, all `ff`, alternating `aa/55`, and deterministic pseudo-random), every base offset 0 through 15, every target size 0 through 192, and every key offset 0 through 23. Each case compared `Obfuscation` to bytewise `original[i] ^ key[(key_offset + i) % 8]`, then applied the operation a second time to require exact round-trip restoration. The probe covered 296,448 cases and returned `cases=296448 checksum=4710587723396259903` for every successful build/run below:

| Compiler | Modes | Result |
| --- | --- | --- |
| Clang 19.1.7 | `-O0`, `-O2`, `-O3`, `-Os`, `-O3 -flto=thin -fuse-ld=lld` | identical case count/checksum; exit 0 |
| GCC 12.2.0 | `-O0`, `-O2`, `-O3`, `-Os`, `-O3 -flto` | identical case count/checksum; exit 0 |
| Clang 19.1.7 | `-O2 -fsanitize=address,undefined -fno-omit-frame-pointer` | identical case count/checksum; no ASan/UBSan diagnostic |

Clang 19 emitted the probe at `-O0` and `-O3` with `-S -emit-llvm -fno-inline`; `opt-19 -passes=verify -disable-output` accepted both modules. The O0/O3 modules were 5,281 and 4,759 lines respectively. The generated IR contains the expected `llvm.assume` alignment fact for the `std::assume_aligned<8>` result and fixed-width `llvm.memcpy` operations. This is structural IR validation, not an Alive2 refinement proof. No output, status, round-trip, or sanitizer divergence was found.

#### Independent header defect

The first self-contained translation unit, `/data/my_storage/tmp/cycle108-obfuscation-selfcontain.cpp`, included only `<util/obfuscation.h>` plus `<array>` and instantiated `Obfuscation`. Before the fix, both exact Clang 19 and GCC 12 `-O2` commands failed because `src/util/obfuscation.h` used `std::memcpy` at lines 67, 101, 110, and 112 without including `<cstring>`. The error was not a transitive project warning: the header could not be compiled as a direct translation unit.

The smallest fix adds `<cstring>` in the standard include block. After the fix, the same direct Clang and GCC commands compiled and ran with exit 0. The rebuilt Clang 19 Release `test_bitcoin` target completed 185 Ninja steps. `streams_tests` passed all 27 cases; `streams_tests,dbwrapper_tests` passed all 41 cases; and the full unit binary passed all 1,123 cases with exit 0 and `*** No errors detected`. The existing compiler warnings and the test-only `DIR_UNIT_TEST_DATA` skip were unchanged.

#### Verdict and limitations

The optimized obfuscation transformation is consistent with its independent bytewise oracle across the tested compilers, optimization levels, LTO modes, alignments, sizes, offsets, and Clang sanitizers. The header self-containment defect is confirmed and fixed in the source commit for this cycle; no claim is made about non-x86 execution, other compiler versions, Alive2 refinement, or the validity of callers that violate the alignment precondition. The scratch probes and IR remain under `/data/my_storage/tmp/cycle108-*`; no scratch source is staged.

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
