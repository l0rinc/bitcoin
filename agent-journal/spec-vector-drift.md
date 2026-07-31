## Cycle 190: BIP341/342 Specification and Vector Parity

### Cycle Identity and Fresh Gate

- Draw sequence: `shuf -i 0-98 -n 1` -> `61` (`stateful-contract-fuzzer`), reroll -> `8` (`locking-threading`), reroll -> `81` (`spec-vector-drift`). Goals 61 and 8 were rerolled because their exact AddrMan and `SignalInterrupt` cells were already closed; this cycle uses the distinct BIP341/342 cell left open by the prior Goal 81 audits.
- Selected goal: `spec-vector-drift` (specification, test-vector, and formal-model drift audit).
- Worktree: `/data/my_storage/bitcoin`.
- Branch: `uber-cycle-190-spec-vector-drift-20260731`.
- Start HEAD: `b3a92ed3e7bb58fd1ec74809391689b71ebbfff8`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Start divergence (`HEAD...origin/master`): `1170 42`.
- Fresh gate: `git diff --check` passed; tracked worktree clean; known unrelated untracked artifacts were preserved. PIDs `777094` and `956381` were alive and untouched.
- Gate hashes: catalog `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, prompt `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, corrected TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, state `bb83d2fd57312fc72612ce6c0e603902a44526fb804d5a18381e0094dfd486dc`.

### Scope and Hypothesis

The prior Goal 81 cells closed BIP340 vector parity, the test-vector generator's `testnet4` metadata drift, and BIP352 silent-payment specification/vector parity. This cycle tests the still-open BIP341/342 Taproot and Tapscript contract: authoritative rules, checked-in vectors, script/interpreter consumers, sighash construction, annex handling, control-block validation, and version/leaf semantics should agree at their exact boundaries.

The trust boundary is the version-pinned BIP341/BIP342 text and vectors, Bitcoin Core's Taproot script and transaction validation code, the functional/unit consumers, and any local vector generator or formal reference. The first hypothesis is that an authoritative rule or edge vector is either absent, stale, generated under a different assumption, or consumed by a path whose behavior is not tested. No consensus change is assumed from a specification difference alone.

### Investigation Plan

1. Pin authoritative BIP341/BIP342 sources and locate all local vectors, generators, script tests, sighash tests, and validation consumers.
2. Build a contract table for witness-v1 key-path/script-path validation, tagged hashes and sighash fields, annex, control-block parity/depth, leaf versions, `OP_SUCCESSx`, MINIMALIF, resource limits, and failure behavior.
3. Compare valid/invalid/edge vectors and independently derive selected results with a small reference or direct byte-level calculation. Search history and prior journal cells before treating a discrepancy as new.
4. Reproduce any mismatch on clean HEAD, classify intentional policy versus drift, and add the smallest regression test or vector fix only when the expected behavior has authoritative provenance.

### Authority and Contract Comparison

- Pinned sources: `https://github.com/bitcoin/bips/blob/9783d61f1b9c81231581fee026c8e8cb9499d265/bip-0341.mediawiki`, `https://github.com/bitcoin/bips/blob/9783d61f1b9c81231581fee026c8e8cb9499d265/bip-0342.mediawiki`, and `https://github.com/bitcoin/bips/blob/9783d61f1b9c81231581fee026c8e8cb9499d265/bip-0341/wallet-test-vectors.json`.
- The `bitcoin/bips` repository `master` ref was pinned at `9783d61f1b9c81231581fee026c8e8cb9499d265`. The fetched BIP341 and BIP342 raw documents had SHA-256 values `463690d4c409587759c0ff6e8b8fc3a2bf6cb2cdda0c06ff193d317e0ed898ec` and `29641340adec741b695be99ae3dc6e2fd025ea6566f9d6c2544ed5d47db4d822`, respectively.
- The BIP341 text requires annex detection and CompactSize hashing, control-block lengths `33 + 32m` for `0 <= m <= 128`, leaf-version masking, output-key parity, and success for future leaf versions after commitment validation. BIP342 requires the listed `OP_SUCCESSx` set, pre-execution stack-limit bypass, MINIMALIF, disabled multisig opcodes, tapscript signature ordering, and the per-input sigops budget.
- `src/script/script.cpp:IsOpSuccess`, `src/script/interpreter.cpp:EvalChecksigTapscript`, `ExecuteWitnessScript`, `VerifyTaprootCommitment`, `SignatureHashSchnorr`, and the `OP_CHECKSIGADD` path match those rules. In particular, `ExecuteWitnessScript` checks `OP_SUCCESSx` before initial stack and element limits, and the tapscript signature helper charges non-empty signatures before accepting known or unknown public-key types.
- `test/functional/test_framework/script.py:is_op_success` contains the same opcode set. The direct functional Taproot scenario constructs and spends script trees, annexes, control blocks, and tapscript edge cases; its vector-generation switch is intentionally disabled (`GEN_TEST_VECTORS = False`) so it is not an artifact producer.

