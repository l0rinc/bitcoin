# Generated-source escaping and provenance boundary audit

## Seed from Cycle 306

Cycle 306 confirmed that the Wycheproof and Silent Payments generators copied
external JSON comments into C block comments without escaping `*/`. A hostile
fixture broke out of the comment in all three generators and changed the
generated header's parse tree. The fix added `sanitize_c_comment()` to the
shared helper and preserved all trusted generated headers byte-for-byte.

This campaign expands the learned surface beyond vector comments. Inventory
every tool that converts external, downloaded, mutable, or user-supplied data
into source code, build metadata, shell commands, manpages, paths, or test
artifacts. Track the exact delimiter, quoting, encoding, provenance, and
consumer boundary. Search history and prior findings before testing, and keep
fixed malicious fixtures that try comment closure, preprocessor insertion,
literal/identifier injection, newline and encoding confusion, path traversal,
argument splitting, and oversized generated output.

First queue:

- Recheck all current C/C++ vector generators and any new generator added after
  the Cycle 306 fix, including provenance comments and string/identifier sinks.
- Audit generated manpages and help2man inputs under locale, path, version, and
  tool-version changes without treating textual differences as code defects.
- Inspect seed, precomputation, Sage, IPC, and build/code-generation tools for
  the same source-boundary shapes, with a clean compile or consumer parser as
  the independent verifier.
- Preserve trusted-corpus byte hashes and distinguish reproducibility drift,
  source injection, malformed output, and intentional data-dependent output.

Do not claim that escaping alone proves supply-chain integrity. Record corpus
provenance, pinning, review evidence, output limits, and the exact downstream
consumer for each candidate.

## Cycle 316: `mpgen` include annotation source injection

Status: confirmed and fixed on `uber-cycle-316-generated-source-boundaries-20260802`.

### Hypothesis and boundary

`$Proxy.include()` and `$Proxy.includeTypes()` are declared as `Text` values
described as extra header paths, but `src/ipc/libmultiprocess/src/mp/gen.cpp`
copied them directly between the quotes of generated `#include` directives.
The schema is a build-time source boundary. It is not a runtime network input,
but an accidental quote, backslash, or control byte can change the generated
C++ translation unit and make a reviewed schema generate unrelated code.

The Cycle 306 vector-comment finding was searched first and excluded as an
exact repeat. History shows the include annotations were introduced as the
public replacement for hardcoded generated includes; current in-tree values
are ordinary relative header paths.

### Independent reproduction

Scratch fixture `/data/my_storage/tmp/cycle316-mpgen/evil.capnp` has SHA-256
`a428179b274a17c59ca7102cf8d9bcba6f994d16a280e575e5636ca72799dbd5` and
contains an escaped quote, a newline, and `int generated_marker = 1;` in the
`include` annotation. Before the fix, the exact `mpgen` invocation returned
status 0. Its generated `evil.capnp.proxy.h` (SHA-256
`698f794fd11ea5c97d178bfdde37cf5e3e23953106987b07f6314121b6f3bd7a`)
contained:

    #include "safe.h"
    int generated_marker = 1; //" // IWYU pragma: export

The generated header passed `g++ -std=c++20 -fsyntax-only`, so the injected
declaration was accepted by the downstream consumer rather than merely
causing a syntax error. A second fixture isolated the same behavior in
`includeTypes`.

### Fix and regression coverage

`ValidateIncludePath()` now rejects empty paths, quotes, backslashes, and all
ASCII control bytes. `Generate()` validates both annotation IDs immediately
after Cap'n Proto parsing and before opening any `mpgen` output stream. The
existing annotation interface and ordinary paths remain unchanged.

Permanent fixtures and `mpgen_test.cmake` exercise both annotation types. Each
test requires a nonzero generator result, the expected diagnostic, and no
proxy output files. A clean schema using `safe.h` and `safe-types.h` generated
successfully and its proxy header passed `g++ -std=c++20 -fsyntax-only`.

Validation:

- `ninja -C /data/my_storage/tmp/cycle246-mp-inject mptest` passed after CMake
  regeneration and rebuilt `mpgen` plus normal generated proxy sources.
- `ctest --test-dir /data/my_storage/tmp/cycle246-mp-inject -R '^mpgen_rejects_' --output-on-failure` passed both tests.
- `ctest --test-dir /data/my_storage/tmp/cycle246-mp-inject -R '^mptest$' --output-on-failure` passed.
- `git diff --check` passed.

The fix is intentionally limited to header-path annotations. Namespace,
wrapper, exception, and renamed identifier annotations are separate generated
C++ identifier boundaries and remain on the next queue for an independent
contract audit rather than being conflated with this finding.
