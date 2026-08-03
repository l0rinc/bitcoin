# Secret-dependent control-flow and memory-access audit

## Cycle 320 handoff

- Goal: 14, `secret-control-flow`.
- Branch: `uber-cycle-320-secret-control-flow-20260802`.
- Base at selection: `6631878f49453c8857104accc914fc3661392ff8`.
- Selection commit: `e59a47659d`.
- Working tree contained only the pre-existing untracked agent artifacts before this cycle.
- Scope: current libsecp256k1 secret-bearing entry points, with emphasis on failure status, declassification, and constant-time ctime coverage for MuSig, ECDH, ElligatorSwift, Schnorr, Silent Payments, and scalar/group multiplication.

## Prior evidence and exclusions

Earlier cycles already covered the main valid ECDSA, ECDH, Schnorr, MuSig, ElligatorSwift, and Silent Payments callers with Clang 19 ctime/MSan, paired timing probes, portable/assembly comparisons, and source mutation controls. Existing journals also cover the ECDH invalid-scalar ctime test, MuSig public-key and public-nonce declassification boundaries, EllSwift auxiliary-input ordering, invalid ECDH public-key propagation, and MuSig nonce validation ordering. Those cells were not repeated as findings.

## Hypotheses

1. The MuSig nonce generator's invalid secret-key status could reach a branch or secret-indexed access even though the valid path was already covered by ctime.
2. The newer MuSig failure paths might lack a regression oracle capable of detecting a future secret-dependent branch.
3. The current source's explicit declassification boundaries might be incomplete in one of the secret-to-public output transitions.

## Source review

The relevant implementation in `src/secp256k1/src/modules/musig/session_impl.h` parses secret-key validity into `ret`, derives both nonce scalars through fixed loops, saves the secret nonce, and passes `!ret` to `secp256k1_musig_secnonce_invalidate`, whose `memczero` is constant-time. Public nonce data and the return status are declassified only at their public boundaries. `secp256k1_musig_nonce_gen` checks all-zero session randomness with a constant-time scan, declassifies the invalid-input status, and branches only on that documented invalid public result. The scalar multiplication path uses fixed loops and conditional table moves. The ECDH, EllSwift, Schnorr, and Silent Payments paths showed the same pattern on the reviewed callers: secret scalars are handled by constant-time arithmetic, while public or documented invalid-input results are declassified before branching.

## New ctime coverage experiment

The existing `src/secp256k1/src/ctime_tests.c` covered successful MuSig nonce generation and counter generation, but did not mark an invalid MuSig secret key undefined and exercise the failure path. A focused test-only extension created a valid public key/cache, marked a zero secret key and nonzero session randomness undefined, called `secp256k1_musig_nonce_gen`, defined only the public output/status, and asserted `ret == 0`.

Build and run:

```text
TMPDIR=/data/my_storage/tmp/cycle320-ctime-tmp cmake --build /data/my_storage/tmp/cycle288-secp-msan --target ctime_tests -j2
MSAN_OPTIONS=halt_on_error=1:exit_code=86:report_umrs=1:print_summary=1 /data/my_storage/tmp/cycle288-secp-msan/bin/ctime_tests
```

The first run built `[1/2]` and `[2/2]`, exited 0, and produced an empty log. The restored run after the mutation also built successfully, exited 0, and produced a zero-byte log. `git diff --check` passed.

## Independent oracle sensitivity

For the temporary control, the single production call
`secp256k1_musig_secnonce_invalidate(ctx, secnonce, !ret)` was replaced with an ordinary `if (!ret) ... else ...` branch. With the same ctime test and MSan options, the build succeeded but `ctime_tests` exited 86. The first diagnostic was:

```text
MemorySanitizer: use-of-uninitialized-value
secp256k1_musig_nonce_gen_internal ... session_impl.h:422:9
ctime_tests ... ctime_tests.c:271:15
```

Restoring the production call and rebuilding returned status 0 with no diagnostic. This proves the added ctime case can detect the intended class of regression; the mutation was not retained.

## Candidate ledger

| Candidate | Verdict | Evidence | Change |
| --- | --- | --- | --- |
| MuSig invalid secret-key path has a current secret-dependent branch | Dismissed | Source review, current ctime pass, and a branch mutation caught by MSan | None |
| MuSig invalid secret-key path lacks ctime coverage | Confirmed as a test gap | Existing harness had no undefined invalid-seckey call; focused case passes and kills the branch mutation | Add the focused ctime regression case |
| New production declassification/control-flow defect in reviewed callers | Dismissed | Current source dataflow plus restored MSan ctime pass; no first-invalid operation | None |

## Commit decision

The smallest useful change is the ctime test extension only. It does not alter production behavior, makes the invalid MuSig secret-key contract observable to MSan/Valgrind ctime, and is independently sensitive to a deliberately introduced secret branch. The test does not claim to prove constant-time behavior; it is a regression oracle for secret-dependent control flow and memory use.

## Limitations and next queue

- Evidence is x86_64 Linux with Clang 19 MSan, `SECP256K1_ASM=OFF`, Debug/O1, and the existing module selection. Valgrind, dudect, ARM, 32-bit, big-endian, and full optimization matrices were unavailable or already covered in separate cells.
- The test uses an invalid zero secret key with a valid public key. It does not cover every public argument failure or every malformed serialized MuSig object; those are separate API-contract cells.
- The MSan build was limited to `ctime_tests`; no large rebuild was needed or justified.
- Next: add a catalog goal for secret-input failure-path ctime coverage across crypto APIs, including invalid scalar, zero/session, malformed secret-state, and output-on-failure cases. Then continue with a fresh selector and re-rank this goal if a new caller or declassification boundary appears.