### Vector Consumer and History Evidence

- The official BIP341 wallet vector file and `src/test/data/bip341_wallet_vectors.json` are byte-identical: both are 29,296 bytes, parse as JSON, and have SHA-256 `403e19fb81dd1f31e745699216308f61fb403774b2aafa87b631b8f7c042d37f`; `cmp` returned 0.
- The seven `scriptPubKey` cases are consumed by `script_standard_tests/bip341_spk_test_vectors`. Its recursive parser handles null, leaf, and two-child tree nodes, then checks the expected output script, BIP350 address, merkle root, and every expected control block. The one `keyPathSpending` case is consumed separately by `script_tests/bip341_keypath_test_vectors`, which checks all precomputed hashes, the tweak, Schnorr signature, Taproot sighash, and hashed SigMsg.
- History search found no newer authoritative BIP341/BIP342 vector or rule update missing from these consumers. The earlier Goal 81 entries already closed BIP340, testnet4 metadata, and BIP352 cells; this cycle stayed on the distinct BIP341/342 cell and did not reopen them.

### Reproduction and Verification

The following current-source commands passed:

```text
CCACHE_DIR=/data/my_storage/tmp/cycle190-ccache cmake --build /data/my_storage/tmp/cycle105-clang19-release --target test_bitcoin -j2
TMPDIR=/data/my_storage/tmp/cycle190-script-runtime-1 /data/my_storage/tmp/cycle105-clang19-release/bin/test_bitcoin --run_test=script_standard_tests/bip341_spk_test_vectors --random=190341 --log_level=test_suite --report_level=short
Running 1 test case ... 46 assertions out of 46 passed
TMPDIR=/data/my_storage/tmp/cycle190-script-runtime-2 /data/my_storage/tmp/cycle105-clang19-release/bin/test_bitcoin --run_test=script_standard_tests --random=190342 --log_level=message --report_level=short
8 test cases out of 1148 passed; 199 assertions out of 199 passed
TMPDIR=/data/my_storage/tmp/cycle190-script-runtime-3 /data/my_storage/tmp/cycle105-clang19-release/bin/test_bitcoin --run_test=script_tests/bip341_keypath_test_vectors --random=190343 --log_level=message --report_level=short
1 test case out of 1148 passed; 55 assertions out of 55 passed
```

The preceding sanitized script run also passed `script_tests` in full: 27 cases and 505,273 assertions. The direct `test/functional/feature_taproot.py` run passed its unit, activation, 2,800 generated spending cases, and nonstandard spending cases. The release build of `bitcoind` and `bitcoin-cli` also completed successfully. An initial parallel test attempt used nonexistent temporary directories and collided on shared test globals; it was discarded as an environment/setup failure and rerun sequentially with existing per-run `TMPDIR` values.

### Verdict and Handoff

**Dismissed as current BIP341/BIP342 specification or vector drift; no confirmed finding.** The authoritative rules, exact wallet vectors, production consumers, and independent functional scenarios agree on the inspected contract. The apparent script-path consumer gap was a false lead: the complete official `scriptPubKey` section is already checked by `bip341_spk_test_vectors`. No source, vector, or test change is justified by this cycle.

