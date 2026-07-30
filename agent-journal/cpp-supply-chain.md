# C/C++ Supply-Chain Cycle 119

## Cycle 119 Start: Dependency, Tool, and Build-Gate Audit

### Identity and Gate

- Cycle: `119`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `59`
- Goal: `C/C++ supply-chain and security-gate audit`
- Slug: `cpp-supply-chain`
- Branch: `uber-cycle-119-cpp-supply-chain-20260730`
- Start HEAD: `7c2204b3fcab8e7e359c137338bad4fb479274f6`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- `origin/master...HEAD` at the gate: `40 1027`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Tracked and staged state at the gate: clean; persistent untracked artifacts and `test/cache/` were preserved.
- PID `777094` (`test_bitcoin --run_test=wallet_tests`) and parent PID `725042` were observed and will not be touched.

### Closed Cells and New Scope

The Cycle 41/63 trusted-signature threshold, duplicate-signer, and Guix cached-archive provenance findings are closed and must not be rediscovered. This cycle audits dependency/tool downloads, CI action and container references, generated build inputs, source/license gates, hash/signature binding, and whether a security check runs before untrusted bytes can affect compilation or release artifacts.

### Initial Hypotheses

1. A dependency or compiler/tool fetch may have a mutable URL/cache identity, weak or absent digest/signature check, or a verification step that occurs after extraction or use.
2. A CI action, container, or bootstrap tool may be referenced by a moving tag or mutable image without a repository-local integrity or review gate.
3. A generated input, vendored file, or license/provenance check may omit an optional or platform-specific source that still reaches a build or release artifact.
4. A check may bind metadata to a path or archive name rather than to the exact bytes and version consumed; distinguish real exposure from an intentional authenticated boundary.

### Protocol

For each candidate, inspect implementation, callers, history, docs, CI, and the relevant upstream contract; reproduce with scratch caches and deterministic local substitutes; and lock confirmed, dismissed, or inconclusive before changing code. Keep the prior signature/archive findings excluded, preserve raw commands and hashes, and require a pre-use failure or an independently verifiable artifact mismatch before committing a fix.

## Cycle 119 Finding: Mutable External CI Action References

### Applicability and Trust Boundary

- Before the fix, the active root workflow and composite actions referenced 20 external actions by moving major tags: `WarpBuilds/cache/{restore,save}@v1`, `actions/checkout@v6`, `actions/cache/{restore,save}@v5`, `actions/upload-artifact@v7`, `actions/download-artifact@v8`, `actions/github-script@v8`, and `docker/setup-buildx-action@v4`.
- The workflow selects `provider: warp` for `github.repository == 'bitcoin/bitcoin'` in the cross-build and matrix jobs. The selected third-party cache action therefore executes on the project's trusted default-repository runners, not only on an isolated developer machine.
- `configure-docker` runs `actions/github-script` before the container build and exports every environment variable whose name starts with `ACTIONS_` into `GITHUB_ENV`, including the cache service variables named in the adjacent comment. The cache action's checked-out `restore/action.yml` and `save/action.yml` are Node 24 actions with checked-in `dist` entry points, so a tag change changes executable workflow code without a Bitcoin repository diff.
- The `WarpBuilds/cache` `v1` ref resolves via `git ls-remote` to `40f3443ae7b70e568d6e2070ea897f3df94d7553`; the shallow clone reports that ref as a commit and `show-ref` reports a lightweight tag, not a signed tag object. The official action refs similarly resolved to `actions/checkout` `d23441a48e516b6c34aea4fa41551a30e30af803`, `actions/cache` `caa296126883cff596d87d8935842f9db880ef25`, `actions/upload-artifact` `043fb46d1a93c77aae656e7c1c64a875d1fc6a0a`, `actions/download-artifact` `3e5f45b2cfb9172054b4087a40e8e0b5a5461e7c`, `actions/github-script` `ed597411d8f924073f98dfc5c65a23a2325f34cd`, and `docker/setup-buildx-action` `bb05f3f5519dd87d3ba754cc423b652a5edd6d2c`.

