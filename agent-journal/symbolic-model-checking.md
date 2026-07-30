# Symbolic Execution and Bounded Model Checking

## Cycle 145: bounded conversion and amount kernels

### Gate and scope

- Draw command: shuf -i 0-98 -n 1
- Draw: 77
- Selected goal: symbolic-model-checking (Symbolic execution and bounded-model-checking campaign)
- Branch: uber-cycle-145-symbolic-model-checking-20260730
- Start HEAD: bd1e93b918d52ba360a4bbe686c842e49a70042e
- origin/master: 67efced1fc83a0b7215cc1513e7c4754fee0f12f
- Merge base: a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b
- Explicit git rev-list --left-right --count HEAD...origin/master: 1074 42 (HEAD-only, origin-only)
- The tracked/index tree and git diff --check were clean at the gate. PID 777094 was alive and was not touched. Catalog, prompt, TSV, and protocol hashes remained 5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8, 10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec, babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb, and 954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0.
- No prior agent-journal/symbolic-model-checking.md or repository-native CBMC/KLEE harness was present. Previous symbolic secp256k1 Sage proofs were treated as related evidence, not as a duplicate of this bounded C/C++ campaign.

### Tool and kernel selection

- cbmc and klee were absent from the host. A disposable CBMC 6.10.0 Linux package was downloaded from the Diffblue release and extracted under /data/my_storage/tmp/cycle145-cbmc; no system package or repository file was changed. The verifier reported 6.10.0 (cbmc-6.10.0).
- ConvertBits was selected as a high-risk, pure production template used by Bech32, base32, base64, key/address conversion, and decoding paths. Current call sites include ConvertBits<8,5,true>, ConvertBits<5,8,false>, ConvertBits<8,6,true>, and ConvertBits<6,8,false> in src/util/strencodings.cpp, src/key_io.cpp, and src/bench/bech32.cpp.
- ParseFixedPoint was selected as a second bounded parser kernel because it feeds amount and fee parsing and has an explicit failure-output contract.

### ConvertBits model

Scratch harness: /data/my_storage/tmp/cycle145/convert_bits_cbmc.c.

The harness transcribes the production accumulator, mask, bit-count, padding, and output-callback behavior. Its reference path materializes an independent MSB-first bitstream and regroups it, avoiding the production accumulator arithmetic. It models up to eight input symbols and sixteen output symbols, all input values for 8-bit sources, and the valid 5-bit/6-bit domains for reverse conversions.

The following command shape was run for each of ten entry points (check_8_to_5_padded, check_8_to_5_unpadded, check_5_to_8_padded, check_5_to_8_unpadded, check_8_to_6_padded, check_8_to_6_unpadded, check_6_to_8_padded, check_6_to_8_unpadded, and the two invalid-mapping checks):

  /data/my_storage/tmp/cycle145-cbmc/root/usr/bin/cbmc /data/my_storage/tmp/cycle145/convert_bits_cbmc.c --function <entrypoint> --unwind 20 --unwinding-assertions --bounds-check --pointer-check --pointer-overflow-check --signed-overflow-check --unsigned-overflow-check --conversion-check --undefined-shift-check --div-by-zero-check --no-built-in-assertions

Results for all ten entry points were exit 0, VERIFICATION SUCCESSFUL, and zero failed properties. All loop unwinding assertions passed. The checked properties covered:

- Success/failure equivalence with the independent bitstream reference for both padded and unpadded conversions in both directions.
- Exact output values and output lengths when conversion succeeds.
- Bounded output callback writes and output lengths.
- Array bounds, pointer bounds/overflow, signed and unsigned arithmetic overflow, conversion range, undefined shifts, and division-by-zero checks.
- A forced negative decode-table mapping at an arbitrary position for both 5-to-8 and 6-to-8 paths. Both models reject it and retain bounded partial output.

An earlier run with MAX_INPUT=8, MAX_OUTPUT=16, and --unwind 12 failed two unwinding assertions in the 16-slot comparison loop and one reference loop. This was an incomplete proof, not a semantic counterexample. Re-running with --unwind 20 passed all properties. The raw model remains outside the repository and is preserved at the stated path.

### ParseFixedPoint model

Scratch harness: /data/my_storage/tmp/cycle145/parse_fixed_point_cbmc.c.

parse_model follows the production parser's grammar, trailing-zero handling, exponent finalization, overflow guards, output write, and null-output behavior. reference_parse parses the grammar independently, computes a digit coefficient and scale, trims fractional trailing zeros, and evaluates the bounded integer result. The model checks success/value equivalence, the (-10^18, 10^18) bound, failure preservation of a sentinel, and equality of status with a null output pointer.

- An arbitrary-byte model with eight-byte input timed out at the resource limit. A five-byte retry also timed out after 120 seconds and is inconclusive; neither run is treated as a defect or proof.
- The reduced model uses at most three input bytes and decimals in [0,2]. With --unwind 20, all 17 properties and all unwinding assertions passed: VERIFICATION SUCCESSFUL.
- The first reduced run found "3.0" as a mismatch, but the counterexample was in the reference: it did not normalize fractional trailing zeroes while production intentionally does. Adding that normalization made the reduced proof pass. This harness correction is recorded so the result is not misreported as a production finding.

### Native and sanitizer validation

- cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j2 initially failed because the default ccache temporary directory did not exist. Re-running with CCACHE_DIR=/data/my_storage/tmp/cycle145-ccache rebuilt test_bitcoin successfully. The first failure was environment setup only.
- TMPDIR=/data/my_storage/tmp/cycle145-test-tmp /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=bech32_tests,util_tests --random=145077 --log_level=message --report_level=short --color_output=false passed 85 selected cases, 4,575 assertions.
- TMPDIR=/data/my_storage/tmp/cycle145-test-tmp /data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=base32_tests,base64_tests,bech32_tests --random=145078 --log_level=message --report_level=short --color_output=false passed all 12 selected cases and 670 assertions.
- ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 TMPDIR=/data/my_storage/tmp/cycle145-test-tmp /data/my_storage/tmp/cycle106-clang19-ubsan/bin/test_bitcoin --run_test=base32_tests,base64_tests,bech32_tests --random=145079 --log_level=message --report_level=short --color_output=false passed all 12 cases and 670 assertions with no sanitizer diagnostics.
- A direct attempt to compile a C++ harness including src/util/strencodings.h with CBMC's goto-cc failed in the installed C++ standard-library headers (c++config.h, assert.h, cstddef, and std::byte parsing). This is a CBMC 6.10.0 C++20 frontend/toolchain limitation. The production template was still represented exactly in the C model, and native current-tree tests provide the source-level behavioral check.

### Verdict and next queue

No source or permanent test change is justified. Within the stated eight-symbol conversion bounds, the production accumulator matches an independent bitstream reference, has bounded output behavior, rejects invalid decoder mappings, and passes native and sanitizer tests. The reduced amount-parser slice also passes, while broader symbolic amount coverage remains open because of solver cost.

Next distinct cells: broaden the amount model using a grammar-constrained alphabet or split properties to reduce solver size; investigate a CBMC frontend/toolchain that can compile the actual C++20 template; and extend conversion bounds or callback/error-state contracts without duplicating the completed eight-symbol cell. Preserve the scratch harnesses and exact raw commands as evidence for the next run.
