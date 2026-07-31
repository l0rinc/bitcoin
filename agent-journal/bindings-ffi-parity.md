# Bindings, FFI, and language-wrapper parity

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
