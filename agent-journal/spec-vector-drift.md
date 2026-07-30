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
