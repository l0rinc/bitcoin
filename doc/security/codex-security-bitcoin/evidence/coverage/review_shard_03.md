# Discovery review shard 03

- Assigned manifest slice: lines 1645–2466 inclusive
- Assigned paths: 822
- Reviewed paths: 822
- Unique coverage paths: 822
- Raw security candidates: 0
- Unreadable or blocked paths: 0

## Scope

Every assigned path was under `node_modules/`. The slice covers packaged source,
generated JavaScript and source maps, declarations, schemas and lookup tables,
tests/examples, documentation, metadata, build configuration, and nested
third-party CI configuration. The largest package groups were `ajv` (370 paths)
and `incur` (200 paths).

The repository threat model classifies this untracked dependency tree as tooling
used by the scan environment rather than a Bitcoin product or release path unless
an execution or packaging path is demonstrated. Repository and lockfile review
found no Bitcoin source, root build, release, or CI reference to the packages in
this shard. Nested package workflows and package-development scripts are not
active Bitcoin workflows or install hooks.

## Review performed

All 822 files were read. Runtime sources and their generated forms were reviewed
for command/code execution, filesystem and archive handling, path traversal,
network requests and server behavior, unsafe parsing or deserialization,
prototype/object mutation, schema/code generation, worker construction, resource
exhaustion, information exposure, and trust-boundary mistakes. JSON and source
maps parsed successfully, JavaScript syntax checks passed, and generated or
declarative files were reviewed together with their consuming runtime code.

Notable security-sensitive implementations included AJV schema compilation and
generated validation/parsing code, `extract-zip` destination checks and symlink
handling, `fflate` decompression and worker construction, `fast-uri` authority
and delimiter handling, and `incur` fetch/OpenAPI/MCP/filesystem/process helper
surfaces. These were either safely constrained, explicit local/developer APIs,
or generic dependency behavior with no demonstrated route from a Bitcoin
attacker-controlled input to an affected Bitcoin product or release sink.

## Result

No reportable raw candidate was identified. This conclusion is based on both the
implementation review and the absence of a target-reachable execution,
packaging, or deployment path required by the supplied threat model. The
coverage JSONL contains exactly one `reviewed` row for every assigned manifest
path in manifest order.
