# Bindings, FFI, and language-wrapper parity

## Cycle 270: invalid Minisketch copy assignment leaves stale native state

### Selection and gate

- Exact selector after the Cycle 269 state close: `shuf -i 0-98 -n 1` -> `94` (`bindings-ffi-parity`); no reroll was needed.
- Branch: `uber-cycle-270-bindings-ffi-parity-20260802`.
- Cycle start and branch-gate HEAD: `ad31a18954843f110225714f46c2a97fb76ab901`.
- `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence (`HEAD...origin/master`): `1329 45`.
- The tracked/index state was clean and `git diff --check` passed at entry. Existing untracked agent artifacts, package files, `node_modules/`, `test/cache/`, crash files, and profiling output were preserved and excluded from staging.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Corrected TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Protected long-running tests `777094`, `956381`, `1138182`, `1157959`, `1312049`, `1312050`, and `1346200` were alive and were not touched. Disposable builds, probes, and test temporary data used `/data/my_storage/tmp`.

### Scope and exclusions

The tree does not ship maintained Rust, Python, Java, Go, or C# bindings. Prior Goal 94 cells for the libbitcoinkernel pointer-array conversion, tracing-demo field omission, callback ownership and serialization-failure behavior, nullable ancestor boundary, and the external Rust MuSig secret-nonce wrapper were searched and excluded.

The current IPC/Cap'n Proto boundary was also checked as a separate schema-parity cell. The external Rust consumer `2140-dev/bitcoin-capnp-types` was cloned at `/data/my_storage/tmp/cycle270-bitcoin-capnp-types`, HEAD `b1a9876f6563dbb802e66c20bae27dc42fca7917` (2026-07-24); its `mining.capnp` body matches the Core schema. Apparent `BlockCreateOptions` omissions were checked against history: `block_max_weight` was explicitly not exposed to IPC by commit `128da7c3ff`, and `test_block_validity` was explicitly not exposed by `020166080c`. The remaining `block_min_fee_rate`/`print_modified_fee` and 32-bit `size_t` width questions remain separate queue cells, not findings here. Rust tooling is unavailable, so the external consumer was used for static comparison only.

### Working hypothesis

`src/minisketch/include/minisketch.h` represents construction failures, unsupported field/implementation combinations, and internal OOM as an invalid object (`m_minisketch == nullptr`), and explicitly requires callers to check `operator bool()` before using it. The copy constructor preserves that invalid state by leaving its destination invalid. The copy-assignment operator instead guarded the entire assignment with `sketch.m_minisketch`, so assigning an invalid source to a valid destination silently retained the destination's old native sketch. This violates the wrapper's value semantics and can make a caller continue using stale state after an operation that intentionally propagates an invalid result.

### Independent discovery and verification

The old operator at lines 239-246 was:

```cpp
if (this != &sketch && sketch.m_minisketch) {
    m_minisketch = std::unique_ptr<minisketch, Deleter>(minisketch_clone(sketch.m_minisketch.get()));
}
```

The constructor documentation at lines 262-267 identifies invalid objects as a supported state, not an impossible input. Existing `minisketch_tests` covered valid copy construction/assignment indirectly through wrapper operations but had no invalid-source copy-assignment case.

A standalone public-wrapper probe in `agent-journal/minisketch_cycle270_probe.cpp` was compiled with `g++ -std=c++17 -O2 -Wall -Wextra -Werror` against the pre-fix library `/data/my_storage/tmp/cycle214-build/src/libminisketch.a`. It produced:

```text
source_valid=0 destination_valid=1 destination_bytes_unchanged=1
```

and returned failure because the destination stayed valid and serialized exactly as before. This is a direct pre-fix reproduction independent of the project test harness.

### Fix and verification

The operator now assigns either a cloned native handle or `nullptr` for every non-self assignment. The regression test constructs a valid sketch and a documented invalid sketch, assigns the latter, and asserts that the destination is invalid.

- Isolated build: CMake Debug, Clang 19, `BUILD_TESTS=ON`, wallet/IPC/GUI/bench/BDB/ZMQ disabled, `/data/my_storage/tmp/cycle270-minisketch-build`; `cmake --build ... --target test_bitcoin -j2` passed.
- Focused test: `/data/my_storage/tmp/cycle270-minisketch-build/bin/test_bitcoin --run_test=minisketch_tests --random=270001 --log_level=test_suite --report_level=short --color_output=false` passed 3 cases and 705/705 assertions.
- Rebuilt public-wrapper probe with the current library under Clang 19: `source_valid=0 destination_valid=0 destination_bytes_unchanged=0`, exit 0.
- Rebuilt the same probe under GCC 12 against the same current library: `source_valid=0 destination_valid=0 destination_bytes_unchanged=0`, exit 0.
- The source/test diff passed `git diff --check`; no protected process or default datadir was used.

Verdict: confirmed local C++ wrapper contract defect. The minimal fix is to clear the destination native handle when the source is invalid, and the regression is self-contained in `minisketch_tests`. This is not a consensus or cryptographic algorithm change; it prevents stale wrapper state from surviving a documented invalid value. The remaining limitation is that the test does not inject clone-time OOM or exercise 32-bit ABI widths; the invalid-source path itself is a normal documented state and is directly reproduced.

Next queue: continue with the remaining IPC schema-width/output-on-failure and wrapper lifetime cells, searching this entry and the prior Goal 94 cells before selecting a new hypothesis. Do not reopen invalid copy assignment unless a distinct recurrence appears in another maintained boundary.

## Cycle 208: external Rust MuSig secret-nonce exposure

### Selection and gate

- Exact selector after the Cycle 207 state close: `shuf -i 0-98 -n 1` -> `94` (`bindings-ffi-parity`); no reroll was needed.
- Branch: `uber-cycle-208-bindings-ffi-parity-20260731`.
- Cycle start: `2026-07-31T11:53:55Z`.
- Branch gate: `2026-07-31T11:54:13Z`.
- Cycle start and branch-gate HEAD: `a34a9fede73301ed7263285e2231a246bf419bdf`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence (`HEAD...origin/master`): `1207 42`.
- Uber-goal state SHA-256 at the gate: `e8eecf6ed575c396d9d0b2bc359fe581cf4af06cc6ddd2c4f9cc494f034ab7a1`.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Corrected TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- The four protected long-running tests (`777094`, `956381`, `1138182`, `1157959`) were alive and were not touched. Disposable external-repository work used `/data/my_storage/tmp`.

### Scope and exclusions

The tree does not ship maintained Rust, Python, Java, Go, or C# bindings. Its in-tree wrapper surface remains the `libbitcoinkernel` C API and C++ `btck` wrapper. Cycle 70's opaque pointer-array conversion, Cycle 87's tracing-demo field omission, Cycle 186's callback ownership and serialization-failure analysis, Cycle 198's nullable ancestor boundary, and Cycle 72's direct C MuSig failure/state-machine probe were searched and excluded as prior cells.

To exercise a maintained ecosystem boundary, I cloned `rust-bitcoin/rust-secp256k1` at `/data/my_storage/tmp/cycle208-rust-secp-fPGt4P`:

- Remote: `https://github.com/rust-bitcoin/rust-secp256k1.git`.
- HEAD: `9fc86f756f0cd24a5167c131551b31b748c8cdb3` (`2026-02-28`, `Merge rust-bitcoin/rust-secp256k1#895: Introduce radnomness in ElligatorSwift encoding`).
- Its `secp256k1-sys` vendored revision is `a660a4976efe880bae7982ee410b9e0dc59ac983`.
- The external wrapper was used as evidence only; no external checkout or unrelated repository artifact is staged in Bitcoin Core.

