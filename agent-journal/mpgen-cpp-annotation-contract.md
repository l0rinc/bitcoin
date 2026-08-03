# Generated C++ annotation identifier and type contract audit

## Seed from Cycle 316

Cycle 316 found and fixed raw C++ source injection through `mpgen`'s
`include` and `includeTypes` header-path annotations. The fix is committed as
`f8973a2540`; regression tests reject quotes, backslashes, and control bytes
before any `mpgen` proxy output is opened while preserving ordinary relative
header paths.

This goal deliberately excludes that repaired header-path defect. Continue
through the other Text annotations in
`src/ipc/libmultiprocess/src/mp/gen.cpp`: namespace, wrap, exception, name,
and any related values emitted as C++ namespaces, types, identifiers,
declarations, or template arguments.

## Initial queue

- Map every annotation sink to its generated file, exact C++ grammar position,
  and downstream compiler or include consumer.
- Derive valid domains from Cap'n Proto schemas, C++ grammar, existing users,
  history, and compatibility documentation before rejecting values.
- Use isolated schemas with quotes, backslashes, comments, newlines, control
  bytes, punctuation, Unicode, keywords, duplicate scopes, and empty values;
  preserve generated artifacts and minimized compiler diagnostics.
- Compare parser acceptance with generated parse trees and consumer behavior.
  Distinguish intended C++ naming flexibility from malformed or injected
  source, and validate before opening outputs when fail-closed behavior is
  required.
- Recheck all current and historical annotation users and add independent
  generation and consumer tests for any confirmed mismatch.

Do not treat the source-controlled schema as a network trust boundary without
evidence. Record the build-integrity, reproducibility, compatibility, and
review implications separately, and do not repeat the Cycle 316 include-path
fix unless a distinct caller or failure mode is proven.