### Limitations and Next Queue

- This cycle did not provide ARM, 32-bit, big-endian, GCC, Valgrind, timing, or compiler-differential evidence; those remain separate platform and implementation campaigns.
- The BIP341/BIP342 functional run used deterministic local scratch data and the current source, but did not regenerate the disabled JSON producer. The checked-in official artifact was independently compared to the pinned upstream file instead.
- Next cycle: perform a fresh gate, draw with the exact selector, and exclude this BIP341/BIP342 cell unless the pinned BIP text, vector file, or local consumer changes. Keep BIP327/MuSig and BIP324 as distinct future vector families.

## Cycle 153: Test-Vector Generator Metadata Drift

### Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `81`
- Selected goal: `spec-vector-drift` (specification, test-vector, and formal-model drift audit)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `uber-cycle-153-spec-vector-drift-20260730`
- Start HEAD: `0ad309ac7e393a6d7a56a85438abaa44f3804df6`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `1090 42` (`origin/master...HEAD`)

### Scope and Hypothesis

This cycle audited the network metadata contract for generated key/address vectors. The checked-in `src/test/data/key_io_valid.json` uses `testnet4`, while `contrib/testgen/gen_key_io_test_vectors.py` is the deterministic producer consumed by the documented regeneration command. The hypothesis was that regeneration silently produced a different chain model and therefore stale or misleading test vectors.

The trust boundary is the test-vector generator, its JSON artifact, and `key_io_tests`, which maps the metadata through `ChainTypeFromString` before decoding and re-encoding each address/key. The current chain model names the replacement network `testnet4` and gives it the `tb` Bech32 HRP, so the generated metadata must use that exact chain identifier.

### Evidence Before the Fix

- History search found commit `7c200ece80575d399a552f5757c07ac2c8c7ec6c` (`test: use testnet4 in key_io_valid.json`). It changed 18 checked-in metadata fields from `test` to `testnet4` in the JSON file, but did not update the generator.
- The generator still emitted `test` in four Base58 templates and four Bech32 templates. Its deterministic seed and 70-vector count otherwise reproduced the current artifact exactly.
- Reproduction before the edit:

  ```text
  python3 contrib/testgen/gen_key_io_test_vectors.py valid 70 > /data/my_storage/tmp/cycle153-keyio-generated.json
  diff -u src/test/data/key_io_valid.json /data/my_storage/tmp/cycle153-keyio-generated.json
  ```

  The diff contained only the 18 `test` versus `testnet4` metadata changes. This was not merely cosmetic: a regenerated file would direct `key_io_tests` through `ChainType::TESTNET` rather than the checked-in `ChainType::TESTNET4` contract.
- `src/kernel/chainparams.cpp` confirms that both supported test networks use `bech32_hrp = "tb"`, while `src/util/chaintype.cpp` maps `testnet4` to `ChainType::TESTNET4`; the checked-in data and current chain model therefore agree on the intended identifier.
- The RPC BIP350 test also leaves official valid `tb1` examples commented because that test class is explicitly mainnet-only. The independent Python SegWit reference decoded the three official BIP350 testnet valid vectors and reproduced their expected script payloads; this cycle did not broaden the mainnet-only functional test.

### Fix

Changed only the eight generator metadata tuples in `contrib/testgen/gen_key_io_test_vectors.py` so test-network outputs use `testnet4`. No generated JSON artifact or production address implementation required a change because the checked-in artifact already had the intended metadata.

### Verification

After the edit, deterministic regeneration was byte-identical:

```text
python3 contrib/testgen/gen_key_io_test_vectors.py valid 70 > /data/my_storage/tmp/cycle153-keyio-generated-after.json
cmp -s src/test/data/key_io_valid.json /data/my_storage/tmp/cycle153-keyio-generated-after.json
valid vectors: byte-identical
sha256: 90bd1d35d12763e0d00c5400b2c9fe551e532a821e1e466c20cad3aced70a7fe
```