### Working hypothesis

The libsecp256k1 C header marks `secp256k1_musig_secnonce` as a secret opaque value that MUST NOT be copied or read because copying it can reuse a nonce and leak the signing key. The Rust high-level wrapper correctly makes `SecretNonce` non-`Copy`/non-`Clone`, but its `Debug` implementation and the low-level FFI type may bypass that boundary. Verify the complete trait and byte-access path, construct a report-ready safe-Rust misuse, and distinguish an external-wrapper defect from an in-tree Bitcoin Core defect.

### Cycle 208 verification and result

The candidate is confirmed as an external-wrapper contract violation with two connected exposures:

1. `secp256k1-sys/src/lib.rs:1448-1458` defines public `MusigSecNonce` with `Copy`, `Clone`, `PartialEq`, and `Eq`, and provides `dangerous_into_bytes` as a safe method. `impl_array_newtype!` additionally gives it safe `AsRef<[u8; 132]>` and indexing access. This directly conflicts with the vendored C contract at `secp256k1-sys/depend/secp256k1/include/secp256k1_musig.h:47-57`, which says the secret nonce must not be copied or read and explains the nonce-reuse key leak.
2. `src/musig.rs:675-677` derives `Debug` for the high-level `SecretNonce`. That delegates to `MusigSecNonce`'s `impl_raw_debug!` at `secp256k1-sys/src/lib.rs:1451`; the macro at `secp256k1-sys/src/macros.rs:68-79` iterates over every byte and writes it as hexadecimal. Therefore `format!("{secret_nonce:?}")`, logging, or a panic containing a `SecretNonce` exposes all 132 bytes of secret nonce state. The crate's own secret-display policy at `src/secret.rs:52-61` says secrets should not implement `Debug` directly, and the nearby `SecretKey` implementation deliberately prints only a short hash.