### Independent Verification

- A pre-change static inventory found all 20 external refs in `.github/workflows/ci.yml` and `.github/actions/**/action.yml` used tags rather than immutable object IDs.
- The action metadata and workflow conditions independently established that the third-party action is on a trusted build path and receives the cache integration context. The reproducible failure mode is ref substitution: moving or compromised tag contents would be executed by CI while the Bitcoin tree remained unchanged; this is a provenance/control failure even without simulating an external account compromise.
- A post-change Python scanner enumerated the same 20 active external refs, asserted each matched `[0-9a-f]{40}`, and reported `all_external_refs_are_full_sha=yes`. `git diff --check` passed.

### Fix

Replace every active root-workflow external tag with the exact commit resolved from that tag, including both Warp cache entry points, both GitHub cache entry points, checkout, artifact transfer, GitHub Script, and Buildx. Local composite-action references remain local. This keeps behavior at the audited tag while preventing silent tag movement from changing CI code.

### Validation and Limits

- The exact tag resolutions were captured before editing with `git ls-remote`; the Warp action was also cloned at `v1` and its checked-out commit matched the pinned SHA.
- The static full-SHA scanner and `git diff --check` passed. `actionlint` and Ruby YAML parsing are unavailable in this environment, so a live GitHub Actions run was not attempted. No functional source build was needed for this workflow-only change.
- Nested workflows vendored under `src/` are not active root workflows and were not changed. CI package/bootstrap downloads, SDK archives, and generated-input gates remain distinct queued cells; the Cycle 41/63 signature/archive findings remain excluded.

### Verdict

**Confirmed and fixed.** The active CI trust boundary depended on mutable external action tags, including a third-party cache action on trusted runners. Full-SHA pinning removes the unreviewed ref-movement path; the exact action versions and validation limitations are recorded for future bump review.

## C/C++ Supply-Chain Cycle 41

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

## Cycle 63 Reopen: Toolchain, Dependency, and Provenance Gates

### Identity and Gate

- Cycle: `63`
- Draw command: `shuf -i 0-98 -n 1`
- Draw: `59`
- Goal: `C/C++ supply-chain and security-gate audit`
- Slug: `cpp-supply-chain`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `70caa034b9aa598e10bfd5091f2e6358f843a610`
- `origin/master...HEAD` at the gate: `2 896`
- Tracked/staged state at the gate: clean. Existing untracked agent-goal files, journals, probes, and `test/cache/` were preserved.
- `goals.tsv` validation: `validated_rows=99 total_lines=100 status=ok`
- No relevant test, fuzz, sanitizer, daemon, or profiling process was running at the gate.

### Closed Surface and Scope

The cycle-41 verify-binaries findings are closed: trusted-signature thresholds and duplicate trusted signer packets must not be rediscovered. This reopen covers compiler and build-tool downloads, package-cache reuse, source/archive provenance, generated inputs, license gates, and whether a security-relevant check is performed before the artifact it protects can influence a build or release.

### Hypotheses

1. A download/cache path may trust an existing artifact or generated input without revalidating its hash, URL identity, or provenance at the point of use.
2. A compiler/tool bootstrap path may accept a mutable or unpinned input, or may record metadata without binding it to the bytes actually consumed.
3. A license or vendored-source gate may cover only a subset of generated/optional files, creating a false assurance rather than a defect.
4. A candidate may be intentional because the authenticated boundary is enforced by Guix, depends, CI, or an upstream package manager outside this repository; distinguish that from an omitted local check.

### Evidence Log

