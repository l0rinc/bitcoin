# libbitcoinkernel C ABI nullability, callback, and width matrix

This campaign was added after Cycle 309's bindings/FFI audit. The current
`btck_chainstate_manager_import_blocks` implementation accepted malformed
parallel arrays at the raw C boundary while the C++ wrapper always constructed
valid arrays. The fixed regression is `d3dc50785a`; this journal is reserved
for distinct C ABI/wrapper boundary cells and must not reopen that import-path
finding.

## Seed evidence

- C header: `src/kernel/bitcoinkernel.h` documents pointer nullability,
  borrowed handle lifetimes, callback ownership, and pointer/length pairs.
- C implementation: `src/kernel/bitcoinkernel.cpp` translates those contracts
  into opaque handles, callbacks, status codes, and filesystem/database calls.
- C++ wrapper: `src/kernel/bitcoinkernel_wrapper.h` adds owning handles, views,
  ranges, spans, exceptions, and virtual callback classes.
- Cycle 309 found that a raw C caller can express invalid array state that the
  wrapper cannot create; future cells should compare the two surfaces instead
  of assuming wrapper tests cover all C consumers.

## Initial queue

1. Build a declaration/implementation/wrapper table for every pointer plus
   length, nullable output, borrowed view, callback, enum, and fixed-width
   integer parameter. Record whether invalid input returns an error, returns
   null, throws in C++, asserts, or is an undocumented precondition.
2. Trace callback replacement and destruction under locks. Test whether a
   user destroy callback can re-enter options or context APIs, and whether
   callback exceptions or allocation failures can cross the C ABI.
3. Compare raw-C nullable results with C++ `View`/`Handle` constructors and
   `std::optional` methods. Exercise empty, missing, stale, and failed-output
   states with independent raw-C and wrapper oracles.
4. Audit `size_t`, `uint32_t`, `int32_t`, `unsigned int`, and `int` conversions
   at counts, heights, indexes, lengths, flags, progress, and worker options.
   Use compile-time layout checks, sanitized boundary probes, and a 32-bit
   build or explicit proof where execution is unavailable.
5. Check length-delimited paths and byte buffers for embedded NUL handling,
   empty-data pointer rules, output initialization, and lifetime after the
   callback returns. Preserve minimized consumers and ABI notes.

## Protocol

Search this journal, `agent-journal/bindings-ffi-parity.md`, all current
headers/wrappers, detached history, and review discussion before selecting a
cell. A source fix requires a raw-C or wrapper-level failing-before test and a
passing-after test, plus an independent contract or width check. Do not treat
the C++ wrapper's stronger preconditions as proof that the public C API is
safe. Keep C ABI changes minimal and document whether an existing invalid
caller receives a return code, null, exception, or preserved output.
