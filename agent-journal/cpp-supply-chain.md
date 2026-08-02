# C/C++ Supply-Chain Cycle 290

## Cycle 290 Completion: pin CI container image manifests

### Identity and Gate

- The fresh gate fetched `origin/master` before branch creation. The exact
  selector `shuf -i 0-98 -n 1` returned `59`; no reroll was needed because
  Goal 59 retained distinct vcpkg, container, generated-input, and license
  cells. Goal: `C/C++ supply-chain and security-gate audit`; slug:
  `cpp-supply-chain`.
- Branch: `uber-cycle-290-cpp-supply-chain-20260802`. Gate and cycle-start HEAD
  were `150dbb5ae37a80035f321dd0536d355eda4f02ac`; fetched `origin/master` was
  `556988790a7f961693a8fd93f73725baea66476a`; merge-base was
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `45 1370`
  (`origin/master...HEAD`); and the entry state SHA-256 was
  `6bf328a509a95aa68d4081ca35f7fe95f04eda90a0da9e21accba2a855aa50e7`.
- Catalog, random prompt, goals TSV, and protocol hashes were unchanged:
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`,
  and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Tracked and index state were clean at the gate. Existing untracked agent
  artifacts were preserved. Protected long-running PIDs
  `777094`, `956381`, `1138182`, `1157959`, `1312049`, `1312050`, and
  `1346200` were observed alive and left untouched.

### Closed Scope and Hypothesis

Cycles 119, 154, 207, 229, 239, 264, 265, and 279 already closed mutable
GitHub Action refs, executable lint assets, script vectors, Guix archives,
release signatures, compiler/toolchain refs, SDK archives, qa-assets,
`pyzmq`, and `pycapnp`. This cycle selected the distinct container-image
provenance cell. The active root CI environment files supplied 24 mutable
`CI_IMAGE_NAME_TAG` values: Debian `trixie`, Ubuntu `22.04`/`24.04`/`26.04`,
Alpine `3.24`, and the untagged Chimera image. `ci/lint_imagefile` had a
mutable Ubuntu base plus mutable `uv:0.11` and `ruff:0.15` build stages.
The vendored minisketch and libsecp256k1 CI Dockerfiles used mutable
`debian:stable` and `debian:stable-slim` bases.

These images select compilers, linkers, package managers, native libraries,
and lint tooling before C/C++ code is built. A registry tag can move without a
Bitcoin commit, so a successful CI run or release-adjacent build could execute
different native code from the one reviewed by the project. The dynamic
`FROM ${CI_IMAGE_NAME_TAG}` in `ci/test_imagefile` is intentional plumbing;
the security boundary is the values assigned by every maintained environment
file.

### Registry and Architecture Evidence

Manifest-list digests were resolved on 2026-08-02 with `skopeo inspect` and
registry `Docker-Content-Digest` headers. The selected immutable values are:

| Tag | Manifest-list digest |
| --- | --- |
| `mirror.gcr.io/debian:trixie` | `sha256:fac46bff2e02f51425b6e33b0e1169f55dfb053d83511ca28aa50c09fd5ed7a4` |
| `mirror.gcr.io/alpine:3.24` | `sha256:28bd5fe8b56d1bd048e5babf5b10710ebe0bae67db86916198a6eec434943f8b` |
| `mirror.gcr.io/chimeralinux/chimera:latest` | `sha256:29102d7e12a1f464707d7aba19ce53e652d277861838ed4129178d0655444b1a` |
| `mirror.gcr.io/ubuntu:22.04` | `sha256:0e0a0fc6d18feda9db1590da249ac93e8d5abfea8f4c3c0c849ce512b5ef8982` |
| `mirror.gcr.io/ubuntu:24.04` | `sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90` |
| `mirror.gcr.io/ubuntu:26.04` | `sha256:3131b4cc82a783df6c9df078f86e01819a13594b865c2cad47bd1bca2b7063bb` |
| `ghcr.io/astral-sh/uv:0.11` | `sha256:77280f2f771df71f90786c314fe1bbc1e023feac652969bbf139c280babf2eb7` |
| `ghcr.io/astral-sh/ruff:0.15` | `sha256:d9e5710276b88ae2c0a33bd394aeb2877f5f852e23235ebcaa1c5bda4998569a` |
| `docker.io/library/debian:stable` | `sha256:8e109a974a9659354791cab2c001e5e3c3153805c344ccec7c1ef98d814187e7` |
| `docker.io/library/debian:stable-slim` | `sha256:328d16499860ae6cb9b345e2e4cebca08c2a36e4f7278482c7bd1f39d71e5bfd` |

The raw registry responses were OCI image indexes, not single-architecture
child manifests. Debian, Ubuntu, and Alpine indexes include the existing
amd64/arm64 and other CI architectures; Chimera includes amd64, arm64,
ppc64le, and riscv64; the uv and ruff indexes include amd64 and arm64. The
pins therefore preserve platform selection while fixing the selected bytes.

### Independent Controls and Fix

The old-tree inventory reported 24 CI assignments with 0 digest pins and five
concrete Dockerfile base/stage references with 0 digest pins. After the
change, the same scanners report 24/24 CI assignments and 5/5 concrete
Dockerfile references digest-pinned. A repository search finds no remaining
concrete unpinned `FROM`, `COPY --from`, or `CI_IMAGE_NAME_TAG` reference in
the active root, lint, minisketch, or libsecp256k1 paths; only the intentional
variable/argument plumbing remains.

Replaced the mutable tags with `tag@sha256:<manifest-list-digest>` in all 24
root CI environment assignments, the Ubuntu/uv/ruff lint image, and the two
vendored project Dockerfiles. Each digest-only reference was resolved again
through its registry after the edit and returned the exact requested digest.
`bash -n ci/test/00_setup_env_*.sh`, Python compilation of
`ci/test/02_run_container.py`, and `git diff --check` passed.

### Verification and Verdict

**Confirmed and fixed.** Before the change, CI and vendored Docker builds
trusted mutable registry tags for C/C++ toolchain and native-dependency
selection. The source now binds those image inputs to reviewed manifest-list
digests while retaining the original release-family tags for update review.

No Docker daemon/buildx, Windows runner, or full CI matrix was available in
this environment, so no image build was executed locally. `skopeo` validated
the digest-only image references and the registry headers bound each original
tag to the recorded digest; its installed parser does not accept the standard
combined `tag@digest` spelling, so combined-reference parsing was not claimed
as a local `skopeo` test. The pins do not provide image signatures, SBOM
verification, or automatic security updates; a future refresh must deliberately
replace the recorded digest after reviewing the new image.

### Handoff

Keep vcpkg tool/manifest/cache provenance, hosted-runner labels, generated
inputs, and license gates as separate Goal 59 cells. Do not reopen the closed
action, lint-tool, script-vector, Guix-archive, release-signature,
compiler-ref, SDK-archive, qa-assets, pyzmq, or pycapnp findings without new
evidence.

# C/C++ Supply-Chain Cycle 279

## Cycle 279 Completion: pin CI pycapnp installations

### Identity and Gate

- The fresh gate fetched `origin/master` before branch creation. The exact selector `shuf -i 0-98 -n 1` returned `59`; no reroll was needed because the Goal 59 journal retained distinct open cells. Goal: `C/C++ supply-chain and security-gate audit`; slug: `cpp-supply-chain`.
- Branch: `uber-cycle-279-cpp-supply-chain-20260802`.
- Start HEAD: `1d351660070c67d4aa0367e43b992f6950449ef4`; `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `45 1348` (`origin/master...HEAD`).
- The tracked tree and index were clean at the gate, and `git diff --check` passed. Existing untracked agent artifacts, goal files, probes, `node_modules/`, `test/cache/`, and crash/profiling artifacts were preserved. Protected long-running tests were observed and not touched.
- Catalog, prompt, TSV, protocol, and pre-cycle state hashes were unchanged: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, and `386e5b3f8d7e5d0a01fd926b67d0cde37432df97aea1ab42c03475069362c519`.

