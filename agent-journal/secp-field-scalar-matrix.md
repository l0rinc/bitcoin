# secp256k1 field and scalar representation matrix

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