The invalid generator also emitted 70 entries accepted by `python3 -m json.tool`, and `python3 -m py_compile contrib/testgen/gen_key_io_test_vectors.py` passed. The direct C++ consumer test passed:

```text
/data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=key_io_tests --log_level=test_suite
Running 4 test cases ... *** No errors detected
```

`git diff --check` passed. The long-running wallet test process PID 777094 was observed and left untouched. No production daemon, sanitizer, fuzz, or profiling process was started.

### Verdict and Handoff

**Confirmed test-vector generator drift; fixed.** A checked-in deterministic artifact could not be reproduced from its own generator after the testnet4 migration. The generator now names the same chain selected by the artifact and current chain type implementation. The next cycle should rerun the full gate, redraw with the exact selector, and exclude this closed generator/artifact cell unless a new authoritative vector change creates a distinct hypothesis.

---

# Specification, Test-Vector, and Formal-Model Drift Audit

## Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `81`
- Selected goal: `spec-vector-drift` (specification, test-vector, and formal-model drift audit)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD: `1dcc2da988ee625fbc5d7d55eb6f894c1103ec52`
- Catalog: `agent-journal/reusable-continuous-agent-goals.md`

## Scope and Hypothesis

This cycle selected the BIP and vector drift surface. The concrete hypothesis was that the repository's BIP340 vectors or their consumers had drifted from the authoritative BIP340 test-vector file, or that a recent variable-length-message update had left a production-relevant validation path untested or using an obsolete message contract.

The trust boundary is the BIP340 Schnorr implementation and its two consumers: Bitcoin Core's `CKey`/`XOnlyPubKey` wrappers and the embedded libsecp256k1 test suite. The expected contract is exact signing/verifying behavior for the supported message domain, with invalid public keys and signatures rejected.

## Specification and History Evidence

The repository file `test/functional/test_framework/bip340_test_vectors.csv` has 19 data rows, including the four variable-length-message cases added by Bitcoin Core commit `b184f5c87c` (`test: update BIP340 test vectors and implementation (variable-length messages)`). That commit records the upstream BIP pull #1446 commit `200f9b26fe0a2f235a2af8b30c4be9f12f6bc9cb` as its source.

The BIP repository `master` ref resolved for this cycle to `9783d61f1b9c81231581fee026c8e8cb9499d265`. Comparing the local CSV with the pinned upstream file at that commit produced no content difference after removing the upstream CRLF line endings:

```text
diff -u <(sed 's/[[:space:]]*$//' test/functional/test_framework/bip340_test_vectors.csv) <(curl --fail --location --silent https://raw.githubusercontent.com/bitcoin/bips/9783d61f1b9c81231581fee026c8e8cb9499d265/bip-0340/test-vectors.csv | sed 's/[[:space:]]*$//')
pinned_bip340_diff_status=0
```

The line-ending-normalized SHA-256 was identical on both sides:

```text
01c8cabba63b4c9b2f44c975902990086a4fe56eee9d265b187d1e2c1d98ccfb
```

The raw byte hashes differ only because the upstream file uses CRLF and the repository copy uses LF. This is a representation difference, not vector drift.

## Consumer and Domain Check

The Python functional framework reads all 19 CSV rows and explicitly permits variable-length messages. The libsecp256k1 Schnorr test implementation also has variable-length `msglen` helpers and covers the complete BIP vector set, including the zero-, one-, 17-, and 100-byte cases.

Bitcoin Core's public wrappers intentionally use `const uint256&` for `CKey::SignSchnorr`, `KeyPair::SignSchnorr`, and `XOnlyPubKey::VerifySchnorr`. The Core unit test therefore embeds the fixed 32-byte BIP340 signing and verification vectors only. It does not silently truncate or reduce the four variable-length vectors; those inputs are outside this wrapper's declared contract. The wrapper test does cover the relevant valid and invalid 32-byte signing/verification cases, including the field-size, group-parity, infinity, and curve-order rejection vectors.

The targeted current build test was:

```text
build_unit_clang19/bin/test_bitcoin --run_test=key_tests/bip340_test_vectors --random=123456 --log_level=test_suite
```

It ran one enabled case, exited with status 0, and ended with `*** No errors detected`. The skipped suites were unrelated to the selected case. Prior fresh current-source libsecp backend runs also passed the Schnorr vector group; no new rebuild was needed to resolve the content/domain question.

## Review and History Search

The relevant history search found the vector update commit and the earlier BIP340 vector introduction. The update's rationale is explicit in its subject and source reference, and the current API signatures and comments support the fixed-`uint256` Core wrapper boundary. No later local follow-up changes that contract. No live external review or issue state was used in this cycle.

## Verdict

**Dismissed as current specification or vector drift; no confirmed finding.** The authoritative BIP340 vectors match the repository copy, all supported Core-domain vectors pass, and the variable-length additions are covered by the interfaces that accept arbitrary message lengths. The Core wrapper's fixed 32-byte domain is explicit in its type contract and is not an accidental omission.

## Limitations and Rejected Leads

- The full functional test runner was not executed; the direct C++ vector case and pinned source comparison were sufficient for this hypothesis.
- This cycle did not audit BIP341/342 transaction sighash vectors, BIP327/328 MuSig vectors, or BIP324 transport vectors.
- The upstream source was fetched over the network at a pinned commit, but no change was made to the repository or to the vector file.
- A future cycle should use the same pinned-source method for one of the remaining BIP341/342, MuSig, or transport vector families and inspect any newly updated upstream vectors.

## Next Queue

1. Draw another eligible catalog goal and record the exact command and draw before work.
2. Reopen this goal for a different protocol vector family or when a specification commit changes the supported Core domain.
3. Preserve the distinction between Core's fixed-hash wrapper and libsecp's variable-length message API when reviewing future vector additions.

## Cycle 92: BIP341, MuSig2, and BIP324 Vector Recurrence

### Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `81`
- Selected goal: `spec-vector-drift`
- Branch: `uber-cycle-92-spec-vector-drift-20260729`
- Start HEAD: `ceca2378b0aeb0906f57d70bf2fbacae576ac0ea`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `2 971` (`origin/master...HEAD`)

### Scope and Hypotheses

The preserved BIP340 record was excluded. This cycle audited the distinct BIP341/BIP342 wallet and script-vector consumers, BIP327 MuSig2 generated vectors, and BIP324 packet and EllSwift XDH vectors. The main hypotheses were that an authoritative vector update had not reached a consumer, that a generated header was stale relative to its JSON source, or that a test comment still claimed provenance that no longer held.

The BIP324 packet CSV is an external cryptographic test oracle. The independent verification forms were the current `bitcoin/bips` CSV, Bitcoin Core's `BIP324Cipher` packet test, and libsecp256k1's direct `secp256k1_ellswift_xdh` test. The local source and test paths were checked on clean source before the fix.

### Evidence Before the Fix

- `src/test/data/bip341_wallet_vectors.json` has 15 `given` entries and was byte-identical to `https://raw.githubusercontent.com/bitcoin/bips/master/bip-0341/wallet-test-vectors.json`. Both raw SHA-256 values were `403e19fb81dd1f31e745699216308f61fb403774b2aafa87b631b8f7c042d37f`.
- Regenerating `src/secp256k1/src/modules/musig/vectors.h` with the repository's `tools/test_vectors_musig2_generate.py` and all six current BIP327 JSON inputs produced an exact semantic match. The only raw difference was the generator path in the banner; after normalizing that path, both headers hashed to `def098fc713451ce9e4c605226be538c0387aad342677608a217f6fbafe672`.
- Bitcoin Core's seven `TestBIP324PacketVector` calls matched all 14 mapped fields of the current BIP324 packet CSV. The normalized extracted-field SHA-256 was `3924170e7b64f674d92090a3ed51e6b8323f3602096cb52dccbcae16cde264f3`.
- The distinct libsecp array `ellswift_xdh_tests_bip324[]` had seven rows, but six rows still used the former packet corpus. Its extracted XDH tuple hash was `b0f995dc8bba65304be92036ec23de54bf91ebc9228efa046b2972fde31b4f70` before the refresh, versus the current BIP324 tuple hash `1ac9d5c3a82dc3781788c8dd456d455901afa63f902a5b58b70e9028ddcd22d3`.
- The stale rows were reachable by `ellswift_xdh_test_vectors_tests`; they were not dead data. They remained mathematically valid old test cases, so the test passed while failing to exercise the current BIP324 vector corpus. Bitcoin Core commit `51877f2fc5eb02b4229258b4b43731c4da843793` updated the packet vectors from BIP PR #2016, but the later libsecp subtree test file had not received the corresponding six-row update.