### Closed Scope and Hypothesis

The previous Goal 59 cycle pinned `pyzmq`, leaving unpinned `pycapnp`, vcpkg manifest/cache inputs, container image provenance, generated-input checks, and license gates. Nine active `ci/test/00_setup_env_*.sh` files and the Ubuntu commit-test workflow installed `pycapnp` without a version. `test/README.md` also recommended a bare PyPI install and retained a source fallback at the older `v2.2.1` tag.

The trust boundary is CI and developer test preparation: `pycapnp` is a native extension and parser/RPC binding loaded by the IPC functional tests. An unbounded PyPI resolution lets a future wheel, bundled Cap'n Proto, Python-version compatibility change, or dependency behavior alter code executed by CI without a Bitcoin tree change. This cycle targets the package version and matching documentation only; full artifact hash locking and source-tag verification remain separate concerns.

### Provenance and Compatibility Evidence

- The PyPI JSON endpoint `https://pypi.org/pypi/pycapnp/2.2.4/json` identified the reviewed immutable release. The current project metadata also lists Python 3.9+ support; the local verifier used CPython 3.11. The release changelog includes fixes for malformed Text handling and an async capability-client memory leak.
- `pip download --no-cache-dir --no-deps --only-binary=:all: pycapnp==2.2.4` retrieved the CPython 3.11 manylinux wheel. Its SHA-256 was `7055771afef9fdfabedcabcde0c11938c016779cd512d93e3bc04d6ac6dd65d2`; it was installed into `/data/my_storage/tmp/cycle279-pycapnp/target` without modifying the host interpreter.
- Existing repository evidence had already exercised `pycapnp` 2.2.4 in `doc/fuzzing-findings.md`. The prior macOS pin history (`53b34c80c6`) established 2.2.1 compatibility, while the current IPC helper explicitly handles the 2.2.x Data-field behavior. The release tag used by the manual source fallback was advanced to `v2.2.4`.

### Independent Verification

