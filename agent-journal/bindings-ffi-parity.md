# Bindings, FFI, and language-wrapper parity

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
