# CI, Coverage-Bot, and Review-Bot Follow-up Audit

## Cycle 292: failed Windows PR follow-up

### Cycle identity and gate

- The exact selector shuf -i 0-98 -n 1 returned 42, selecting ci-bot-followup. No reroll was made. The dedicated branch is uber-cycle-292-ci-bot-followup-20260802.
- Cycle-start HEAD was 6b574d31651b5563839e061f0f01355713ef943a; origin/master was 556988790a7f961693a8fd93f73725baea66476a; merge-base was a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b; start divergence origin/master...HEAD was 45 1374; and the entry uber-state SHA-256 was e0f1cabf9f08f3c5b19659508eada7e3fce8a4e8535b523e0e595779f88ba4c0.
- The tracked and index tree was clean apart from the pre-existing untracked agent artifacts. Catalog, random prompt, goals TSV, and uber-protocol hashes matched 5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8, 10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec, babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb, and 954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0. All seven protected long-running processes were alive and untouched. New raw API responses and probes were stored only under /data/my_storage/tmp/cycle292-ci-bot.
- Prior Goal 42 cells excluded: static workflow/matrix reference checking, macOS GUI coverage follow-up, and vendored action SHA pinning. The new queue cell was a live failed check on an open PR, independently compared with current-master checks and the PR's review/bot evidence.

### Hypotheses and scope

1. A current open PR has a failed or unresolved CI check whose failure is caused by a source or build-contract regression rather than an opaque transient.
2. A review-bot or coverage-bot follow-up is present but not reflected in the changed source or test contract.
3. A Windows-specific change can compile under the repository's MSVC Unicode configuration and preserve the intended UTF-8 process boundary.

The trust boundary is the upstream pull request, GitHub check-run conclusions, review discussion, CoreCheck report, and the local CMake/source contract. External status is a seed; a source classification requires an independent local reproduction or a direct source/configuration proof. No current-branch production change is justified for an unmerged remote PR without an analogous defect in origin/master.

### Current check-run and review evidence

- The current origin/master commit 556988790a7f961693a8fd93f73725baea66476a returned 29 check runs: 28 success and the expected test ancestor commits skip. The Windows native, VS job 91386601965 and Windows native, fuzz, VS job 91386601949 were both successful.
- Open PR 35704, windows: remove deprecated codecvt via UTF-8 narrow APIs, had head 217926dfdf06dc0f0cf3f76ded7d9f4b73af9da2 and base 556988790a7f961693a8fd93f73725baea66476a. Its check-run endpoint returned 29 checks: 26 success, one skipped, and two failures. The failed jobs were Windows native, VS, job 91515361967, and Windows native, fuzz, VS, job 91515362008. Both stopped in the Build step; executable-manifest checks and tests were skipped.
- The failed check-run annotations contained only Process completed with exit code 1 and a pull-request-number notice. The unauthenticated GitHub job-log endpoint returned HTTP 403 with Must have admin rights to Repository, so the hosted compiler diagnostic was not treated as directly observed evidence.
- The PR review trail is consistent with the changed boundary. Review comment https://github.com/bitcoin/bitcoin/pull/35704#discussion_r3580156813 says the earlier Windows wide conversion should be reverted after the UTF-8 active-code-page change. Review comment https://github.com/bitcoin/bitcoin/pull/35704#discussion_r3580628463 asks whether a narrow execvp path can be used. A later review asks about CreateProcess and the PR head updates to the generic API in commit 217926d. The latest issue comment records that update, while the two Windows builds then fail.
- The CoreCheck report linked by DrahtBot is https://corecheck.dev/bitcoin/bitcoin/pulls/35704. Its latest report 3447 is status success with benchmark_status pending and no coverage data for the current head. It is not a failing coverage or benchmark gate and does not explain the Windows build failure.

### Independent source and compiler-contract reproduction