- A repository-wide inventory of CI and test documentation found no remaining bare `pycapnp` package requirement in active install paths. Sourcing each changed environment file and inspecting `PIP_PACKAGES` showed `pycapnp==2.2.4`; `bash -n` passed for all changed shell files and `git diff --check` passed.
- The isolated 2.2.4 import loaded `echo`, `init`, `mining`, and `proxy` from the current Bitcoin IPC schemas and reported the expected mining schema ID. This checks the package and schema-import boundary independently of the text inventory.
- The IPC functional test ran against the existing IPC-enabled build with the isolated 2.2.4 module. Echo, mining, deprecated-interface failure, disconnect cancellation, and thread-busy tests all passed. The initial attempt against a build without the standalone `bitcoin` IPC executable was rejected by the harness before test execution and is not treated as a product result.
- Full container, Windows, sanitizer, and cross-architecture CI were unavailable. The wheel pin is a version constraint, not a complete per-platform `--require-hashes` lock; the Alpine/Chimera source-build paths still need their native environments for end-to-end validation.

### Fix and Verdict

Pinned `pycapnp==2.2.4` in all nine native CI environment bundles and the Ubuntu GitHub Actions install. Updated the IPC test README's direct install and source fallback to the same reviewed release. This keeps every maintained CI and documented installation path on one tested package version without changing IPC protocol code.

**Confirmed and fixed.** Before the change, active CI and documented installs accepted an arbitrary future `pycapnp` release. After the change, all audited package consumers select the reviewed 2.2.4 release, and the IPC functional suite passes with that package. The package pin does not authenticate every platform wheel or the GitHub source tag; a future cycle may evaluate hash-locked Python requirements or immutable source verification as a distinct supply-chain cell.

### Handoff

Keep vcpkg manifest/cache inputs, container image provenance, generated-input checks, and license gates as distinct Goal 59 cells. Do not reopen the closed qa-assets corpus, pyzmq, action-reference, lint-tool, script-vector, Guix-archive, release-signature, compiler-ref, or SDK-archive findings.

# C/C++ Supply-Chain Cycle 265

## Cycle 265 Completion: pin CI pyzmq installations

### Identity and Gate

- The fresh gate fetched `origin/master` before branch creation. The exact selector `shuf -i 0-98 -n 1` drew `59`; no reroll was needed because the Goal 59 journal still had distinct open cells. Goal: `C/C++ supply-chain and security-gate audit`; slug: `cpp-supply-chain`.
- Branch: `uber-cycle-265-cpp-supply-chain-pyzmq-20260802`.
- Start HEAD: `546eb9d130600f8674434b6441711580e402261f`; `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1319 45` (`HEAD...origin/master`).
- The tracked tree was clean at the gate. Existing untracked agent artifacts, goal files, probes, `node_modules/`, `test/cache/`, and crash/profiling artifacts were preserved. Protected long-running tests were observed and not touched.
- Catalog, prompt, TSV, and protocol hashes were unchanged: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

### Closed Scope and Hypothesis

The selected journal's open Goal 59 queue explicitly retained `pip install pyzmq`, vcpkg inputs/cache, container image provenance, generated inputs, and license gates. The previous qa-assets corpus cell was closed in Cycle 264. Four native CI environment files passed `pyzmq` without a version through `ci/test/01_base_install.sh`; `.github/ci-windows.py` and `.github/ci-windows-cross.py` also invoked `pip install pyzmq` without a version. The lint environment already used `pyzmq==27.1.0` in `ci/lint/requirements.txt`.

The trust boundary is CI preparation: pyzmq is a native Python extension used by the functional ZMQ tests and Python lint dependency checks. An unpinned install lets a future PyPI release, wheel selection, or dependency change alter code executed by CI without a Bitcoin commit, causing non-reproducible results or introducing behavior at the test/tool boundary. This cycle targets pyzmq only; the separately queued unpinned `pycapnp` paths remain open.

### Provenance and Compatibility Evidence

- PyPI JSON for `pyzmq` reports current version `27.1.0`, 92 release artifacts, and only the conditional PyPy dependency `cffi`; the project already selected this version for linting. The exact-release file metadata was read from `https://pypi.org/pypi/pyzmq/27.1.0/json`.
- A CPython 3.11 Linux wheel downloaded with `pip download --no-deps --only-binary=:all: pyzmq==27.1.0` had SHA-256 `5bbf8d3630bf96550b3be8e1fc0fea5cbdc8d5466c1192887bd94869da17a63e`, matching the PyPI digest. The relevant Windows, macOS, manylinux, and musllinux wheel hashes were also present in the same immutable release metadata.
- Installing that downloaded wheel into a scratch target under `/data/my_storage/tmp` imported `zmq` at version `27.1.0`, created and closed a `zmq.PAIR` socket, and terminated its context successfully. The host interpreter's unrelated installed pyzmq was `24.0.1`; no global package was modified.

### Independent Verification

