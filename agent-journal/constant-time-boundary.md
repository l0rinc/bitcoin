# Constant-Time Boundary Cycle 204

## Identity and Gate

- Cycle: `204`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `45`
- Goal: `Constant-time boundary and declassification audit`
- Slug: `constant-time-boundary`
- Selector output was recorded on the provisional gate branch before this branch was renamed for the selected goal.
- Branch: `uber-cycle-204-constant-time-boundary-20260731`
- Base: `origin/master` at `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `2637a1dd9a18eef099d50570cab8b5a751128155`
- `HEAD...origin/master` at the gate: `1198 42`
- Pre-cycle uber state SHA-256: `cd3f2d3e1d2f5573bdbdb4f03114833f432456b47614ae40b430fa13c7f894cb`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- `goals.tsv` SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Tracked/staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- `goals.tsv` validation remains `validated_rows=99 total_lines=100` from the prior gate.
- `git diff --check` passed; `src/net_processing.cpp` had no tracked diff.
- Protected unrelated processes were checked and preserved; no Cycle 203 or held-out test process remained.

This is a distinct current-source cell. Cycles 40 and 59 covered EllSwift auxiliary-input and MuSig/session/keypair declassification boundaries; cycle 44 fixed the MuSig counter public-key boundary; cycle 71 and cycle 110 covered EllSwift XDH and Silent Payments caller/timing cells; cycle 46 covered the GCC/backend/optimization timing matrix. Those cells are excluded unless new source evidence makes them recur.

## Cycle 204 Hypotheses

1. The ECDH API's invalid-secret path may have an incomplete constant-time boundary: zero or overflow scalars are replaced with one before fixed-window multiplication, but the ctime test covers only a valid secret and does not exercise the secret-marked invalid path or its public failure status.
2. The ECDH hash callback boundary may expose secret-derived intermediate state or leave the output contract under-specified when the callback returns zero, especially because the callback is invoked before the public `!overflow` result is returned.
3. The ECDH path may be correctly constant-time but its ctime harness may fail to model the documented output/status contract, creating a missing regression oracle rather than a production defect.

For each hypothesis I will trace the public header, scalar replacement, fixed-window multiplication, hash callback, output writes, cleanup, caller assumptions, and existing invalid-input vectors. I will use the current Clang 19 MSan ctime build where possible, add only scratch harness coverage unless a missing permanent oracle is proven, and require a first-invalid-operation diagnostic or independent source/dataflow proof before changing production code.

## Cycle 204 Evidence Log

- Source review of `src/secp256k1/include/secp256k1_ecdh.h:36-57` and `src/secp256k1/src/modules/ecdh/main_impl.h:34-76` found the documented contract: zero and overflow scalars return `0`, but the scalar is replaced with one before `secp256k1_ecmult_const`; the default SHA-256 callback then hashes the derived point before the status is combined with `!overflow`. The only branch on `hashfp` selects a public API argument. The invalidity bit is used by `secp256k1_scalar_cmov` and bitwise return arithmetic, not a secret-dependent branch or memory index. `secp256k1_ecmult_const` uses the fixed-window conditional-move path for the scalar. The implementation clears the derived coordinates, scalar, point, and Jacobian state before returning.
- A current-source Clang 19 MSan ctime build at `/data/my_storage/tmp/cycle110-statistical-timing/msan` was rebuilt for both `secp256k1` and `ctime_tests` after the temporary source experiments were applied and removed. It is `SECP256K1_ASM=OFF`, ECDH-enabled, and uses `-fsanitize=memory -fno-sanitize-memory-param-retval`. The restored baseline command `TMPDIR=/data/my_storage/tmp/cycle204-ctime-restored MSAN_OPTIONS=halt_on_error=1:exit_code=86:report_umrs=1:print_summary=1 /data/my_storage/tmp/cycle110-statistical-timing/msan/bin/ctime_tests` exited `0` with an empty log.
- A temporary ctime harness extension initialized an all-zero `invalid_ecdh_key`, marked it undefined, called default `secp256k1_ecdh`, defined the written output and public return, and asserted `ret == 0`. The current-source MSan run exited `0`; the resulting empty log is `/data/my_storage/tmp/cycle204-ctime-invalid/current.log`. This closes the invalid-zero input/status cell and is retained as a permanent ctime regression test in this cycle.
- Oracle sensitivity was independently checked by temporarily adding `if (overflow) output[0] ^= (unsigned char)overflow;` immediately after `overflow |= secp256k1_scalar_is_zero(&s)` in `ecdh/main_impl.h`. With the same undefined all-zero scalar, MSan exited `86` and reported the first invalid operation at `secp256k1_ecdh`, `main_impl.h:52:9`, called from the temporary ctime case. Removing the mutation, rebuilding, and rerunning restored status `0`. The production source is clean after restoration; the raw mutation report is `/data/my_storage/tmp/cycle204-ctime-overflow/overflow-branch.log` (SHA-256 `8ed48334344fc6949faa46722717ccd121ce619d7d6a97c18814c52bd985c44b`).
- A temporary custom ECDH callback that branched on `x32[0]` did not produce a diagnostic in the normal ctime build because the project deliberately appends `-fno-sanitize-memory-param-retval` for ctime tests, so an indirect callback ABI boundary does not carry the shadow state. A separate build with `-fsanitize-memory-param-retval` appended failed at the first intentional secret load in `ec_pubkey_create` before reaching ECDH, so it is not evidence about the callback. The callback receives secret-derived coordinates and remains a caller-owned constant-time obligation; no library declassification of those coordinates is justified.
- Existing independent functional evidence in `agent-journal/backend-differential.md` covers 512 deterministic ECDH vectors, default and custom hash callbacks, zero/overflow invalid scalars, callback failure, output bytes, and status. The recorded Clang 19/GCC 12 and portable/assembly comparisons all returned `vectors=512 failures=0 digest=fe288ea1ddb151fb` where the ECDH cell was enabled. This corroborates the API/status contract but is not a constant-time proof.
- Valgrind and dudect remain unavailable. No permanent callback test was added because its behavior is outside the library's control and the available ctime ABI configuration cannot observe it. The test-only invalid-scalar case is the smallest sensitive oracle for the library-owned path.

## Cycle 204 Verdict

- **No production constant-time or declassification defect confirmed.** ECDH's invalid scalar is replaced before fixed-window multiplication, the invalid status is returned through constant-time bitwise composition, and the temporary secret-dependent branch was caught by the MSan oracle. The previous harness omission was real and is fixed by one permanent invalid-scalar ctime case. No declassification of the shared point before a caller-supplied hash callback is justified; custom callbacks must preserve the caller's constant-time requirements.
- The cycle's source/test commit is authored as `Lőrinc <pap.lorinc@gmail.com>`. Remaining limitations are the unavailable Valgrind/dudect tools and inability to observe arbitrary callback parameter taint under the project's intentionally disabled MSan parameter/return instrumentation.

## Cycle 204 Handoff

- Close this cycle after the focused ctime test, libsecp256k1 tests, and diff checks pass. The next selector must draw a fresh goal from `0..98`. Reopen this ECDH cell only for a changed callback contract, a supported parameter-taint ctime configuration, Valgrind/dudect, or new source/history evidence.


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
