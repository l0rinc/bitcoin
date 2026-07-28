# Serialization, deserialization, and untrusted-input sweep

## Cycle 81 start

- Selector: exact `shuf -i 0-98 -n 1` -> `6` (`serialization-untrusted-input`).
- Branch: `uber-cycle-81-serialization-untrusted-input-20260728`.
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`.
- Cycle-start HEAD: `7918b638fd060edaa6023853038d075e798e6cb2` (`journal: close external advisory cycle 80`).
- Cycle-start gate: `origin/master...HEAD` is `2 942`; tracked changes from the prior cycle are journal/state-only. Known untracked agent artifacts and `test/cache` are preserved and excluded from scope. Catalog, uber protocol, and goals TSV hashes match their recorded values. No relevant process is running.

## Scope and exclusions

This cycle audits byte-to-object boundaries in Bitcoin Core and libsecp256k1: network message framing, RPC/JSON and config parsing, block/transaction/script encodings, public-key/signature/scalar parsing, wallet/database/log formats, fuzz entry points, and persisted state. The goal is to find a concrete missing bound, non-canonical acceptance, truncation/cast error, mutation-before-failure, or unsafe output contract.

Closed local cells are excluded unless a distinct byte boundary or caller is proven: cycle 80's advisory variants, cycle 79's `parse_numbers` engine comparison, cycle 75's descriptor x-only lookup, cycle 76's compact-block read-failure, cycle 78's TokenPipe status contract, cycle 73's LevelDB constructor ownership, and earlier generic serialization/property campaigns. Existing findings and regression tests are evidence, not fresh candidates.

## Hypotheses

1. A length or count decoded from untrusted bytes may be widened, narrowed, multiplied, or used in a loop/allocation before a complete domain check on a production network, RPC, or persisted-input path.
2. A parser may accept multiple encodings or leave a partially written output/object after a later field fails, creating inconsistent state for a caller that checks only the return value.
3. A parser or serializer may have a valid-domain mismatch between direct, wrapper, fuzz, and persistence callers, especially for empty, truncated, oversized, duplicate, or non-canonical values.

## Initial surface map

- Framed network payloads and compact/block/transaction decoding in `src/net.cpp`, `src/net_processing.cpp`, `src/streams.cpp`, `src/serialize.h`, and `src/blockencodings.cpp`.
- Transaction, script, descriptor, address, key, signature, and scalar input paths in `src/primitives`, `src/script`, `src/key_io.cpp`, `src/pubkey.cpp`, `src/descriptor.cpp`, and `src/secp256k1`.
- RPC/JSON/config and persisted database boundaries in `src/rpc`, `src/common/args.cpp`, `src/wallet`, `src/dbwrapper.cpp`, `src/txdb.cpp`, and block/index storage.
- Existing unit/fuzz contracts in `src/test/*serialization*`, `src/test/fuzz/*deserialize*`, parser-specific fuzzers, and malformed-input functional tests.

## Candidate ledger

| ID | Surface / hypothesis | Trust boundary and planned evidence | Verdict |
|---|---|---|---|
| S0 | Inventory and prior-finding search | Search journals, history, test names, and existing malformed fixtures before selecting a byte path | open |
| S1 | Compact-size/count and allocation bounds | Trace varint/count readers into allocations and loops; use boundary fixtures and sanitizer/static evidence | open |
| S2 | Canonical and truncated transaction/script/object parsing | Compare direct and wrapper callers; test empty, truncation, duplicate, non-canonical, and trailing-byte cases | open |
| S3 | Output-on-failure and partial mutation | Identify output parameters/object mutation before late parse failure; compare complete pre/post state | open |
| S4 | Persisted/network format parity | Compare serialization, deserialization, restart, and fuzz contracts across block/index/wallet/database paths | open |

## Required evidence and handoff

For every candidate, retain the smallest input and exact command. A confirmed finding needs failing-before/passing-after behavior, a minimized sanitizer/static trace, or an executable proof of the violated contract. A dismissed candidate needs the domain invariant, caller trace, and a test or reference comparison that falsifies reachability. Preserve any temporary mutation and restore the tree before committing. If no source fix is justified, close with one journal/state snapshot and continue the uber loop.