- Sourcing each changed native environment file and inspecting the resulting `PIP_PACKAGES` value showed `pyzmq==27.1.0`; a repository-wide check found no remaining unpinned pyzmq install in the CI or README paths. The lint requirements pin remained unchanged.
- Python compilation of both Windows helpers and shell syntax checks for all four changed environment files passed. A source-level Windows command check found both `prepare_tests` paths use the exact `pyzmq==27.1.0` requirement.
- The old and new command shapes were compared against a controlled package-resolution model: the old bare `pyzmq` requirement leaves the resolver free to select a later release, while the new requirement can resolve only the reviewed version. No full CI job was run because the required Windows, container, and sanitizer environments are unavailable and protected tests must remain untouched.

### Fix and Verdict

Pinned `pyzmq==27.1.0` in the four native CI `PIP_PACKAGES` bundles, both Windows helper installs, and the macOS test README. The existing lint requirement already had the same pin. This is a small reproducibility/provenance fix and does not change the functional test protocol or install `pycapnp` differently.

**Confirmed and fixed.** Before the change, active CI preparation paths accepted an unbounded PyPI pyzmq release. After the change, all repository-maintained pyzmq install instructions and CI consumers name the same reviewed version. The pin does not provide `--require-hashes` for every platform-specific wheel; adding a complete hash lock is a separate scope decision, and the unpinned `pycapnp` dependency remains in the next queue.

### Handoff

Keep `pycapnp` installation, vcpkg manifest/cache inputs, container image provenance, generated-input checks, and license gates as distinct Goal 59 cells. Do not reopen the Cycle 264 qa-assets commit/cache finding or earlier action, lint-tool, script-vector, Guix-archive, release-signature, compiler-ref, or SDK-archive findings.

# C/C++ Supply-Chain Cycle 264

## Cycle 264 Completion: pinned qa-assets fuzz corpus provenance

### Identity and Gate

- Selector history: the first exact draw was `61`, but the selected stateful-contract-fuzzer evidence cell had just been closed in Cycle 262, so the protocol rerolled exactly once; the reroll was `59`, selecting `C/C++ supply-chain and security-gate audit` (`cpp-supply-chain`).
- Branch: `uber-cycle-264-cpp-supply-chain-security-gate-20260802`.
- Gate HEAD: `419fed9c60c7372270d925a3ae850c27712475b4`; `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `1317 45`.
- The tracked tree was clean at the gate. Existing untracked agent artifacts, goal files, probes, `node_modules/`, `test/cache/`, and crash/profiling artifacts were preserved. Protected long-running tests were observed and not touched.
- Catalog, prompt, TSV, and protocol hashes were unchanged: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

### Closed Scope and Hypothesis

Earlier Goal 59 cycles closed mutable GitHub Action refs, lint-tool assets, script test vectors, Guix cached archives, release-signature handling, compiler/toolchain Git refs, and cross-build SDK archives. The distinct open cell was the fuzz corpus. `ci/test/03_test_script.sh` cloned `https://github.com/bitcoin-core/qa-assets` from the moving default branch and skipped the clone whenever `${DIR_QA_ASSETS}/fuzz_corpora/` already existed. `.github/ci-windows.py` cloned the same moving branch and only printed its HEAD.

The trust boundary is the CI test preparation step. `test/fuzz/test_runner.py` feeds every corpus file to production fuzz binaries; the corpus is not executable source, but its contents control test coverage, runtime, memory use, and whether existing fuzz-target defects are exercised. A moving external ref also makes CI results non-reproducible, while the Unix cache condition allowed an arbitrary or stale pre-existing corpus directory to bypass the repository's intended source.

### Provenance Evidence

- `git ls-remote https://github.com/bitcoin-core/qa-assets refs/heads/main` resolved `main` to `918cdd36fec3c78f8b8f6a1dc0ec6688e7559c9e` on 2026-08-02. The commit is recorded as the pinned corpus version; updates must change the two consumer constants together.
- History shows the clone was introduced for fuzz seed corpora in `fa8a428c92d` and later moved to the current script path by CI refactors. The existing log line `Using qa-assets repo from commit ...` was observational only; it did not validate a commit or content identity.
- The Unix guard checks only the corpus subdirectory, not the Git repository or commit. Therefore a populated stale cache never invokes `git clone` or any provenance check. The Windows path has no corresponding cache guard, but its shallow clone follows the mutable default branch on every run.

### Independent Verification

- A local bare Git remote with commit A followed by commit B reproduced the stale-cache case. The old `if [ ! -d cache/fuzz_corpora ]; then git clone ...; fi` path returned success while using cached commit A after the remote moved to B. The post-change HEAD comparison rejected the same cache (`old_stale_cache_status=0`, `new_stale_cache_rejected=1`).
- The same remote reproduced the fresh-clone case: after `main` moved to B, a shallow clone resolved B. Comparing it with the expected A rejected the moving-ref result (`moving_head` was B and `new_moving_ref_rejected=1`).
- A Python harness imported `.github/ci-windows.py` and exercised `prepare_tests("fuzz")` with a matching fake Git HEAD and a mismatching fake Git HEAD. The first was accepted; the second exited with the expected commit mismatch before the log/test step.
- `bash -n ci/test/03_test_script.sh`, `python3 -m py_compile .github/ci-windows.py`, and `git diff --check` passed. The live `git ls-remote` check matched the pinned value. Full container, Windows, and fuzz runs were not executed because the required environments are unavailable and protected long-running tests must remain untouched.

