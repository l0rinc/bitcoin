# Random Goal Run Ledger

## Cycle 308

- Selected index: `6`
- Goal slug: `serialization-untrusted-input`
- Goal title: Serialization, deserialization, and untrusted-input sweep
- Selection command: `shuf -i 0-107 -n 1`
- Catalog SHA-256: `633bb1216c6ddf55f6e4bdeda2a99dfb3eb8bb1878b0e6370dd49521c05069c9`
- Base commit: `49b264dd560a024702c81ca5c3493e180e5322d3`
- Branch: `uber-cycle-308-serialization-untrusted-input-20260802`
- Timestamp: `2026-08-02T23:36:21Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-6`

## Cycle 308 Result

- Finding: `wallet::DatabaseBatch::Read()` deserialized persisted values
  directly into caller-owned objects. The exact malformed value
  `01 00 00 00 02 03` changed a sentinel `PartiallyDecoded::first` to `1`
  before returning `false` in both SQLite and in-memory SQLite backends.
- Verdict: confirmed and fixed in `72c003fef9` (`walletdb: preserve outputs on
  decode failure`), authored as `Lőrinc <pap.lorinc@gmail.com>`.
- Fix: decode into a temporary, value-initialize trivially default-constructible
  outputs, and commit only after successful deserialization. The focused
  regression passed 16/16 assertions after the fix; the combined
  `db_tests,walletdb_tests,walletload_tests` run passed 12 cases and 643
  assertions.
- GCS candidate: the local block-filter index skips semantic Golomb-Rice
  validation after checking its coupled database hash. A huge-N payload is a
  learned bounded-work/recovery hypothesis, not a current finding, because
  ordinary one-sided file corruption fails the stored hash and a malformed
  hash-consistent record requires coupled local corruption or a writer defect.
- Learned suspicious surface: persisted GCS filter payload validation and
  file/LevelDB recovery symmetry. Added contiguous Goal 108,
  `persisted-gcs-filter-validation`, with seed journal
  `agent-journal/persisted-gcs-filter-validation.md`.
- Goal/catalog/seed commit: `7edd1f9112` (`goal: add persisted GCS filter
  validation campaign`). Catalog now contains 109 goals with IDs `0..108`;
  catalog SHA-256 is `6284d0369462c9c426d557943b9c4b71fd20e06658f7993aba04f1811ecb686a`,
  manifest SHA-256 is `280600d2c11b4437ff7d0bccee1211231aaa7c3620075cf03c0ecb08b7562c68`,
  generator SHA-256 is `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`,
  and random-run prompt SHA-256 is
  `56f2d4093caa99fcc54c8709bd18b5548228bde2d96a2b485ab9fe3a1cd55c2`.
- Validation limitation: `/data` and `/` are full, so no sanitizer rebuild or
  broad wallet suite was attempted; protected long-running jobs were kept
  alive and all scratch state is isolated under `/data/my_storage/tmp/cycle308-*`.

## Cycle 304

- Selected index: `52`
- Goal slug: `integer-overflow`
- Goal title: Integer overflow, narrowing, signedness, and division audit
- Selection command: `shuf -i 0-103 -n 1`
- Catalog SHA-256: `6ab27a9d21d866210694348713a6cecc2c4bf407fa4615986c8f482fba95747f`
- Base commit: `0f220b92529f28210a09ad965f8c49eff29b9297`
- Branch: `uber-cycle-304-integer-overflow-20260802`
- Timestamp: `2026-08-02T22:27:05Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-52`

## Cycle 304 Result

- Finding: `-maxconnections=4294967296` narrowed from `int64_t` to `int`,
  became zero automatic connections, and allowed startup; a representable
  `INT_MAX` value also made the old file-descriptor request exceed the `int`
  API domain.
