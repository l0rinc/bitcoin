# Random Goal Run Ledger

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
