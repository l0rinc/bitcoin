# Constant-Time Boundary Cycle 59

## Identity and Gate

- Cycle: `59`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `45`
- Goal: `Constant-time boundary and declassification audit`
- Slug: `constant-time-boundary`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `3b4c458d37861c084d074becaae27e4228fd07d0`
- `origin/master...HEAD` at the gate: `2 887`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- `goals.tsv` validation: `validated_rows=99 total_lines=100`
- No relevant test, fuzz, sanitizer, daemon, or profiling process was running at the gate.

Cycle 40 closed the EllSwift `secp256k1_ellswift_create` auxiliary-input boundary after an MSan ctime failure. This cycle is a distinct current-source cell: MuSig session/keypair transitions and their explicit public declassification. The EllSwift finding and cycle-44 MuSig counter-public-key fix are negative controls, not candidates for duplicate repair.

## Cycle 59 Hypotheses

1. A MuSig session or keypair helper declassifies an intermediate before all secret-dependent processing has finished, allowing a secret-bearing value to influence a branch or memory access after the claimed boundary.
2. A public output is declassified too late or too broadly, causing checkmem to report a false secret use, or too early, allowing a secret-derived value to cross the boundary before its documented public representation is complete.
3. A ctime test marks a composite secret/public object with a boundary that does not match the API contract, leaving a reachable caller path untested.

For each candidate I will trace the public header contract, declassification helper, secret/public object layout, caller lifecycle, and checkmem tests. A finding requires a first-invalid-operation diagnostic or a precise source-level taint proof, plus a negative control showing the existing boundary is intentionally public. Any source change must be limited to one independent contract defect and carry a focused ctime or API regression.

## Cycle 59 Evidence Log

- Static inventory: `secp256k1_musig_session` is explicitly public in `secp256k1_musig.h`; `secp256k1_musig_session_load` therefore has no secret boundary to declassify. `secp256k1_musig_secnonce` keeps both scalar nonces secret, while `secp256k1_musig_secnonce_invalidate` declassifies only the four-byte magic and the 64-byte public-key encoding after its constant-time wipe. `secp256k1_musig_secnonce_load` declassifies only `is_zero`, the documented invalidation status used by `ARG_CHECK`, not either nonce scalar.
- The nonce generator declassifies `ret` only before branching on an all-zero `session_secrand32` invalid-input condition. After that boundary, secret-derived validity is used only by constant-time `memczero` and returned to the caller. `secp256k1_keypair_load` declassifies the public half of the opaque keypair immediately before `secp256k1_pubkey_load` performs its argument check; `secp256k1_keypair_seckey_load` declassifies only its public validity result. `secp256k1_musig_nonce_gen_counter` separately declassifies the public key copied out of a keypair before passing it to the nonce generator, as fixed and tested in cycle 44.
- Live baseline: rebuilding `/data/my_storage/tmp/constant-time-boundary-cycle40-msan-1` and running `MSAN_OPTIONS=halt_on_error=1:exit_code=86:report_umrs=1 .../bin/ctime_tests` exited successfully with no diagnostic. `cmake --build build_unit_wallet_clang19 --target tests exhaustive_tests -j2` passed; `build_unit_wallet_clang19/src/secp256k1/bin/tests` completed silently with `iterations = 16`, `jobs = 0`, and exit 0; `exhaustive_tests` exited 0. `git diff --check` passed and the tracked tree had no modifications.
- Independent mutation 1: removing `secp256k1_declassify(ctx, pubkey, sizeof(*pubkey))` from `secp256k1_keypair_load` and rerunning the MSan ctime binary failed at `secp256k1_pubkey_load` (`secp256k1.c:261`) through `keypair_load` and `keypair_xonly_tweak_add`. Restoring the line returned ctime to success.
- Independent mutation 2: removing `secp256k1_declassify(ctx, &is_zero, sizeof(is_zero))` from `secp256k1_musig_secnonce_load` failed at the `is_zero` check (`session_impl.h:63`) through `secp256k1_musig_partial_sign`. Restoring the line returned ctime to success.
- Independent mutation 3: removing the nonce-generator `ret` declassification and temporarily adding a ctime call with a secret-marked all-zero session random buffer failed at the invalid-input branch (`session_impl.h:451`). Restoring both the production line and temporary control returned ctime to success. This confirms the branch is an invalid-input status boundary, not an accidental exposure of nonce material.
- Valgrind is unavailable locally. The current evidence is a source-level taint/dataflow proof, three first-invalid-operation MSan mutations, a restored MSan baseline, regular secp tests, and exhaustive order-13 coverage.

