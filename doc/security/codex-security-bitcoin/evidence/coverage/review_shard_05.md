# Review shard 05

- Inventory range: lines 3289-4110 inclusive
- Expected paths: 822
- Reviewed paths: 822
- Unique paths: 822
- First path: `node_modules/yaml/dist/schema/common/seq.d.ts`
- Last path: `src/base58.cpp`
- Raw candidates: 0
- Unreadable files: 0
- Binary files accounted: 8

## Scope composition

- Untracked `node_modules`: 784 files (`yaml`, `yauzl`, and `zod`)
- Untracked npm scan-environment manifests: 2 files
- Bitcoin Core `share/` content: 19 files
- Bitcoin Core `src/` content: 16 files
- Developer lint configuration: 1 file

The JavaScript dependency tree and root npm manifests are untracked files installed for the
Codex Security scan environment. They are not referenced by Bitcoin Core's tracked CMake,
Depends, packaging, or runtime surfaces. Their executable parser/library implementations were
still read and reviewed, but dependency presence alone does not establish target reachability.

## Review performed

Every assigned path was opened and read completely (5,429,421 bytes; 135,935 logical lines).
Generated declarations and duplicated ESM/CJS outputs were reconciled with their package source,
tests and benchmarks were distinguished from runtime code, and every binary asset was read and
accounted by media type and size. Focused checks covered archive traversal/decompression,
YAML merge/prototype behavior, Zod object/prototype handling, regex construction and JIT code
generation, npm integrity/install metadata, build-time command invocation, installer path/URI
handling, RPC credential generation, addrman P2P state and persistence, fixed-width consensus
arithmetic, ban-state concurrency, and Base58 bounds/checksums.

The Bitcoin Core production files in this range use fixed addrman bucket geometry, serialized
entry caps, network-magic/checksum validation, lock-guarded shared state, saturating ban-time
arithmetic, and bounded Base58 output contracts. No lower-trust source-to-sink path survived
discovery as a plausible security candidate.

## Binary assets

- `share/pixmaps/bitcoin.ico`: read successfully; static packaging/GUI media, not source-reviewable.
- `share/pixmaps/bitcoin128.png`: read successfully; static packaging/GUI media, not source-reviewable.
- `share/pixmaps/bitcoin16.png`: read successfully; static packaging/GUI media, not source-reviewable.
- `share/pixmaps/bitcoin256.png`: read successfully; static packaging/GUI media, not source-reviewable.
- `share/pixmaps/bitcoin32.png`: read successfully; static packaging/GUI media, not source-reviewable.
- `share/pixmaps/bitcoin64.png`: read successfully; static packaging/GUI media, not source-reviewable.
- `share/pixmaps/nsis-header.bmp`: read successfully; static packaging/GUI media, not source-reviewable.
- `share/pixmaps/nsis-wizard.bmp`: read successfully; static packaging/GUI media, not source-reviewable.

## Classification counts

- Bitcoin Core consensus header: 1
- Bitcoin Core consensus source: 1
- Bitcoin Core portability header: 1
- Bitcoin Core production header: 5
- Bitcoin Core production source: 5
- binary packaging asset: 8
- build configuration: 1
- build tooling: 1
- developer tooling configuration: 3
- documentation: 1
- installer template: 1
- packaging example: 1
- packaging template: 1
- shipped administrative helper: 1
- textual generated packaging asset: 5
- third-party dependency metadata: 11
- third-party documentation or license: 2
- third-party documentation or metadata: 3
- third-party generated declaration: 241
- third-party generated runtime: 240
- third-party source: 53
- third-party source localization: 55
- third-party test or benchmark: 179
- untracked scan-environment dependency metadata: 2

## Coverage gaps

None. There were no missing or unreadable files. Binary media was accounted rather than decoded
as executable source. The candidate file is intentionally empty because this shard produced no
plausible target-reachable findings.
