# Property, Exhaustive, and Algebraic Oracle Expansion

## Cycle 28

- Date: 2026-07-27
- Selector: `shuf -i 0-98 -n 1`
- Draw: `48`
- Slug: `property-oracle-expansion`
- Goal: expand weak example tests into properties, exhaustive small domains, differential references, metamorphic relations, failure-state postconditions, and minimized counterexamples.
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at gate: `a49a0b851502138111a9007a9a86d2f4136573fe`
- Divergence at gate: `origin/master...HEAD = 2 826`
- Tracked source diff and staged index at gate: empty
- Active process at gate and after verification: none

## Evidence map

The selected goal was checked against existing properties before adding another oracle.

- Transaction serialization and cached-hash contracts are already independently exercised in `src/test/fuzz/primitives_transaction.cpp`, `src/test/fuzz/transaction.cpp`, and `src/test/transaction_tests.cpp`. They cover witness/no-witness serialization, txid/wtxid identity, witness presence, total size, weight, mutable reconstruction, and exact reader exhaustion.
- Block serialization is already round-tripped in `src/test/fuzz/block.cpp`. `src/test/fuzz/block_header.cpp` covers header hash, null-state postconditions, conversion to `CBlock`, and locator cleanup. The kernel tests contain a fixed 80-byte header vector and compare header bytes from a block with the first 80 bytes of the raw block.
- Partial Merkle tree fuzzing and unit tests already cover independent Merkle roots, mutation flags, path reconstruction, malformed trees, output clearing, repeatability, and non-byte-aligned bit/byte padding. Recent history includes `87a1f211ad`, `8c9f7ce968`, and `2e8ffac12f` for those oracle contracts.
- Script operation counting has more than the current fuzz inequality: `script_ops` checks `accurate <= legacy` and parser postconditions, while `sigopcount_tests` covers known templates, malformed PUSHDATA sequences, P2SH extraction, multisig boundaries, witness versions, and transaction sigop costs. Recent history includes `85fee538b1` and `3a16ef4c2f`.
- Block-filter construction and encoding already have an independent element-set expectation, constructor/checker round trips, serialized byte stability, `MatchAny` versus individual `Match` equivalence, and BIP158 vector/header checks in `blockfilter_tests` and `blockfilter` fuzzing.

## Hypotheses and probes

### Empty transaction witness serialization

Hypothesis: serializing a default empty `CMutableTransaction` with `TX_WITH_WITNESS` might not round-trip because an empty input vector is also the witness marker position. This is an invalid-domain question because `CheckTransaction` explicitly rejects transactions with no inputs.

A temporary unit probe was added only to classify the behavior. The no-witness form consumed and reconstructed the empty object. The expected `std::ios_base::failure` for the all-zero witness form was not raised, so the probe exited 201 with `exception std::ios_base::failure expected but not raised`. The temporary test was removed immediately. The all-zero object is not a valid transaction or a valid network transaction fixture, and existing production callers use it as a default/failure sentinel. No source or test change is justified by this behavior.

### Script sigop accounting

The current `script_ops` target was inspected for a weak oracle. Its exact implementation path was compared with `CScript::GetSigOpCount`, P2SH handling, malformed parser behavior, known-template tests, and recent oracle-hardening history. No untested high-risk semantic cell was isolated without duplicating the implementation or current tests. A mutation would need to alter an already-covered sigop boundary or parser contract; no production defect was reproduced.

### Block and block-filter algebraic relations

The existing fuzzer and unit vectors already provide the relevant round-trip and metamorphic relations. A new header or filter round-trip would be duplicate coverage without an independent wire oracle. The fixed kernel header vector and BIP158 vectors remain the appropriate independent references.

## Commands and results

- `git fetch origin master --quiet`: passed.
- Gate recheck: branch and base above, `git diff --name-only` empty, `git diff --cached --name-only` empty, no `bitcoind`, `test_bitcoin`, fuzz, or build process.
- `cmake --build build_unit_clang19 --target test_bitcoin -j$(nproc)`: passed after the temporary probe was removed.
- `build_unit_clang19/bin/test_bitcoin --run_test=script_tests,sigopcount_tests --log_level=test_suite`: 29 selected cases passed; no errors.
- `build_unit_clang19/bin/test_bitcoin --run_test=blockfilter_tests --log_level=test_suite`: 8 cases passed; no errors.
- `build_unit_clang19/bin/test_bitcoin --run_test=transaction_tests --log_level=test_suite`: 18 cases passed; no errors.
- `FUZZ=script_ops build_fuzz_asan_clang19/bin/fuzz -runs=1000 /data/my_storage/tmp/property-oracle-expansion-cycle28/empty`: 1000 runs, no crash or sanitizer diagnostic.
- `FUZZ=blockfilter build_fuzz_asan_clang19/bin/fuzz -runs=300 /data/my_storage/tmp/property-oracle-expansion-cycle28/empty`: 323 runs, no crash or sanitizer diagnostic.
- `FUZZ=block_header build_fuzz_asan_clang19/bin/fuzz -runs=300 /data/my_storage/tmp/property-oracle-expansion-cycle28/empty`: 300 runs, no crash or sanitizer diagnostic.
- Temporary probe command: `cmake --build build_unit_clang19 --target test_bitcoin -j$(nproc) && build_unit_clang19/bin/test_bitcoin --run_test=transaction_tests/tx_empty_serialization_probe --log_level=test_suite`; it intentionally failed only because the invalid-domain assumption was disproved, then the probe was removed.

## Verdict

Dismissed for this cycle. The draw found no confirmed production defect, no justified test-oracle change, and no independent missing property. The empty witness serialization behavior is confined to an invalid transaction domain already rejected by consensus validation. Existing transaction, block, Merkle, script, and block-filter properties are stronger than the initial inventory suggested.

## Limitations

The local checkout has no `qa-assets` directory, so the fuzz smoke runs used a dedicated empty scratch corpus and libFuzzer-generated inputs. No change was made to production or tracked test sources. This cycle did not run MSan, Valgrind, or cross-architecture builds. It also did not claim that the broader property surface is exhausted.

## Next queue

1. Draw a new goal after the journal handoff.
2. Keep a separate future cell for exact x-only/taproot public-object validation and inference, rather than reopening the descriptor fix from cycle 27.
3. Revisit script sigop properties only with an independent P2SH/witness reference or a new consensus rule change.
4. Revisit transaction serialization only with a valid-domain boundary, a new witness format, or a reproducible failure involving accepted transactions.
