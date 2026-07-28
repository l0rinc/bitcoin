# Public API Output-on-Failure Audit

## Cycle 56

- Selected by the uber loop: `shuf -i 0-98 -n 1` -> `46`
- Goal: `public-interface-output-failure`
- Started from HEAD: `dd4cac6cddbf4e6131b7be23c3d65c4bd7553705`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence at start: `origin/master...HEAD` = `2 879`
- Dirty-state gate: tracked and staged state clean; only the known agent-owned untracked artifacts remain
- Process gate: no relevant test, build, daemon, fuzz, sanitizer, or profiling process running
- Catalog/protocol/TSV hashes: unchanged from the uber ledger

## Scope and Prior Evidence

This cycle exercises malformed and failing public calls and checks whether caller-owned outputs are unchanged, zeroed, invalidated, or intentionally partially committed. The audit begins with persistence-facing APIs because corrupt records are an externalized failure path and the current DB wrapper deserializes directly into its output argument.

The following historical output-contract cells are closed and will not be reopened without new evidence: block-write output clearing, wallet transaction-detail clearing, txindex failed-lookup clearing, mining solution failure clearing, fixed-point parsing, argument parser rollback, SOCKS5 late-failure replies, versionbits null-stat clearing, UniValue trailing-data failure clearing, compact-key recovery invalidation, ECDSA signing failure clearing, script-signing output clearing, wallet key mismatch cleanup, fee-estimator no-estimate clearing, failed `GetOp` output handling, address decode error-location reset, script verification error output, txdownload empty-request handling, BIP324/AES/AEAD decrypt failure outputs, PSBT result contracts, compact-block fill failure, blockfilter failed-range preservation, script compression output clearing, hex/base58 failure outputs, HTTP parse atomicity, mempool dump preservation, fee-estimator read atomicity, and PSBT analysis result contracts.

## Initial Hypotheses

1. `CDBWrapper::Read` may mutate a caller's value before a malformed serialized value throws; the failure result then leaves a misleading partial value.
2. `CDBIterator::GetKey` and `GetValue` may have the same problem for corrupted iterator records, while their current tests only prove that scratch-stream state is reusable.
3. Interface methods that return `bool` may intentionally preserve outputs on failure; any candidate must be checked against callers, documentation, history, and a deterministic failing-before regression before a fix is justified.

## Evidence Ledger

- `src/dbwrapper.h:164-190`: `CDBIterator::GetKey` and `GetValue` deserialize directly into output references and catch only `std::ios_base::failure`.
- `src/dbwrapper.h:232-254`: `CDBWrapper::Read` deserializes directly into its output reference after reading and deobfuscating the stored bytes.
- `src/test/dbwrapper_tests.cpp:255-311`: existing iterator tests check failed decode and exhausted-iterator preservation, but do not check a partially decoded object from a malformed stored record.
- `src/test/dbwrapper_tests.cpp:518-551`: existing DB deserialize tests classify exception types, but use a serializer that throws before mutating a structured output.
- Existing callers and prior fixes must be searched before reporting; no source change is assumed yet.

## Required Verification

- Create a malformed stored payload whose first field is valid and a later field is truncated.
- Assert `Read`, `GetKey`, and `GetValue` return failure and preserve sentinels before the production edit.
- If the regression fails, patch the smallest API boundary that makes failed deserialization atomic, then rerun the same test after the patch.
- Independently verify both direct lookup and iterator paths, both obfuscation modes where applicable, and a normal successful read.
- Run the focused DB wrapper unit suite, dependent persistence tests, a sanitizer smoke, and the relevant fuzzer or record replay if available.
- Search history/issues/PRs for an existing contract or duplicate finding before committing.

## Verification Results

### Failing-before proof

The regression test was added before changing production code. With the original direct-deserialization implementation, this command exited `201`:

`TMPDIR=/data/my_storage/tmp/cycle56-dbwrapper-before build_unit_clang19/bin/test_bitcoin --run_test=dbwrapper_tests/dbwrapper_output_unchanged_on_deserialize_failure --catch_system_error=no --report_level=short --log_level=test_suite`

The old implementation returned `false` but changed `PartiallyDecoded::first` from `0xa1b2c3d4` to `1` in direct `Read`, iterator `GetValue`, and iterator `GetKey`; 5 of 18 assertions failed across plain and obfuscated cases. The stored malformed payload is exactly six bytes: `01 00 00 00 02 03`, so the first field is complete and the second field is truncated.

### Source fix

`src/dbwrapper.h` now parses keys and values into temporary objects and commits them only after the complete stream succeeds. Trivially default-constructible scalar outputs are value-initialized without reading possibly uninitialized caller storage; nontrivial or non-default-constructible objects are initialized from their already-constructed destination so serializer configuration and existing behavior are preserved. Normal assignment is used when available; types such as `CDiskBlockIndex` with intentionally deleted assignment are reconstructed at the same address only after successful parsing. Runtime exceptions from custom unserializers remain uncaught, while `std::ios_base::failure` still returns `false`.

### Passing-after and compatibility proof

- `cmake --build build_unit_clang19 --target test_bitcoin -j2` passed after compiling all DB wrapper callers, including non-default `DBHeightKey` and non-assignable `CDiskBlockIndex` instantiations.
- The focused malformed-output regression passed 18/18 assertions in both obfuscation modes.
- `TMPDIR=/data/my_storage/tmp/cycle56-dbwrapper-suite build_unit_clang19/bin/test_bitcoin --run_test=dbwrapper_tests --catch_system_error=no --report_level=short --log_level=test_suite` passed 13 cases and 2,472 assertions, including the existing runtime-exception classification and successful read tests.
- The dependent `blockfilter_index_tests,coinstatsindex_tests,txindex_tests,txospenderindex_tests,validation_chainstate_tests,validation_flush_tests` selection passed 19 cases and 61,104 assertions.
- The rebuilt normal `FUZZ=dbwrapper` target replayed all 1,835 corpus files plus initialization, 1,836 executions, with no crash, no new units, coverage `6035`, and peak RSS `1650 MB`.
- The rebuilt Clang ASan/UBSan `FUZZ=dbwrapper` target replayed the same 1,835 corpus files plus initialization, 1,836 executions, with no sanitizer diagnostic, no new units, coverage `18806`, and peak RSS `1683 MB`.
- The existing fuzzer's `FailUnserialize` and `UnexpectedFailUnserialize` oracles remain covered by the full DB wrapper suite and normal corpus replay; runtime errors still propagate.

### Verdict

Confirmed local persistence/API defect: a malformed or truncated database record could return failure while publishing a partially decoded caller output. This is reachable through corrupted or incompatible persisted bytes, affects direct lookup and iterator key/value APIs, and is not a network parser or consensus change. The smallest shared boundary fix is justified because all three APIs expose the same bool-plus-output contract and existing callers/tests treat failed reads as non-committing operations.

Independent verification forms were the failing-before regression, the fixed-after regression, historical exception classification, successful and non-default production instantiations, dependent index/chainstate behavior, normal corpus replay, and ASan/UBSan replay. The disposable mutation restoring direct deserialization only in `CDBWrapper::Read` rebuilt successfully and made exactly 2 direct-read sentinel assertions fail while leaving the 2 iterator paths passing; after restoration, the focused regression passed 18/18 again. The mutation did not enter history.

## Handoff

The source finding is ready for one independently authored source commit. Then update the uber ledger with the source commit and final state commit, verify no processes remain, and draw cycle 57 from the full catalog.
