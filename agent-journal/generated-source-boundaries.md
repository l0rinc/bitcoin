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