- The PR diff from origin/master FETCH_HEAD changes src/util/subprocess.h from CreateProcessW with a wchar_t command line and STARTUPINFOW semantics to generic CreateProcess with a char command line and generic STARTUPINFO. It also changes the Windows quote helper and ExecVp path to narrow strings, removes the codecvt deprecation suppression, and removes the HAVE__WSYSTEM probe. The current origin/master source still uses CreateProcessW.
- The repository's CMakeLists.txt explicitly adds _UNICODE and UNICODE to core_interface for MSVC at lines 325-326. Under the Windows SDK macros, generic CreateProcess therefore resolves to CreateProcessW and generic STARTUPINFO resolves to STARTUPINFOW.
- A minimal clang-cl-19 type reproduction was run with target x86_64-pc-windows-msvc, a synthetic Windows declaration, and UNICODE defined. The call used char command_line[2], generic STARTUPINFO, and generic CreateProcess. It failed with: no matching function for call to CreateProcessW; candidate not viable because char[2] cannot convert to wchar_t*. This reproduces the exact narrow/wide type incompatibility independently of the unavailable hosted log.
- A control with the same UNICODE condition but explicit CreateProcessA, STARTUPINFOA, and char command_line passed clang-cl-19 -fsyntax-only with exit 0. This is a minimal repair-direction proof, not a proposed local commit. Retaining the existing W implementation would also preserve the current wide contract; whether A or W is appropriate for the PR must be resolved by its author and Windows UTF-8 policy.
- The source/configuration check is reproducible from the checkout without a Windows SDK: git grep on origin/master finds only CreateProcessW in src/util/subprocess.h and the MSVC _UNICODE;UNICODE definition; git grep on FETCH_HEAD finds generic CreateProcess with the same definition. git diff --check origin/master FETCH_HEAD passed.

### Verdict

**Confirmed remote-only PR build defect; no current-tree finding.** The two Windows CI failures are source-correlated: the PR changes a wide Windows API call to a generic macro while the same build explicitly defines UNICODE, and the independent clang-cl type probe fails on the resulting W signature. Current master uses the explicit wide API and its corresponding Windows jobs pass. The hosted log remains inaccessible, so the exact CI compiler line is an inference from the source contract and the failed Build-step location, not a quoted remote diagnostic.

No source or permanent test change is justified on this branch because the defect exists only in unmerged PR 35704, not in origin/master or the local current source. The review/bot evidence did not reveal a separate current coverage or analyzer omission. The report-ready remote follow-up is the PR URL, both failed job URLs, the exact source diff, the UNICODE CMake line, and the minimal clang-cl reproduction above.

### Limitations and next queue

- No MSVC or Windows SDK is installed locally, so the full PR cannot be built here. The type-level clang-cl reproduction is narrower than a hosted Windows build.
- GitHub job logs require repository-admin access and returned HTTP 403. Check-run metadata, annotations, job-step boundaries, review comments, PR history, CoreCheck HTML, and local source contracts were available.
- CoreCheck's latest report has no coverage data and a pending benchmark status; it is not evidence that the changed process paths are covered.
- Do not patch the current tree for the unmerged PR. Reopen this CI cell only with a new head, a post-fix failed check, a current-master analogue, or accessible logs showing a different mechanism. The next cycle should draw a fresh goal and avoid repeating this PR's stale Windows source cell.

## Cycle 267: PR check failure and vendored workflow follow-up

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `42`
- Selected goal: `ci-bot-followup` (CI, coverage-bot, and review-bot follow-up audit)
- Branch: `uber-cycle-267-ci-bot-followup-20260802`
- Gate HEAD: `bb9a5319cd3638e269fba946343fe9cb8cd74d1a`
- `origin/master`: `556988790a7f961693a8fd93f73725baea66476a`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Gate divergence: local `1323`, upstream `45`
- The worktree had the expected pre-existing untracked artifacts. The protected long-running test processes remained alive. The catalog, prompt, TSV, protocol, and state hashes passed the gate.

### Hypotheses and scope

