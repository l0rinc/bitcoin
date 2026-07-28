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

## Handoff

The next action is to add only the failing regression test for the DB output contract. Do not broaden the patch until the old implementation demonstrably mutates a sentinel on a malformed record. Keep scratch databases under a temporary directory and preserve raw malformed bytes and exact commands in this journal.
