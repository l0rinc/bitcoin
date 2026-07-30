# Loupe-Style Scout, Verifier, Fixer, and Reporter Pipeline

## Cycle 112: release verifier work-directory ownership

### Gate and scope

- Selected by exact `shuf -i 0-98 -n 1` -> `63` (`loupe-pipeline`).
- Branch: `uber-cycle-112-loupe-pipeline-20260729`.
- Cycle-start HEAD: `b7e9fff692afc1fcfd520f117e6ec0b9573b15f2`.
- Base: `origin/master` at `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` is `40 1013`.
- Catalog SHA-256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber prompt SHA-256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Goals TSV SHA-256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- The fresh gate passed fetch, tracked/index cleanliness, `git diff --check`, catalog hashes, and process checks. PID `777094` (`wallet_tests`, parent `725042`) was preserved.

Cycle 85's `verify-commits` ancestry-status finding is excluded. This cycle uses a distinct scout/verifier/fixer/reporter pipeline on `contrib/verify-binaries/verify.py`, specifically `verify_published_handler()`'s predictable `/tmp/bitcoin_verify_binaries.<version>` directory and fixed `wget -O` output names. The trust boundary is a release operator sharing a host with another local user or process; the protected artifact is the verifier's downloaded, later hash-checked release file and checksum metadata.

### Scout receipt and deduplication

The Codex Security raw candidate `published-release-predictable-workdir` and `investigation-report.md` proposed a symlink-sentinel reproduction but explicitly marked reproduction pending and remediation uncommitted. Exact and semantic searches found that candidate only in the security evidence plus this cycle's selected journal; the prior `cpp-supply-chain` cycle fixed trusted-signature counting and did not cover filesystem ownership. The candidate is therefore a seed, not proof or an already accepted finding.

Scout hypothesis: `verify_published_handler()` accepts a pre-existing version-derived directory (`os.makedirs(..., exist_ok=True)`), changes into it, and invokes `wget -O` on attacker-predictable filenames. A local actor able to create a symlink at one output name can redirect the verifier's write before signature/hash verification, modifying a file outside the work directory. A confirmed finding requires a real pre-fix trace, applicability under the actual code path, an independent verifier receipt, and a minimal fix that owns the staging directory.

### Initial pipeline queue

1. Reproduce the scout with a deterministic local `file://` source, real `wget`, a pre-created output symlink, and a sentinel outside the work directory.
2. Independently verify the source path, return code, downloaded content, and sentinel mutation without relying on the security artifact's report.
3. Compare the candidate with `verify_binaries_handler()`'s local-file path, cleanup behavior, platform assumptions, and history before deciding scope.
4. If confirmed, fix private work-directory creation and cleanup with the smallest regression test; then rerun the pre-fix PoC, after-fix guard, syntax/test controls, and final report checks.

### Scout and verifier receipts

The scout probe `agent-journal/loupe_cycle112_probe.py` used a loopback HTTP server, the real `wget -O` implementation, a pre-created `/data/my_storage/tmp/cycle112-loupe-pre-fix/tmp/bitcoin_verify_binaries.31.0/release.bin` symlink, and a sentinel outside that directory. It stubbed only the signed metadata fetch and GPG result, so the production handler still parsed the checksum, downloaded the payload, and verified the hash. Before any source edit, the handler returned `status=0`, printed `VERIFIED: release.bin`, left the output as a symlink, and changed the sentinel from `sentinel-before` to the release payload.

The independent verifier `agent-journal/loupe_cycle112_verifier.py` repeated the handler path with a different work root, payload, sentinel, and loopback server. It independently recorded `status=0`, the external sentinel payload, and the symlink target. The 404 fixture attempt was rejected as a harness path error; its truncation side effect is retained only as a note, not as evidence. The corrected successful receipt is the authoritative verifier result. Exact/hash deduplication found no prior journal or source fix for this filesystem-ownership path; the security artifact remained an unverified seed.