The live target was bitcoin/bitcoin#35762, `test: optionally run functional tests via CTest`, whose head is `b7ad3bff61c4e466ae6e16c605d6b0aef2e318e6` and base is `26b730cdbf2c77c12f684fd50bd376212b725394`.

1. Its failed RISC-V check might expose a source regression or an omitted CI follow-up.
2. The new CTest functional-test mode might silently violate an existing runner contract.
3. Vendored libsecp256k1 CI might still contain mutable external action references despite the earlier root CI pinning work.

The earlier Goal 59 root-workflow pinning was not repeated. The selected journal's prior Goal 42 cycles were also searched; they require a new failed/non-skipped check, workflow/test-list change, raw coverage evidence, review-bot comment, or reproducible job failure before reopening the same evidence cell.

### PR and CTest evidence

The PR has an unresolved `maflcko` review comment on `ci/test/00_setup_env.sh`: enabling CTest mode silently ignores `TEST_RUNNER_EXTRA`, leaving two ways to select functional tests. The PR itself documents the CTest limitations, including missing support for `--coverage`, `--resultsfile`, `--nocleanup`, `--tmpdirprefix`, interactive options, and arbitrary multi-argument test specifications.

The PR head was configured and built in an isolated scratch worktree with `BUILD_TESTS=ON`, `BUILD_FUNCTIONAL_TESTS=ON`, `BUILD_GUI=OFF`, wallet/ZMQ/bench disabled, and debug `-O0 -g0` flags. CMake configuration and the full `all` target completed successfully. CTest discovery produced 295 tests under the `functional` label. With `TEST_RUNNER_EXTRA='--exclude feature_fee_estimation.py'`, discovery still included `functional.feature_fee_estimation`; this is a confirmed CTest-mode contract gap on the PR branch, but it is not a current `origin/master` defect and the PR is not the local source being fixed in this cycle.

### RISC-V check investigation

The PR head check-runs endpoint returned 29 checks: 28 `success` and one `failure`, `riscv32 bare metal, static libbitcoin_consensus` (job `90826618769`, run `30046119080`). Its only failed step was `CI script` (step 11); setup, checkout, environment configuration, cache restore, Docker configuration, teardown, and completion all succeeded. The job page identifies the failure as `Process completed with exit code 1`, but both the raw job-log endpoint and the log-step page require authenticated access; the API returned HTTP 403 for logs and the unauthenticated HTML returned only a sign-in prompt.

The RISC-V environment file and `ci/test/link-riscv.sh` are unchanged between the PR head and `origin/master`. The PR's new `BUILD_FUNCTIONAL_TESTS` option defaults to `OFF`, and its `enable_testing()` condition remains enabled by the existing `BUILD_TESTS=ON` default, so the changed CMake logic is inert for this job. The PR base's same check passed (`89323958957`), and the same check on current master passed (`91386602059`). The PR had only one workflow run for this head. Therefore the failure is a remote, opaque, stale/transient CI result rather than a reproducible source regression. No RISC-V production or CI patch is justified.

### Confirmed nested CI configuration finding

Root `.github` action references had already been pinned by commit `85083424af`, but the vendored libsecp256k1 workflow still used mutable tags in three files: `src/secp256k1/.github/workflows/ci.yml`, `src/secp256k1/.github/actions/run-in-docker-action/action.yml`, and `src/secp256k1/.github/actions/install-homebrew-valgrind/action.yml`. These workflows are independently executable when the vendored subtree is tested, so the root-only inventory missed an applicable CI supply-chain boundary.

The source fix pins every external reference in that subtree to the verified commits below:

- `actions/checkout@fbc6f3992d24b796d5a048ff273f7fcc4a7b6c09`
- `actions/cache@caa296126883cff596d87d8935842f9db880ef25`
- `docker/setup-buildx-action@bb05f3f5519dd87d3ba754cc423b652a5edd6d2c`
- `docker/build-push-action@53b7df96c91f9c12dcc8a07bcb9ccacbed38856a`