- Verdict: confirmed and fixed.
- Finding commit: `a6abfded86` (`init: reject overflowing maxconnections`)
- Focused validation: `node_init_tests`, 4 cases and 6 assertions passed.
- Independent validation: Clang 19 `implicit-conversion` build completed;
  oversized direct startup returned the explicit range error, and `INT_MAX`
  startup completed with the expected system-limit reduction. The broad
  sanitizer configuration also emitted unrelated pre-existing libsecp256k1,
  crypto, and CRC32C diagnostics; none referenced `src/init.cpp`.
- Learned suspicious surface: integer option narrowing in `-par`, duration
  options, wallet fee sizes, and block-filter/index height arithmetic.
- Added contiguous goal: `104` (`integer-option-boundaries`)
- Goal/seed/catalog commit: `d93bc4098e`
- Catalog count after extension: 105 goals, IDs `0..104`
- Catalog SHA-256: `3b62db081945f5375ac7f152e31ace458c1c807f89380f79ac088944aecc3ffa`
- Manifest SHA-256: `6f7281d74b0f621ff0f28b50813ebcedeedf0ff3c66bc6e4cebe7aee9e47fd42`
- Generator SHA-256: `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`
- Random-run prompt SHA-256: `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`

## Cycle 307

- Selected index: `55`
- Goal slug: `alternative-implementation`
- Goal title: Alternative-implementation compatibility-difference audit
- Selection command: `shuf -i 0-106 -n 1`
- Catalog SHA-256: `fb4f3f314db4d15105120db4109ddb2bcfda208e26290a400ddb9028644d7a62`
- Base commit: `bf8b74bacfde5bb19be05bb960acbedf68b54afa`
- Branch: `uber-cycle-307-alternative-implementation-20260802`
- Timestamp: `2026-08-02T23:19:53Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-55`

## Cycle 307 Result

- Finding: the pinned rust-bitcoin V2 message decoder recognizes BIP324 short IDs 1-28, but its long-command dispatcher routes optimized messages such as `mempool`, `tx`, `cmpctblock`, and `blocktxn` to `NetworkMessage::Unknown`. The valid long-form `mempool` fixture is `00 6d 65 6d 70 6f 6f 6c 00 00 00 00 00`; short form `0f` decodes as `MemPool`.
- Core verdict: dismissed. Core's reserved short IDs 29-36 were intentionally added as ignored extension slots by `6a129983c9b`; ID 37 is BIP434 FEATURE and is version-gated. btcd's missing compact-block IDs are an unsupported-message boundary because the pinned wire package has no compact-block message types.
- External verdict: report-ready rust-bitcoin long-form interoperability gap at pinned commit `607e8b2fe0d8f1ebe06923dbbc0ca6afdf00d1d1`; no local source/test change justified.
- Learned suspicious surface: BIP324 short/long message-type parity, extension-ID freshness, bidirectional wire fixtures, and version-gated message registries. Added Goal 107, `bip324-short-id-parity`, with seed journal `agent-journal/bip324-short-id-parity.md`.
- Verification: BIP324 v1.0.2 specification, Core source/history, rust-bitcoin source, and btcd source inventory. Rust/Go execution was unavailable. Core `net_tests` execution was blocked by full `/` and `/data` filesystems during chain fixture setup.

## Cycle 306 Result

- Finding: all three libsecp256k1 vector generators copied external JSON
  comments into C block comments without escaping `*/`. Minimal hostile
  ECDSA, ECDH, and Silent Payments fixtures emitted a live `int
  generated_marker` into the generated test-vector initializer and failed C
  syntax compilation before the fix.
- Verdict: confirmed and fixed.
- Finding commit: `1ec5c95460` (`secp256k1: sanitize generated vector comments`)
- Fix: shared `sanitize_c_comment()` replaces `*/` with `* /`; all three
  generators use it, and `tools/wycheproof_utils.py` is now listed in
  `Makefile.am` distribution inputs.
- Focused verification: hostile generated headers compiled under `cc
  -std=c11 -fsyntax-only` after the fix; all three returned `compile=0`.