## Cycle 59 Verdict

- Dismissed for a new defect; no source change justified. The selected MuSig/keypair cells have explicit, minimal declassification boundaries and each tested mutation fails at the expected public validation branch. The previous EllSwift cell and cycle-44 counter-public-key repair remain closed negative controls.

## Cycle 59 Handoff

- Close with a journal-only handoff authored as `Lőrinc <pap.lorinc@gmail.com>`. The next selector must draw a fresh goal from `0..98`; do not reopen this cycle's cells without a new caller, compiler/backend, architecture, Valgrind/dudect, or changed API-contract signal.

---

# Constant-Time Boundary Cycle 40

## Identity and Gate

- Cycle: `40`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `45`
- Goal: `Constant-time boundary and declassification audit`
- Slug: `constant-time-boundary`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `4d8df13cea2fea8917d6e305862979862b948448`
- `origin/master...HEAD` at the gate: `2 847`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- `goals.tsv` validation: `validated_rows=99 total_lines=100`
- No relevant test, fuzz, sanitizer, daemon, or profiling process was running at the gate.

The cycle-39 `setlabel` RPC return-value lead remains a separate queued hypothesis. It was not used to select or merge this EllSwift finding.

## Hypothesis

`secp256k1_ellswift_create` promises constant-time behavior in both `seckey32` and optional `auxrnd32`, but its checkmem boundary declassifies the SHA-256 state after hashing only the secret key and zero padding. If `auxrnd32` is secret, its bytes taint the hash state again before the variable-time ElligatorSwift search. The search uses hash-derived branch values in `xswiftec_inv_var`, so the current boundary could expose secret-dependent control flow to MSan/Valgrind and violate the public API contract.

## Contract and Dataflow Review

- `src/secp256k1/include/secp256k1_ellswift.h:129-144` states that `secp256k1_ellswift_create` is constant time in `seckey32` and `auxrnd32`, but not in its public-key output.
- `src/secp256k1/src/modules/ellswift/main_impl.h:452-461` constructs `H(privkey || zero32 || auxrnd32 || counter)`, then calls `secp256k1_ellswift_elligatorswift_var`.
- Before this cycle, `main_impl.h` declassified `hash` immediately after `seckey32` and `zero32` and before the optional `auxrnd32` write. `hash_impl.h` contains no implicit `CHECKMEM_DEFINE`; its state remains tainted across `sha256_write`.
- `main_impl.h:343-366` draws a three-bit `branch` from hash output and calls `secp256k1_ellswift_xswiftec_inv_var`; `main_impl.h:198` branches on that value. This is intentionally variable-time with respect to the public output, so the input-to-hash boundary must be correct.
- `src/secp256k1/src/checkmem.h` maps `CHECKMEM_UNDEFINE`/`DEFINE` to MSan and Valgrind, and `ctime_tests.c` only runs when one of those mechanisms is active.
- The existing EllSwift ctime case passed `ellswift` as both output and auxiliary input after defining the output, so it did not test a secret auxiliary buffer. This was a harness gap, not evidence that the API allowed public `auxrnd32`.
- Blame/history places the original EllSwift implementation in `901336eee75` and the current hash-write lines in subtree update `dfd54c959ef`; no review record was found that narrows the header contract to public auxiliary randomness.

## Independent Verification

Two forms of evidence agreed:

1. Static contract/dataflow review: the public header requires constant-time `auxrnd32`, while the only explicit declassification precedes the auxiliary write and the subsequent search branches on hash-derived data.
2. MSan ctime execution: a fresh Clang 19 build was configured with `-fsanitize=memory -fPIE`, `-DSECP256K1_BUILD_CTIME_TESTS=ON`, all default modules including EllSwift, and no Valgrind. The ctime harness was temporarily extended to copy a defined auxiliary buffer, mark it undefined, and pass it separately from the output buffer.