`git diff --check` passed. A repository scan found seven external references in the nested workflow/action files and confirmed `all_external_refs_sha_pinned=true`. PyYAML parsing passed for the workflow and all three action files. Each four tag-to-commit mapping was independently checked with `git ls-remote` against the corresponding GitHub tag dereference. The diff is three files and seven replacements.

This does not make the Homebrew Valgrind source fully immutable: the action still fetches `LouisBrunner/valgrind/valgrind`, and that tap currently points at the mutable `main` branch of `valgrind-macos.git`. That residual is recorded rather than silently treated as fixed.

### Verdict

**Confirmed:** vendored libsecp256k1 CI had mutable external action references outside the root CI pinning inventory; the smallest source fix is the SHA-pinning patch described above.

**Dismissed as a current source regression:** PR #35762's RISC-V failure is isolated to an opaque remote `CI script` step; the unchanged RISC-V path passed on both the PR base and current master. The CTest `TEST_RUNNER_EXTRA` behavior is a real PR review-contract gap, but not a current-tree finding while the PR remains unmerged.

### Limitations and next queue

- GitHub denied unauthenticated raw logs, so the RISC-V command-level failure could not be independently minimized. The exact job, failed step, exit code, unchanged source paths, base result, and current-master result are preserved for a future authenticated follow-up.
- The vendored Homebrew action's mutable formula/source fetch remains a separate supply-chain lead; do not duplicate it as an action-pin finding.
- Commit the source fix with this journal, then commit the authoritative uber-goal cycle close separately and continue with a fresh gate and random draw.

## Cycle 244: macOS GUI follow-up verification

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `42`
- Selected goal: `ci-bot-followup` (CI, coverage-bot, and review-bot follow-up audit)
- Branch: `uber-cycle-244-ci-bot-followup-20260731`
- Gate HEAD before cycle work: `f11066832929628c9110d243af7266da417ec830`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence at the gate: local `42`, upstream `1271`
- The tracked/index tree was clean. The six protected long-running test processes remained alive. The catalog, prompt, TSV, and uber-goal protocol hashes passed the pre-cycle gate.

### Hypotheses

1. The recent `qa: Enable interface_gui.py on macOS` change might be present in source but omitted, skipped, or misconfigured in the macOS CI matrix.
2. The change might have left an unresolved CI failure or review-bot follow-up after its first macOS failure.
3. CoreCheck might report a relevant coverage loss or benchmark regression requiring a test or implementation follow-up.

### CI wiring and source evidence

The upstream change was inspected at `45f5609f2ed9e35ae2a109c1e8a7c085e0a78006` and its merge `67efced1fc83a0b7215cc1513e7c4754fee0f12f`. The standard macOS matrix entry uses `ci/test/00_setup_env_mac_native.sh`, whose `--preset=dev-mode` configuration leaves GUI enabled, and the common setup defaults `RUN_FUNCTIONAL_TESTS=true`. The fuzz matrix entry intentionally disables functional tests. The common script invokes the build-directory `test/functional/test_runner.py` when that gate is true.

`interface_gui.py` is explicitly present in `BASE_SCRIPTS` in `test/functional/test_runner.py`, not only in the extended list. Its `skip_if_no_gui()` check is satisfied by the standard macOS GUI build; only Windows remains an explicit platform skip. The test framework selects the GUI wrapper (`bitcoin -m gui` when `BITCOIN_CMD="bitcoin -m"`) and, for GUI nodes on Darwin, sets `QT_QPA_PLATFORM=minimal` and defaults `QT_STYLE_OVERRIDE=fusion`. The latter is the relevant fix for the Cocoa/minimal-platform crash.

The earlier PR discussion supplied a concrete failure, not a speculative lead: `fanquake` reported `interface_gui.py` exiting with status `-11` on macOS when built using depends. The follow-up added the Fusion style workaround, and `maflcko` ACKed the resulting commit. This history explains why the change is a CI repair rather than an unresolved omission.

### Live checks and bot evidence

