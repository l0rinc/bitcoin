# secp256k1 field and scalar representation matrix

## Cycle 128: GCC 12 forced int128 versus int64 backend matrix

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `82`
- Selected slug: `secp-field-scalar-matrix`
- Branch: `uber-cycle-128-secp-field-scalar-matrix-20260730`
- HEAD before the cycle: `845800dfff00f9965ef6bb73092964f27b107c2b`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `1044 40` from `git rev-list --left-right --count HEAD...origin/master`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- `git fetch origin master --quiet`, tracked/index status, `git diff --check`, and the PID check all passed at the fresh gate. PID `777094` (`test_bitcoin --run_test=wallet_tests`) and its Codex parent were not touched. Existing untracked agent artifacts, `node_modules/`, package files, and `test/cache/` were preserved.

### Scope and hypothesis

Cycle 24 already covered the field/scalar edge-vector matrix under Clang 19. This cycle used the explicitly queued GCC cell: GCC 12.2.0, `SECP256K1_ASM=OFF`, and the test-only wide-multiplication overrides `int128` and `int64`. The falsifiable hypothesis was that GCC's 5x52 field plus 4x64 scalar implementation and 10x26 field plus 8x32 scalar implementation diverge on a valid boundary value, arithmetic operation, inverse path, representation invariant, or serialization result.

The source contract was re-read in `src/secp256k1/src/field.h`, `field_5x52.h`, `field_10x26.h`, `scalar.h`, `scalar_4x64.h`, `scalar_8x32.h`, `field_impl.h`, and `scalar_impl.h`. The field representation carries magnitude and normalization metadata in VERIFY builds; field multiplication restricts the third operand from aliasing; scalar inputs are reduced modulo the group order. The temporary probe respected those contracts. History showed only the expected vendored subtree updates for these representation files; no newer local fix or review precedent identified an unclosed GCC-specific defect.

### Isolated GCC build matrix

Both configurations were fresh standalone CMake/Ninja `RelWithDebInfo` trees using GCC 12.2.0, all current optional modules, assembly disabled, regular and no-VERIFY tests enabled, and exhaustive tests enabled:

```text
cmake -S src/secp256k1 -B /data/my_storage/tmp/cycle128-secp-gcc-int128 -G Ninja -DCMAKE_C_COMPILER=/usr/bin/gcc-12 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSECP256K1_ASM=OFF -DSECP256K1_BUILD_TESTS=ON -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=ON -DSECP256K1_BUILD_BENCHMARK=OFF -DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int128
cmake -S src/secp256k1 -B /data/my_storage/tmp/cycle128-secp-gcc-int64 -G Ninja -DCMAKE_C_COMPILER=/usr/bin/gcc-12 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSECP256K1_ASM=OFF -DSECP256K1_BUILD_TESTS=ON -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=ON -DSECP256K1_BUILD_BENCHMARK=OFF -DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64
cmake --build /data/my_storage/tmp/cycle128-secp-gcc-int128 --target all -j2
cmake --build /data/my_storage/tmp/cycle128-secp-gcc-int64 --target all -j2
```

All four build commands completed successfully. The paired fixed-seed runs used:

```text
<tree>/bin/tests --iterations=2 --seed=0123456789abcdef --jobs=2 --log=1
<tree>/bin/noverify_tests --iterations=2 --seed=0123456789abcdef --jobs=2 --log=1
<tree>/bin/exhaustive_tests
```

Both `tests` binaries exited 0 with every reported test passing; total execution was 27.309 seconds for int128 and 27.950 seconds for int64. Both `noverify_tests` binaries exited 0 with every reported test passing; total execution was 16.146 seconds for int128 and 15.160 seconds for int64. The test order differed between backends, but no case failed and no error, assertion, sanitizer, or abort text appeared. Both exhaustive binaries exited 0 and reported `Exhaustive tests for order 13`, `test count = 2`, and `no problems found`.

### Independent internal-header probe

The temporary `agent-journal/secp_field_cycle128_probe.c` included `int128_impl.h`, `field_impl.h`, and `scalar_impl.h`, compiled with GCC 12 and `-DVERIFY`, and was removed after the run. Its 12 deterministic inputs were zero, one, field-prime minus one, field-prime, field-prime plus one, group order, order minus one, order plus one, `2^255`, `2^255-1`, a patterned value, and `2^256-1`.