The shortest report-ready high-level reproduction is:

```rust
let key = SecretKey::from_secret_bytes([1u8; 32]).unwrap();
let public = PublicKey::from_secret_key(&key);
let session = SessionSecretRand::assume_unique_per_nonce_gen([2u8; 32]);
let (secret_nonce, _public_nonce) = new_nonce_pair(session, None, Some(key), public, None, None);
println!("{secret_nonce:?}");
```

`new_nonce_pair` returns the `SecretNonce` at `src/musig.rs:181-188`, and the derived formatter follows the raw-byte path above. The low-level copy reproduction is a safe trait operation once a `MusigSecNonce` exists: `let duplicate = original;` is accepted because of the public `Copy` implementation; subsequent FFI use requires the crate's intended unsafe boundary, but the high-level `Debug` leak does not.

Independent checks:

- `git grep -n -F 'impl_raw_debug!(MusigSecNonce)' -- secp256k1-sys/src/lib.rs`, `git grep -n -F 'pub struct SecretNonce' -- src/musig.rs`, and the C-contract search all returned the expected single definitions.
- The static contract check passed after confirming the `Copy` derive, raw-debug implementation, public high-level type, and C warning. It is a source-level proof of the formatter path rather than a pattern-only name match.
- `cargo test` was attempted and a compile-time Rust reproduction was prepared for the toolchain gate, but the environment has neither `cargo` nor `rustc` (`/bin/bash: cargo: command not found`). No runtime claim is made beyond the direct trait/macro expansion.
- The current Bitcoin Core C header carries the same secret-nonce warning at `src/secp256k1/include/secp256k1_musig.h:47-57`; current direct C MuSig behavior was already covered by the excluded Cycle 72 state-machine campaign. No in-tree Rust consumer exists to patch.

Verdict: confirmed external finding, not a Bitcoin Core source finding. The report should request a redacted `Debug` implementation for `SecretNonce`, removal of `Debug`/`Copy`/`Clone`/byte-reading traits from the low-level secret nonce wrapper, and a regression test asserting that ordinary formatting cannot reveal raw nonce bytes. Any change must preserve the unsafe FFI call representation without exposing the C opaque bytes through safe APIs. A local source commit is not justified because the affected files are in the external clone; this cycle uses one clearly labeled journal-only handoff snapshot.

Limitations: the external checkout is a shallow snapshot at `9fc86f756f0cd24a5167c131551b31b748c8cdb3`, so prior issue/PR history was not available locally; the report must be checked against upstream's current review state. Rust tooling is absent, so the safe-Rust snippet and formatter expansion were not compiled. No Bitcoin Core tracked source, tests, build files, or public ABI were changed.

Next queue: rerun the full gate, draw a distinct catalog goal, search its journal and prior finding index, and retain this external-wrapper finding as a linked non-duplicate rather than reopening the direct C MuSig state-machine cell.

## Cycle 198: out-of-range ancestor parity at the C ABI

### Selection and gate