The GitHub check-runs endpoint for merge `67efced1fc83a0b7215cc1513e7c4754fee0f12f` was queried on 2026-07-31:

```text
total 29
28 executed checks: success
1 check: test ancestor commits, skipped
macOS native: completed success
macOS native, fuzz: completed success
```

The macOS native job `90858907736` completed every reported step successfully, including `CI script`. Its unauthenticated log-download endpoint returned HTTP 403, so raw job-log text was not treated as available evidence; the job-step API, source wiring, and test-list inspection were used instead. The commit-status endpoint reported zero status contexts, so check-runs were the applicable GitHub result source.

CoreCheck report 3393 for PR #35838 (`https://corecheck.dev/bitcoin/bitcoin/pulls/35838`) reported `status=success` and `benchmark_status=success` for commit `45f5609f2ed9e35ae2a109c1e8a7c085e0a78006`, with base commit `9611a356035be531d62bfc40879f388d5dc359c4`. It had no coverage data for new code and no actionable coverage annotation. The report’s listed lost/gained baseline files were unrelated to the two-file PR, and no benchmark table entry was present. An earlier report failed because an essential container exited, but the replacement report was successful and the PR merged only after the fix.

The PR API showed the final state as closed/merged with no unresolved review-bot objection. Human review explicitly documented the initial crash, requested the Fusion-style workaround, and then ACKed the fixed commit. The merge check matrix independently passed afterward.

### Verdict

**Dismissed as a current CI, coverage-bot, or review-bot omission; no confirmed finding.** The only concrete failure found was the macOS GUI crash that motivated the follow-up and was fixed before merge. The current matrix builds the GUI and runs the functional suite on standard macOS, the test is in that suite, the Darwin environment fix is present, and the live job/check/CoreCheck evidence is successful. No repository change is justified by this cycle.

### Limitations and next queue

- The full macOS CI job was not run locally because this Linux environment cannot reproduce the hosted macOS runner and its Cocoa/Qt stack.
- GitHub denied unauthenticated raw job-log downloads with HTTP 403, so the exact individual `interface_gui.py` pass line was not extracted from the hosted log. Source-level inclusion plus the successful standard macOS job and the resolved failure/review trail are the available independent evidence.
- CoreCheck reported no line-level coverage data for this two-file change; this is a monitoring limitation, not evidence of a coverage defect.
- Reopen Goal 42 only with a new workflow or test-list change, a failed/non-skipped check, a new review-bot comment, raw coverage evidence tied to the changed path, or a reproducible macOS GUI failure.
- Continue the uber loop by drawing another eligible catalog goal and recording its fresh gate.

## Cycle 143: live bot and coverage follow-up

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `42`
- Selected goal: `ci-bot-followup` (CI, coverage-bot, and review-bot follow-up audit)
- Branch: `uber-cycle-143-ci-bot-followup-20260730`
- Gate HEAD before branch creation: `2118a502f1daa581190ecbad17824d03366638b9`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence at the gate: local `40`, upstream `1070`
- The tracked/staged tree and `git diff --check` were clean. The unrelated long-running wallet test process with PID `777094` remained alive and was not touched.

The previous Goal 59 cycle already independently covered the local commit `85083424af` (`ci: pin external action references`). Its action-reference pinning was excluded from this cycle to avoid duplicating that finding.

### Hypotheses

1. A current workflow, matrix entry, or runner gate could silently omit the check named by the job.
2. A live coverage or review bot could have reported a follow-up that the local journal did not contain.
3. A current CoreCheck performance or coverage anomaly could be attributable to the merged LineReader PR and require a follow-up.

### CI and live bot evidence

The current tree was checked with the workflow and runner inventory. The scan found 8 workflow jobs, 20 external action references, 33 shell entrypoints, and 6 CI Python entrypoints. Every external action reference was a full 40-hex SHA. `bash -n` passed for all shell entrypoints, `python3 -m py_compile` passed for all CI Python files, and the workflow parsed with PyYAML. Every workflow-referenced setup environment existed, and no `FILE_ENV` reference pointed at a missing file.