### Fix and Verdict

Added the qa-assets commit `918cdd36fec3c78f8b8f6a1dc0ec6688e7559c9e` to the Unix and Windows fuzz-test preparation paths. The Unix path still reuses an existing corpus directory, but now resolves and compares its Git HEAD and fails closed for a missing repository, stale cache, or unexpected commit. A fresh clone is checked before the corpus is passed to `test_runner.py`. The Windows path resolves the shallow clone's HEAD and fails before the fuzz runner if it differs from the pinned commit.

**Confirmed and fixed.** Before the change, CI could consume moving or locally seeded fuzz inputs without a repository change or identity check. The change binds both active fuzz-corpus consumers to a reviewed Git object ID and makes stale or moved input fail closed. This does not attest the contents of an already-extracted or copied corpus outside these preparation paths, and it does not prove fuzz-target behavior safe; those remain separate evidence cells.

### Handoff

Keep `pip install pyzmq`, vcpkg manifest/cache inputs, container image provenance, generated-input checks, and license gates as distinct Goal 59 cells. Do not reopen the closed action, lint-tool, script-vector, Guix-archive, release-signature, compiler-ref, or SDK-archive findings.

# C/C++ Supply-Chain Cycle 239

## Cycle 239 Start: cross-build SDK archive integrity

### Identity and Gate

- Selector command: `shuf -i 0-98 -n 1` -> `59`; no reroll. Goal: `C/C++ supply-chain and security-gate audit`; slug: `cpp-supply-chain`.
- Branch: `uber-cycle-239-cpp-supply-chain-20260731`.
- Start HEAD: `0e295c72946700e3f06160e104061034bd27196f`; `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- The tracked tree was clean at the gate. Existing untracked agent artifacts, goal files, probes, `node_modules/`, `test/cache/`, and the crash artifact were preserved. Protected long-running test processes were observed and not touched.
- Catalog, prompt, TSV, and protocol hashes were unchanged: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

### Closed Scope and Hypothesis

Prior Goal 59 cycles closed mutable GitHub Action refs, lint-tool release assets, script test vectors, Guix cached Git archives, release signature thresholds, and compiler/toolchain Git refs. This cycle selected the distinct cross-build SDK archive cell. `ci/test/01_base_install.sh` downloaded or reused macOS, NetBSD, FreeBSD, and OpenBSD SDK archives by path and extracted them without checking their bytes. Docker image layers, host-side CI volumes, or a manually seeded `DEPENDS_DIR` could therefore supply stale or modified archives that reached the cross compiler and linker.

The hypothesis was that the missing pre-extraction digest check was a practical provenance failure, not merely a theoretical URL concern. The trust boundary is the CI image/base-install step: SDK headers, libraries, and startup objects influence the resulting cross-built binaries even though the SDK archives are outside the Bitcoin source tree.

### Artifact Provenance

- The current macOS archive `Xcode-26.1.1-17B100-extracted-SDK-with-libcxx-headers.tar` from the configured `bitcoincore.org` SDK source was 204,748,800 bytes with SHA-256 `9600fa93644df674ee916b5e2c8a6ba8dacf631996a65dc922d003b98b5ea3b1`.
- NetBSD `11.0_RC6` `base.tar.xz` and `comp.tar.xz` were independently matched to the official `SHA512` manifest (`0323dbb284913cae27f1e67a41c395dc6c7a4160dc65e79866ca8a69e12bad5fd782cc3f5da81a9d718020a857ccbc764e007f6b2696d88d7b9df83baa86f45b` and `a33f9cb6886713a2cc9d3e7bfbecbcb77b82d86cd43fa688531e06b389ad1820f1f63e501a37f899aee700270618b0865aacbf569b65a91fdb77ddd48f23d940`) and recorded with SHA-256 `b1f0ef68d15a0c199d1095139926d0df02f162889d4ed9c8187e757df29422ca` and `d1df700590f12e10d2e85fb43371abae88b3d4d2c2311f3ac6a46e1ae4423f99`.
- FreeBSD `15.1-RELEASE/base.txz` matched the official release `MANIFEST`, SHA-256 `3768988b151c20f965679062b065c63a977d6bbb9f47fd83695ec2c40790c18f`.
- OpenBSD `7.9` `SHA256`/`SHA256.sig` lists `base79.tgz` as `923d2e03f06408d50d4848334398c6d04b5514dcac7917badfc178a0eef248de` and `comp79.tgz` as `21a67af20aebcabf85b09f4206fc95b4cae0a35d42b154b976f0159f457724f9`. The values come from the release checksum manifest rather than an uncontrolled bulk download.

### Independent Verification

A scratch directory seeded a valid but wrong tar archive at the exact macOS SDK cache path. Running the pre-change script from `HEAD` returned `before_status=0` and extracted a marker (`before_extracted=0`); the archive was accepted solely because it existed. Running the patched production script against the same shape returned `post_status=1`, `post_extracted=1`, and reported the actual digest versus the expected pinned digest before invoking `tar`.

A second harness supplied the downloaded, correctly hashed macOS, NetBSD, and FreeBSD archives through the real production path while replacing only `tar` with a no-op that created its destination. All four verification calls completed and the script returned `valid_status=0`. `bash -n` for the changed shell files and `git diff --check` passed. Docker, ShellCheck, and complete cross-build jobs are unavailable or too resource-heavy in this environment, so no full image build or target binary build was claimed.

### Fix and Verdict

Added `verify_sha256` to `ci/test/01_base_install.sh`. It runs after download-or-cache lookup and before extraction, so a restored archive must match the version-specific digest just like a fresh download. Added fixed SHA-256 values beside each supported macOS, NetBSD, FreeBSD, and OpenBSD SDK version; a missing or wrong value fails closed. The change covers both fresh downloads and existing archive files without changing archive URLs or extraction layout.

**Confirmed and fixed.** Before the change, a modified SDK archive could be supplied through the CI/base-install cache boundary and influence a cross-build without a repository diff or local integrity failure. The check is intentionally limited to archives entering the extraction branch; it does not attest an already-existing extracted SDK directory when no archive is extracted. SDK-version bumps must update the version and digest together.

### Handoff

Keep `pip install pyzmq`, vcpkg manifest/cache inputs, mutable `qa-assets` fuzz-corpus cloning, container image provenance, generated-input checks, and license gates as distinct next cells. Do not reopen the closed action, lint-tool, test-vector, Guix-archive, release-signature, or compiler-Git-ref findings.

# C/C++ Supply-Chain Cycle 229

## Cycle 229 Start: mutable CI toolchain source refs

### Identity and Gate

- Selector draw: `shuf -i 0-98 -n 1` -> `59`; no reroll. Goal: `C/C++ supply-chain and security-gate audit`; slug: `cpp-supply-chain`.
- Branch: `uber-cycle-229-cpp-supply-chain-20260731`.
- Start HEAD: `d3e548fadf59a80e098270d7c12031d7537a46ee`; `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `HEAD...origin/master`: `1241 42`.
- Catalog, prompt, TSV, and protocol hashes were unchanged: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- The tracked source tree was clean at the gate; pre-existing untracked agent artifacts, goal files, probes, `node_modules/`, `test/cache/`, and the crash artifact were preserved. Protected PIDs `777094`, `956381`, `1138182`, and `1157959` remained alive and were not touched.