Applicability is confirmed: `verify_published_handler()` creates the version-derived directory with `exist_ok=True`, `chdir`s into it, and passes fixed relative output names to `wget -O`; no owner, exclusivity, or symlink defense intervenes before hash verification. The local attacker needs the ability to pre-position entries in the verifier's temporary directory, so impact is a same-host release-integrity boundary rather than a remote node vulnerability. The verifier authorized a minimal fixer change: replace the predictable shared directory with `tempfile.mkdtemp(prefix="bitcoin_verify_binaries.")`, then add a deterministic unit regression proving a pre-existing predictable symlink is not touched.

### Fixer and final reviewer receipts

- The fixer changed `verify_published_handler()` to allocate its staging directory with `tempfile.mkdtemp(prefix="bitcoin_verify_binaries.")` after version parsing. This preserves the previous no-directory behavior for malformed versions and gives each valid invocation a newly created private directory. The existing cleanup path and `--cleanup` behavior remain unchanged.
- `contrib/verify-binaries/test.py` now pre-creates the old predictable directory and a symlinked `release.bin`, stubs only metadata/signature/download boundaries, and asserts the external sentinel is unchanged while the private workdir contains the verified payload. The focused regression passed:

      python3 -c 'import importlib.util; s=importlib.util.spec_from_file_location("verify_test", "contrib/verify-binaries/test.py"); m=importlib.util.module_from_spec(s); s.loader.exec_module(m); m.test_published_verifier_uses_private_workdir(); print("focused workdir regression passed")'
      [INFO] downloading release.bin to /tmp/verify-binaries-workdir-test.3fpwwyax/private-workdir
      VERIFIED: release.bin
      focused workdir regression passed

- An independent post-fix loopback-HTTP verifier, using a different root, payload, sentinel, and output name, passed with `--expect-protected`: `status=0`, `sentinel_after=b'independent-before\\n'`, `output_is_symlink=True`, and the symlink still resolved to the outside sentinel. `python3 -m py_compile contrib/verify-binaries/verify.py contrib/verify-binaries/test.py agent-journal/loupe_cycle112_probe.py agent-journal/loupe_cycle112_verifier.py` also passed.
- The full `python3 contrib/verify-binaries/test.py` run reached the new private-workdir test and passed its earlier checks, but stopped in the pre-existing 22.0 published-release test because this environment has zero trusted historical release signer keys. It emitted `JSONDecodeError` after `not enough trusted sigs to meet threshold (0 vs. 3)`; this is an external key-fixture limitation, not a regression in the new test. The focused regression and independent verifier are the actionable validation receipts.
- Final review checked the source/test diff, `git diff --check`, parse-failure directory timing, exact/semantic deduplication, and preserved unrelated dirty state. Status transitions are `scouted -> independently verified -> fixed -> independently reviewed -> reported`. The candidate is confirmed and reportable as a same-host local release-integrity issue; no remote P2P impact is claimed.

## Cycle 85 start

- Selected by the uber loop: exact `shuf -i 0-98 -n 1` -> `63` (`loupe-pipeline`).
- Branch: `uber-cycle-85-loupe-pipeline-20260729`.
- Cycle-start HEAD: `eb9a35d13935d67f56d0a967eca598dfe648bae9` (`journal: close deterministic simulation cycle 84`).
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` is `2 956`.
- Dirty state: only preserved untracked agent/user artifacts are present; no tracked edits at initialization. No relevant process is running.
- Catalog/protocol/manifest hashes: catalog `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Scope: separate scouting, independent verification, fixing, and reporting with leases, regression PoCs, applicability checks, semantic/hash deduplication, and final review evidence.
- Exclusions: do not repeat the closed cycles' individual source cells, and do not treat the pre-existing Codex Security documentation artifacts as proof of a current repository defect. Their candidate ledgers remain provenance to inspect, not an already verified pipeline.

## Campaign contract

Every candidate must have a trust boundary, contract/invariant, source and history evidence, a reproducible discovery artifact, an independent verifier verdict before fixing, a deduplication result, and a final report/commit trail. A scout may produce a PoC or exact fixture but must not silently patch the candidate. A verifier must be able to reject the candidate and must record applicability, reachability, expected behavior, and the first invalid operation or failed oracle. A fixer may act only on a confirmed local finding. A reporter must preserve the raw evidence, limitations, and review rationale.

