# Discovery Review Shard 04

- Inventory range: lines 2467–3288 inclusive
- Expected files: 822
- Receipt rows: 822
- Raw candidates: 0
- Unreadable or missing files: 0

## Scope

This shard contains 13 third-party JavaScript packages under the scan-environment `node_modules` tree:

| Package | Files |
|---|---:|
| papaparse | 16 |
| pdfjs-dist | 489 |
| pend | 5 |
| pump | 10 |
| require-from-string | 4 |
| safer-buffer | 7 |
| signal-exit | 29 |
| smol-toml | 22 |
| tokenx | 5 |
| undici-types | 48 |
| universal-user-agent | 12 |
| wrappy | 4 |
| yaml | 171 |

Every listed file was read and hashed. Executable JavaScript was syntax-checked and reviewed for unsafe parsing, dynamic code execution, command/process execution, filesystem and network access, DOM injection, prototype pollution, resource exhaustion, and error/crash paths. Test and demo behavior was reviewed rather than excluded. JSON and source maps were structurally parsed; source maps were correlated with readable builds. SVGs were checked for scripts, event handlers, external references, and foreign content. CMaps, fonts, GIFs, ICC data, and WebAssembly modules were accounted for using hashes, format metadata, headers/strings, and their adjacent loader code.

## Reachability and decisions

- No package in this shard is imported by Bitcoin Core product, build, release, or root CI code. The repository-root JavaScript manifest and lockfile are scan-environment additions whose sole top-level dependency is `@openai/codex-security`.
- `pdfjs-dist` is reached only by the scan SDK's optional knowledge-base PDF extraction. That consumer supplies local byte data and explicitly sets `isEvalSupported: false` and `stopAtErrors: true`. PDF scripting/viewer bundles, browser XHR loaders, and generated PostScript compilation are not reached by Bitcoin Core.
- `papaparse` is reached only by scan SDK bulk/multiscan helpers. Worker/download support and formula escaping are caller-selected library behavior. The bundled player demo loads jQuery over plain HTTP, but the demo is not served, invoked, or packaged by Bitcoin Core.
- `smol-toml` is used only for scan-environment configuration. Its parser handles `__proto__` with own data properties and applies a default nesting limit.
- `yaml` is used only by the scan-environment CLI dependency. Object conversion protects prototype keys, and alias expansion is capped by default. Its `--visit` dynamic import is an explicit local CLI capability.
- `require-from-string` intentionally compiles caller-provided code; its only repository consumer is AJV's standalone generator, and no lower-trust Bitcoin Core input reaches it.
- Nested dependency workflows and tests are not repository-root automation and do not receive Bitcoin Core CI privileges.

Dangerous primitives were therefore not promoted without an attacker-controlled source and a supported Bitcoin Core sink. No plausible reportable candidate survived source/control/sink tracing in this shard.

## Coverage gaps

No files were unreadable or unresolved. Binary assets were reviewed by metadata and loader-path analysis rather than source-level semantic disassembly; this is recorded in each corresponding receipt.