### Closed Scope and Hypothesis

Previous Goal 59 cycles closed mutable GitHub Action refs, lint-tool assets, script test-vector downloads, Guix cached archives, and release signature threshold behavior. This cycle selected the queued compiler/toolchain source-ref cell. `ci/test/01_base_install.sh` cloned LLVM, RISC-V GNU toolchain, and IWYU by named Git refs, then configured or built the checked-out source. The refs were not bound to the commit that CI was expected to consume.

The trust boundary is the CI image build: LLVM libc++ sources are configured by CMake, RISC-V sources run `configure` and `make`, and IWYU sources are patched, configured, and installed. A moved tag or branch can therefore change build scripts, compiler/runtime sources, or the installed analysis tool without a Bitcoin repository diff.

### Provenance Evidence

- `llvmorg-22.1.7` resolves to annotated tag object `7979ad438a4904e5ff57dc85e962992242f81688`, whose peeled commit is `a255c1ed36a1d06f79bd2633ba9f8d900153007c`.
- RISC-V `2026.06.06` resolves to commit `81bb1f89664aad156df3d2773195177c92dedc3a`.
- IWYU `clang_22` resolves to branch commit `01a091d16b3dedb808db21f32ed3e761737a3691`.
- History confirms these are mutable ref choices rather than immutable content bindings: `324caa8497` deliberately changed LLVM selection to a release tag, `c43b7a1115` added the NetBSD-style CI bootstrap pattern, `d64ea15824` added OpenBSD bootstrap, and `619dc8a1f9` temporarily used a fixed IWYU branch before the current upstream branch was restored.

### Independent Verification

`/data/my_storage/tmp/cycle229_gitref_probe.sh` created a local repository, recorded commit A, moved a `release` tag to commit B, and ran both command shapes. The old production shape cloned B and reached a simulated build. The new helper accepted an unchanged `stable` ref at A, rejected the moved `release` ref, and did not reach the simulated build:

```text
stable_clone_commit=993df4cbdd5fcdd436c4a2bcf0f8197abf419b8c
moved_commit=a12619a5f00f0bf265591627913a046f521bf8f1
old_clone_commit=a12619a5f00f0bf265591627913a046f521bf8f1
old_build_reached=yes
post_fix_rejected=1
post_fix_build_reached=0
```

The live `git ls-remote` checks matched all three pinned commits. `bash -n ci/test/01_base_install.sh`, `git diff --check`, and a no-op run of the production install script with optional branches disabled passed. Docker, ShellCheck, and actionlint are unavailable, so no containerized base-image build or full cross-toolchain build was run.

### Fix and Verdict