## Initial hypotheses

1. Existing agent/security ledgers may mix discovery and verification statuses, allowing a candidate to be treated as confirmed without an independent reproducible verdict.
2. Hash/semantic deduplication may collapse distinct recurrence or changed-reachability findings, or fail to connect prior finding fingerprints to current candidates.
3. A report/commit may omit the PoC, verifier receipt, applicability result, or exact source/HEAD, making the final finding non-reproducible even when the code fix is correct.
4. Leases or ownership markers may be absent or advisory, allowing parallel scouts to duplicate work or a fixer to act before verification.

## Initial queue and protocol

- Inventory `agent-journal` finding ledgers, `doc/security` scan artifacts, candidate/review receipts, source inventories, regression PoCs, commit messages, and any lease/ownership mechanism.
- Trace a small number of completed and rejected findings end to end from discovery through independent verification, source fix or dismissal, final report, and journal/state update.
- Search prior findings by exact hash, symbol/path, semantic summary, and commit/PR provenance before selecting a new pipeline cell.
- Choose the highest-risk unproven pipeline contract, create or reuse the smallest deterministic fixture, and record whether the defect is in repository code, test/harness, documentation, tool, dependency, or process evidence.

## Cycle 85 verification: `verify-commits` ancestry error

### Scout receipt and deduplication

The round-two security shard at `doc/security/codex-security-bitcoin-round2/shard_00.jsonl`
reported the candidate `verify-commits-ancestry-error-success`. Its independent
triage overview, `overview.md` rows 11 and 83-98, explicitly recorded the
reproducer and requested a return-code distinction plus a regression. The scout
produced evidence only and did not modify the source.

The trust boundary is the release/CI operator invoking `contrib/verify-commits/verify-commits.py`
against a candidate revision and a Git object database. A missing or otherwise
unresolvable candidate must never be reported as successfully verified. A valid
`git merge-base --is-ancestor` result has status 0 for ancestor and 1 for
non-ancestor; status 128 is an operational error and must fail closed.

Exact and semantic searches found this instance only in the round-two shard and
overview, with no earlier journal finding or separate candidate fingerprint for
the same path and bug shape. The first-round artifacts contain only inventory
and broad review coverage for this file, not a prior verifier receipt or fix.

### Independent verifier result before fixing

On clean current Cycle 85 source, before the patch:

    python3 contrib/verify-commits/verify-commits.py this-object-does-not-exist
    status=0
    "this-object-does-not-exist" predates the trusted root, stopping!

The underlying command returned status 128 and printed `Not a valid object
name`. The verifier therefore exited before `git verify-commit`, tree hashing,
or parent checks. This independently reproduced the scout's claimed behavior
on the current checkout. History shows the broad nonzero check was introduced
by `bb86887527` (`verify-commits: Skip checks for commits older than trusted
roots`); no regression test for Git's status-1-versus-error distinction exists.

### Fix and regression evidence

The verified candidate was confirmed as a local release-integrity tool defect.
Both trusted-root ancestry checks now use `is_ancestor`, which accepts only
statuses 0 and 1 and exits 1 for every other status. The new executable
`contrib/verify-commits/test.py` independently checks mocked statuses 0, 1,
and 128, then invokes the real script with the missing revision. The focused
command passed:

    python3 contrib/verify-commits/test.py
    Ran 3 tests ... OK

After the fix, the real missing-revision invocation returned status 1 and
reported `git merge-base --is-ancestor failed with status 128`; no success
message was emitted. `git diff --check` passed. The README documents the
regression command.

The final report limitation remains that ordinary CI normally supplies an
existing ref, so practical impact requires an incomplete object database,
invalid candidate ref, or another invocation path that reaches the verifier
with an unresolved revision. This is nevertheless a confirmed fail-open
contract violation in security verification tooling, not a remote P2P finding.

Repository-wide `test/lint/lint-files.py` was also run and reported five
pre-existing shebang-permission failures in unrelated files; `lint-python.py`
was skipped because `lief` is unavailable. Those files were not changed.
