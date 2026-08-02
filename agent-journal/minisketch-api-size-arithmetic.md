# Minisketch API Size and Count Arithmetic Audit

## Seed from Cycle 305

Cycle 305 Goal 69 found and fixed a public C API boundary where
`minisketch_decode(size_t max_elements, ...)` narrowed the bound to `int` and
overflowed its degree check. A one-element sketch incorrectly returned `-1`
for `INT_MAX`, `INT_MAX + 1`, and `SIZE_MAX`; generic and CLMUL probes plus
UBSan independently confirmed the defect. The fix keeps the bound as `size_t`
and tests the extreme values across implementations.

The same inspection exposed a distinct unchecked-size queue. Do not reopen the
repaired decode-bound cell, the Cycle 301 Minisketch field matrix, or the
earlier backend comparisons without new evidence.

## Initial scope

Audit the public C and C++ API for arithmetic that turns field sizes,
capacities, element counts, serialized byte lengths, or decoded root counts
into another integer domain. Prioritize:

- `minisketch_serialized_size()` and `bits * syndromes + 7` at
  allocation-feasible and overflow boundaries;
- `minisketch_compute_capacity()` and
  `minisketch_compute_max_elements()` for zero/one/max bits, `size_t` maximum
  counts, `uint32_t` false-positive bits, and monotonicity;
- `Sketch::Decode` and the `ssize_t` API return count when capacities approach
  signed return limits;
- C++ wrapper vector allocation, output pointer/length contracts, and failure
  behavior for huge but representable requests;
- serialization/deserialization buffer sizes across generic, CLMUL, VERIFY,
  sanitized, and alternate compiler configurations.

For every candidate, write the mathematical domain and expected boundary table
before testing. Use a small faithful arithmetic model, direct caller-owned
buffers, allocation-failure hooks or bounded mocks where available, and
sanitized API probes. Distinguish impossible-to-allocate theoretical values
from values reachable with ordinary memory. Require a failing-before test,
first-invalid-operation trace, or rigorous proof before changing the library.

## Learned queue

Search prior Minisketch history, API documentation, bindings, and callers
before each hypothesis. Preserve serialized compatibility and the existing
generic/optimized implementation contract. A size-safe rejection or saturating
behavior must leave the sketch and caller output in a documented state.