The latest upstream merge commit `9611a356035be531d62bfc40879f388d5dc359c4` was queried through the GitHub API on 2026-07-30:

- Check-run endpoint: `https://api.github.com/repos/bitcoin/bitcoin/commits/9611a356035be531d62bfc40879f388d5dc359c4/check-runs`
- 29 check runs were returned. The 28 executed CI, sanitizer, fuzz, analyzer, cross-build, platform, lint, and release checks were successful.
- The only non-success conclusion was the expected `test ancestor commits` skip, which is conditional on a multi-commit pull request.
- The commit-status endpoint had no separate status contexts (`total=0`), so check-runs are the relevant result source.

The corresponding PR was merged as bitcoin/bitcoin#35828:

- PR: `https://github.com/bitcoin/bitcoin/pull/35828`
- `pinheadmz` approved; `furszy` left an ACK comment; there were no review-code comments.
- The PR's issue comments were bot metadata and ACKs only. No unresolved review-bot objection or failed check was found.

### CoreCheck verification

CoreCheck report 3332 for PR #35828 was successful: `https://corecheck.dev/bitcoin/bitcoin/pulls/35828`. It compared PR commit `dff44e4c8f` with base report commit `b33a7fcd`.

The report listed no coverage data for uncovered new code, listed covered new code in `src/httpserver.cpp`, `src/torcontrol.cpp`, and `src/util/string.cpp`, and listed lost baseline coverage in `src/addrman.cpp`, `src/torcontrol.cpp`, `src/util/sock.cpp`, `src/util/string.h`, `src/validation.cpp`, and `src/wallet/spend.cpp`. Four of those six files were untouched by the PR. The two touched files do not, by themselves, establish a behavior or test omission; the report had no failing coverage gate or actionable annotation.

The benchmark section showed `OrphanageEraseForPeer` at `+22.70% ns/op`, `+21.78% cycles`, and `-18.50% op/s`. This is not attributable to the PR: `git show b33a7fcd:src/bench/txorphanage.cpp` and `git show dff44e4c8f:src/bench/txorphanage.cpp` produced identical hashes, and the PR diff contained no benchmark change. The result is therefore a bot measurement signal to monitor, not a source regression or justified optimization commit.

### Shell/path follow-up

The current path-space hardening remains wired through `BASE_BUILD_DIR` and the runner quotes its filesystem uses. The remaining unquoted `source $FILE_ENV` and GitHub environment-file redirects use the repository's fixed matrix filenames and runner-provided temporary path; no current input allows an attacker-controlled space-bearing `FILE_ENV` or `GITHUB_ENV` path. Treating these lines as a defect would be speculative and would duplicate the existing path-space work.

### Verdict

**Dismissed as a current CI, coverage-bot, or review-bot omission; no confirmed finding.** The live upstream checks were green, the current orchestration inventory was internally consistent, and the only reported CoreCheck benchmark regression was on unchanged benchmark source. No production, test, or CI change is justified by this cycle.

### Limitations and next queue

- The full containerized CI job was not run locally; its script intentionally requires the CI container contract.
- CoreCheck did not provide raw coverage data for independent line-level replay, so the coverage movement was classified as non-actionable rather than proved absent.
- The live evidence covered the latest upstream merge, not a newly submitted local branch. The local action pinning was already handled by Goal 59.
- Reopen Goal 42 only with a new workflow change, failed/non-skipped check, raw coverage artifact showing a relevant regression, review-bot comment, or reproducible job failure.
- Continue the uber loop by drawing another eligible catalog goal and recording its fresh gate.

## Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `42`
- Selected goal: `ci-bot-followup` (CI, coverage-bot, and review-bot follow-up audit)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD: `1dcc2da988ee625fbc5d7d55eb6f894c1103ec52`
- Catalog: `agent-journal/reusable-continuous-agent-goals.md`

