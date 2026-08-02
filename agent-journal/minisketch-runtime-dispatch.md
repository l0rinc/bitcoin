# Minisketch Runtime Implementation Dispatch and Availability Audit

## Seed from Cycle 301

Cycle 301 Goal 69 compared the generic, CLMUL, and trinomial-CLMUL Minisketch
implementations for every field size 2..64 on an x86_64 host. The independent
public API probe and Release plus ASan/UBSan VERIFY/non-VERIFY suites passed.
That result does not execute the runtime-unavailable branch because the host
advertises PCLMULQDQ.

The next falsifiable cell is `src/minisketch/src/minisketch.cpp`:

- `SystemIntrospection.cmake` decides whether CLMUL sources are compiled;
- `EnableClmul()` reads CPUID at runtime;
- `minisketch_implementation_supported()` and `minisketch_create()` must agree
  when implementation 1 or 2 is unavailable;
- implementation 0 must remain usable for every compiled field size;
- disabled-field builds must not advertise or construct omitted sizes.

Use a real CPU-feature boundary if available, or a narrowly scoped CPUID test
shim/emulation that still proves no CLMUL instruction executes. Check process
startup, shared/static libraries, C and C++ wrappers, invalid implementation
numbers, OOM/error status, and the exact fallback behavior. Compare CMake and
Autotools configuration outputs, supported-field matrices, and compiler/OS
assumptions. Preserve a minimized replay and distinguish a project defect from
an environment limitation. Do not rerun Cycle 301's passing x86_64 backend
matrix without exercising a new runtime-availability or build-configuration
boundary.