### Fix

Updated only the six stale rows in `src/secp256k1/src/modules/ellswift/tests_impl.h` from the current BIP324 CSV fields `in_priv_ours`, `in_ellswift_ours`, `in_ellswift_theirs`, `in_initiating`, and `mid_shared_secret`. The first row was already current. No production implementation or unrelated test corpus changed.

### Verification

The refreshed extracted XDH tuple hash equals the current BIP324 hash:

```text
BIP324 XDH mapped upstream: b0f995dc8bba65304be92036ec23de54bf91ebc9228efa046b2972fde31b4f70
BIP324 XDH mapped local:    b0f995dc8bba65304be92036ec23de54bf91ebc9228efa046b2972fde31b4f70
```

The following deterministic runs passed:

```text
/data/my_storage/tmp/cycle92-secp-ellswift/bin/tests --target=ellswift_xdh_test_vectors_tests --iterations=1 --seed=92 --log=1
Test ellswift_xdh_test_vectors_tests PASSED

/data/my_storage/tmp/cycle92-secp-ellswift/bin/tests --target=ellswift --iterations=1 --seed=92
/data/my_storage/tmp/cycle92-secp-ellswift/bin/tests --target=musig --iterations=1 --seed=92

/data/my_storage/tmp/cycle92-secp-ellswift/bin/tests --target=ellswift --iterations=16 --seed=92 --log=1
All EllSwift vector, round-trip, create, shared-secret, correctness, bad-scalar, and context tests passed; total execution time 23.670 sec.

/data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=bip324_tests,script_tests,script_standard_tests --catch_system_error=no --log_level=message
Running 37 test cases... *** No errors detected
```

`git diff --check` passed. The test build could not run Valgrind because Valgrind is not installed; this is unrelated to the vector refresh.

### Verdict and Handoff

**Confirmed test-vector drift; fixed.** The drift was limited to libsecp's embedded BIP324 EllSwift XDH oracle, not the BIP324 implementation or the BIP341/MuSig2 corpora. The current vectors now agree across the authoritative CSV, Bitcoin Core packet tests, and direct libsecp XDH tests. A source/test/journal commit should contain this six-row refresh, authored as `Lőrinc <pap.lorinc@gmail.com>`; the cycle close snapshot must follow it. Remaining scope for later protocol audits: BIP342 semantic vectors/formal rules and any future BIP324 source update.
## Cycle 159: BIP352 Silent-Payment Vector Drift

### Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `81`
- Selected goal: `spec-vector-drift` (specification, test-vector, and formal-model drift audit)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `uber-cycle-159-spec-vector-drift-20260730`
- Start HEAD: `b89ef9a7569c2120c0ee62148b4d6fb729644d9a`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Start divergence: `42 1100` (`origin/master...HEAD`)

### Scope and Hypothesis

This cycle selected the unclosed BIP352 silent-payment vector family. The concrete hypothesis was that the checked-in JSON corpus, generated C header, local libsecp256k1 consumer, or recent BIP352 corner-case additions had drifted from the authoritative BIP text and reference implementation. BIP328 and the new BIP32 framework vectors were surveyed first and did not supply a stronger open mismatch; the BIP352 corpus was selected as the distinct authoritative cell.