`ci/test/01_base_install.sh` now uses `clone_and_verify_commit` for all three toolchain clones. It retains the existing ref for version selection, checks `git -C <destination> rev-parse HEAD` against the repository-local expected commit, and returns nonzero before any fetched source reaches CMake, `configure`, patching, or `make`.

**Confirmed and fixed.** Before the change, mutable CI toolchain refs could silently change trusted build inputs. The current worktree binds the bytes by Git object ID and fails closed on ref movement. This closes only the mutable Git source-ref finding. SDK archives and their cache reuse, PyPI installs, apt LLVM key/repository setup, vcpkg inputs, mutable `qa-assets` fuzz-corpus cloning, generated inputs, and license gates remain distinct queue cells.


## Cycle 207 Start: mutable Windows CI test asset

### Identity and Gate

- Selector draw: first draw `21` was rejected because Cycle 206 had just closed that exact cell; exact reroll draw `59` selected `cpp-supply-chain`.
- Branch: `uber-cycle-207-cpp-supply-chain-20260731`.
- Gate timestamps: `2026-07-31T11:41:39Z` and branch gate `2026-07-31T11:41:58Z`.
- Start HEAD: `903650fe3e15045327f9a32b857e5a931d6dffdb`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Divergence at gate: `1204` ahead, `42` behind.
- Pre-cycle state SHA: `d74fda9ddb35f70b9dbc0ec571d9f31f4a148d8243d5186fd670d4bcffa7f7ee`.
- Catalog SHA: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Prompt SHA: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- TSV SHA: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Protocol SHA: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Pre-existing untracked agent artifacts, catalog files, `node_modules/`, and `test/cache/` were preserved. Protected long-running test processes were observed and not touched.

Prior closed cells excluded from this cycle: immutable GitHub Action refs, pinned lint-tool downloads, Guix cache/archive verification, and release-binary signature-threshold handling. Compiler/SDK downloads, generated inputs, license gates, release manifests, and other `qa-assets` paths remain separate queue cells.

### Scope and Evidence

`test/get_previous_releases.py` downloads release archives from `bitcoincore.org` but validates each archive against the in-tree `SHA256_SUMS` allowlist before extraction. The distinct gap was `test/download_utils.py::download_script_assets`, which fetched `https://github.com/bitcoin-core/qa-assets/raw/main/unit_test_data/script_assets_test.json` and accepted it after HTTP/status and content-length checks only. `.github/ci-windows.py` and `.github/ci-windows-cross.py` call it before unit/functional tests; `ci/test/03_test_script.sh` had a duplicate mutable fetch for container CI.

`src/test/script_assets_tests.cpp:196-222` parses the file as a JSON array and invokes `VerifyScript` for every case. It requires only `tests.size() > 0`, so a replacement can silently remove regression coverage or add pathological work while still producing a nominally passing test. The asset is data, not an executable script, but it is a trusted test oracle on the native test process.

History and upstream provenance support a stable pin: the asset path history ends at `b33d85102d169b54d966ea315ad81a636680aefa` (2022-10-24, `Add case with invalid internal pubkey`), while `qa-assets/main` resolved to `918cdd36fec3c78f8b8f6a1dc0ec6688e7559c9e` on 2026-07-31. The blob at the last-change commit and at current `main` is identical: 9,243,520 bytes, SHA-256 `cd789a58ec45916e1721cdd14e82ca4c93100959f1cef4e229b22e3bf539f095`.

### Independent Verification

- Baseline scratch harness monkeypatched `download_from_url` to write `[{"tampered": true}]`; the old helper returned successfully and accepted the bytes.
- Post-fix scratch harness used the same substitution, disabled the retry sleep, and observed checksum rejection on both attempts: `post-fix tamper test: rejected after two verified attempts`.
- An actual download from the pinned raw URL completed and produced the expected digest `cd789a58ec45916e1721cdd14e82ca4c93100959f1cef4e229b22e3bf539f095`.
- `DIR_UNIT_TEST_DATA=<scratch> /data/my_storage/tmp/cycle203-heldout-35819-build/bin/test_bitcoin --run_test=script_assets_tests --catch_system_error=no --log_level=test_suite -- -printtoconsole=1` ran the `script_assets_test` case over the downloaded vectors in 2.326 seconds and ended with `*** No errors detected`.
- `python3` compilation via `compile(...)`, `bash -n ci/test/03_test_script.sh`, and `git diff --check` passed. ShellCheck is unavailable. Full Windows/container CI was not run.

### Fix and Verdict

`test/download_utils.py` now pins the URL to the immutable last-change commit and verifies the expected SHA-256 after every download attempt. The Unix CI script now calls this shared helper instead of its independent mutable `curl` path; Windows callers inherit the same check. This is **confirmed and fixed**: before the change, mutable external test data could alter the trusted native test oracle without a repository diff or content-integrity failure; after the change, revision and bytes are bound before tests run.

Source-and-journal commit: `a0aa2a4d93685d72f604ac979ae4d5e94631bcc1` (`ci: verify pinned script test assets`), authored as `Lőrinc <pap.lorinc@gmail.com>`. Keep `pip install pyzmq`, vcpkg cache/tool inputs, compiler/SDK downloads, and the mutable fuzz-corpus clone as independently verified next candidates. Do not treat this fix as closing all `qa-assets` provenance.