- Exact selector after the Cycle 197 state close: `shuf -i 0-98 -n 1` -> `94` (`bindings-ffi-parity`); no reroll was needed.
- Branch: `uber-cycle-198-bindings-ffi-parity-20260731`.
- Cycle start: `2026-07-31T08:20:14Z`.
- Cycle start HEAD: `0b033a6f657812a17d25fb5bd9b9eb7b49ec978f`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence (`origin/master...HEAD`): `42 1186`.
- Tracked/index state was clean and `git diff --check` passed at entry. Known unrelated untracked agent artifacts, `node_modules/`, package metadata, and `test/cache/` were preserved and excluded from staging.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Corrected TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Uber-goal state SHA-256 at the gate: `1c3138db101b2be6427dedcb820ba16cf53770999fdfa51c74fbe9fb418638a4`.
- Protected long-running tests with PIDs `777094`, `956381`, `1138182`, and `1157959` were alive and were not touched. Disposable build/runtime files use `/data/my_storage/tmp`.

### New cell and exclusions

This cycle excludes Cycle 70's fixed opaque pointer-array conversion, Cycle 87's
fixed tracing-demo field omission, and Cycle 186's callback ownership and open
serialization-failure analysis. The repository does not vendor maintained Rust,
Python, Java, Go, or C# bindings; the maintained wrapper surface here is the
`libbitcoinkernel` C API and its C++ `btck` wrapper.

The selected distinct cell is parity for the public ancestor-height boundary:
`CBlockIndex::GetAncestor` deliberately returns null for negative or too-high
heights, but `btck_block_tree_entry_get_ancestor` currently asserts that result
is non-null. The C++ wrapper's existing `View` validation already translates a
null handle into `std::runtime_error`; the C declaration does not currently
document the nullable result.

### Working hypothesis and verification plan

An external C caller can pass an out-of-range `int32_t` height without violating
the non-null handle precondition. On the current Debug kernel build, the public
adapter aborts the embedding process at `assert(ancestor)` instead of returning
null. Add raw C assertions for negative and too-high heights, reproduce the
failure before changing production code, then make the smallest implementation
and documentation change and verify the C++ exception behavior remains stable.

Detached history refs `04cd1db690` and `6552e3bc05` contain similar prior fixes,
but neither is an ancestor of the cycle start. They are search leads only; the
current tree will be reproduced and fixed independently.

### Cycle 198 verification and result

The underlying `CBlockIndex::GetAncestor(int)` contract in `src/chain.cpp`
returns null when `height < 0` or `height > nHeight`. The public kernel adapter
then asserted that result at `src/kernel/bitcoinkernel.cpp:955`. The handle
argument remains a valid non-null precondition; the height is a caller-provided
`int32_t` and is a normal input value at the C boundary.

The regression assertions were added before the production change. On the
current Debug Clang 19 kernel build, this exact command rebuilt the test and
reproduced the failure:

```text
cmake --build /data/my_storage/tmp/cycle107-kernel-clang19 --target test_kernel -j2
env TMPDIR=/data/my_storage/tmp/cycle198-kernel-runtime /data/my_storage/tmp/cycle107-kernel-clang19/bin/test_kernel --run_test=btck_block_tree_entry_tests --random=19801 --log_level=test_suite --report_level=short --color_output=false
```

The pre-fix run exited `201` after `17` of `18` assertions, with
`Assertion 'ancestor' failed` and `signal: SIGABRT (application abort requested)`
at the negative-height call. This demonstrates an embedding-process abort from
an ordinary out-of-range scalar rather than a malformed handle.

The fix changes the adapter to return `nullptr` when the internal lookup has no
ancestor and documents that nullable result in the public C header. The C++
wrapper is unchanged: its existing `View` constructor rejects the null handle
with `std::runtime_error`, preserving the wrapper's established exception
translation. The test now checks raw C null results for `-1` and `3`, plus the
C++ exception for `3`; valid heights `2`, `1`, and `0` remain checked against
the expected entries.

Validation after the change:

- `git diff --check` passed.
- The same kernel build completed successfully.
- The focused command with seed `19802` passed 1 case and 20 assertions.
- The complete `/data/my_storage/tmp/cycle107-kernel-clang19/bin/test_kernel`
  run with seed `19803` passed all 19 cases and 3,717 assertions.