- Regeneration verification: ECDSA, ECDH, and Silent Payments production
  headers remained byte-identical before and after the fix. Hashes were
  `1e3c11ff4c5c83cbd0d79b3ede6a47309e2074f0f9432f3aef09e3bb2c9004c5`,
  `040085b0859e4cc41105bfecec825c76c616fe89a703f8221807c6433ba9f3d2`, and
  `8d88aead1f2f359aca31ac8c803001c55b1231be187839cafbe9c6959cedbbcc`.
- Consumer validation: Clang 19 and GCC ECDSA/Silent Payments modules passed;
  a fresh Clang 14 Release ECDH-enabled CMake build passed all ECDH tests,
  including `test_ecdh_wycheproof`.
- Learned suspicious surface: generated-source escaping and provenance across
  C/C++, Rust, shell, manpage, build, and metadata generators. Extend the
  catalog with goal `106`, `generated-source-boundaries`, and seed journal
  `agent-journal/generated-source-boundaries.md`.
- Goal/catalog commit: `72c09f6e2c` (`goal: add generated source boundary campaign`)
- Catalog count after extension: 107 goals, IDs `0..106`
- Catalog SHA-256: `fb4f3f314db4d15105120db4109ddb2bcfda208e26290a400ddb9028644d7a62`
- Manifest SHA-256: `5769fb6a16ca00af236d50c081377375d6c1bb1a2642fab182b4b7e99bc18573`
- Generator SHA-256: `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`
- Random-run prompt SHA-256: `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`

## Cycle 305

- Selected index: `69`
- Goal slug: `backend-differential`
- Goal title: SIMD, assembly, and portable-reference backend differential
- Selection command: `shuf -i 0-104 -n 1`
- Catalog SHA-256: `3b62db081945f5375ac7f152e31ace458c1c807f89380f79ac088944aecc3ffa`
- Base commit: `54afa66613a649a55974ec83c67efd12cb02052a`
- Branch: `uber-cycle-305-backend-differential-20260802`
- Timestamp: `2026-08-02T22:50:50Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-69`

## Cycle 306

- Selected index: `39`
- Goal slug: `deterministic-artifacts`
- Goal title: Generated-artifact and test-vector determinism audit
- Selection command: `shuf -i 0-105 -n 1`
- Catalog SHA-256: `ae927e6bca7b2406e318ac893962e481862f22ff46443228e6908131ff7dca13`
- Base commit: `ed71bceff261929dc735765286c95445bb89327d`
- Branch: `uber-cycle-306-deterministic-artifacts-20260802`
- Timestamp: `2026-08-02T23:07:27Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-39`

## Cycle 305 Result

- Finding: `minisketch_decode()` narrowed its public `size_t max_elements`
  through `int`, and the decoder then evaluated signed `1 + max_count`.
  Generic and CLMUL builds returned `-1` for a valid one-element sketch when
  the caller supplied `INT_MAX`, `INT_MAX + 1`, or `SIZE_MAX`.
- Verdict: confirmed and fixed.
- Finding commit: `9a8cf446ba` (`minisketch: preserve large decode bounds`)
- Focused verification: generic and CLMUL external probes returned element 7
  for all three large bounds; pre-fix ASan/UBSan replay independently reported
  signed overflow at `src/minisketch/src/sketch_impl.h:401` in both backends.
- Broad validation: normal and sanitized no-VERIFY/VERIFY Minisketch suites
  passed at complexities 2 and 4 in generic and CLMUL trees.
- Learned suspicious surface: Minisketch serialized-size multiplication,
  capacity/max-elements arithmetic, and decode return counts.
- Added contiguous goal: `105` (`minisketch-api-size-arithmetic`)
- Goal/seed/catalog commit: `14d0a8782f`
- Catalog count after extension: 106 goals, IDs `0..105`
- Catalog SHA-256: `ae927e6bca7b2406e318ac893962e481862f22ff46443228e6908131ff7dca13`
- Manifest SHA-256: `233d8f1d52e98a4a3d8d134df4e7bd35792af2ea6814d5af02185cf8fdc20510`
- Generator SHA-256: `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`
- Random-run prompt SHA-256: `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`