The trust boundary is the BIP352 specification and its `send_and_receive_test_vectors.json` plus `reference.py`, the local conversion script, `vectors.h`, and the `silentpayments` test module. The relevant specification contract includes compressed/X-only input handling, outpoint ordering, zero-sum rejection, label derivation, output scanning, and the `K_max = 2323` per-group limit.

### Authority and Reproduction Evidence

- The current `bitcoin/bips` master resolved to `9783d61f1b9c81231581fee026c8e8cb9499d265`. Its BIP352 document is version 1.1.1 dated 2026-04-16 and records both the intermediate-zero/final-nonzero vector and the 2323 per-group limit changes.
- The checked-in JSON and the authoritative raw BIP file were byte-identical. Both SHA-256 values were `f5f9ed4afd76a1b76f3c70b1cbe67532f89abbe559f8e02d7fc3d8ecb93af4a1`; both files were 418648 bytes and `cmp` returned 0.
- The local `tests_silentpayments_generate.py` matched the current `bitcoin-core/secp256k1` master copy byte-for-byte, with SHA-256 `3bebf03f30f7f7929c6a3d1b26bcd04860b15ffe0d35354104010aec744d5759`. Regenerating the header from the checked-in JSON produced a byte-identical `vectors.h`; both SHA-256 values were `8d88aead1f2f359aca31ac8c803001c55b1231be187839cafbe9c6959cedbbcc`.
- Parsing the JSON found 28 sending/receiving test cases. The current corpus includes the documented `Input keys intermediate sum is zero but final sum is non-zero` case as vector 27 and `Maximum per-group recipient limit K_max is exceeded (2324 matches)` as vector 28.
- The BIP reference implementation was run from a depth-one checkout of the authoritative BIP repository against its own vector file. It printed all 28 case names and ended with `All tests passed`.

### Local Consumer Verification

The current standalone libsecp256k1 LTO builds from the preceding backend matrix were reused because the source and generated files were unchanged between cycles. The portable and x86_64 assembly builds both ran:

```text
tests --target=silentpayments --iterations=2 --seed=1593520000000001 --jobs=2 --log=1
```

Both passed all seven silent-payment tests, including `run_silentpayments_test_vectors`:

```text
portable: Test run_silentpayments_test_vectors PASSED (11.215 sec), rc=0
assembly: Test run_silentpayments_test_vectors PASSED (11.113 sec), rc=0
```

The regenerated header and local consumer therefore agree on the current full corpus, while the independent BIP reference agrees with the same corpus and expected behavior.

### Review and History Search

The subtree history contains the current vector/generator/header update in `c26d4e2d6f03e26c6c20a4cc854f26d150f38b14` (`Update secp256k1 subtree to latest master`). The BIP changelog explains the recent vector and limit additions. Search of the local silent-payment module found the vector consumer, generator, K_max boundary tests, and tagged-hash tests; no stale alternate copy or unregistered vector source was found.

### Verdict

**Dismissed as current specification or vector drift; no confirmed finding.** The authoritative specification artifacts, local JSON, generator, generated header, portable and assembly consumers, and independent reference implementation all agree. No source, test, or generated-artifact change was justified.

### Limitations and Rejected Leads

- The local binaries were reused from the immediately preceding Cycle 158 standalone builds because no source or generated artifact changed; a new build was not needed to resolve this content question.
- This cycle did not provide ARM, 32-bit, big-endian, QEMU, GCC/full-LTO, sanitizer, Valgrind, timing, or PGO/BOLT evidence. Those limitations remain platform/backend campaign evidence, not a BIP352 vector mismatch.
- The BIP reference checkout created only Python bytecode caches outside the repository. No repository files were modified in the investigation.
- BIP328/MuSig2 values and the new BIP32 framework were surveyed and appeared current; they remain separate future specification cells rather than duplicate BIP352 work.

### Next Queue

1. Draw another eligible catalog goal after a fresh gate and record the exact selector output.
2. Keep this BIP352 cell closed unless the authoritative BIP or subtree changes.
3. Preserve the distinction between exact vector synchronization and broader implementation, platform, and constant-time audits.