- `git grep` found only the public declaration, implementation, C++ wrapper,
  and the focused regression calls; no in-tree caller assumed a non-null result
  without the wrapper's existing check.

The source-level fix is independently reproduced and verified despite matching
the unmerged detached history leads. The remaining limitation is execution on
the available Linux x86_64 Debug/Clang 19 kernel build; other ABI consumers and
platform builds were not run, and the repository has no maintained foreign
language wrapper to exercise in-tree.

## Cycle 186: kernel wrapper failure and output-state parity

### Selection and gate

- Exact selector after the Cycle 185 state close: `shuf -i 0-98 -n 1` -> `94` (`bindings-ffi-parity`); no reroll was needed.
- Branch: `uber-cycle-186-bindings-ffi-parity-20260731`.
- Cycle start HEAD: `bc846462a38b84d782632d9341775953bb04520a`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- `git rev-list --left-right --count HEAD...origin/master`: `1162 42`.
- Tracked/index state was clean at entry; known unrelated untracked artifacts were preserved and excluded from all staging.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Corrected TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Uber-goal state SHA-256: `8acd97b1be0790bae551489904cb99bb380dc2d7c76f2101f88bee0176ba36be`.
- TSV validation: one header plus 99 four-field records, IDs 0 through 98 exactly once.
- Storage gate: `/` had about 55 MiB free and `/data` about 49 GiB free; disposable build/test files must use `/data/my_storage/tmp`.
- Unrelated long-running test processes with PIDs 777094 and 956381 were alive and were not touched.

### New cell and exclusions

This cycle excludes Cycle 70's fixed opaque pointer-array conversion, Cycle 87's
fixed tracing-demo field omission, and the prior source inventory of ordinary
view lifetime and empty-range precondition cells. The selected distinct cell is
failure and output-state parity at the `libbitcoinkernel` C API and its C++
`btck` wrapper: status-to-exception translation, callback exception capture,
output initialization on failure, and generated/example declarations. The
repository does not vendor maintained Rust, Python, Java, Go, or C# bindings,
so those are inventory-only unless a shipped wrapper is found.

### Working hypothesis

The open `write_bytes` candidate may call `std::rethrow_exception(nullptr)`
when a native C serialization function returns failure without a captured C++
exception. A related wrapper may also leave an output handle, byte buffer, or
callback-owned state partially initialized when the C API reports failure. First
establish the authoritative C header and implementation contracts, then seek a
valid production-triggerable failure or a proof that the callback always stores
an exception before any nonzero return. Do not change the bridge on a synthetic
invalid object or on a C API precondition violation.

### Cycle 186 verification

The `write_bytes` candidate remains unconfirmed. The valid transaction, block,
script, input, and witness serialization paths return nonzero only after the
internal `WriterStream` has converted a nonzero writer result into an exception;
the C++ adapter's writer catches its own allocation failure and stores the
`std::exception_ptr` before returning nonzero. No valid object path was found
that returns failure with a null exception, so the bridge was not changed.

A separate exception-safety defect was confirmed in the C++ callback adapters.
`ContextOptions::SetNotifications` and `SetValidationInterface` allocate a
heap `std::shared_ptr<T>` and pass it to the C API, whose corresponding setter
constructs a `std::shared_ptr<KernelNotifications>` or
`std::shared_ptr<KernelValidationInterface>` with `std::make_shared`. The old
wrapper released the raw payload before that potentially throwing assignment;
an allocation failure therefore leaked the callback state. The same ownership
transfer pattern was checked independently with `git grep`: these two setters
are the only context callback payload releases, while `Logger` has a distinct
constructor state machine and remains an open follow-up rather than being
folded into this fix.

The wrapper now passes `heap_notifications.get()` and `heap_vi.get()`, then
calls `release()` only after the respective C setter returns. The C API contract
still receives ownership on successful return, and an exception before the
assignment leaves the `unique_ptr` responsible for cleanup. This is a source-
level exception-safety proof; no deterministic allocation-failure injector is
available in the kernel test harness, so the proof does not claim a measured
OOM trace.

Validation from the current branch:

