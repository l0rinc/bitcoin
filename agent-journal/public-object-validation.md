# Public object parsing and validation variant analysis

## Cycle 27: descriptor inference must round-trip invalid public keys

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `15`
- Slug: `public-object-validation`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at the cycle gate: `ceb274f236881febe70aabeb534bde7f5d8babfd`
- HEAD after the source fix: `c125061aa4`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Gate: `git fetch origin master --quiet` passed; `origin/master...HEAD` was `2 822`; tracked source was clean; no source, test, fuzz, sanitizer, daemon, or profiling process was running.
- Agent-owned journals/catalog files and the pre-existing untracked `test/cache/` were left untouched.

### Scope and contract map

The selected goal compares equivalent public-object parsing and validation paths, with emphasis on malformed, noncanonical, impossible, and mixed-format keys. The first pass covered `CPubKey`, x-only keys, signatures, address/key I/O, scripts, descriptors, PSBT public-key storage, and wrapper/fuzz entry points.

The key contract distinction is intentional in consensus code:

- `CPubKey::IsValid()` checks serialized size and header shape only. `src/pubkey.h` documents that it does not check curve membership, and `CPubKey::Verify()` subsequently parses the key with libsecp256k1 before verification.
- `CPubKey::IsFullyValid()` checks curve membership with `secp256k1_ec_pubkey_parse`.
- Descriptor parsing already requires `IsFullyValid()` in `ParsePubkeyInner`, because a `pk(...)` descriptor must contain a usable curve point.
- Script inference used `InferPubkey()` to create the same `pk(...)` provider but only rejected hybrid encodings with `IsValidNonHybrid()`. It could therefore serialize a syntactically shaped, off-curve key into a descriptor that its own parser rejected.
- The public header for `InferDescriptor()` states that inference falls back to `raw()` when a more specific descriptor cannot be produced. A raw descriptor preserves an otherwise valid script byte-for-byte without asserting that its pushed bytes are a usable public key.

Other reviewed paths were not changed:

- `ExtractDestination()` intentionally uses syntactic validity for P2PK because that is the consensus script classification, and P2PK has no address destination.
- PSBT public-key insertion validates with `IsFullyValid()` before updating its provider map.
- Key I/O clears error locations at entry and existing address/key tests cover malformed base58 and bech32 inputs.
- Signature DER and descriptor checksum regression coverage already exercises prior parser-contract bugs.
- `InferXOnlyPubkey()` and taproot inference are a distinct adjacent cell. It was not changed in this cycle because the confirmed reproducer and minimal fix are for full public keys in P2PK inference; x-only validity and all taproot callers remain in the next queue.

### Fixed corpus and discovery evidence

A deterministic scratch corpus was created outside the repository at `/data/my_storage/tmp/public-object-validation-cycle27/`:

- `key_io/`: empty, valid and invalid Base58, valid/uppercase/noncanonical Bech32, and a short secret;
- `descriptor/`: valid `pk`/`tr`, truncated and hybrid keys, and a bad checksum;
- `script/`: fixed script-text cases;
- `signature/`: empty, short, and malformed DER bytes.

The existing and selected fuzz targets accepted these inputs without sanitizer diagnostics:

- Clang 19 ASan/UBSan libFuzzer: `key_io`, `descriptor_parse`, `parse_script`, and `ecdsa_signature_parse_der_lax`, each with a fixed seed and `-runs=1`;
- GCC 12 ASan/UBSan custom driver: the same four targets, using the driver's input-file interface.

The Clang unit command below passed before the production edit with 46 cases and 34,570 assertions:

```text
build_unit_clang19/bin/test_bitcoin --run_test=key_io_tests,base58_tests,bech32_tests,bip32_tests,descriptor_tests,key_tests,outputtype_tests --log_level=test_suite --report_level=short
```

The sanitizer version of that parser slice also passed before the edit with the same 46 cases and 34,570 assertions. These runs established the broad parser baseline; the defect required a cross-path round-trip oracle rather than a crash oracle.

### Confirmed finding

The following minimal script contains a 33-byte compressed-looking key whose x coordinate is all zero and is not a curve point:

```text
02 0000000000000000000000000000000000000000000000000000000000000000 OP_CHECKSIG
```

Before the fix, a temporary descriptor test constructed this P2PK script, called `InferDescriptor()`, then fed `ToString()` back into `Parse()`. The test failed with exit code 201:

```text
inferred descriptor: pk(020000000000000000000000000000000000000000000000000000000000000000)#rxgq8p4q
Parse error: pk(): Pubkey '020000000000000000000000000000000000000000000000000000000000000000' is invalid
```

This is a confirmed local contract defect. The output of a public inference API was not parseable by the corresponding public descriptor parser, and the invalid key was reachable from a script that the script classifier intentionally treats as P2PK based on its serialized shape. The failure is not a consensus acceptance change: consensus verification still uses the existing full libsecp256k1 parse.

### Fix and regression oracle

Commit `c125061aa4` (`descriptor: keep inferred invalid pubkeys parseable`) adds an `IsFullyValid()` check at the start of `InferPubkey()`. If the key is not a curve point, `InferScript()` continues to its existing raw-script fallback. The new test `descriptor_tests/infer_descriptor_off_curve_pubkey_falls_back_to_raw` asserts all of the following:

1. inference returns a descriptor;
2. the descriptor begins with `raw(`;
3. parsing the inferred descriptor succeeds; and
4. the parsed descriptor serializes back to the identical descriptor string.

The test is sensitive to the old behavior: before the three-line guard, the same test produced the `pk(...)` descriptor and failed during `Parse()`.

### Validation

Normal Clang 19 build and tests:

- `cmake --build build_unit_clang19 --target test_bitcoin -j$(nproc)`: exit 0;
- focused regression: 1 case, 4 assertions passed;
- `build_unit_clang19/bin/test_bitcoin --run_test=descriptor_tests --log_level=test_suite --report_level=short`: 11 cases, 30,415 assertions passed.

Clang 19 ASan/LSan/UBSan build and tests:

- `cmake --build /data/my_storage/tmp/sanitizer-analysis-matrix-cycle26/asan-unit-clang --target test_bitcoin -j$(nproc)`: exit 0;
- focused regression with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and the repository UBSan suppression/options: 1 case, 4 assertions passed;
- full `descriptor_tests` under the same options: 11 cases, 30,415 assertions passed;
- no ASan, LSan, UBSan, runtime-error, or leak diagnostic appeared.

The pre-fix failure, post-fix focused result, normal full-suite result, and sanitizer full-suite result are the independent behavioral and runtime evidence for the source change. `git diff --check` and the staged diff check passed. No source, test, fuzz, sanitizer, daemon, or profiling process remains running.

### Dismissed and adjacent candidates

- P2PK destination extraction using `IsValid()` is intentional consensus classification, not a descriptor round-trip defect.
- PSBT public-key validation and key I/O output/error-state behavior showed no new discrepancy in this cycle.
- Hybrid public keys already fall back to `raw()` and have an existing regression test.
- x-only/taproot inference may have a related validity predicate mismatch. It needs its own caller map and a P2TR round-trip corpus before any change; no finding is claimed yet.

### Handoff

Reopen this goal for the x-only/taproot cell, descriptor inference inside nested script contexts, public-key wrapper/binding parity, or a new malformed-input recurrence. Start by searching this journal and commit `c125061aa4` so the P2PK off-curve recurrence is not rediscovered. The next uber-goal cycle must re-check branch, base/HEAD, dirty state, processes, this ledger, the catalog, existing journals, history, issues, pull requests, and review precedent before drawing a distinct eligible goal.
