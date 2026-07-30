# Serialization, deserialization, and untrusted-input sweep

## Cycle 121 start

- Initial selector: exact `shuf -i 0-98 -n 1` -> `36` (`sanitizer-analysis-matrix`), rejected because that campaign's sanitizer/static-analysis cells were already closed in Cycles 26 and 78.
- Reroll selector: exact `shuf -i 0-98 -n 1` -> `6` (`serialization-untrusted-input`).
- Branch: `uber-cycle-121-serialization-untrusted-input-20260730`.
- Cycle-start HEAD: `2c580e30ae0cc89042ab37cf53a6713d011d691a` (`uber-goal: close cycle 120 historical recipes`).
- Base: `origin/master` at `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` at the gate was `40 1031`.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Fresh `git fetch origin master`, tracked/index checks, and `git diff --check` passed. Persistent untracked artifacts and `test/cache/` are preserved and excluded.
- PID `777094` (`test_bitcoin --run_test=wallet_tests --log_level=test_suite`) and parent PID `725042` were observed and will not be touched.

### Scope and prior-finding exclusions

Mine a distinct byte-to-object contract from current code, recent history, tests, fuzz targets, and review precedent. Exclude Cycle 81's fixed Taproot BIP32 PSBT value-boundary over-read, Cycle 80's external advisory variants, Cycle 79's `parse_numbers` comparison, Cycle 78's TokenPipe status contract, Cycle 76's compact-block read-failure path, Cycle 75's descriptor x-only lookup, Cycle 73's LevelDB ownership path, and any generic serialization/property result already indexed elsewhere. A current finding needs a smallest malformed fixture or deterministic trace, an independently verified first invalid operation or state divergence, and a self-contained fix/test commit; a recipe-only close remains valid if all candidates are dismissed.

### Initial queue

1. Recent parser and deserializer fixes since Cycle 81, especially network message counts, RPC/JSON arrays, persisted database records, and public cryptographic objects.
2. Length/count fields that cross a stream boundary, allocation, cast, or loop before a complete domain check.
3. Non-canonical, truncated, duplicate, or trailing-byte encodings whose direct and wrapper callers may disagree.
4. Output and object mutation on late parse failure, including restart/recovery and fuzz harness paths.

Record exact source/history links, valid domains, malformed bytes, first-invalid operation, current tests, and any temporary mutation. Do not use a default datadir, wallet, key, or production database.

## Cycle 121 result