- `git diff --check` passed.
- `cmake --build /data/my_storage/tmp/cycle107-kernel-clang19 --target test_kernel -j2`
  rebuilt the current kernel library and test binary successfully with Clang
  19. The build used `/data/my_storage/tmp/cycle186-runtime` for temporary
  runtime data.
- `env TMPDIR=/data/my_storage/tmp/cycle186-runtime /data/my_storage/tmp/cycle107-kernel-clang19/bin/test_kernel --run_test=btck_context_tests,logging_tests --log_level=test_suite --report_level=short --color_output=false`
  passed 2 cases and 20 assertions.
- The same binary without a filter passed all 19 kernel cases and 3,714
  assertions, including notification, validation-interface, logger, and
  chain-manager lifetimes.

The selected source change is ready for one self-contained commit. Remaining
open cells are logger construction failure cleanup, callback exception
containment, null shared-pointer misuse, and any valid serialization failure
that can be demonstrated without violating a C API precondition.

This journal records cycle 70 of the uber investigation. The selected catalog goal is 94, `bindings-ffi-parity`.

## Cycle 70: opaque pointer-array parity

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `94`
- TSV row: `bindings-ffi-parity` / `Bindings, FFI, and language-wrapper parity audit`
- Branch: `uber-cycle-70-bindings-ffi-parity-20260728`
- HEAD at gate: `3fac7b6827c9a45d91d299ad717276b5539c0572`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `2 914`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Goal TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Tracked/staged state was clean except the known untracked agent artifacts and `test/cache/`. No daemon, unit-test, fuzz, sanitizer, or benchmark process was running.

### Inventory and scope

The repository does not vendor maintained Rust, Python, Java, Go, or C# bindings. The maintained wrapper surfaces are the C `libbitcoinkernel` API and its C++ `btck` wrapper, the Cap'n Proto IPC boundary, and the libsecp256k1 C API consumed by C examples and C++ callers. This cycle starts with the C/C++ kernel wrapper because it translates ownership, widths, spans, callbacks, and C pointer arrays directly.

The prior cycle-51 descriptor journal already lists wrapper/binding parity as an eligible future cell, but no kernel-wrapper cell has been tested. The setlabel, RPC, and wallet interface work from cycle 69 is excluded.

### Candidate ledger

| Surface | Hypothesis | Evidence | Verdict |
|---|---|---|---|
| `btck::PrecomputedTransactionData` constructor | A `span<const TransactionOutput>` is reinterpreted as `const btck_TransactionOutput**`, although the C API expects an array of opaque pointers. The conversion relies on the current C++ object layout and is invalid when the span contains multiple objects or that layout changes. | `btck_precomputed_transaction_data_create` indexes `spent_outputs_[i]` and dereferences each opaque pointer. Existing taproot coverage uses non-empty spans and passes only because the current handle base occupies the expected leading bytes; the wrapper still does not construct the documented pointer array. | Confirmed; fixed below |
| `write_bytes` error bridge | A nonzero C return with no captured callback exception calls `std::rethrow_exception(nullptr)`. | C serialization functions return nonzero for caught exceptions, while the callback stores allocation exceptions. Need a controlled C-side serialization failure to prove a null exception is reachable through a valid wrapper object. | Open; do not change without a reachable witness |
| `btck::View` and range iterators | Returned views may outlive their owning handle or underflow on `front()`/`back()` for empty ranges. | The wrapper documents view lifetime in tests; empty `front()`/`back()` are standard precondition violations. Treat as API documentation/usage contract unless a production caller violates it. | Dismissed for this cell |
| Enum and bitmask casts | C enum values or flag combinations may not match the C++ wrapper's static mappings. | The wrapper enums are directly initialized from the C constants and tests exercise normal flags. Continue with a compile/static matrix after the pointer-array candidate. | No finding yet |

### Verification protocol

Add one direct non-empty spent-output case to `btck_precomputed_txdata`. Run it against the current source, replace the cast with an explicit vector of `const btck_TransactionOutput*` pointers, rebuild the kernel test target, run the focused and full kernel suites, and restore the old cast as a negative control. Preserve the test's real transaction/output count invariant and avoid using a fake C implementation as the oracle.