The probe exercised field `set_b32_mod`, strict-limit parsing, normalization, canonical serialization, constant-time and variable-time inversion, halving, negation, storage round trips, multiplication, squaring, and addition. It exercised scalar reduction and overflow reporting, canonical serialization, constant-time and variable-time inversion, halving, negation, multiplication, and addition. It emitted 265 lines and 22,232 bytes in each mode. `sizeof(secp256k1_fe)=48` and `sizeof(secp256k1_scalar)=32` in both modes. The complete outputs compared byte-for-byte:

```text
cmp_status=0
592291746a53f723fb511739a3635990cfbd0532070810a99753eaef8920c54d  probe-int128.log
592291746a53f723fb511739a3635990cfbd0532070810a99753eaef8920c54d  probe-int64.log
```

The boundary rows also matched: field strict-limit parsing accepted `p-1` and rejected `p`, `p+1`, and `2^256-1`; modular field parsing mapped `p` to zero and `p+1` to one; scalar parsing reported overflow at and above the group order and produced identical reduced values. Both stderr streams were empty. An initial direct compile omitted `int128_impl.h` and failed to link the internal helpers; adding that required probe include corrected the harness and did not change production code.

### Candidate ledger and verdict

| Candidate | Classification | Verdict |
|---|---|---|
| GCC int128 versus int64 differs on field/scalar boundary vectors or arithmetic | Production representation differential | Dismissed; full native suites and the 265-line canonical probe matched |
| GCC-specific VERIFY metadata or inverse path diverges | Production invariant differential | Dismissed; constant-time/variable-time inverse, normalization, storage, and metadata outputs matched |
| The first standalone probe failure indicates a libsecp defect | Scratch harness setup | Dismissed; the missing `int128_impl.h` definition was the complete cause and the corrected probe passed |

**Cycle verdict: dismissed; no confirmed production finding and no source or permanent test change justified.**

### Limitations and next queue

This cycle executed GCC 12.2.0 on x86_64 little-endian with assembly disabled and did not test 32-bit, cross-architecture, big-endian, `int128_struct`, assembly/reference parity, sanitizer builds, timing, or constant-time equivalence. A passing functional comparison is evidence of representation correctness for this matrix, not a proof of constant-time behavior. The raw GCC build trees, logs, and probe outputs remain under `/data/my_storage/tmp/cycle128-secp-gcc-*`; the temporary source was removed. Reopen this goal only for a distinct architecture, assembly, sanitizer, `int128_struct`, or new source/history cell.

## Cycle 24: forced int128 versus int64 backends

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `82`
- Selected slug: `secp-field-scalar-matrix`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD before the cycle: `91725b89ffac1e7b6875d4bd2344335277b76282`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- `git diff --check`: passed at the gate and after the cycle.
- Tracked source was clean at the gate. Existing agent artifacts and `test/cache/` were preserved.
- No daemon, test, fuzz, sanitizer, or profiling process was running at the gate.

### Scope and hypothesis

The falsifiable hypothesis was that the 5x52 field/4x64 scalar representation and the 10x26 field/8x32 scalar representation diverge on an allowed edge value or algebraic operation. The trust boundary is libsecp256k1's internal field/scalar contract as exercised by Bitcoin Core's cryptographic callers. The comparison covered zero, one, wide integer edges, field-prime edges, group-order edges, a high bit, and a patterned value.

The source contract was checked in `field.h`, `scalar.h`, `field_impl.h`, and the representation headers. VERIFY tracks magnitude and normalization, field multiplication disallows aliasing through its third operand, and scalar inputs are reduced modulo the group order. The scratch harness therefore copied the right operand for field multiplication and initialized every input before pairwise operations.

### Isolated build matrix

Two clean standalone Clang 19 RelWithDebInfo trees were configured with `SECP256K1_ASM=OFF`, regular and exhaustive tests enabled, ECDH enabled, and these explicit overrides:

```text
/data/my_storage/tmp/secp-field-matrix-int128/CMakeCache.txt:
  SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int128
/data/my_storage/tmp/secp-field-matrix-int64/CMakeCache.txt:
  SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64
cmake --build /data/my_storage/tmp/secp-field-matrix-int128 --target all -j4
cmake --build /data/my_storage/tmp/secp-field-matrix-int64 --target all -j4
```

Both builds completed successfully. The six representation-specific binaries also completed successfully:

| Backend | Binary | Result |
|---|---|---|
| int128 | `bin/exhaustive_tests` | exit 0; exhaustive order 13; no problems |
| int64 | `bin/exhaustive_tests` | exit 0; exhaustive order 13; no problems |
| int128 | `bin/noverify_tests` | exit 0; total execution `53.602 s` |
| int64 | `bin/noverify_tests` | exit 0; total execution `50.735 s` |
| int128 | `bin/tests` | exit 0; total execution `98.475 s` |
| int64 | `bin/tests` | exit 0; total execution `95.322 s` |

The matrix exercised the normal, no-VERIFY, and exhaustive paths without assembly. No representation-specific assertion, test failure, or output mismatch was observed.

### Independent edge-vector probe

The temporary probe was `/data/my_storage/tmp/secp-field-matrix-probe.c`; it was not added to the repository. It was compiled directly against the current internal headers:

```text
clang-19 -std=c99 -O2 -g -Wall -Wextra -Wno-unused-function -DVERIFY -DUSE_FORCE_WIDEMUL_INT128=1 -Isrc/secp256k1/src /data/my_storage/tmp/secp-field-matrix-probe.c -o /data/my_storage/tmp/secp-field-matrix-probe-int128
clang-19 -std=c99 -O2 -g -Wall -Wextra -Wno-unused-function -DVERIFY -DUSE_FORCE_WIDEMUL_INT64=1 -Isrc/secp256k1/src /data/my_storage/tmp/secp-field-matrix-probe.c -o /data/my_storage/tmp/secp-field-matrix-probe-int64
/data/my_storage/tmp/secp-field-matrix-probe-int128 > /data/my_storage/tmp/probe-int128.out 2> /data/my_storage/tmp/probe-int128.err
/data/my_storage/tmp/secp-field-matrix-probe-int64 > /data/my_storage/tmp/probe-int64.out 2> /data/my_storage/tmp/probe-int64.err
```

The first two probe attempts correctly exposed harness mistakes rather than library defects. The first passed the same field object as both restricted multiply operands when `i == j`; VERIFY rejected that contract. The second initialized field and scalar arrays lazily while reading all pairwise entries; VERIFY rejected the resulting uninitialized state. After copying the right operand and initializing both arrays in a separate pass, both probes exited 0, each emitted 754 canonical records and 59,012 bytes, and each emitted the same operation trace. The canonical outputs compared identically:

```text
cmp -s /data/my_storage/tmp/probe-int128.out /data/my_storage/tmp/probe-int64.out
canonical_cmp_status=0
315b522a08b0f7c0ab9fbf21a02c8244143c50d8ee911645f54c8e888db92828  probe-int128.out
315b522a08b0f7c0ab9fbf21a02c8244143c50d8ee911645f54c8e888db92828  probe-int64.out
```

The same 13 vectors were used for both representations. The field half, inverse, multiplication, addition, normalization, and canonical serialization paths were exercised; scalar half, inverse, multiplication, addition, reduction, and canonical serialization were exercised. The probe's stderr trace hashes also matched at `bdcc97cc3a3abb81c6287ea7a375917174dcf075d1e951f0e272e9a005a85e14`.

### Candidate ledger and verdict

| Candidate | Classification | Verdict |
|---|---|---|
| 5x52/4x64 versus 10x26/8x32 differs on fixed field/scalar edge vectors | Production representation differential | Dismissed; matrix and canonical probe matched |
| Field multiply aliasing is a backend defect | Scratch harness violated the documented restricted third operand contract | Dismissed; harness corrected |
| Scalar/field output mismatch from the first probe run | Scratch harness read uninitialized pairwise entries | Dismissed; two-pass initialization corrected and rerun matched |

**Cycle verdict: dismissed; no confirmed production finding and no source change justified.**

### Limitations and next queue

This cycle used Clang 19 on the current x86_64 host with assembly disabled. GCC, 32-bit or cross-architecture execution, assembly/reference parity, and timing/constant-time analysis remain separate cells. Reopen this goal only for one of those distinct configurations or for new source/history evidence. The corrected scratch source and raw outputs remain under `/data/my_storage/tmp/` for independent replay; no repository probe or test artifact was added.

The next cycle must perform a fresh gate, draw a new eligible goal, record the selector and evidence, and continue the uber loop.