# C/C++ Supply-Chain Cycle 154

## Cycle 154 Start: Tool Asset Integrity Audit

### Identity and Gate

- Cycle: `154`
- Draw command: `shuf -i 0-98 -n 1`
- First draw: `27` (closed error-path state cell; rerolled with the exact command)
- Selected draw: `59`
- Goal: `C/C++ supply-chain and security-gate audit`
- Slug: `cpp-supply-chain`
- Branch: `uber-cycle-154-cpp-supply-chain-20260730`
- Start HEAD: `2712998b621542ffdd6c6f2ba2b308af492a901e`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- `origin/master...HEAD` at the gate: `1090 42`
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Uber protocol SHA-256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Tracked/staged state at the gate: clean; all pre-existing untracked agent artifacts, catalog files, `node_modules/`, and `test/cache/` were preserved.
- PID `777094` (`test_bitcoin --run_test=wallet_tests`) and parent PID `725042` were observed and not touched.

### Scope and Prior Evidence

Cycle 119 closed mutable external GitHub Action references. Cycle 41/63 closed the release signature threshold and Guix cached-archive cells. This cycle reopens only a distinct executable-tool download surface and leaves container image tags, compiler/SDK downloads, generated test inputs, license gates, and release manifests queued.

The current `ci/lint/01_install.sh` installs ShellCheck and mlc as root during the trusted lint image build. The active root workflow at `.github/workflows/ci.yml:586-614` builds that image on the Bitcoin repository's trusted Warp runner and runs the resulting lint container. Before this cycle, the script used:

```text
curl --fail -L <ShellCheck release URL> | tar --xz -xf - --directory /tmp/
mv /tmp/shellcheck-v0.11.0/shellcheck /usr/bin/
curl --fail -L <mlc release URL> -o /usr/bin/mlc
chmod +x /usr/bin/mlc
```

The version/tag and HTTPS transport did not bind the bytes to an immutable digest. The repository's `doc/fuzzing-findings.md:1522-1526` records short IDs `48d6460315` and `09a2c19d8e` as fixes on a separate `audit/supply-chain` line. `git ls-remote` found no such branch or ref, GitHub's commit API returned HTTP 422 for both IDs, and the configured forks exposed no matching refs, so that note was treated as prior evidence rather than as an importable patch. Current history still contained the unverified install shape.

### Independent Verification

- Release API asset inventories showed ShellCheck v0.11.0 Linux `x86_64` and `aarch64` archives and mlc v1.2.0 `x86_64` and `aarch64` Linux binaries. Downloads to `/data/my_storage/tmp/cycle154-assets.04wJHq/` produced these exact bytes:

  ```text
  8c3be12b05d5c177a04c29e3c78ce89ac86f1595681cab149b65b97c4e227198  shellcheck-v0.11.0.linux.x86_64.tar.xz
  12b331c1d2db6b9eb13cfca64306b1b157a86eb69db83023e261eaa7e7c14588  shellcheck-v0.11.0.linux.aarch64.tar.xz
  7a72a93d5b3ee8a554cb840abdfe90aefb709418f225461b52021e3a058238a2  mlc-x86_64-linux
  01ec8e086f3b625616d461b63451be9175a02557de6b591bac7cde6791ab074b  mlc-aarch64-linux
  ```

- A deterministic scratch harness substituted a `curl` function that returned `attacker-controlled` bytes and a `tar` function that placed an attacker-controlled executable at the expected path. The old production command shape returned `old_pipeline_exit=0` and installed both payload markers. This proves the missing check is pre-use and is not dependent on a malformed archive.
- The patched verification helper accepted the downloaded x86_64 artifacts, extracted/installed them into a scratch directory, and executed `/data/my_storage/tmp/cycle154-postproof.*/shellcheck --version` and `mlc --version`, reporting ShellCheck and mlc v1.2.0. The same helper rejected a modified MLC file with `tampered_asset=rejected_before_install`.
- `bash -n ci/lint/01_install.sh`, ShellCheck v0.11.0 on the changed script, and `git diff --check` passed. Docker is unavailable in this environment, so the full `ci/lint.py` image build and the complete lint suite were not run.

### Fix

Replace the pipe-to-extraction and direct-to-`/usr/bin` downloads with architecture-specific SHA-256 pins for the two Linux architectures supplied by both tools. Download both assets into a private `mktemp` directory, verify the expected digest before extraction or installation, fail closed for unsupported architectures, and clean the directory on exit. `install --mode=755` preserves executable permissions without placing unverified bytes in the image.

### Verdict and Handoff

**Confirmed and fixed in the current worktree.** Before this fix, mutable GitHub release contents could execute in a trusted CI image without a repository-local integrity check. The source change is intentionally limited to lint-tool acquisition. The source-and-journal commit and separate uber-goal state close will record the exact hashes; leave compiler/SDK downloads, mutable image tags, qa-assets, and generated-input provenance in the next queue.

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