If the candidate is fixed, inspect all other C++ wrapper constructors for similar object-array/pointer-array casts and record the C header contract, exact commands, mutation result, and remaining tool limitations before closing the cycle.

### Verification results

- The C header contract at `src/kernel/bitcoinkernel.h` documents `spent_outputs` as a nullable array of `const btck_TransactionOutput*`. The C implementation indexes the array and passes each entry to `btck_TransactionOutput::get`; it does not accept an array of C++ wrapper objects.
- The initial source was built in the isolated kernel configuration with:
  `cmake -S . -B build_kernel_cycle70_clang19 -DBUILD_KERNEL_LIB=ON -DBUILD_KERNEL_TEST=ON -DBUILD_TESTS=ON -DBUILD_GUI=OFF -DBUILD_BENCH=OFF -DWITH_BDB=OFF -DWITH_ZMQ=OFF -DENABLE_WALLET=OFF -DENABLE_IPC=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19`.
- Baseline focused run, seed 7041, passed on the unmodified layout because the current `TransactionOutput` object happens to expose its handle pointer at the same leading address and size. This was treated as evidence of layout dependence, not correctness.
- The fix constructs a temporary `std::vector<const btck_TransactionOutput*>` by calling each wrapper's typed `get()` method, then passes that vector to the C API. The direct non-empty case is retained in `btck_precomputed_txdata`.
- Corrected focused run, seed 7042, passed. Corrected full kernel run covered all 19 test cases, seed 7043, and passed with no errors.
- For an independent negative control, the old cast was restored and a temporary private `std::byte` member was added to `TransactionOutput`. The build passed, but `btck_script_verify_tests` with the existing two-output taproot vector exited 139 under seed 7044. The explicit-pointer version was restored, rebuilt, and the same affected suite passed under seed 7045.
- A scan of `src/kernel/bitcoinkernel_wrapper.h` and `src/kernel/bitcoinkernel.cpp` found no other wrapper-object-to-C-pointer-array conversion. `ChainMan::ImportBlocks` already creates explicit path and length arrays. Remaining casts are byte buffers, status/enum representations, or opaque-handle conversions.

### Cycle verdict and limitations

The candidate is confirmed as an ABI/layout-dependent FFI defect. The fix is limited to the C++ wrapper and its regression coverage; no C ABI change is needed. Evidence was collected on Linux x86_64 with Clang 19 in a RelWithDebInfo kernel build, with wallet and IPC disabled. The negative control is an intentional layout mutation and is not committed. No maintained language-wrapper repository is vendored in this tree, so Rust/Python/Java/Go/C# parity was inventory-only this cycle. The `write_bytes` null-exception path remains an open candidate requiring a reachable failure witness.

## Cycle 87 start

- Exact selector draw: `shuf -i 0-98 -n 1` -> `71` (`deterministic-simulation`), rejected because Cycle 84 closed that campaign and no new schedule evidence exists.
- Exact reroll: `shuf -i 0-98 -n 1` -> `94` (`bindings-ffi-parity`).
- Branch: `uber-cycle-87-bindings-ffi-parity-20260729`.
- Cycle-start HEAD: `a38b4da22fcb0df5411bfd2e023be4cee9689278` (`journal: close performance regression cycle 86`).
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` is `2 961`.
- Dirty state: no tracked edits; preserved unrelated untracked agent/user artifacts and `test/cache/` remain outside this cycle.
- Catalog/protocol/manifest hashes: catalog `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.

## Cycle 87 contract and queue

This cycle extends the prior kernel-wrapper work with distinct wrapper and FFI
surfaces. Compare C/C++ public APIs with maintained Rust, Python, Java, Go, C#,
JNI, JavaScript, or other wrappers that are actually present or explicitly
shipped by this repository. Audit widths, signedness, ownership, lifetimes,
nullability, callbacks, exceptions/status mapping, thread safety, buffer
lengths, secret cleanup, feature flags, generated declarations, and
output-on-failure.

For each candidate, identify the authoritative C/C++ contract and wrapper
boundary, then use shared deterministic vectors and misuse cases. A wrapper
bug, generated-file drift, core API defect, and documentation mismatch are
separate verdicts. Do not change core behavior to accommodate a broken wrapper
unless the core contract is independently unsafe. External projects and
reported issues are seeds; retain a report-ready reproducer for remote-only
defects and do not claim their fix is local.

