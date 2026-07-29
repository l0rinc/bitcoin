# Discovery review shard 08

- Inventory range: lines 5755-6571 inclusive
- Expected/reviewed files: 817 / 817
- First path: `src/test/net_tests.cpp`
- Last path: `vcpkg.json`
- Readability: every listed file exists and was readable; no blockers
- Receipt: `artifacts/03_coverage/review_shard_08.jsonl`
- Raw candidates: `.work/raw_candidates_08.jsonl`

## Review method

Every file was read or inspected in full, with source-oriented passes over wallet import and
unlock paths, Berkeley DB parsing, UniValue parsing, Tor control, transaction/validation
state, shared utilities, ZMQ notification code, test runners/frameworks, build metadata, and
non-executable fixtures. Python files were syntax-parsed, JSON fixtures were distinguished
from intentionally invalid parser vectors, and filesystem/process/network/memory/resource
patterns were reviewed. Test-only behavior was retained as evidence but not promoted without
a production path.

## Candidate findings

1. `overflow-page-cycle`: a zero-length self-referential BDB overflow page can keep
   `BerkeleyRODatabase::Open` in a non-progressing loop.
2. `repeated-internal-page-references`: duplicated internal B-tree references can multiply
   parsing work at every level without a visited-page or work budget.
3. `wallet-master-key-iteration-count`: a persisted unbounded KDF iteration count can cause
   billions of SHA-512 operations during wallet unlock and is converted from unsigned to
   signed `int`.

These are discovery candidates for downstream validation and severity calibration. The
wallet load/import and authorized unlock preconditions are captured in their contexts.

## Classification counts

- `build_metadata`: 4
- `bundled_library`: 7
- `developer_lint_tool`: 25
- `functional_test`: 284
- `functional_test_framework`: 36
- `fuzz_harness`: 9
- `package_metadata`: 1
- `parser_fixture`: 55
- `production`: 170
- `test_build_metadata`: 2
- `test_cache_artifact`: 3
- `test_configuration`: 4
- `test_documentation`: 4
- `test_fixture`: 77
- `test_tool`: 3
- `unit_test`: 133

## Coverage gaps and blockers

None. All 817 assigned inventory rows were accounted for exactly once.