## Scope and Hypothesis

The selected goal asks for CI failures, coverage comments, analyzer reports, review-bot output, stale suppressions, and follow-up omissions. This bounded cycle focused on a concrete static hypothesis: a current CI matrix entry may reference a missing setup script, or a runner gate may be silently unconfigured and skip the checks that its job name implies.

The trust boundary is the repository's CI orchestration. A confirmed defect would be a dangling workflow reference, a syntax-invalid entrypoint, or a matrix job whose configured gate is not consumed by `ci/test/03_test_script.sh`. Review comments and external check-run state were not treated as available evidence because this cycle had no online status snapshot.

## Evidence

The workflow references 22 setup environments. The following command checked every distinct `ci/test/00_setup_env_*.sh` path found under `.github` and `ci`:

```text
grep -Rho 'ci/test/00_setup_env_[A-Za-z0-9_]*\.sh' .github ci | sort -u | while read -r f; do test -f "$f" && printf 'OK %s\n' "$f" || printf 'MISSING %s\n' "$f"; done
```

All 22 paths were present: ARM, macOS native and cross, Windows, i686, the native ASan/MSan/TSan/fuzz/tidy/IWYU/previous-release/no-wallet jobs, Alpine, the BSD cross jobs, and bare-metal RISC-V. No `MISSING` line was produced.

The entrypoints passed syntax checks:

```text
bash -n ci/test/03_test_script.sh ci/test_run_all.sh
python3 -m py_compile .github/ci-test-each-commit-exec.py .github/ci-windows.py .github/ci-windows-cross.py
```

Result: `syntax=ok` and exit status 0.

The common setup file defaults `RUN_UNIT_TESTS=true`, `RUN_FUNCTIONAL_TESTS=true`, `RUN_TIDY=false`, `RUN_FUZZ_TESTS=false`, and `GOAL=install`. Specialized jobs explicitly override the relevant gates: fuzz jobs disable unit and functional tests and enable fuzzing; IWYU and tidy jobs disable unrelated tests and enable their analyzer; cross and deploy jobs disable runtime tests; and the bare-metal RISC-V job selects its three supported build targets and invokes `link-riscv.sh`. The runner consumes these gates in the expected build, unit-test, functional-test, tidy, IWYU, dependency-check, Valgrind, fuzz-input, and bare-metal branches.

Recent history contains the CI path-space follow-up (`a31c30290`, merged as part of the current history), and the current runner still carries the intentional dummy default-datadir guard and the path-with-space environment. No stale reference or accidental omission was found in this matrix slice.

## Review and History Search

The local history search covered recent commits mentioning CI, coverage, review bots, and follow-up. It found current CI maintenance merges and the path-space hardening follow-up, but no local unresolved bot finding that contradicted the present matrix. GitHub check runs, coverage-bot comments, and review-bot discussions were not fetched in this cycle, so their absence here is not evidence that no remote follow-up exists.

## Verdict

**Dismissed as a current CI matrix or entrypoint omission; no confirmed finding.** Every workflow-referenced environment script exists, both shell and Python entrypoints parse, and the inspected feature gates map to explicit job settings or common defaults. No production or CI change is justified by this cycle.

## Limitations and Rejected Leads

- The full containerized CI job was not run locally; `ci/test/03_test_script.sh` intentionally refuses host execution without its container contract and can make unsafe global changes.
- No live GitHub check-run, coverage-bot, review-bot, or issue state was available in the local checkout.
- This did not prove analyzer findings, sanitizer output, or coverage quality; it only checked orchestration references and gate wiring.
- The remaining high-value follow-up is to fetch current check-run/review data or inspect a specific failing job, then compare the reported status with the exact matrix environment and logs.

## Next Queue

1. Draw another eligible catalog goal and record the exact command and draw before work.
2. Reopen this goal only with new CI configuration, a live bot/check-run report, a changed runner contract, or a reproduced job failure.
3. Keep the matrix-reference check as a reusable preflight for future CI edits.
