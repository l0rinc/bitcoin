# C/C++ Supply-Chain Cycle 41

## Identity and Gate

- Cycle: `41`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `59`
- Goal: `C/C++ supply-chain and security-gate audit`
- Slug: `cpp-supply-chain`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `f1215d5b3963797682e9910038bf674d3381a72c`
- `origin/master...HEAD` at the gate: `2 849`
- Tracked and staged state at the gate: clean. Existing untracked agent artifacts and `test/cache/` were preserved.
- `goals.tsv` validation: `validated_rows=99 total_lines=100`
- No relevant test, fuzz, sanitizer, daemon, or profiling process was running at the gate.

## Scope and Inventory

- `depends/funcs.mk` downloads package archives to temporary files, writes a declared SHA-256 sidecar, verifies it with `sha256sum -c`, and only then renames the archive into the cache. The default package metadata and special Qt fetches have SHA-256 declarations, and observed package URLs use HTTPS.
- `depends/packages/*.mk` includes a local-directory hash for the native libmultiprocess source, while the regular vendored subtree is checked by `test/lint/git-subtree-check.sh`.
- Vendored/subtree components include LevelDB, crc32c, UniValue, libsecp256k1, minisketch, and libmultiprocess. Subtree history and optional remote commit checks were reviewed; no missing subtree provenance was confirmed.
- Guix manifests pin source URLs, git commits, and hashes. `contrib/guix/guix-verify` requires valid signatures for manifest verification and compares signed manifest diffs.
- The release binary verifier in `contrib/verify-binaries/verify.py` is the high-impact security gate selected for detailed verification. Its docs promise a minimum threshold of signatures from trusted public keys.

## Hypothesis

`verify_shasums_signature` counts every cryptographically good signature toward `--min-good-sigs`, even when the signer is not trusted. An attacker able to add valid signatures to the checksum signature file could therefore meet a threshold intended to require trusted release signers.

## Contract and History Review

- `contrib/verify-binaries/verify.py:12-24` documents the threshold as signatures from public keys we trust, with GPG trust and explicit `--trusted-keys` support.
- `contrib/verify-binaries/README.md` describes the user-selected trusted keys and the minimum number of good signatures.
- Historical commit `37c9fb7a59a3179b90ed1deaebaabb539976504b` introduced multisignature verification specifically to require a threshold of trusted signatures and distinguished good-but-untrusted signatures. Current blame places the erroneous count in that feature's implementation; no later correction was found.
- The surrounding code already partitions signatures into `good_trusted` and `good_untrusted`; adding the untrusted list back into the threshold count contradicts both the documented contract and the feature's stated rationale.

## Independent Verification

Static contract/history evidence was paired with a deterministic parser-level execution probe. Three synthetic GPG status records were marked `TRUST_UNDEFINED`, while the threshold was three and no explicit trusted keys were supplied. Before the patch:

```text
[INFO] got 3 good signatures
status=0 trusted=0 untrusted=3
```

The function returned success despite having zero trusted signatures. This probe replaces only the GPG subprocess and exercises the production status parser and threshold decision; it does not claim that synthetic status records are a complete GPG integration test.

## Fix

Count only `good_trusted` signatures and make the diagnostic name the counted quantity. Add a deterministic unit control to `contrib/verify-binaries/test.py` that uses a temporary module load and a mocked GPG status response; the control requires `NOT_ENOUGH_GOOD_SIGS` for three valid but untrusted signatures.

## Validation

- The pre-fix synthetic probe reproduced success with `trusted=0` and `untrusted=3`.
- The post-fix threshold unit control passed with return code `NOT_ENOUGH_GOOD_SIGS`; the failure path correctly returned no verified signature lists.
- Python syntax compilation for both verifier files passed.
- The full release verification integration script was not run because it downloads historical release artifacts and depends on external network/keyserver state; the deterministic control is independent of that environment.
- The duplicate-signer threshold behavior was separately reproduced before this patch (`AAAA1111` repeated three times returned success) and remains a queued distinct hypothesis for the next finding.

## First Verdict

**Confirmed and fixed.** The old release security gate accepted a threshold met entirely by valid but untrusted signatures. The minimal fix makes the threshold match its documented trusted-key contract and preserves the existing reporting of untrusted signatures.

## Follow-up Hypothesis: Duplicate Trusted Signers

The same threshold must count distinct trusted signer keys, not repeated valid signatures from one key. The README describes a threshold over public keys, and the original multisignature commit used a set for `good_trusted`; refactor `17575c0efa` changed that collection to a list to return it from the helper but retained a signature-count threshold. The current `SigData` objects are identity-distinct, so three repeated status records for `AAAA1111` were counted as three.

The pre-fix parser-level execution probe used three `GOODSIG AAAA1111` records with `TRUST_FULLY` and `--min-good-sigs 3`. It returned success and logged three trusted signatures even though only one signer key was present. This is a separate failure mode from counting untrusted signatures.

Fix the threshold count to `len({sig.key for sig in good_trusted})` while retaining the full trusted signature list for reporting and JSON output. Add a separate deterministic regression control requiring `NOT_ENOUGH_GOOD_SIGS` for three repeated signatures from one trusted key.

## Follow-up Validation

- The duplicate-signer regression control passes after the distinct-key fix; the same status sequence now returns `NOT_ENOUGH_GOOD_SIGS`.
- The untrusted-signer regression continues to pass after the follow-up fix.
- Python syntax compilation for both verifier files and `git diff --check` pass.
- The full release verification integration script remains unrun because it downloads historical release artifacts and depends on external artifacts and keyserver state.

## Overall Verdict

**Confirmed and fixed.** The release security gate now requires the configured number of distinct trusted signer keys and rejects thresholds met by untrusted signatures or duplicate packets from one key.

## Limitations and Next Queue

1. Run the release verifier integration tests when network and keyserver state are available, including JSON output and explicit trusted-key modes.
2. Continue the supply-chain inventory across compiler/tool downloads, generated inputs, license gates, and release signing manifests without repeating these threshold findings.