- Prior evidence and deduplication: the repository's tracked `doc/security/codex-security-bitcoin/investigation-report.md` sections 9 and 10, `doc/security/codex-security-bitcoin/evidence/raw_candidates_06.jsonl`, and the review coverage files already described this generic helper boundary as `psbt-global-unsigned-tx-value-boundary` and `psbt-input-non-witness-utxo-value-boundary`, with reproduction pending. This cycle treats those records as provenance and independently verifies/remediates them; it is not a new duplicate claim. The earlier Cycle 81 fix for the Taproot BIP32 helper remains excluded as a separate, already-fixed path.
- Hypothesis: `UnserializeFromVector` reads a declared PSBT value length, then deserializes directly from the outer stream and checks the byte count only afterward. A malformed short value can therefore let a nested parser consume bytes from later map data, perform parsing/allocation work before rejection, or report the wrong first failure. The helper is used by global unsigned transactions, input UTXOs, script witnesses, scalar fields, locktimes, and other PSBT values.
- Pre-fix reproducer: `psbt_tests/psbt_value_deserialization_does_not_read_past_value` encoded `PSBT_OUT_TAP_INTERNAL_KEY` with an empty declared value followed by a 36-byte later-map tail. On clean pre-fix code, the focused command exited 201 with `reader.size() == 4` instead of `36`; the fixed-size parser consumed 32 bytes beyond the declared field before throwing. This is a direct first-invalid-operation/reader-state proof, not a semantic-acceptance claim.
- Global entry-point reproducer: `psbt_tests/psbt_global_value_deserialization_does_not_read_past_value` encodes the PSBT magic, `PSBT_GLOBAL_UNSIGNED_TX` with an empty value, and ten zero bytes. The old outer parser can consume those ten bytes as an empty transaction before the late size mismatch; the regression requires all ten bytes to remain in the outer reader.
- Fix: `UnserializeFromVector` now rejects a declared length larger than the remaining stream, copies exactly the declared bytes into a bounded vector, deserializes through a `SpanReader` over that vector, and requires the subreader to be exhausted. Existing `ReadCompactSize` range checking remains in force, so the new copy is bounded by the project's 32 MiB serialization limit and by available input bytes.
- Validation: the initial normal build exposed only an environment issue (`ccache` could not create `/root/.cache/ccache/tmp`); `CCACHE_DISABLE=1 cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j2` passed. The normal focused and full `psbt_tests` runs passed, ending at 13 cases and 144/144 assertions. A clean Clang UBSan build in `/data/my_storage/tmp/cycle106-clang19-ubsan` was rebuilt after the header and test changes; with `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, the global regression and final full PSBT group passed, ending at 13 cases and 144/144 assertions. The UBSan build emitted only its pre-existing `-O0` object-size-sanitizer warning and no runtime diagnostic.
- Independent verification: the caller inventory matched the generic helper's global and input/output sites; the old-versus-new reader-size result independently establishes the boundary change; normal and UBSan builds exercised both a direct global transaction path and a fixed-size output path. No valid PSBT behavior changed in the focused/full regression set. Impact is limited here to pre-rejection cross-field parsing/resource work and malformed-input error locality; this cycle does not claim a remotely exploitable semantic-acceptance bypass or quantify worst-case amplification.
- Disposition: confirmed and fixed as a remediation of prior indexed security evidence. Preserve the two malformed fixtures, the exact commands, and the bounded-reader rationale in the source/test/journal commit. Next queue after close: revisit other untrusted serialization surfaces, excluding this helper and the Cycle 81 Taproot helper.

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

## Confirmed finding: Taproot BIP32 keypath values crossed PSBT field boundaries

- Surface: `PSBT_IN_TAP_BIP32_DERIVATION` and `PSBT_OUT_TAP_BIP32_DERIVATION` in `src/psbt.h`.
- Trust boundary: externally supplied PSBT bytes reach these maps through `DecodeRawPSBT`, including `decodepsbt`, `processpsbt`, `combinepsbt`, wallet PSBT RPCs, and fuzz deserialization.
- Mechanism: the old parser read the nested `std::set<uint256>` directly from the outer stream after reading the value length. It checked `hashes_len > value_len` only after the set parser had consumed and inserted every claimed hash. A malformed one-byte value containing a leaf-hash count of one therefore consumed 32 bytes from the following map entry before rejecting the field; larger following data could cause disproportionate set allocation and work.
- Minimal reproducer: `psbt_tests/taproot_bip32_keypath_does_not_read_past_value` and `psbt_tests/taproot_bip32_input_keypath_does_not_read_past_value` construct a one-byte value followed by a complete 36-byte unknown map entry and assert that the reader remains at the next field.
- Independent before evidence: after adding the output regression but before the source change, `./build_unit_clang19/bin/test_bitcoin --run_test=psbt_tests/taproot_bip32_keypath_does_not_read_past_value --log_level=message` failed with `reader.size() == tail.size()` reported as `[4 != 36]`, proving 32 bytes crossed the declared value boundary.
- Fix: read the declared value into a vector, parse it through a bounded `SpanReader`, check the leaf-hash count against the remaining bytes before set insertion, and then parse the remaining bytes as the key origin. Apply the same helper to input and output maps.
- After evidence: `ninja -C build_unit_clang19 test_bitcoin -j2` succeeded; `./build_unit_clang19/bin/test_bitcoin --run_test=psbt_tests --log_level=message` passed all 10 PSBT cases, including both regressions. `git diff --check` is clean.
- Classification: confirmed local parser defect; fixed in source/test commit `1a1a51aa96873c8fd8715b5a2b9b74a7e550a65f`.

## Cycle 81 completion

- Source/test commit: `1a1a51aa96873c8fd8715b5a2b9b74a7e550a65f` (`psbt: bound Taproot BIP32 keypath deserialization`), authored as `Lőrinc <pap.lorinc@gmail.com>`.
- Narrow verification: `ninja -C build_unit_clang19 test_bitcoin -j2`; `./build_unit_clang19/bin/test_bitcoin --run_test=psbt_tests --log_level=message` passed all 10 cases.
- Independent verification: `ninja -C build_unit_tsan_clang19 test_bitcoin -j2`; `./build_unit_tsan_clang19/bin/test_bitcoin --run_test=psbt_tests --log_level=message` passed all 10 cases with no TSAN report.
- The old-code regression failure and the passing-after result are recorded above. `git diff --check` passed before the source commit.
- No production behavior outside malformed Taproot BIP32 PSBT rejection changed. No relevant build, test, fuzz, sanitizer, daemon, or profiling process remains running.
- Cycle status: complete. The next run must re-check branch/base/HEAD, dirty state, known artifacts, process state, catalog/protocol/TSV hashes, existing journals/history, and review precedent before selecting a distinct next cell.