- `contrib/guix/guix-build:412-442` maps the writable `OUTDIR_BASE` into the isolated build container and sets `DIST_ARCHIVE_BASE=/outdir-base/dist-archive`. `contrib/guix/libexec/setup.sh:102-108` previously created `${DISTNAME}.tar.gz` only when absent. Every platform build then extracted that path directly (`build_linux.sh:72-73`, with equivalent macOS/Windows consumers).
- The source archive's digest is recorded only after extraction/build by `contrib/guix/libexec/package.sh:133-143`; that output attestation does not prevent a bad archive from influencing the build that produces the attestation. The historical rationale in `d522d8006b` confirms the archive is treated as a build input, so the check must precede extraction.
- The cache key is a short/tag-derived `DISTNAME` (`contrib/guix/libexec/prelude.bash:116-117`), and `OUTDIR_BASE` may be kept outside the worktree for reuse. An existing archive therefore crosses a cache boundary without a pre-use comparison to the checked-out tree.
- Controlled baseline replay at gate `70caa034b9aa598e10bfd5091f2e6358f843a610`: seeded `/data/my_storage/tmp/cpp-supply-chain-archive-probe-1785254185129255019/bitcoin-probe.tar.gz` from `54de023a7c9^`, then sourced the old `setup.sh` with the same `DISTNAME`. The script returned success and left the stale bytes in place:

  ```text
  old_hash=152c5861294c9b301eeceb03a5a9afa70b12c4412a8bb88e7d84ceab4562f356
  kept_hash=152c5861294c9b301eeceb03a5a9afa70b12c4412a8bb88e7d84ceab4562f356
  expected_hash=339e8f43a28b940e41dd5cc6add96859bcc3b015ff4f403e5e9f7f3d751a1425
  archive_reused=yes
  current_setup_present=1
  reused_setup_present=0
  source_sha_mismatch=yes
  ```
- The stale archive came from a real Git commit and was accepted without any malformed-tar or extraction failure, so this is a provenance mismatch rather than a parser robustness issue. The same unchecked reuse existed for detached signature archives in `contrib/guix/libexec/codesign.sh:55-61`.

### Verdict

**Confirmed and fixed.** An existing Guix source or detached-signature archive was trusted solely because its cache path existed; the build extracted it before any comparison with the repository that was supposed to supply it. The shared verifier now compares the cached bytes with a fresh `git archive` digest and fails closed on mismatch.

### Fix

- Added `contrib/guix/libexec/archive.sh:create_or_verify_git_archive`, which creates missing archives and compares existing archives against `git archive --format=tar.gz --prefix=... HEAD` before returning success.
- `setup.sh` uses the verifier for the Bitcoin source archive; `codesign.sh` uses it for the detached-signature archive. The check covers both build-input paths without changing archive contents or output naming.
- Commit: `2ac7b583af3a8935962d75b8125fd1c67675268b` (`guix: verify cached Git archives before extraction`), authored as `Lőrinc <pap.lorinc@gmail.com>`.

### Validation

- The post-fix source replay seeded the cache from `HEAD^`; `setup.sh` returned status `1` with `does not match HEAD`. A cache generated from current `HEAD` returned status `0`. Appending one byte to that archive returned status `1` with the same mismatch diagnostic.
- The detached-signature helper accepted an archive generated from current `HEAD` and rejected one generated from `HEAD^` with status `1`.
- `bash -n contrib/guix/libexec/archive.sh contrib/guix/libexec/setup.sh contrib/guix/libexec/codesign.sh`, `git diff --check`, and `python3 test/lint/lint-shell.py` passed; the shell lint command explicitly skipped because `shellcheck` is not installed.
- `guix` is unavailable, so a full containerized Guix build and external substitute verification were not run. The regression invokes the production archive setup/helper path without requiring the Guix daemon. No relevant process remains running.

### Handoff

- Continue the full catalog loop. Keep the fixed archive-integrity cell closed; the next `cpp-supply-chain` draw, if repeated, must select a distinct dependency/tool pin, generated-input, license, or substitute-provenance surface. Preserve the Guix-unavailable limitation and the exact post-fix probe directory `/data/my_storage/tmp/cpp-supply-chain-postcommit-probe-1785254451337294661/`.