Every finding needs a hard oracle: a failing-before/passing-after regression,
shared vector mismatch, minimized malformed input, sanitizer or static trace,
ABI/layout evidence, or a rigorous contract proof. For secret, persistence,
consensus, or remotely reachable paths use two independent verifier forms when
practical. Keep one self-contained commit per confirmed finding, authored as
`Lőrinc <pap.lorinc@gmail.com>`, and commit no speculative compatibility or
style cleanup. If no source fix is justified, close with one exact journal/state
handoff commit.

### Initial hypotheses

1. A maintained wrapper or generated declaration may use a narrower or unsigned
   type than the C API for lengths, flags, counts, sizes, or error values.
2. A wrapper may translate C failure into an exception/status but expose an
   output buffer, object, callback, or secret-derived temporary in a state that
   differs from the native contract.
3. The repository may ship a binding/example/build target whose feature flags,
   symbol list, ABI assumptions, or thread/lifetime rules have drifted from the
   current C/C++ API.

### Initial queue

- Inventory public headers, exported symbols, generated bindings, FFI examples,
  package manifests, build targets, and maintained language-specific code.
- Search all wrapper declarations and call sites against native signatures and
  tests; record unsupported language bindings separately from shipped ones.
- Build shared vectors for sizes, null/empty buffers, malformed public objects,
  callback failure, output-on-failure, and secret cleanup where applicable.
- Check ABI width/alignment and ownership across 32/64-bit-relevant types, then
  run the smallest focused wrapper tests before broader configurations.
- Search history, issues, PRs, and prior journals for binding fixes and reviewer
  precedent before selecting a candidate. The next unchecked cell must be a
  distinct boundary, not a repeat of the prior opaque pointer-array finding.

## Cycle 87 finding: tracing demo omitted replacement-kind field

### Candidate and contract

The maintained `contrib/tracing/mempool_monitor.py` BCC consumer modeled
`mempool:replaced` with seven values and submitted a seven-field event. The
native tracepoint in `src/validation.cpp` passes eight values: the eighth is
`replaced_with_tx`, which distinguishes a replacement transaction ID from a
package hash. `doc/tracing.md` documents the same eighth `bool`, and
`test/functional/interface_usdt_mempool.py` already reads and asserts it.

This is a concrete observability/FFI parity defect. Package RBF is a reachable
production path, and the demo's replacement hash is otherwise ambiguous. The
history search found the field-introducing commit `5736d1ddacc4` ("tracing:
pass if replaced by tx/pkg to trace"), which explicitly added the discriminator
because the tracepoint can now expose either a transaction ID or package hash.

### Verification and fix

- Source comparison: `src/validation.cpp` passes argument 8 as
  `replaced_with_tx`; `doc/tracing.md` specifies the argument; the functional
  BPF struct and `ctypes.Structure` both contain
  `replaced_by_transaction` and read argument 8. The demo lacked both pieces
  and rendered the replacement hash without a kind.
- Applied fix: add `bool replaced_by_transaction` to the demo event, read
  `bpf_usdt_readarg(8, ...)`, and render `transaction` or `package` before the
  replacement hash. This is one self-contained source change; no native ABI
  change is needed.
- `python3 -m py_compile contrib/tracing/mempool_monitor.py`: passed.
- `git diff --check`: passed.
- Structural review of the patched event declaration/read sequence against the
  functional harness: passed.
- Live BCC validation was unavailable because `python3 -c 'import bcc'` failed
  with `ModuleNotFoundError: No module named 'bcc'`. The repository functional
  runner also could not start because `test/functional/../config.ini` is absent
  in this checkout. These are environment limitations, not test failures in
  the patch.

### Verdict

Confirmed. The demo silently discarded a documented, reachable tracepoint
field and could not tell users whether its replacement hash was a transaction
ID or package hash. The narrow fix preserves the existing event layout and
adds the missing semantic label. Remaining queue: inspect the other shipped
USDT/BCC consumers for the same class of dropped or stale fields, then close
the cycle only after the patched file is committed with this journal.
