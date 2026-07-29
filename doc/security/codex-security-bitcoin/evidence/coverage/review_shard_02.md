# Discovery shard 02 review

- Inventory range: lines 823-1644 inclusive
- Assigned rows: 822 (all unique; exact range match verified)
- First path: `node_modules/@modelcontextprotocol/server/dist/ajvProvider-ZaoO9afR.cjs`
- Last path: `node_modules/ajv/dist/refs/json-schema-2020-12/meta/format-annotation.json`
- Unreadable or missing files: 0
- Raw candidates: 0

## Outcome

Every assigned file was read start-to-finish. Textual runtime files were reviewed for process, filesystem, network, parser/deserializer, dynamic-code, archive, and authentication-sensitive behavior. JSON and source maps were fully parsed; Python sources were AST-parsed. Opaque native executables/addons and compressed/generated assets were read in full and metadata-accounted. The two compressed MCP runtime chunks were combined and decompressed for a full review, and the compressed MCP app HTML was decompressed and reviewed.

This shard is entirely under `node_modules/`. Repository evidence shows that the root `package.json`, `package-lock.json`, and `node_modules/` are untracked scan-environment additions. Tracked Bitcoin sources, build files, CI, and release paths do not import or package these dependencies. Potentially dangerous primitives are explicit library/scan-tool capabilities, and no supported Bitcoin execution path crosses an attacker boundary into them. Accordingly, no raw candidate was emitted.

## Status counts

- `metadata_accounted`: 117
- `reviewed`: 705

## Classification counts

- `dependency_documentation`: 70
- `dependency_executable_binary`: 5
- `dependency_native_binary`: 1
- `dependency_package_metadata`: 20
- `dependency_runtime`: 125
- `dependency_type_declaration`: 423
- `generated_binary_asset`: 1
- `generated_compressed_bundle`: 3
- `generated_schema_or_fixture`: 21
- `generated_source_map`: 107
- `scan_tool_configuration`: 14
- `scan_tool_runtime`: 32

## Risk-area occurrences

- `api_surface_contract`: 423 files
- `archive_extraction`: 3 files
- `authentication_secrets`: 16 files
- `dom_rendering`: 1 files
- `dynamic_code_generation`: 9 files
- `filesystem_io`: 23 files
- `generated_content`: 110 files
- `native_code`: 6 files
- `network_io`: 5 files
- `parsing_deserialization`: 40 files
- `process_execution`: 14 files
- `supply_chain_metadata`: 34 files

## Coverage gaps and blockers

No assigned file is missing or unreadable. Opaque stripped ELF files were metadata-accounted rather than source-decompiled; they have no tracked Bitcoin runtime, build, or release reachability. No unresolved candidate or review blocker remains in this shard.