The unpatched implementation produced:

```text
==410251==WARNING: MemorySanitizer: use-of-uninitialized-value
    #0 ... secp256k1_ellswift_xswiftec_inv_var ... main_impl.h:198:9
    #1 ... secp256k1_ellswift_xelligatorswift_var ... main_impl.h:365:13
    #2 ... secp256k1_ellswift_elligatorswift_var ... main_impl.h:376:5
    #3 ... secp256k1_ellswift_create ... main_impl.h:461:5
    #4 ... run_tests ... ctime_tests.c:284:11
SUMMARY: MemorySanitizer: use-of-uninitialized-value ...
Exiting
```

The command used was:

```text
MSAN_OPTIONS=halt_on_error=1:exit_code=86:report_umrs=1 /data/my_storage/tmp/constant-time-boundary-cycle40-msan-1/bin/ctime_tests
```

This is a first-invalid-operation diagnostic, not a timing inference. It occurs at the branch that consumes the tainted hash-derived `branch` value.

## Fix

Move `secp256k1_declassify(ctx, &hash, sizeof(hash))` below the optional `auxrnd32` write and update its comment to cover both secret inputs. Extend the ctime test with a separate `auxrnd[32]` buffer initialized before tainting, then mark both `key` and `auxrnd` undefined for the auxiliary-input call. This preserves the documented public-output declassification while making the input boundary explicit.

## Validation

Fresh MSan build/configuration:

```text
CC=clang-19 cmake -S src/secp256k1 -B /data/my_storage/tmp/constant-time-boundary-cycle40-msan-1 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=memory -fno-omit-frame-pointer -g -fPIE' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=memory -pie' -DSECP256K1_BUILD_BENCHMARK=OFF -DSECP256K1_BUILD_TESTS=OFF -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=OFF -DSECP256K1_BUILD_CTIME_TESTS=ON -DSECP256K1_ENABLE_MODULE_ELLSWIFT=ON
cmake --build /data/my_storage/tmp/constant-time-boundary-cycle40-msan-1 --target ctime_tests -j2
MSAN_OPTIONS=halt_on_error=1:exit_code=86:report_umrs=1 /data/my_storage/tmp/constant-time-boundary-cycle40-msan-1/bin/ctime_tests
```

The patched ctime command exited successfully with no MSan diagnostic. Normal validation also passed:

- `cmake --build build_unit_wallet_clang19 --target test_bitcoin -j2`
- `./build_unit_wallet_clang19/bin/test_bitcoin --run_test=crypto_tests --log_level=test_suite`: 23 cases, no errors
- `cmake --build build_unit_wallet_clang19 --target tests exhaustive_tests -j2`
- `./build_unit_wallet_clang19/src/secp256k1/bin/tests`: exit `0`
- `./build_unit_wallet_clang19/src/secp256k1/bin/exhaustive_tests`: order `13`, no problems found
- `git diff --check`: passed

## Verdict

**Confirmed and fixed.** The old boundary failed an independent MSan check when `auxrnd32` was treated as secret, and the minimal post-input declassification removes the diagnostic while matching the header contract. The source, ctime regression, and this journal are intended for one self-contained commit authored as `Lőrinc <pap.lorinc@gmail.com>`.

## Limitations and Rejected Leads

- Valgrind is not installed, so no independent Valgrind ctime run was available; MSan and static dataflow are the two verifier forms used here.
- The runtime proof uses Clang 19 on x86_64. GCC, 32-bit, other architectures, and optimized cross-toolchain behavior remain in the broader constant-time queue.
- The public-output variable-time behavior was not changed. No claim is made about unrelated declassification sites in keypair, MuSig, Schnorr, or silent-payment code; those remain separate cells for future cycles.
- The ctime harness still uses its established public-output marking for subsequent EllSwift XDH tests; the new separate auxiliary buffer specifically closes the previously untested secret-input case.

## Next Queue

1. Reopen the remaining declassification sites only with a distinct boundary hypothesis, starting with MuSig session invalidation and keypair load contracts.
2. Repeat the EllSwift ctime check under Valgrind if available, and under another compiler/backend when the environment supports it.
3. Keep the cycle-39 wallet `setlabel` return-value lead separate and do not duplicate it in this journal.
